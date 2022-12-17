#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "table.h" 

Table_* create(int length) {
    Table_* new_table = malloc(sizeof(Table_));
    new_table->length = length;
    new_table->page = malloc(sizeof(int) * length);
    new_table->frame = malloc(sizeof(int) * length);
    new_table->LRU_time = malloc(sizeof(int) * length);

    for (int i = 0; i < length; i++) {
        new_table->page[i] = 0;
    }

    if(new_table == NULL || new_table->page == NULL || new_table->frame == NULL) {
        exit(-1);
    }
    return new_table;
}

void free_table(Table_** table) {
    if ((*table)->page != NULL) {
        free((*table)->page);
    }
    if ((*table)->frame != NULL) {
        free((*table)->frame);
    }
    if ((*table)->LRU_time != NULL) {
        free((*table)->LRU_time);
    }
    free(*table);
}

int** memory_fill(int blockSize) {
    int** temp;
    temp = malloc(blockSize * sizeof(int*));
    for(int i = 0; i < blockSize; i++) {
        temp[i] = (int*)malloc(sizeof(int) * blockSize);
        for(int j = 0; j < blockSize; j++) {
            temp[i][j] = 0;
        }
    }
    if(temp == NULL) {
        exit(-1);
    }
    return temp;
}

void freememory(int*** dblPtrArr, int blockSize) {
  for (int i = 0; i < blockSize; i++) {
      if ((*dblPtrArr)[i] != NULL) {
          free((*dblPtrArr)[i]);
      }
  }
  free(*dblPtrArr);
}