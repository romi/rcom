#ifndef ROMI_RCOMPP_REGISTRYENTRY_H
#define ROMI_RCOMPP_REGISTRYENTRY_H

#include <string>

#include "IRegistryEntry.h"

class RegistryEntry : public IRegistryEntry
{
public:
    RegistryEntry(const std::string& id);
    ~RegistryEntry() override = default;

private:
    // Might not need this with a copy constructor.
    const std::string _id;

};


#endif //ROMI_ROVER_BUILD_AND_TEST_REGISTRYENTRY_H
