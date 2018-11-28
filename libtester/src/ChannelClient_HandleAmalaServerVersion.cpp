#include "ChannelClient.h"

// libcomp Includes
#include "Log.h"

using namespace libtester;

void ChannelClient::HandleAmalaServerVersion(libcomp::ReadOnlyPacket& p)
{
    uint8_t major = p.ReadU8();
    uint8_t minor = p.ReadU8();
    uint8_t patch = p.ReadU8();

    libcomp::String codename = p.ReadString16Little(
        libcomp::Convert::Encoding_t::ENCODING_UTF8);

    LOG_INFO(libcomp::String("Connected to COMP_hack v%1.%2.%3 (%4)\n").Arg(
        major).Arg(minor).Arg(patch).Arg(codename));

    libcomp::String commit = p.ReadString16Little(
        libcomp::Convert::Encoding_t::ENCODING_UTF8, true);
    libcomp::String repo = p.ReadString16Little(
        libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

    if(!commit.IsEmpty() && !repo.IsEmpty())
    {
        LOG_INFO(libcomp::String("Server is built from Git source:\n"));
        LOG_INFO(libcomp::String("  Commit: %1\n").Arg(commit));
        LOG_INFO(libcomp::String("  Repo URL: %1\n").Arg(repo));
    }

    int32_t userLevel = p.ReadS32Little();

    LOG_INFO(libcomp::String("Your user level is %1.\n").Arg(userLevel));
}
