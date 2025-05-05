#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "InstMemory.h"
#include "DataMemory.h"
#include "GPRS.h"
#include "SPRS.h"
#include <stdarg.h>

InstMemory instMem;
DataMemory dataMem;
GPRS gprs;
SPRS sprs;

typedef struct
{
    int operation;    // 0 - 11
    char Type;        // I OR R
    int opr1;         // 1st Register
    int opr2;         // 2nd Register
    int8_t imm;       // Immediate OR Address
    char mnemonic[5]; // Instruction name ("ADD", "SUB" and so on)
    uint16_t pcSnap;  // 3shan caputre
} DecodedInst;

typedef struct 
{ 
    int taken; 
    uint16_t target;
} BranchInfo;

DecodedInst stageID, stageEX;
BranchInfo  brInfo = {0};

static void trace(const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    printf("    >> ");                 /* 4-space indent */
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
}

static void writeReg(int r, int8_t val)
{
    int8_t old = loadReg(&gprs, r);
    if (old != val) trace("R%d changed %d -> %d", r, old, val);
    storeReg(&gprs, r, val);
}

static void writeMem(uint16_t addr, int8_t val)
{
    int8_t old = loadData(&dataMem, addr);
    if (old != val) trace("MEM[%d] changed %d -> %d", addr, old, val);
    storeData(&dataMem, addr, val);
}

#define F_C 0x10
#define F_V 0x08
#define F_N 0x04
#define F_S 0x02
#define F_Z 0x01

int clock_cycles = 1;
int stages[3] = {-1, -1, -1};

enum Opcodes
{
    OP_ADD = 0,
    OP_SUB = 1,
    OP_MUL = 2,
    OP_MOVI = 3,
    OP_BEQZ = 4,
    OP_ANDI = 5,
    OP_EOR = 6,
    OP_BR = 7,
    OP_SAL = 8,
    OP_SAR = 9,
    OP_LDR = 10,
    OP_STR = 11
};


static void setNZ(int8_t res)
{
    if (res == 0) sprs.SREG |=  F_Z; else sprs.SREG &= ~F_Z;
    if (res & 0x80) sprs.SREG |=  F_N; else sprs.SREG &= ~F_N;
}

static void setCarry(uint16_t wide)/* ADD only */
{
    if (wide & 0x100) sprs.SREG |=  F_C;
    else               sprs.SREG &= ~F_C;
}

static void setV_add(int8_t a, int8_t b, int8_t res)/* ADD */
{
    if (((a ^ b) & 0x80) == 0 && ((a ^ res) & 0x80))
        sprs.SREG |=  F_V;
    else
        sprs.SREG &= ~F_V;
}

static void setV_sub(int8_t a, int8_t b, int8_t res)/* SUB = a-b */
{
    if (((a ^ b) & 0x80) && ((a ^ res) & 0x80))
        sprs.SREG |=  F_V;
    else
        sprs.SREG &= ~F_V;
}

static void updateS(void)/* S = N xor V */
{
    int n = (sprs.SREG & F_N) ? 1 : 0;
    int v = (sprs.SREG & F_V) ? 1 : 0;
    if (n ^ v) sprs.SREG |=  F_S;
    else       sprs.SREG &= ~F_S;
}


void printAllRegs()
{
}

void printMemory()
{
}

void readProgramAndStore()
{
    int pos = 0;
    char fileName[] = "Program.txt";
    // store into memory
    storeInst(&instMem, pos++, 61440);
}

void ADD(int rd, int rs)
{
    uint8_t oldS = sprs.SREG;                     /* snapshot */
    int8_t  a    = loadReg(&gprs, rd);
    int8_t  b    = loadReg(&gprs, rs);
    uint16_t w   = (uint8_t)a + (uint8_t)b;       /* 9-bit space */
    int8_t  res  = (int8_t)w;

    writeReg(rd, res);                            /* prints change */

    setCarry(w);
    setV_add(a, b, res);
    setNZ(res);
    updateS();

    if(oldS != sprs.SREG)
        trace("SREG 0x%02X -> 0x%02X", oldS, sprs.SREG);
}


void SUB(int rd, int rs)
{
    uint8_t oldS = sprs.SREG;

    int8_t a   = loadReg(&gprs, rd);
    int8_t b   = loadReg(&gprs, rs);
    int8_t res = a - b;

    writeReg(rd, res);

    setV_sub(a, b, res);
    setNZ(res);
    updateS();

    if (oldS != sprs.SREG)
        trace("SREG 0x%02X -> 0x%02X", oldS, sprs.SREG);
}

void MUL(int rd, int rs)
{
    uint8_t oldS = sprs.SREG;

    int8_t res = loadReg(&gprs, rd) * loadReg(&gprs, rs);
    writeReg(rd, res);

    setNZ(res);

    if (oldS != sprs.SREG)
        trace("SREG 0x%02X -> 0x%02X", oldS, sprs.SREG);
}

void EOR(int rd, int rs)
{
    uint8_t oldS = sprs.SREG;

    int8_t res = loadReg(&gprs, rd) ^ loadReg(&gprs, rs);
    writeReg(rd, res);

    setNZ(res);

    if (oldS != sprs.SREG)
        trace("SREG 0x%02X -> 0x%02X", oldS, sprs.SREG);
}


BranchInfo BR(int rHigh, int rLow)
{
    uint16_t target = ((uint16_t)(uint8_t)loadReg(&gprs, rHigh) << 8) |
                       (uint8_t)loadReg(&gprs, rLow);

    sprs.PC = target;
    trace("PC <- %u  (BR)", target);

    return (BranchInfo){1, target};   /* taken = 1 */
}

void MOVI(int rd, int8_t imm)
{
    writeReg(rd, imm);          /* prints change */
}

BranchInfo BEQZ(int rd, int8_t imm, uint16_t pcSnap)
{
    if (loadReg(&gprs, rd) == 0) {
        uint16_t target = pcSnap + 1 + (int8_t)imm;
        sprs.PC = target;
        trace("PC <- %u  (BEQZ taken)", target);
        return (BranchInfo){1, target};
    }
    return (BranchInfo){0, 0};        /* branch not taken */
}

void ANDI(int rd, int8_t imm)
{
    uint8_t oldS = sprs.SREG;

    int8_t res = loadReg(&gprs, rd) & imm;
    writeReg(rd, res);

    setNZ(res);

    if (oldS != sprs.SREG)
        trace("SREG 0x%02X -> 0x%02X", oldS, sprs.SREG);
}

void SAL(int rd, int8_t imm)
{
    uint8_t oldS = sprs.SREG;

    int8_t res = loadReg(&gprs, rd) << imm;
    writeReg(rd, res);

    setNZ(res);

    if (oldS != sprs.SREG)
        trace("SREG 0x%02X -> 0x%02X", oldS, sprs.SREG);
}   

void SAR(int rd, int8_t imm)
{
    uint8_t oldS = sprs.SREG;

    int8_t res = loadReg(&gprs, rd) >> imm;   /* arithmetic shift */
    writeReg(rd, res);

    setNZ(res);

    if (oldS != sprs.SREG)
        trace("SREG 0x%02X -> 0x%02X", oldS, sprs.SREG);
}

void LDR(int rd, int8_t address)          /* no flags */
{
    int8_t memVal = loadData(&dataMem, (uint8_t)address);
    writeReg(rd, memVal);                 /* prints “R changed …” */
}

void STR(int rs, int8_t address)
{
    int8_t R = loadReg(&gprs, rs);
    writeMem((uint16_t)(uint8_t)address, R);
}


short int fetch(int PC)
{
    if (PC < 0 || PC >= 1024)
    {
        printf("Error: PC out of instruction memory bounds\n");
        return 0;
    }

    short int instruction = loadInst(&instMem, PC);

    printf("Fetched instruction %04X\n", instruction);

    return instruction;
}

DecodedInst decode(short int inst)
{
    DecodedInst decoded;

    decoded.operation = (inst >> 12) & 0b1111;

    if (decoded.operation == OP_ADD || decoded.operation == OP_SUB ||
        decoded.operation == OP_MUL || decoded.operation == OP_EOR ||
        decoded.operation == OP_BR)
    {
        decoded.Type = 'R';
    }
    else
    {
        decoded.Type = 'I';
    }

    decoded.opr1 = (inst >> 6) & 0b00111111;

    decoded.opr2 = inst & 0b00111111;

    decoded.imm = inst & 0b00111111;

    if (decoded.imm & 0b100000)
    {
        decoded.imm |= 0b11000000; // Sign extend to 8 bits
    }

    switch (decoded.operation)
    {
    case OP_ADD:
        strcpy(decoded.mnemonic, "ADD");
        break;
    case OP_SUB:
        strcpy(decoded.mnemonic, "SUB");
        break;
    case OP_MUL:
        strcpy(decoded.mnemonic, "MUL");
        break;
    case OP_MOVI:
        strcpy(decoded.mnemonic, "MOVI");
        break;
    case OP_BEQZ:
        strcpy(decoded.mnemonic, "BEQZ");
        break;
    case OP_ANDI:
        strcpy(decoded.mnemonic, "ANDI");
        break;
    case OP_EOR:
        strcpy(decoded.mnemonic, "EOR");
        break;
    case OP_BR:
        strcpy(decoded.mnemonic, "BR");
        break;
    case OP_SAL:
        strcpy(decoded.mnemonic, "SAL");
        break;
    case OP_SAR:
        strcpy(decoded.mnemonic, "SAR");
        break;
    case OP_LDR:
        strcpy(decoded.mnemonic, "LDR");
        break;
    case OP_STR:
        strcpy(decoded.mnemonic, "STR");
        break;
    default:
        strcpy(decoded.mnemonic, "UNKN");
        break;
    }

    printf("Decoded instruction: %s R%d", decoded.mnemonic, decoded.opr1);
    if (decoded.Type == 'R')
    {
        printf(" R%d\n", decoded.opr2);
    }
    else
    {
        printf(" %d\n", decoded.imm);
    }

    return decoded;
}

BranchInfo execute(DecodedInst d)
{
    BranchInfo br = {0,0};
    switch (d.operation)
    {
        case OP_ADD:  ADD (d.opr1, d.opr2); break;
        case OP_SUB:  SUB (d.opr1, d.opr2); break;
        case OP_MUL:  MUL (d.opr1, d.opr2); break;
        case OP_MOVI: MOVI(d.opr1, d.imm ); break;
        case OP_BEQZ: br = BEQZ(d.opr1, d.imm, d.pcSnap); break;
        case OP_ANDI: ANDI(d.opr1, d.imm ); break;
        case OP_EOR:  EOR (d.opr1, d.opr2); break;
        case OP_BR:   br = BR  (d.opr1, d.opr2);          break;
        case OP_SAL:  SAL (d.opr1, d.imm ); break;
        case OP_SAR:  SAR (d.opr1, d.imm ); break;
        case OP_LDR:  LDR (d.opr1, d.imm ); break;
        case OP_STR:  STR (d.opr1, d.imm ); break;
    }
    return br;
}



void initREGS()
{
    initPC(&sprs);
    initSREG(&sprs);
}

void run()
{
    initREGS();
    readProgramAndStore();
    short int fetchedInst;
    DecodedInst decodedInst;

    do
    {
        stages[2] = stages[1];
        stageEX = stageID;
        stages[1] = stages[0];

        if (brInfo.taken) { stages[0] = -1; brInfo.taken = 0; }
        else if (loadInst(&instMem, sprs.PC) != 61440)
        {
            stages[0] = sprs.PC;
             short fetched = fetch(stages[0]);

            stageID = decode(fetched);
            stageID.pcSnap = stages[0];

            incPC(&sprs);
        }
        else stages[0] = -1;


        if (stages[0] == -1 && stages[1] == -1 && stages[2] == -1)
        {   
            break;
        }

        printf("Clock cycle : %d\n", clock_cycles);
        if (stages[0] != -1)
        {
            printf("Fetch stage : Instruction %d\n", stages[0] + 1);
        }
        if (stages[1] != -1)
        {
            printf("Decode stage : Instruction %d\n", stages[1] + 1);
        }
        if (stages[2] != -1)
        {
            printf("Execute stage : Instruction %d\n", stages[2] + 1);
        }

        if (stages[2] != -1)
        {
            brInfo = execute(stageEX);
        }

        clock_cycles++;

    } while (stages[0] != -1 || stages[1] != -1 || stages[2] != -1);

    printAllRegs();
    printMemory();
}

int main()
{
    SPRS sprs;
    initSREG(&sprs);
    printf("%d\n", getC(&sprs));
    printf("%d\n", getV(&sprs));
    printf("%d\n", getN(&sprs));
    printf("%d\n", getS(&sprs));
    printf("%d\n", getZ(&sprs));
    setCarry(&sprs);
    setV(&sprs);
    setN(&sprs);
    setS(&sprs);
    setZ(&sprs);
    printf("%d\n", getC(&sprs));
    printf("%d\n", getV(&sprs));
    printf("%d\n", getN(&sprs));
    printf("%d\n", getS(&sprs));
    printf("%d\n", getZ(&sprs));
    unsetC(&sprs);
    unsetV(&sprs);
    unsetN(&sprs);
    unsetS(&sprs);
    unsetZ(&sprs);
    printf("%d\n", getC(&sprs));
    printf("%d\n", getV(&sprs));
    printf("%d\n", getN(&sprs));
    printf("%d\n", getS(&sprs));
    printf("%d\n", getZ(&sprs));
    return 0;
}