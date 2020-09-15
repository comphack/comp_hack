#include "ChannelClient.h"

// libcomp Includes
#include "Log.h"

using namespace libtester;

void ChannelClient::HandleAmalaServerVersion(libcomp::ReadOnlyPacket& p) {
  uint8_t major = p.ReadU8();
  uint8_t minor = p.ReadU8();
  uint8_t patch = p.ReadU8();

  libcomp::String codename =
      p.ReadString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8);

  LogGeneralInfo([&]() {
    return libcomp::String("Connected to COMP_hack v%1.%2.%3 (%4)\n")
        .Arg(major)
        .Arg(minor)
        .Arg(patch)
        .Arg(codename);
  });

  libcomp::String commit =
      p.ReadString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8, true);
  libcomp::String repo =
      p.ReadString16Little(libcomp::Convert::Encoding_t::ENCODING_UTF8, true);

  if (!commit.IsEmpty() && !repo.IsEmpty()) {
    LogGeneralInfo([&]() {
      return libcomp::String("Server is built from Git source:\n");
    });

    LogGeneralInfo(
        [&]() { return libcomp::String("  Commit: %1\n").Arg(commit); });

    LogGeneralInfo(
        [&]() { return libcomp::String("  Repo URL: %1\n").Arg(repo); });
  }

  int32_t userLevel = p.ReadS32Little();

  LogGeneralInfo([&]() {
    return libcomp::String("Your user level is %1.\n").Arg(userLevel);
  });
}
