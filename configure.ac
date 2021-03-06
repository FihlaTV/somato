## Copyright (c) 2004-2017  Daniel Elstner  <daniel.kitta@gmail.com>
##
## This file is part of Somato.
##
## Somato is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## Somato is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with Somato.  If not, see <http://www.gnu.org/licenses/>.

AC_INIT([Somato], [0.8], [daniel.kitta@gmail.com], [somato],
        [https://github.com/danielkitta/somato])
AC_PREREQ([2.63])

AC_CONFIG_SRCDIR([src/cubescene.cc])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11 -Wall no-define nostdinc])
AM_SILENT_RULES([yes])

AC_PROG_CXX
AX_PROG_CXX_FOR_BUILD

AC_ARG_WITH([texture-compression],
  [AS_HELP_STRING([--with-texture-compression=[[dxt|etc]]],
                  [select texture compression codec [dxt]])],
  [SOMATO_TEXTURE_COMPRESSION=$withval],
  [SOMATO_TEXTURE_COMPRESSION=dxt])

AC_MSG_CHECKING([how to compress textures])
AS_CASE([$SOMATO_TEXTURE_COMPRESSION], [dxt|etc],,
        [AC_MSG_FAILURE([[invalid codec "$SOMATO_TEXTURE_COMPRESSION"]])])
AC_MSG_RESULT([$SOMATO_TEXTURE_COMPRESSION])
AC_SUBST([SOMATO_TEXTURE_COMPRESSION])

DK_PKG_CHECK_BUILD_MODULES([MESHDATA_MODULES], [glib-2.0 assimp >= 3.0])
PKG_CHECK_MODULES([SOMATO_MODULES], [gthread-2.0 epoxy >= 1.3 gtkmm-3.0 >= 3.22])

DK_PKG_CONFIG_SUBST([GLIB_COMPILE_RESOURCES],
                    [--variable=glib_compile_resources gio-2.0],,
                    [AC_MSG_FAILURE([[GLib resource compiler not found.]])])

DK_PKG_PATH_PROG([GTK_UPDATE_ICON_CACHE], [gtk+-3.0], [gtk-update-icon-cache])

AC_DEFINE([RESOURCE_PREFIX], ["/org/gtk/somato/"],
          [Base path for loading resources.])

AC_LANG([C++])
DK_PROG_CXX_STD([SOMATO_EXTRA_CXXFLAGS], [14], [17])

AC_MSG_CHECKING([host byte order])
DK_SH_VAR_PUSH([CPPFLAGS], ["$CPPFLAGS $SOMATO_MODULES_CFLAGS"])
AC_COMPUTE_INT([somato_little_endian],
               [G_BYTE_ORDER == G_LITTLE_ENDIAN], [#include <glib.h>],
               [AC_MSG_FAILURE([[Failed to detect host byte order]])])
DK_SH_VAR_POP([CPPFLAGS])
AS_CASE([$somato_little_endian],
        [0], [SOMATO_BYTE_ORDER=--be somato_endianness=big],
        [1], [SOMATO_BYTE_ORDER=--le somato_endianness=little],
             [SOMATO_BYTE_ORDER= somato_endianness=unknown])
AC_SUBST([SOMATO_BYTE_ORDER])
AC_MSG_RESULT([$somato_endianness endian])

DK_CHECK_VISIBILITY_SUPPORT([SOMATO_EXTRA_CXXFLAGS], [SOMATO_EXPORT])

DK_CHECK_COMPILE_FLAGS([SOMATO_EXTRA_CXXFLAGS], [math without errno],
                       [-fno-math-errno])
DK_CHECK_COMPILE_FLAGS([SOMATO_EXTRA_CXXFLAGS], [link-time optimization],
                       [-flto])

DK_SH_VAR_PUSH([CPPFLAGS], ["$SOMATO_MODULES_CFLAGS $CPPFLAGS"])

# Before running any of the other tests, check whether it is necessary
# to explicitely link -lm on this platform.
AC_SEARCH_LIBS([atan2], [m])

SOMATO_ARG_ENABLE_VECTOR_SIMD
DK_ARG_ENABLE_WARNINGS([SOMATO_WARNING_FLAGS], [-Wall], [-Wall -Wextra])

DK_SH_VAR_POP([CPPFLAGS])

DK_SH_VAR_PUSH([CXX], [$CXX_FOR_BUILD])
DK_SH_VAR_PUSH([CXXFLAGS], [$CXXFLAGS_FOR_BUILD])
DK_SH_VAR_PUSH([LDFLAGS], [$LDFLAGS_FOR_BUILD])

DK_PROG_CXX_STD([TOOL_EXTRA_CXXFLAGS], [14], [17])
DK_CHECK_COMPILE_FLAGS([TOOL_EXTRA_CXXFLAGS], [math without errno (build)],
                       [-fno-math-errno])
DK_CHECK_COMPILE_FLAGS([TOOL_EXTRA_CXXFLAGS], [link-time optimization (build)],
                       [-flto])
DK_ARG_ENABLE_WARNINGS([TOOL_WARNING_FLAGS], [-Wall], [-Wall -Wextra])

DK_SH_VAR_POP([LDFLAGS])
DK_SH_VAR_POP([CXXFLAGS])
DK_SH_VAR_POP([CXX])

AC_CONFIG_FILES([Makefile src/tool/Makefile ui/somato.gresource.xml])
AC_OUTPUT
