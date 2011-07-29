/*
 * Extend std::string with a varargs-based format method.
 *
 * Richard Sharpe. Cribbed from elsewhere.
 */

#include <cstdarg>
#include "EString.h"

/*
 * Implement the Format method. We use vsnprintf twice. The first time with
 * NULL to find the length needed. The second time with the correct sized
 * string. Since this is mostly used in error paths we don't care about 
 * efficiency.
 */
void SCSIString::Format(const char *format, ...)
{
    va_list ap;
    int size;

    // We do one pass to get the size
    va_start(ap, format);
    size = vsnprintf(NULL, 0, format, ap);
    va_end(ap);

    // Now format the string after setting the size correctly
    clear();
    reserve(size + 1);
    resize(size);
    va_start(ap, format);
    vsnprintf(const_cast<char *>(data()), size + 1, format, ap);
    va_end(ap);
}
