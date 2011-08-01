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

/**
 * A SCSI Request class ... wraps some of Ronnie Sahlberg's stuff ...
 *
 * Authors: Richard Sharpe and Asad Saeed
 */

#include <arpa/inet.h>
#include <cstdarg>

#include "SCSIRequest.h"
#include "SCSIInquiry.h"
#include "SCSIReportLuns.h"
#include "SCSIRead.h"
#include "EString.h"
#include <exception>
#include "CException.h"

#define CHECK_BUFFER_OVERREAD(_bufferSize, _byteOffset, _byteLength)           \
    if ((_byteOffset + _byteLength ) > _bufferSize) {                          \
        EString estr;                                                          \
        estr.Format("%s: Invalid byte Offset: buffer over read, "              \
                "Buffer size: %u, accessing byte: %lu",                        \
                __func__, _bufferSize,                                         \
                (long unsigned int) (_byteOffset + _byteLength));              \
        throw CException(estr);                                                \
    }

#define CHECK_BUFFER_OVERFLOW(_bufferSize, _byteOffset, _byteLength)           \
    if ((_byteOffset + _byteLength) > _bufferSize) {                           \
        EString estr;                                                          \
        estr.Format("%s: Invalid byte Offset: buffer over flow, "              \
                "Buffer size: %u, accessing byte: %lu",                        \
                __func__, _bufferSize,                                         \
                (long unsigned int) (_byteOffset + _byteLength));              \
        throw CException(estr);                                                \
    }

#define CHECK_BYTE_BOUNDARY(_bitOffset, _bitLength)                            \
    if ((_bitOffset + _bitLength) > 8 ) {                                      \
        EString estr;                                                          \
        estr.Format("%s: Invalid bit Offset/Length, byte boundary crossed! "   \
                "Bit Offset: %u, Bit Length: %u",                              \
                __func__, _bitOffset, _bitLength);                             \
        throw CException(estr);                                                \
    }

SCSIRequest::SCSIRequest() :
    mExecuted(false),
    mLinkBit(false),
    mOutBuffer(NULL),
    mOutBufferSize(0),
    mInBuffer(NULL),
    mInBufferSize(0)
{
    mTask = (struct scsi_task *)malloc(sizeof(scsi_task));
    if (mTask == NULL)
        throw std::bad_alloc();  // Convert to standard exception
    memset(mTask, 0, sizeof(scsi_task));
    mTask->cdb_size = 6;
    mTask->xfer_dir = SCSI_XFER_NONE;
}

SCSIRequest::SCSIRequest(unsigned int cdbSize) :
    mExecuted(false),
    mOutBuffer(NULL),
    mOutBufferSize(0),
    mInBuffer(NULL),
    mInBufferSize(0)
{
    if (cdbSize > sizeof(mTask->cdb)) {
        EString estr;
        estr.Format("%s: Invalid CDB Size: cdbSize: %u, task CDB size: %u",
		__func__, cdbSize, mTask->cdb);
        throw CException(estr);
    }

    mTask = (struct scsi_task *)malloc(sizeof(scsi_task));
    if (mTask == NULL)
        throw std::bad_alloc();  // Convert to standard exception
    memset(mTask, 0, sizeof(scsi_task));
    mTask->cdb_size = cdbSize;
    mTask->xfer_dir = SCSI_XFER_NONE;
}

SCSIRequest::SCSIRequest(unsigned int cdbSize,
                         boost::shared_array<uint8_t> outBuffer,
                         unsigned int outBufferSize,
                         boost::shared_array<uint8_t> inBuffer,
                         unsigned int inBufferSize) :
    mExecuted(false),
    mOutBuffer(outBuffer),
    mOutBufferSize(outBufferSize),
    mInBuffer(inBuffer),
    mInBufferSize(inBufferSize)
{
    if (cdbSize > sizeof(mTask->cdb)) {
        throw CException("Invalid CDB Size");
    }

    mTask = (struct scsi_task *)malloc(sizeof(scsi_task));
    if (mTask == NULL)
        throw std::bad_alloc();  // Convert to standard exception
    memset(mTask, 0, sizeof(scsi_task));
    mTask->cdb_size = cdbSize;
    mTask->xfer_dir = SCSI_XFER_NONE;
}

void SCSIRequest::setCdbBitArray(unsigned int byteOffset,
                                 unsigned int startBit, // starts at 0
                                 unsigned int bitLength,
                                 uint8_t val)
{
    setBufferBitArray(mTask->cdb, mTask->cdb_size, byteOffset,
                      startBit, bitLength, val);
}

void SCSIRequest::setCdbByte(unsigned int byteOffset, uint8_t val)
{

    setBufferByte(mTask->cdb, mTask->cdb_size, byteOffset, val);
}

void SCSIRequest::setCdbShort(unsigned int byteOffset, uint16_t val)
{
    setBufferShort(mTask->cdb, mTask->cdb_size, byteOffset, val);
}

void SCSIRequest::setCdbLong(unsigned int byteOffset, uint32_t val)
{
    setBufferLong(mTask->cdb, mTask->cdb_size, byteOffset, val);
}


/*
 * Convert Status codes into a string
 */
std::string SCSIRequest::StatusString()
{
    EString errStr;

    switch (mTask->status)
    {
        case SCSI_STATUS_GOOD:
            return "STATUS Good";

        case SCSI_STATUS_CHECK_CONDITION:
            return "CHECK CONDITION";

        case SCSI_STATUS_CONDITION_MET:
            return "CONDITION MET";

        case SCSI_STATUS_BUSY:
            return "BUSY";

        case SCSI_STATUS_RESERVATION_CONFLICT:
            return "RESERVATION CONFLICT";

        case SCSI_STATUS_CANCELLED:
            return "CANCELLED";

        case SCSI_STATUS_ERROR:
            return "ERROR";

        default:
            errStr.Format("Unknown status code: %08x", mTask->status);
            return errStr;
    }
}

/*
 * Convert sense codes to strings
 */
std::string SCSIRequest::SenseKeyString()
{
    EString errStr;
    switch (mTask->sense.key)
    {
        case SCSI_SENSE_NO_SENSE:
            return "No Sense";

        case SCSI_SENSE_RECOVERED_ERROR:
            return "Recovered Error";

        case SCSI_SENSE_NOT_READY:
            return "Not Ready";

        case SCSI_SENSE_MEDIUM_ERROR:
            return "Medium Error";

        case SCSI_SENSE_HARDWARE_ERROR:
            return "Hardware Error";

        case SCSI_SENSE_ILLEGAL_REQUEST:
            return "Illegal Request";

        case SCSI_SENSE_UNIT_ATTENTION:
            return "Unit Attention";

        case SCSI_SENSE_DATA_PROTECTION:
            return "Data Protection";

        case SCSI_SENSE_BLANK_CHECK:
            return "Blank Check";

        default:
            errStr.Format("Unknown sense code: %02x", mTask->sense.key);
            return errStr;
    }
}

/*
 * Convert ASCQ values to strings
 */

std::string SCSIRequest::ASCQString()
{
    EString errStr;

    switch (mTask->sense.ascq)
    {
        case SCSI_SENSE_ASCQ_INVALID_FIELD_IN_CDB:
            return "Invalid field in CDB (0x2400)";

        case SCSI_SENSE_ASCQ_LOGICAL_UNIT_NOT_SUPPORTED:
            return "Logical unit not supported (0x2500)";

        case SCSI_SENSE_ASCQ_BUS_RESET:
            return "Power on, Reset or Bus Device Reset Occurred (0x2900)";

        case 0x4900:
            return "Invalid Message Error (0x4900)";

        default:
            errStr.Format("Unknown ASCQ: %04x", mTask->sense.ascq);
            return errStr;
    }
}


void SCSIRequest::setOutBuffer(boost::shared_array<uint8_t> buffer,
                               unsigned int bufferSize)
{
    mOutBuffer = buffer;
    mOutBufferSize = bufferSize;
}

void SCSIRequest::createOutBuffer(unsigned int length) {
    mOutBuffer = boost::shared_array<uint8_t>(new uint8_t[length]);
    mOutBufferSize = length;
    memset(mOutBuffer.get(), 0, mOutBufferSize);
}

void SCSIRequest::SetOutBufferBitArray(unsigned int byteOffset,
                                       unsigned int startBit, // starts at 0
                                       unsigned int bitLength,
                                       uint8_t val)
{
    setBufferBitArray(mOutBuffer.get(), mOutBufferSize, byteOffset,
                      startBit, bitLength, val);
}

void SCSIRequest::SetOutBufferBool(unsigned int byteOffset,
                                   unsigned int bitOffset,
                                   bool val)
{
    setBufferBool(mOutBuffer.get(), mOutBufferSize, byteOffset, bitOffset, val);
}

void SCSIRequest::SetOutBufferByte(unsigned int byteOffset,
                                   uint8_t val)
{
    setBufferByte(mOutBuffer.get(), mOutBufferSize, byteOffset, val);
}

void SCSIRequest::SetOutBufferShort(unsigned int byteOffset,
                                    uint16_t val)
{
    setBufferShort(mOutBuffer.get(), mOutBufferSize, byteOffset, val);
}

void SCSIRequest::SetOutBufferLong(unsigned int byteOffset,
                                   uint32_t val)
{
    setBufferLong(mOutBuffer.get(), mOutBufferSize, byteOffset, val);
}

void SCSIRequest::SetOutBufferString(unsigned int byteOffset,
                                      const std::string &val)
{
    setBufferString(mOutBuffer.get(), mOutBufferSize, byteOffset, val);

}


void SCSIRequest::setBufferBitArray(uint8_t *buffer,
                                    unsigned int bufferLength,
                                    unsigned int byteOffset,
                                    unsigned int startBit, // starts at 0
                                    unsigned int bitLength,
                                    uint8_t val)
{
    if (!buffer)
        throw CException("Uninitialized Buffer");

    CHECK_BUFFER_OVERFLOW(bufferLength, byteOffset, sizeof(uint8_t));
    CHECK_BYTE_BOUNDARY(startBit, bitLength);

    /* Check if value fits in bitLength, should be 0 (false) when shifted */
    if (val >> bitLength) {
        EString estr;
        estr.Format("%s: Value is not the correct Length: "
                "val=%u, expected length=%u", __func__, val, bitLength);
        throw CException(estr);
    }

    /*
     *  Example: startBit = 3, bitLength = 3, val = 0b111,
     *  buffer[byte] = 0b01010101
     *  Step 1: ~(0xFF << bitLength) = 0b00000111
     *  Step 2: ~(0b00000111 << startBit) = 0b11000111
     *  Step 3: buffer[byte] & 0b11000111 = 0b01000101
     *  Step 4: buffer[byte] = 0b01000101 | 0b00111000
     *  buffer[byte] = 011110101
     */
    buffer[byteOffset] = (buffer[byteOffset] & ~(~(0xFF << bitLength) << startBit)) |
                   (val << startBit);
}

void SCSIRequest::setBufferBool(uint8_t *buffer,
                                unsigned int bufferLength,
                                unsigned int byteOffset,
                                unsigned int bitOffset,
                                bool val)
{
    setBufferBitArray(buffer,bufferLength, byteOffset, bitOffset, 1, val ? 1:0);
}

void SCSIRequest::setBufferByte(uint8_t *buffer,
                                unsigned int bufferLength,
                                unsigned int byteOffset,
                                uint8_t val)
{
    if (!buffer)
        throw CException("Uninitialized Buffer");

    CHECK_BUFFER_OVERFLOW(bufferLength, byteOffset, sizeof(uint8_t));

    buffer[byteOffset] = val;
}

void SCSIRequest::setBufferShort(uint8_t *buffer,
                                 unsigned int bufferLength,
                                 unsigned int byteOffset,
                                 uint16_t val)
{
    if (!buffer)
        throw CException("Uninitialized Buffer");

    CHECK_BUFFER_OVERFLOW(bufferLength, byteOffset, sizeof(uint16_t));

    val = htons(val);
    memcpy(&buffer[byteOffset], &val, sizeof(uint16_t));
}

void SCSIRequest::setBufferLong(uint8_t *buffer,
                                unsigned int bufferLength,
                                unsigned int byteOffset,
                                uint32_t val)
{
    if (!buffer)
        throw CException("Uninitialized Buffer");

    CHECK_BUFFER_OVERFLOW(bufferLength, byteOffset, sizeof(uint32_t));

    val = htonl(val);
    memcpy(&buffer[byteOffset], &val, sizeof(uint32_t));
}

void SCSIRequest::setBufferString(uint8_t *buffer,
                                   unsigned int bufferLength,
                                   unsigned int byteOffset,
                                   const std::string &val)
{
    if (!buffer)
        throw CException("Uninitialized Buffer");

    CHECK_BUFFER_OVERFLOW(bufferLength, byteOffset, val.length());

    memcpy(&buffer[byteOffset], val.c_str(), val.length());
}

void SCSIRequest::setInBuffer(boost::shared_array<uint8_t> buffer,
                              unsigned int bufferSize)
{
    mInBuffer = buffer;
    mInBufferSize = bufferSize;
}

void SCSIRequest::createInBuffer(unsigned int length) {
    mInBuffer = boost::shared_array<uint8_t>(new uint8_t[length]);
    mInBufferSize = length;
    memset(mInBuffer.get(), 0, mInBufferSize);
}

bool SCSIRequest::GetInBufferBool(unsigned int byteOffset,
                                  unsigned int bitOffset) const
{
    return GetInBufferBitArray(byteOffset, bitOffset, 1) ? true: false;
}

uint8_t SCSIRequest::GetInBufferBitArray(unsigned int byteOffset,
                                         unsigned int startBit,
                                         unsigned int bitLength) const
{
    if (mTask->datain.data == NULL)
        throw CException("Uninitialized Buffer");

    CHECK_BUFFER_OVERREAD((unsigned int)mTask->datain.size, byteOffset, sizeof(uint8_t));
    CHECK_BYTE_BOUNDARY(startBit, bitLength);

    /*
     * Example: byte 01010101 need 3 bitLength at startBit 2
     * Step 1: (data[byteOffset] >> startBit): 00010101
     * Step 2: ~(0xFF << bitLength) 00000111
     * Returns: 00000101
     */
    return (uint8_t) ((mTask->datain.data[byteOffset] >> startBit) & ~(0xFF << bitLength));
}

uint8_t SCSIRequest::GetInBufferByte(unsigned int byteOffset) const {
    if (mTask->datain.data == NULL)
        throw CException("Uninitialized Buffer");

    CHECK_BUFFER_OVERREAD((unsigned int)mTask->datain.size, byteOffset, sizeof(uint8_t));
    return mTask->datain.data[byteOffset];
}


uint16_t SCSIRequest::GetInBufferShort(unsigned int byteOffset) const {
    uint16_t val;

    if (mTask->datain.data == NULL)
        throw CException("Uninitialized Buffer");

    CHECK_BUFFER_OVERREAD((unsigned int)mTask->datain.size, byteOffset, sizeof(uint16_t));

    memcpy(&val, &mTask->datain.data[byteOffset], sizeof(uint16_t));
    return ntohs(val);
}

uint32_t SCSIRequest::GetInBufferLong(unsigned int byteOffset) const {
    uint32_t val = 0;

    if (mTask->datain.data == NULL)
        throw CException("Uninitialized Buffer");

    CHECK_BUFFER_OVERREAD((unsigned int)mTask->datain.size, byteOffset, sizeof(uint32_t));
    memcpy(&val, &mTask->datain.data[byteOffset], sizeof(uint32_t));
    return ntohl(val);
}


std::string SCSIRequest::GetInBufferString(unsigned int byteOffset,
                                               unsigned int byteLength) const
{

    if (mTask->datain.data == NULL)
        throw CException("Uninitialized Buffer");

    CHECK_BUFFER_OVERREAD((unsigned int)mTask->datain.size, byteOffset, byteLength);

    return std::string((char *) &mTask->datain.data[byteOffset], byteLength);
}

