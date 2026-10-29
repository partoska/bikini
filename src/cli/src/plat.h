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

#ifndef BKC_PLAT_H
#define BKC_PLAT_H

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "bktypes.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Strings
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * Measures a string.
 *
 * @param str NUL-terminated string.
 * @return Number of bytes before the terminating NUL.
 */
BKSize bkcStrLen (const BKChar *str);

/**
 * Compares two strings byte by byte, case-sensitively.
 *
 * @param a NUL-terminated string.
 * @param b NUL-terminated string.
 * @return BK_TRUE when both hold the same bytes.
 */
BKBool bkcStrEq (const BKChar *a, const BKChar *b);

/**
 * Finds the first occurrence of a character.
 *
 * @param str NUL-terminated string to search.
 * @param c   Character to find; '\0' finds the terminator.
 * @return Pointer into str at the first c, or NULL when there is none.
 */
BKChar *bkcStrFind (const BKChar *str, BKChar c);

/**
 * Finds the last occurrence of a character.
 *
 * @param str NUL-terminated string to search.
 * @param c   Character to find; '\0' finds the terminator.
 * @return Pointer into str at the last c, or NULL when there is none.
 */
BKChar *bkcStrFindLast (const BKChar *str, BKChar c);

/**
 * Copies the start of a string into a new one; release it with
 * bkcMemFree().
 *
 * @param str    Bytes to copy; need not be NUL-terminated.
 * @param length Number of bytes to copy.
 * @return New NUL-terminated string of length bytes, or NULL when out of
 *         memory.
 */
BKChar *bkcStrDup (const BKChar *str, BKSize length);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Memory
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * Allocates uninitialized memory; release it with bkcMemFree().
 *
 * @param size Number of bytes; 0 still gives a valid, unique block.
 * @return New block, or NULL when out of memory.
 */
BKVoid *bkcMemAlloc (BKSize size);

/**
 * Releases memory from bkcMemAlloc() or bkcStrDup(). Accepts NULL.
 *
 * @param ptr Block to release.
 */
void bkcMemFree (BKVoid *ptr);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Streams
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * Prepares the standard streams; call once, before any other stream
 * function. On Windows it switches standard input and output to binary
 * mode, so they carry the same bytes as bkSave() and other platforms.
 */
void bkcStreamInit (void);

/**
 * Reads standard input; a BKReadFn for bkRead().
 *
 * @param user Pointer to a BKInt that receives the system error code
 *             (errno) of a failed read; untouched otherwise.
 * @param data Buffer to fill; not NUL-terminated.
 * @param size Capacity of the buffer in bytes.
 * @return Number of bytes stored, at most size; 0 at the end of the input,
 *         or BK_READ_ERROR on failure.
 */
BKSize bkcReadIn (BKVoid *user, BKChar *data, BKSize size);

/**
 * Writes to standard output; a BKWriteFn for bkWrite().
 *
 * @param user Ignored.
 * @param data Text to write; not NUL-terminated.
 * @param size Number of bytes.
 * @return Number of bytes written; anything but size is a failure.
 */
BKSize bkcWriteOut (BKVoid *user, const BKChar *data, BKSize size);

/**
 * Writes a string to standard output, as is; no newline is added.
 *
 * @param str NUL-terminated string.
 * @return BK_FALSE when the write failed.
 */
BKBool bkcPrint (const BKChar *str);

/**
 * Flushes standard output; call before exiting to catch late write
 * failures, e.g. a full disk or a closed pipe.
 *
 * @return BK_FALSE when buffered data could not be written, or an earlier
 *         write to standard output failed.
 */
BKBool bkcFlush (void);

/**
 * Formats a message to standard error; no newline is added.
 *
 * @param fmt printf()-style format.
 * @param ... Arguments of fmt.
 */
void bkcError (const BKChar *fmt, ...) BK_PRINTF (1, 2);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - System errors
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * Gets the code of the last system error.
 *
 * @return errno, as left by the last failed system call.
 */
BKInt bkcSysError (void);

/**
 * Describes a system error code.
 *
 * @param code System error code, e.g. from bkcSysError() or BKError.code.
 * @return Human-readable text owned by the C library; valid until the next
 *         call.
 */
const BKChar *bkcSysErrorText (BKInt code);

#endif
