#ifndef __SCSIRead_h__
#define __SCSIRead_h__

#include "iSCSILibWrapper.h"
#include "SCSIRequest.h"

class SCSIRead10 : public SCSIRequest
{
public:
    SCSIRead10(unsigned int transferLength,
               boost::shared_array<uint8_t> buffer = boost::shared_array<uint8_t>());
    ~SCSIRead10();

    void SetLBA(uint32_t lba);

private:
    SCSIRead10();
    unsigned int mLBA;
};

#endif
