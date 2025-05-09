#ifndef INST_MEMORY_H
#define INST_MEMORY_H

typedef struct
{
    uint16_t instructions[1024];
} InstMemory;

void storeInst(InstMemory *memory, int pos, uint16_t instruction);
uint16_t loadInst(InstMemory *memory, int pos);

#endif