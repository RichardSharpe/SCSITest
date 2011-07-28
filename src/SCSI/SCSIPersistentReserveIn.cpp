#include "SCSIPersistentReserveIn.h"

SCSIPersistentReserveIn::SCSIPersistentReserveIn(ServiceAction action,
        unsigned int allocationLength) :
        SCSIRequest(10),
        mServiceAction(action)
{
    setCdbByte(0x00, SCSI_OPCODE_PERSISTENTRESERVEIN);
    setCdbByte(0x01, (uint8_t) action);
    setCdbShort(0x07, allocationLength);
    createInBuffer(allocationLength);
    SetXferDir(SCSI_XFER_READ);
}

uint32_t SCSIPersistentReserveIn::GetPRGeneration() {
    return GetInBufferLong(0);
}

SCSIPersistentReserveInReadKeys::SCSIPersistentReserveInReadKeys(
        unsigned int allocationLength) :
        SCSIPersistentReserveIn(READ_KEYS, allocationLength) {}


uint32_t SCSIPersistentReserveInReadKeys::GetKeyCount(void) {
    return (GetInBufferLong(4) +1) / 8;
}

Forte::FString SCSIPersistentReserveInReadKeys::GetKey(unsigned int iterator) {
    return GetInBufferFString((iterator + 1) * 8, 8);
}

SCSIPersistentReserveInReadReservation::SCSIPersistentReserveInReadReservation(
        unsigned int allocationLength) :
        SCSIPersistentReserveIn(READ_RESERVATION, allocationLength) {}

Forte::FString SCSIPersistentReserveInReadReservation::GetReservation(void)
{
    if (GetInBufferLong(4) == 0){
        return Forte::FString();
    }
    return GetInBufferFString(8, 8);
}

scsi_persistent_reservation_type
SCSIPersistentReserveInReadReservation::GetReservationType(void)
{
    scsi_persistent_reservation_type type;
    int bitArray;
    if (GetInBufferLong(4) == 0)
        throw ESCSIInsufficientData();

    bitArray = GetInBufferBitArray(21, 0, 4);

    if (bitArray == RESERVATION_TYPE_WRITE_EXCLUSIVE)
        type = RESERVATION_TYPE_WRITE_EXCLUSIVE;
    else if(bitArray == RESERVATION_TYPE_EXCLUSIVE_ACCESS)
        type = RESERVATION_TYPE_EXCLUSIVE_ACCESS;
    else if(bitArray == RESERVATION_TYPE_WRITE_EXCLUSIVE_REGISTRANTS_ONLY)
        type = RESERVATION_TYPE_WRITE_EXCLUSIVE;
    else if(bitArray == RESERVATION_TYPE_EXCLUSIVE_ACCESS_REGISTRANTS_ONLY)
        type = RESERVATION_TYPE_EXCLUSIVE_ACCESS_REGISTRANTS_ONLY;
    else if(bitArray == RESERVATION_TYPE_WRITE_EXCLUSIVE_ALL_REGISTRANTS)
        type = RESERVATION_TYPE_WRITE_EXCLUSIVE_ALL_REGISTRANTS;
    else if(bitArray == RESERVATION_TYPE_EXCLUSIVE_ACCESS_ALL_REGISTRANTS)
        type = RESERVATION_TYPE_EXCLUSIVE_ACCESS_ALL_REGISTRANTS;
    else
        throw ESCSIExceptionInvalidValue();

    return type;
}
