#ifndef SPRS_H
#define SPRS_H

#include <stdint.h>

typedef struct {
    int8_t SREG;
    short int PC;
} SPRS;

void initPC(SPRS* sprs);
void incPC(SPRS* sprs);
void initSREG(SPRS* sprs);
void addToPC(SPRS* sprs,int v);
int8_t getC(SPRS* sprs);
int8_t getV(SPRS* sprs);
int8_t getN(SPRS* sprs);
int8_t getS(SPRS* sprs);
int8_t getZ(SPRS* sprs);
void setC(SPRS* sprs);
void setV(SPRS* sprs);
void setN(SPRS* sprs);
void setS(SPRS* sprs);
void setZ(SPRS* sprs);
void unsetC(SPRS* sprs);
void unsetV(SPRS* sprs);
void unsetN(SPRS* sprs);
void unsetS(SPRS* sprs);
void unsetZ(SPRS* sprs);

#endif