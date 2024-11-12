#ifndef CACHE_GUARD
#define CACHE_GUARD

#include <vector>
#include <cmath>
#include "simulator.hh"

class CACHE
{
private:
    struct line
    {
        bool valid;
        bool dirty;
        int RPdata;
        int tag;
        std::vector<u_int8_t> block;
        line()
        {
            dirty = valid = false;
        }
    };

    enum replacementPolicy
    {
        FIFO,
        LRU,
        RANDOM
    };
    enum writePolicy
    {
        WB,
        WT
    };

    std::ofstream file;

    replacementPolicy RP;
    writePolicy WP;
    int timeCounter;
    int misses;
    int hits;

    int cacheSize;
    int noOfLines;
    int associativity;
    int blockSize;
    int blockOffset;
    std::vector<std::vector<line>> table;
    int findVictim(int hashValue);
    int checkHitOrMiss(int hashValue, int tag);

public:
    CACHE(int cacheSize, int blockSize, int associativity, std::string replacementPolicy, std::string writePolicy);
    long long read(simulator &sim, int address, int size, bool isSigned);
    void write(simulator &sim, long long data, int address, int size);
    void printStatus();
    void invalidate(simulator& sim);
    void printStats();
    void printCache(std::string fileName);
};

#endif