#include <stdio.h>
#include <stdlib.h>

typedef struct{
    short int instructions[1024];
} InstMemory;

void storeInst(InstMemory* memory,int pos,short int instruction){
    if(pos < 0 || pos > 1023){
        printf("Invalid position");
    }
    memory->instructions[pos] = instruction;
}

short int loadInst(InstMemory* memory,int pos){
    if(pos < 0 || pos > 1023){
        printf("Invalid position");
    }
    return memory->instructions[pos];
}
