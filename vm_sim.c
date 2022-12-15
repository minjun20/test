#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <alloca.h>
#include <time.h>
#include "vmtypes.h"


#define FRAME             256       
#define FRAME_COUNT       256       
#define PAGE_MASK         0xFF00    
#define OFFSET_MASK       0xFF      
#define SHIFT             8         
#define TLB_SIZE          16        
#define PAGE_TABLE_SIZE   256       
#define MAX_LEN           10        
#define PAGE_READ_SIZE    256       

typedef enum { false = 0, true = !false } bool; 

vmTable_t* tlbTable; 
vmTable_t* pageTable; 
int** dram; 

int nextTLBentry = 0; 
int nextPage = 0;  
int nextFrame = 0; 

FILE* address_file;
FILE* backing_store;

char addressReadBuffer[MAX_LEN];
int virtual_addr;
int page_num;
int offset_num;

clock_t start, end;
double cpu_time;
int functionCallCount = 0;

signed char fileReadBuffer[PAGE_READ_SIZE];
signed char translatedValue;

void addressTranslator();
void readStore(int pageNumber);
void LRU(int pageNumber, int frameNumber);
int getLRU(int tlbSize);


int main(int argc, char *argv[])
{
    tlbTable = create(TLB_SIZE); 
    pageTable = create(PAGE_TABLE_SIZE); 
    dram = dramAllocate(FRAME_COUNT, FRAME); 
    int translationCount = 0;

    if (argc != 2) {
        fprintf(stderr,"Usage: ./vm_sim [input file]\n");
        return -1;
    }

    backing_store = fopen("BACKING_STORE.bin", "rb");

    if (backing_store == NULL) {
        fprintf(stderr, "Error opening BACKING_STORE.bin %s\n","BACKING_STORE.bin");
        return -1;
    }

    address_file = fopen(argv[1], "r");

    if (address_file == NULL) {
        fprintf(stderr, "Error opening %s. Expecting [InputFile].txt or equivalent.\n",argv[1]);
        return -1;
    }

    printf("\nNumber of logical pages: %d\nPage size: %d bytes\nPage Table Size: %d\nTLB Size: %d entries\nNumber of Physical Frames: %d\nPhysical Memory Size: %d bytes", PAGE_TABLE_SIZE, PAGE_READ_SIZE, PAGE_TABLE_SIZE, TLB_SIZE, FRAME, PAGE_READ_SIZE * FRAME);

    while (fgets(addressReadBuffer, MAX_LEN, address_file) != NULL) {
        virtual_addr = atoi(addressReadBuffer); 

        page_num = getPageNumber(PAGE_MASK, virtual_addr, SHIFT);

        offset_num = getOffset(OFFSET_MASK, virtual_addr);

        addressTranslator();
        translationCount++;  
    }

    printf("\n-----------------------------------------------------------------------------------\n");
    printf("Number of translated addresses = %d\n", translationCount);
    double pfRate = (double)pageTable->pageFaultCount / (double)translationCount;
    double TLBRate = (double)tlbTable->tlbHitCount / (double)translationCount;

    printf("Page Faults = %d\n", pageTable->pageFaultCount);
    printf("Page Fault Rate = %.3f %%\n",pfRate * 100);
    printf("TLB Hits = %d\n", tlbTable->tlbHitCount);
    printf("TLB Hit Rate = %.3f %%\n", TLBRate * 100);
    printf("\n-----------------------------------------------------------------------------------\n");

    fclose(address_file);
    fclose(backing_store);

    freeVMtable(&tlbTable);
    freeVMtable(&pageTable);
    freeDRAM(&dram, FRAME_COUNT);
    return 0;
}

void addressTranslator() {
    int frame_number = -1;


    for (int i = 0; i < tlbTable->length; i++) {
        if (tlbTable->pageNumArr[i] == page_num) {
            frame_number = tlbTable->frameNumArr[i];
            tlbTable->tlbHitCount++;
            break;
        }
    }

    if (frame_number == -1) {
        tlbTable->tlbMissCount++; 
        for(int i = 0; i < nextPage; i++){
            if(pageTable->pageNumArr[i] == page_num){ 
                frame_number = pageTable->frameNumArr[i];
                break;
            }
        }
        if(frame_number == -1) {  // miss
            pageTable->pageFaultCount++; 
            start = clock();
            readStore(page_num);
            cpu_time += (double)(clock() - start) / CLOCKS_PER_SEC;
            functionCallCount++;
            frame_number = nextFrame - 1; 
        }
    }

    LRU(page_num, frame_number);


    translatedValue = dram[frame_number][offset_num];

    printf("\nVirtual address: %d\t\tPhysical address: %d\t\tValue: %d", virtual_addr, (frame_number << SHIFT) | offset_num, translatedValue);
}

void readStore(int pageNumber) {
    if (fseek(backing_store, pageNumber * PAGE_READ_SIZE, SEEK_SET) != 0) {
        fprintf(stderr, "Error seeking in backing store\n");
    }
    if (fread(fileReadBuffer, sizeof(signed char), PAGE_READ_SIZE, backing_store) == 0) {
        fprintf(stderr, "Error reading from backing store\n");
    }
    for (int i = 0; i < PAGE_READ_SIZE; i++) {
        dram[nextFrame][i] = fileReadBuffer[i];
    }
    pageTable->pageNumArr[nextPage] = pageNumber;
    pageTable->frameNumArr[nextPage] = nextFrame;

    nextFrame++;
    nextPage++;
}

void LRU(int pageNumber, int frameNumber) {
    bool freeSpotFound = false;
    bool alreadyThere = false;
    int replaceIndex = -1;

    for (int i = 0; i < TLB_SIZE; i++) {
        if ((tlbTable->pageNumArr[i] != pageNumber) && (tlbTable->pageNumArr[i] != 0)) {
            tlbTable->entryAgeArr[i]++;
        }
        else if ((tlbTable->pageNumArr[i] != pageNumber) && (tlbTable->pageNumArr[i] == 0)) {
            if (!freeSpotFound) {
                replaceIndex = i;
                freeSpotFound = true;
            }
        }
        else if (tlbTable->pageNumArr[i] == pageNumber) {
            if(!alreadyThere) {
                tlbTable->entryAgeArr[i] = 0;
                alreadyThere = true;
            }
        }
    }

    if (alreadyThere) {
        return;
    }
    else if (freeSpotFound) { 
        tlbTable->pageNumArr[replaceIndex] = pageNumber;   
        tlbTable->frameNumArr[replaceIndex] = frameNumber;
        tlbTable->entryAgeArr[replaceIndex] = 0;
    }
    else {
        replaceIndex = getLRU(TLB_SIZE);
        tlbTable->pageNumArr[replaceIndex] = pageNumber;    
        tlbTable->frameNumArr[replaceIndex] = frameNumber;
        tlbTable->entryAgeArr[replaceIndex] = 0;

    }
}

int getLRU(int tlbSize) {
  int i, max, index;

  max = tlbTable->entryAgeArr[0];
  index = 0;

  for (i = 1; i < tlbSize; i++) {
    if (tlbTable->entryAgeArr[i] > max) {
       index = i;
       max = tlbTable->entryAgeArr[i];
    }
  }
  return index;
}
