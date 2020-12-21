#ifndef ROMI_RCOMPP_IREGISTRYENTRY_H
#define ROMI_RCOMPP_IREGISTRYENTRY_H


enum {
    TYPE_MESSAGELINK = 0,
    TYPE_MESSAGEHUB,
    TYPE_SERVICE
//    ,
//    TYPE_STREAMER,
//    TYPE_STREAMERLINK,
//    TYPE_DATALINK,
//    TYPE_DATAHUB
};

class IRegistryEntry
{
public:
    virtual ~IRegistryEntry() = default;

};


#endif //ROMI_ROVER_BUILD_AND_TEST_REGISTRYENTRY_H
