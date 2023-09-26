#ifndef _UTIL
#define _UTIL
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

uint64_t GetTimeUS();
void SleepUS(unsigned long us);

FILE* DuplicateFileHandle(FILE *fp);

bool ReadByte(FILE *fp, int8_t *ptr);
bool ReadShort(FILE *fp, int16_t *ptr);
bool ReadTempo(FILE *fp, int32_t *ptr);
bool ReadInt(FILE *fp, int32_t *ptr);
bool ReadLong(FILE *fp, int64_t *ptr);
bool ReadVarInt(FILE *fp, int32_t *ptr);
bool ReadVarInt2(FILE *fp, int32_t *ptr, int8_t firstByte);

#endif