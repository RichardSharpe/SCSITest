AC_PREREQ(2.59)
AC_INIT([SCSITest], [0.1], [realrichardsharpe@gmail.com])
AC_CONFIG_SRCDIR([examples/simple_example.cpp])
#AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

AC_BOOST_BASE([1.43.0])
AC_BOOST_THREAD
# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h stdint.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset strdup strerror])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
