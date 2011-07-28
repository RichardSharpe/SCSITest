/**
 * A SCSI Read class. All READ requests should go in here.
 *
 * Author: Richard Sharpe
 */

#include "SCSIRequest.h"
#include "SCSIReserve.h"

SCSIReserve6::SCSIReserve6() :
    SCSIRequest(6) 
{
    setCdbByte(0, SCSI_OPCODE_RESERVE6); // That's a RESERVE 10 request
}

SCSIReserve6::~SCSIReserve6()
{
}
