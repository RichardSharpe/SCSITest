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
#include "SCSIReserve.h"

SCSIReserve6::SCSIReserve6() :
    SCSIRequest(6) 
{
    setCdbByte(0, SCSI_OPCODE_RESERVE6); // That's a RESERVE 10 request
}

SCSIReserve6::~SCSIReserve6()
{
}
