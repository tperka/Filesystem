#ifndef SUPERBLOCK_HPP
#define SUPERBLOCK_HPP

#define BLOCK_SIZE 4096
#define FILESYSTEM_NAME "myfilesys"

struct Superblock {
	unsigned int blockSize;
	unsigned int iNodeSize;
	unsigned int nOfiNodes;
	unsigned int nOfBlocksForiNodes;
	unsigned int iNodesOffset;

	unsigned int nOfDataBlocks;
	unsigned int dataOffset;
	
	unsigned int iNodeBitmapOffset;
	unsigned int dataBitmapOffset;
	
};
#endif
