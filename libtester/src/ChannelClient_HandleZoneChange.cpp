#include "ChannelClient.h"

using namespace libtester;

void ChannelClient::HandleZoneChange(libcomp::ReadOnlyPacket& p)
{
    (void)p.ReadS32Little(); // old zone
    mZoneID = p.ReadS32Little();
}
