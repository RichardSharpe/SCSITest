#ifndef __SCSIPersistentReserveIn_h__
#define __SCSIPersistentReserveIn_h__

#include "SCSIRequest.h"

using namespace Forte;

class SCSIPersistentReserveIn : public SCSIRequest
{
public:
    enum ServiceAction {
        READ_KEYS               = 0x00,
        READ_RESERVATION        = 0x01,
        REPORT_CAPABILITIES     = 0x02,
        READ_FULL_STATUS        = 0x03,
    };

public:
    SCSIPersistentReserveIn(ServiceAction action,
                            unsigned int allocationLength = 255);
    virtual ~SCSIPersistentReserveIn() {}
    uint32_t GetPRGeneration();
private:
    ServiceAction mServiceAction;
};

class SCSIPersistentReserveInReadKeys : public SCSIPersistentReserveIn
{
public:
    SCSIPersistentReserveInReadKeys(unsigned int allocationLength = 255);
    virtual ~SCSIPersistentReserveInReadKeys() {}
    uint32_t GetKeyCount(void);
    Forte::FString GetKey(unsigned int iterator);
};

class SCSIPersistentReserveInReadReservation : public SCSIPersistentReserveIn
{
public:
    SCSIPersistentReserveInReadReservation(unsigned int allocationLength = 255);
    virtual ~SCSIPersistentReserveInReadReservation() {}
    Forte::FString GetReservation(void);
    scsi_persistent_reservation_type GetReservationType(void);
};



#endif
