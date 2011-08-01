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

#ifndef __SCSIRead_h__
#define __SCSIRead_h__

#include "iSCSILibWrapper.h"
#include "SCSIRequest.h"

class SCSIRead10 : public SCSIRequest
{
public:
    SCSIRead10(unsigned int transferLength,
               boost::shared_array<uint8_t> buffer = boost::shared_array<uint8_t>());
    ~SCSIRead10();

    void SetLBA(uint32_t lba);

private:
    SCSIRead10();
    unsigned int mLBA;
};

#endif
