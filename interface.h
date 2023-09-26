#include <stdint.h>

#ifndef _INTERFACE
#define _INTERFACE

struct MidiOutInterface_;

typedef struct MidiOutInterface_ MidiOutInterface;

struct MidiOutInterface_
{
    void (*WriteShortMessage) (MidiOutInterface *ifc, uint8_t status, uint8_t data1, uint8_t data2);
    void (*WriteLongMessage)  (MidiOutInterface *ifc, const char *sysex, int32_t len);
    void (*Close)             (MidiOutInterface *ifc);
};

MidiOutInterface* FindMidiDevices(const char *devname);

#endif