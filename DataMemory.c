#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct
{
    int8_t data[2048];
} DataMemory;

void storeData(DataMemory *memory, int pos, int8_t val)
{
    if (pos < 0 || pos > 2047)
    {
        printf("Invalid position");
        return;
    }
    memory->data[pos] = val;
}

int8_t loadData(DataMemory *memory, int pos)
{
    if (pos < 0 || pos > 2047)
    {
        printf("Invalid position");
        return -1;
    }
    return memory->data[pos];
}
