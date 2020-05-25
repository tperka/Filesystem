#include <fstream>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <cmath>
#include <string.h>
#include <vector>
#include <iomanip>

#include "filesystem.hpp"

using namespace std;

void error(const char* message)
{
    cerr << message << endl;
    exit(EXIT_FAILURE);
}
bool Filesystem::doesFS_exist()
{
    ifstream test(FILESYSTEM_NAME);
    return test.good();
}

void Filesystem::load()
{
    if(!doesFS_exist())
        error("Could not load: filesystem does not exist");

    fstream fs(FILESYSTEM_NAME, ios::in | ios::binary);

    //wczytywanie superbloku
    fs.seekg(0);
    fs.read((char*) &super, sizeof(Superblock));
    
    //wczytywanie bitmapy iNodeów
    iNodesBitmap = new char[super.nOfiNodes];
    fs.seekg(BLOCK_SIZE * super.iNodeBitmapOffset);
    fs.read(iNodesBitmap, super.nOfiNodes);
    
    //wczytywanie bitmapy danych uzytkownika
    userDataBitmap = new char[super.nOfDataBlocks];
    fs.seekg(BLOCK_SIZE * super.dataBitmapOffset);
    fs.read(userDataBitmap, super.nOfDataBlocks);

    //wczytywanie tablicy iNodeów
    iNodesTable = new iNode[super.nOfiNodes];
    fs.seekg(BLOCK_SIZE * super.iNodesOffset);
    fs.read((char*)iNodesTable, super.nOfiNodes * super.iNodeSize);
    
    //wczytywanie bloków pamięci użytkownika
    userData = new Block[super.nOfDataBlocks];
    fs.seekg(BLOCK_SIZE * super.dataOffset);
    fs.read((char*)userData, BLOCK_SIZE * super.nOfDataBlocks);
    
    fs.close();
}

void Filesystem::save()
{
    fstream fs(FILESYSTEM_NAME, ios::out | ios::binary);
    if(!fs.good())
        error("Could not open filesystem");

    //zapisywanie superbloku
    fs.seekp(0);
    fs.write((char*) &super, sizeof(Superblock));
    
    //zapisywanie bitmapy iNodeów
    iNodesBitmap = new char[super.nOfiNodes];
    fs.seekp(BLOCK_SIZE * super.iNodeBitmapOffset);
    fs.write(iNodesBitmap, super.nOfiNodes);
    
    //zapisywanie bitmapy danych uzytkownika
    fs.seekp(BLOCK_SIZE * super.dataBitmapOffset);
    fs.write(userDataBitmap, super.nOfDataBlocks);

    //zapisywanie tablicy iNodeów
    fs.seekp(BLOCK_SIZE * super.iNodesOffset);
    fs.write((char*)iNodesTable, super.nOfiNodes * super.iNodeSize);
    
    //zapisywanie bloków pamięci użytkownika
    fs.seekp(BLOCK_SIZE * super.dataOffset);
    fs.write((char*)userData, BLOCK_SIZE * super.nOfDataBlocks);
    
    fs.close();
}
void Filesystem::createFilesystem(unsigned int nOfBlocks)
{
    //sprawdz czy nie powinno sie zmodyfikowac bitmap
    // zeby pozwolic na wieksze systemy plikow
    if(doesFS_exist)
        error("Filesystem already exists!");

    fstream fs(FILESYSTEM_NAME, ios::out | ios::binary);

    if(!fs.good())
        error("Could not create filesystem");

    const char nullSign = '\0';

    for(int i = 0; i < nOfBlocks * BLOCK_SIZE; i++)
    {
        fs.write(&nullSign, 1);
    }
    super.blockSize = BLOCK_SIZE;
    super.iNodeSize = sizeof(iNode); //128 bajtów
    int iNodesPerBlock = super.blockSize/super.iNodeSize;
    super.nOfiNodes = nOfBlocks;
    super.nOfBlocksForiNodes = (unsigned int)ceil((float)super.nOfiNodes / (float)iNodesPerBlock);
    super.iNodeBitmapOffset = 1;
    super.dataBitmapOffset = 2;
    super.iNodesOffset = 3;
    
    super.nOfDataBlocks = nOfBlocks - 3 - super.nOfBlocksForiNodes;
    super.dataOffset = 3+super.nOfBlocksForiNodes;
    // tu mamy zainicjowany superblok, zapisuujemy go
    fs.seekp(0);
    fs.write((char*)&super, sizeof(Superblock) );
    // tworzenie bitmap
    iNodesBitmap = new char[super.nOfiNodes];
    userDataBitmap = new char[super.nOfDataBlocks];

    for(int i = 0; i < super.nOfiNodes; i++)
        iNodesBitmap[i] = 0;
    
    for(int i = 0; i < super.nOfDataBlocks; i++)
        userDataBitmap[i] = 0;

    //zapisywanie bitmapy inodeów
    fs.seekp(BLOCK_SIZE * super.iNodeBitmapOffset);
    fs.write(iNodesBitmap, super.nOfiNodes);

    //zapissywanie bitmapy bloków użytkownika
    fs.seekp(BLOCK_SIZE * super.dataBitmapOffset);
    fs.write(userDataBitmap, super.nOfDataBlocks);


    //zapisywanie bloków użytkownika
    userData = new Block[super.nOfDataBlocks];
    fs.seekp(BLOCK_SIZE * super.dataOffset);
    fs.write((char*) userData, BLOCK_SIZE * super.nOfDataBlocks);

    fs.close();
}


void Filesystem::ls()
{
    load();
    for(int i = 0; i < super.nOfiNodes; i++)
    {
        if(iNodesBitmap[i] != 0)
            cout << iNodesTable[i] << endl;
    }
}

void Filesystem::rm(char* name)
{
    load();
    for(int i = 0; i < super.nOfiNodes; i++)
    {
        if(iNodesBitmap[i] != 0)
        {
            if(strcmp(iNodesTable[i].name, name) == 0)
            {
                int iter = iNodesTable[i].firstBlock;
                //zwalnianie blokow pamieci pliku
                do 
                {
                   userDataBitmap[iter] = 0; 
                   iter = userData[iter].next;
                }while(userData[iter].next != -1);
                //zwalnianie iNodea
                cout << "File: " << iNodesTable[i].name <<" was successfully deleted" <<endl;
                iNodesBitmap[i] = 0;
                 
                 //zapisywanie zmian do pliku
                fstream fs(FILESYSTEM_NAME, ios::out | ios::binary);
                fs.seekp(BLOCK_SIZE * super.iNodeBitmapOffset);
                fs.write(iNodesBitmap, super.nOfiNodes);

                fs.seekp(BLOCK_SIZE * super.dataBitmapOffset);
                fs.write(userDataBitmap, super.nOfDataBlocks);
                return;
            }
        }
    }
    error("File not found");
}

void Filesystem::copyFromLinux(char* name, unsigned char rights)
{
    load();
    //sprawdzam, czy przypadkiem już nie wgrano takiego pliku
    for (int i = 0; i < super.nOfiNodes; i++)
    {
        if(iNodesBitmap[i] != 0)
        {
            if(strcmp(iNodesTable[i].name, name) == 0)
                error("Could not copy a file: file already exists");
        }
    }
    fstream fileIn(name, ios::binary | ios::in);
    if(!fileIn.good())
        error("Could not copy a file: file error (does it exist?)"); 
    
    //okreslanie wielkosci pliku wejsciowego
    streampos begin, end;
    begin = fileIn.tellg();
    fileIn.seekg(0, ios::end);
    end = fileIn.tellg();
    int fileSize = end - begin;

    //wczytywanie bajtów z pliku
    char data[fileSize];
    fileIn.seekg(0);
    fileIn.read(data, fileSize);
    fileIn.close();

    //sprawdzam, czy w moim systemie jest miejsce na ten plik
    int nOfFreeBlocks = 0;
    for(int i = 0; i < super.nOfDataBlocks; i++)
    {
        if(userDataBitmap[i] == 0)
            nOfFreeBlocks++;
    }
    if(nOfFreeBlocks * (DATA_PER_BLOCK) < fileSize)
        error("Could not copy a file: file is too big");
    
    //przeliczam ile blokow pamieci zajmie plik
    int blocksNeeded = (int)ceil((float)fileSize/(float)(DATA_PER_BLOCK));

    //mamy wszystko w pamieci, szukamy miejsce na inode'a
    for(int i = 0; i < super.nOfiNodes; i++)
    {
        if(iNodesBitmap[i] == 0)
        {
           //mamy pustego inode'a, blokujemy go i szukamy bloków do rozlokowania pliku
           iNodesBitmap[i] = 1;
           int blocksAllocated[blocksNeeded];
           for (int j = 0, counter = 0; counter < blocksNeeded; j++)
           {
               if(userDataBitmap[j] == 0)
               {
                   //mamy pusty blok pamięci, dorzucamy go do listy i blokujemy
                   blocksAllocated[counter] = j;
                   userDataBitmap[j] = 1;
                   counter++; 
               }
           }
            //zapisujemy iNode'a
            iNodesTable[i].firstBlock = blocksAllocated[0];
            iNodesTable[i].rights = rights;
            iNodesTable[i].size = fileSize;
            iNodesTable[i].ownerID = getuid(); 
            time_t now;
            ctime(&now);
            iNodesTable[i].lastAccessed = iNodesTable[i].lastModified = iNodesTable[i].created = now;
            //kopiujemy nazwe
            strcpy(iNodesTable[i].name, name);
            //mamy uzupelnionego iNode'a, czas na dane
            //najpierw linkujemy ze sobą wszystkie bloki składające się na plik
            for(int k = 1; k < blocksNeeded; k++)
                userData[blocksAllocated[k-1]].next = blocksAllocated[k];
            
            //są polinkowane, czas na wpisanie danych
            int bytesLeft = fileSize;
            for(int k = 0; k < blocksNeeded; k++)
            {
                if(bytesLeft >= (DATA_PER_BLOCK))
                {
                    for(int l = 0; l < (DATA_PER_BLOCK); l++)
                        userData[blocksAllocated[k]].data[l] = data[k*(DATA_PER_BLOCK) + l];

                    bytesLeft -= DATA_PER_BLOCK;
                }
                else
                {
                    for(int l = 0; l < bytesLeft; l++)
                        userData[blocksAllocated[k]].data[l] = data[k*DATA_PER_BLOCK + l];
                    
                    bytesLeft = 0;
                }
            }
            //zapisaliśmy wszystko w blockach, inode nadpisany, więc wprowadzamy zmiany do pliku
            save();
            cout << "File copied successfully" << endl;
            return;
        }
    }
    error("Could not copy a file: could not find free iNode");
}


void Filesystem::rmfs()
{
    if(!doesFS_exist())
        error("Could not remove filesystem: it does not exist");
    if(remove(FILESYSTEM_NAME) != 0)
        error("Could not remove filesystem: remove error");
}
    

void Filesystem::copyToLinux(char* name)
{
    load();

    //szukamy pliku o zadanej nazwie
    for(int i = 0; i < super.nOfiNodes; i++)
    {
        if(iNodesBitmap[i] != 0 && strcmp(iNodesTable[i].name, name) == 0)
        {
            //znaleźliśmy szukany plik, szukamy numerów user data bloków, w których jest rozlokowany
            vector<unsigned int> blocks;
            blocks.push_back(iNodesTable[i].firstBlock);
            unsigned int nextBlock = userData[blocks[0]].next;
            while(nextBlock != -1)
            {
                blocks.push_back(nextBlock);
                nextBlock = userData[nextBlock].next;
            }
            //tutaj w blocks mamy po kolei wszystkie bloki zawierające nasz plik
            //tworzymy bufor na dane oraz licznik pozostałych bajtów
            char data[iNodesTable[i].size + 1];
            int bytesLeft = iNodesTable[i].size;

            for(int k = 0; k < blocks.size(); k++)
            {
                if(bytesLeft >= (DATA_PER_BLOCK))
                {
                    for(int l = 0; l < (DATA_PER_BLOCK); l++)
                        data[k*(DATA_PER_BLOCK) + l] = userData[blocks[k]].data[l];

                    bytesLeft -= DATA_PER_BLOCK;
                }
                else
                {
                    for(int l = 0; l < bytesLeft; l++)
                        data[k*DATA_PER_BLOCK + l]= userData[blocks[k]].data[l];
                    
                    bytesLeft = 0;
                }
            }
            //zapisujemy do pliku
            fstream outFile(name, ios::binary | ios::out);
            if(outFile.bad())
                error("Could not copy a file: could not create a file");

            outFile.write(data, iNodesTable[i].size);
            outFile.close();
            cout << "Successfully copied file from filesystem to Linux" << endl;
        }
    }
    error("Could not copy a file: file not found");
}


void Filesystem::showInfo()
{
    load();
    int filesCounter = 0, dataBlocksCounter = 0, spaceUsed = 0;
    for(int i = 0; i < super.nOfiNodes; i++)
    {
        if(iNodesBitmap[i] != 0)
        {
            filesCounter++;
            spaceUsed += iNodesTable[i].size;
        }
    }
    cout << setprecision(2) << fixed;
    cout << "There are " << filesCounter << " files in filesystem, occuping " << spaceUsed <<" bytes of memory out of " << super.nOfDataBlocks * super.blockSize << " (" <<100*((float)spaceUsed/(float)(super.blockSize * super.nOfDataBlocks)) <<"%)" << endl;
    cout << "iNode occupancy bitmap:" <<endl;
    for(int i = 0; i < super.nOfiNodes; i++)
        cout << "|" << (iNodesBitmap[i] == 0 ? 'O' : 'X');
    
    cout << "|" << endl;
    
    //liczymy zajęte bloki pamięci
    for(int i = 0; i < super.nOfDataBlocks; i++)
    {
        if(userDataBitmap[i] != 0) 
            dataBlocksCounter++;
    }

    cout << "There are " << dataBlocksCounter << " occupied data blocks out of " << super.nOfDataBlocks << "avaible (" << 100*((float)dataBlocksCounter/(float)super.nOfDataBlocks) << endl;
    cout << "As we are using linked list allocation, there is no external fragmentation." << endl;
    cout << spaceUsed <<" bytes of memory are used out of " << dataBlocksCounter * DATA_PER_BLOCK << " allocated (" << 100*((float)spaceUsed/(float)(dataBlocksCounter * DATA_PER_BLOCK)) << "%)" <<endl;

    cout << "Data blocks occupancy bitmap:" <<endl;
    for(int i = 0; i < super.nOfDataBlocks ;i++)
        cout << "|" << (userDataBitmap[i] == 0 ? 'O' : 'X');

    cout <<"|" << endl;

}