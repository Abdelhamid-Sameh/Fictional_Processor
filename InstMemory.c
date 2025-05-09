#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct
{
    uint16_t instructions[1024];
} InstMemory;

void storeInst(InstMemory *memory, int pos, uint16_t instruction)
{
    if (pos < 0 || pos > 1023)
    {
        printf("Invalid position");
        return;
    }
    memory->instructions[pos] = instruction;
}

uint16_t loadInst(InstMemory *memory, int pos)
{
    if (pos < 0 || pos > 1023)
    {
        printf("Invalid position");
        return 0;
    }
    return memory->instructions[pos];
}
