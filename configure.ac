AC_INIT(configure.in)

AC_PROG_CXX(CC g++)

AC_CANONICAL_HOST
case "$host" in
  *-*-linux*)
dnl     CXXFLAGS="$CXXFLAGS -funroll-loops -ffast-math -malign-double -mcpu=pentiumpro -march=pentiumpro -fomit-frame-pointer -fno-exceptions -DLinux -Wall"
     CXXFLAGS="$CXXFLAGS -Wall"
     CPPFLAGS="$CPPFLAGS -DLinux"
     ADDLDFLAGS="-lpthread -lrt"
     AC_ENABLE_SHARED(yes) 
     AC_ENABLE_STATIC(no)
 ;;
  *-*-solaris*)
    CPPFLAGS="$CPPFLAGS -DSunOS"
     AC_ENABLE_SHARED(no)
     AC_ENABLE_STATIC(yes)
  ;;
  *)
    AC_MSG_ERROR([default compiler unknown for this system!])
  ;;
esac


AM_INIT_AUTOMAKE(dataflow,1.00)


AC_ARG_ENABLE(database,
[  --enable-database   enable code to write to the database],
[case "${enableval}" in
  yes) database=true ; AC_DEFINE(DATABASE,"1") ;;
  no)  database=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --database) ;;
esac ],[database=false])
AM_CONDITIONAL(DATABASE, test x$database = xtrue)


AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_SUBST(ADDLDFLAGS)
AC_HEADER_STDC
AC_CHECK_HEADERS(getopt.h)
AC_CHECK_HEADERS(bstring.h)

AC_OUTPUT(Makefile)
