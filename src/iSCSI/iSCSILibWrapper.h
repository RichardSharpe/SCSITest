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

#ifndef __iSCSILibWrapper_h__
#define __iSCSILibWrapper_h__

#include <vector>
#include <signal.h>

#include "SCSIRequest.h"
#include "EString.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include <poll.h>
extern "C" {
	#include "iscsi.h"
	#include "scsi-lowlevel.h"
}

/**
 * \struct client_state
 *
 * The state that gets used by call back functions. 
 * Not clear that it needs to be C stuff as yet.
 */

struct wrapper_client_state {
    int finished;
    int error;
    char *message;
    char *error_message;
    int connected;
    int logged_in;
    int has_discovered_target;
    char *target_name;
    char *target_address;
    int lun;
    int block_size;
};

class SCSIRequest;

/**
 * \class DiscoveryPair
 *
 * This provides abstracts a discovery pair, consisting of a Target name and
 * target address.
 **/
class WrapperDiscoveryPair
{
public:
    WrapperDiscoveryPair(const char *target, const char *address) :
        mTarget(target), mAddress(address)
        {}

    const std::string &GetTarget() const { return mTarget; }
    const std::string &GetAddress() const { return mAddress; }

private:
    std::string mTarget;
    std::string mAddress;
};

class iSCSILibWrapper;

/**
 * class iSCSIBackGround
 *
 * A singleton class that allows certain operations, like NOP_IN to be handled
 * when a connection is not actively being dealt with.
 */
class iSCSIBackGround
{
public:
    ~iSCSIBackGround();

    static iSCSIBackGround& GetInstance();

    void AddConnection(iSCSILibWrapper &iscsi);
    void RemoveConnection(iSCSILibWrapper &iscsi);

    // Stop the background thread if no connections
    void StopBackGroundTask();

private:
    iSCSIBackGround() {}
    iSCSIBackGround(iSCSIBackGround const &); // Hidden copy const
    iSCSIBackGround& operator=(iSCSIBackGround const&); // and assignment

    // Start the background thread ...
    void StartBackGroundTask();

    // The Background thread ...
    void BackGroundThread();  // We call it like a method

    boost::mutex mWorkMutex;
    boost::condition mWorkCond;
    boost::thread mThread;

    bool mStop;

    std::vector<iSCSILibWrapper *> mConnections;
};

/**
 * \class iSCSILibWrapper
 *
 * A wrapper class around the libiscsi library to make life easier for
 * test writers.
 *
 * It is intended to be subclassed so that users can replace certain methods
 * to perform their own operations.
 *
 * There will also have to be SCSI classes so we can send SCSI requests
 **/

class iSCSILibWrapper
{
friend class iSCSIBackGround;

protected:

public:
    enum ConnectionType {
        DiscoveryLogin, NormalLogin
    };

    iSCSILibWrapper(int timeout = -1);
    virtual ~iSCSILibWrapper();

    struct wrapper_client_state *GetClientState() { return &mClient; }

    void SetDiscoveryTarget(void) 
    {
        mTarget = "iqn.2008-09.com.scalecomputing:scqadleader";
    }

    void SetTarget(const std::string &target) { mTarget = target; }
    const std::string &GetTarget(void) const { return mTarget; }
    void SetAddress(const std::string &address) { mAddress = address; }
    const std::string &GetAddress(void) const { return mAddress; }
    const std::string &GetError(void) const { return mErrorString; }
    bool IsRedirected() { return mRedirected; }
    std::string &GetNewAddress() { return mNewAddress; }

    void iSCSIConnect(void);
    void iSCSIDiscoveryLogin(void);
    void iSCSIPerformDiscovery(void);
    void iSCSIDiscoveryLogout(void);
    void iSCSINormalLogin(void);

    // Use this call if you don't want to handle redirects yourself.
    // It still allows you to check for whether it was redirected and to
    // retrieve the address you were redirected to.
    void iSCSINormalLoginWithRedirect(void);
    void iSCSINormalLogout(void);
    void iSCSIExecSCSISync(SCSIRequest &request, unsigned int lun);
    void iSCSIDisconnect(void);

    // Task Management functions
    void iSCSITaskAbort(SCSIRequest &request);
    void iSCSITaskSetAbort(void);
    void iSCSILUNReset(uint32_t lun);
    void iSCSITargetWarmReset();
    void iSCSITargetColdReset();

    std::vector<WrapperDiscoveryPair> &GetDiscoveryList(void)
        { return mDiscoveryPairs; }
    void AddDiscoveryPair(const char *target, const char *addr)
        { mDiscoveryPairs.push_back(WrapperDiscoveryPair(target, addr)); }

    // Setting and getting the time when this should be looked at
    void SetTimeoutTime()
    {
        mBGTimeout = boost::get_system_time() + boost::posix_time::seconds(15);
    }

    boost::system_time GetTimeoutTime() { return mBGTimeout; }

    /*
     * These methods provide default ways of testing for success, but can be
     * overridden in the case that the user has a different or more rigorous
     * test.
     *
     * We only want to test mClient.error. Other errors would have been
     * thrown.
     */
    virtual bool iSCSIConnectTest(void) { return mClient.error == 0; }
    virtual bool iSCSIDiscoveryLoginTest(void) { return mClient.error == 0; }
    virtual bool iSCSIPerformDiscoveryTest(void) { return mClient.error == 0; }
    virtual bool iSCSIDiscoveryLogoutTest(void) { return mClient.error == 0; }
    virtual bool iSCSINormalLoginTest(void) { return mClient.error == false; }
    virtual bool iSCSINormalLogoutTest(void) { return mClient.error == false; }
    virtual bool iSCSIDisconnetTest(void) { return mClient.error == false; }

protected:

    void ServiceISCSIEvents(bool oneShot = false);

    int mTimeout;
    bool mError;
    bool mRedirected;
    std::string mNewAddress;
    EString mErrorString;
    struct wrapper_client_state mClient;
    struct iscsi_context *mIscsi;
    struct pollfd mPfd;
    std::string mAddress;
    std::string mTarget;
    std::vector<WrapperDiscoveryPair> mDiscoveryPairs;
    boost::system_time mBGTimeout;
};

#endif
