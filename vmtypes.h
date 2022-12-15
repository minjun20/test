#ifndef VMTYPES_H_ 
#define VMTYPES_H_

typedef struct vmTable_t {
    int *pageNumArr; 
    int *frameNumArr; 
    int *entryAgeArr; 
    int length;
    int pageFaultCount;
    int tlbHitCount;
    int tlbMissCount;
} vmTable_t;

vmTable_t* create(int length);

void displayTable(vmTable_t** tableToView);

void freeVMtable(vmTable_t** table);

int** dramAllocate(int frameCount, int blockSize);

void freeDRAM(int*** dblPtrArr, int frameCount);

int getPageNumber(int mask, int value, int shift);

int getOffset(int mask, int value);


#endif 