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

    void load();
    void save();
    bool doesFS_exist();
public:
    void createFilesystem(unsigned int nOfBlocks);
    void ls();
    void rm(char* name);
    void copyFromLinux(char* name, unsigned char rights);
    void copyToLinux(char* name);
    void rmfs();
    void showInfo();
};
inline void error(const char* message);

#endif // !FILESYSTEM_HPP
