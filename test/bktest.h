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

#ifndef BKTEST_H
#define BKTEST_H

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bktypes.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Helpers
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static BKInt bkAssertions = 0;
static BKInt bkFailures = 0;

static inline void
bkAssertImpl (BKBool cond, const BKChar *expr, const BKChar *file, BKInt line)
{
  ++bkAssertions;
  if (!cond)
    {
      fprintf (stderr, "\n    assertion failed: %s (%s:%d)", expr, file, line);
      ++bkFailures;
    }
}

static inline BKBool
bkStrEq (const BKChar *a, const BKChar *b)
{
  return a && b && strcmp (a, b) == 0;
}

#define BK_ASSERT(cond) bkAssertImpl (!!(cond), #cond, __FILE__, __LINE__)
#define BK_ASSERT_EQ(a, b) BK_ASSERT ((a) == (b))
#define BK_ASSERT_STR_EQ(a, b) BK_ASSERT (bkStrEq ((a), (b)))

#define BK_RUN(fn)                                                            \
  do                                                                          \
    {                                                                         \
      BKInt failuresBefore = bkFailures;                                      \
      printf ("  %-48s", #fn);                                                \
      fflush (stdout);                                                        \
      fn ();                                                                  \
      printf ("%s\n", bkFailures == failuresBefore ? "ok" : "FAIL");          \
    }                                                                         \
  while (0)

#define BK_SUITE(name) printf ("\n%s\n", name)

#define BK_SUMMARY()                                                          \
  do                                                                          \
    {                                                                         \
      printf ("\n%d assertions, %d failed\n", bkAssertions, bkFailures);      \
      return bkFailures > 0 ? EXIT_FAILURE : EXIT_SUCCESS;                    \
    }                                                                         \
  while (0)

#endif
