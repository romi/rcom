#include "gmock/gmock.h"
#include "IWebSocketServerListener.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"

class MockWebSocketServerListener : public rcom::IWebSocketServerListener
{
public:
        MOCK_METHOD(void, onconnect, (rcom::IWebSocket& link), (override));                
        MOCK_METHOD(void, onmessage, (rcom::IWebSocket& link, rpp::MemBuffer& message), (override));
        MOCK_METHOD(void, onclose, (rcom::IWebSocket& link), (override));                
};
