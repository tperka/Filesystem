#ifndef INODE_HPP
#define INODE_HPP
#include <ctime>
#include <iostream>
#include "block.hpp"
struct iNode
{
	unsigned char rights;
	int ownerID;
	unsigned int size;
	time_t created;
	time_t lastAccessed;
	time_t lastModified;
	char name[84];
	unsigned int firstBlock;
};
std::ostream& operator<<(std::ostream& os, const iNode& i)
{
	os << i.name << i.size;
	return os;
}
#endif
