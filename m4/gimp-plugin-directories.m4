#                         -*- Autoconf -*-

AC_DEFUN([GIMP_USER_INSTALL],
[
AC_MSG_CHECKING([whether to install the plugin in home directory])
AC_ARG_ENABLE([user-install],
              [AC_HELP_STRING([--enable-user-install],
                              [install the plugin in home directory @<:@no@:>@])],
              [case "${enableval}" in
                  yes) USER_INSTALL="yes" ; AC_MSG_RESULT([yes]) ;;
                  no)  USER_INSTALL="no"  ; AC_MSG_RESULT([no])  ;;
                  *) AC_MSG_RESULT([unknown])
                     AC_MSG_ERROR([bad value ${enableval} for --enable-user-install]) ;;
	      esac],
              [AC_MSG_RESULT([no])
              USER_INSTALL="no"])

AC_ARG_WITH([user-install-dir],
            [AC_HELP_STRING([--with-user-install-dir=DIR],
                            [directory for local installation of the plugin])],
            [XXX_WITH_USER_INSTALL_DIR="yes"],
            [XXX_WITH_USER_INSTALL_DIR="no"])
])

AC_DEFUN([GIMP_PLUGIN_DIRECTORIES],
[
AC_REQUIRE([GIMP_USER_INSTALL])
AC_PATH_PROG(GIMPTOOL, gimptool-2.0)

if test x$USER_INSTALL = xyes ; then
  INSTALL_OPT="install-bin"
else
  INSTALL_OPT="install-admin-bin"
fi
GIMPTOOL_OUTPUT=`$GIMPTOOL -n --$INSTALL_OPT dummy`

# "/home/username/.gimp-2.6/plug-ins" or "/usr/lib/gimp/2.0/plug-ins"
GIMP_PLUGIN_BINDIR=`echo "${GIMPTOOL_OUTPUT}" | cut -f 3 -d ' '`

# "/usr"
GIMP_PREFIX=`${PKG_CONFIG} --variable=prefix gimp-2.0`

# "/usr/share/locale"
GIMP_PLUGIN_LOCALEDIR=`${PKG_CONFIG} --variable=gimplocaledir gimp-2.0`

# We define the parent of the localedir to be the datadir for this
# plugin: "/usr/share"
GIMP_PLUGIN_DATADIR=`dirname ${GIMP_PLUGIN_LOCALEDIR}`

# Make GIMP_PLUGIN_DATADIR and GIMP_PLUGIN_LOCALEDIR relative to '${prefix}'.
QUOTED_GIMP_PREFIX=`echo ${GIMP_PREFIX} | sed -e 's/\\//\\\\\\//g'`
SED_SCRIPT="s/${QUOTED_GIMP_PREFIX}/\${prefix}/"

# "${prefix}/share"
GIMP_PLUGIN_DATADIR=`echo ${GIMP_PLUGIN_DATADIR} | sed -e ${SED_SCRIPT}`

# "${prefix}/share/locale"
GIMP_PLUGIN_LOCALEDIR=`echo ${GIMP_PLUGIN_LOCALEDIR} | sed -e ${SED_SCRIPT}`

# Make GIMP_PLUGIN_BINDIR relative to '${prefix}' and if necessary,
# adjust the prefix for installation in home directory.
if test x$USER_INSTALL = xyes ; then

  # "/home/username/.gimp-2.6"
  GIMP_PREFIX=`dirname ${GIMP_PLUGIN_BINDIR}`
  QUOTED_GIMP_PREFIX=`echo ${GIMP_PREFIX} | sed -e 's/\\//\\\\\\//g'`
  SED_SCRIPT="s/${QUOTED_GIMP_PREFIX}/\${prefix}/"

  # "${prefix}/plug-ins"
  GIMP_PLUGIN_BINDIR=`echo ${GIMP_PLUGIN_BINDIR} | sed -e ${SED_SCRIPT}`

  if test x$XXX_WITH_USER_INSTALL_DIR = xyes; then
    if test x$with_user_install_dir = x; then
      # "/home/username/.gimp-2.6"
      prefix=`echo ${GIMP_PREFIX}`
    else
      # "/path/to/some/other/place"
      prefix=`echo ${with_user_install_dir}`
    fi
  else
    # "/home/username/.gimp-2.6"
    prefix=`echo ${GIMP_PREFIX}`
  fi
  AC_SUBST(prefix)

  AC_DEFINE(RELATIVE_DIRS, 1, [Data and locale dirs are relative to plug-in executable])
  AC_DEFINE(RELATIVE_DIR_SHARE, "share", [Directory name for "share"])
  AC_DEFINE(RELATIVE_DIR_LOCALE, "locale", [Directory name for "locale"])

  dnl # This is a hack to make the *local* installation of i18n files work
  dnl # also in systems where such files are not installed in share.
  dnl # FIXME. Find a better solution instead of this hack.
  dnl DATADIRNAME=share
  dnl AC_SUBST(DATADIRNAME)

else

  # "${prefix}/lib/gimp/2.0/plug-ins"
  GIMP_PLUGIN_BINDIR=`echo ${GIMP_PLUGIN_BINDIR} | sed -e ${SED_SCRIPT}`

  AC_DEFINE(RELATIVE_DIRS, 0, [Data and locale dirs are relative to plug-in executable])

fi

AC_SUBST(GIMP_PLUGIN_BINDIR)
AC_SUBST(GIMP_PLUGIN_DATADIR)
AC_SUBST(GIMP_PLUGIN_LOCALEDIR)
AC_SUBST(GIMP_PLUGIN_RELATIVE_DIRS)
])
