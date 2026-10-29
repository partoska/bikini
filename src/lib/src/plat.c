/*
 * Bikini Library - Small, minimal INI reader and writer.
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

#include "plat.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Memory
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

BKVoid *
bkMemZalloc (BKSize size)
{
  return calloc (1, size ? size : 1);
}

BKVoid *
bkMemRealloc (BKVoid *ptr, BKSize size)
{
  return realloc (ptr, size ? size : 1);
}

void
bkMemFree (BKVoid *ptr)
{
  free (ptr);
}

void
bkMemCopy (BKVoid *dst, const BKVoid *src, BKSize size)
{
  memcpy (dst, src, size);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Strings
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static BKInt
bkLower (BKChar c)
{
  return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : (BKByte)c;
}

BKSize
bkStrLen (const BKChar *str)
{
  return strlen (str);
}

BKInt
bkStrCmp (const BKChar *a, const BKChar *b, BKBool icase)
{
  if (!icase)
    {
      return strcmp (a, b);
    }

  // Portable ASCII folding: no strcasecmp() on Windows, no locale surprises.
  while (*a && bkLower (*a) == bkLower (*b))
    {
      ++a;
      ++b;
    }

  return bkLower (*a) - bkLower (*b);
}

BKInt
bkStrCmpN (const BKChar *a, const BKChar *b, BKSize n)
{
  return strncmp (a, b, n);
}

BKChar *
bkStrFind (const BKChar *str, BKChar c)
{
  return strchr (str, c);
}

BKChar *
bkStrFindLast (const BKChar *str, BKChar c)
{
  return strrchr (str, c);
}

BKChar *
bkStrFindAny (const BKChar *str, const BKChar *set)
{
  return strpbrk (str, set);
}

BKChar *
bkStrDup (const BKChar *str, BKSize length)
{
  BKChar *copy = malloc (length + 1);
  if (copy)
    {
      memcpy (copy, str, length);
      copy[length] = '\0';
    }

  return copy;
}

BKInt
bkStrFmt (BKChar *dst, BKSize size, const BKChar *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  BKInt n = vsnprintf (dst, size, fmt, args);
  va_end (args);

  return n;
}

BKInt
bkStrFmtV (BKChar *dst, BKSize size, const BKChar *fmt, BKVaList args)
{
  return vsnprintf (dst, size, fmt, args);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Files
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

BKFile *
bkFileOpen (const BKChar *path, BKBool write)
{
  return (BKFile *)fopen (path, write ? "wb" : "rb");
}

BKSize
bkFileRead (BKFile *file, BKVoid *data, BKSize size)
{
  return fread (data, 1, size, (FILE *)file);
}

BKSize
bkFileWrite (BKFile *file, const BKVoid *data, BKSize size)
{
  return fwrite (data, 1, size, (FILE *)file);
}

BKBool
bkFileFailed (BKFile *file)
{
  return ferror ((FILE *)file) != 0;
}

BKBool
bkFileClose (BKFile *file)
{
  return fclose ((FILE *)file) == 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - System errors
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

BKInt
bkSysError (void)
{
  return errno;
}

const BKChar *
bkSysErrorText (BKInt code)
{
  return strerror (code);
}
