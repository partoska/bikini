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

#include "bikini.h"
#include "plat.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Types
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef enum BKCExit
{
  BKC_EXIT_OK = 0,
  BKC_EXIT_MISSING = 1,
  BKC_EXIT_ERROR = 2
} BKCExit;

typedef enum BKCMode
{
  BKC_MODE_QUERY,
  BKC_MODE_KEYS,
  BKC_MODE_SET,
  BKC_MODE_ADD,
  BKC_MODE_DELETE
} BKCMode;

typedef struct BKCArgs
{
  BKDword flags;
  BKCMode mode;
  BKBool write;
  const BKChar *delimiter;
  const BKChar *value;
  const BKChar *query;
  const BKChar **paths;
  BKSize count;
} BKCArgs;

typedef struct BKCPath
{
  const BKChar *query;
  BKBool root;
  const BKChar *whole;
  const BKChar *section;
  const BKChar *key;
  BKSize sindex;  /* With --multi: index of the section, */
  BKSize kindex;  /* and of the key. */
  BKBool sall;    /* With --multi: [*] in place of an index, */
  BKBool kall;    /* for the section and the key. */
  BKBool indexed; /* With --multi: the key has an index. */
  BKChar *buffer;
} BKCPath;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Private
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void
bkcLogo (void)
{
  bkcPrint (
      "   ___  _ __    _      _\n"
      "  / _ )(_) /__ (_)__  (_)\n"
      " / _  / /  '_// / _ \\/ /\n"
      "/____/_/_/\\_\\/_/_//_/_/\n"
      "\n"
      "Bikini v" BK_VERSION ", <https://lab.partoska.com/bikini>\n"
      "Copyright (C) 2026 Fabrika Charvat s.r.o. All rights reserved.\n"
      "Licensed under the MIT License.\n"
      "\n");
}

static void
bkcUsage (void)
{
  bkcLogo ();
  bkcPrint ("Usage: bikini [OPTIONS] [QUERY] [FILE...]\n"
            "\n"
            "Queries an INI document read from FILE, or from standard input\n"
            "when FILE is missing or -. Several files are merged in order,\n"
            "later values replacing earlier ones.\n"
            "\n"
            "Query:\n"
            "  .                  The whole document, normalized (default)\n"
            "  SECTION            One section, as INI text\n"
            "  SECTION.KEY        The raw value of a key\n"
            "  \"NAME\"             A quoted section or key name, which may\n"
            "                     contain dots; \\\" and \\\\ escape\n"
            "  NAME[N], NAME[*]   With --multi, every name needs an index\n"
            "                     among its repeats, or * for all of them\n"
            "\n"
            "Options:\n"
            "  -k, --keys         List the sections of the document, or the\n"
            "                     keys of SECTION\n"
            "  -s, --set VALUE    Set SECTION.KEY to VALUE and print the\n"
            "                     document\n"
            "  --add VALUE        With --multi, add VALUE to SECTION[N].KEY\n"
            "                     after its last value, or as a new section\n"
            "                     one past the last one, and print the\n"
            "                     document\n"
            "  -d, --delete       Delete SECTION or SECTION.KEY and print\n"
            "                     the document\n"
            "  -w, --write        Save the edit back to FILE instead\n"
            "  -i, --ignore-case  Match sections and keys case-insensitively\n"
            "  --multi            Allow repeated sections and keys, and\n"
            "                     concatenate several files\n"
            "  --delimiter TEXT   Write TEXT between key and value, such as\n"
            "                     =, : or ' = ' (default)\n"
            "  -h, --help         Show this help\n"
            "  -V, --version      Show the version\n"
            "\n"
            "Exit status: 0 found, 1 not found, 2 usage or input error.\n");
}

static BKCExit
bkcFailMem (void)
{
  bkcError ("bikini: %s\n", bkStatusText (BK_EMEM));

  return BKC_EXIT_ERROR;
}

static const BKChar *
bkcInputName (const BKChar *path)
{
  return path ? path : "standard input";
}

static BKCExit
bkcSetMode (BKCArgs *args, BKCMode mode)
{
  if (args->mode != BKC_MODE_QUERY && args->mode != mode)
    {
      bkcError ("bikini: options -k, -s, --add and -d exclude each other\n");
      return BKC_EXIT_ERROR;
    }
  args->mode = mode;

  return BKC_EXIT_OK;
}

static BKCExit
bkcParseArgs (BKInt argc, BKChar *argv[], BKCArgs *args, BKBool *done)
{
  BKBool query = BK_FALSE;
  BKBool options = BK_TRUE;
  for (BKInt i = 1; i < argc; ++i)
    {
      const BKChar *arg = argv[i];
      if (options && arg[0] == '-' && arg[1] != '\0')
        {
          BKCExit result = BKC_EXIT_OK;
          if (bkcStrEq (arg, "--"))
            {
              options = BK_FALSE;
            }
          else if (bkcStrEq (arg, "-i") || bkcStrEq (arg, "--ignore-case"))
            {
              args->flags |= BK_FICASE;
            }
          else if (bkcStrEq (arg, "--multi"))
            {
              args->flags |= BK_FMULTI;
            }
          else if (bkcStrEq (arg, "--delimiter"))
            {
              if (++i == argc)
                {
                  bkcError ("bikini: option '%s' needs a value\n", arg);
                  return BKC_EXIT_ERROR;
                }
              args->delimiter = argv[i];
            }
          else if (bkcStrEq (arg, "-k") || bkcStrEq (arg, "--keys"))
            {
              result = bkcSetMode (args, BKC_MODE_KEYS);
            }
          else if (bkcStrEq (arg, "-s") || bkcStrEq (arg, "--set")
                   || bkcStrEq (arg, "--add"))
            {
              if (++i == argc)
                {
                  bkcError ("bikini: option '%s' needs a value\n", arg);
                  return BKC_EXIT_ERROR;
                }
              args->value = argv[i];
              result = bkcSetMode (
                  args, bkcStrEq (arg, "--add") ? BKC_MODE_ADD : BKC_MODE_SET);
            }
          else if (bkcStrEq (arg, "-d") || bkcStrEq (arg, "--delete"))
            {
              result = bkcSetMode (args, BKC_MODE_DELETE);
            }
          else if (bkcStrEq (arg, "-w") || bkcStrEq (arg, "--write"))
            {
              args->write = BK_TRUE;
            }
          else if (bkcStrEq (arg, "-h") || bkcStrEq (arg, "--help"))
            {
              bkcUsage ();
              *done = BK_TRUE;
            }
          else if (bkcStrEq (arg, "-V") || bkcStrEq (arg, "--version"))
            {
              bkcPrint ("bikini " BK_VERSION "\n");
              *done = BK_TRUE;
            }
          else
            {
              bkcError ("bikini: unknown option '%s'\n", arg);
              result = BKC_EXIT_ERROR;
            }
          if (result != BKC_EXIT_OK || *done)
            {
              return result;
            }
          continue;
        }

      if (!query)
        {
          args->query = arg;
          query = BK_TRUE;
        }
      else
        {
          args->paths[args->count++] = bkcStrEq (arg, "-") ? NULL : arg;
        }
    }

  // No file reads standard input.
  if (args->count == 0)
    {
      args->paths[args->count++] = NULL;
    }

  return BKC_EXIT_OK;
}

static BKCExit
bkcQueryErr (const BKChar *query, const BKChar *what)
{
  bkcError ("bikini: invalid query '%s': %s\n", query, what);

  return BKC_EXIT_ERROR;
}

/* Splits a query with quoted names at the dots outside of quotes: the last
 * name is the key, the others joined by dots the section. */
static BKCExit
bkcParseQuoted (const BKChar *query, BKCPath *path)
{
  path->buffer = bkcStrDup (query, bkcStrLen (query));
  if (!path->buffer)
    {
      return bkcFailMem ();
    }

  // Unescaped names are never longer than the query, so they fit.
  const BKChar *in = query;
  BKChar *out = path->buffer;
  BKChar *split = NULL;
  for (;;)
    {
      const BKChar *name = out;
      if (*in == '"')
        {
          for (++in; *in != '"'; ++in)
            {
              if (*in == '\0')
                {
                  return bkcQueryErr (query, "unterminated quote");
                }
              if (*in == '\\' && (in[1] == '"' || in[1] == '\\'))
                {
                  ++in;
                }
              *out++ = *in;
            }
          ++in;
        }
      else
        {
          for (; *in != '.' && *in != '\0'; ++in)
            {
              if (*in == '"')
                {
                  return bkcQueryErr (query, "quote inside a name");
                }
              *out++ = *in;
            }
        }

      if (out == name)
        {
          return bkcQueryErr (query, "empty name");
        }
      if (*in == '\0')
        {
          break;
        }
      if (*in != '.')
        {
          return bkcQueryErr (query, "expected . after a quoted name");
        }
      split = out;
      *out++ = *in++;
    }
  *out = '\0';

  if (!split)
    {
      path->whole = path->buffer;
      return BKC_EXIT_OK;
    }

  *split = '\0';
  path->section = path->buffer;
  path->key = split + 1;

  return BKC_EXIT_OK;
}

/* Reads an index between brackets, a number or *, and moves past it. */
static BKBool
bkcParseIndex (const BKChar **in, BKSize *index, BKBool *all)
{
  const BKChar *at = *in;
  if (*at++ != '[')
    {
      return BK_FALSE;
    }

  *index = 0;
  *all = *at == '*';
  if (*all)
    {
      ++at;
    }
  else if (*at < '0' || *at > '9')
    {
      return BK_FALSE;
    }
  for (; *at >= '0' && *at <= '9'; ++at)
    {
      BKSize digit = (BKSize)(*at - '0');
      if (*index > ((BKSize)-1 - digit) / 10)
        {
          return BK_FALSE;
        }
      *index = *index * 10 + digit;
    }
  if (*at++ != ']')
    {
      return BK_FALSE;
    }

  *in = at;

  return BK_TRUE;
}

/* Copies a quoted name, unescaping \" and \\, and moves past it. */
static BKBool
bkcParseQuote (const BKChar **in, BKChar **out)
{
  const BKChar *at = *in + 1;
  for (; *at != '"'; ++at)
    {
      if (*at == '\0')
        {
          return BK_FALSE;
        }
      if (*at == '\\' && (at[1] == '"' || at[1] == '\\'))
        {
          ++at;
        }
      *(*out)++ = *at;
    }
  *in = at + 1;

  return BK_TRUE;
}

/* Splits a query of --multi, where every name carries an index: a section
 * name ends at the first ], so its dots need no quotes; the key is the rest,
 * with its index at the end. */
static BKCExit
bkcParseIndexed (const BKChar *query, BKCPath *path)
{
  path->buffer = bkcStrDup (query, bkcStrLen (query));
  if (!path->buffer)
    {
      return bkcFailMem ();
    }

  // Names never grow when copied, and each ends where its index starts.
  const BKChar *in = query;
  BKChar *out = path->buffer;
  const BKChar *name = out;
  if (*in == '"')
    {
      if (!bkcParseQuote (&in, &out))
        {
          return bkcQueryErr (query, "unterminated quote");
        }
    }
  else
    {
      const BKChar *close = bkcStrFind (in, ']');
      const BKChar *open = close ? in : NULL;
      for (const BKChar *at = in; close && at < close; ++at)
        {
          open = *at == '[' ? at : open;
        }
      if (!close || *open != '[')
        {
          return bkcQueryErr (query, "missing index of the section");
        }
      for (; in < open; ++in)
        {
          *out++ = *in;
        }
    }
  if (out == name)
    {
      return bkcQueryErr (query, "empty name");
    }
  *out++ = '\0';
  if (!bkcParseIndex (&in, &path->sindex, &path->sall))
    {
      return bkcQueryErr (query, "invalid index of the section");
    }

  if (*in == '\0')
    {
      path->whole = name;
      return BKC_EXIT_OK;
    }
  if (*in++ != '.')
    {
      return bkcQueryErr (query, "expected . after an index");
    }

  path->section = name;
  path->key = out;
  if (*in == '"')
    {
      if (!bkcParseQuote (&in, &out))
        {
          return bkcQueryErr (query, "unterminated quote");
        }
    }
  else
    {
      // The index of the key follows its last [, if it is at the end.
      const BKChar *end = in + bkcStrLen (in);
      const BKChar *open = bkcStrFindLast (in, '[');
      if (end > in && end[-1] == ']' && open)
        {
          end = open;
        }
      for (; in < end; ++in)
        {
          *out++ = *in;
        }
    }
  if (out == path->key)
    {
      return bkcQueryErr (query, "empty name");
    }
  *out = '\0';

  path->indexed = *in != '\0';
  if (path->indexed
      && (!bkcParseIndex (&in, &path->kindex, &path->kall) || *in != '\0'))
    {
      return bkcQueryErr (query, "invalid index of the key");
    }

  return BKC_EXIT_OK;
}

static BKCExit
bkcParsePath (const BKChar *query, BKCPath *path, BKBool multi)
{
  path->query = query;
  if (bkcStrEq (query, "."))
    {
      path->root = BK_TRUE;
      return BKC_EXIT_OK;
    }

  if (multi)
    {
      return bkcParseIndexed (query, path);
    }

  if (bkcStrFind (query, '"'))
    {
      return bkcParseQuoted (query, path);
    }

  // A whole section name wins, so that sections may contain dots; otherwise
  // the key follows the last dot.
  path->whole = query;
  const BKChar *dot = bkcStrFindLast (query, '.');
  if (!dot)
    {
      return BKC_EXIT_OK;
    }

  path->buffer = bkcStrDup (query, bkcStrLen (query));
  if (!path->buffer)
    {
      return bkcFailMem ();
    }
  path->buffer[dot - query] = '\0';
  path->section = path->buffer;
  path->key = path->buffer + (dot - query) + 1;

  return BKC_EXIT_OK;
}

/* Rejects combinations of options and query that make no sense. */
static BKCExit
bkcCheck (const BKCArgs *args, const BKCPath *path)
{
  if (args->mode == BKC_MODE_SET && !path->key)
    {
      bkcError ("bikini: --set needs SECTION.KEY\n");
      return BKC_EXIT_ERROR;
    }

  if (args->mode == BKC_MODE_ADD && !(args->flags & BK_FMULTI))
    {
      bkcError ("bikini: --add needs --multi\n");
      return BKC_EXIT_ERROR;
    }

  if (args->mode == BKC_MODE_ADD && (!path->key || path->indexed))
    {
      bkcError ("bikini: --add needs SECTION[N].KEY, the key without index\n");
      return BKC_EXIT_ERROR;
    }

  if (args->mode != BKC_MODE_ADD && path->key && (args->flags & BK_FMULTI)
      && !path->indexed)
    {
      bkcError ("bikini: invalid query '%s': missing index of the key\n",
                path->query);
      return BKC_EXIT_ERROR;
    }

  if ((args->mode == BKC_MODE_SET || args->mode == BKC_MODE_ADD
       || args->mode == BKC_MODE_KEYS)
      && (path->sall || path->kall))
    {
      bkcError ("bikini: [*] only selects in queries and --delete\n");
      return BKC_EXIT_ERROR;
    }

  if (args->mode == BKC_MODE_DELETE && path->root)
    {
      bkcError ("bikini: --delete needs SECTION or SECTION.KEY\n");
      return BKC_EXIT_ERROR;
    }

  if (!args->write)
    {
      return BKC_EXIT_OK;
    }

  if (args->mode != BKC_MODE_SET && args->mode != BKC_MODE_ADD
      && args->mode != BKC_MODE_DELETE)
    {
      bkcError ("bikini: --write needs --set, --add or --delete\n");
      return BKC_EXIT_ERROR;
    }

  if (args->count != 1 || !args->paths[0])
    {
      bkcError ("bikini: --write needs exactly one FILE\n");
      return BKC_EXIT_ERROR;
    }

  return BKC_EXIT_OK;
}

static BKCExit
bkcLoad (BKIni *ini, const BKChar *path)
{
  if (path)
    {
      BKStatus status = bkLoad (ini, path);
      if (status == BK_EIO)
        {
          // The message names the file already.
          bkcError ("bikini: %s\n", bkError (ini)->message);
          return BKC_EXIT_ERROR;
        }

      if (status != BK_OK)
        {
          bkcError ("bikini: %s: %s\n", path, bkError (ini)->message);
          return BKC_EXIT_ERROR;
        }

      return BKC_EXIT_OK;
    }

  BKInt code = 0;
  if (bkRead (ini, bkcReadIn, &code) != BK_OK)
    {
      bkcError ("bikini: standard input: %s\n",
                code ? bkcSysErrorText (code) : bkError (ini)->message);
      return BKC_EXIT_ERROR;
    }

  return BKC_EXIT_OK;
}

/* Copies a section into a document, merging it with one of the same name. */
static BKStatus
bkcSectionCopy (BKIni *ini, const BKSection *section)
{
  const BKChar *name = bkSectionName (section);
  BKStatus status = bkSectionSet (ini, name);
  for (BKSize i = 0; status == BK_OK && i < bkKeyCount (section); ++i)
    {
      status = bkSet (ini, name, bkKeyAt (section, i), bkValueAt (section, i));
    }

  return status;
}

/* Loads the inputs in order; a section repeated in a later input adds its
 * keys to the earlier one, replacing the values of keys both have. With
 * --multi the inputs are concatenated instead, as if they were one. */
static BKCExit
bkcLoadAll (BKIni *ini, const BKCArgs *args)
{
  BKCExit result = bkcLoad (ini, args->paths[0]);
  for (BKSize i = 1; result == BKC_EXIT_OK && i < args->count; ++i)
    {
      if (args->flags & BK_FMULTI)
        {
          result = bkcLoad (ini, args->paths[i]);
          continue;
        }

      BKIni *more = bkInit (args->flags);
      if (!more)
        {
          return bkcFailMem ();
        }

      result = bkcLoad (more, args->paths[i]);
      for (BKSize j = 0; result == BKC_EXIT_OK && j < bkSectionCount (more);
           ++j)
        {
          if (bkcSectionCopy (ini, bkSectionAt (more, j)) != BK_OK)
            {
              bkcError ("bikini: %s: %s\n", bkcInputName (args->paths[i]),
                        bkError (ini)->message);
              result = BKC_EXIT_ERROR;
            }
        }
      bkDestroy (more);
    }

  return result;
}

static BKCExit
bkcPrintLine (const BKChar *text)
{
  return bkcPrint (text) && bkcPrint ("\n") ? BKC_EXIT_OK : BKC_EXIT_ERROR;
}

static BKCExit
bkcDocPrint (BKIni *ini)
{
  if (bkWrite (ini, bkcWriteOut, NULL) != BK_OK)
    {
      bkcError ("bikini: %s\n", bkError (ini)->message);
      return BKC_EXIT_ERROR;
    }

  return BKC_EXIT_OK;
}

/* Prints a name with its index among repeats, as --multi queries take it. */
static BKCExit
bkcPrintIndexed (const BKChar *name, BKSize index)
{
  BKChar digits[32];
  BKChar *at = digits + sizeof (digits);
  *--at = '\0';
  do
    {
      *--at = (BKChar)('0' + index % 10);
      index /= 10;
    }
  while (index > 0);

  return bkcPrint (name) && bkcPrint ("[") && bkcPrint (at) && bkcPrint ("]\n")
             ? BKC_EXIT_OK
             : BKC_EXIT_ERROR;
}

/* Gets the index of a section among the sections of its name. */
static BKSize
bkcSectionIndex (const BKIni *ini, const BKSection *section)
{
  BKSize index = 0;
  while (bkSectionFindNth (ini, bkSectionName (section), index) != section)
    {
      ++index;
    }

  return index;
}

/* Gets the index of the key at a position among the values of that key. */
static BKSize
bkcKeyIndex (const BKSection *section, BKSize position)
{
  const BKChar *key = bkKeyAt (section, position);
  const BKChar *value = bkValueAt (section, position);
  BKSize index = 0;
  while (bkSectionGetNth (section, key, index) != value)
    {
      ++index;
    }

  return index;
}

/* Writes sections of one name, from first up to last, as INI text, by
 * removing every other section from the document. */
static BKCExit
bkcSectionPrint (BKIni *ini, const BKChar *name, BKSize first, BKSize last)
{
  BKSize count = last - first;
  const BKSection **keep = bkcMemAlloc (sizeof (*keep) * count);
  if (!keep)
    {
      return bkcFailMem ();
    }
  for (BKSize i = 0; i < count; ++i)
    {
      keep[i] = bkSectionFindNth (ini, name, first + i);
    }

  for (BKSize position = bkSectionCount (ini); position > 0; --position)
    {
      const BKSection *section = bkSectionAt (ini, position - 1);
      BKBool kept = BK_FALSE;
      for (BKSize i = 0; i < count; ++i)
        {
          kept = kept || keep[i] == section;
        }
      if (!kept)
        {
          bkSectionRemoveNth (ini, bkSectionName (section),
                              bkcSectionIndex (ini, section));
        }
    }
  bkcMemFree (keep);

  return bkcDocPrint (ini);
}

/* Resolves the sections a --multi query selects, from first up to last. */
static BKBool
bkcRange (const BKIni *ini, const BKCPath *path, BKSize *first, BKSize *last)
{
  const BKChar *name = path->whole ? path->whole : path->section;
  BKSize count = bkSectionCountOf (ini, name);
  if (path->sall)
    {
      *first = 0;
      *last = count;
    }
  else if (path->sindex < count)
    {
      *first = path->sindex;
      *last = path->sindex + 1;
    }
  else
    {
      return BK_FALSE;
    }

  return *first < *last;
}

/* Finds the section a query names as a whole. */
static const BKSection *
bkcFindWhole (const BKIni *ini, const BKCPath *path)
{
  return path->whole ? bkSectionFind (ini, path->whole) : NULL;
}

static BKCExit
bkcQuery (BKIni *ini, const BKCPath *path)
{
  if (path->root)
    {
      return bkcDocPrint (ini);
    }

  if (bkcFindWhole (ini, path))
    {
      return bkcSectionPrint (ini, path->whole, 0, 1);
    }

  const BKChar *value
      = path->key ? bkGet (ini, path->section, path->key) : NULL;

  return value ? bkcPrintLine (value) : BKC_EXIT_MISSING;
}

/* Prints what a --multi query selects: sections as INI text, values one
 * per line. */
static BKCExit
bkcQueryIndexed (BKIni *ini, const BKCPath *path)
{
  if (path->root)
    {
      return bkcDocPrint (ini);
    }

  BKSize first = 0;
  BKSize last = 0;
  if (!bkcRange (ini, path, &first, &last))
    {
      return BKC_EXIT_MISSING;
    }
  if (path->whole)
    {
      return bkcSectionPrint (ini, path->whole, first, last);
    }

  BKBool found = BK_FALSE;
  for (BKSize i = first; i < last; ++i)
    {
      const BKSection *section = bkSectionFindNth (ini, path->section, i);
      BKSize count = bkKeyCountOf (section, path->key);
      BKSize from = path->kall ? 0 : path->kindex;
      BKSize to = path->kall             ? count
                  : path->kindex < count ? path->kindex + 1
                                         : 0;
      for (BKSize j = from; j < to; ++j)
        {
          if (bkcPrintLine (bkSectionGetNth (section, path->key, j))
              != BKC_EXIT_OK)
            {
              return BKC_EXIT_ERROR;
            }
          found = BK_TRUE;
        }
    }

  return found ? BKC_EXIT_OK : BKC_EXIT_MISSING;
}

/* Prints the names of the sections of the document, or of the keys of a
 * section, one per line. */
static BKCExit
bkcKeys (const BKIni *ini, const BKCPath *path)
{
  BKCExit result = BKC_EXIT_OK;
  if (path->root)
    {
      for (BKSize i = 0; result == BKC_EXIT_OK && i < bkSectionCount (ini);
           ++i)
        {
          result = bkcPrintLine (bkSectionName (bkSectionAt (ini, i)));
        }
      return result;
    }

  const BKSection *section = bkcFindWhole (ini, path);
  if (section)
    {
      for (BKSize i = 0; result == BKC_EXIT_OK && i < bkKeyCount (section);
           ++i)
        {
          result = bkcPrintLine (bkKeyAt (section, i));
        }
      return result;
    }

  if (path->key && bkGet (ini, path->section, path->key))
    {
      bkcError ("bikini: %s: a value has no keys\n", path->query);
      return BKC_EXIT_ERROR;
    }

  return BKC_EXIT_MISSING;
}

/* Prints the names of a --multi query with their indexes, ready to be
 * queried: the sections of the document, or the keys of a section. */
static BKCExit
bkcKeysIndexed (const BKIni *ini, const BKCPath *path)
{
  BKCExit result = BKC_EXIT_OK;
  if (path->root)
    {
      for (BKSize i = 0; result == BKC_EXIT_OK && i < bkSectionCount (ini);
           ++i)
        {
          const BKSection *section = bkSectionAt (ini, i);
          result = bkcPrintIndexed (bkSectionName (section),
                                    bkcSectionIndex (ini, section));
        }
      return result;
    }

  if (path->whole)
    {
      const BKSection *section
          = bkSectionFindNth (ini, path->whole, path->sindex);
      if (!section)
        {
          return BKC_EXIT_MISSING;
        }
      for (BKSize i = 0; result == BKC_EXIT_OK && i < bkKeyCount (section);
           ++i)
        {
          result = bkcPrintIndexed (bkKeyAt (section, i),
                                    bkcKeyIndex (section, i));
        }
      return result;
    }

  if (bkGetNth (ini, path->section, path->sindex, path->key, path->kindex))
    {
      bkcError ("bikini: %s: a value has no keys\n", path->query);
      return BKC_EXIT_ERROR;
    }

  return BKC_EXIT_MISSING;
}

/* Sets or deletes without --multi. */
static BKCExit
bkcChange (BKIni *ini, const BKCArgs *args, const BKCPath *path)
{
  if (args->mode == BKC_MODE_SET)
    {
      if (bkSet (ini, path->section, path->key, args->value) != BK_OK)
        {
          bkcError ("bikini: %s\n", bkError (ini)->message);
          return BKC_EXIT_ERROR;
        }
      return BKC_EXIT_OK;
    }

  return (path->whole && bkSectionRemove (ini, path->whole))
                 || (path->key && bkRemove (ini, path->section, path->key))
             ? BKC_EXIT_OK
             : BKC_EXIT_MISSING;
}

/* Deletes what a --multi query selects; reports whether anything went. */
static BKBool
bkcDeleteIndexed (BKIni *ini, const BKCPath *path)
{
  BKBool removed = BK_FALSE;
  if (path->whole && !path->sall)
    {
      return bkSectionRemoveNth (ini, path->whole, path->sindex);
    }
  if (path->whole)
    {
      while (bkSectionRemove (ini, path->whole))
        {
          removed = BK_TRUE;
        }
      return removed;
    }

  // Removing keys leaves the indexes of the sections as they are.
  BKSize first = 0;
  BKSize last = 0;
  if (!bkcRange (ini, path, &first, &last))
    {
      return BK_FALSE;
    }
  for (BKSize i = first; i < last; ++i)
    {
      if (!path->kall)
        {
          removed
              = bkRemoveNth (ini, path->section, i, path->key, path->kindex)
                || removed;
          continue;
        }
      while (bkRemoveNth (ini, path->section, i, path->key, 0))
        {
          removed = BK_TRUE;
        }
    }

  return removed;
}

/* Sets, adds or deletes with --multi; a missing target is not found. */
static BKCExit
bkcChangeIndexed (BKIni *ini, const BKCArgs *args, const BKCPath *path)
{
  BKStatus status = BK_OK;
  switch (args->mode)
    {
    case BKC_MODE_SET:
      if (!bkGetNth (ini, path->section, path->sindex, path->key,
                     path->kindex))
        {
          return BKC_EXIT_MISSING;
        }
      status = bkSetNth (ini, path->section, path->sindex, path->key,
                         path->kindex, args->value);
      break;
    case BKC_MODE_ADD:
      if (path->sindex > bkSectionCountOf (ini, path->section))
        {
          return BKC_EXIT_MISSING;
        }
      status
          = bkAdd (ini, path->section, path->sindex, path->key, args->value);
      break;
    default:
      return bkcDeleteIndexed (ini, path) ? BKC_EXIT_OK : BKC_EXIT_MISSING;
    }

  if (status != BK_OK)
    {
      bkcError ("bikini: %s\n", bkError (ini)->message);
      return BKC_EXIT_ERROR;
    }

  return BKC_EXIT_OK;
}

/* Edits, then prints the document or saves it to its file. A missing
 * target leaves the file as it is. */
static BKCExit
bkcEdit (BKIni *ini, const BKCArgs *args, const BKCPath *path)
{
  BKCExit result = (args->flags & BK_FMULTI)
                       ? bkcChangeIndexed (ini, args, path)
                       : bkcChange (ini, args, path);
  if (result == BKC_EXIT_ERROR)
    {
      return result;
    }

  if (!args->write)
    {
      BKCExit printed = bkcDocPrint (ini);
      return printed == BKC_EXIT_OK ? result : printed;
    }

  if (result != BKC_EXIT_OK)
    {
      return result;
    }

  if (bkSave (ini, args->paths[0]) != BK_OK)
    {
      // The message names the file already.
      bkcError ("bikini: %s\n", bkError (ini)->message);
      return BKC_EXIT_ERROR;
    }

  return BKC_EXIT_OK;
}

static BKCExit
bkcRun (const BKCArgs *args)
{
  BKCPath path = { NULL, BK_FALSE, NULL,     NULL,     NULL, 0,
                   0,    BK_FALSE, BK_FALSE, BK_FALSE, NULL };
  BKCExit result
      = bkcParsePath (args->query, &path, (args->flags & BK_FMULTI) != 0);
  if (result == BKC_EXIT_OK)
    {
      result = bkcCheck (args, &path);
    }

  if (result != BKC_EXIT_OK)
    {
      bkcMemFree (path.buffer);
      return result;
    }

  BKIni *ini = bkInit (args->flags);
  if (!ini)
    {
      bkcMemFree (path.buffer);
      return bkcFailMem ();
    }

  if (args->delimiter && bkSetDelimiter (ini, args->delimiter) != BK_OK)
    {
      bkcError ("bikini: %s\n", bkError (ini)->message);
      result = BKC_EXIT_ERROR;
    }
  if (result == BKC_EXIT_OK)
    {
      result = bkcLoadAll (ini, args);
    }
  if (result == BKC_EXIT_OK)
    {
      switch (args->mode)
        {
        case BKC_MODE_QUERY:
          result = (args->flags & BK_FMULTI) ? bkcQueryIndexed (ini, &path)
                                             : bkcQuery (ini, &path);
          break;
        case BKC_MODE_KEYS:
          result = (args->flags & BK_FMULTI) ? bkcKeysIndexed (ini, &path)
                                             : bkcKeys (ini, &path);
          break;
        case BKC_MODE_SET:
        case BKC_MODE_ADD:
        case BKC_MODE_DELETE:
          result = bkcEdit (ini, args, &path);
          break;
        }
    }

  bkDestroy (ini);
  bkcMemFree (path.buffer);

  return result;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Definitions - Public
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

BKInt
main (BKInt argc, BKChar *argv[])
{
  bkcStreamInit ();

  BKCArgs args
      = { BK_FNONE, BKC_MODE_QUERY, BK_FALSE, NULL, NULL, ".", NULL, 0 };
  args.paths = bkcMemAlloc (sizeof (*args.paths) * ((BKSize)argc + 1));
  if (!args.paths)
    {
      return (BKInt)bkcFailMem ();
    }

  BKBool done = BK_FALSE;
  BKCExit result = bkcParseArgs (argc, argv, &args, &done);
  if (result == BKC_EXIT_OK && !done)
    {
      result = bkcRun (&args);
    }
  bkcMemFree (args.paths);

  if (!bkcFlush ())
    {
      bkcError ("bikini: standard output: %s\n",
                bkcSysErrorText (bkcSysError ()));
      return BKC_EXIT_ERROR;
    }

  return (BKInt)result;
}
