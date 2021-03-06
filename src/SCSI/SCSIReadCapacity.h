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

#ifndef __SCSIReadCapacity_h__
#define __SCSIReadCapacity_h__

#include "iSCSILibWrapper.h"
#include "SCSIRequest.h"

class SCSIReadCapacity10 : public SCSIRequest
{
public:
    SCSIReadCapacity10();
    ~SCSIReadCapacity10();

    void SetLBA(uint32_t lba) { setCdbLong(2, lba); }
    unsigned int GetCapacity(void) { return GetInBufferLong(0); }
    unsigned int GetLogicalBlockLen(void) { return GetInBufferLong(4); }

private:
};

class SCSIReadCapacity16 : public SCSIRequest
{
public:
    SCSIReadCapacity16();
    ~SCSIReadCapacity16();

    void SetLBA(uint64_t lba) { setCdbLongLong(2, lba); }
    void SetAllocationLen(uint32_t len) { setCdbLong(10, len); }
};

#endif
