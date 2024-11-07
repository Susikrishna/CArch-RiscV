#include "utilities.hh"

void utilities::spiltLineIntoWords(std::string s, std::vector<std::string> &v)
{
    std::stringstream ss(s);
    std::string word;
    while (ss >> word)
        v.push_back(word);
}

bool utilities::checkIfValueIsInBetween(char c, char min, char max)
{
    if (c - min >= 0 && max - c >= 0)
        return true;
    return false;
}

bool utilities::checkBase10(std::string s)
{
    for (char c : s)
        if (!utilities::checkIfValueIsInBetween(c, '0', '9'))
            return false;
    return true;
}

bool utilities::checkBase16(std::string s)
{
    for (char c : s)
        if (!utilities::checkIfValueIsInBetween(c, '0', '9') && !utilities::checkIfValueIsInBetween(c, 'a', 'f'))
            return false;
    return true;
}

void utilities::format(std::string &s)
{
    long long index = 0;
    while (s[index] == ' ')
        index++;
    s = s.substr(index);

    index = s.size() - 1;
    while (s[index] == ' ')
        index--;
    s = s.substr(0, index + 1);

    index = s.find(";");
    if (index != std::string::npos)
        s = s.substr(0, index);

    for (int i = 0; i < s.size(); i++)
        if (s[i] == ',')
            s[i] = ' ';
}