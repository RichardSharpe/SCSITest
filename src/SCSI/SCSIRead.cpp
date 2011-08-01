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
 * A SCSI Read class. All READ requests should go in here.
 *
 * Author: Richard Sharpe
 */

#include "SCSIRequest.h"
#include "SCSIRead.h"
#include <boost/shared_array.hpp>

SCSIRead10::SCSIRead10(unsigned int transferLength, 
                       boost::shared_array<uint8_t> buffer) :
    SCSIRequest(10), 
    mLBA(0)
{
    setCdbByte(0, SCSI_OPCODE_READ10); // That's a READ 10 request
    setCdbLong(2, 0);    // Default to LBA 0
    setCdbShort(7, transferLength);

    if (!buffer)
        createInBuffer(transferLength);
    else
        setInBuffer(buffer, transferLength);

    SetXferDir(SCSI_XFER_READ);
}

SCSIRead10::~SCSIRead10()
{
}

void SCSIRead10::SetLBA(uint32_t lba)
{
    mLBA = lba;
    setCdbLong(2, mLBA);
}
