#include <Windows.h>
#include <stdlib.h>
#include <stdint.h>
#include "interface.h"

typedef struct WinMMOutInterface {
    MidiOutInterface ifc;
    HMIDIOUT handle;
} WinMMOutInterface;

static void WinMMWriteShortMessage(MidiOutInterface *ifc, uint8_t status, uint8_t data1, uint8_t data2) {
    WinMMOutInterface *winmm = (WinMMOutInterface *) ifc;
    midiOutShortMsg(winmm->handle, (DWORD) (status) | ((DWORD) (data1) << 8) | ((DWORD) (data2) << 16));
}

static void WinMMWriteLongMessage(MidiOutInterface *ifc, const char *sysex, int32_t len) {
    WinMMOutInterface *winmm = (WinMMOutInterface *) ifc;
    MIDIHDR header;
    memset(&header, 0, sizeof(MIDIHDR));
    HMIDIOUT midiOut = winmm->handle;
    header.lpData = (LPSTR) sysex;
    header.dwBufferLength = len;
    header.dwFlags = 0;
    midiOutPrepareHeader(midiOut, &header, sizeof(MIDIHDR));
    midiOutLongMsg(midiOut, &header, sizeof(MIDIHDR));
    midiOutUnprepareHeader(midiOut, &header, sizeof(MIDIHDR));
}

static void WinMMClose(MidiOutInterface *ifc) {
    WinMMOutInterface *winmm = (WinMMOutInterface *) ifc;
    midiOutClose(winmm->handle);
    free(ifc);
}

MidiOutInterface* FindMidiDevices(const char *devname) {
    WinMMOutInterface *ifc = NULL;
    int devs = midiOutGetNumDevs();
    MIDIOUTCAPSA caps;
    for(int i = -1; i < devs; i++) {
        MMRESULT result = midiOutGetDevCapsA(i, &caps, sizeof(MIDIOUTCAPSA));
        if(result == MMSYSERR_NOERROR) {
            if(strcmp(caps.szPname, devname) == 0) {
                HMIDIOUT handle;
                result = midiOutOpen(&handle, i, (DWORD_PTR) NULL, (DWORD_PTR) NULL, CALLBACK_NULL);
                if(result == MMSYSERR_NOERROR) {
                    ifc = malloc(sizeof(WinMMOutInterface));
                    ifc->handle = handle;
                    ifc->ifc.WriteShortMessage = WinMMWriteShortMessage;
                    ifc->ifc.WriteLongMessage = WinMMWriteLongMessage;
                    ifc->ifc.Close = WinMMClose;
                    break;
                }
            }
        }
    }
    return (MidiOutInterface *)ifc;
}