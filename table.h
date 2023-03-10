#ifndef VMTYPES_H_ 
#define VMTYPES_H_

typedef struct Table_ {
    int *page; 
    int *frame; 
    int *LRU_time; 
    int length;
} Table_;

Table_* create(int length);

void free_table(Table_** table);

int** memory_fill(int blockSize);

void freememory(int*** dblPtrArr, int frameCount);

#endif 