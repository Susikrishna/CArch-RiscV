#include <iostream>
#include <iomanip>
#include <cstring>
#include <fstream>
#include <sstream>
#include <set>
#include <map>
#include <cmath>
#include <vector>
#include "utilities.hh"
#include "simulator.hh"

int main()
{
    simulator test;
    bool loaded = false;
    while (true)
    {
        std::string input, command, errorChecker;
        getline(std::cin, input);

        std::stringstream ss(input);
        ss >> command;

        if (command == "load")
        {
            std::string fileName;
            ss >> fileName;
            getline(ss, errorChecker);
            if (fileName.empty() || !errorChecker.empty())
                std::cout << "Invalid Command, Expected: load <filename>" << std::endl;
            else
            {
                test.load(fileName);
                loaded = true;
            }
        }
        else if (command == "regs")
        {
            getline(ss, errorChecker);
            if (!errorChecker.empty())
                std::cout << "Invalid Command, Expected: regs" << std::endl;
            else
                test.printRegisters();
        }
        else if (command == "mem")
        {
            std::string index, count;
            ss >> index >> count;
            getline(ss, errorChecker);
            if (index.empty() || count.empty() || !errorChecker.empty())
                std::cout << "Invalid Command, Expected: mem <index> <count>" << std::endl;
            else
                test.printMemory(index, count);
        }
        else if (command == "show-stack")
        {
            getline(ss, errorChecker);
            if (!errorChecker.empty())
                std::cout << "Invalid Command, Expected: show-stack" << std::endl;
            else
                test.showStack();
        }
        else if (command == "cache_sim")
        {
            std::string subCommand, fileName;
            ss >> subCommand >> fileName;
            if (subCommand == "enable")
                if (loaded)
                    std::cout << "Cache cannot be enabled after the file is loaded" << std::endl;
                else
                    test.enableCache(fileName);
            else if (subCommand == "disable" && fileName == "")
                if (loaded)
                    std::cout << "Cache cannot be disabled after the file is loaded" << std::endl;
                else
                    test.disableCache();
            else if (subCommand == "status" && fileName.empty())
                test.printStatus();
            else if (subCommand == "stats" && fileName.empty())
                test.printStats();
            else if (subCommand == "invalidate" && fileName.empty())
                test.invalidateCache();
            else if (subCommand == "dump")
                test.printCache(fileName);
            else
                std::cout << "Invalid cache command" << std::endl;
        }
        else if (command == "exit")
        {
            getline(ss, errorChecker);
            if (!errorChecker.empty())
                std::cout << "Wrong command: Expected: exit" << std::endl;
            else
            {
                std::cout << "Exited the simulator" << std::endl;
                return 0;
            }
        }
        else if (loaded)
        {
            if (command == "run")
            {
                getline(ss, errorChecker);
                if (!errorChecker.empty())
                    std::cout << "Invalid Command, Expected: run" << std::endl;
                else
                    test.run(false);
            }
            else if (command == "step")
            {
                getline(ss, errorChecker);
                if (!errorChecker.empty())
                    std::cout << "Invalid Command, Expected: step" << std::endl;
                else
                    test.run(true);
            }
            else if (command == "break")
            {
                std::string lineNumber;
                ss >> lineNumber;
                getline(ss, errorChecker);

                if (lineNumber.empty() || !errorChecker.empty() || !utilities::checkBase10(lineNumber))
                    std::cout << "Invalid Command, Expected: break <line number>" << std::endl;
                else
                    test.addBreakPoint(stoll(lineNumber));
            }
            else if (command == "del")
            {
                std::string lineNumber, checkBreak;
                ss >> checkBreak >> lineNumber;
                getline(ss, errorChecker);

                if (checkBreak != "break" || lineNumber.empty() || !errorChecker.empty() || !utilities::checkBase10(lineNumber))
                    std::cout << "Invalid Command, Expected: del break <line number>" << std::endl;
                else
                    test.deleteBreakpoint(stoll(lineNumber));
            }
        }
        else
            std::cout << "Command Not found or File not loaded" << std::endl;
    }
}