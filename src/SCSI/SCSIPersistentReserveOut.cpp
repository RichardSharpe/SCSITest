#include "SCSIPersistentReserveOut.h"
#include <boost/shared_array.hpp>

SCSIPersistentReserveOut::SCSIPersistentReserveOut(ServiceAction action,
                                                   unsigned int allocationLength) :
        SCSIRequest(10),
        mServiceAction(action)
{
    setCdbByte(0x00, SCSI_OPCODE_PERSISTENTRESERVEOUT);
    setCdbByte(0x01, (uint8_t) mServiceAction);
    setCdbShort(0x07, allocationLength);
    createOutBuffer(allocationLength);
    SetXferDir(SCSI_XFER_WRITE);
}

void SCSIPersistentReserveOut::SetReservationType(scsi_persistent_reservation_type type)
{
    setCdbBitArray(2, 0, 4, type);
}

void SCSIPersistentReserveOut::SetReservationKey(const std::string &key) {
    if (key.length() != 8)
        throw CException("Invalid Value");

    SetOutBufferString(0, key);
}

void SCSIPersistentReserveOut::SetServiceActionReservationKey(
        const std::string &key)
{
    if (key.length() != 8)
        throw CException("Invalid Value");

    SetOutBufferString(8, key);
}

void SCSIPersistentReserveOut::SetTransportIdList(uint8_t *buffer, unsigned int length) {
    if (buffer == NULL || length == 0)
        throw CException("Invalid Value");

    // copy shared_array, end of function call will call oldbuffer to be destroyed
    boost::shared_array<uint8_t> oldBuffer = mOutBuffer;

    createOutBuffer(length + 24);
    setCdbShort(0x07, length + 24);

    memcpy(mOutBuffer.get(), oldBuffer.get(), 24);
    memcpy(&mOutBuffer[24], buffer, length);
    SetOutBufferBool(20, 4, true); // set SPEC_I_PT
}


SCSIPersistentReserveOutRegisterAndMove::SCSIPersistentReserveOutRegisterAndMove(
        const std::string &transportId) :
        SCSIPersistentReserveOut(PR_REGISTER_AND_MOVE, 24 + transportId.length())
{
    unsigned int transportIdLength = transportId.length();

    if (transportIdLength < 24 || transportIdLength % 4 != 0)
        throw CException("Invalid Value");

    SetOutBufferLong(20, transportIdLength);
    SetOutBufferString(24, transportId);
}

