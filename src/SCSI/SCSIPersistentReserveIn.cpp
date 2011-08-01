/*
 * Copyright (C) 2011 by Scale Computing, Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author(s): Richard Sharpe <realrichardsharpe@gmail.com>
 *            Asad Saeed, Scale Computing
 */

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

std::string SCSIPersistentReserveInReadKeys::GetKey(unsigned int iterator) {
    return GetInBufferString((iterator + 1) * 8, 8);
}

SCSIPersistentReserveInReadReservation::SCSIPersistentReserveInReadReservation(
        unsigned int allocationLength) :
        SCSIPersistentReserveIn(READ_RESERVATION, allocationLength) {}

std::string SCSIPersistentReserveInReadReservation::GetReservation(void)
{
    if (GetInBufferLong(4) == 0){
        return std::string();
    }
    return GetInBufferString(8, 8);
}

scsi_persistent_reservation_type
SCSIPersistentReserveInReadReservation::GetReservationType(void)
{
    scsi_persistent_reservation_type type;
    int bitArray;
    if (GetInBufferLong(4) == 0)
        throw CException("Insufficient Data");

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
        throw CException("Invalid Value");

    return type;
}
