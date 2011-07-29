/*
 * A common exception class ...
 *
 * Richard Sharpe
 */
#ifndef __EEXCEPTION_H__
#define __EEXCEPTION_H__

#include <exception> 
#include <string>

class CException: public std::exception
{
public:
    CException(std::string &reason) : mReason(reason) {} 
    CException(const char *reason) : mReason(reason) {}

    virtual ~CException() throw() {}

    virtual const std::string &getDesc() const throw() { return mReason; }

private:
    std::string mReason;
};
#endif
