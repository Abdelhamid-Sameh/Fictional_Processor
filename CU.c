#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "InstMemory.h"
#include "DataMemory.h"
#include "GPRS.h"
#include "SPRS.h"

typedef struct{
    int operation; // 0 - 11
    char Type; // I OR R
    int opr1; // 1st Register
    int opr2; // 2nd Register
    int8_t imm; // Immediate OR Address
    char mnemonic[5]; // Instruction name ("ADD", "SUB" and so on)
} DecodedInst;

InstMemory instMem;
DataMemory dataMem;
GPRS gprs;
SPRS sprs;

int clock_cycles = 1;
int stages[3] = {-1,-1,-1};

enum Opcodes {
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

void printAllRegs(){

}

void printMemory(){

}

int readProgramAndStore(){
    char fileName[] = "Program.txt";
}

void ADD(int opr1,int opr2){

}

void SUB(int opr1,int opr2){

}

void MUL(int opr1,int opr2){

}

void EOR(int opr1,int opr2){

}

void BR(int opr1,int opr2){

}

void MOVI(int opr1,int8_t imm){

}

void BEQZ(int opr1,int8_t imm){

}

void ANDI(int opr1,int8_t imm){

}

void SAL(int opr1,int8_t imm){

}

void SAR(int opr1,int8_t imm){

}

void LDR(int opr1,int8_t address){

}

void STR(int opr1,int8_t address){

}

short int fetch(int PC) {
    if (PC < 0 || PC >= 1024) {
        printf("Error: PC out of instruction memory bounds\n");
        return 0; 
    }
    
    short int instruction = loadInst(&instMem, PC);
    
    printf("Fetched instruction %04X from address %d\n", instruction, PC);
    
    return instruction;
}

DecodedInst decode(short int inst) {
    DecodedInst decoded;
    
    // 1st 4 bits
    decoded.operation = (inst >> 12) & 0xF;
    
    if (decoded.operation == OP_ADD || decoded.operation == OP_SUB || 
        decoded.operation == OP_MUL || decoded.operation == OP_EOR || 
        decoded.operation == OP_BR) {
        decoded.Type = 'R';
        
        decoded.opr1 = (inst >> 6) & 0x3F;
        
        decoded.opr2 = inst & 0x3F;
        
        decoded.imm = 0;
    } 
    else { 
        decoded.Type = 'I';
        
        decoded.opr1 = (inst >> 6) & 0x3F;
        
        decoded.imm = inst & 0x3F;
        // Sign extend if negative (bit 5 is sign bit for 6-bit immediate)
        if (decoded.imm & 0x20) {
            decoded.imm |= 0xC0;  // Sign extend to 8 bits
        }
        
        decoded.opr2 = 0;
    }
    
    switch (decoded.operation) {
        case OP_ADD: strcpy(decoded.mnemonic, "ADD"); break;
        case OP_SUB: strcpy(decoded.mnemonic, "SUB"); break;
        case OP_MUL: strcpy(decoded.mnemonic, "MUL"); break;
        case OP_MOVI: strcpy(decoded.mnemonic, "MOVI"); break;
        case OP_BEQZ: strcpy(decoded.mnemonic, "BEQZ"); break;
        case OP_ANDI: strcpy(decoded.mnemonic, "ANDI"); break;
        case OP_EOR: strcpy(decoded.mnemonic, "EOR"); break;
        case OP_BR: strcpy(decoded.mnemonic, "BR"); break;
        case OP_SAL: strcpy(decoded.mnemonic, "SAL"); break;
        case OP_SAR: strcpy(decoded.mnemonic, "SAR"); break;
        case OP_LDR: strcpy(decoded.mnemonic, "LDR"); break;
        case OP_STR: strcpy(decoded.mnemonic, "STR"); break;
        default: strcpy(decoded.mnemonic, "UNKN"); break;
    }
    
    printf("Decoded instruction: %s R%d", decoded.mnemonic, decoded.opr1);
    if (decoded.Type == 'R') {
        printf(" R%d\n", decoded.opr2);
    } else {
        printf(" %d\n", decoded.imm);
    }
    
    return decoded;
}

void execute(DecodedInst decodedInst){

}

void initREGS(){
    initPC(&sprs);
    initSREG(&sprs);
}

void run(){
    initREGS();
    int instCount = readProgramAndStore();
    short int fetchedInst;
    DecodedInst decodedInst;

    do{
        stages[2] = stages[1];
        stages[1] = stages[0];

        if(sprs.PC < instCount){
            stages[0] = sprs.PC;
        }
        else{
            stages[0] = -1;
        }

        if(stages[0] != -1 || stages[1] != -1 || stages[2] != -1){
            break;
        }

        printf("Clock cycle : %d\n",clock_cycles);
        if(stages[0] != -1){
            printf("Fetch stage : Instruction %d\n",stages[0]+1);
        }
        if(stages[1] != -1){
            printf("Decode stage : Instruction %d\n",stages[1]+1);
        }
        if(stages[2] != -1){
            printf("Execute stage : Instruction %d\n",stages[2]+1);
        }

        if(stages[2] != -1){
            execute(decodedInst);
        }
        if(stages[1] != -1){
            decodedInst = decode(fetchedInst);
        }
        if(stages[0] != -1){
            fetchedInst = fetch(stages[0]);
            incPC(&sprs);
        }

        clock_cycles++;
   
    }while(stages[0] != -1 || stages[1] != -1 || stages[2] != -1);

    printAllRegs();
    printMemory();

}

int main()
{
    return 0;
}