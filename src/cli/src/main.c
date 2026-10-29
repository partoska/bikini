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
  BKC_MODE_DELETE
} BKCMode;

typedef struct BKCArgs
{
  BKDword flags;
  BKCMode mode;
  BKBool write;
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
            "\n"
            "Options:\n"
            "  -k, --keys         List the sections of the document, or the\n"
            "                     keys of SECTION\n"
            "  -s, --set VALUE    Set SECTION.KEY to VALUE and print the\n"
            "                     document\n"
            "  -d, --delete       Delete SECTION or SECTION.KEY and print\n"
            "                     the document\n"
            "  -w, --write        Save the edit back to FILE instead\n"
            "  -i, --ignore-case  Match sections and keys case-insensitively\n"
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
      bkcError ("bikini: options -k, -s and -d exclude each other\n");
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
          else if (bkcStrEq (arg, "-k") || bkcStrEq (arg, "--keys"))
            {
              result = bkcSetMode (args, BKC_MODE_KEYS);
            }
          else if (bkcStrEq (arg, "-s") || bkcStrEq (arg, "--set"))
            {
              if (++i == argc)
                {
                  bkcError ("bikini: option '%s' needs a value\n", arg);
                  return BKC_EXIT_ERROR;
                }
              args->value = argv[i];
              result = bkcSetMode (args, BKC_MODE_SET);
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

static BKCExit
bkcParsePath (const BKChar *query, BKCPath *path)
{
  path->query = query;
  if (bkcStrEq (query, "."))
    {
      path->root = BK_TRUE;
      return BKC_EXIT_OK;
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

  if (args->mode == BKC_MODE_DELETE && path->root)
    {
      bkcError ("bikini: --delete needs SECTION or SECTION.KEY\n");
      return BKC_EXIT_ERROR;
    }

  if (!args->write)
    {
      return BKC_EXIT_OK;
    }

  if (args->mode != BKC_MODE_SET && args->mode != BKC_MODE_DELETE)
    {
      bkcError ("bikini: --write needs --set or --delete\n");
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
  BKStatus status = bkSectionAdd (ini, name);
  for (BKSize i = 0; status == BK_OK && i < bkSectionSize (section); ++i)
    {
      status = bkSet (ini, name, bkSectionKey (section, i),
                      bkSectionValue (section, i));
    }

  return status;
}

/* Loads the inputs in order; a section repeated in a later input adds its
 * keys to the earlier one, replacing the values of keys both have. */
static BKCExit
bkcLoadAll (BKIni *ini, const BKCArgs *args)
{
  BKCExit result = bkcLoad (ini, args->paths[0]);
  for (BKSize i = 1; result == BKC_EXIT_OK && i < args->count; ++i)
    {
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

/* Writes one section as INI text, through a document of its own. */
static BKCExit
bkcSectionPrint (const BKSection *section, BKDword flags)
{
  BKIni *out = bkInit (flags);
  if (!out)
    {
      return bkcFailMem ();
    }

  BKCExit result = BKC_EXIT_OK;
  if (bkcSectionCopy (out, section) != BK_OK)
    {
      bkcError ("bikini: %s\n", bkError (out)->message);
      result = BKC_EXIT_ERROR;
    }
  else
    {
      result = bkcDocPrint (out);
    }

  bkDestroy (out);

  return result;
}

/* Finds the section a query names as a whole. */
static const BKSection *
bkcFindWhole (const BKIni *ini, const BKCPath *path)
{
  return path->whole ? bkSectionFind (ini, path->whole) : NULL;
}

static BKCExit
bkcQuery (BKIni *ini, const BKCPath *path, BKDword flags)
{
  if (path->root)
    {
      return bkcDocPrint (ini);
    }

  const BKSection *section = bkcFindWhole (ini, path);
  if (section)
    {
      return bkcSectionPrint (section, flags);
    }

  const BKChar *value
      = path->key ? bkGet (ini, path->section, path->key) : NULL;

  return value ? bkcPrintLine (value) : BKC_EXIT_MISSING;
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
      for (BKSize i = 0; result == BKC_EXIT_OK && i < bkSectionSize (section);
           ++i)
        {
          result = bkcPrintLine (bkSectionKey (section, i));
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

/* Sets or deletes, then prints the document or saves it to its file. A
 * missing target of a delete leaves the file as it is. */
static BKCExit
bkcEdit (BKIni *ini, const BKCArgs *args, const BKCPath *path)
{
  BKCExit result = BKC_EXIT_OK;
  if (args->mode == BKC_MODE_SET)
    {
      if (bkSet (ini, path->section, path->key, args->value) != BK_OK)
        {
          bkcError ("bikini: %s\n", bkError (ini)->message);
          return BKC_EXIT_ERROR;
        }
    }
  else if (!(path->whole && bkSectionRemove (ini, path->whole))
           && !(path->key && bkRemove (ini, path->section, path->key)))
    {
      result = BKC_EXIT_MISSING;
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
  BKCPath path = { NULL, BK_FALSE, NULL, NULL, NULL, NULL };
  BKCExit result = bkcParsePath (args->query, &path);
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

  result = bkcLoadAll (ini, args);
  if (result == BKC_EXIT_OK)
    {
      switch (args->mode)
        {
        case BKC_MODE_QUERY:
          result = bkcQuery (ini, &path, args->flags);
          break;
        case BKC_MODE_KEYS:
          result = bkcKeys (ini, &path);
          break;
        case BKC_MODE_SET:
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

  BKCArgs args = { BK_FNONE, BKC_MODE_QUERY, BK_FALSE, NULL, ".", NULL, 0 };
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
