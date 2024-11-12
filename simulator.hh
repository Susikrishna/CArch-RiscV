#ifndef SIMULATOR_GUARD
#define SIMULATOR_GUARD

#include <string>
#include <vector>
#include <map>
#include <set>
#include <iomanip>
#include <iostream>
#include <fstream>

class CACHE;
class simulator
{
private:
    std::string fileName;
    u_int8_t memory[0x50001];
    long long registers[32];
    std::vector<std::vector<std::string>> lines;
    long long PC;
    long long MC;
    long long lineCounter;
    bool error;
    std::vector<std::pair<std::string, int>> Stack;
    std::map<std::string, std::pair<long long, long long>> Labels;
    std::set<long long> breakPoints;
    bool cacheEnabled;
    CACHE *cacheSim;

    void reset();

    void printError(std::string s);

    void checkProperLabel(std::string s);

    long long loadData(int index, int size, bool isSigned);

    void storeData(long long data, int index, int size);

    long long solveImmediateSigned(std::string s, int max);

    long long solveImmediateNonNegative(std::string s, int max);

    long long solveDataFromDataSection(std::string s, int max);

    int solveRegister(std::string s);

    void preprocess(std::string s);

    void storeInstructions(std::string fileName);

    long long findLineNumberWrtPC(long long PC);

public:
    friend class CACHE;

    simulator()
    {
        cacheEnabled = false;
    }
    
    void run(bool step);

    void load(std::string fileName);

    void printRegisters();

    void printMemory(std::string index, std::string count);

    void showStack();

    void addBreakPoint(int lineNumber);

    void deleteBreakpoint(int lineNumber);

    void enableCache(std::string fileName);

    void disableCache();

    void printStatus();

    void printStats();

    void invalidateCache();

    void printCache(std::string fileName);
};

#endif