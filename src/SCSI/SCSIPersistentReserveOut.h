#ifndef __SCSIPersistentReserveOut_h__
#define __SCSIPersistentReserveOut_h__

#include "SCSIRequest.h"

class SCSIPersistentReserveOut : public SCSIRequest
{
public:
    enum ServiceAction {
        PR_REGISTER                         = 0x00,
        PR_RESERVE                          = 0x01,
        PR_RELEASE                          = 0x02,
        PR_CLEAR                            = 0x03,
        PR_PREEMPT                          = 0x04,
        PR_REEMPT_AND_ABORT                 = 0x05,
        PR_REGISTER_AND_IGNORE_EXISTING_KEY = 0x06,
        PR_REGISTER_AND_MOVE                = 0x07,
        PR_READ_FULL_STATUS                 = 0x08,
    };

public:
    SCSIPersistentReserveOut(ServiceAction action,
                             unsigned int allocationLength = 24);
    virtual ~SCSIPersistentReserveOut() {}
    void SetReservationKey(const std::string &key);
    void SetReservationType(scsi_persistent_reservation_type type);
    void SetServiceActionReservationKey(const std::string &key);
    virtual void SetAPTPL(bool aptpl) { SetOutBufferBool(20, 0, aptpl); }
    virtual void SetAllTgPt(bool allTgPt) { SetOutBufferBool(20, 2, allTgPt); }
    virtual void SetTransportIdList(uint8_t *buffer, unsigned int length);
private:
    ServiceAction mServiceAction;
};

class SCSIPersistentReserveOutRegisterAndMove : public SCSIPersistentReserveOut
{
public:
    SCSIPersistentReserveOutRegisterAndMove(const std::string &transportId);
    virtual ~SCSIPersistentReserveOutRegisterAndMove() {}

    void SetRelativeTargetPortId(uint16_t id) { SetOutBufferShort(18, id); }
    virtual void SetAPTPL(bool aptpl) { SetOutBufferBool(17, 0, aptpl); }
    void SetUNREG(bool unreg) { SetOutBufferBool(17, 1, unreg); }

    virtual void SetAllTgPt(bool allTgPt) 
        { throw CException("Method Not Supported"); }
    virtual void SetTransportIdList(uint8_t *buffer, unsigned int length)
        { throw CException("Method Not Supported"); }

};

#endif
