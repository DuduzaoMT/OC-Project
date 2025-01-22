#include "L2Cache.h"

uint8_t DRAM[DRAM_SIZE];
uint32_t time;
Cache cache;

/**************** Time Manipulation ***************/
void resetTime() { time = 0; }

uint32_t getTime() { return time; }

/****************  RAM memory (byte addressable) ***************/
void accessDRAM(uint32_t address, uint8_t *data, uint32_t mode) {

  if (address >= DRAM_SIZE - WORD_SIZE + 1)
    exit(-1);

  if (mode == MODE_READ) {

    memcpy(data, &(DRAM[address]), BLOCK_SIZE);
    time += DRAM_READ_TIME;
  }

  if (mode == MODE_WRITE) {

    memcpy(&(DRAM[address]), data, BLOCK_SIZE);
    time += DRAM_WRITE_TIME;
  }
}

/*********************** L1 cache *************************/

void initCache() { cache.init = 0; }

void accessL1(uint32_t address, uint8_t *data, uint32_t mode) {

  uint32_t index, tag, offset, memAddress;
  uint8_t TempBlock[BLOCK_SIZE];

  /* init cache */
  if (cache.init == 0) {
    cache.init = 1;

    for (int i = 0; i < L1_SIZE/BLOCK_SIZE; i++)
    {
      cache.l1.lines[i].Valid = 0;
      cache.l1.lines[i].Dirty = 0;
      cache.l1.lines[i].Tag = 0;
      memset(&(cache.l1.lines[i].Block), 0, BLOCK_SIZE * sizeof(uint8_t));
    }

    for (int i = 0; i < L2_SIZE/BLOCK_SIZE; i++)
    {
      cache.l2.lines[i].Valid = 0;
      cache.l2.lines[i].Dirty = 0;
      cache.l2.lines[i].Tag = 0;
      memset(&(cache.l2.lines[i].Block), 0, BLOCK_SIZE * sizeof(uint8_t));
    }
  }

  /*  
      Index:
      This cache can handle L1_SIZE/BLOCK_SIZE lines (or blocks).
      Therefore, we need log2(L1_SIZE/BLOCK_SIZE) bits of the address.
      In this case, L1 cache has 256 lines, so,
      2^bits = 256, bits = log2(256) = 8

      Offset:
      Each line contains a block. To index all the words inside each
      block we need log2(BLOCK_SIZE) bits of the address.
      In this case, each block has 64 bytes, so,
      2^bits = 64, bits = log2(64) = 6

      Tag:
      The tag will consist on the rest of the bits of the address (32-6-8)
  */

  index = (address >> 6) & 0xFF;
  offset = address & 0x3F;
  tag = address >> 14;

  CacheLine *Line = &(cache.l1.lines[index]);

  /* access Cache*/

  if (!Line->Valid || Line->Tag != tag) {         // if block not present - miss
    // address-offset will give the address of the first byte of the block
    memAddress = address-offset;
    accessL2(memAddress, TempBlock, MODE_READ); // get new block from L2

    if ((Line->Valid) && (Line->Dirty)) { // line has dirty block
      // The address of the block in memory will be the tag followed by the index 
      memAddress = (Line->Tag << 14) + (index << 6);
      accessL2(memAddress, Line->Block, MODE_WRITE); // then write back old block
    }

    memcpy(Line->Block, TempBlock,
           BLOCK_SIZE); // copy new block to cache line
    Line->Valid = 1;
    Line->Tag = tag;
    Line->Dirty = 0;
  } // if miss, then replaced with the correct block

  if (mode == MODE_READ) {    // read data from cache line
    memcpy(data, &(Line->Block[offset]), WORD_SIZE);
    time += L1_READ_TIME;
  }

  if (mode == MODE_WRITE) { // write data from cache line
    memcpy(&(Line->Block[offset]), data, WORD_SIZE);
    time += L1_WRITE_TIME;
    Line->Dirty = 1;
  }
}

void accessL2(uint32_t address, uint8_t *data, uint32_t mode) {

  uint32_t index, tag, memAddress;
  uint8_t TempBlock[BLOCK_SIZE];

  // Cache is initialized because it has gone through L1 first

  /*  
      Index:
      This cache can handle L2_SIZE/BLOCK_SIZE lines (or blocks).
      Therefore, we need log2(L2_SIZE/BLOCK_SIZE) bits of the address.
      In this case, L2 cache has 512 lines, so,
      2^bits = 512, bits = log2(512) = 9

      Offset:
      In this case, we dont need the offset because it will always
      be 0, as we only need to get the whole block.

      Tag:
      The tag will consist on the rest of the bits of the address (32-6-9)
  */
  index = (address >> 6) & 0x1FF;
  tag = address >> 15;

  CacheLine *Line = &(cache.l2.lines[index]);

  /* access Cache*/

  if (!Line->Valid || Line->Tag != tag) {         // if block not present - miss
    // in this case, the address variable will always carry the address
    // of the first byte of the block.
    accessDRAM(address, TempBlock, MODE_READ); // get new block from DRAM

    if ((Line->Valid) && (Line->Dirty)) { // line has dirty block
      // The address of the block in memory will be the tag followed by the index 
      memAddress = (Line->Tag << 15) + (index << 6);
      accessDRAM(memAddress, Line->Block, MODE_WRITE); // then write back old block
    }

    memcpy(Line->Block, TempBlock,
           BLOCK_SIZE); // copy new block to cache line
    Line->Valid = 1;
    Line->Tag = tag;
    Line->Dirty = 0;
  } // if miss, then replaced with the correct block

  if (mode == MODE_READ) {    // read data from cache line
    memcpy(data, &(Line->Block), BLOCK_SIZE);
    time += L2_READ_TIME;
  }

  if (mode == MODE_WRITE) { // write data from cache line
    memcpy(&(Line->Block), data, BLOCK_SIZE);
    time += L2_WRITE_TIME;
    Line->Dirty = 1;
  }
}

void read(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_WRITE);
}
