#include "L1Cache.h"

uint8_t DRAM[DRAM_SIZE];
uint32_t time;
Cache L1Cache;

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

void initCache() { L1Cache.init = 0; }

void accessL1(uint32_t address, uint8_t *data, uint32_t mode) {

  uint32_t index, tag, offset, memAddress;
  uint8_t TempBlock[BLOCK_SIZE];

  /* init cache */
  if (L1Cache.init == 0) {
    L1Cache.init = 1;

    for (int i = 0; i < L1_SIZE/BLOCK_SIZE; i++)
    {
      L1Cache.lines[i].Valid = 0;
      L1Cache.lines[i].Dirty = 0;
      L1Cache.lines[i].Tag = 0;
      memset(&(L1Cache.lines[i].Block), 0, BLOCK_SIZE * sizeof(uint8_t));
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
      The tag will consist on the rest of the bits of the address
  */

  index = (address >> 6) & 0xFF;
  offset = address & 0x3F;
  tag = address >> 14;

  CacheLine *Line = &(L1Cache.lines[index]);

  /* access Cache*/

  if (!Line->Valid || Line->Tag != tag) {         // if block not present - miss
    // address-offset will give the address of the first byte of the block
    memAddress = address-offset;
    accessDRAM(memAddress, TempBlock, MODE_READ); // get new block from DRAM

    if ((Line->Valid) && (Line->Dirty)) { // line has dirty block
      // The address of the block in memory will be the tag followed by the index 
      memAddress = (Line->Tag << 14) + (index << 6);
      accessDRAM(memAddress, Line->Block, MODE_WRITE); // then write back old block
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

void read(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_WRITE);
}
