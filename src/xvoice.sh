#!/bin/sh

# required to force xvoice to ignore glibc library clashes
export LD_ASSUME_KERNEL=2.4.1

# source the viavoice runtime environment
. vvsetenv

# if -g, run in gdb (developer option)
if test "$1" = "-g" ; then
  shift
  gdb="gdb "
  if test -f xvoice ; then
	gdb="$gdb ./"
  fi
  flags=

# if -l, run xvoice from the local directory (developer option)
elif test "$1" = "-l" ; then
  shift
  gdb="./"
  flags=--disable-crash-dialog

# disable the crash dialog, since it doesn't work
else
  gdb=
  flags=--disable-crash-dialog
fi

exec ${gdb}xvoice.bin $flags $*

