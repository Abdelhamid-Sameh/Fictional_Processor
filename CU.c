#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
} DecodedInst;

InstMemory instMem;
DataMemory dataMem;
GPRS gprs;
SPRS sprs;

int clock_cycles = 1;

int stages[3] = {-1,-1,-1};

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

short int fetch(int PC){
    
}

DecodedInst decode(short int inst){
    

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

