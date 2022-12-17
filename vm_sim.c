#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <alloca.h>
#include <time.h>
#include "table.h"

#define FRMAE_SIZE        256       
#define PCB_SIZE          16        
#define PAGE_TABLE_SIZE   256    
#define PAGE_READ_SIZE    256       
#define MAX               10        

Table_* pcbTable; 
Table_* pageTable; 
FILE* address_file;
FILE* backing_store;
clock_t start, end;

int** memory; 
char address_buffer[MAX];

int nextPCBentry = 0; 
int total_page = 0;  
int total_frame = 0; 

int vm_address;
int page_num;
int offset_num;

double cpu_time;

signed char fileReadBuffer[PAGE_READ_SIZE];
signed char translatedValue;

void addressTranslator();
void readStore(int pageNumber);
void LRU(int pageNumber, int frameNumber);
int getLRU(int pcbSize);


int main(int argc, char *argv[])
{
    pcbTable = create(PCB_SIZE); 
    pageTable = create(PAGE_TABLE_SIZE); 
    memory = memory_fill(FRMAE_SIZE); 

    backing_store = fopen("backing_store.bin", "rb");
    address_file = fopen(argv[1], "r");

    while (fgets(address_buffer, MAX, address_file) != NULL) {
        vm_address = atoi(address_buffer); 

        page_num = (vm_address & 0xFF00) >> 8;     
        offset_num = vm_address & 0xFF;

        addressTranslator();
    }

    fclose(address_file);
    fclose(backing_store);

    free_table(&pcbTable);
    free_table(&pageTable);

    freememory(&memory, FRMAE_SIZE);

    return 0;
}

void addressTranslator() {
    int frame_number = -1;

    //Check if in Table
    for (int i = 0; i < pcbTable->length; i++) {
        if (pcbTable->page[i] == page_num) {
            frame_number = pcbTable->frame[i];
            break;
        }
    }

    //If not in Table
    if (frame_number == -1) {
        for(int i = 0; i < total_page; i++){
            if(pageTable->page[i] == page_num){ 
                frame_number = pageTable->frame[i];
                break;
            }
        }
        if(frame_number == -1) {  // miss   
            start = clock();
            readStore(page_num);
            cpu_time += (double)(clock() - start) / CLOCKS_PER_SEC;
            frame_number = total_frame - 1; 
        }
    }

    LRU(page_num, frame_number);

    translatedValue = memory[frame_number][offset_num];

    printf("\nVirtual address: %d\t\tPhysical address: %d\t\tValue: %d", vm_address, (frame_number << 8) | offset_num, translatedValue);
}

void readStore(int pageNumber) {
    if (fseek(backing_store, pageNumber * PAGE_READ_SIZE, SEEK_SET) != 0) {
        fprintf(stderr, "Error seeking in backing store\n");
    }
    if (fread(fileReadBuffer, sizeof(signed char), PAGE_READ_SIZE, backing_store) == 0) {
        fprintf(stderr, "Error reading from backing store\n");
    }
    for (int i = 0; i < PAGE_READ_SIZE; i++) {
        memory[total_frame][i] = fileReadBuffer[i];
    }
    pageTable->page[total_page] = pageNumber;
    pageTable->frame[total_page] = total_frame;

    total_frame++;
    total_page++;
}

void LRU(int pageNumber, int frameNumber) {
    bool empty = false;
    bool exist = false;
    int replaceIndex = -1;

    for (int i = 0; i < PCB_SIZE; i++) {
        if ((pcbTable->page[i] != pageNumber) && (pcbTable->page[i] != 0)) { //update LRU_TIME
            pcbTable->LRU_time[i]++;
        }
        else if ((pcbTable->page[i] != pageNumber) && (pcbTable->page[i] == 0)) {
            if (!empty) {
                replaceIndex = i;
                empty = true;
            }
        }
        else if (pcbTable->page[i] == pageNumber) {
            if(!exist) {
                pcbTable->LRU_time[i] = 0;
                exist = true;
            }
        }
    }

    if (exist) {
        return;
    }
    else if (empty) { //if no space
        pcbTable->page[replaceIndex] = pageNumber;   
        pcbTable->frame[replaceIndex] = frameNumber;
        pcbTable->LRU_time[replaceIndex] = 0;
    }
    else {
        replaceIndex = getLRU(PCB_SIZE);
        pcbTable->page[replaceIndex] = pageNumber;    
        pcbTable->frame[replaceIndex] = frameNumber;
        pcbTable->LRU_time[replaceIndex] = 0;

    }
}

int getLRU(int pcbSize) {
  int i, max, index;

  max = pcbTable->LRU_time[0];
  index = 0;

  for (i = 1; i < pcbSize; i++) {
    if (pcbTable->LRU_time[i] > max) {
       index = i;
       max = pcbTable->LRU_time[i];
    }
  }
  return index;
}