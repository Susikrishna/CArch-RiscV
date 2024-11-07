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

struct info
{
    int opcode;
    int func3;
    int func7;
};

std::map<std::string, std::string> riscv_registers = {
    {"zero", "x0"}, {"ra", "x1"}, {"sp", "x2"}, {"gp", "x3"}, {"tp", "x4"}, {"t0", "x5"}, {"t1", "x6"}, {"t2", "x7"}, {"s0", "x8"}, {"fp", "x8"}, {"s1", "x9"}, {"a0", "x10"}, {"a1", "x11"}, {"a2", "x12"}, {"a3", "x13"}, {"a4", "x14"}, {"a5", "x15"}, {"a6", "x16"}, {"a7", "x17"}, {"s2", "x18"}, {"s3", "x19"}, {"s4", "x20"}, {"s5", "x21"}, {"s6", "x22"}, {"s7", "x23"}, {"s8", "x24"}, {"s9", "x25"}, {"s10", "x26"}, {"s11", "x27"}, {"t3", "x28"}, {"t4", "x29"}, {"t5", "x30"}, {"t6", "x31"}};

std::map<std::string, info> riscInfo = {
    {"lui", {0b0110111, -1, -1}},
    {"auipc", {0b0010111, -1, -1}},
    {"jal", {0b1101111, -1, -1}},
    {"jalr", {0b1100111, 0x0, -1}},
    {"beq", {0b1100011, 0x0, -1}},
    {"bne", {0b1100011, 0x1, -1}},
    {"blt", {0b1100011, 0x4, -1}},
    {"bge", {0b1100011, 0x5, -1}},
    {"bltu", {0b1100011, 0x6, -1}},
    {"bgeu", {0b1100011, 0x7, -1}},
    {"lb", {0b0000011, 0x0, -1}},
    {"lh", {0b0000011, 0x1, -1}},
    {"lw", {0b0000011, 0x2, -1}},
    {"ld", {0b0000011, 0x3, -1}},
    {"lbu", {0b0000011, 0x4, -1}},
    {"lhu", {0b0000011, 0x5, -1}},
    {"lwu", {0b0000011, 0x6, -1}},
    {"sb", {0b0100011, 0x0, -1}},
    {"sh", {0b0100011, 0x1, -1}},
    {"sw", {0b0100011, 0x2, -1}},
    {"sd", {0b0100011, 0x3, -1}},
    {"addi", {0b0010011, 0x0, -1}},
    {"slti", {0b0010011, 0x2, -1}},
    {"sltiu", {0b0010011, 0x3, -1}},
    {"xori", {0b0010011, 0x4, -1}},
    {"ori", {0b0010011, 0x6, -1}},
    {"andi", {0b0010011, 0x7, -1}},
    {"slli", {0b0010011, 0x1, 0x0}},
    {"srli", {0b0010011, 0x5, 0x0}},
    {"srai", {0b0010011, 0x5, 0x10}},
    {"add", {0b0110011, 0x0, 0x0}},
    {"sub", {0b0110011, 0x0, 0x20}},
    {"sll", {0b0110011, 0x1, 0x0}},
    {"slt", {0b0110011, 0x2, 0x0}},
    {"sltu", {0b0110011, 0x3, 0x0}},
    {"xor", {0b0110011, 0x4, 0x0}},
    {"srl", {0b0110011, 0x5, 0x0}},
    {"sra", {0b0110011, 0x5, 0x20}},
    {"or", {0b0110011, 0x6, 0x0}},
    {"and", {0b0110011, 0x7, 0x0}},
};

class simulator
{
private:
    u_int8_t memory[0x50001];
    long long registers[32];
    std::vector<std::vector<std::string>> lines;
    long long PC;
    long long MC;
    long long lineCounter;
    bool loaded;
    bool error;
    std::vector<std::pair<std::string, int>> Stack;
    std::map<std::string, std::pair<long long, long long>> Labels;
    std::set<long long> breakPoints;

    void reset()
    {
        PC = 0;
        lineCounter = 1;
        MC = 0x10000;
        loaded = false;
        error = false;

        Stack.clear();
        Stack.push_back({"main", 0});

        lines.clear();
        lines.push_back(std::vector<std::string>{});

        Labels.clear();
        breakPoints.clear();
        memset(memory, 0, sizeof(memory));
        memset(registers, 0, sizeof(registers));
    }

    void printError(std::string s)
    {
        std::cerr << "Line " << lineCounter << ": " << s << std::endl;
        error = true;
    }

    void checkProperLabel(std::string s)
    {
        if (s.size() == 0)
            printError("No Label is provided");
        if (!(utilities::checkIfValueIsInBetween(s[0], 'A', 'Z') || utilities::checkIfValueIsInBetween(s[0], 'a', 'z') || s[0] == '_'))
            printError("Invalid Label Name: A Label should begin only with underscore or alphabet");
        for (auto c : s.substr(1))
            if (!(utilities::checkIfValueIsInBetween(c, 'A', 'Z') || utilities::checkIfValueIsInBetween(c, 'a', 'z') || c == '_' || utilities::checkIfValueIsInBetween(c, '0', '9')))
                printError("Invalid Label Name: Symbols and spaces can't be used");
    }

    long long loadData(int index, int size, bool isSigned)
    {
        if (index > 0x50000 || index < 0)
            printError("Address Out of range");
        else
        {
            long long data = 0;
            for (int i = 0; i < size; i += 8)
                data = data | ((long long)memory[index++] << i);

            if (isSigned && (data >> (size - 1)) == 1)
                data = data | (~0ULL << size);
            return data;
        }
        return 0;
    }

    void storeData(long long data, int index, int size)
    {
        if (index > 0x50000 || index < 0)
            printError("Address Out of range");
        else
            for (int i = 0; i < size; i = i + 8)
                memory[index++] = (data >> i) & 0b11111111;
    }

    long long solveImmediateSigned(std::string s, int max)
    {
        try
        {
            long long value;
            if (s[0] == '-' ? s[1] == '0' && s[2] == 'x' && utilities::checkBase16(s.substr(3)) : s[0] == '0' && s[1] == 'x' && utilities::checkBase16(s.substr(2)))
                value = stoull(s, nullptr, 16);
            else if (s[0] == '-' ? utilities::checkBase10(s.substr(1)) : utilities::checkBase10(s))
                value = stoull(s);
            else
                printError("Invalid Immediate value");
            // Check if the value can be stored in given number of bits.
            if (pow(2, max - 1) > value && value >= -pow(2, max - 1))
                return value;
            else
                printError("The Immediate value is too big to fit in " + std::to_string(max) + " bits");
        }
        catch (const std::exception &e)
        {
            printError("The Immediate value is too big to fit in " + std::to_string(max) + " bits");
        }
        return 0;
    }

    long long solveImmediateNonNegative(std::string s, int max)
    {
        try
        {
            long long value;
            if (s[0] == '-')
                printError("The immediate value cannot be negative for this instruction");
            else if (s[0] == '0' && s[1] == 'x' && utilities::checkBase16(s.substr(2)))
                value = stoll(s, nullptr, 16);
            else if (utilities::checkBase10(s))
                value = stoll(s);
            else
                printError("Invalid Immediate value");
            // Check if the value can be stored in given number of bits.
            if (pow(2, max) > value && value >= 0)
                return value;

            printError("The Immediate value is too big to fit in " + std::to_string(max) + " bits");
        }
        catch (const std::exception &e)
        {
            printError("The Immediate value is too big to fit in " + std::to_string(max) + " bits");
        }
        return 0;
    }

    long long solveDataFromDataSection(std::string s, int max)
    {
        try
        {
            long long value;
            if (s[0] == '-' ? s[1] == '0' && s[2] == 'x' && utilities::checkBase16(s.substr(3)) : s[0] == '0' && s[1] == 'x' && utilities::checkBase16(s.substr(2)))
                value = stoull(s, nullptr, 16);
            else if (s[0] == '-' ? utilities::checkBase10(s.substr(1)) : utilities::checkBase10(s))
                value = stoull(s);
            else
                printError("Invalid data value");
            // Check if the value can be stored in given number of bits.
            if (pow(2, max) > value && value >= -pow(2, max - 1))
                return value;
            else
                printError("The value is too big to fit in " + std::to_string(max) + " bits");
        }
        catch (const std::exception &e)
        {
            printError("The value is too big to fit in " + std::to_string(max) + " bits");
        }
        return 0;
    }

    int solveRegister(std::string s)
    {
        // Check if the register is alias to some register
        auto index = riscv_registers.find(s);
        if (index != riscv_registers.end())
            s = index->second;

        if (s[0] == 'x' && utilities::checkIfValueIsInBetween(s[1], '0', '9') && (s.size() == 3 ? (s[1] != '0' && utilities::checkIfValueIsInBetween(s[1], '1', '9')) : 1))
        {
            int ans;
            if (s.size() == 2)
                ans = s[1] - '0';
            else
                ans = (s[1] - '0') * 10 + s[2] - '0';
            if (ans < 32)
                return ans;
        }
        printError(s + " is a Invalid register");
        return 0;
    }

    void preprocess(std::string s)
    {
        std::ifstream inputFile(s);
        std::string line;
        while (getline(inputFile, line))
        {
            utilities::format(line);
            std::vector<std::string> v;
            if (line[0] == '.')
            {
                if (line == ".data")
                {
                    lines.push_back(v);
                    lineCounter++;
                    while (getline(inputFile, line))
                    {
                        lines.push_back(v);
                        utilities::format(line);
                        if (line.empty())
                            continue;

                        std::stringstream ss(line);
                        std::string type;
                        ss >> type;

                        if (type == ".dword")
                            while (ss >> type)
                            {
                                storeData(solveDataFromDataSection(type, 64), MC, 64);
                                MC += 8;
                            }
                        else if (type == ".word")
                            while (ss >> type)
                            {
                                storeData(solveDataFromDataSection(type, 32), MC, 32);
                                MC += 4;
                            }
                        else if (type == ".half")
                            while (ss >> type)
                            {
                                storeData(solveDataFromDataSection(type, 16), MC, 16);
                                MC += 2;
                            }
                        else if (type == ".byte")
                            while (ss >> type)
                            {
                                storeData(solveDataFromDataSection(type, 8), MC, 8);
                                MC += 1;
                            }
                        else if (type == ".text")
                        {
                            lineCounter++;
                            break;
                        }
                        else
                            printError("Invalid directive inside .data");
                        lineCounter++;
                    }
                }
                else if (line == ".text")
                {
                    lines.push_back(v);
                    lineCounter++;
                }
                else
                    printError("Illegal directives");
            }
            else
            {
                long long index = line.find(':');
                if (index != std::string::npos)
                {
                    // checks for duplicate Labels
                    if (Labels.find(line.substr(0, index)) == Labels.end())
                    {
                        checkProperLabel(line.substr(0, index));
                        Labels.insert({line.substr(0, index), {PC, lineCounter}});
                        if (line.substr(0, index) == "main")
                            Stack[0].second = lineCounter - 1;
                    }
                    else
                        printError("Duplicate Label name");
                }
                else
                    index = -1;
                utilities::spiltLineIntoWords(line.substr(index + 1), v);
                lines.push_back(v);
                // If no instruction follows the label or line empty, PC is not updated
                if (index != line.size() - 1)
                    PC += 4;
                lineCounter++;
            }
        }
        inputFile.close();
    }

    void storeInstructions(std::string fileName)
    {
        preprocess(fileName);
        lineCounter = 1;
        PC = 0;

        while (lineCounter < lines.size())
        {
            std::vector<std::string> v = lines[lineCounter];
            if (v.empty())
            {
                lineCounter++;
                continue;
            }

            int encode = 0;
            std::map<std::string, std::pair<long long, long long>>::iterator index;

            switch (riscInfo[v[0]].opcode)
            {
            // R type Instructions
            case 0b0110011:
                // If number of arguments don't match
                if (v.size() != 4)
                    printError("Wrong arguments: Expected rd, rs1, rs2");
                encode += 0b0110011;
                encode += riscInfo[v[0]].func3 << 12;
                encode += riscInfo[v[0]].func7 << 25;
                encode += solveRegister(v[1]) << 7;
                encode += solveRegister(v[2]) << 15;
                encode += solveRegister(v[3]) << 20;
                break;

            // I Type Instructions (Arthemetic)
            case 0b0010011:
                // If number of arguments don't match
                if (v.size() != 4)
                    printError("Wrong arguments: Expected rd, rs1, imm");
                encode += 0b0010011;
                encode += riscInfo[v[0]].func3 << 12;
                encode += solveRegister(v[1]) << 7;
                encode += solveRegister(v[2]) << 15;
                // For srai,srli,slli as they have func6 parameters
                if (riscInfo[v[0]].func7 != -1)
                {
                    encode += solveImmediateNonNegative(v[3], 6) << 20;
                    encode += riscInfo[v[0]].func7 << 26;
                }
                else
                    encode += solveImmediateSigned(v[3], 12) << 20;
                break;
            // I type Instructions (Load instructions)
            case 0b0000011:
                encode += 0b0000011;
                encode += riscInfo[v[0]].func3 << 12;
                encode += solveRegister(v[1]) << 7;
                if (v.size() == 4)
                {
                    encode += solveRegister(v[3]) << 15;
                    encode += solveImmediateSigned(v[2], 12) << 20;
                }
                else if (v.size() == 3)
                {
                    int start = v[2].find('(');
                    int end = v[2].find(')');
                    if (start == std::string::npos || end == std::string::npos)
                        printError("Wrong arguments: Expected rd, imm, rs1 or rd, imm(rs1)");
                    encode += solveRegister(v[2].substr(start + 1, end - start - 1)) << 15;
                    encode += solveImmediateSigned(v[2].substr(0, start), 12) << 20;
                }
                else
                    printError("Wrong arguments: Expected rd, imm, rs1 or rd, imm(rs1)");
                break;

            // S type instructions
            case 0b0100011:
                encode += 0b0100011;
                encode += riscInfo[v[0]].func3 << 12;
                encode += solveRegister(v[1]) << 20;
                if (v.size() == 4)
                {
                    encode += solveRegister(v[3]) << 15;
                    int immediate = solveImmediateSigned(v[2], 12);
                    encode += (immediate & 0b11111) << 7;
                    encode += (immediate >> 5) << 25;
                }
                else if (v.size() == 3)
                {
                    int start = v[2].find('(');
                    int end = v[2].find(')');
                    if (start == std::string::npos || end == std::string::npos)
                        printError("Wrong arguments: Expected rd, imm, rs1 or rd, imm(rs1)");
                    encode += solveRegister(v[2].substr(start + 1, end - start - 1)) << 15;
                    int immediate = solveImmediateSigned(v[2].substr(0, start), 12);
                    encode += (immediate & 0b11111) << 7;
                    encode += (immediate >> 5) << 25;
                }
                else
                    printError("Wrong arguments : Expected rs2, imm, rs1 or rs2, imm(rs1)");
                break;

            // B type instruction
            case 0b1100011:
                if (v.size() != 4)
                    printError("Wrong arguments: Expected rs1, rs2, imm");
                encode += 0b1100011;
                encode += riscInfo[v[0]].func3 << 12;
                encode += solveRegister(v[1]) << 15;
                encode += solveRegister(v[2]) << 20;
                index = Labels.find(v[3]);
                if (index != Labels.end())
                {
                    int offset = index->second.first - PC;
                    encode += ((offset & 0b11110) | (offset >> 11 & 0b1)) << 7;
                    encode += (((offset >> 5) & 0b111111) | ((offset >> 12 & 0b1) << 6)) << 25;
                }
                else if ((v[3][0] == '-' ? utilities::checkBase10(v[3].substr(1)) : utilities::checkBase10(v[3])) || (v[3][0] == '-' ? v[3][1] == '0' && v[3][2] == 'x' && utilities::checkBase16(v[3].substr(3)) : v[3][0] == '0' && v[3][1] == 'x' && utilities::checkBase16(v[3].substr(2))))
                {
                    int offset = solveImmediateSigned(v[3], 13);
                    encode += ((offset & 0b11110) | (offset >> 11 & 0b1)) << 7;
                    encode += (((offset >> 5) & 0b111111) | ((offset >> 12 & 0b1) << 6)) << 25;
                }
                else
                    printError(" The Label " + v[3] + " doesn't exist");
                break;

            // JAL instruction
            case 0b1101111:
                if (v.size() != 3)
                    printError("Invalid Arguments: Expected jal rd, imm");
                encode += 0b1101111;
                encode += solveRegister(v[1]) << 7;
                index = Labels.find(v[2]);
                if (index != Labels.end())
                {
                    int offset = index->second.first - PC;
                    encode += ((offset >> 12) & 0b11111111) << 12;
                    encode += ((offset >> 11) & 0b1) << 20;
                    encode += ((offset >> 1) & 0b1111111111) << 21;
                    encode += ((offset >> 20) & 0b1) << 31;
                }
                else if ((v[2][0] == '-' ? utilities::checkBase10(v[2].substr(1)) : utilities::checkBase10(v[2])) || (v[2][0] == '-' ? v[2][1] == '0' && v[2][2] == 'x' && utilities::checkBase16(v[2].substr(3)) : v[2][0] == '0' && v[2][1] == 'x' && utilities::checkBase16(v[2].substr(2))))
                {
                    int offset = solveImmediateSigned(v[2], 21);
                    encode += ((offset >> 12) & 0b11111111) << 12;
                    encode += ((offset >> 11) & 0b1) << 20;
                    encode += ((offset >> 1) & 0b1111111111) << 21;
                    encode += ((offset >> 20) & 0b1) << 31;
                }
                else
                    printError("The Label " + v[3] + " doesn't exist");
                break;

            // JALR Instruction
            case 0b1100111:
                encode += 0b1100111;
                encode += riscInfo[v[0]].func3 << 12;
                encode += solveRegister(v[1]) << 7;
                if (v.size() == 4)
                {
                    encode += solveRegister(v[2]) << 15;
                    encode += solveImmediateSigned(v[3], 12) << 20;
                }
                else if (v.size() == 3)
                {
                    int start = v[2].find('(');
                    int end = v[2].find(')');
                    if (start == std::string::npos || end == std::string::npos)
                        printError("Wrong arguments: Expected jalr rd, rs1, imm or jalr rd, imm(rs1)");
                    encode += solveRegister(v[2].substr(start + 1, end - start - 1)) << 15;
                    encode += solveImmediateSigned(v[2].substr(0, start), 12) << 20;
                }
                else
                    printError("Wrong arguments: Expected jalr rd, rs1, imm or jalr rd, imm(rs1)");
                break;

            // LUI Instruction
            case 0b0110111:
                if (v.size() != 3)
                    printError("Wrong arguments: Expected lui rs1, imm");

                encode += 0b0110111;
                encode += solveRegister(v[1]) << 7;
                encode += solveImmediateNonNegative(v[2], 20) << 12;
                break;

            default:
                printError("The instruction " + v[0] + " doesn't exist");
            }
            storeData(encode, PC, 32);
            PC += 4;
            lineCounter++;
        }
    }

    long long findLineNumberWrtPC(long long PC)
    {
        int dummyPC = 0;
        int lineNumber = 1;
        while (dummyPC != PC && lineNumber < lines.size())
            if (!lines[lineNumber++].empty())
                dummyPC += 4;

        return lineNumber;
    }

public:
    void run(bool step)
    {
        if (!loaded || error)
        {
            std::cout << "File not loaded or there is some error in the file" << std::endl;
            return;
        }

        if (lineCounter >= lines.size())
            if (step)
                std::cout << "Nothing to step" << std::endl;
            else
                std::cout << "Nothing to run" << std::endl;
        else
            do
            {
                auto checkbreakPoint = breakPoints.find(lineCounter);
                if (!step || checkbreakPoint != breakPoints.end())
                {
                    std::cout << "Execution stopped at breakpoint" << std::endl;
                    break;
                }

                std::vector<std::string> v = lines[lineCounter];
                while (v.empty() && lineCounter < lines.size())
                    v = lines[++lineCounter];

                Stack[Stack.size() - 1].second = lineCounter;

                std::cout << "Executed";
                for (int i = 0; i < v.size(); i++)
                {
                    std::cout << ' ' << v[i] << ((i == 0 || i == v.size() - 1) ? "" : ",");
                }
                std::cout << "; PC=0x" << std::hex << std::setw(8) << std::setfill('0') << PC << std::endl;

                bool doJump = false;
                std::map<std::string, std::pair<long long, long long>>::iterator index;
                switch (riscInfo[v[0]].opcode)
                {
                // R Type Instructions
                case 0b0110011:
                    switch (riscInfo[v[0]].func3)
                    {
                    case 0x0:
                        if (riscInfo[v[0]].func7 == 0x0)
                            registers[solveRegister(v[1])] = registers[solveRegister(v[2])] + registers[solveRegister(v[3])];
                        else
                            registers[solveRegister(v[1])] = registers[solveRegister(v[2])] - registers[solveRegister(v[3])];
                        break;
                    case 0x1:
                        registers[solveRegister(v[1])] = registers[solveRegister(v[2])] << (registers[solveRegister(v[3])] % 64);
                        break;
                    case 0x2:
                        registers[solveRegister(v[1])] = registers[solveRegister(v[2])] < registers[solveRegister(v[3])];
                        break;
                    case 0x3:
                        registers[solveRegister(v[1])] = (unsigned long long)registers[solveRegister(v[2])] < (unsigned long long)registers[solveRegister(v[3])];
                        break;
                    case 0x4:
                        registers[solveRegister(v[1])] = registers[solveRegister(v[2])] ^ registers[solveRegister(v[3])];
                        break;
                    case 0x5:
                        if (riscInfo[v[0]].func7 == 0x0)
                            registers[solveRegister(v[1])] = registers[solveRegister(v[2])] >> (registers[solveRegister(v[3])] % 64);
                        else
                        {
                            if (registers[solveRegister(v[2])] >> 63 == 1)
                                registers[solveRegister(v[1])] = (registers[solveRegister(v[2])] >> (registers[solveRegister(v[3])] % 64)) | (~0ULL << (64 - (registers[solveRegister(v[3])] % 64)));
                            else
                                registers[solveRegister(v[1])] = (registers[solveRegister(v[2])] >> (registers[solveRegister(v[3])] % 64));
                        }
                        break;
                    case 0x6:
                        registers[solveRegister(v[1])] = registers[solveRegister(v[2])] | registers[solveRegister(v[3])];
                        break;
                    case 0x7:
                        registers[solveRegister(v[1])] = registers[solveRegister(v[2])] & registers[solveRegister(v[3])];
                        break;
                    }
                    break;
                // I type Instructions
                case 0b0010011:
                    switch (riscInfo[v[0]].func3)
                    {
                    case 0x0:
                        registers[solveRegister(v[1])] = registers[solveRegister(v[2])] + solveImmediateSigned(v[3], 12);
                        break;
                    case 0x1:
                        registers[solveRegister(v[1])] = registers[solveRegister(v[2])] << solveImmediateNonNegative(v[3], 6);
                        break;
                    case 0x2:
                        registers[solveRegister(v[1])] = registers[solveRegister(v[2])] < solveImmediateSigned(v[3], 12);
                        break;
                    case 0x3:
                        registers[solveRegister(v[1])] = (unsigned long long)registers[solveRegister(v[2])] < solveImmediateSigned(v[3], 12);
                        break;
                    case 0x4:
                        registers[solveRegister(v[1])] = registers[solveRegister(v[2])] ^ solveImmediateSigned(v[3], 12);
                        break;
                    case 0x5:
                        if (riscInfo[v[0]].func7 == 0x0)
                            registers[solveRegister(v[1])] = registers[solveRegister(v[2])] >> solveImmediateNonNegative(v[3], 6);
                        else
                        {
                            if (registers[solveRegister(v[2])] >> 63 == 1)
                                registers[solveRegister(v[1])] = (registers[solveRegister(v[2])] >> solveImmediateNonNegative(v[3], 6)) | (-1LL << (64 - solveImmediateNonNegative(v[3], 6)));
                            else
                                registers[solveRegister(v[1])] = (registers[solveRegister(v[2])] >> solveImmediateNonNegative(v[3], 6));
                        }
                        break;
                    case 0x6:
                        registers[solveRegister(v[1])] = registers[solveRegister(v[2])] | solveImmediateSigned(v[3], 12);
                        break;
                    case 0x7:
                        registers[solveRegister(v[1])] = registers[solveRegister(v[2])] & solveImmediateSigned(v[3], 12);
                        break;
                    }
                    break;
                // Load Type Instructions
                case 0b0000011:
                    if (v.size() == 3)
                    {
                        int start = v[2].find('(');
                        int end = v[2].find(')');
                        if (riscInfo[v[0]].func3 == 0x0)
                            registers[solveRegister(v[1])] = loadData(registers[solveRegister(v[2].substr(start + 1, end - start - 1))] + solveImmediateSigned(v[2].substr(0, start), 12), 8, true);
                        else if (riscInfo[v[0]].func3 == 0x1)
                            registers[solveRegister(v[1])] = loadData(registers[solveRegister(v[2].substr(start + 1, end - start - 1))] + solveImmediateSigned(v[2].substr(0, start), 12), 16, true);
                        else if (riscInfo[v[0]].func3 == 0x2)
                            registers[solveRegister(v[1])] = loadData(registers[solveRegister(v[2].substr(start + 1, end - start - 1))] + solveImmediateSigned(v[2].substr(0, start), 12), 32, true);
                        else if (riscInfo[v[0]].func3 == 0x3)
                            registers[solveRegister(v[1])] = loadData(registers[solveRegister(v[2].substr(start + 1, end - start - 1))] + solveImmediateSigned(v[2].substr(0, start), 12), 64, true);
                        else if (riscInfo[v[0]].func3 == 0x4)
                            registers[solveRegister(v[1])] = loadData(registers[solveRegister(v[2].substr(start + 1, end - start - 1))] + solveImmediateSigned(v[2].substr(0, start), 12), 8, false);
                        else if (riscInfo[v[0]].func3 == 0x5)
                            registers[solveRegister(v[1])] = loadData(registers[solveRegister(v[2].substr(start + 1, end - start - 1))] + solveImmediateSigned(v[2].substr(0, start), 12), 16, false);
                        else if (riscInfo[v[0]].func3 == 0x6)
                            registers[solveRegister(v[1])] = loadData(registers[solveRegister(v[2].substr(start + 1, end - start - 1))] + solveImmediateSigned(v[2].substr(0, start), 12), 32, false);
                    }
                    else if (v.size() == 4)
                    {
                        if (riscInfo[v[0]].func3 == 0x0)
                            registers[solveRegister(v[1])] = loadData(registers[solveRegister(v[3])] + solveImmediateSigned(v[2], 12), 8, true);
                        else if (riscInfo[v[0]].func3 == 0x1)
                            registers[solveRegister(v[1])] = loadData(registers[solveRegister(v[3])] + solveImmediateSigned(v[2], 12), 16, true);
                        else if (riscInfo[v[0]].func3 == 0x2)
                            registers[solveRegister(v[1])] = loadData(registers[solveRegister(v[3])] + solveImmediateSigned(v[2], 12), 32, true);
                        else if (riscInfo[v[0]].func3 == 0x3)
                            registers[solveRegister(v[1])] = loadData(registers[solveRegister(v[3])] + solveImmediateSigned(v[2], 12), 64, true);
                        else if (riscInfo[v[0]].func3 == 0x4)
                            registers[solveRegister(v[1])] = loadData(registers[solveRegister(v[3])] + solveImmediateSigned(v[2], 12), 8, false);
                        else if (riscInfo[v[0]].func3 == 0x5)
                            registers[solveRegister(v[1])] = loadData(registers[solveRegister(v[3])] + solveImmediateSigned(v[2], 12), 16, false);
                        else if (riscInfo[v[0]].func3 == 0x6)
                            registers[solveRegister(v[1])] = loadData(registers[solveRegister(v[3])] + solveImmediateSigned(v[2], 12), 32, false);
                    }
                    break;
                // S type Instructions
                case 0b0100011:
                    if (v.size() == 3)
                    {
                        int start = v[2].find('(');
                        int end = v[2].find(')');
                        if (riscInfo[v[0]].func3 == 0x0)
                            storeData(registers[solveRegister(v[1])], registers[solveRegister(v[2].substr(start + 1, end - start - 1))] + solveImmediateSigned(v[2].substr(0, start), 12), 8);
                        else if (riscInfo[v[0]].func3 == 0x1)
                            storeData(registers[solveRegister(v[1])], registers[solveRegister(v[2].substr(start + 1, end - start - 1))] + solveImmediateSigned(v[2].substr(0, start), 12), 16);
                        else if (riscInfo[v[0]].func3 == 0x2)
                            storeData(registers[solveRegister(v[1])], registers[solveRegister(v[2].substr(start + 1, end - start - 1))] + solveImmediateSigned(v[2].substr(0, start), 12), 32);
                        else if (riscInfo[v[0]].func3 == 0x3)
                            storeData(registers[solveRegister(v[1])], registers[solveRegister(v[2].substr(start + 1, end - start - 1))] + solveImmediateSigned(v[2].substr(0, start), 12), 64);
                    }
                    else if (v.size() == 4)
                    {
                        if (riscInfo[v[0]].func3 == 0x0)
                            storeData(registers[solveRegister(v[1])], registers[solveRegister(v[3])] + solveImmediateSigned(v[2], 12), 8);
                        else if (riscInfo[v[0]].func3 == 0x1)
                            storeData(registers[solveRegister(v[1])], registers[solveRegister(v[3])] + solveImmediateSigned(v[2], 12), 16);
                        else if (riscInfo[v[0]].func3 == 0x2)
                            storeData(registers[solveRegister(v[1])], registers[solveRegister(v[3])] + solveImmediateSigned(v[2], 12), 32);
                        else if (riscInfo[v[0]].func3 == 0x3)
                            storeData(registers[solveRegister(v[1])], registers[solveRegister(v[3])] + solveImmediateSigned(v[2], 12), 64);
                    }
                    break;
                // B Type Instruction
                case 0b1100011:
                    if (riscInfo[v[0]].func3 == 0x0)
                    {
                        if (registers[solveRegister(v[1])] == registers[solveRegister(v[2])])
                            doJump = true;
                    }
                    else if (riscInfo[v[0]].func3 == 0x1)
                    {
                        if (registers[solveRegister(v[1])] != registers[solveRegister(v[2])])
                            doJump = true;
                    }
                    else if (riscInfo[v[0]].func3 == 0x4)
                    {
                        if (registers[solveRegister(v[1])] < registers[solveRegister(v[2])])
                            doJump = true;
                    }
                    else if (riscInfo[v[0]].func3 == 0x5)
                    {
                        if (registers[solveRegister(v[1])] >= registers[solveRegister(v[2])])
                            doJump = true;
                    }
                    else if (riscInfo[v[0]].func3 == 0x6)
                    {
                        if ((unsigned long long)registers[solveRegister(v[1])] < (unsigned long long)registers[solveRegister(v[2])])
                            doJump = true;
                    }
                    else if (riscInfo[v[0]].func3 == 0x7)
                    {
                        if ((unsigned long long)registers[solveRegister(v[1])] >= (unsigned long long)registers[solveRegister(v[2])])
                            doJump = true;
                    }
                    if (doJump)
                    {
                        index = Labels.find(v[3]);
                        if (index != Labels.end())
                        {
                            lineCounter = index->second.second;
                            PC = index->second.first;
                        }
                        else
                        {
                            PC += solveImmediateSigned(v[2], 21);
                            if (PC % 2)
                                PC--;
                            lineCounter = findLineNumberWrtPC(PC);
                        }
                    }
                    break;
                // JAL Instruction
                case 0b1101111:
                    registers[solveRegister(v[1])] = PC + 4;
                    index = Labels.find(v[2]);
                    if (index != Labels.end())
                    {
                        PC = index->second.first;
                        lineCounter = index->second.second;
                    }
                    else
                    {
                        PC += solveImmediateSigned(v[2], 21);
                        if (PC % 2)
                            PC--;
                        lineCounter = findLineNumberWrtPC(PC);
                    }
                    Stack.push_back({v[2], lineCounter - 1});
                    doJump = true;
                    break;
                // JALR Instruction
                case 0b1100111:
                    registers[solveRegister(v[1])] = PC + 4;
                    if (v.size() == 4)
                        PC = registers[solveRegister(v[2])] + solveImmediateSigned(v[3], 12);
                    else if (v.size() == 3)
                    {
                        int start = v[2].find('(');
                        int end = v[2].find(')');
                        PC = registers[solveRegister(v[2].substr(start + 1, end - start - 1))] + solveImmediateSigned(v[2].substr(0, start), 12);
                    }
                    lineCounter = findLineNumberWrtPC(PC);
                    Stack.pop_back();
                    doJump = true;
                    break;
                // AUIPC Instruction
                case 0b0010111:
                    registers[solveRegister(v[1])] = PC + (solveImmediateNonNegative(v[2], 20) << 12);
                    break;
                // LUI Instruction
                case 0b0110111:
                    long long value = solveImmediateNonNegative(v[2], 20);
                    if (value >> 19 == 1)
                        value = value | (~0ULL << 20);
                    registers[solveRegister(v[1])] = value << 12;
                    break;
                }

                if (!doJump)
                {
                    PC += 4;
                    lineCounter++;
                }
                registers[0] = 0;
            } while (!step && lineCounter < lines.size());
    }

    void load(std::string fileName)
    {
        storeInstructions(fileName);
        PC = 0;
        lineCounter = 1;
        if (error)
            loaded = false;
        else
            loaded = true;
    }

    void printRegisters()
    {
        std::cout << "Registers:" << std::endl;
        for (int i = 0; i < 32; i++)
            std::cout << "x" << std::dec << i << (i < 10 ? " " : "") << " = 0x" << std::hex << registers[i] << std::endl;
    }

    void printMemory(std::string index, std::string count)
    {
        int i = solveImmediateNonNegative(index, 20);
        int c = solveImmediateNonNegative(count, 20);
        while (c--)
            std::cout << "Memory[" << "0x" << std::hex << i << "] = " << "0x" << (long long)(memory[i++]) << std::endl;
    }

    void showStack()
    {
        if (lineCounter >= lines.size())
            std::cout << "Empty Call Stack: Execution complete" << std::endl;
        else
        {
            std::cout << "Call Stack:" << std::endl;
            for (auto p : Stack)
                std::cout << std::dec << p.first << ':' << p.second << std::endl;
        }
    }

    void addBreakPoint(int lineNumber)
    {
        breakPoints.insert(lineNumber);
        std::cout << "Breakpoint set at line " << lineNumber << std::endl;
    }

    void deleteBreakpoint(int lineNumber)
    {
        if (breakPoints.find(lineNumber) == breakPoints.end())
            std::cout << "Break Point Doesn't exist at Line number: " << lineNumber << std::endl;
        else
            breakPoints.erase(lineNumber);
    }
};

int main()
{
    while (true)
    {
        simulator test;
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
                test.load(fileName);
        }

        if (command == "run")
        {
            getline(ss, errorChecker);
            if (!errorChecker.empty())
                std::cout << "Invalid Command, Expected: run" << std::endl;
            else
                test.run(false);
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
        else if (command == "step")
        {
            getline(ss, errorChecker);
            if (!errorChecker.empty())
                std::cout << "Invalid Command, Expected: step" << std::endl;
            else
                test.run(true);
        }
        else if (command == "show-stack")
        {
            getline(ss, errorChecker);
            if (!errorChecker.empty())
                std::cout << "Invalid Command, Expected: show-stack" << std::endl;
            else
                test.showStack();
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
        else
            std::cout << "Command Not found" << std::endl;
        std::cout << std::endl;
    }
}