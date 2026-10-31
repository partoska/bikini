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

#ifndef BIKINI_H
#define BIKINI_H

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Includes
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "bktypes.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define BK_VERSION "2.0.1"
#define BK_ERROR_MAX (256)
#define BK_READ_ERROR (~(BKSize)0)

#ifdef __cplusplus
extern "C"
{
#endif

  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   * Types
   * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

  /** Result of an operation: BK_OK, or a negative error code. */
  typedef enum BKStatus
  {
    BK_OK = 0, /**< Success. */

    BK_EARG = -1,      /**< Invalid argument, e.g. an unsavable key. */
    BK_EMEM = -2,      /**< Out of memory. */
    BK_EIO = -3,       /**< File or stream failure; BKError.code is errno. */
    BK_ESYNTAX = -4,   /**< Malformed line; BKError.line tells which. */
    BK_EDUPLICATE = -5 /**< Repeated section or key; BKError.line too. */
  } BKStatus;

  /** Options of a document, combined with | and given to bkInit(). */
  typedef enum BKFlag
  {
    BK_FNONE = 0, /**< Default behavior. */

    /** Match section names and keys case-insensitively (ASCII), in lookups
     * and duplicate checks alike. Names are kept as written. */
    BK_FICASE = 1 << 0,

    /** Allow sections repeated within a text and keys repeated within a
     * section, instead of BK_EDUPLICATE. Each repeat of a section stays a
     * section of its own; repeats are told apart by their index among
     * those of the same name, in document order. Functions without an
     * index address the first, index 0. */
    BK_FMULTI = 1 << 1
  } BKFlag;

  /** Details of the most recent operation on a document; see bkError(). */
  typedef struct BKError
  {
    BKStatus status;              /**< BK_OK when the last operation worked. */
    BKSize line;                  /**< Offending line, or 0. */
    BKInt code;                   /**< System error code (errno), or 0. */
    BKChar message[BK_ERROR_MAX]; /**< Readable, e.g. "line 3: duplicate key
                                       'mode' in [selection]". */
  } BKError;

  /**
   * Receives serialized INI text from bkWrite().
   *
   * @param user Pointer given to bkWrite().
   * @param data Text to write; not NUL-terminated.
   * @param size Number of bytes.
   * @return Number of bytes written; anything but size is a failure.
   */
  typedef BKSize (*BKWriteFn) (BKVoid *user, const BKChar *data, BKSize size);

  /**
   * Supplies INI text to bkRead().
   *
   * @param user Pointer given to bkRead().
   * @param data Buffer to fill; not NUL-terminated.
   * @param size Capacity of the buffer in bytes.
   * @return Number of bytes stored, at most size; 0 at the end of the input,
   *         or BK_READ_ERROR on failure.
   */
  typedef BKSize (*BKReadFn) (BKVoid *user, BKChar *data, BKSize size);

  /** Opaque INI document: its sections in order, and the last error. */
  typedef struct BKIni BKIni;

  /** Opaque section of a document: its name and keys in order; owned by
   * the document. */
  typedef struct BKSection BKSection;

  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   * Declarations - Document
   * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

  /**
   * Allocates an empty document.
   *
   * @param flags Combination of BKFlag values.
   * @return New document, or NULL when out of memory or for unknown flags.
   */
  BKIni *bkInit (BKDword flags);

  /**
   * Releases a document and every string it owns. Accepts NULL.
   *
   * @param ini Document to destroy.
   */
  void bkDestroy (BKIni *ini);

  /**
   * Gets the details of the most recent operation on a document.
   *
   * @param ini Document to inspect.
   * @return Error record owned by the document, NULL for NULL; status is
   *         BK_OK when the last operation succeeded.
   */
  const BKError *bkError (const BKIni *ini);

  /**
   * Describes a status code.
   *
   * @param status Status to describe.
   * @return Static, human-readable text.
   */
  const BKChar *bkStatusText (BKStatus status);

  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   * Declarations - Reading and writing
   * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

  /**
   * Parses INI text into a document. Sections and keys are added to those
   * already present; a repeat is BK_EDUPLICATE, unless BK_FMULTI makes it
   * one more section or value. On failure the document
   * holds whatever was parsed before the offending line.
   *
   * @param ini  Document to populate.
   * @param text NUL-terminated INI text; a leading UTF-8 BOM is skipped.
   * @return BK_OK on success, or a BK_E* error code.
   */
  BKStatus bkParse (BKIni *ini, const BKChar *text);

  /**
   * Reads and parses an INI file, as bkParse() does. A NUL byte in the
   * file is BK_ESYNTAX; it and read failures leave the document unchanged.
   *
   * @param ini  Document to populate.
   * @param path File to read.
   * @return BK_OK on success, or a BK_E* error code.
   */
  BKStatus bkLoad (BKIni *ini, const BKChar *path);

  /**
   * Reads INI text from a stream to its end and parses it, as bkParse()
   * does; the counterpart of bkWrite() for standard input, sockets or
   * memory. A NUL byte is BK_ESYNTAX; it and read failures leave the
   * document unchanged.
   *
   * @param ini  Document to populate.
   * @param read Supplies the text, in pieces, until it returns 0.
   * @param user Passed to read.
   * @return BK_OK on success, or a BK_E* error code; BK_EIO when read
   *         fails or returns more than asked.
   */
  BKStatus bkRead (BKIni *ini, BKReadFn read, BKVoid *user);

  /**
   * Serializes a document as INI text, section by section in document
   * order, keys and values separated as bkSetDelimiter() chose. Multi-line
   * values become tab-indented continuation lines.
   *
   * @param ini   Document to serialize.
   * @param write Receives the text, in pieces.
   * @param user  Passed to write.
   * @return BK_OK on success, or BK_EIO when write fails.
   */
  BKStatus bkWrite (BKIni *ini, BKWriteFn write, BKVoid *user);

  /**
   * Chooses what bkWrite() and bkSave() put between key and value.
   *
   * @param ini       Document to modify.
   * @param delimiter Optional blanks, = or :, and optional blanks, so that
   *                  the text reads back: e.g. "=", ": " or " = ", the
   *                  default.
   * @return BK_OK on success, or a BK_E* error code.
   */
  BKStatus bkSetDelimiter (BKIni *ini, const BKChar *delimiter);

  /**
   * Writes a document to a file, replacing its contents. Callers that need
   * an atomic replacement write a temporary file with bkWrite() and rename
   * it.
   *
   * @param ini  Document to serialize.
   * @param path Destination file.
   * @return BK_OK on success, or a BK_E* error code.
   */
  BKStatus bkSave (BKIni *ini, const BKChar *path);

  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   * Declarations - Values
   * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

  /**
   * Looks up a value; with repeats, the first value of the first section of
   * that name.
   *
   * @param ini     Document to search.
   * @param section Section name.
   * @param key     Key name.
   * @return The value, or NULL when the section or key is missing.
   */
  const BKChar *bkGet (const BKIni *ini, const BKChar *section,
                       const BKChar *key);

  /**
   * Looks up a value by position among repeats.
   *
   * @param ini     Document to search.
   * @param section Section name.
   * @param sindex  Index among the sections of that name.
   * @param key     Key name.
   * @param kindex  Index among the values of that key in the section.
   * @return The value, or NULL when the section or key is missing.
   */
  const BKChar *bkGetNth (const BKIni *ini, const BKChar *section,
                          BKSize sindex, const BKChar *key, BKSize kindex);

  /**
   * Sets a value, adding the section and key when missing; with repeats,
   * the first value of the first section of that name. Keys must be
   * non-empty, without surrounding blanks, line breaks, = or :, and must not
   * start with [, ; or #. Values must not contain carriage returns.
   *
   * @param ini     Document to modify.
   * @param section Section name.
   * @param key     Key name.
   * @param value   Value to store; the document keeps a copy.
   * @return BK_OK on success, or a BK_E* error code.
   */
  BKStatus bkSet (BKIni *ini, const BKChar *section, const BKChar *key,
                  const BKChar *value);

  /**
   * Replaces an existing value, addressed by position among repeats; names
   * and values as for bkSet().
   *
   * @param ini     Document to modify.
   * @param section Section name.
   * @param sindex  Index among the sections of that name.
   * @param key     Key name.
   * @param kindex  Index among the values of that key in the section.
   * @param value   Value to store; the document keeps a copy.
   * @return BK_OK on success, or a BK_E* error code; BK_EARG when the
   *         section or value is missing.
   */
  BKStatus bkSetNth (BKIni *ini, const BKChar *section, BKSize sindex,
                     const BKChar *key, BKSize kindex, const BKChar *value);

  /**
   * Adds a value after the last value of the key in a section, or at the
   * end of the section for a new key; names and values as for bkSet(). An
   * index one past the last section of that name adds a section. Repeats
   * need BK_FMULTI.
   *
   * @param ini     Document to modify.
   * @param section Section name.
   * @param sindex  Index among the sections of that name.
   * @param key     Key name.
   * @param value   Value to store; the document keeps a copy.
   * @return BK_OK on success, or a BK_E* error code; BK_EARG for a missing
   *         section or a repeat without BK_FMULTI.
   */
  BKStatus bkAdd (BKIni *ini, const BKChar *section, BKSize sindex,
                  const BKChar *key, const BKChar *value);

  /**
   * Removes a key and its value, the first one with repeats; the section
   * stays, even when empty.
   *
   * @param ini     Document to modify.
   * @param section Section name.
   * @param key     Key name.
   * @return BK_TRUE when the key was removed, BK_FALSE when missing.
   */
  BKBool bkRemove (BKIni *ini, const BKChar *section, const BKChar *key);

  /**
   * Removes a value, addressed by position among repeats; later values of
   * the key move down one index.
   *
   * @param ini     Document to modify.
   * @param section Section name.
   * @param sindex  Index among the sections of that name.
   * @param key     Key name.
   * @param kindex  Index among the values of that key in the section.
   * @return BK_TRUE when the value was removed, BK_FALSE when missing.
   */
  BKBool bkRemoveNth (BKIni *ini, const BKChar *section, BKSize sindex,
                      const BKChar *key, BKSize kindex);

  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
   * Declarations - Sections
   * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

  /**
   * Adds an empty section unless one of that name already exists.
   *
   * @param ini  Document to modify.
   * @param name Section name; non-empty, without line breaks or ].
   * @return BK_OK on success, or a BK_E* error code.
   */
  BKStatus bkSectionSet (BKIni *ini, const BKChar *name);

  /**
   * Adds an empty section at the end of the document, as one more repeat
   * when one of that name exists, which needs BK_FMULTI.
   *
   * @param ini  Document to modify.
   * @param name Section name; non-empty, without line breaks or ].
   * @return BK_OK on success, or a BK_E* error code; BK_EARG for a repeat
   *         without BK_FMULTI.
   */
  BKStatus bkSectionAdd (BKIni *ini, const BKChar *name);

  /**
   * Removes a section with all its keys, the first one with repeats. Later
   * sections move up one position.
   *
   * @param ini  Document to modify.
   * @param name Section name.
   * @return BK_TRUE when the section was removed, BK_FALSE when missing.
   */
  BKBool bkSectionRemove (BKIni *ini, const BKChar *name);

  /**
   * Removes a section, addressed by position among repeats; later repeats
   * move down one index.
   *
   * @param ini   Document to modify.
   * @param name  Section name.
   * @param index Index among the sections of that name.
   * @return BK_TRUE when the section was removed, BK_FALSE when missing.
   */
  BKBool bkSectionRemoveNth (BKIni *ini, const BKChar *name, BKSize index);

  /**
   * Gets the number of sections.
   *
   * @param ini Document to inspect.
   * @return Section count, 0 for NULL.
   */
  BKSize bkSectionCount (const BKIni *ini);

  /**
   * Gets a section by position, in document order, repeats included.
   *
   * @param ini   Document to inspect.
   * @param index Position below bkSectionCount().
   * @return The section, or NULL when out of range.
   */
  const BKSection *bkSectionAt (const BKIni *ini, BKSize index);

  /**
   * Finds a section by name, the first one with repeats.
   *
   * @param ini  Document to search.
   * @param name Section name.
   * @return The section, or NULL when missing. Valid until the section is
   *         removed or the document destroyed; removing a repeat makes the
   *         next one the first.
   */
  const BKSection *bkSectionFind (const BKIni *ini, const BKChar *name);

  /**
   * Finds a section by name and position among repeats.
   *
   * @param ini   Document to search.
   * @param name  Section name.
   * @param index Index among the sections of that name.
   * @return The section, or NULL when missing; valid as for
   *         bkSectionFind().
   */
  const BKSection *bkSectionFindNth (const BKIni *ini, const BKChar *name,
                                     BKSize index);

  /**
   * Counts the sections of a name.
   *
   * @param ini  Document to inspect.
   * @param name Section name.
   * @return Number of sections of that name: 0 or 1 without BK_FMULTI.
   */
  BKSize bkSectionCountOf (const BKIni *ini, const BKChar *name);

  /**
   * Gets the name of a section.
   *
   * @param section Section to inspect.
   * @return Section name, NULL for NULL.
   */
  const BKChar *bkSectionName (const BKSection *section);

  /**
   * Gets the number of keys in a section, repeats included.
   *
   * @param section Section to inspect.
   * @return Key count, 0 for NULL.
   */
  BKSize bkKeyCount (const BKSection *section);

  /**
   * Gets a key by position, in document order, repeats included.
   *
   * @param section Section to inspect.
   * @param index   Position below bkKeyCount().
   * @return The key, or NULL when out of range.
   */
  const BKChar *bkKeyAt (const BKSection *section, BKSize index);

  /**
   * Gets a value by position, in document order.
   *
   * @param section Section to inspect.
   * @param index   Position below bkKeyCount().
   * @return The value, or NULL when out of range.
   */
  const BKChar *bkValueAt (const BKSection *section, BKSize index);

  /**
   * Looks up a value within a section, the first one with repeats.
   *
   * @param section Section to search; may be NULL.
   * @param key     Key name.
   * @return The value, or NULL when missing.
   */
  const BKChar *bkSectionGet (const BKSection *section, const BKChar *key);

  /**
   * Looks up a value within a section by position among repeats.
   *
   * @param section Section to search; may be NULL.
   * @param key     Key name.
   * @param index   Index among the values of that key.
   * @return The value, or NULL when missing.
   */
  const BKChar *bkSectionGetNth (const BKSection *section, const BKChar *key,
                                 BKSize index);

  /**
   * Counts the values of a key within a section.
   *
   * @param section Section to inspect; may be NULL.
   * @param key     Key name.
   * @return Number of values: 0 or 1 without BK_FMULTI.
   */
  BKSize bkKeyCountOf (const BKSection *section, const BKChar *key);

#ifdef __cplusplus
}
#endif

#endif
