#include <UuId.h>
#include "RegistryEntry.h"


RegistryEntry::RegistryEntry(const std::string& name, const std::string& topic, const RegistryType& type, const addr_t& address, std::shared_ptr<ILink> link)
: _id(UuId::create()), _name(name), _topic(topic), _type(type), _address(address), _link(std::move(link))
{
}

RegistryEntry::RegistryEntry(const RegistryEntry& copy)
: _id(copy._id), _name(copy._name), _topic(copy._topic), _type(copy._type), _address(copy._address), _link(copy._link)
{
}

const addr_t& RegistryEntry::address()
{
    return _address;
}