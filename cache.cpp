#include "cache.hh"
#include "string"
#include "fstream"
#include "vector"
#include <cstdlib>

CACHE::CACHE(int cacheSize, int blockSize, int associativity, std::string replacementPolicy, std::string writePolicy)
{
    file.close();

    if (writePolicy == "WT")
        WP = WT;
    else
        WP = WB;

    if (replacementPolicy == "FIFO")
        RP = FIFO;
    else if (replacementPolicy == "LRU")
        RP = LRU;
    else
        RP = RANDOM;

    this->cacheSize = cacheSize;
    this->blockSize = blockSize;
    this->blockOffset = std::log2(blockSize);
    if (associativity == 0)
        this->associativity = cacheSize / blockSize;
    else
        this->associativity = associativity;
    this->noOfLines = cacheSize / (blockSize * associativity);
    this->timeCounter = this->hits = this->misses = 0;

    for (int i = 0; i < noOfLines; i++)
    {
        table.push_back(std::vector<line>{});
        for (int j = 0; j < associativity; j++)
        {
            table[i].push_back(line{});
            for (int k = 0; k < blockSize; k++)
                table[i][j].block.push_back(0);
        }
    }
}

int CACHE::findVictim(int hashValue)
{
    if (RP == RANDOM)
        return std::rand() % associativity;

    int index, hold = INT32_MAX;
    for (int i = 0; i < table[hashValue].size(); i++)
        if (!table[hashValue][i].valid)
            return i;
        else if (table[hashValue][i].RPdata < hold)
        {
            hold = table[hashValue][i].RPdata;
            index = i;
        }
    return index;
}

int CACHE::checkHitOrMiss(int hashValue, int tag)
{
    for (int i = 0; i < table[hashValue].size(); i++)
        if (table[hashValue][i].valid && table[hashValue][i].tag == tag)
            return i;
    return -1;
}

long long CACHE::read(simulator &sim, int address, int size, bool isSigned)
{
    int blockIndex = address % blockSize;
    int hashValue = (address >> blockOffset) % noOfLines;
    int tag = address >> (int(log2(noOfLines)) + blockOffset);

    int toBeReplacedIndex = checkHitOrMiss(hashValue, tag);
    if (toBeReplacedIndex == -1)
    {
        misses++;
        toBeReplacedIndex = findVictim(hashValue);
        if (WP == WB && table[hashValue][toBeReplacedIndex].valid && table[hashValue][toBeReplacedIndex].dirty)
        {
            int dummy = ((table[hashValue][toBeReplacedIndex].tag << int(log2(noOfLines))) + hashValue) << blockOffset;
            for (int i = 0; i < blockSize; i++)
                sim.memory[dummy++] = table[hashValue][toBeReplacedIndex].block[i];
        }

        int dummy = (address >> blockOffset) << blockOffset;
        for (int i = 0; i < blockSize; i++)
            table[hashValue][toBeReplacedIndex].block[i] = sim.memory[dummy++];

        table[hashValue][toBeReplacedIndex].RPdata = timeCounter++;
        table[hashValue][toBeReplacedIndex].valid = true;
        table[hashValue][toBeReplacedIndex].dirty = false;
        table[hashValue][toBeReplacedIndex].tag = tag;

        if (!this->file.is_open())
            this->file.open(sim.fileName.substr(0, sim.fileName.find('.')) + ".output", std::ios::app);
        file << "R: Address: 0x" << std::hex << address << ", Set: 0x" << hashValue << ", Miss, Tag: 0x" << tag << (table[hashValue][toBeReplacedIndex].dirty ? ", Dirty" : ", Clean") << std::endl;
    }
    else
    {
        hits++;
        if (RP == LRU)
            table[hashValue][toBeReplacedIndex].RPdata = timeCounter++;
        if (!this->file.is_open())
            this->file.open(sim.fileName.substr(0, sim.fileName.find('.')) + ".output", std::ios::app);
        file << "R: Address: 0x" << std::hex << address << ", Set: 0x" << hashValue << ", Hit, Tag: 0x" << tag << (table[hashValue][toBeReplacedIndex].dirty ? ", Dirty" : ", Clean") << std::endl;
    }

    long long data = 0;
    for (int i = 0; i < size; i += 8)
        data = data | ((long long)(table[hashValue][toBeReplacedIndex].block[blockIndex++]) << i);

    if (isSigned && (data >> (size - 1)) == 1)
        data = data | (~0ULL << size);
    return data;
}

void CACHE::write(simulator &sim, long long data, int address, int size)
{
    int blockIndex = address % blockSize;
    int hashValue = (address >> blockOffset) % noOfLines;
    int tag = address >> (int(log2(noOfLines)) + blockOffset);

    int toBeReplacedIndex = checkHitOrMiss(hashValue, tag);
    if (toBeReplacedIndex == -1)
    {
        misses++;
        toBeReplacedIndex = findVictim(hashValue);
        if (WP == WB)
        {
            if (table[hashValue][toBeReplacedIndex].valid && table[hashValue][toBeReplacedIndex].dirty)
            {
                int dummy = ((table[hashValue][toBeReplacedIndex].tag << int(log2(noOfLines))) + hashValue) << blockOffset;
                for (int i = 0; i < blockSize; i++)
                    sim.memory[dummy++] = table[hashValue][toBeReplacedIndex].block[i];
            }
            int dummy = (address >> blockOffset) << blockOffset;
            for (int i = 0; i < blockSize; i++)
                table[hashValue][toBeReplacedIndex].block[i] = sim.memory[dummy++];

            for (int i = 0; i < size; i = i + 8)
                table[hashValue][toBeReplacedIndex].block[blockIndex++] = (data >> i) & 0b11111111;

            table[hashValue][toBeReplacedIndex].valid = table[hashValue][toBeReplacedIndex].dirty = true;
            table[hashValue][toBeReplacedIndex].RPdata = timeCounter++;
            table[hashValue][toBeReplacedIndex].tag = tag;
        }
        else
        {
            int dummy = address;
            for (int i = 0; i < size; i = i + 8)
                sim.memory[dummy++] = (data >> i) & 0b11111111;
        }

        if (!this->file.is_open())
            this->file.open(sim.fileName.substr(0, sim.fileName.find('.')) + ".output", std::ios::app);
        file << "W: Address: 0x" << std::hex << address << ", Set: 0x" << hashValue << ", Miss, Tag: 0x" << tag << (table[hashValue][toBeReplacedIndex].dirty ? ", Dirty" : ", Clean") << std::endl;
    }
    else
    {
        hits++;
        int dummy = address;
        for (int i = 0; i < size; i = i + 8)
        {
            table[hashValue][toBeReplacedIndex].block[blockIndex++] = (data >> i) & 0b11111111;
            if (WP == WT)
                sim.memory[dummy++] = (data >> i) & 0b11111111;
        }
        if (WP == WB)
            table[hashValue][toBeReplacedIndex].dirty = true;
        if (RP == LRU)
            table[hashValue][toBeReplacedIndex].RPdata = timeCounter++;

        if (!this->file.is_open())
            this->file.open(sim.fileName.substr(0, sim.fileName.find('.')) + ".output", std::ios::app);
        file << "W: Address: 0x" << std::hex << address << ", Set: 0x" << hashValue << ", Hit, Tag: 0x" << tag << (table[hashValue][toBeReplacedIndex].dirty ? ", Dirty" : ", Clean") << std::endl;
    }
}

void CACHE::printStatus()
{
    std::cout << "Cache Size: " << cacheSize << std::endl
              << "Block Size: " << blockSize << std::endl
              << "Associativity: " << associativity << std::endl
              << "Replacement Policy: " << (RP == LRU ? "LRU" : (RP == FIFO ? "FIFO" : "RANDOM")) << std::endl
              << "Write Back Policy: " << (WP == WT ? "WT" : "WB") << std::endl;
}

void CACHE::invalidate(simulator &sim)
{
    for (int i = 0; i < noOfLines; i++)
    {
        for (int j = 0; j < associativity; j++)
        {
            if (table[i][j].valid && table[i][j].dirty)
            {
                int dummy = ((table[i][j].tag << int(log2(noOfLines))) + i) << blockOffset;
                for (int k = 0; k < blockSize; k++)
                    sim.memory[dummy++] = table[i][j].block[k];
            }
            table[i][j].valid = false;
        }
    }
}

void CACHE::printStats()
{
    std::cout << "D-cache statistics: Accesses=" << hits + misses << ", Hit=" << hits << ", Miss=" << misses << ", Hit Rate=" << std::setprecision(2) << (double)hits / (hits + misses) << std::endl;
}

void CACHE::printCache(std::string fileName)
{
    std::ofstream output(fileName);
    for (int i = 0; i < noOfLines; i++)
        for (int j = 0; j < associativity; j++)
            if (table[i][j].valid)
                output << "Set: 0x" << std::hex << i << ", Tag: 0x" << table[i][j].tag << ", " << (table[i][j].dirty ? "Dirty" : "Clean") << std::endl;
    output.close();
}