#include "ChannelClient.h"

using namespace libtester;

void ChannelClient::HandleDemonBoxData(libcomp::ReadOnlyPacket& p)
{
    /// @todo Handle the box id
    (void)p.ReadS8(); // box id
    int8_t slot = p.ReadS8();
    mDemonIDs[slot] = p.ReadS64Little();
}
