#ifndef DATA_MEMORY_H
#define DATA_MEMORY_H

#include <stdint.h>

typedef struct
{
    int8_t data[2048];
} DataMemory;

void storeData(DataMemory *memory, int pos, int8_t val);
int8_t loadData(DataMemory *memory, int pos);

#endif