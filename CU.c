#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "InstMemory.h"
#include "DataMemory.h"
#include "GPRS.h"
#include "SPRS.h"

typedef struct
{
    int operation;    // 0 - 11
    char Type;        // I OR R
    int opr1;         // 1st Register
    int opr2;         // 2nd Register
    int8_t imm;       // Immediate OR Address
    char mnemonic[5]; // Instruction name ("ADD", "SUB" and so on)
} DecodedInst;

InstMemory instMem;
DataMemory dataMem;
GPRS gprs;
SPRS sprs;

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

void ADD(int opr1, int opr2)
{
    int8_t Result = loadReg(&gprs,opr1) + loadReg(&gprs,opr2);
    storeReg(&gprs, opr1, Result);
}

void SUB(int opr1, int opr2)
{
    int8_t Result = loadReg(&gprs,opr1) - loadReg(&gprs,opr2);
    storeReg(&gprs, opr1, Result);
}

void MUL(int opr1, int opr2)
{
    int8_t Result = loadReg(&gprs,opr1) * loadReg(&gprs,opr2);
    storeReg(&gprs, opr1, Result);
}

void EOR(int opr1, int opr2)
{
    int8_t Result = loadReg(&gprs,opr1) ^ loadReg(&gprs,opr2);
    storeReg(&gprs, opr1, Result);
}

void BR(int opr1, int opr2)
{
    int8_t Result = (loadReg(&gprs,opr1)<<4) | loadReg(&gprs,opr2);
    initPC(&sprs);
    addToPC(&sprs, Result);
}

void MOVI(int opr1, int8_t imm)
{
    storeReg(&gprs, opr1, imm);
}

void BEQZ(int opr1, int8_t imm)
{
    if (loadReg(&gprs, opr1) == 0)
    {
        addToPC(&sprs, 1 + imm);
    }
}

void ANDI(int opr1, int8_t imm)
{
    int8_t R = loadReg(&gprs, opr1);
    int8_t Result = R & imm;
    storeReg(&gprs, opr1, Result);
}

void SAL(int opr1, int8_t imm)
{
    int8_t R = loadReg(&gprs, opr1);
    int8_t Result = R << imm;
    storeReg(&gprs, opr1, Result);
}

void SAR(int opr1, int8_t imm)
{
    int8_t R = loadReg(&gprs, opr1);
    int8_t Result = R >> imm;
    storeReg(&gprs, opr1, Result);
}

void LDR(int opr1, int8_t address)
{
    int8_t memoryValue = loadData(&dataMem, address);
    storeReg(&gprs, opr1, memoryValue);
}

void STR(int opr1, int8_t address)
{
    int8_t R = loadReg(&gprs, opr1);
    storeData(&dataMem, opr1, address);
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

void execute(DecodedInst decodedInst)
{
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
        stages[1] = stages[0];

        if (loadInst(&instMem, sprs.PC) != 61440)
        {
            stages[0] = sprs.PC;
        }
        else
        {
            stages[0] = -1;
        }

        if (stages[0] != -1 || stages[1] != -1 || stages[2] != -1)
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
            execute(decodedInst);
        }
        if (stages[1] != -1)
        {
            decodedInst = decode(fetchedInst);
        }
        if (stages[0] != -1)
        {
            fetchedInst = fetch(stages[0]);
            incPC(&sprs);
        }

        clock_cycles++;

    } while (stages[0] != -1 || stages[1] != -1 || stages[2] != -1);

    printAllRegs();
    printMemory();
}

int main()
{
    return 0;
}