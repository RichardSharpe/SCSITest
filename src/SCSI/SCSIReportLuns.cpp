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
 * Author: Richard Sharpe
 */

#include "SCSIRequest.h"
#include "SCSIReportLuns.h"

SCSIReportLuns::SCSIReportLuns(unsigned int allocationLength) :
    SCSIRequest(12)
{
    setCdbByte(0, SCSI_OPCODE_REPORTLUNS); // That's a REPORT LUNS request
    setCdbByte(2, 0x02); // All of them
    setCdbShort(6, allocationLength);
    createInBuffer(allocationLength);
    SetXferDir(SCSI_XFER_READ);
}
