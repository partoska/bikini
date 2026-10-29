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

#ifndef BK_PLAT_H
#define BK_PLAT_H

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "bktypes.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Types
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/** Opaque open file, from bkFileOpen(); released by bkFileClose(). */
typedef struct BKFile BKFile;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Memory
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * Allocates zeroed memory; release it with bkMemFree().
 *
 * @param size Number of bytes; 0 still gives a valid, unique block.
 * @return New block, or NULL when out of memory.
 */
BKVoid *bkMemZalloc (BKSize size);

/**
 * Resizes a block, keeping its contents up to the smaller size; bytes
 * beyond the old size are uninitialized.
 *
 * @param ptr  Block from bkMemZalloc() or bkMemRealloc(), or NULL for a new
 *             one.
 * @param size New size in bytes; 0 still gives a valid, unique block.
 * @return Resized block, possibly moved, or NULL when out of memory; ptr
 *         then stays valid and unchanged.
 */
BKVoid *bkMemRealloc (BKVoid *ptr, BKSize size);

/**
 * Releases memory from bkMemZalloc(), bkMemRealloc() or bkStrDup().
 * Accepts NULL.
 *
 * @param ptr Block to release.
 */
void bkMemFree (BKVoid *ptr);

/**
 * Copies bytes between blocks that do not overlap.
 *
 * @param dst  Destination, at least size bytes.
 * @param src  Source, at least size bytes.
 * @param size Number of bytes.
 */
void bkMemCopy (BKVoid *dst, const BKVoid *src, BKSize size);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Strings
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * Measures a string.
 *
 * @param str NUL-terminated string.
 * @return Number of bytes before the terminating NUL.
 */
BKSize bkStrLen (const BKChar *str);

/**
 * Compares two strings, byte by byte or ASCII case-insensitively; locale
 * settings play no part.
 *
 * @param a     NUL-terminated string.
 * @param b     NUL-terminated string.
 * @param icase BK_TRUE to treat A to Z as a to z.
 * @return Negative, zero or positive as a sorts before, equal to or after
 *         b.
 */
BKInt bkStrCmp (const BKChar *a, const BKChar *b, BKBool icase);

/**
 * Compares at most the first n bytes of two strings, case-sensitively.
 *
 * @param a String; stops at its NUL if shorter than n.
 * @param b String; stops at its NUL if shorter than n.
 * @param n Maximum number of bytes to compare.
 * @return Negative, zero or positive as a sorts before, equal to or after
 *         b.
 */
BKInt bkStrCmpN (const BKChar *a, const BKChar *b, BKSize n);

/**
 * Finds the first occurrence of a character.
 *
 * @param str NUL-terminated string to search.
 * @param c   Character to find; '\0' finds the terminator.
 * @return Pointer into str at the first c, or NULL when there is none.
 */
BKChar *bkStrFind (const BKChar *str, BKChar c);

/**
 * Finds the last occurrence of a character.
 *
 * @param str NUL-terminated string to search.
 * @param c   Character to find; '\0' finds the terminator.
 * @return Pointer into str at the last c, or NULL when there is none.
 */
BKChar *bkStrFindLast (const BKChar *str, BKChar c);

/**
 * Finds the first occurrence of any character from a set.
 *
 * @param str NUL-terminated string to search.
 * @param set NUL-terminated characters to find.
 * @return Pointer into str at the first match, or NULL when there is none.
 */
BKChar *bkStrFindAny (const BKChar *str, const BKChar *set);

/**
 * Copies the start of a string into a new one; release it with
 * bkMemFree().
 *
 * @param str    Bytes to copy; need not be NUL-terminated.
 * @param length Number of bytes to copy.
 * @return New NUL-terminated string of length bytes, or NULL when out of
 *         memory.
 */
BKChar *bkStrDup (const BKChar *str, BKSize length);

/**
 * Formats into a buffer like snprintf(), truncating to fit.
 *
 * @param dst  Buffer; always NUL-terminated when size is not 0.
 * @param size Capacity of dst in bytes.
 * @param fmt  printf()-style format.
 * @param ...  Arguments of fmt.
 * @return Length of the full result without the NUL, which is size or more
 *         when truncated; negative on an encoding error.
 */
BKInt bkStrFmt (BKChar *dst, BKSize size, const BKChar *fmt, ...)
    BK_PRINTF (3, 4);

/**
 * Formats into a buffer like vsnprintf(), as bkStrFmt() does.
 *
 * @param dst  Buffer; always NUL-terminated when size is not 0.
 * @param size Capacity of dst in bytes.
 * @param fmt  printf()-style format.
 * @param args Arguments of fmt.
 * @return Length of the full result without the NUL, which is size or more
 *         when truncated; negative on an encoding error.
 */
BKInt bkStrFmtV (BKChar *dst, BKSize size, const BKChar *fmt, BKVaList args);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - Files
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * Opens a file in binary mode, so bytes pass unchanged on every platform.
 *
 * @param path  File to open.
 * @param write BK_FALSE to read; BK_TRUE to write, creating the file or
 *              truncating it.
 * @return Open file, or NULL on failure, with bkSysError() telling why.
 */
BKFile *bkFileOpen (const BKChar *path, BKBool write);

/**
 * Reads from a file.
 *
 * @param file File opened for reading.
 * @param data Buffer to fill.
 * @param size Capacity of the buffer in bytes.
 * @return Number of bytes stored; less than size at the end of the file or
 *         on failure, which bkFileFailed() tells apart.
 */
BKSize bkFileRead (BKFile *file, BKVoid *data, BKSize size);

/**
 * Writes to a file.
 *
 * @param file File opened for writing.
 * @param data Bytes to write.
 * @param size Number of bytes.
 * @return Number of bytes written; anything but size is a failure.
 */
BKSize bkFileWrite (BKFile *file, const BKVoid *data, BKSize size);

/**
 * Reports whether a read or write on a file failed.
 *
 * @param file Open file.
 * @return BK_TRUE once any read or write on file has failed.
 */
BKBool bkFileFailed (BKFile *file);

/**
 * Closes a file, writing out buffered data; the file is released even when
 * that fails.
 *
 * @param file Open file; not NULL.
 * @return BK_FALSE when buffered data could not be written.
 */
BKBool bkFileClose (BKFile *file);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Declarations - System errors
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * Gets the code of the last system error.
 *
 * @return errno, as left by the last failed system call.
 */
BKInt bkSysError (void);

/**
 * Describes a system error code.
 *
 * @param code System error code, e.g. from bkSysError().
 * @return Human-readable text owned by the C library; valid until the next
 *         call.
 */
const BKChar *bkSysErrorText (BKInt code);

#endif
