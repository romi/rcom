// http://www.martinbroadhurst.com/how-to-trim-a-stdstring.html

#include "StringUtils.h"
#include <string>

const std::string StringUtils::whitespace_chars = "\t\n\v\f\r ";

std::string& StringUtils::ltrim(std::string& str, const std::string& chars)
{
    str.erase(0, str.find_first_not_of(chars));
    return str;
}

std::string& StringUtils::rtrim(std::string& str, const std::string& chars)
{
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}

std::string& StringUtils::trim(std::string& str, const std::string& chars)
{
    return ltrim(rtrim(str, chars), chars);
}

