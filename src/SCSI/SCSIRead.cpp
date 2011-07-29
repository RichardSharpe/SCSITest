/**
 * A SCSI Read class. All READ requests should go in here.
 *
 * Author: Richard Sharpe
 */

#include "SCSIRequest.h"
#include "SCSIRead.h"
#include <boost/shared_array.hpp>

SCSIRead10::SCSIRead10(unsigned int transferLength, 
                       boost::shared_array<uint8_t> buffer) :
    SCSIRequest(10), 
    mLBA(0)
{
    setCdbByte(0, SCSI_OPCODE_READ10); // That's a READ 10 request
    setCdbLong(2, 0);    // Default to LBA 0
    setCdbShort(7, transferLength);

    if (!buffer)
        createInBuffer(transferLength);
    else
        setInBuffer(buffer, transferLength);

    SetXferDir(SCSI_XFER_READ);
}

SCSIRead10::~SCSIRead10()
{
}

void SCSIRead10::SetLBA(uint32_t lba)
{
    mLBA = lba;
    setCdbLong(2, mLBA);
}
