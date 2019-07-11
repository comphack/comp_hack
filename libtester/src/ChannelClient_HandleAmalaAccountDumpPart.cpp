#include "ChannelClient.h"

// libcomp Includes
#include "Crypto.h"
#include "Log.h"

using namespace libtester;

#define PART_SIZE (1024)

void ChannelClient::HandleAmalaAccountDumpPart(libcomp::ReadOnlyPacket& p)
{
    uint32_t partNumber = p.ReadU32Little();
    uint32_t partSize = p.ReadU32Little();

    mLastAccountDumpPart = partNumber;

    char *szStart = &mAccountDumpData[(partNumber - 1) * PART_SIZE];

    p.ReadArray(szStart, partSize);

    if(partNumber == mAccountDumpParts)
    {
        libcomp::String checksum = libcomp::Crypto::SHA1(mAccountDumpData);

        if(checksum == mAccountDumpChecksum)
        {
            FILE *out = fopen(libcomp::String("%1.xml").Arg(
                mAccountDumpAccountName).C(), "wb");

            if(!out || 1 != fwrite(&mAccountDumpData[0],
                mAccountDumpData.size(), 1, out))
            {
                LOG_ERROR("Failed to write account dump to disk.\n");
            }
            else
            {
                LOG_INFO(libcomp::String("Wrote backup of account '%1' to "
                    "'%2.xml'\n").Arg(mAccountDumpAccountName).Arg(
                    mAccountDumpAccountName));
            }

            if(out)
            {
                fclose(out);
            }
        }
        else
        {
            LOG_ERROR("Failed to save account dump due to corruption!\n");
        }
    }
}
