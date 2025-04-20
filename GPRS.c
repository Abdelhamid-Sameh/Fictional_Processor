#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct{
    int8_t Registers[64];
} GPRS;

void storeReg(GPRS* gprs,int regPos,int8_t data){
    if(regPos < 0 || regPos > 63){
        printf("Invalid position");
    }
    gprs->Registers[regPos] = data;
}

int8_t loadReg(GPRS* gprs,int pos){
    if(pos < 0 || pos > 63){
        printf("Invalid position");
    }
    return gprs->Registers[pos];
}
