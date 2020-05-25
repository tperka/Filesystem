#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include "inode.hpp"
#include "superblock.hpp"
#include "block.hpp"

class Filesystem
{
    Superblock super;
    char* userDataBitmap;
    char* iNodesBitmap;
    iNode* iNodesTable;
    Block* userData;

public:
    void createFilesystem(unsigned int nOfBlocks);
    bool doesFS_exist();
    void ls();
    void rm(char* name);
    void load();
    void save();
    void copyFromLinux(char* name, unsigned char rights);
    void copyToLinux(char* name);
    void rmfs();
};
inline void error(const char* message);

#endif // !FILESYSTEM_HPP
