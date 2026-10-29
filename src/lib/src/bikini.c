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

#include "bikini.h"
#include "plat.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Macros
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define BK_READ_CHUNK (4096)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Types - Private
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef struct BKEntry
{
  BKChar *key;
  BKChar *value;
} BKEntry;

struct BKSection
{
  BKChar *name;
  BKEntry *entries;
  BKSize count;
  BKSize capacity;
  BKBool icase;
};

struct BKIni
{
  BKSection **sections;
  BKSize count;
  BKSize capacity;
  BKDword flags;
  BKError error;
};

/* Reader of bkLoad(), keeping errno of a failed read. */
typedef struct BKFileReader
{
  BKFile *file;
  BKInt code;
} BKFileReader;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Errors
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void
bkSucceed (BKIni *ini)
{
  ini->error.status = BK_OK;
  ini->error.line = 0;
  ini->error.code = 0;
  ini->error.message[0] = '\0';
}

/* Records a failure; a line number prefixes the message. */
static BKStatus bkFail (BKIni *ini, BKStatus status, BKSize line,
                        const BKChar *fmt, ...) BK_PRINTF (4, 5);

static BKStatus
bkFail (BKIni *ini, BKStatus status, BKSize line, const BKChar *fmt, ...)
{
  BKChar text[BK_ERROR_MAX];
  BKVaList args;
  va_start (args, fmt);
  bkStrFmtV (text, sizeof (text), fmt, args);
  va_end (args);

  BKError *error = &ini->error;
  error->status = status;
  error->line = line;
  error->code = 0;
  if (line > 0)
    {
      // %zu is unknown to the msvcrt of MinGW, so the line goes as %lu
      bkStrFmt (error->message, sizeof (error->message), "line %lu: %s",
                (BKULong)line, text);
    }
  else
    {
      bkStrFmt (error->message, sizeof (error->message), "%s", text);
    }

  return status;
}

/* Records a failed system call on a path, keeping errno. */
static BKStatus
bkFailSys (BKIni *ini, const BKChar *path, BKInt code)
{
  bkFail (ini, BK_EIO, 0, "%s: %s", path, bkSysErrorText (code));
  ini->error.code = code;

  return BK_EIO;
}

static BKStatus
bkFailMem (BKIni *ini)
{
  return bkFail (ini, BK_EMEM, 0, "out of memory");
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Private
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static BKBool
bkIsBlank (BKChar c)
{
  return c == ' ' || c == '\t' || c == '\f' || c == '\v' || c == '\r';
}

/* Trims blanks on both ends in place and returns the new start. */
static BKChar *
bkTrim (BKChar *str)
{
  while (bkIsBlank (*str))
    {
      ++str;
    }

  BKChar *end = str + bkStrLen (str);
  while (end > str && bkIsBlank (end[-1]))
    {
      --end;
    }
  *end = '\0';

  return str;
}

/* Makes room for at least needed items, doubling the capacity. */
static BKBool
bkGrow (BKVoid **items, BKSize *capacity, BKSize needed, BKSize size)
{
  if (needed <= *capacity)
    {
      return BK_TRUE;
    }

  BKSize maximum = (BKSize)-1 / size;
  BKSize next = *capacity ? *capacity : 8;
  while (next < needed)
    {
      if (next > maximum / 2)
        {
          return BK_FALSE;
        }
      next *= 2;
    }
  if (next > maximum)
    {
      return BK_FALSE;
    }

  BKVoid *grown = bkMemRealloc (*items, next * size);
  if (!grown)
    {
      return BK_FALSE;
    }

  *items = grown;
  *capacity = next;

  return BK_TRUE;
}

static BKSection *
bkFind (const BKIni *ini, const BKChar *name)
{
  BKBool icase = (ini->flags & BK_FICASE) != 0;
  for (BKSize i = 0; i < ini->count; ++i)
    {
      if (bkStrCmp (ini->sections[i]->name, name, icase) == 0)
        {
          return ini->sections[i];
        }
    }

  return NULL;
}

static BKEntry *
bkFindEntry (const BKSection *section, const BKChar *key)
{
  for (BKSize i = 0; i < section->count; ++i)
    {
      if (bkStrCmp (section->entries[i].key, key, section->icase) == 0)
        {
          return &section->entries[i];
        }
    }

  return NULL;
}

static BKSection *
bkSectionNew (BKIni *ini, const BKChar *name)
{
  if (!bkGrow ((BKVoid **)&ini->sections, &ini->capacity, ini->count + 1,
               sizeof (BKSection *)))
    {
      return NULL;
    }

  BKSection *section = bkMemZalloc (sizeof (BKSection));
  if (!section)
    {
      return NULL;
    }

  section->name = bkStrDup (name, bkStrLen (name));
  section->icase = (ini->flags & BK_FICASE) != 0;
  if (!section->name)
    {
      bkMemFree (section);
      return NULL;
    }

  ini->sections[ini->count++] = section;

  return section;
}

static BKBool
bkEntryNew (BKSection *section, const BKChar *key, const BKChar *value)
{
  if (!bkGrow ((BKVoid **)&section->entries, &section->capacity,
               section->count + 1, sizeof (BKEntry)))
    {
      return BK_FALSE;
    }

  BKEntry entry;
  entry.key = bkStrDup (key, bkStrLen (key));
  entry.value = bkStrDup (value, bkStrLen (value));
  if (!entry.key || !entry.value)
    {
      bkMemFree (entry.key);
      bkMemFree (entry.value);
      return BK_FALSE;
    }

  section->entries[section->count++] = entry;

  return BK_TRUE;
}

static void
bkFreeSection (BKSection *section)
{
  for (BKSize i = 0; i < section->count; ++i)
    {
      bkMemFree (section->entries[i].key);
      bkMemFree (section->entries[i].value);
    }
  bkMemFree (section->entries);
  bkMemFree (section->name);
  bkMemFree (section);
}

/* Appends a continuation line to a value, joined by a newline. */
static BKBool
bkAppend (BKEntry *entry, const BKChar *more)
{
  BKSize old = bkStrLen (entry->value);
  BKSize add = bkStrLen (more);
  BKChar *joined = bkMemRealloc (entry->value, old + add + 2);
  if (!joined)
    {
      return BK_FALSE;
    }

  joined[old] = '\n';
  bkMemCopy (joined + old + 1, more, add + 1);
  entry->value = joined;

  return BK_TRUE;
}

static BKBool
bkValidSection (const BKChar *name)
{
  return name && *name && !bkStrFindAny (name, "]\r\n");
}

static BKBool
bkValidKey (const BKChar *key)
{
  if (!key || !*key || bkStrFindAny (key, "=:\r\n") || bkStrFind ("[;#", *key))
    {
      return BK_FALSE;
    }

  return !bkIsBlank (key[0]) && !bkIsBlank (key[bkStrLen (key) - 1]);
}

/* Parses a [section] header line. */
static BKStatus
bkParseHeader (BKIni *ini, BKChar *line, BKSize number, BKSection **section)
{
  BKChar *end = bkStrFindLast (line, ']');
  if (!end || end[1] != '\0')
    {
      return bkFail (ini, BK_ESYNTAX, number, "malformed section header");
    }

  *end = '\0';
  BKChar *name = bkTrim (line + 1);
  if (*name == '\0')
    {
      return bkFail (ini, BK_ESYNTAX, number, "empty section name");
    }
  if (bkFind (ini, name))
    {
      return bkFail (ini, BK_EDUPLICATE, number, "duplicate section [%s]",
                     name);
    }

  *section = bkSectionNew (ini, name);

  return *section ? BK_OK : bkFailMem (ini);
}

/* Parses a key = value line. */
static BKStatus
bkParseEntry (BKIni *ini, BKChar *line, BKSize number, BKSection *section)
{
  BKChar *sep = bkStrFindAny (line, "=:");
  if (!sep)
    {
      return bkFail (ini, BK_ESYNTAX, number, "expected key = value");
    }
  if (!section)
    {
      return bkFail (ini, BK_ESYNTAX, number, "key outside of any section");
    }

  *sep = '\0';
  BKChar *key = bkTrim (line);
  BKChar *value = bkTrim (sep + 1);
  if (*key == '\0')
    {
      return bkFail (ini, BK_ESYNTAX, number, "empty key");
    }
  if (bkFindEntry (section, key))
    {
      return bkFail (ini, BK_EDUPLICATE, number, "duplicate key '%s' in [%s]",
                     key, section->name);
    }

  return bkEntryNew (section, key, value) ? BK_OK : bkFailMem (ini);
}

static BKStatus
bkParseLines (BKIni *ini, BKChar *text)
{
  BKSection *section = NULL;
  BKBool continuable = BK_FALSE;
  BKSize lastIndent = 0;
  BKSize blanks = 0;

  for (BKSize number = 1; *text; ++number)
    {
      BKChar *next = bkStrFind (text, '\n');
      if (next)
        {
          *next++ = '\0';
        }
      else
        {
          next = text + bkStrLen (text);
        }

      BKChar *line = bkTrim (text);
      BKSize indent = (BKSize)(line - text);
      text = next;

      if (*line == '\0' || *line == ';' || *line == '#')
        {
          // Blank lines belong to a value if a continuation follows them.
          if (*line == '\0' && continuable)
            {
              ++blanks;
            }
          continue;
        }

      // A line indented deeper than its key continues that key's value.
      if (continuable && indent > lastIndent && *line != '[')
        {
          BKEntry *entry = &section->entries[section->count - 1];
          for (; blanks > 0; --blanks)
            {
              if (!bkAppend (entry, ""))
                {
                  return bkFailMem (ini);
                }
            }
          if (!bkAppend (entry, line))
            {
              return bkFailMem (ini);
            }
          continue;
        }
      blanks = 0;

      BKStatus status;
      if (*line == '[')
        {
          status = bkParseHeader (ini, line, number, &section);
          continuable = BK_FALSE;
        }
      else
        {
          status = bkParseEntry (ini, line, number, section);
          continuable = BK_TRUE;
          lastIndent = indent;
        }

      if (status != BK_OK)
        {
          return status;
        }
    }

  return BK_OK;
}

/* Parses text the caller owns, in place; skips a leading UTF-8 BOM. */
static BKStatus
bkParseOwned (BKIni *ini, BKChar *text)
{
  if (bkStrCmpN (text, "\xef\xbb\xbf", 3) == 0)
    {
      text += 3;
    }

  return bkParseLines (ini, text);
}

/* Collects everything a reader supplies into a new NUL-terminated buffer. */
static BKStatus
bkReadAll (BKIni *ini, BKReadFn read, BKVoid *user, BKChar **text,
           BKSize *length)
{
  BKChar *buffer = NULL;
  BKSize used = 0;
  BKSize capacity = 0;
  for (;;)
    {
      if (!bkGrow ((BKVoid **)&buffer, &capacity, used + BK_READ_CHUNK + 1, 1))
        {
          bkMemFree (buffer);
          return bkFailMem (ini);
        }

      BKSize n = read (user, buffer + used, BK_READ_CHUNK);
      if (n == 0)
        {
          break;
        }
      // Covers BK_READ_ERROR as well as readers overrunning the buffer.
      if (n > BK_READ_CHUNK)
        {
          bkMemFree (buffer);
          return bkFail (ini, BK_EIO, 0, "cannot read INI text");
        }
      used += n;
    }

  buffer[used] = '\0';
  *text = buffer;
  *length = used;

  return BK_OK;
}

/* Parses read text unless it holds a NUL byte, which would end it early. */
static BKStatus
bkParseRead (BKIni *ini, BKChar *text, BKSize length)
{
  if (bkStrLen (text) != length)
    {
      return bkFail (ini, BK_ESYNTAX, 0, "NUL byte in INI text");
    }

  return bkParseOwned (ini, text);
}

/* Passes text to the writer, failing on a short write. */
static BKBool
bkEmit (BKWriteFn write, BKVoid *user, const BKChar *text, BKSize size)
{
  return size == 0 || write (user, text, size) == size;
}

static BKBool
bkEmitStr (BKWriteFn write, BKVoid *user, const BKChar *text)
{
  return bkEmit (write, user, text, bkStrLen (text));
}

/* Writes a value, turning line breaks into continuation lines. */
static BKBool
bkEmitValue (BKWriteFn write, BKVoid *user, const BKChar *value)
{
  for (;;)
    {
      const BKChar *end = bkStrFind (value, '\n');
      if (!end)
        {
          return bkEmitStr (write, user, value);
        }
      if (!bkEmit (write, user, value, (BKSize)(end - value))
          || !bkEmitStr (write, user, "\n\t"))
        {
          return BK_FALSE;
        }

      value = end + 1;
    }
}

static BKSize
bkReadFile (BKVoid *user, BKChar *data, BKSize size)
{
  BKFileReader *reader = user;
  BKSize n = bkFileRead (reader->file, data, size);
  if (n == 0 && bkFileFailed (reader->file))
    {
      reader->code = bkSysError ();
      return BK_READ_ERROR;
    }

  return n;
}

static BKSize
bkWriteFile (BKVoid *user, const BKChar *data, BKSize size)
{
  return bkFileWrite (user, data, size);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Public
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

BKIni *
bkInit (BKDword flags)
{
  if (flags & ~(BKDword)BK_FICASE)
    {
      return NULL;
    }

  BKIni *ini = bkMemZalloc (sizeof (BKIni));
  if (ini)
    {
      ini->flags = flags;
    }

  return ini;
}

void
bkDestroy (BKIni *ini)
{
  if (!ini)
    {
      return;
    }

  for (BKSize i = 0; i < ini->count; ++i)
    {
      bkFreeSection (ini->sections[i]);
    }

  bkMemFree (ini->sections);
  bkMemFree (ini);
}

const BKError *
bkError (const BKIni *ini)
{
  return ini ? &ini->error : NULL;
}

const BKChar *
bkStatusText (BKStatus status)
{
  switch (status)
    {
    case BK_OK:
      return "Success";
    case BK_EARG:
      return "Invalid argument";
    case BK_EMEM:
      return "Out of memory";
    case BK_EIO:
      return "Input/output error";
    case BK_ESYNTAX:
      return "Syntax error";
    case BK_EDUPLICATE:
      return "Duplicate section or key";
    }

  return "Unknown error";
}

BKStatus
bkParse (BKIni *ini, const BKChar *text)
{
  if (!ini)
    {
      return BK_EARG;
    }

  bkSucceed (ini);
  if (!text)
    {
      return bkFail (ini, BK_EARG, 0, "no text to parse");
    }

  BKChar *copy = bkStrDup (text, bkStrLen (text));
  if (!copy)
    {
      return bkFailMem (ini);
    }

  BKStatus status = bkParseOwned (ini, copy);
  bkMemFree (copy);

  return status;
}

BKStatus
bkLoad (BKIni *ini, const BKChar *path)
{
  if (!ini)
    {
      return BK_EARG;
    }

  bkSucceed (ini);
  if (!path)
    {
      return bkFail (ini, BK_EARG, 0, "no file to load");
    }

  BKFileReader reader = { bkFileOpen (path, BK_FALSE), 0 };
  if (!reader.file)
    {
      return bkFailSys (ini, path, bkSysError ());
    }

  // The file is closed before parsing, so that a failing close leaves the
  // document unchanged.
  BKChar *text = NULL;
  BKSize length = 0;
  BKStatus status = bkReadAll (ini, bkReadFile, &reader, &text, &length);
  if (status == BK_EIO)
    {
      status = bkFailSys (ini, path, reader.code);
    }

  if (!bkFileClose (reader.file) && status == BK_OK)
    {
      status = bkFailSys (ini, path, bkSysError ());
    }

  if (status == BK_OK)
    {
      status = bkParseRead (ini, text, length);
    }

  bkMemFree (text);

  return status;
}

BKStatus
bkRead (BKIni *ini, BKReadFn read, BKVoid *user)
{
  if (!ini)
    {
      return BK_EARG;
    }

  bkSucceed (ini);
  if (!read)
    {
      return bkFail (ini, BK_EARG, 0, "no reader");
    }

  BKChar *text = NULL;
  BKSize length = 0;
  BKStatus status = bkReadAll (ini, read, user, &text, &length);
  if (status == BK_OK)
    {
      status = bkParseRead (ini, text, length);
    }

  bkMemFree (text);

  return status;
}

BKStatus
bkWrite (BKIni *ini, BKWriteFn write, BKVoid *user)
{
  if (!ini)
    {
      return BK_EARG;
    }

  bkSucceed (ini);
  if (!write)
    {
      return bkFail (ini, BK_EARG, 0, "no writer");
    }

  for (BKSize i = 0; i < ini->count; ++i)
    {
      const BKSection *section = ini->sections[i];
      BKBool ok = (i == 0 || bkEmitStr (write, user, "\n"))
                  && bkEmitStr (write, user, "[")
                  && bkEmitStr (write, user, section->name)
                  && bkEmitStr (write, user, "]\n");

      for (BKSize j = 0; ok && j < section->count; ++j)
        {
          ok = bkEmitStr (write, user, section->entries[j].key)
               && bkEmitStr (write, user, " = ")
               && bkEmitValue (write, user, section->entries[j].value)
               && bkEmitStr (write, user, "\n");
        }

      if (!ok)
        {
          return bkFail (ini, BK_EIO, 0, "cannot write INI text");
        }
    }

  return BK_OK;
}

BKStatus
bkSave (BKIni *ini, const BKChar *path)
{
  if (!ini)
    {
      return BK_EARG;
    }

  bkSucceed (ini);
  if (!path)
    {
      return bkFail (ini, BK_EARG, 0, "no file to save");
    }

  BKFile *file = bkFileOpen (path, BK_TRUE);
  if (!file)
    {
      return bkFailSys (ini, path, bkSysError ());
    }

  BKStatus status = bkWrite (ini, bkWriteFile, file);
  BKInt code = status == BK_EIO ? bkSysError () : 0;
  if (!bkFileClose (file) && status == BK_OK)
    {
      status = BK_EIO;
      code = bkSysError ();
    }

  return status == BK_EIO ? bkFailSys (ini, path, code) : status;
}

const BKChar *
bkGet (const BKIni *ini, const BKChar *section, const BKChar *key)
{
  return ini && section ? bkSectionGet (bkFind (ini, section), key) : NULL;
}

BKStatus
bkSet (BKIni *ini, const BKChar *section, const BKChar *key,
       const BKChar *value)
{
  if (!ini)
    {
      return BK_EARG;
    }

  bkSucceed (ini);
  if (!bkValidSection (section))
    {
      return bkFail (ini, BK_EARG, 0, "invalid section name");
    }

  if (!bkValidKey (key))
    {
      return bkFail (ini, BK_EARG, 0, "invalid key in [%s]", section);
    }

  if (!value || bkStrFind (value, '\r'))
    {
      return bkFail (ini, BK_EARG, 0, "invalid value for '%s' in [%s]", key,
                     section);
    }

  BKSection *target = bkFind (ini, section);
  if (!target && !(target = bkSectionNew (ini, section)))
    {
      return bkFailMem (ini);
    }

  BKEntry *entry = bkFindEntry (target, key);
  if (!entry)
    {
      return bkEntryNew (target, key, value) ? BK_OK : bkFailMem (ini);
    }

  BKChar *copy = bkStrDup (value, bkStrLen (value));
  if (!copy)
    {
      return bkFailMem (ini);
    }

  bkMemFree (entry->value);
  entry->value = copy;

  return BK_OK;
}

BKBool
bkRemove (BKIni *ini, const BKChar *section, const BKChar *key)
{
  BKSection *target = ini && section ? bkFind (ini, section) : NULL;
  BKEntry *entry = target && key ? bkFindEntry (target, key) : NULL;
  if (!entry)
    {
      return BK_FALSE;
    }

  bkMemFree (entry->key);
  bkMemFree (entry->value);
  for (BKEntry *end = target->entries + --target->count; entry < end; ++entry)
    {
      entry[0] = entry[1];
    }

  return BK_TRUE;
}

BKStatus
bkSectionAdd (BKIni *ini, const BKChar *name)
{
  if (!ini)
    {
      return BK_EARG;
    }

  bkSucceed (ini);
  if (!bkValidSection (name))
    {
      return bkFail (ini, BK_EARG, 0, "invalid section name");
    }

  return bkFind (ini, name) || bkSectionNew (ini, name) ? BK_OK
                                                        : bkFailMem (ini);
}

BKBool
bkSectionRemove (BKIni *ini, const BKChar *name)
{
  BKSection *section = ini && name ? bkFind (ini, name) : NULL;
  if (!section)
    {
      return BK_FALSE;
    }

  bkFreeSection (section);
  BKSize i = 0;
  while (ini->sections[i] != section)
    {
      ++i;
    }
  for (--ini->count; i < ini->count; ++i)
    {
      ini->sections[i] = ini->sections[i + 1];
    }

  return BK_TRUE;
}

BKSize
bkSectionCount (const BKIni *ini)
{
  return ini ? ini->count : 0;
}

const BKSection *
bkSectionAt (const BKIni *ini, BKSize index)
{
  return ini && index < ini->count ? ini->sections[index] : NULL;
}

const BKSection *
bkSectionFind (const BKIni *ini, const BKChar *name)
{
  return ini && name ? bkFind (ini, name) : NULL;
}

const BKChar *
bkSectionName (const BKSection *section)
{
  return section ? section->name : NULL;
}

BKSize
bkSectionSize (const BKSection *section)
{
  return section ? section->count : 0;
}

const BKChar *
bkSectionKey (const BKSection *section, BKSize index)
{
  return section && index < section->count ? section->entries[index].key
                                           : NULL;
}

const BKChar *
bkSectionValue (const BKSection *section, BKSize index)
{
  return section && index < section->count ? section->entries[index].value
                                           : NULL;
}

const BKChar *
bkSectionGet (const BKSection *section, const BKChar *key)
{
  if (!section || !key)
    {
      return NULL;
    }

  const BKEntry *entry = bkFindEntry (section, key);

  return entry ? entry->value : NULL;
}
