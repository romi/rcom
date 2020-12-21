#ifndef RPP_ILINK_H
#define RPP_ILINK_H

#include <string>

class ILink
{
public:
    virtual ~ILink() = default;
    virtual int send_string(const std::string& string) = 0;
};

#endif
