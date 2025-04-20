#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct{
    int8_t data[2048];
} DataMemory;

void storeData(DataMemory* memory,int pos,int8_t data){
    if(pos < 0 || pos > 2047){
        printf("Invalid position");
    }
    memory->data[pos] = data;
}

int8_t loadData(DataMemory* memory,int pos){
    if(pos < 0 || pos > 2074){
        printf("Invalid position");
    }
    return memory->data[pos];
}
