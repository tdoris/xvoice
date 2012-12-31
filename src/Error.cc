/*
 * Error.cc
 *
 * Description:  Convenience functions for logging.
 *
 * Copyright (c) 2000, Brian Craft.
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author Brian Craft
 * $Revision: 1.8 $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "sys.h"
#include <cstdio>
#include <cstdarg>
#include <stdarg.h>
#include "Error.h"

using namespace std;

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#define MAXMSG 10*1024
void fstring::vappendf(const char* fmt, va_list ap)
{
  char* buf;
  int n = vsnprintf(buf, 0, fmt, ap);
  if (n < 0) return;
  n = MIN(MAXMSG - size(), (unsigned int)n);
  buf = new char[n+1];
  vsnprintf(buf, n+1, fmt, ap);
  append(buf);
  delete buf;
}

void fstring::appendf(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vappendf(fmt, ap);
  va_end(ap);
}
