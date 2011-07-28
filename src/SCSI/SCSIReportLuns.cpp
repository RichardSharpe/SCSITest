/**
 * A SCSI Request class ... wraps some of Ronnie Sahlberg's stuff ...
 *
 * Author: Richard Sharpe
 */

#include "SCSIRequest.h"
#include "SCSIReportLuns.h"

SCSIReportLuns::SCSIReportLuns(unsigned int allocationLength) :
    SCSIRequest(12)
{
    setCdbByte(0, SCSI_OPCODE_REPORTLUNS); // That's a REPORT LUNS request
    setCdbByte(2, 0x02); // All of them
    setCdbShort(6, allocationLength);
    createInBuffer(allocationLength);
    SetXferDir(SCSI_XFER_READ);
}
