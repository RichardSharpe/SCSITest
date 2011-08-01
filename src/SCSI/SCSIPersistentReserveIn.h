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

#ifndef __SCSIPersistentReserveIn_h__
#define __SCSIPersistentReserveIn_h__

#include "SCSIRequest.h"

class SCSIPersistentReserveIn : public SCSIRequest
{
public:
    enum ServiceAction {
        READ_KEYS               = 0x00,
        READ_RESERVATION        = 0x01,
        REPORT_CAPABILITIES     = 0x02,
        READ_FULL_STATUS        = 0x03,
    };

public:
    SCSIPersistentReserveIn(ServiceAction action,
                            unsigned int allocationLength = 255);
    virtual ~SCSIPersistentReserveIn() {}
    uint32_t GetPRGeneration();
private:
    ServiceAction mServiceAction;
};

class SCSIPersistentReserveInReadKeys : public SCSIPersistentReserveIn
{
public:
    SCSIPersistentReserveInReadKeys(unsigned int allocationLength = 255);
    virtual ~SCSIPersistentReserveInReadKeys() {}
    uint32_t GetKeyCount(void);
    std::string GetKey(unsigned int iterator);
};

class SCSIPersistentReserveInReadReservation : public SCSIPersistentReserveIn
{
public:
    SCSIPersistentReserveInReadReservation(unsigned int allocationLength = 255);
    virtual ~SCSIPersistentReserveInReadReservation() {}
    std::string GetReservation(void);
    scsi_persistent_reservation_type GetReservationType(void);
};

#endif
