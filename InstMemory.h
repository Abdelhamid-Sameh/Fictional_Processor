#ifndef INST_MEMORY_H
#define INST_MEMORY_H

typedef struct {
    short int instructions[1024];
} InstMemory;

void storeInst(InstMemory* memory, int pos, short int instruction);
short int loadInst(InstMemory* memory, int pos);

#endif