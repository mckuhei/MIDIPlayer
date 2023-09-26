#include <stdio.h>
#ifndef _PLAYER
#define _PLAYER
#include "interface.h"

typedef enum {
    MIDIPLAYER_NOERROR      = 0,
    MIDIPLAYER_EOF          = 1,
    MIDIPLAYER_NOT_SMF      = 2,
    MIDIPLAYER_NOT_SUPPORT  = 3
} MPRESULT;

enum {
    NOTE_OFF            = 0x80,
    NOTE_ON             = 0x90,
    POLY_PRESSURE       = 0xA0,
    CONTROL_CHANGE      = 0xB0,
    PROGRAM_CHANGE      = 0xC0,
    CHANNEL_PRESSURE    = 0xD0,
    PITCH_BEND          = 0xE0
};

struct Player_;

typedef struct Player_ Player;

struct Player_
{
    MPRESULT (*Dispose) (Player *player);
    MPRESULT (*Play) (Player *player);
    void *reserved[4];
};

MPRESULT CreateMidiPlayer(Player **player, FILE *fp, MidiOutInterface *ifc);

#endif