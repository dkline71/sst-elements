
AC_DEFUN([SST_CHECK_PRESSIM], [
  AC_ARG_WITH([presSim],
    [AS_HELP_STRING([--with-presSim@<:@=DIR@:>@],
      [Use PresSim package installed in optionally specified DIR])])

  sst_check_pressim_happy="yes"
  AS_IF([test "$with_presSim" = "no"], [sst_check_pressim_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_presSim" -a "$with_presSim" != "yes"],
    [PRESSIM_CPPFLAGS="-I$with_presSim -DHAVE_PRESSIM"
     CPPFLAGS="$PRESSIM_CPPFLAGS $CPPFLAGS"
     PRESSIM_LDFLAGS="-L$with_presSim"
     PRESSIM_LIBDIR="$with_presSim"
     LDFLAGS="$PRESSIM_LDFLAGS $LDFLAGS"],
    [PRESSIM_CPPFLAGS=
     PRESSIM_LDFLAGS=
     PRESSIM_LIBDIR=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([PresSim.h], [], [sst_check_pressim_happy="no"])
  AC_CHECK_LIB([presSim], [libpresSim_is_present],
    [PRESSIM_LIB="-lpresSim"], [sst_check_pressim_happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([PRESSIM_CPPFLAGS])
  AC_SUBST([PRESSIM_LDFLAGS])
  AC_SUBST([PRESSIM_LIB])
  AC_SUBST([PRESSIM_LIBDIR])
  AM_CONDITIONAL([HAVE_PRESSIM], [test "$sst_check_pressim_happy" = "yes"])
  AS_IF([test "$sst_check_pressim_happy" = "yes"],
        [AC_DEFINE([HAVE_PRESSIM], [1], [Set to 1 if PresSim was found])])
  AC_DEFINE_UNQUOTED([PRESSIM_LIBDIR], ["$PRESSIM_LIBDIR"], [Path to PresSim library])

  AS_IF([test "$sst_check_pressim_happy" = "yes"], [$1], [$2])
])
