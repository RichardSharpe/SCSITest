/**
 * A SCSI Test Unit Reade Request class ...
 *
 * Author: Richard Sharpe
 */

#include "SCSIRequest.h"
#include "SCSITestUnitReady.h"

SCSITestUnitReady::SCSITestUnitReady() : SCSIRequest()
{
    setCdbByte(0, 0x00);  // That's a TUR
    SetXferDir(SCSI_XFER_NONE);
}
