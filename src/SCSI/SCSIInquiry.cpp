/**
 * A SCSI Inquiry Request class ... 
 *
 * Author: Richard Sharpe
 */

#include "SCSIInquiry.h"

std::string SCSIInquiry::GetT10VendorID(void)
{
    EString id;

    if (mEvpd)
    {
        id.Format("Method Not Supported: "
		  "This request has the EVPD bit set and is for page %02X."
                  " It does not support this request: %s!", 
                  mTask->cdb[2],
                  __func__);
        throw CException(id); 
    }

    if (GetInBufferTransferSize() < 16)
    {
        id.Format("Method Not Supported: "
		  "Allocation Length (%u) too short for this request: %s."
                  "Last request probably had a residual.",
                  mInBufferSize, __func__);
        throw CException(id);
    }


    return GetInBufferString(8, 8);
}

std::string SCSIInquiry::GetProductID()
{
    EString id;

    if (mEvpd)
    {
        id.Format("Method Not Supported: "
		  "This request has the EVPD bit set and is for page %02X."
                  " It does not support this request: %s!", 
                  mTask->cdb[2],
                  __func__);
        throw CException(id); 
    }

    return GetInBufferString(16, 16);
}

std::string SCSIInquiry::GetProductRev()
{
    EString rev;

    if (mEvpd)
    {
        rev.Format("Method Not Supported: "
		   "This request has the EVPD bit set and is for page %02X."
                   " It does not support this request: %s!", 
                   mTask->cdb[2],
                   __func__);
        throw CException(rev); 
    }

    return GetInBufferString(32, 4);
}

SCSIInquirySupportedVPDPages::SCSIInquirySupportedVPDPages() :
    SCSIInquiry(4 + 32),
    mPageCount(32)
{
    SetEVPD(true);
    setCdbByte(2, 0x00);
}


bool SCSIInquirySupportedVPDPages::HasPage(uint8_t page)
{
    unsigned int pageCount;

    // Check that the residual did not reduce the pages returned!
    // If we got an underflow, ie, there was less data than we asked
    // for, just use the returned count, otherwise subtract from
    // mPageCount the number of bytes that are missing, which will be zero
    // in the case that there was no residual :-)
    pageCount = (GetResidualType() == SCSI_RESIDUAL_UNDERFLOW) ?
                      GetInBufferByte(3) : mPageCount - GetResidual();

    for (unsigned int i = 0; i < pageCount; i++)
        if (page == GetInBufferByte(4+i))
            return true;
    return false;
}

SCSIInquiryUnitSerialNumVPDPage::SCSIInquiryUnitSerialNumVPDPage(unsigned int size) :
    SCSIInquiry(4 + size)
{
    SetEVPD(true);
    setCdbByte(2, 0x80);
}

std::string SCSIInquiryUnitSerialNumVPDPage::GetUnitSerialNum()
{
    EString usn;

    unsigned int serialLength = GetInBufferByte(3);
    return GetInBufferString(4,serialLength);
}

SCSIDeviceID::CodeSet SCSIDeviceID::GetCodeSet() const
{
    uint8_t ret = mPage->GetInBufferBitArray(mOffset, 0, 4);
    switch (ret)
    {
    case CODESET_BINARY:
    case CODESET_ASCII:
    case CODESET_UTF_8:
        break;
    default:
        ret = CODESET_RESERVED;
    }
    return static_cast<CodeSet>(ret);
}

const std::string SCSIDeviceID::GetCodeSetFString() const
{
    std::string codeSet;

    switch (GetCodeSet())
    {
    case CODESET_BINARY:
        codeSet = "Binary  ";
        break;
    case CODESET_ASCII:
        codeSet = "ASCII   ";
        break;
    case CODESET_UTF_8:
        codeSet = "UTF-8   ";
        break;
    default:
        codeSet = "Reserved";
        break;
    }

    return codeSet;
}

const std::string SCSIDeviceID::GetProtocolIDFString() const
{
    std::string protoId;

    if (!GetPIV())
    {
        protoId = "Reserved";
    }
    else
    {
        protoId = "Reserved"; // Figure this out later
    }

    return protoId;
}

SCSIDeviceID::IdentifierType SCSIDeviceID::GetIDType() const
{
    uint8_t idType = mPage->GetInBufferBitArray(mOffset + 1, 0, 4);
    switch(idType)
    {
    case VENDOR_SPECIFIC:
    case T10_VENDOR_ID:
    case EUI_64:
    case NAA:
    case RELATIVE_TARGET_PORT_ID:
    case TARGET_PORT_GROUP:
    case LOGICAL_UNIT_GROUP:
    case MD5_LOGICAL_UNIT_ID:
    case SCSI_NAME_STRING:
        break;
    default:
        idType = IDENTIFIER_RESERVED;
        break;
    }
    return static_cast<IdentifierType>(idType);
}

const std::string SCSIDeviceID::GetIDTypeFString() const
{
    std::string idType;

    switch (GetIDType())
    {
    case VENDOR_SPECIFIC:
        idType = "Vendor specific                ";
        break;
    case T10_VENDOR_ID:
        idType = "T10 vendor ID based            ";
        break;
    case EUI_64:
        idType = "EUI-64 based                   ";
        break;
    case NAA:
        idType = "NAA                            ";
        break;
    case RELATIVE_TARGET_PORT_ID:
        idType = "Relative target port identifier";
        break;
    case TARGET_PORT_GROUP:
        idType = "Target port group              ";
        break;
    case LOGICAL_UNIT_GROUP:
        idType = "Logical unit group             ";
        break;
    case MD5_LOGICAL_UNIT_ID:
        idType = "MD5 logical unit identifier    ";
        break;
    case SCSI_NAME_STRING:
        idType = "SCSI name string               ";
        break;
    default:
        idType = "Reserved                       ";
        break;
    }

    return idType;
}

SCSIDeviceID::Association SCSIDeviceID::GetAssociation() const
{
    return static_cast<Association>(mPage->GetInBufferBitArray(mOffset + 1, 4, 2));
}

const std::string SCSIDeviceID::GetHexID() const
{
    std::string id;
    id.reserve(GetIDLength() * 2);

    for (unsigned int i = 0; i < GetIDLength(); i++)
    {
         EString byte;
         byte.Format("%02X", mPage->GetInBufferByte(mOffset + 4 + i));
         id.append(byte);
    }

    return id;
}

SCSIInquiryDeviceIdVPDPage::SCSIInquiryDeviceIdVPDPage(unsigned int size) :
    SCSIInquiry(4 + size), 
    mParsed(false)
{
    SetEVPD(true);
    setCdbByte(2, 0x83);
}

unsigned int SCSIInquiryDeviceIdVPDPage::GetDescriptorCount()
{
    if (!mParsed)
    {
        // Parse them 
        unsigned int pageLength = GetInBufferShort(2);
        unsigned int offset = 4;

        // Offset is into the total response, while pageLength 
        // excludes the header
        while (offset < (pageLength + 4))
        {
            // There is a four byte header on the front
            unsigned int descLen = GetInBufferByte(offset + 3) + 4;

            mDescriptors.push_back(SCSIDeviceID(this, offset));

            offset += descLen;
        }
        mParsed = true;
    }

    return mDescriptors.size();
}
