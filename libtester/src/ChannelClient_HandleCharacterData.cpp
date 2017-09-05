#include "ChannelClient.h"

using namespace libtester;

void ChannelClient::HandleCharacterData(libcomp::ReadOnlyPacket& p)
{
    mEntityID = p.ReadS32Little();
}
