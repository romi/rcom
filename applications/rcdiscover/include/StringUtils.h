// http://www.martinbroadhurst.com/how-to-trim-a-stdstring.html


#include <string>

class StringUtils
{
public:
    static const std::string whitespace_chars;
    static std::string& ltrim(std::string& str, const std::string& chars);
    static std::string& rtrim(std::string& str, const std::string& chars);
    static std::string& trim(std::string& str, const std::string& chars);
};


