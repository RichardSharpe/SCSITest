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

#ifndef __SCSIInquiry_h__
#define __SCSIInquiry_h__

#include "iSCSILibWrapper.h"
#include "SCSIRequest.h"

/*
 * This class is for a basic inquiry. If you want EVPDs, subclass it 
 */
class SCSIInquiry : public SCSIRequest
{
public:
    SCSIInquiry() : 
        SCSIRequest(), 
        mEvpd(false)
    {
        createInBuffer(36);
        setCdbByte(0, 0x12); // That's an INQUIRY
        setCdbShort(3, mInBufferSize);
        SetXferDir(SCSI_XFER_READ);
    }

    SCSIInquiry(unsigned int allocationLength) :
        SCSIRequest(),
        mEvpd(false)
    {
        createInBuffer(allocationLength);
        setCdbByte(0, 0x12); // That's an INQUIRY
        setCdbShort(3, mInBufferSize);
        SetXferDir(SCSI_XFER_READ);
    }

    ~SCSIInquiry() {}

    uint8_t GetPeripheralQualifier(void) 
        { return GetInBufferBitArray(0, 5, 3); }
    uint8_t GetPeripheralType(void) { return GetInBufferBitArray(0, 0, 5); }
    bool GetRMB(void) { return GetInBufferBool(1, 7); }
    uint8_t GetVersion(void) { return GetInBufferByte(2); }
    void SetEVPD(bool evpd) { mEvpd = evpd; if (mEvpd) setCdbByte(1, 0x01); }

    std::string GetT10VendorID();
    std::string GetProductID();
    std::string GetProductRev();

protected:
    bool mEvpd;

private:
};

class SCSIInquirySupportedVPDPages : public SCSIInquiry
{
public:
    // Default to 32 supported pages in the response
    SCSIInquirySupportedVPDPages();

    // Do we have that page?
    bool HasPage(uint8_t page);

private:
    unsigned int mPageCount;
};

class SCSIDeviceID;

class SCSIInquiryUnitSerialNumVPDPage : public SCSIInquiry
{
public:
    SCSIInquiryUnitSerialNumVPDPage(unsigned int size = 16);

    std::string GetUnitSerialNum();
private:
};

class SCSIInquiryDeviceIdVPDPage : public SCSIInquiry
{
public:
    SCSIInquiryDeviceIdVPDPage(unsigned int size = 251);

    ~SCSIInquiryDeviceIdVPDPage() {}

    unsigned int GetDescriptorCount();
    const SCSIDeviceID& GetDescriptor(unsigned int descNo) const {
        return mDescriptors.at(descNo); }

private:
    bool mParsed;
    std::vector<SCSIDeviceID> mDescriptors;
};

// This is a helper class that provides access to the Device ID Descriptors
class SCSIDeviceID
{
public:
    enum IdentifierType {
        VENDOR_SPECIFIC         = 0x00,
        T10_VENDOR_ID           = 0x01,
        EUI_64                  = 0x02,
        NAA                     = 0x03,
        RELATIVE_TARGET_PORT_ID = 0x04,
        TARGET_PORT_GROUP       = 0x05,
        LOGICAL_UNIT_GROUP      = 0x06,
        MD5_LOGICAL_UNIT_ID     = 0x07,
        SCSI_NAME_STRING        = 0x08,
        IDENTIFIER_RESERVED     = 0x09,
    };

    enum CodeSet {
        CODESET_RESERVED        = 0x00,
        CODESET_BINARY          = 0x01,
        CODESET_ASCII           = 0x02,
        CODESET_UTF_8           = 0x03,
    };

    enum Association {
        ASSOCIATION_LUN           = 0x00,
        ASSOCIATION_TARGET_PORT   = 0x01,
        ASSOCIATION_TARGET_DEVICE = 0x02,
        ASSOCIATION_RESERVED      = 0x03,
    };

    SCSIDeviceID(const SCSIInquiryDeviceIdVPDPage *page, unsigned int offset) :
        mPage(page),
        mOffset(offset) {}

    unsigned int GetOffset() const { return mOffset; }
    CodeSet GetCodeSet() const;
    const std::string GetCodeSetFString() const;
    const std::string GetProtocolIDFString() const;
    IdentifierType GetIDType() const;
    const std::string GetIDTypeFString() const;
    unsigned int GetIDLength() const {
        return mPage->GetInBufferByte(mOffset + 3);
    }
    bool GetPIV() const { return mPage->GetInBufferBool(mOffset + 1, 7); }
    Association GetAssociation() const;
    const std::string GetID() const {
        return mPage->GetInBufferString(mOffset + 4, GetIDLength());
    }
    const std::string GetHexID() const;
    uint8_t GetIDShort(unsigned int offset) const {
        return mPage->GetInBufferShort(mOffset + 4 + offset);
    }

private:
    // These point into our parent
    const SCSIInquiryDeviceIdVPDPage *mPage;
    unsigned int mOffset;
};

#endif
