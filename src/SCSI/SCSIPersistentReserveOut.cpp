#include "SCSIPersistentReserveOut.h"

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

void SCSIPersistentReserveOut::SetReservationKey(const Forte::FString &key) {
    if (key.length() != 8)
        throw ESCSIExceptionInvalidValue();

    SetOutBufferFString(0, key);
}

void SCSIPersistentReserveOut::SetServiceActionReservationKey(
        const Forte::FString &key)
{
    if (key.length() != 8)
        throw ESCSIExceptionInvalidValue();

    SetOutBufferFString(8, key);
}

void SCSIPersistentReserveOut::SetTransportIdList(uint8_t *buffer, unsigned int length) {
    if (buffer == NULL || length == 0)
        throw ESCSIExceptionInvalidValue();

    // copy shared_array, end of function call will call oldbuffer to be destroyed
    shared_array<uint8_t> oldBuffer = mOutBuffer;

    createOutBuffer(length + 24);
    setCdbShort(0x07, length + 24);

    memcpy(mOutBuffer.get(), oldBuffer.get(), 24);
    memcpy(&mOutBuffer[24], buffer, length);
    SetOutBufferBool(20, 4, true); // set SPEC_I_PT
}


SCSIPersistentReserveOutRegisterAndMove::SCSIPersistentReserveOutRegisterAndMove(
        const Forte::FString &transportId) :
        SCSIPersistentReserveOut(PR_REGISTER_AND_MOVE, 24 + transportId.length())
{
    unsigned int transportIdLength = transportId.length();

    if (transportIdLength < 24 || transportIdLength % 4 != 0)
        throw ESCSIExceptionInvalidValue();

    SetOutBufferLong(20, transportIdLength);
    SetOutBufferFString(24, transportId);
}

