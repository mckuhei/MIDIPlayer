#include <string.h>
#include "player.h"
#include "util.h"

static void ShowUsage(const char *filename) {
    printf("%s <midi> [-dev Device name]", filename);
}

int main(int argc, char const *argv[])
{
    const char *filename = NULL;
    const char *devname = "Microsoft MIDI Mapper";
    for(int i = 1; i < argc; i++) {
        if(strcmp("-dev", argv[i]) == 0) {
            devname = argv[++i];
            continue;
        }
        if(filename == NULL) {
            filename = argv[i];
        }
    }
    if(filename == NULL) {
        ShowUsage(argv[0]);
        return 0;
    }
    MidiOutInterface *midi = FindMidiDevices(devname);
    if(midi == NULL) {
        fprintf(stderr, "Error: Unable to open device: %s", devname);
        return -1;
    }
    FILE *fp = fopen(filename, "rb");
    if(fp == NULL) {
        fprintf(stderr, "Error: Unable to open file: %s", filename);
        return -1;
    }
    Player *player;
    MPRESULT result = CreateMidiPlayer(&player, fp, midi);
    if(result == MIDIPLAYER_NOERROR) {
        result = player->Play(player);
        player->Dispose(player);
    }
    printf("%d\n", result);
    // midi->WriteLongMessage(midi, "\xF0\x7E\x7F\x09\x01\xF7", 6);
    // for(int i = 0; i < 129; i++) {
    //     if(i < 128) midi->WriteShortMessage(midi, 0x90, i    , 0x7F);
    //     if(i > 0  ) midi->WriteShortMessage(midi, 0x80, i - 1, 0x00);
    //     SleepUS(50000);
    // }
    // SleepUS(1000000);
    midi->Close(midi);
    return 0;
}
