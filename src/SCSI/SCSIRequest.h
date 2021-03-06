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

#ifndef __SCSIRequest_h__
#define __SCSIRequest_h__

#include <stdint.h>
#include <string.h>
#include <exception>
#include <cstdarg>
#include <string>

#include <boost/shared_array.hpp>

#include "iSCSILibWrapper.h"

#include "EString.h"
#include "CException.h"

extern "C" {
    #include "scsi-lowlevel.h"
    #include "iscsi.h"
}

/**
 * \class SCSIRequest
 *
 * A base class for SCSI Requests. These will be extended for the particular
 * classes of interest.
 *
 * This class should provide a basic set of services that can be added to by
 * the actual request classes, like SCSI_Test_Unit_Ready, SCSI_Inquiry, etc
 **/

class SCSIRequest
{
public:
    enum { SCSI_DEF_SENSE_BUFFER_SIZE = 96 };
    SCSIRequest();
    SCSIRequest(unsigned int cdbSize);
    SCSIRequest(unsigned int cdbSize, 
                boost::shared_array<uint8_t> outBuffer, unsigned int outBufferSize,
                boost::shared_array<uint8_t> inBuffer, unsigned int inBufferSize);

    virtual ~SCSIRequest()
    {
        // We are not responsible for any inBuffer or outBuffer ...
        scsi_free_scsi_task(mTask);
    }

    /*enum SCSI_STATUS_CODES
    {
        SCSI_STATUS_GOOD                 = 0,
        SCSI_STATUS_CHECK_CONDITION      = 2,
        SCSI_STATUS_CONDITION_MET        = 4,
        SCSI_STATUS_BUSY                 = 8,
        SCSI_STATUS_RESERVATION_CONFLICT = 0x000000018,
        SCSI_STATUS_TASK_SET_FULL        = 0x000000028,
        SCSI_STATUS_ACA_ACTIVE           = 0x000000030,
        SCSI_STATUS_TASK_ABORTED         = 0x000000040,
        SCSI_STATUS_CANCELLED            = 0x0f0000000,
        SCSI_STATUS_ERROR                = 0x0f0000001
    };*/

    void SetXferDir(enum scsi_xfer_dir dirn) { mTask->xfer_dir = dirn; }
    scsi_status GetStatus(void) { return (scsi_status)mTask->status; }

    boost::shared_array<uint8_t> GetOutBuffer(void) { return mOutBuffer; }
    unsigned int GetOutBufferSize(void) { return mOutBufferSize; }
    void ResetOutBuffer(void) { if (mOutBuffer) memset(mOutBuffer.get(), 0, mOutBufferSize); }
    void SetOutBufferBitArray(unsigned int byteOffset,
                              unsigned int startBit, // starts at 0
                              unsigned int bitLength,
                              uint8_t val);
    void SetOutBufferBool(unsigned int byteOffset,
                          unsigned int bitOffset,
                          bool val);
    void SetOutBufferByte(unsigned int byteOffset,
                          uint8_t val);
    void SetOutBufferShort(unsigned int byteOffset,
                           uint16_t val);
    void SetOutBufferLong(unsigned int byteOffset,
                          uint32_t val);
    void SetOutBufferString(unsigned int byteOffset,
                            const std::string &val);

    boost::shared_array<uint8_t> GetInBuffer(void) { return mInBuffer; }
    unsigned int GetInBufferSize(void) { return mInBufferSize; }

    /**
     *  Gets size of data actually written to the InBuffer
     *  @params[out] length Amount of data actually written to the buffer
     */
    void ResetInBuffer(void) { if (mInBuffer) memset(mInBuffer.get(), 0, mInBufferSize); }
    unsigned int GetInBufferTransferSize(void) { return mTask->datain.size; }
    bool GetInBufferBool(unsigned int byteOffset,
                                      unsigned int bitOffset) const;
    uint8_t GetInBufferBitArray(unsigned int byteOffset,
                                             unsigned int startBit,
                                             unsigned int bitLength) const;
    uint8_t GetInBufferByte(unsigned int byteOffset) const;
    uint16_t GetInBufferShort(unsigned int byteOffset) const;
    uint32_t GetInBufferLong(unsigned int byteOffset) const;
    std::string GetInBufferString(unsigned int byteOffset,
                                  unsigned int byteLength) const;

    unsigned char GetSCSIErrorType() { return mTask->sense.error_type; }
    enum scsi_sense_key GetSCSISenseKey() { return mTask->sense.key; }
    unsigned int GetSCSIASCQ() { return (unsigned int)mTask->sense.ascq; }
    scsi_residual GetResidualType() { return mTask->residual_status; }
    unsigned int GetResidual() { return mTask->residual; }

    void SetExecuted(void) { mExecuted = true; }
    bool IsExecuted(void) { return mExecuted; }

    std::string StatusString();
    std::string ErroTypeString();
    std::string SenseKeyString();
    std::string ASCQString();

    scsi_task *GetTask(void) { return mTask; }

protected:
    /**
     *  Set's Buffer to be sent
     *  @params[in] buffer the buffer, pass NULL if empty
     *  @params[in] bufferSize size of buffer, 0 if empty
     */
    void setOutBuffer(boost::shared_array<uint8_t> buffer, unsigned int bufferSize);
    /**
     *  Creates's Buffer to be sent
     *  @params[in] length positive integer describing size of buffer to create
     */
    void createOutBuffer(unsigned int length);
    /**
     *  Set's Buffer to write recieve data to
     *  @params[in] buffer the buffer, pass NULL if empty
     *  @params[in] bufferSize size of buffer, 0 if empty
     */
    void setInBuffer(boost::shared_array<uint8_t> buffer, unsigned int bufferSize);
    /**
     *  Creates's Buffer to be sent
     *  @params[in] length positive integer describing size of buffer to create
     */
    void createInBuffer(unsigned int length);

    void setCdbBitArray(unsigned int byteOffset,
                        unsigned int startBit, // starts at 0
                        unsigned int bitLength,
                        uint8_t val);
    void setCdbByte(unsigned int byteOffset, uint8_t val);
    void setCdbShort(unsigned int byteOffset, uint16_t val);
    void setCdbLong(unsigned int byteOffset, uint32_t val);
    void setCdbLongLong(unsigned int byteOffset, uint64_t val);

    /* Helper methods for writing values into buffers */
    void setBufferBitArray(uint8_t *buffer,
                           unsigned int bufferLength,
                           unsigned int byteOffset,
                           unsigned int startBit, // starts at 0
                           unsigned int bitLength,
                           uint8_t val);

    void setBufferBool(uint8_t *buffer,
                       unsigned int bufferLength,
                       unsigned int byteOffset,
                       unsigned int bitOffset,
                       bool val);

    void setBufferByte(uint8_t *buffer,
                       unsigned int bufferLength,
                       unsigned int byteOffset,
                       uint8_t val);

    void setBufferShort(uint8_t *buffer,
                        unsigned int bufferLength,
                        unsigned int byteOffset,
                        uint16_t val);

    void setBufferLong(uint8_t *buffer,
                       unsigned int bufferLength,
                       unsigned int byteOffset,
                       uint32_t val);

    void setBufferLongLong(uint8_t *buffer,
                           unsigned int bufferLength,
                           unsigned int byteOffset,
                           uint64_t val);

    void setBufferString(uint8_t *buffer,
                         unsigned int bufferLength,
                         unsigned int byteOffset,
                         const std::string &val);


    // Want these accessible ...
    struct scsi_task *mTask;

    // Dirty little secret ... 
    void scsi_clean_scsi_task(struct scsi_task *task);

protected:
    bool mExecuted;
    bool mLinkBit;
    int lun;
    boost::shared_array<uint8_t> mOutBuffer;
    unsigned int mOutBufferSize;
    boost::shared_array<uint8_t> mInBuffer;
    unsigned int mInBufferSize;

};

#endif
