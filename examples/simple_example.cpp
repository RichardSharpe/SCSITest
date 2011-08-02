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

/*
 * A simple example that:
 * 1. Connects to the target specified as argv[1],
 * 2. Does a discovery login and discovery
 * 3. Connects to the first target, if any.
 * 4. Lists the LUNs.
 * 5. Does an INQUIRY against the first LUN and lists the info.
 *
 * Richard Sharpe, Scale Computing.
 */

#include <stdio.h>
#include <vector>
#include <string>

#include "iSCSILibWrapper.h"
#include "SCSIReportLuns.h"
#include "SCSITestUnitReady.h"
#include "SCSIInquiry.h"
#include "SCSIReadCapacity.h"

#include "EString.h"
#include "CException.h"

/*
 * A simple extension class for detecting redirects, if we want to. However,
 * you will have to allow some exceptions to continue below if you want to
 * have this work. We don't actually need this class in this example, but it
 * is included to show what to do.
 */
class myiSCSILibWrapper : public iSCSILibWrapper
{

public:
    virtual bool iSCSINormalLoginTest(void);

private:
};

bool myiSCSILibWrapper::iSCSINormalLoginTest(void)
{
    if (mClient.error)
    {
        std::string errStr(iscsi_get_error(mIscsi));

        if (errStr.find("Target moved temporarily(257)") != std::string::npos)
        {
            return true;
        }

        return false;
    }

    return true;
}

// Look at some of the LUNs. The SCSIReportLuns object is passed by reference
// It contains info about the actual LUN numbers ...
static bool ReportOnLuns(myiSCSILibWrapper &iscsi,
                         SCSIReportLuns &reportLuns)
{
    for (unsigned int i = 0; i < reportLuns.GetLunCount(); i++)
    {
        SCSITestUnitReady tur;

        unsigned int lun = reportLuns.GetLun(i);

        printf("\nAbout to issue TEST UNIT READY for lun %u\n", lun);

        // If this is the first command, we expect a BUS RESET, so
        // could test for that ...
        iscsi.iSCSIExecSCSISync(tur, lun);
        if (tur.GetStatus() != SCSI_STATUS_GOOD)
        {
            // Check what the problem is
            if (tur.GetStatus() != SCSI_STATUS_GOOD)
            {
                unsigned int ascq = tur.GetSCSIASCQ();

                if (ascq == SCSI_SENSE_ASCQ_BUS_RESET)
                    printf("Received expected BUS RESET\n");
                else 
                {
                    printf("Test Unit Ready failed: "
                           "Status: %s, SenseKey: %s, ASCQ: %s\n",
                           tur.StatusString().c_str(),
                           tur.SenseKeyString().c_str(),
                           tur.ASCQString().c_str());
                    return false;
                }
            }
            else
            {
                printf("Test Unit Ready failed: "
                       "Status: %s, SenseKey: %s, ASCQ: %s\n",
                       tur.StatusString().c_str(),
                       tur.SenseKeyString().c_str(),
                       tur.ASCQString().c_str());
                return false;
            }
        }

        // Now, send another TUR to see if all OK.
        SCSITestUnitReady tur2;

        iscsi.iSCSIExecSCSISync(tur2, lun);

        if (tur2.GetStatus() != SCSI_STATUS_GOOD)
        {
            printf("Test Unit Ready failed: "
                   "Status: %s, SenseKey: %s, ASCQ: %s\n",
                   tur.StatusString().c_str(),
                   tur.SenseKeyString().c_str(),
                   tur.ASCQString().c_str());
            return false;
        }

        /*
         * Send an Inquiry
         * 
         * The various Inquiry classes have methods that give access to the
         * important fields. There are several sub-classes as well, like 
         * SCSIInquirySupportedVPDPages and so forth.
         */

        SCSIInquiry inq;  // Just a normal inquiry

        iscsi.iSCSIExecSCSISync(inq, lun);

        if (inq.GetStatus() != SCSI_STATUS_GOOD)
        {
            printf("Inquiry fauled: Status %s, SenseKey: %s, ASCQ: %s\n",
                   inq.StatusString().c_str(),
                   inq.SenseKeyString().c_str(),
                   inq.ASCQString().c_str());
            return false;
        }

        printf("T10 Vendor ID for LUN %u is \"%s\"\n",
               lun, inq.GetT10VendorID().c_str());
        printf("Product ID for LUN %u is \"%s\"\n",
               lun, inq.GetProductID().c_str());
        printf("Product Rev for LUN %u is \"%s\"\n",
               lun, inq.GetProductRev().c_str());

        SCSIInquirySupportedVPDPages supported;

        iscsi.iSCSIExecSCSISync(supported, lun);

        if (supported.GetStatus() != SCSI_STATUS_GOOD)
        {
            printf("Inquiry failed: Status %s, SenseKey: %s, ASCQ: %s\n",
                   supported.StatusString().c_str(),
                   supported.SenseKeyString().c_str(),
                   supported.ASCQString().c_str());
            return false;
        }

        if (supported.HasPage(0))
        {
            printf("Lun %u supports VPD Page 0, checking page 0x80\n", lun);

            if (supported.HasPage(0x80))
            {
                SCSIInquiryUnitSerialNumVPDPage usn;

                iscsi.iSCSIExecSCSISync(usn, lun);

                if (usn.GetStatus() == SCSI_STATUS_GOOD)
                {
                    printf("LUN %u Unit Serial Num is \"%s\"\n", lun,
                           usn.GetUnitSerialNum().c_str());
                }
            }

            // More pages left as an exercise for the reader ...

        }
        else
        {
            printf("Weird, LUN %u does not suppot VPD Page 0!\n", lun);
        }

        // Retrieve the capacity
        SCSIReadCapacity10 capacity10;

        iscsi.iSCSIExecSCSISync(capacity10, lun);

        if (capacity10.GetStatus() != SCSI_STATUS_GOOD)
        {
            printf("ReadCapacity19 failed: Status %s, SenseKey: %s, ASCQ: %s\n",
                   capacity10.StatusString().c_str(),
                   capacity10.SenseKeyString().c_str(),
                   capacity10.ASCQString().c_str());
            return false;
        }

        printf("Lun %u has a capacity of %u blocks of length %u bytes!\n",
               lun, 
               capacity10.GetCapacity(),
               capacity10.GetLogicalBlockLen());
    }

    return true;
}

/*
 * Connect to the first arg ...
 */
int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        printf("Usage: %s <ip-addr-of-target>\n", argv[0]);
        exit(1);
    }

    /*
     * Now try to connect etc ...
     */
    myiSCSILibWrapper iscsi;

    /*
     * If you want multiple SCSI connections here, just declare more 
     * myiSCSILibWrapper objects and give them different initiator names.
     */
    iscsi.SetInitiator(std::string("iqn.2011-07.com.testiscsi.initiator1"));

    // This target name is not really relevant initially
    iscsi.SetTarget(std::string("iqn.2011-07.com.example:testtarget1"));
    iscsi.SetAddress(argv[1]);

    try {
        printf("Connecting to %s:%s\n", 
               iscsi.GetTarget().c_str(),
               iscsi.GetAddress().c_str());

        // Exception thrown if problems ...
        iscsi.iSCSIConnect();

        // Do discovery login, exception thrown etc.
        iscsi.iSCSIDiscoveryLogin();

        iscsi.iSCSIPerformDiscovery();

        // Now get the targets
        std::vector<WrapperDiscoveryPair> &DiscoveryList =
                                               iscsi.GetDiscoveryList();

        if (DiscoveryList.size() == 0) 
            throw CException("No targets defined!");

        for (unsigned int i = 0; i < DiscoveryList.size(); i++)
        {
            printf("Target \"%s\" is at address \"%s\"\n",
                   DiscoveryList[i].GetTarget().c_str(),
                   DiscoveryList[i].GetAddress().c_str());
        }

        // Now logout and disconnect 
        iscsi.iSCSIDiscoveryLogout();

        iscsi.iSCSIDisconnect();

        // Now, connect to the second target found above
        // You might have to adjust this!
        iscsi.SetTarget(DiscoveryList[1].GetTarget());
        iscsi.SetAddress(DiscoveryList[1].GetAddress());

        printf("\nConnecting to target %s at address %s\n",
               iscsi.GetTarget().c_str(),
               iscsi.GetAddress().c_str());

        iscsi.iSCSIConnect();

        // Now, log in ...
        iscsi.iSCSINormalLogin();

        // Now, find out how many LUNs there are.
        SCSIReportLuns *reportLuns = new SCSIReportLuns();

        printf("\nSending REPORT LUNS request\n");
        iscsi.iSCSIExecSCSISync(*reportLuns, 0);  // Always against LUN 0

        printf("Number of LUNs reported: %u\n", reportLuns->GetLunCount());

        // Check if we had a SCSI_RESIDUAL_OVERFLOW
        if (reportLuns->GetResidualType() == SCSI_RESIDUAL_OVERFLOW)
        {
            // Try again
            printf("Underflow on REPORT LUNS!\n");
            unsigned int size = reportLuns->GetInBufferTransferSize() +
                                reportLuns->GetResidual();
            printf("Trying REPORT LUNS with larger buffer size of %u\n", size);
            delete reportLuns;

            // Allocate a new object with the correct buffer size
            reportLuns = new SCSIReportLuns(size);

            iscsi.iSCSIExecSCSISync(*reportLuns, 0);
        }

        if (!ReportOnLuns(iscsi, *reportLuns)) {
            delete reportLuns;  // Gotta free it!
            throw CException("Error reporting on LUNS");
        }

        delete reportLuns;  // Free that object

        printf("\nLogging out and disconnecting\n");
        iscsi.iSCSINormalLogout();

        iscsi.iSCSIDisconnect();

        printf("\n");
    }
    catch (CException &e)
    {
        // If you want to handle redirects you need to check what sort of
        // Exception message it is and return if it is a redirect
        printf("Caught Exception: %s\n", e.getDesc().c_str());
    }

    printf("Test done!\n");
}
