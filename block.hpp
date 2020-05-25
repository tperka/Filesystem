#ifndef BLOCK_HPP
#define BLOCK_HPP
#include "superblock.hpp"
#define DATA_PER_BLOCK BLOCK_SIZE-sizeof(unsigned int)

struct Block
{
    char data[DATA_PER_BLOCK];
    unsigned int next;
    Block()
    {
        for(int i = 0; i < DATA_PER_BLOCK; i++)
            data[i] = 0;

        next = -1;
    }
};

#endif