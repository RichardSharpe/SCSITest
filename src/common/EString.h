/*
 * An extended string class. This extends std::string by adding a varargs
 * format method.
 *
 * Richard Sharpe (cribbed from elsewhere).
 */
#ifndef __ESTRING_H__
#define __ESTRING_H__
class EString: public std::string
{
public:
  void Format(const char *format, ...) __attribute__((format(printf,2,3)));
};
#endif
