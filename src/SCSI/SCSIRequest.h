#ifndef __SCSIRequest_h__
#define __SCSIRequest_h__

#include "Exception.h"

#include "iSCSILibWrapper.h"
#include <boost/shared_array.hpp>

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

EXCEPTION_CLASS(ESCSIException);
EXCEPTION_SUBCLASS(ESCSIException, ESCSIExceptionBadCDBOffset);
EXCEPTION_SUBCLASS(ESCSIException, ESCSIExceptionInvalidCDBSize);
EXCEPTION_SUBCLASS(ESCSIException, ESCSIExceptionNoTransport);
EXCEPTION_SUBCLASS(ESCSIException, ESCSIMethodNotSupported);
EXCEPTION_SUBCLASS(ESCSIException, ESCSIInsufficientData);
EXCEPTION_SUBCLASS(ESCSIException, ESCSIExceptionUninitializedBuffer);
EXCEPTION_SUBCLASS(ESCSIException, ESCSIExceptionBufferOverRead);
EXCEPTION_SUBCLASS(ESCSIException, ESCSIExceptionBufferOverFlow);
EXCEPTION_SUBCLASS(ESCSIException, ESCSIExceptionByteBoundary);
EXCEPTION_SUBCLASS(ESCSIException, ESCSIExceptionInvalidValue);


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
        scsi_clean_scsi_task(mTask);
        delete mTask;
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
    void SetOutBufferFString(unsigned int byteOffset,
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
    std::string GetInBufferFString(unsigned int byteOffset,
                                                  unsigned int byteLength) const;

    unsigned char GetSCSIErrorType() { return mTask->sense.error_type; }
    enum scsi_sense_key GetSCSISenseKey() { return mTask->sense.key; }
    unsigned int GetSCSIASCQ() { return (unsigned int)mTask->sense.ascq; }
    scsi_residual GetResidualType() { return mTask->residual_status; }
    unsigned int GetResidual() { return mTask->residual; }

    void SetExecuted(void) { mExecuted = true; }
    bool IsExecuted(void) { return mExecuted; }

    virtual void Reset(void) 
    {
        mExecuted = false;
        scsi_clean_scsi_task(mTask);
    }

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

    void setBufferFString(uint8_t *buffer,
                                       unsigned int bufferLength,
                                       unsigned int byteOffset,
                                       const std::string &val);


    // Want these accessible ...
    struct scsi_task *mTask;

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
