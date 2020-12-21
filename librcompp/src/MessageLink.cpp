#include <iostream>
#include "MessageLink.h"

MessageLink::MessageLink()
{}

int MessageLink::send_string(const std::string &string)
{
    std::cout << string;
    return 0;
}