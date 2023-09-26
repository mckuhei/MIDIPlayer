#include <stdlib.h>
#include <string.h>
#include "player.h"
#include "util.h"

struct ChannelStatus
{
    FILE *fp;
    int32_t next;
    uint8_t *nextEventData;
    int nextEventLength;
    int lastStatus;
    bool completed;
};


struct PlayerImpl_;

typedef struct PlayerImpl_ PlayerImpl;

struct PlayerImpl_
{
    MPRESULT (*Dispose) (PlayerImpl *player);
    MPRESULT (*Play) (PlayerImpl *player);
    MidiOutInterface *midiOut;
    FILE *fp;
    int16_t tracks;
    int16_t resolution;
    int32_t totalTicks;
    long *startPos;
};

inline bool IsVaildStatus(uint8_t status) {
    return (status & 0x80) != 0;
}

inline int getDataLen(int status) {
    switch(status & 0xF0) {
    case NOTE_OFF: // Note off
    case NOTE_ON: // Note on
    case POLY_PRESSURE: // Polyphonic Aftertouch
    case CONTROL_CHANGE: // Control Change
    case PITCH_BEND: // Pitch blend
        return 2;
    case PROGRAM_CHANGE: // Program Change
    case CHANNEL_PRESSURE: // Channel Aftertouch
        return 1;
    }
    return 0;
}

static bool ReadEvent(struct ChannelStatus *ch) {
    if(ch->completed) return false;
    FILE *fp = ch->fp;
    if(!ReadVarInt(fp, &ch->next)) {
        ch->completed = true;
        return false;
    }
    uint8_t status;
    int8_t data, data2;
    if(!ReadByte(fp, (int8_t *) &status)) {
        return false;
    }
    if(!IsVaildStatus(status)) {
        data = (int8_t) status;
        status = ch->lastStatus;
    } else {
        ch->lastStatus = status;
        ReadByte(fp, &data);
    }
    uint8_t *dat = ch->nextEventData;
    int32_t len;
    switch (status)
    {
    case 0xF7:
    case 0xF0:
        ReadVarInt2(fp, &len, data);
        *dat = (int8_t) status;
        // printf("sysex: %d\n", len);
        fread(dat + 1, 1, len++, fp);
        break;
    case 0xFF:
        ReadVarInt(fp, &len);
        *dat++ = 0xFF;
        *dat++ = (int8_t) data;
        // printf("meta: %d\n", len);
        fread(dat, 1, len, fp);
        dat -= 2;
        len += 2;
        if(data == 0x2F) {
            ch->completed = true;
            return false;
        }
        break;
    default:
        len = 3;
        if(getDataLen(status) == 2) {
            ReadByte(fp, &data2);
        } else {
            data2 = 0;
        }
        *dat++ = (int8_t) status;
        *dat++ = (int8_t) data;
        *dat++ = (int8_t) data2;
        dat -= 3;
        break;
    }
    ch->nextEventLength = len;
    // printf("A: ");
    // for(int i = 0; i < len; i++) {
    //     printf("%02X ", *(dat + i));
    // }
    // printf("\n");
}

static MPRESULT Play(PlayerImpl *player) {
    int16_t tracks = player->tracks;
    struct ChannelStatus status[tracks];
    memset(status, 0, sizeof(struct ChannelStatus) * tracks);
    for (int id = 0; id < tracks; id++) {
        status[id].fp = DuplicateFileHandle(player->fp);
        fseek(status[id].fp, player->startPos[id] + 8, SEEK_SET);
        status[id].nextEventData = malloc(65536);
        ReadEvent(&status[id]);
    }

    MidiOutInterface *midiOut = player->midiOut;
    int16_t resolution = player->resolution;
    int32_t TPQN = 500000;
    int32_t sleepUs = (long) (((double) TPQN / (double) resolution)), sleep;
    uint64_t targetUs = GetTimeUS();
    int32_t currentTick = 0, totalTicks = player->totalTicks;
    struct ChannelStatus *ch;
    while (currentTick <= totalTicks) {
        targetUs += sleepUs;
        for(int i = 0; i < tracks; i++) {
            ch = &status[i];
            for(; !ch->completed && ch->next == 0; ReadEvent(ch)) {
                uint8_t *data = ch->nextEventData;
                uint8_t status = *data;
                if(status == 0xFF) {
                    switch (data[1]) {
                    case 0x51:
                        TPQN = (data[2] << 16) | (data[3] << 8) | data[4];
                        sleepUs = (long) (((double) TPQN / (double) resolution));
                        break;
                    
                    default:
                        break;
                    }
                } else if(status == 0xF7) {
                    midiOut->WriteLongMessage(midiOut, data + 1, ch->nextEventLength - 1);
                } else if(status == 0xF0) {
                    midiOut->WriteLongMessage(midiOut, data, ch->nextEventLength);
                } else {
                    midiOut->WriteShortMessage(midiOut, data[0], data[1], data[2]);
                }
            }
            ch->next--;
        }
        // getchar();
        sleep = (targetUs - GetTimeUS());
        // printf("sleep: %d\n", sleep);
        if(sleep > 0) SleepUS(sleep);
        currentTick++;
    }
    

    for (int id = 0; id < tracks; id++) {
        free(status[id].nextEventData);
        fclose(status[id].fp);
    }
    return MIDIPLAYER_NOERROR;
}

static MPRESULT Dispose(PlayerImpl *player) {
    fclose(player->fp);
    free(player->startPos);
    free(player);
    return MIDIPLAYER_NOERROR;
}

MPRESULT CreateMidiPlayer(Player **playerPtr, FILE *fp, MidiOutInterface *ifc) {
    long filebase = ftell(fp);
    char mark[4];
    if(fread(mark, 1, 4, fp) != 4) return MIDIPLAYER_EOF;
    if(memcmp(mark, "MThd", 4)) {
        return MIDIPLAYER_NOT_SMF;
    }
    int32_t headerSize;
    int16_t midiType, tracks, resolution;
    if(!ReadInt(fp, &headerSize) || !ReadShort(fp, &midiType) || !ReadShort(fp, &tracks) || !ReadShort(fp, &resolution)) return MIDIPLAYER_EOF;
    if(midiType < 0 || midiType > 1 || (resolution & 0x8000) != 0) return MIDIPLAYER_NOT_SUPPORT;
    fseek(fp, headerSize - 6, SEEK_CUR);
    long offset = ftell(fp);
    long trackStart[tracks];
    long totalTicks = 0, totalTick;
    for(int id = 0; id < tracks; id++) {
        trackStart[id] = offset;
        fseek(fp, offset, SEEK_SET);
        if(fread(mark, 1, 4, fp) != 4) return MIDIPLAYER_EOF;
        if(memcmp(mark, "MTrk", 4)) {
            return MIDIPLAYER_NOT_SMF;
        }
        int trackSize;
        if(!ReadInt(fp, &trackSize)) return MIDIPLAYER_EOF;
        offset += 8 + (trackSize & 0xFFFFFFFFL);
        int32_t delta;
        uint8_t status, lastStatus = 0x00;
        int8_t data;
        totalTick = 0;
        while(true) {
            if(!ReadVarInt(fp, &delta) || !ReadByte(fp, (int8_t *) &status)) return MIDIPLAYER_EOF;
            totalTick += delta;
            if(!IsVaildStatus(status)) {
                data = (int8_t) status;
                status = lastStatus;
            } else {
                lastStatus = status;
                if(!ReadByte(fp, (int8_t *) &data)) return MIDIPLAYER_EOF;
            }
            int32_t datalen;
            switch (status)
            {
            case 0xF7:
            case 0xF0:
                if(!ReadVarInt2(fp, &datalen, data)) return MIDIPLAYER_EOF;
                fseek(fp, datalen, SEEK_CUR);
                break;
            case 0xFF:
                if(!ReadVarInt(fp, &datalen)) return MIDIPLAYER_EOF;
                fseek(fp, datalen, SEEK_CUR);
                if(data == 0x2F) {
                    goto end;
                }
                break;
            default:
                if(getDataLen(status) == 2) if(!ReadByte(fp, (int8_t *) &data)) return MIDIPLAYER_EOF;
                break;
            }
        }
        end:
        if(totalTick > totalTicks) totalTicks = totalTick;
    }
    PlayerImpl *player = malloc(sizeof(PlayerImpl));
    player->midiOut = ifc;
    player->fp = fp;
    player->totalTicks = totalTicks;
    player->resolution = resolution;
    player->tracks = tracks;
    player->startPos = calloc(tracks, sizeof(long));
    memcpy(player->startPos, trackStart, sizeof(long) * tracks);
    player->Dispose = Dispose;
    player->Play = Play;
    *playerPtr = (Player *) player;
    return MIDIPLAYER_NOERROR;
}