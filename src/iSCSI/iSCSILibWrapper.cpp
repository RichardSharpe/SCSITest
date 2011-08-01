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
 * A wrapper class around Ronnie Sahlberg's libiscsi C library.
 * This allows us to use C++ idioms in writing SCQAD tests.
 *
 * Author: Richard Sharpe
 **/

#include <algorithm>
#include <signal.h>
#include <errno.h>
#include "iSCSILibWrapper.h"
#include "EString.h"
#include "CException.h"

extern "C" {
	#include "iscsi-private.h"
}

// We should make sure that the thread has died ...
iSCSIBackGround::~iSCSIBackGround()
{
    StopBackGroundTask();  // All gone after here ...
}

iSCSIBackGround& iSCSIBackGround::GetInstance()
{
    static iSCSIBackGround theInstance; // Note, static

    return theInstance;
}

void iSCSIBackGround::StartBackGroundTask()
{
    mStop = false;
    //printf("%s: Starting background thread", __func__);
    mThread = boost::thread(&iSCSIBackGround::BackGroundThread, this);
}

void iSCSIBackGround::StopBackGroundTask()
{
    //printf("%s: checking whether to stop thread", __func__);
    // Only do this if no connections left
    if (mThread.joinable() && !mConnections.size())
    {
       // printf("%s: stopping background thread", __func__);

        boost::mutex::scoped_lock lock(mWorkMutex);

        mStop = true;

        lock.unlock();  // Have to let go some time

        mThread.join();
    }
}

//
// Take the work mutex and then add the iscsi object to the end of the
// vector. The object itself tells us when it should time out. Also
// signal the work condition variable to have the background thread take notice
//
// When ever the connection is controlled by the background thread the
// foreground has to take the object off the list to work on it. It must also
// take the mutex to do that.
void iSCSIBackGround::AddConnection(iSCSILibWrapper &iscsi)
{
    boost::mutex::scoped_lock lock(mWorkMutex); // Take the lock ...

    if (!mThread.joinable())
    {
        StartBackGroundTask();
    }

    iscsi.SetTimeoutTime();

    mConnections.push_back(&iscsi);

    mWorkCond.notify_one();
}

void iSCSIBackGround::RemoveConnection(iSCSILibWrapper &iscsi)
{
    unsigned int index = mConnections.size();

    boost::mutex::scoped_lock lock(mWorkMutex); // Take the lock ...

    // Now find and remove the object of interest
    for (unsigned int i = 0; i < mConnections.size(); i++)
    {
        if (mConnections[i] == &iscsi)
        {
            index = i;
            break;
        }
    }

    if (index < mConnections.size())
    {
        mConnections.erase(mConnections.begin() + index);  // Remove it
        mWorkCond.notify_one();  // Perhaps we should unlock first
    }
    else
    {
        EString errorStr;

        errorStr.Format("%s: No such item in vector: %s",
                        __func__,
                        strerror(errno));
        throw CException(errorStr);
    }
}

//
// The background thread. We wait for a condition variable to be signalled
// or a timeout to occur. If we get a timeout, it must be for the top item
// on the queue, so check to see if there are any incoming data on the
// connection and if so, process it.
//
// We will never get in the way of a real request response because the 
// iSCSILibWrapper must take connections away from us when it wants to 
// perform requests

void iSCSIBackGround::BackGroundThread()
{
    //printf("%s started", __func__);

    boost::system_time timeOut;

    bool signalled = true;

    while (!mStop)
    {
        boost::mutex::scoped_lock lock(mWorkMutex);

        // Figure out when to check again
        if (mConnections.size())
        {
            timeOut = mConnections[0]->GetTimeoutTime();
        }
        else
        {
            timeOut = boost::get_system_time() + boost::posix_time::seconds(30);
        }

        signalled = mWorkCond.timed_wait(lock, timeOut);

        // Do some work ... we have the lock ...

        //printf("%s should do some work ... signalled = %s",
        //     __func__, (signalled ? "yes" : "timed out"));

        if (!signalled)
        {
            if (mConnections.size())
            {
                mConnections[0]->ServiceISCSIEvents(true);

                mConnections[0]->SetTimeoutTime();

                if (mConnections.size() > 1)
                {
                    mConnections.push_back(mConnections[0]);
                    mConnections.erase(mConnections.begin());
                }

            }
            else
            {
                printf("%s: No connections after timeout!", __func__);
            }
        }

        // We don't actually do anything if we were signalled ...

        if (mStop)
        {
            //printf("%s: We were signalled to stop!", __func__);
            break;
        }

    }
    //printf("%s stopping", __func__);
}

/*
 * Constructor ...
 */
iSCSILibWrapper::iSCSILibWrapper(int timeout)
{
    mTimeout = timeout;  // Wait forever.
    mError = false;
    mRedirected = false;
    memset(&mClient, 0, sizeof(mClient));
    mIscsi = NULL;
}

iSCSILibWrapper::~iSCSILibWrapper()
{
    // This ungracefully shuts down the session
    if (mClient.connected)
        iscsi_disconnect(mIscsi);
    if (mClient.error_message)
        free(mClient.error_message);
    if (mClient.target_name)
        free(mClient.target_name);
    if (mClient.target_address)
        free(mClient.target_address);
    if (mIscsi)
        iscsi_destroy_context(mIscsi);
}

void iSCSILibWrapper::ServiceISCSIEvents(bool oneShot)
{
    // Event loop to drive the connection through its paces

    while (mClient.finished == 0 && mClient.error == 0 || oneShot)
    {
        int res = 0;

        mPfd.fd = iscsi_get_fd(mIscsi);
        mPfd.events = iscsi_which_events(mIscsi);

        if ((res = poll(&mPfd, 1, mTimeout)) <= 0)
        {
            mError = true;
            if (res)
                mErrorString.Format("%s: poll failed: %s ", 
                                    __func__, 
                                    strerror(errno));
            else
                mErrorString.Format("%s: poll timed out: %d mSec",
                                    __func__,
                                    mTimeout);
            throw CException(mErrorString);
        }

        if (iscsi_service(mIscsi, mPfd.revents) < 0)
        {
            mError = true;
            mErrorString.Format("%s: iscsi_service failed with: %s",
                               __func__,
                              iscsi_get_error(mIscsi));
            throw CException(mErrorString);
        }

        if (oneShot)
        {
            break;
        }
    }

    // Get the iscsi error if there is one at this point
    if (mClient.error != 0)
    {
        mErrorString.Format("%s: %s: %s", __func__,
                            mClient.error_message, 
                            iscsi_get_error(mIscsi));
	throw CException(mErrorString);
    }
}

/*
 * Connect Callback 
 */
static void connect_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data)
{
    struct wrapper_client_state *client = 
		((iSCSILibWrapper *)private_data)->GetClientState();

    if (status)
    {
	EString error;
        error.Format("%s: connection failed: %s",
                     __func__, 
                     iscsi_get_error(iscsi));
	client->error_message =  strdup(error.c_str());
        client->finished = 1;
        client->error = 1;
        return;
    }

    client->finished = 1;
    client->connected = 1;
}

/*
 * Connect to the target. Must set a target and address before calling this
 */
void iSCSILibWrapper::iSCSIConnect(void)
{
    if (mIscsi)
    {
        mError = true;
        mErrorString.Format("%s: Cannot connect! Already connected!",
                           __func__);
        throw CException(mErrorString);
    }

    mIscsi = iscsi_create_context(mTarget.c_str());
    if (!mIscsi)
    {
        mError = true;
        mErrorString.Format("%s: Creation of iscsi context for %s failed: %s",
                           __func__,
                           mTarget.c_str(),
                           iscsi_get_error(mIscsi));
        throw CException(mErrorString);
    }

    if (iscsi_set_alias(mIscsi, "scqadleader") != 0)
    {
        mError = true;
        mErrorString.Format("%s: Failed to add an alias for target %s: %s",
                           __func__,
                           mTarget.c_str(),
                            iscsi_get_error(mIscsi));
        throw CException(mErrorString);
    }

    std::string target = std::string(mAddress);

    // If it does not have the port number, add it on. This is actually not
    // The best test ... but we will often only be given strings from discovery.
    if (target.find(":3260") == std::string::npos)
        target.append(":3260");

    mClient.message = "Hello Server";
    mClient.has_discovered_target = 0;
    mClient.finished = 0;

    if (iscsi_connect_async(mIscsi, target.c_str(), connect_cb, this) 
        != 0)
    {
        mError = true;
        mErrorString.Format("%s : iscsi_connect_async to target %s failed: %s",
                           __func__,
                           target.c_str(),
                           iscsi_get_error(mIscsi));
        throw CException(mErrorString);
    }

    ServiceISCSIEvents();
}

/*
 * Discovery login callback
 */
static void discoverylogin_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data)
{
    struct wrapper_client_state *client = 
		((iSCSILibWrapper *)private_data)->GetClientState();

    if (status)
    {
        client->finished = 1;
        client->error = 1;
        return;
    }

    client->logged_in = 1;
    client->finished = 1;

}

/*
 * Do a Discovery login to the target
 */
void iSCSILibWrapper::iSCSIDiscoveryLogin(void)
{
    if (!mClient.connected || mClient.error)
    {
        if (mClient.error)
            mErrorString.Format("%s: previous error prevents login: %s",
                               __func__,
                               iscsi_get_error(mIscsi));
        else
            mErrorString.Format("%s: Login not possible without a connection!",
                               __func__);
        mError = true;
        throw CException(mErrorString);
    }

    iscsi_set_session_type(mIscsi, ISCSI_SESSION_DISCOVERY);

    mClient.finished = 0;

    if (iscsi_login_async(mIscsi, discoverylogin_cb, this))
    {
        mErrorString.Format("%s: iscsi_login_async for target %s failed: %s",
                           __func__,
                           mTarget.c_str(),
                           iscsi_get_error(mIscsi));
        mError = true;
        throw CException(mErrorString);
    }

    ServiceISCSIEvents();
}

/*
 * Discovery callback
 */
static void discovery_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data)
{
    iSCSILibWrapper *obj = (iSCSILibWrapper *)private_data;
    struct wrapper_client_state *client = obj->GetClientState();
    struct iscsi_discovery_address *addr;

    if (status)
    {
        EString error;
        error.Format("%s: Failed to perform discovery: %s",
                     __func__,
                     iscsi_get_error(iscsi));
        client->error_message = strdup(error.c_str());
        client->finished = 1;
        client->error = 1;
        return;
    }

    for (addr = (struct iscsi_discovery_address *)command_data; addr; addr = addr->next)
    {
        // Add them to the object that called us
        obj->AddDiscoveryPair(addr->target_name, addr->target_address);
    }

    client->finished = 1;
}

/*
 * Perform actual discovery against the target
 */
void iSCSILibWrapper::iSCSIPerformDiscovery(void)
{
    mClient.finished = 0;

    // Clean up those pairs ... from any previous discovery
    if (mDiscoveryPairs.size())
    {
       while (mDiscoveryPairs.size())
           mDiscoveryPairs.erase(mDiscoveryPairs.begin());
    }

    if (iscsi_discovery_async(mIscsi, discovery_cb, this))
    {
	mError = true;
        mErrorString.Format("%s: Failed to send discovery for target %s: %s",
                          __func__,
                          mTarget.c_str(),
                          iscsi_get_error(mIscsi));
        throw CException(mErrorString);
    }

    ServiceISCSIEvents();
}

/*
 * Discovery logout callback 
 */
static void discoverylogout_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data)
{
    struct wrapper_client_state *client = 
		((iSCSILibWrapper *)private_data)->GetClientState();

    if (status)
    {
	EString error;
        error.Format("%s: Failed to logout: %s",
                     __func__,
                     iscsi_get_error(iscsi));
        client->error_message = strdup(error.c_str());
        client->finished = 1;
        client->error = 1;
        return;
    }

    client->finished = 1;
}


/*
 * Perform a logout from a discovery session
 */
void iSCSILibWrapper::iSCSIDiscoveryLogout(void)
{
    mClient.finished = 0;

    if (iscsi_logout_async(mIscsi, discoverylogout_cb, this))
    {
        mErrorString.Format("%s: Logout failed for target %s: %s",
                          __func__,
                          mTarget.c_str(),
                          iscsi_get_error(mIscsi));
        mError = true;
        throw CException(mErrorString);
    }

    ServiceISCSIEvents();
}

/*
 * Perform a NormalLogin against the target. Note, the target must be fully
 * specified here.
 */
void iSCSILibWrapper::iSCSINormalLogin(void)
{
    if (!mClient.connected || mClient.error)
    {
        if (mClient.error)
            mErrorString.Format("%s: previous error prevents login for target %s: %s",
                        __func__,
                        mTarget.c_str(),
                        iscsi_get_error(mIscsi));
        else
            mErrorString.Format("%s: Login to target %s not possible without a connection!",
                               __func__,
                               mTarget.c_str());
        mError = true;
        throw CException(mErrorString);
    }

    mClient.finished = 0;

    iscsi_set_session_type(mIscsi, ISCSI_SESSION_NORMAL);
    iscsi_set_header_digest(mIscsi, ISCSI_HEADER_DIGEST_CRC32C_NONE);

    if (iscsi_set_targetname(mIscsi, mTarget.c_str()))
    {
        mErrorString.Format("%s: failed to set target (%s) name: %s",
                           __func__,
                           mTarget.c_str(),
                           iscsi_get_error(mIscsi));
        mError = true;
        throw CException(mErrorString);
    }

    if (iscsi_login_async(mIscsi, discoverylogin_cb, this))
    {
        mErrorString.Format("%s: iscsi_login_async to target %s failed: %s",
                           __func__,
                           mTarget.c_str(),
                           iscsi_get_error(mIscsi));
        mError = true;
        throw CException(mErrorString);
    }

    ServiceISCSIEvents();

    // Now check to see if we have been redirected and recover the new address.
    if (mClient.error)
    {
        std::string errStr(iscsi_get_error(mIscsi));

        if (errStr.find("Target moved temporarily(257)") != 
            std::string::npos)
        {
            mRedirected = true;
            mNewAddress = std::string(iscsi_get_target_address(mIscsi));
        }
    }
}

/*
 * Perform a normal login and do a redirect if needed. Caller should perform
 * a normal login test. We don't throw anything but the calls we make might
 * and we just let them through ...
 */
void iSCSILibWrapper::iSCSINormalLoginWithRedirect(void)
{
    iSCSINormalLogin();

    // If we were redirected, then disconnect and start again
    // We just perform all the correct steps
    if (IsRedirected())
    {
        std::string newAddress = GetNewAddress();

        iSCSIDisconnect();

        SetAddress(newAddress);

        iSCSIConnect();

        iSCSINormalLogin();
    }
}

/*
 * Perform a normal session logout
 */
void iSCSILibWrapper::iSCSINormalLogout(void)
{
    if (!mClient.connected || mClient.error)
    {
        if (mClient.error)
            mErrorString.Format("%s: previous error prevents logout from target %s: %s",
                               __func__,
                               mTarget.c_str(),
                               iscsi_get_error(mIscsi));
        else
            mErrorString.Format("%s: Logout from target %s not possible without a connection!",
                               __func__,
                               mTarget.c_str());
        mError = true;
        throw CException(mErrorString);
    }

    mClient.finished = 0;

    if (iscsi_logout_async(mIscsi, discoverylogout_cb, this))
    {
        mErrorString.Format("%s: Logout from target %s failed: %s",
                           __func__,
                           mTarget.c_str(),
                           iscsi_get_error(mIscsi));
        mError = true;
        throw CException(mErrorString);
    }

    ServiceISCSIEvents();
}

/*
 * Disconnect from the target
 */
void iSCSILibWrapper::iSCSIDisconnect(void)
{
    if (!mIscsi)
    {
        mErrorString.Format("%s: Disconnect from target %s not possible without a connection!",
                            __func__,
                            mTarget.c_str());
        mError = true;
        throw CException(mErrorString);
    }

    if (mClient.connected && iscsi_disconnect(mIscsi))
    {
        mErrorString.Format("%s: Failed to disconnect from target %s disconnect: %s",
                           __func__,
                           mAddress.c_str(),
                           iscsi_get_error(mIscsi));
        mError = true;
        throw CException(mErrorString);
    }

    mClient.finished = 0;

    iscsi_destroy_context(mIscsi);
    mIscsi = NULL;
    mClient.connected = 0; // We are no longer connected
    mClient.error = 0;     // There can be no error from here on in
}

/*
 * The callback for command execution
 */
static void exec_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data)
{
    struct wrapper_client_state *client = 
		((iSCSILibWrapper *)private_data)->GetClientState();
    struct scsi_task *task = (struct scsi_task *)command_data;

    if (task)
        task->status = status;
    client->finished = 1;
}

/*
 * Execute a SCSI request synchronously
 */
void iSCSILibWrapper::iSCSIExecSCSISync(SCSIRequest &request, unsigned int lun)
{
    struct iscsi_data data;
    struct scsi_task *task = request.GetTask();

    if (!mClient.connected || mClient.error)
    {
        if (mClient.error)
            mErrorString.Format("%s: previous error prevents executing SCSI request on target %s: %s",
                               __func__,
                               mTarget.c_str(),
                               iscsi_get_error(mIscsi));
        else
            mErrorString.Format("%s: Executing request on target %s not possible without a connection!",
                               __func__,
                               mTarget.c_str());
        mError = true;
        throw CException(mErrorString);
    }

    // You cannot re-execute a request unless you reset it
    if (request.IsExecuted())
    {
        EString estr;
        estr.Format("%s: SCSI Request already executed!", __func__);
        mError = true;
        throw CException(estr);
    }

    if (!task)  // Throw an exception
    {
        EString estr;
        estr.Format("%s: SCSIRequest does not have a task defined", __func__);
        mError = true;
        throw CException(estr);
    }

    switch (task->xfer_dir)
    {
        default:
        case SCSI_XFER_NONE:
            data.data = NULL;
            data.size = 0;
            break;

        case SCSI_XFER_READ:
            data.data = (unsigned char *)request.GetInBuffer().get();
            data.size = request.GetInBufferSize();
            break;

        case SCSI_XFER_WRITE:
            data.data = (unsigned char *)request.GetOutBuffer().get();
            data.size = request.GetOutBufferSize();
            break;
    }

    task-> expxferlen = data.size;

    mClient.finished = 0;

    if (iscsi_scsi_command_async(mIscsi, 
                                 lun, 
                                 task,
                                 exec_cb, 
                                 &data,
                                 this))
    {
        EString estr;
        estr.Format("%s: Error executing SCSI request: %s", __func__,
                    iscsi_get_error(mIscsi));
        mError = true;
        throw CException(mErrorString);
    }

    ServiceISCSIEvents();

    /*
     * Now, transfer any data back ... this might have to change if Ronnie
     * adds support for it in the library ...
     */

    switch (task->xfer_dir)
    {
    case SCSI_XFER_READ:
        memcpy(request.GetInBuffer().get(),
               task->datain.data, 
               std::min(request.GetInBufferSize(), 
                        (unsigned int)task->datain.size));
        break;
    default:
        break;
    }

    request.SetExecuted();
}

// Task management functions ...
static void lun_reset_cb(struct iscsi_context *iscsi, int status, void *command_data, void *private_data)
{
    struct wrapper_client_state *client = 
		((iSCSILibWrapper *)private_data)->GetClientState();
    struct scsi_task *task = (struct scsi_task *)command_data;

    if (task)
        task->status = status;
    if (status)
        client->error = 1;
    client->finished = 1;
}

void iSCSILibWrapper::iSCSILUNReset(uint32_t lun)
{
    if (!mClient.connected || mClient.error)
    {
        if (mClient.error)
            mErrorString.Format("%s: previous error prevents sending LUN reset to target %s: %s",
                               __func__,
                               mTarget.c_str(),
                               iscsi_get_error(mIscsi));
        else
            mErrorString.Format("%s: Sending LUN reset to target %s not possible without a connection!",
                               __func__,
                               mTarget.c_str());
        mError = true;
        throw CException(mErrorString);
    }

    mClient.finished = 0;

    if (iscsi_task_mgmt_async(mIscsi, 
                              lun, 
                              ISCSI_TM_LUN_RESET,
                              0xffffffff,
                              0,
                              lun_reset_cb,
                              this))
    {
        EString estr;
        estr.Format("%s: Error executing SCSI request: %s", __func__,
                    iscsi_get_error(mIscsi));
        mError = true;
        throw CException(mErrorString);
    }

    ServiceISCSIEvents();
}
