#ifndef RPP_MESSAGELINK_H
#define RPP_MESSAGELINK_H

#include "ILink.h"

class MessageLink : public ILink
{
public:
    MessageLink();
    ~MessageLink() override = default;
    int send_string(const std::string &string) override;
};

#endif
