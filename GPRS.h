#ifndef GPRS_H
#define GPRS_H

#include <stdint.h>

typedef struct {
    int8_t Registers[64];
} GPRS;

void storeReg(GPRS* gprs, int regPos, int8_t data);
int8_t loadReg(GPRS* gprs, int pos);

#endif