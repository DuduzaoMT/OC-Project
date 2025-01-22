#ifndef L2CACHE_H
#define L2CACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "Cache.h"

void resetTime();

uint32_t getTime();

/****************  RAM memory (byte addressable) ***************/
void accessDRAM(uint32_t, uint8_t *, uint32_t);

/*********************** Cache *************************/

void initCache();
void accessL1(uint32_t, uint8_t *, uint32_t);
void accessL2(uint32_t, uint8_t *, uint32_t);

typedef struct CacheLine {
  uint8_t Valid;
  uint8_t Dirty;
  uint32_t Tag;
  uint8_t Block[BLOCK_SIZE];
} CacheLine;

typedef struct L1Cache{
  CacheLine lines[L1_SIZE/BLOCK_SIZE];
} L1Cache;

typedef struct L2Cache{
  CacheLine lines[L2_SIZE/BLOCK_SIZE];
} L2Cache;

typedef struct Cache {
  uint32_t init;
  L1Cache l1;
  L2Cache l2;
} Cache;

/*********************** Interfaces *************************/

void read(uint32_t, uint8_t *);

void write(uint32_t, uint8_t *);

#endif
