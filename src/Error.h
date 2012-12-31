/**
 * Error.h
 *
 * Description:  Convenience functions for logging.
 *
 * Copyright (c) 2000, Brian Craft.
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author Brian Craft
 * $Revision: 1.11 $
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

#ifndef _ERROR_H
#define _ERROR_H

#include <string>
#include <cstdio>
#include <cstdarg>

class fstring : public std::string            //  error reporting
{
  public:
    void appendf(const char*, ...);
    void vappendf(const char* fmt, va_list ap);
    fstring() {};
    fstring(const char *s) : std::string(s){};
    fstring(size_type n, char c) : std::string(n,c){};
};

enum {
  /*
   * Due to missing files, bad syntax or environment variables etc.  We
   * try to handle these gracefully.
   */
  E_CONFIG,
  /*
   * Probably due to XVoice bugs, e.g. trying to enable the same
   * vocabulary twice. Probably not recoverable.
   */
  E_SEVERE,
  /*
   *  Outside of our control, e.g. memory or voice engine. Not
   *  recoverable.
   */
  E_FATAL };

  /*
   * macro varargs are not portable, but they should be.
   * the alternatives are ugly.
   */
#define LogMessage(sev, fmt, args...) do { \
  fprintf(stderr, "%s: ", __FUNCTION__);\
    fprintf(stderr , fmt , ## args); \
    gMainWin->errorMsg(sev, __FUNCTION__ , fmt , ## args); \
} while (0)

#ifdef DEBUG
#define dbgprintf(x) do { \
  printf("%s: ", __FUNCTION__);\
    printf x ;\
} while (0)
#else
#define dbgprintf(x)
#endif

#endif /* _ERROR_H */
