#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct
{
    int8_t SREG;
    short int PC;
} SPRS;

void initPC(SPRS *sprs) { sprs->PC = 0; }

void incPC(SPRS *sprs) { sprs->PC++; }

void addToPC(SPRS *sprs, int v) { sprs->PC += v; }

void initSREG(SPRS *sprs) { sprs->SREG = 0; }

int8_t getC(SPRS *sprs)
{
    int8_t mask = 1 << 4;
    return (sprs->SREG & mask) >> 4;
}

int8_t getV(SPRS *sprs)
{
    int8_t mask = 1 << 3;
    return (sprs->SREG & mask) >> 3;
}

int8_t getN(SPRS *sprs)
{
    int8_t mask = 1 << 2;
    return (sprs->SREG & mask) >> 2;
}

int8_t getS(SPRS *sprs)
{
    int8_t mask = 1 << 1;
    return (sprs->SREG & mask) >> 1;
}

int8_t getZ(SPRS *sprs)
{
    int8_t mask = 1;
    return sprs->SREG & mask;
}

void setC(SPRS *sprs)
{
    int8_t mask = 1 << 4;
    sprs->SREG = sprs->SREG | mask;
}

void setV(SPRS *sprs)
{
    int8_t mask = 1 << 3;
    sprs->SREG = sprs->SREG | mask;
}

void setN(SPRS *sprs)
{
    int8_t mask = 1 << 2;
    sprs->SREG = sprs->SREG | mask;
}

void setS(SPRS *sprs)
{
    int8_t mask = 1 << 1;
    sprs->SREG = sprs->SREG | mask;
}

void setZ(SPRS *sprs)
{
    int8_t mask = 1;
    sprs->SREG = sprs->SREG | mask;
}

void unsetC(SPRS *sprs)
{
    int8_t mask = 1 << 4;
    sprs->SREG = (sprs->SREG & ~(mask));
}

void unsetV(SPRS *sprs)
{
    int8_t mask = 1 << 3;
    sprs->SREG = (sprs->SREG & ~(mask));
}

void unsetN(SPRS *sprs)
{
    int8_t mask = 1 << 2;
    sprs->SREG = (sprs->SREG & ~(mask));
}

void unsetS(SPRS *sprs)
{
    int8_t mask = 1 << 1;
    sprs->SREG = (sprs->SREG & ~(mask));
}

void unsetZ(SPRS *sprs)
{
    int8_t mask = 1;
    sprs->SREG = (sprs->SREG & ~(mask));
}
