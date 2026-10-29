/*
 * Bikini - Small, minimal INI library and query tool.
 * Copyright (C) 2026 Fabrika Charvat s.r.o. All rights reserved.
 * Developed by Partoska Laboratory team, <https://lab.partoska.com>
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * You can contact the author(s) via email at ask <at> partoska.com.
 */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "bikini.h"
#include "plat.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Strings
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

BKSize
bkcStrLen (const BKChar *str)
{
  return strlen (str);
}

BKBool
bkcStrEq (const BKChar *a, const BKChar *b)
{
  return strcmp (a, b) == 0;
}

BKChar *
bkcStrFind (const BKChar *str, BKChar c)
{
  return strchr (str, c);
}

BKChar *
bkcStrFindLast (const BKChar *str, BKChar c)
{
  return strrchr (str, c);
}

BKChar *
bkcStrDup (const BKChar *str, BKSize length)
{
  BKChar *copy = malloc (length + 1);
  if (copy)
    {
      memcpy (copy, str, length);
      copy[length] = '\0';
    }

  return copy;
}

BKVoid *
bkcMemAlloc (BKSize size)
{
  return malloc (size ? size : 1);
}

void
bkcMemFree (BKVoid *ptr)
{
  free (ptr);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Streams
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
bkcStreamInit (void)
{
#ifdef _WIN32
  // Text mode would turn LF into CRLF and stop reading at Ctrl-Z: binary
  // standard streams carry the same bytes as bkSave() and other platforms.
  _setmode (_fileno (stdin), _O_BINARY);
  _setmode (_fileno (stdout), _O_BINARY);
#endif
}

BKSize
bkcReadIn (BKVoid *user, BKChar *data, BKSize size)
{
  BKSize n = fread (data, 1, size, stdin);
  if (n == 0 && ferror (stdin))
    {
      *(BKInt *)user = errno;
      return BK_READ_ERROR;
    }

  return n;
}

BKSize
bkcWriteOut (BKVoid *user, const BKChar *data, BKSize size)
{
  BK_UNUSED (user);

  return fwrite (data, 1, size, stdout);
}

BKBool
bkcPrint (const BKChar *str)
{
  return fputs (str, stdout) >= 0;
}

BKBool
bkcFlush (void)
{
  return fflush (stdout) == 0 && !ferror (stdout);
}

void
bkcError (const BKChar *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  vfprintf (stderr, fmt, args);
  va_end (args);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - System errors
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

BKInt
bkcSysError (void)
{
  return errno;
}

const BKChar *
bkcSysErrorText (BKInt code)
{
  return strerror (code);
}
