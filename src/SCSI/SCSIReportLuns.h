#ifndef __SCSIReportLuns_h__
#define __SCSIReportLuns_h__

#include "iSCSILibWrapper.h"
#include "SCSIRequest.h"

class SCSIReportLuns : public SCSIRequest
{
public:
    // Space for header and five LUNs
    SCSIReportLuns(unsigned int allocationLength = 48);

    ~SCSIReportLuns() {}

    unsigned int GetLunCount() { return GetInBufferLong(0) / 8; }

    // First 2 bytes of every 8th byte contains lun number.
    unsigned int GetLun(unsigned int lunNo)
    { 
        return GetInBufferShort((8 * (lunNo + 1)));
    }
};

#endif
