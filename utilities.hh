#ifndef UTILITIES_GUARD
#define UTILITIES_GUARD

#include <string>
#include <vector>
#include <sstream>

namespace utilities
{
    void spiltLineIntoWords(std::string s, std::vector<std::string> &v);
    bool checkIfValueIsInBetween(char c, char min, char max);
    bool checkBase10(std::string s);
    bool checkBase16(std::string s);
    void format(std::string &s);
}

#endif