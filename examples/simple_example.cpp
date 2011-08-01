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

#include <vector>
#include <string>

#include "iSCSILibWrapper.h"
#include "SCSIReportLuns.h"
#include "SCSITestUnitReady.h"
#include "SCSIInquiry.h"

#include "EString.h"
#include "CException.h"

/*
 * A simple extension class for detecting redirects, if we want to. However,
 * you will have to allow some exceptions to continue below if you want to
 * have this work.
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

    iscsi.SetTarget(std::string("iqn.2011-07.com.example:testtarget"));
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
        SCSIReportLuns reportLuns;

        printf("\nSending REPORT LUNS request\n");
        iscsi.iSCSIExecSCSISync(reportLuns, 0);  // Always against LUN 0

        printf("Number of LUNs reported: %u\n", reportLuns.GetLunCount());

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
