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
BranchInfo brInfo = {0};

static void trace(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printf("    >> "); /* 4-space indent */
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
}

static void writeReg(int r, int8_t val)
{
    int8_t old = loadReg(&gprs, r);
    if (old != val)
        trace("R%d changed %d -> %d", r, old, val);
    storeReg(&gprs, r, val);
}

static void writeMem(uint16_t addr, int8_t val)
{
    int8_t old = loadData(&dataMem, addr);
    if (old != val)
        trace("MEM[%d] changed %d -> %d", addr, old, val);
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
    if (res == 0)
        sprs.SREG |= F_Z;
    else
        sprs.SREG &= ~F_Z;
    if (res & 0x80)
        sprs.SREG |= F_N;
    else
        sprs.SREG &= ~F_N;
}

static void setCarry(uint16_t wide) /* ADD only */
{
    if (wide & 0x100)
        sprs.SREG |= F_C;
    else
        sprs.SREG &= ~F_C;
}

static void setV_add(int8_t a, int8_t b, int8_t res) /* ADD */
{
    if (((a ^ b) & 0x80) == 0 && ((a ^ res) & 0x80))
        sprs.SREG |= F_V;
    else
        sprs.SREG &= ~F_V;
}

static void setV_sub(int8_t a, int8_t b, int8_t res) /* SUB = a-b */
{
    if (((a ^ b) & 0x80) && ((a ^ res) & 0x80))
        sprs.SREG |= F_V;
    else
        sprs.SREG &= ~F_V;
}

static void updateS(void) /* S = N xor V */
{
    int n = (sprs.SREG & F_N) ? 1 : 0;
    int v = (sprs.SREG & F_V) ? 1 : 0;
    if (n ^ v)
        sprs.SREG |= F_S;
    else
        sprs.SREG &= ~F_S;
}

void printAllRegs()
{
    printf("\n==== Register File ====\n");
    for (int i = 0; i < 64; i++)
    {
        printf("R%-2d = %d\n", i, loadReg(&gprs, i));
    }
    printf("\n==== Special Registers ====\n");
    printf("PC   = %u\n", sprs.PC);
    printf("SREG = 0x%02X [C=%d V=%d N=%d S=%d Z=%d]\n",
           sprs.SREG,
           !!(sprs.SREG & F_C),
           !!(sprs.SREG & F_V),
           !!(sprs.SREG & F_N),
           !!(sprs.SREG & F_S),
           !!(sprs.SREG & F_Z));
}

void printMemory()
{
    printf("\n==== Instruction Memory ====\n");
    for (int i = 0; i < 1024; i++)
    {
        uint16_t inst = loadInst(&instMem, i);
        if (inst != 0)
        {
            printf("IMEM[%d] = 0x%04X\n", i, inst);
        }
    }

    printf("\n==== Data Memory ====\n");
    for (int i = 0; i < 1024; i++)
    {
        int8_t val = loadData(&dataMem, i);
        if (val != 0)
        {
            printf("DMEM[%d] = %d\n", i, val);
        }
    }
}

void printBinary16(short int value)
{
    for (int i = 15; i >= 0; i--)
    {
        printf("%d", (value >> i) & 1);
        if (i % 4 == 0 && i != 0)
            printf(" ");
    }
}

short int assembleInstruction(const char *line)
{
    char mnemonic[5];
    int reg1, reg2, immediate;
    short int binaryInst = 0b0000000000000000;

    while (*line == ' ' || *line == '\t')
        line++;

    if (sscanf(line, "%4s R%d R%d", mnemonic, &reg1, &reg2) == 3)
    {
        if (reg1 < 0 || reg1 > 63 || reg2 < 0 || reg2 > 63)
        {
            fprintf(stderr, "Error: Invalid register number in instruction: %s\n", line);
            exit(EXIT_FAILURE);
        }

        if (strcmp(mnemonic, "ADD") == 0)
        {
            binaryInst = (0b0000 << 12) | (reg1 << 6) | reg2;
        }
        else if (strcmp(mnemonic, "SUB") == 0)
        {
            binaryInst = (0b0001 << 12) | (reg1 << 6) | reg2;
        }
        else if (strcmp(mnemonic, "MUL") == 0)
        {
            binaryInst = (0b0010 << 12) | (reg1 << 6) | reg2;
        }
        else if (strcmp(mnemonic, "EOR") == 0)
        {
            binaryInst = (0b0110 << 12) | (reg1 << 6) | reg2;
        }
        else if (strcmp(mnemonic, "BR") == 0)
        {
            binaryInst = (0b0111 << 12) | (reg1 << 6) | reg2;
        }
        else
        {
            fprintf(stderr, "Error: Unknown R-format instruction: %s\n", mnemonic);
            exit(EXIT_FAILURE);
        }
    }
    else if (sscanf(line, "%4s R%d %d", mnemonic, &reg1, &immediate) == 3)
    {
        if (reg1 < 0 || reg1 > 63)
        {
            fprintf(stderr, "Error: Invalid register number in instruction: %s\n", line);
            exit(EXIT_FAILURE);
        }

        if (immediate < -32 || immediate > 31)
        {
            fprintf(stderr, "Error: Immediate value out of range (-32 to 31) in instruction: %s\n", line);
            exit(EXIT_FAILURE);
        }

        int sign_extended = immediate << 10 >> 10;

        if (strcmp(mnemonic, "MOVI") == 0)
        {
            binaryInst = (0b0011 << 12) | (reg1 << 6) | (sign_extended & 0b0000000000111111);
        }
        else if (strcmp(mnemonic, "BEQZ") == 0)
        {
            binaryInst = (0b0100 << 12) | (reg1 << 6) | (sign_extended & 0b0000000000111111);
        }
        else if (strcmp(mnemonic, "ANDI") == 0)
        {
            binaryInst = (0b0101 << 12) | (reg1 << 6) | (sign_extended & 0b0000000000111111);
        }
        else if (strcmp(mnemonic, "SAL") == 0)
        {
            if (immediate < 0)
            {
                fprintf(stderr, "Error: Shift amount cannot be negative in instruction: %s\n", line);
                exit(EXIT_FAILURE);
            }
            binaryInst = (0b1000 << 12) | (reg1 << 6) | (immediate & 0b0000000000111111);
        }
        else if (strcmp(mnemonic, "SAR") == 0)
        {
            if (immediate < 0)
            {
                fprintf(stderr, "Error: Shift amount cannot be negative in instruction: %s\n", line);
                exit(EXIT_FAILURE);
            }
            binaryInst = (0b1001 << 12) | (reg1 << 6) | (immediate & 0b0000000000111111);
        }
        else if (strcmp(mnemonic, "LDR") == 0)
        {
            if (immediate < 0 || immediate > 2047)
            {
                fprintf(stderr, "Error: Memory address out of range (0-2047) in instruction: %s\n", line);
                exit(EXIT_FAILURE);
            }
            binaryInst = (0b1010 << 12) | (reg1 << 6) | (immediate & 0b0000000000111111);
        }
        else if (strcmp(mnemonic, "STR") == 0)
        {
            if (immediate < 0 || immediate > 2047)
            {
                fprintf(stderr, "Error: Memory address out of range (0-2047) in instruction: %s\n", line);
                exit(EXIT_FAILURE);
            }
            binaryInst = (0b1011 << 12) | (reg1 << 6) | (immediate & 0b0000000000111111);
        }
        else
        {
            fprintf(stderr, "Error: Unknown I-format instruction: %s\n", mnemonic);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        fprintf(stderr, "Error: Invalid instruction format: %s\n", line);
        exit(EXIT_FAILURE);
    }

    return binaryInst;
}

// void readProgramAndStore()
// {
//     FILE *fp = fopen("Program.txt", "r");
//     if (!fp)
//     {
//         perror("Error opening Program.txt");
//         exit(EXIT_FAILURE);
//     }

//     char line[100];
//     int address = 0;

//     while (fgets(line, sizeof(line), fp))
//     {
//         if (line[0] == '\n' || line[0] == '#' || strlen(line) < 2)
//             continue;

//         char instr[10];
//         char op1[10], op2[10];
//         int parsed = sscanf(line, "%s %s %s", instr, op1, op2);
//         int opcode = -1;

//         // Map mnemonics to opcodes
//         if (strcmp(instr, "ADD") == 0)
//             opcode = 0;
//         else if (strcmp(instr, "SUB") == 0)
//             opcode = 1;
//         else if (strcmp(instr, "MUL") == 0)
//             opcode = 2;
//         else if (strcmp(instr, "MOVI") == 0)
//             opcode = 3;
//         else if (strcmp(instr, "BEQZ") == 0)
//             opcode = 4;
//         else if (strcmp(instr, "ANDI") == 0)
//             opcode = 5;
//         else if (strcmp(instr, "EOR") == 0)
//             opcode = 6;
//         else if (strcmp(instr, "BR") == 0)
//             opcode = 7;
//         else if (strcmp(instr, "SAL") == 0)
//             opcode = 8;
//         else if (strcmp(instr, "SAR") == 0)
//             opcode = 9;
//         else if (strcmp(instr, "LDR") == 0)
//             opcode = 10;
//         else if (strcmp(instr, "STR") == 0)
//             opcode = 11;
//         else
//         {
//             fprintf(stderr, "Unknown instruction: %s\n", instr);
//             continue;
//         }

//         int r1 = 0, r2_or_imm = 0;
//         if (op1[0] == 'R')
//             r1 = atoi(&op1[1]);
//         else
//             r1 = atoi(op1);

//         if (parsed == 3)
//         {
//             if (op2[0] == 'R')
//                 r2_or_imm = atoi(&op2[1]);
//             else
//                 r2_or_imm = atoi(op2);
//         }

//         // Sign-extend imm to 6-bit if needed
//         if (r2_or_imm < 0)
//             r2_or_imm = (r2_or_imm & 0x3F);

//         // Construct instruction
//         uint16_t instruction;
//         if (opcode <= 2 || opcode == 6 || opcode == 7)
//         { // R-type
//             instruction = (opcode << 12) | (r1 << 6) | (r2_or_imm & 0x3F);
//         }
//         else
//         { // I-type
//             instruction = (opcode << 12) | (r1 << 6) | (r2_or_imm & 0x3F);
//         }

//         storeInst(&instMem, address++, (uint16_t)instruction);
//     }

//     fclose(fp);
// }

void readProgramAndStore()
{
    FILE *file = fopen("Program.txt", "r");
    if (!file)
    {
        perror("Error opening program file");
        exit(EXIT_FAILURE);
    }

    char line[100];
    int pos = 0;

    printf("\n=== Loading Program into Instruction Memory ===\n");

    while (fgets(line, sizeof(line), file))
    {
        char *comment = strchr(line, ';');
        if (comment)
            *comment = '\0';

        int len = strlen(line);
        while (len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t' || line[len - 1] == '\n' || line[len - 1] == '\r'))
        {
            line[--len] = '\0';
        }

        if (len == 0)
            continue;

        if (pos >= 1024)
        {
            fprintf(stderr, "Error: Instruction memory full (max 1024 instructions)\n");
            break;
        }

        short int binaryInst = assembleInstruction(line);

        storeInst(&instMem, pos, binaryInst);

        printf("[%04d] ", pos);
        printBinary16(binaryInst);
        printf("  | %s\n", line);

        pos++;
    }

    printf("=== Program Loaded (%d instructions) ===\n\n", pos);
    fclose(file);
}

void ADD(int rd, int rs)
{
    uint8_t oldS = sprs.SREG; /* snapshot */
    int8_t a = loadReg(&gprs, rd);
    int8_t b = loadReg(&gprs, rs);
    uint16_t w = (uint8_t)a + (uint8_t)b; /* 9-bit space */
    int8_t res = (int8_t)w;

    writeReg(rd, res); /* prints change */

    setCarry(w);
    setV_add(a, b, res);
    setNZ(res);
    updateS();

    if (oldS != sprs.SREG)
        trace("SREG 0x%02X -> 0x%02X", oldS, sprs.SREG);
}

void SUB(int rd, int rs)
{
    uint8_t oldS = sprs.SREG;

    int8_t a = loadReg(&gprs, rd);
    int8_t b = loadReg(&gprs, rs);
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

    return (BranchInfo){1, target}; /* taken = 1 */
}

void MOVI(int rd, int8_t imm)
{
    writeReg(rd, imm); /* prints change */
}

BranchInfo BEQZ(int rd, int8_t imm, uint16_t pcSnap)
{
    if (loadReg(&gprs, rd) == 0)
    {
        uint16_t target = pcSnap + 1 + (int8_t)imm;
        sprs.PC = target;
        trace("PC <- %u  (BEQZ taken)", target);
        return (BranchInfo){1, target};
    }
    return (BranchInfo){0, 0}; /* branch not taken */
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

    int8_t res = loadReg(&gprs, rd) >> imm; /* arithmetic shift */
    writeReg(rd, res);

    setNZ(res);

    if (oldS != sprs.SREG)
        trace("SREG 0x%02X -> 0x%02X", oldS, sprs.SREG);
}

void LDR(int rd, int8_t address) /* no flags */
{
    int8_t memVal = loadData(&dataMem, (uint8_t)address);
    writeReg(rd, memVal); /* prints “R changed …” */
}

void STR(int rs, int8_t address)
{
    int8_t R = loadReg(&gprs, rs);
    writeMem((uint16_t)(uint8_t)address, R);
}

uint16_t fetch(int PC)
{
    if (PC < 0 || PC >= 1024)
    {
        printf("Error: PC out of instruction memory bounds\n");
        return 0;
    }

    uint16_t instruction = loadInst(&instMem, PC);

    printf("Fetched instruction %04X\n", instruction);

    return instruction;
}

DecodedInst decode(uint16_t inst)
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
    BranchInfo br = {0, 0};
    switch (d.operation)
    {
    case OP_ADD:
        ADD(d.opr1, d.opr2);
        break;
    case OP_SUB:
        SUB(d.opr1, d.opr2);
        break;
    case OP_MUL:
        MUL(d.opr1, d.opr2);
        break;
    case OP_MOVI:
        MOVI(d.opr1, d.imm);
        break;
    case OP_BEQZ:
        br = BEQZ(d.opr1, d.imm, d.pcSnap);
        break;
    case OP_ANDI:
        ANDI(d.opr1, d.imm);
        break;
    case OP_EOR:
        EOR(d.opr1, d.opr2);
        break;
    case OP_BR:
        br = BR(d.opr1, d.opr2);
        break;
    case OP_SAL:
        SAL(d.opr1, d.imm);
        break;
    case OP_SAR:
        SAR(d.opr1, d.imm);
        break;
    case OP_LDR:
        LDR(d.opr1, d.imm);
        break;
    case OP_STR:
        STR(d.opr1, d.imm);
        break;
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
    uint16_t fetchedInst;
    DecodedInst decodedInst;

    do
    {
        stages[2] = stages[1];
        stageEX = stageID;
        stages[1] = stages[0];

        if (brInfo.taken)
        {
            stages[0] = -1;
            brInfo.taken = 0;
        }
        else if (loadInst(&instMem, sprs.PC) != 0x0000)
        {
            stages[0] = sprs.PC;
            short fetched = fetch(stages[0]);

            stageID = decode(fetched);
            stageID.pcSnap = stages[0];

            incPC(&sprs);
        }
        else
            stages[0] = -1;

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
            trace("Operands: opr1 = R%d, opr2 = R%d, imm = %d", stageID.opr1, stageID.opr2, stageID.imm);
        }
        if (stages[2] != -1)
        {
            printf("Execute stage : Instruction %d\n", stages[2] + 1);
            trace("Executing: %s on R%d, R%d, imm=%d", stageEX.mnemonic, stageEX.opr1, stageEX.opr2, stageEX.imm);
        }

        if (stages[2] != -1)
        {
            brInfo = execute(stageEX);
        }
        printf("PC = %d\n", sprs.PC);
        clock_cycles++;

    } while (stages[0] != -1 || stages[1] != -1 || stages[2] != -1);

    printAllRegs();
    printMemory();
}

int main()
{
    run();
    return 0;
}