#include "ChannelClient.h"

// libcomp Includes
#include "Log.h"

using namespace libtester;

void ChannelClient::HandleAmalaAccountDumpHeader(libcomp::ReadOnlyPacket& p)
{
    uint32_t dumpSize = p.ReadU32Little();
    mAccountDumpData.resize(dumpSize);

    mAccountDumpParts = p.ReadU32Little();
    mAccountDumpChecksum = p.ReadString16Little(
        libcomp::Convert::Encoding_t::ENCODING_UTF8, true);
    mAccountDumpAccountName = p.ReadString16Little(
        libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

}
