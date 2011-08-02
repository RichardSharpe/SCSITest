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
 */

/**
 * A SCSI ReadCapacity class. All Release requests should go in here.
 *
 * Author: Richard Sharpe
 */

#include "SCSIRequest.h"
#include "SCSIReadCapacity.h"

SCSIReadCapacity10::SCSIReadCapacity10() :
    SCSIRequest(10) 
{
    createInBuffer(8);   // 8 Byte response buffer
    setCdbByte(0, 0x25); // That's a READ CAPACITY 10 request
    SetXferDir(SCSI_XFER_READ);
}

SCSIReadCapacity10::~SCSIReadCapacity10()
{
}

SCSIReadCapacity16::SCSIReadCapacity16() :
    SCSIRequest(16)
{
    createInBuffer(16);   // 16-byte response buffer
    setCdbByte(0, 0x9E);  // ServiceAction In, actually
    setCdbByte(1, 0x10);  // The readCapacity value
}

SCSIReadCapacity16::~SCSIReadCapacity16()
{

}
