#ifndef __SCSITESTUNITREADY_h__
#define __SCSITESTUNITREADY_h__

#include "iSCSILibWrapper.h"
#include "SCSIRequest.h"

class SCSITestUnitReady : public SCSIRequest
{
public:
    SCSITestUnitReady();

    ~SCSITestUnitReady() {}

protected:
};

#endif
