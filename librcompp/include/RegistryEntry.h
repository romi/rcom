#ifndef ROMI_RCOMPP_REGISTRYENTRY_H
#define ROMI_RCOMPP_REGISTRYENTRY_H

#include <memory>
#include <string>

#include "addr.h"
#include "IRegistryEntry.h"
#include "ILink.h"

class RegistryEntry : public IRegistryEntry
{
public:
    RegistryEntry(const std::string& name, const std::string& topic, const RegistryType& type, const addr_t& address, std::shared_ptr<ILink>  link);
    RegistryEntry(const RegistryEntry& copy);
    ~RegistryEntry() override = default;
    const addr_t& address();

private:
    const std::string _id;
    const std::string _name;
    const std::string _topic;
    const RegistryType _type;
    const addr_t _address;
    const std::shared_ptr<ILink> _link;
};


#endif //ROMI_RCOMPP_REGISTRYENTRY_H
