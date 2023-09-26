#include "util.h"

#if (defined(_WIN32) || defined(_WINCE)) 
#include <windows.h>
#include <io.h>
#else
#include <sys/time.h>
#endif

#ifdef _WIN32

uint64_t GetTimeUS() 
{
    LARGE_INTEGER lpPerformanceCount;
    LARGE_INTEGER lpFrequency;
    QueryPerformanceCounter(&lpPerformanceCount);
    QueryPerformanceFrequency(&lpFrequency);
    return (uint64_t)((1000000 * (lpPerformanceCount.QuadPart)) / lpFrequency.QuadPart);
}
#else
uint64_t GetTimeUS()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return((tv.tv_sec*1000000)+(tv.tv_usec));
}
#endif

void SleepUS(unsigned long us) {
#ifdef _WIN32
    Sleep(us / 1000);
    // int64_t current = GetTimeUS();
    // int64_t target = current + us * 10;
    // while(target > current) current = GetTimeUS();
#else
    usleep(us);
#endif
}

FILE* DuplicateFileHandle(FILE *fp) {
#ifdef _WIN32
    HANDLE fileHandle = (HANDLE) _get_osfhandle(_fileno(fp));
    char filePath[MAX_PATH];
    if (GetFinalPathNameByHandleA(fileHandle, filePath, sizeof(filePath), FILE_NAME_NORMALIZED) == 0) {
        perror("Error getting file path");
        return NULL;
    }

    return fopen(filePath, "rb");
#else
    return fdopen(fileno(fp), "rb");
#endif
}

bool ReadByte(FILE *fp, int8_t *ptr) {
    return fread(ptr, 1, 1, fp) == 1;
}

bool ReadShort(FILE *fp, int16_t *ptr) {
    uint8_t buf[2];
    if(fread(buf, 1, 2, fp) != 2) return false;
    *ptr = (int16_t) ((buf[0] << 8) | buf[1]) ;
    return true;
}

bool ReadTempo(FILE *fp, int32_t *ptr) {
    uint8_t buf[3];
    if(fread(buf, 1, 3, fp) != 3) return false;
    *ptr = (int32_t) ((buf[0] << 16) | (buf[1] << 8) | buf[2]);
    return true;
}

bool ReadInt(FILE *fp, int32_t *ptr) {
    uint8_t buf[4];
    if(fread(buf, 1, 4, fp) != 4) return false;
    *ptr = (int32_t) ((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
    return true;
}

bool ReadLong(FILE *fp, int64_t *ptr) {
    uint8_t buf[8];
    if(fread(buf, 1, 8, fp) != 8) return false;
    *ptr = (int64_t) (  ((int64_t) (buf[0]) << 56) | ((int64_t) (buf[1]) << 48) | ((int64_t) (buf[2]) << 40) | ((int64_t) (buf[3]) << 32) |
                        ((int64_t) (buf[4]) << 24) | ((int64_t) (buf[5]) << 16) | ((int64_t) (buf[6]) << 8 ) |  (int64_t) (buf[7])
                    );
    return true;
}

bool ReadVarInt(FILE *fp, int32_t *ptr) {
    int32_t val = 0;
    int8_t b;
    do {
        if(!ReadByte(fp, &b)) return false;
        val = (val << 7) | (b & 0x7F);
    } while((b & 0x80) != 0);
    *ptr = val;
    return true;
}

bool ReadVarInt2(FILE *fp, int32_t *ptr, int8_t firstByte) {
    int32_t val = firstByte & 0x7F;
    int8_t b = firstByte;
    while((b & 0x80) != 0) {
        if(!ReadByte(fp, &b)) return false;
        val = (val << 7) | (b & 0x7F);
    }
    *ptr = val;
    return true;
}