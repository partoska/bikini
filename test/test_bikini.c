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

#include "bikini.h"
#include "bktest.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Helpers
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef struct Buffer
{
  BKChar data[4096];
  BKSize length;
  BKSize limit;
} Buffer;

static BKSize
writeBuffer (BKVoid *user, const BKChar *data, BKSize size)
{
  Buffer *buffer = user;
  if (buffer->limit && buffer->length + size > buffer->limit)
    {
      return 0;
    }

  memcpy (buffer->data + buffer->length, data, size);
  buffer->length += size;
  buffer->data[buffer->length] = '\0';

  return size;
}

static const BKChar *
serialize (BKIni *ini)
{
  static Buffer buffer;
  memset (&buffer, 0, sizeof (buffer));
  BK_ASSERT_EQ (bkWrite (ini, writeBuffer, &buffer), BK_OK);

  return buffer.data;
}

static BKIni *
parse (BKDword flags, const BKChar *text, BKStatus expected)
{
  BKIni *ini = bkInit (flags);
  BK_ASSERT_EQ (bkParse (ini, text), expected);

  return ini;
}

/* Expects text to fail with the status, line and message. */
static void
expectError (const BKChar *text, BKStatus status, BKSize line,
             const BKChar *message)
{
  BKIni *ini = parse (BK_FICASE, text, status);
  const BKError *error = bkError (ini);
  BK_ASSERT_EQ (error->status, status);
  BK_ASSERT_EQ (error->line, line);
  BK_ASSERT_STR_EQ (error->message, message);
  if (!bkStrEq (error->message, message))
    {
      fprintf (stderr, "\n    got: %s", error->message);
    }
  bkDestroy (ini);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Tests
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void
testParseBasic (void)
{
  BKIni *ini = parse (BK_FNONE,
                      "\xef\xbb\xbf; Don't Panic\r\n"
                      "[guide]\r\n"
                      "earth = Mostly harmless\r\n"
                      "  # share and enjoy\n"
                      "answer:42\n"
                      "\n"
                      "[towel]\n"
                      "where=   always know   \n"
                      "lost =\n"
                      "url = http://guide/?sector=ZZ9\n",
                      BK_OK);

  BK_ASSERT_EQ (bkError (ini)->status, BK_OK);
  BK_ASSERT_STR_EQ (bkError (ini)->message, "");
  BK_ASSERT_EQ (bkSectionCount (ini), 2);
  BK_ASSERT_STR_EQ (bkGet (ini, "guide", "earth"), "Mostly harmless");
  BK_ASSERT_STR_EQ (bkGet (ini, "guide", "answer"), "42");
  BK_ASSERT_STR_EQ (bkGet (ini, "towel", "where"), "always know");
  BK_ASSERT_STR_EQ (bkGet (ini, "towel", "lost"), "");
  BK_ASSERT_STR_EQ (bkGet (ini, "towel", "url"), "http://guide/?sector=ZZ9");
  BK_ASSERT (bkGet (ini, "towel", "earth") == NULL);
  BK_ASSERT (bkGet (ini, "magrathea", "earth") == NULL);

  bkDestroy (ini);
}

static void
testCase (void)
{
  const BKChar *text = "[Bridge]\nCAPTAIN = Picard\n";

  // Case-sensitive by default.
  BKIni *ini = parse (BK_FNONE, text, BK_OK);
  BK_ASSERT_STR_EQ (bkGet (ini, "Bridge", "CAPTAIN"), "Picard");
  BK_ASSERT (bkGet (ini, "Bridge", "captain") == NULL);
  BK_ASSERT (bkGet (ini, "bridge", "CAPTAIN") == NULL);
  bkDestroy (ini);

  // Case-insensitive: matched either way, kept as written.
  ini = parse (BK_FICASE, text, BK_OK);
  BK_ASSERT_STR_EQ (bkGet (ini, "bridge", "captain"), "Picard");
  BK_ASSERT_STR_EQ (bkGet (ini, "BRIDGE", "Captain"), "Picard");
  BK_ASSERT_STR_EQ (bkKeyAt (bkSectionAt (ini, 0), 0), "CAPTAIN");
  BK_ASSERT_EQ (bkSet (ini, "bridge", "captain", "Number One"), BK_OK);
  BK_ASSERT_EQ (bkKeyCount (bkSectionFind (ini, "BRIDGE")), 1);
  BK_ASSERT_STR_EQ (bkGet (ini, "Bridge", "CAPTAIN"), "Number One");
  bkDestroy (ini);
}

static void
testContinuation (void)
{
  BKIni *ini = parse (BK_FNONE,
                      "[vogon]\n"
                      "poem = Oh freddled gruntbuggly\n"
                      "  thy micturations\n"
                      "  ; not part of it, sadly\n"
                      "\tare to me\n"
                      "verdict = painful\n"
                      "  [babel]\n"
                      "fish = yellow\n",
                      BK_OK);

  BK_ASSERT_STR_EQ (bkGet (ini, "vogon", "poem"),
                    "Oh freddled gruntbuggly\nthy micturations\nare to me");
  BK_ASSERT_STR_EQ (bkGet (ini, "vogon", "verdict"), "painful");
  // An indented header still starts a section.
  BK_ASSERT_STR_EQ (bkGet (ini, "babel", "fish"), "yellow");

  bkDestroy (ini);

  // Blank lines inside a value are kept, trailing ones are not, and comments
  // between them do not count.
  ini = parse (BK_FNONE,
               "[holly]\n"
               "gap = IQ\n"
               "\n"
               "\t\n"
               "  ; comment\n"
               "  6000\n"
               "\n"
               "\n"
               "lead =\n"
               "\n"
               "  senile\n"
               "tail = dementia\n"
               "\t\n"
               "\n",
               BK_OK);

  BK_ASSERT_STR_EQ (bkGet (ini, "holly", "gap"), "IQ\n\n\n6000");
  BK_ASSERT_STR_EQ (bkGet (ini, "holly", "lead"), "\n\nsenile");
  BK_ASSERT_STR_EQ (bkGet (ini, "holly", "tail"), "dementia");

  bkDestroy (ini);
}

static void
testErrors (void)
{
  expectError ("answer = 42\n", BK_ESYNTAX, 1,
               "line 1: key outside of any section");
  expectError ("[vogon]\nresistance is useless\n", BK_ESYNTAX, 2,
               "line 2: expected key = value");
  expectError ("[heart]\n = of gold\n", BK_ESYNTAX, 2, "line 2: empty key");
  expectError ("[zaphod\n", BK_ESYNTAX, 1, "line 1: malformed section header");
  expectError ("[zaphod] beeblebrox\n", BK_ESYNTAX, 1,
               "line 1: malformed section header");
  expectError ("[  ]\n", BK_ESYNTAX, 1, "line 1: empty section name");
  expectError ("[kirk]\n[spock]\n[KIRK]\n", BK_EDUPLICATE, 3,
               "line 3: duplicate section [KIRK]");
  // They multiply, but not within one section.
  expectError ("[tribbles]\ncount = 1\nCOUNT = 2\n", BK_EDUPLICATE, 3,
               "line 3: duplicate key 'COUNT' in [tribbles]");
}

static void
testPartialParse (void)
{
  BKIni *ini = parse (
      BK_FNONE, "[engine]\nwarp = 9\n[transporter]\nbeam me up\n", BK_ESYNTAX);
  BK_ASSERT_STR_EQ (bkGet (ini, "engine", "warp"), "9");
  BK_ASSERT (bkSectionFind (ini, "transporter") != NULL);

  // A later parse adds to the document and resets the error.
  BK_ASSERT_EQ (bkParse (ini, "[shuttle]\ncrew = 3\n"), BK_OK);
  BK_ASSERT_EQ (bkError (ini)->status, BK_OK);
  BK_ASSERT_EQ (bkError (ini)->line, 0);
  BK_ASSERT_STR_EQ (bkGet (ini, "shuttle", "crew"), "3");
  BK_ASSERT_EQ (bkParse (ini, "[engine]\n"), BK_EDUPLICATE);

  bkDestroy (ini);
}

static void
testSections (void)
{
  BKIni *ini = bkInit (BK_FNONE);

  BK_ASSERT_EQ (bkSet (ini, "starbug", "fuel", "low"), BK_OK);
  BK_ASSERT_EQ (bkSet (ini, "starbug", "pilot", "Lister"), BK_OK);
  BK_ASSERT_EQ (bkSet (ini, "starbug", "fuel", "empty"), BK_OK);
  BK_ASSERT_EQ (bkSectionSet (ini, "stasis"), BK_OK);
  BK_ASSERT_EQ (bkSectionSet (ini, "stasis"), BK_OK);

  BK_ASSERT_EQ (bkSectionCount (ini), 2);
  BK_ASSERT_STR_EQ (bkSectionName (bkSectionAt (ini, 0)), "starbug");
  BK_ASSERT_STR_EQ (bkSectionName (bkSectionAt (ini, 1)), "stasis");
  BK_ASSERT (bkSectionAt (ini, 2) == NULL);

  const BKSection *starbug = bkSectionFind (ini, "starbug");
  BK_ASSERT_EQ (bkKeyCount (starbug), 2);
  BK_ASSERT_STR_EQ (bkKeyAt (starbug, 0), "fuel");
  BK_ASSERT_STR_EQ (bkValueAt (starbug, 0), "empty");
  BK_ASSERT_STR_EQ (bkKeyAt (starbug, 1), "pilot");
  BK_ASSERT (bkKeyAt (starbug, 2) == NULL);
  BK_ASSERT (bkValueAt (starbug, 2) == NULL);
  BK_ASSERT_STR_EQ (bkSectionGet (starbug, "pilot"), "Lister");
  BK_ASSERT_EQ (bkKeyCount (bkSectionFind (ini, "stasis")), 0);

  bkDestroy (ini);
}

static void
testRemove (void)
{
  BKIni *ini = parse (BK_FICASE,
                      "[crew]\nlister = slob\nrimmer = smeghead\ncat = cool\n"
                      "[skutters]\nmood = grumpy\n"
                      "[stasis]\n",
                      BK_OK);

  // Removing keeps the order of the rest.
  BK_ASSERT (bkRemove (ini, "CREW", "RIMMER"));
  BK_ASSERT (!bkRemove (ini, "crew", "rimmer"));
  BK_ASSERT (!bkRemove (ini, "nowhere", "lister"));
  BK_ASSERT (bkRemove (ini, "crew", "cat"));
  BK_ASSERT_EQ (bkKeyCount (bkSectionFind (ini, "crew")), 1);
  BK_ASSERT_STR_EQ (bkGet (ini, "crew", "lister"), "slob");

  // An emptied section stays.
  BK_ASSERT (bkRemove (ini, "skutters", "mood"));
  BK_ASSERT (bkSectionFind (ini, "skutters") != NULL);

  BK_ASSERT (bkSectionRemove (ini, "SKUTTERS"));
  BK_ASSERT (!bkSectionRemove (ini, "skutters"));
  BK_ASSERT_EQ (bkSectionCount (ini), 2);
  BK_ASSERT_STR_EQ (serialize (ini), "[crew]\n"
                                     "lister = slob\n"
                                     "\n"
                                     "[stasis]\n");

  // Removed names can be added again, as a hologram if need be.
  BK_ASSERT_EQ (bkSet (ini, "skutters", "mood", "happy"), BK_OK);
  BK_ASSERT_EQ (bkSet (ini, "crew", "rimmer", "hologram"), BK_OK);
  BK_ASSERT (bkSectionRemove (ini, "stasis"));
  BK_ASSERT (bkSectionRemove (ini, "crew"));
  BK_ASSERT_STR_EQ (serialize (ini), "[skutters]\nmood = happy\n");

  BK_ASSERT (!bkRemove (NULL, "skutters", "mood"));
  BK_ASSERT (!bkRemove (ini, NULL, "mood"));
  BK_ASSERT (!bkRemove (ini, "skutters", NULL));
  BK_ASSERT (!bkSectionRemove (NULL, "skutters"));
  BK_ASSERT (!bkSectionRemove (ini, NULL));

  bkDestroy (ini);
}

static void
testSetRejects (void)
{
  static const BKChar *const keys[]
      = { "", "a=b", "a:b", "a\nb", " a", "a ", "[a", ";a", "#a" };

  BKIni *ini = bkInit (BK_FNONE);
  for (BKSize i = 0; i < BK_COUNTOF (keys); ++i)
    {
      BK_ASSERT_EQ (bkSet (ini, "bridge", keys[i], "engage"), BK_EARG);
    }
  BK_ASSERT_STR_EQ (bkError (ini)->message, "invalid key in [bridge]");

  BK_ASSERT_EQ (bkSet (ini, "bridge", "order", "engage\r"), BK_EARG);
  BK_ASSERT_STR_EQ (bkError (ini)->message,
                    "invalid value for 'order' in [bridge]");
  BK_ASSERT_EQ (bkSet (ini, "bridge", "order", NULL), BK_EARG);
  BK_ASSERT_EQ (bkSet (ini, "", "order", "engage"), BK_EARG);
  BK_ASSERT_EQ (bkSet (ini, "ten]forward", "order", "tea"), BK_EARG);
  BK_ASSERT_STR_EQ (bkError (ini)->message, "invalid section name");
  BK_ASSERT_EQ (bkSectionSet (ini, "ten\nforward"), BK_EARG);
  BK_ASSERT_EQ (bkSet (NULL, "bridge", "order", "engage"), BK_EARG);
  BK_ASSERT_EQ (bkSectionCount (ini), 0);

  bkDestroy (ini);
}

static void
testWrite (void)
{
  BKIni *ini = bkInit (BK_FNONE);
  bkSet (ini, "kryten", "guilt", "");
  bkSet (ini, "kryten", "series", "4000");
  bkSectionSet (ini, "stasis");
  bkSet (ini, "log", "entry", "stardate\n41153.7");

  BK_ASSERT_STR_EQ (serialize (ini), "[kryten]\n"
                                     "guilt = \n"
                                     "series = 4000\n"
                                     "\n"
                                     "[stasis]\n"
                                     "\n"
                                     "[log]\n"
                                     "entry = stardate\n"
                                     "\t41153.7\n");

  // A failing writer is an I/O error.
  Buffer small;
  memset (&small, 0, sizeof (small));
  small.limit = 10;
  BK_ASSERT_EQ (bkWrite (ini, writeBuffer, &small), BK_EIO);
  BK_ASSERT_STR_EQ (bkError (ini)->message, "cannot write INI text");

  bkDestroy (ini);
}

static void
testSaveAndLoad (void)
{
  // ISO C has no temporary directories; ctest runs in the build tree.
  const BKChar *path = "test_bikini_round.ini";

  // Larger than one read chunk, so loading has to grow its buffer.
  BKChar large[9000];
  memset (large, 'x', sizeof (large) - 1);
  large[sizeof (large) - 1] = '\0';

  BKIni *ini = bkInit (BK_FNONE);
  bkSet (ini, "guide", "Question", "6 * 9 = 42 : base 13");
  bkSet (ini, "guide", "multi", "so long\nand thanks");
  bkSet (ini, "guide", "gap", "for all\n\nthe fish");
  bkSet (ini, "earth", "status", "");
  bkSet (ini, "earth", "large", large);
  BK_ASSERT_EQ (bkSave (ini, path), BK_OK);
  bkDestroy (ini);

  ini = bkInit (BK_FNONE);
  BK_ASSERT_EQ (bkLoad (ini, path), BK_OK);
  BK_ASSERT_STR_EQ (bkGet (ini, "guide", "Question"), "6 * 9 = 42 : base 13");
  BK_ASSERT_STR_EQ (bkGet (ini, "guide", "multi"), "so long\nand thanks");
  BK_ASSERT_STR_EQ (bkGet (ini, "guide", "gap"), "for all\n\nthe fish");
  BK_ASSERT_STR_EQ (bkGet (ini, "earth", "status"), "");
  BK_ASSERT_STR_EQ (bkGet (ini, "earth", "large"), large);
  bkDestroy (ini);

  // An embedded NUL byte is an error, not the end of the file.
  FILE *file = fopen (path, "wb");
  BK_ASSERT (file != NULL);
  if (file)
    {
      const BKChar nul[] = "[romulan]\nship = bird\0[cloaked]\n";
      BK_ASSERT_EQ (fwrite (nul, 1, sizeof (nul) - 1, file), sizeof (nul) - 1);
      BK_ASSERT_EQ (fclose (file), 0);
      ini = bkInit (BK_FNONE);
      BK_ASSERT_EQ (bkLoad (ini, path), BK_ESYNTAX);
      BK_ASSERT_EQ (bkSectionCount (ini), 0);
      BK_ASSERT_STR_EQ (bkError (ini)->message, "NUL byte in INI text");
      bkDestroy (ini);
    }

  remove (path);
}

/* Supplies text in pieces of at most step bytes, or fails after limit. */
typedef struct Source
{
  const BKChar *data;
  BKSize length;
  BKSize offset;
  BKSize step;
  BKSize limit;   /* Fail once this many bytes were supplied; 0 for never. */
  BKBool overrun; /* Claim more bytes than asked for. */
  BKInt calls;
} Source;

static BKSize
readSource (BKVoid *user, BKChar *data, BKSize size)
{
  Source *source = user;
  ++source->calls;
  if (source->overrun)
    {
      return size + 1;
    }
  if (source->limit && source->offset >= source->limit)
    {
      return BK_READ_ERROR;
    }

  BKSize n = source->length - source->offset;
  n = n < size ? n : size;
  n = source->step && n > source->step ? source->step : n;
  memcpy (data, source->data + source->offset, n);
  source->offset += n;

  return n;
}

/* Collects written text in a caller-provided array. */
typedef struct Sink
{
  BKChar *data;
  BKSize capacity;
  BKSize length;
} Sink;

static BKSize
writeSink (BKVoid *user, const BKChar *data, BKSize size)
{
  Sink *sink = user;
  if (sink->length + size > sink->capacity)
    {
      return 0;
    }

  memcpy (sink->data + sink->length, data, size);
  sink->length += size;

  return size;
}

static Source
source (const BKChar *data, BKSize length)
{
  Source result = { data, length, 0, 0, 0, BK_FALSE, 0 };
  return result;
}

static void
testRead (void)
{
  // Small pieces, a BOM and a value split across them.
  const BKChar text[]
      = "\xef\xbb\xbf[warp]\nfactor = 9\n  point 9\n[deck]\nten: forward\n";
  Source in = source (text, sizeof (text) - 1);
  in.step = 3;
  BKIni *ini = bkInit (BK_FNONE);
  BK_ASSERT_EQ (bkRead (ini, readSource, &in), BK_OK);
  BK_ASSERT_STR_EQ (bkGet (ini, "warp", "factor"), "9\npoint 9");
  BK_ASSERT_STR_EQ (bkGet (ini, "deck", "ten"), "forward");
  BK_ASSERT_EQ (bkError (ini)->status, BK_OK);
  BK_ASSERT (in.calls > 10);

  // Appends like bkParse(), duplicates included.
  const BKChar more[] = "[q]\nis = omnipotent\n";
  in = source (more, sizeof (more) - 1);
  BK_ASSERT_EQ (bkRead (ini, readSource, &in), BK_OK);
  BK_ASSERT_EQ (bkSectionCount (ini), 3);
  in = source ("[q]\n", 4);
  BK_ASSERT_EQ (bkRead (ini, readSource, &in), BK_EDUPLICATE);
  BK_ASSERT_EQ (bkError (ini)->line, 1);
  bkDestroy (ini);

  // Round trip through bkWrite(), larger than one read chunk.
  static BKChar large[9000];
  memset (large, 'x', sizeof (large) - 1);
  ini = bkInit (BK_FNONE);
  bkSet (ini, "holodeck", "program", large);
  bkSet (ini, "holodeck", "exits", "arch\n\nnone");
  static BKChar written[16384];
  Sink out = { written, sizeof (written), 0 };
  BK_ASSERT_EQ (bkWrite (ini, writeSink, &out), BK_OK);
  bkDestroy (ini);

  in = source (written, out.length);
  ini = bkInit (BK_FNONE);
  BK_ASSERT_EQ (bkRead (ini, readSource, &in), BK_OK);
  BK_ASSERT_STR_EQ (bkGet (ini, "holodeck", "program"), large);
  BK_ASSERT_STR_EQ (bkGet (ini, "holodeck", "exits"), "arch\n\nnone");
  bkDestroy (ini);

  // Empty input is an empty document.
  in = source ("", 0);
  ini = bkInit (BK_FNONE);
  BK_ASSERT_EQ (bkRead (ini, readSource, &in), BK_OK);
  BK_ASSERT_EQ (bkSectionCount (ini), 0);
  BK_ASSERT_EQ (in.calls, 1);
  bkDestroy (ini);
}

static void
testReadErrors (void)
{
  BKIni *ini = bkInit (BK_FNONE);

  // A failing reader leaves the document unchanged, even after good text.
  const BKChar text[] = "[borg]\nresistance = futile\n";
  Source in = source (text, sizeof (text) - 1);
  in.step = 4;
  in.limit = 4;
  BK_ASSERT_EQ (bkRead (ini, readSource, &in), BK_EIO);
  BK_ASSERT_EQ (bkSectionCount (ini), 0);
  BK_ASSERT_STR_EQ (bkError (ini)->message, "cannot read INI text");
  BK_ASSERT_EQ (bkError (ini)->code, 0);

  in = source (text, sizeof (text) - 1);
  in.overrun = BK_TRUE;
  BK_ASSERT_EQ (bkRead (ini, readSource, &in), BK_EIO);
  BK_ASSERT_EQ (bkSectionCount (ini), 0);

  // An embedded NUL byte is an error, not the end of the text.
  const BKChar nul[] = "[romulan]\nship = bird\0[cloaked]\n";
  in = source (nul, sizeof (nul) - 1);
  BK_ASSERT_EQ (bkRead (ini, readSource, &in), BK_ESYNTAX);
  BK_ASSERT_EQ (bkSectionCount (ini), 0);
  BK_ASSERT_STR_EQ (bkError (ini)->message, "NUL byte in INI text");

  in = source ("smeg\n", 5);
  BK_ASSERT_EQ (bkRead (ini, readSource, &in), BK_ESYNTAX);
  BK_ASSERT_EQ (bkError (ini)->line, 1);

  BK_ASSERT_EQ (bkRead (ini, NULL, NULL), BK_EARG);
  BK_ASSERT_EQ (bkRead (NULL, readSource, &in), BK_EARG);

  bkDestroy (ini);
}

static void
testFileErrors (void)
{
  BKIni *ini = bkInit (BK_FNONE);

  BK_ASSERT_EQ (bkLoad (ini, "/nonexistent/bikini.ini"), BK_EIO);
  const BKError *error = bkError (ini);
  BK_ASSERT_EQ (error->code, ENOENT);
  BK_ASSERT_EQ (error->line, 0);
  BK_ASSERT (strncmp (error->message, "/nonexistent/bikini.ini: ", 25) == 0);

  BK_ASSERT_EQ (bkSave (ini, "/nonexistent/bikini.ini"), BK_EIO);
  BK_ASSERT_EQ (bkError (ini)->code, ENOENT);
  BK_ASSERT_EQ (bkLoad (ini, "/"), BK_EIO);
  BK_ASSERT_EQ (bkLoad (ini, NULL), BK_EARG);
  BK_ASSERT_EQ (bkParse (ini, NULL), BK_EARG);
  BK_ASSERT_EQ (bkParse (NULL, ""), BK_EARG);

  bkDestroy (ini);
}

static void
testNullSafety (void)
{
  bkDestroy (NULL);
  BK_ASSERT (bkInit ((BKDword)-1) == NULL);
  BK_ASSERT (bkError (NULL) == NULL);
  BK_ASSERT (bkGet (NULL, "a", "b") == NULL);
  BK_ASSERT (bkSectionFind (NULL, "a") == NULL);
  BK_ASSERT (bkSectionGet (NULL, "a") == NULL);
  BK_ASSERT (bkSectionName (NULL) == NULL);
  BK_ASSERT (bkGetNth (NULL, "a", 0, "b", 0) == NULL);
  BK_ASSERT (bkSectionFindNth (NULL, "a", 0) == NULL);
  BK_ASSERT (bkSectionGetNth (NULL, "a", 0) == NULL);
  BK_ASSERT_EQ (bkSectionCountOf (NULL, "a"), 0);
  BK_ASSERT_EQ (bkKeyCountOf (NULL, "a"), 0);
  BK_ASSERT_EQ (bkAdd (NULL, "a", 0, "b", "c"), BK_EARG);
  BK_ASSERT_EQ (bkSetNth (NULL, "a", 0, "b", 0, "c"), BK_EARG);
  BK_ASSERT (!bkRemoveNth (NULL, "a", 0, "b", 0));
  BK_ASSERT (!bkSectionRemoveNth (NULL, "a", 0));
  BK_ASSERT_EQ (bkKeyCount (NULL), 0);
  BK_ASSERT_EQ (bkSectionCount (NULL), 0);
  BK_ASSERT_STR_EQ (bkStatusText (BK_OK), "Success");
  BK_ASSERT_STR_EQ (bkStatusText (BK_EDUPLICATE), "Duplicate section or key");
  // The answer, but not a status.
  BK_ASSERT_STR_EQ (bkStatusText ((BKStatus)42), "Unknown error");
}

static void
testMulti (void)
{
  const BKChar *text = "[ship]\n"
                       "crew = Lister\n"
                       "crew = Rimmer\n"
                       "name = Red Dwarf\n"
                       "crew = Cat\n"
                       "[shuttle]\n"
                       "name = Starbug 1\n"
                       "[SHIP]\n"
                       "name = Blue Midget\n";

  // Repeats are errors without the flag.
  BKIni *ini = parse (BK_FNONE, text, BK_EDUPLICATE);
  bkDestroy (ini);

  ini = parse (BK_FMULTI | BK_FICASE, text, BK_OK);
  BK_ASSERT_EQ (bkSectionCount (ini), 3);
  BK_ASSERT_EQ (bkSectionCountOf (ini, "ship"), 2);
  BK_ASSERT_EQ (bkSectionCountOf (ini, "nowhere"), 0);
  const BKSection *ship = bkSectionFindNth (ini, "Ship", 0);
  BK_ASSERT (ship == bkSectionFind (ini, "ship"));
  BK_ASSERT_STR_EQ (bkSectionName (bkSectionFindNth (ini, "ship", 1)), "SHIP");
  BK_ASSERT (bkSectionFindNth (ini, "ship", 2) == NULL);
  BK_ASSERT_EQ (bkKeyCount (ship), 4);
  BK_ASSERT_EQ (bkKeyCountOf (ship, "CREW"), 3);
  BK_ASSERT_EQ (bkKeyCountOf (ship, "captain"), 0);
  BK_ASSERT_STR_EQ (bkSectionGetNth (ship, "crew", 2), "Cat");
  BK_ASSERT (bkSectionGetNth (ship, "crew", 3) == NULL);

  // Functions without an index take the first.
  BK_ASSERT_STR_EQ (bkGet (ini, "ship", "crew"), "Lister");
  BK_ASSERT_STR_EQ (bkGetNth (ini, "ship", 0, "crew", 1), "Rimmer");
  BK_ASSERT_STR_EQ (bkGetNth (ini, "ship", 1, "name", 0), "Blue Midget");
  BK_ASSERT (bkGetNth (ini, "ship", 2, "name", 0) == NULL);
  BK_ASSERT_EQ (bkSet (ini, "ship", "name", "Red Dwarf II"), BK_OK);
  BK_ASSERT_STR_EQ (bkGetNth (ini, "ship", 1, "name", 0), "Blue Midget");

  // Written in document order, repeats included.
  BK_ASSERT_STR_EQ (serialize (ini), "[ship]\n"
                                     "crew = Lister\n"
                                     "crew = Rimmer\n"
                                     "name = Red Dwarf II\n"
                                     "crew = Cat\n"
                                     "\n"
                                     "[shuttle]\n"
                                     "name = Starbug 1\n"
                                     "\n"
                                     "[SHIP]\n"
                                     "name = Blue Midget\n");

  // Setting by index only replaces.
  BK_ASSERT_EQ (bkSetNth (ini, "ship", 0, "crew", 2, "Kryten"), BK_OK);
  BK_ASSERT_STR_EQ (bkGetNth (ini, "ship", 0, "crew", 2), "Kryten");
  BK_ASSERT_EQ (bkSetNth (ini, "ship", 0, "crew", 3, "Holly"), BK_EARG);
  BK_ASSERT_STR_EQ (bkError (ini)->message,
                    "no key 'crew' at index 3 in [ship]");
  BK_ASSERT_EQ (bkSetNth (ini, "ship", 2, "crew", 0, "Holly"), BK_EARG);
  BK_ASSERT_STR_EQ (bkError (ini)->message, "no section [ship] at index 2");
  BK_ASSERT_EQ (bkSetNth (ini, "ship", 0, "crew", 0, "a\rb"), BK_EARG);

  // Adding goes after the last value of the key, or at the end.
  BK_ASSERT_EQ (bkAdd (ini, "ship", 0, "crew", "Holly"), BK_OK);
  BK_ASSERT_EQ (bkAdd (ini, "ship", 1, "crew", "Skutter"), BK_OK);
  BK_ASSERT_EQ (bkAdd (ini, "ship", 2, "name", "White Giant"), BK_OK);
  BK_ASSERT_EQ (bkAdd (ini, "ship", 4, "name", "Nowhere"), BK_EARG);
  BK_ASSERT_STR_EQ (bkError (ini)->message, "no section [ship] at index 4");
  BK_ASSERT_EQ (bkAdd (ini, "ship", 0, "[bad", "x"), BK_EARG);
  BK_ASSERT_STR_EQ (serialize (ini), "[ship]\n"
                                     "crew = Lister\n"
                                     "crew = Rimmer\n"
                                     "name = Red Dwarf II\n"
                                     "crew = Kryten\n"
                                     "crew = Holly\n"
                                     "\n"
                                     "[shuttle]\n"
                                     "name = Starbug 1\n"
                                     "\n"
                                     "[SHIP]\n"
                                     "name = Blue Midget\n"
                                     "crew = Skutter\n"
                                     "\n"
                                     "[ship]\n"
                                     "name = White Giant\n");

  // Removing by index moves the later ones down.
  BK_ASSERT (bkRemoveNth (ini, "ship", 0, "crew", 1));
  BK_ASSERT_STR_EQ (bkGetNth (ini, "ship", 0, "crew", 1), "Kryten");
  BK_ASSERT (!bkRemoveNth (ini, "ship", 0, "crew", 3));
  BK_ASSERT (bkRemove (ini, "ship", "crew"));
  BK_ASSERT_STR_EQ (bkGet (ini, "ship", "crew"), "Kryten");
  BK_ASSERT (bkSectionRemoveNth (ini, "ship", 1));
  BK_ASSERT_STR_EQ (bkGetNth (ini, "ship", 1, "name", 0), "White Giant");
  BK_ASSERT (!bkSectionRemoveNth (ini, "ship", 2));
  BK_ASSERT (bkSectionRemove (ini, "ship"));
  BK_ASSERT_STR_EQ (bkGet (ini, "ship", "name"), "White Giant");
  BK_ASSERT_EQ (bkSectionCountOf (ini, "ship"), 1);

  // Setting a section keeps one, adding makes one more.
  BK_ASSERT_EQ (bkSectionSet (ini, "ship"), BK_OK);
  BK_ASSERT_EQ (bkSectionCountOf (ini, "ship"), 1);
  BK_ASSERT_EQ (bkSectionAdd (ini, "ship"), BK_OK);
  BK_ASSERT_EQ (bkSectionCountOf (ini, "ship"), 2);
  BK_ASSERT_EQ (bkKeyCount (bkSectionFindNth (ini, "ship", 1)), 0);
  BK_ASSERT_EQ (bkSectionAdd (ini, "ship]"), BK_EARG);
  bkDestroy (ini);

  // Without the flag, adding may not repeat.
  ini = bkInit (BK_FNONE);
  BK_ASSERT_EQ (bkAdd (ini, "ship", 0, "crew", "Lister"), BK_OK);
  BK_ASSERT_EQ (bkAdd (ini, "ship", 0, "crew", "Rimmer"), BK_EARG);
  BK_ASSERT_STR_EQ (bkError (ini)->message,
                    "repeated key 'crew' in [ship] needs BK_FMULTI");
  BK_ASSERT_EQ (bkAdd (ini, "ship", 1, "crew", "Rimmer"), BK_EARG);
  BK_ASSERT_STR_EQ (bkError (ini)->message,
                    "repeated section [ship] needs BK_FMULTI");
  BK_ASSERT_EQ (bkSectionAdd (ini, "ship"), BK_EARG);
  BK_ASSERT_EQ (bkSectionAdd (ini, "bay"), BK_OK);
  BK_ASSERT_EQ (bkSectionCountOf (ini, "ship"), 1);
  BK_ASSERT_EQ (bkKeyCountOf (bkSectionFind (ini, "ship"), "crew"), 1);
  bkDestroy (ini);

  // A later text adds repeats too.
  ini = parse (BK_FMULTI, "[a]\nk = 1\n", BK_OK);
  BK_ASSERT_EQ (bkParse (ini, "[a]\nk = 2\nk = 3\n"), BK_OK);
  BK_ASSERT_EQ (bkSectionCountOf (ini, "a"), 2);
  BK_ASSERT_STR_EQ (bkGetNth (ini, "a", 1, "k", 1), "3");
  bkDestroy (ini);
}

static void
testDelimiter (void)
{
  BKIni *ini
      = parse (BK_FMULTI, "[ls]\nArg = -l\nArg = -a\nDir: /opt\n", BK_OK);

  BK_ASSERT_EQ (bkSetDelimiter (ini, "="), BK_OK);
  const BKChar *tight = "[ls]\nArg=-l\nArg=-a\nDir=/opt\n";
  BK_ASSERT_STR_EQ (serialize (ini), tight);
  BK_ASSERT_EQ (bkSetDelimiter (ini, "\t: "), BK_OK);
  BK_ASSERT_STR_EQ (serialize (ini),
                    "[ls]\nArg\t: -l\nArg\t: -a\nDir\t: /opt\n");

  // Every accepted delimiter reads back.
  BKIni *again = parse (BK_FMULTI, serialize (ini), BK_OK);
  BK_ASSERT_STR_EQ (bkGetNth (again, "ls", 0, "Arg", 1), "-a");
  BK_ASSERT_STR_EQ (bkGet (again, "ls", "Dir"), "/opt");
  bkDestroy (again);

  static const BKChar *const bad[]
      = { "", " ", "==", "=:", "-", "= x", "\n=", "=\r", "\f=" };
  for (BKSize i = 0; i < BK_COUNTOF (bad); ++i)
    {
      BK_ASSERT_EQ (bkSetDelimiter (ini, bad[i]), BK_EARG);
    }
  BK_ASSERT_STR_EQ (bkError (ini)->message, "invalid delimiter");
  BK_ASSERT_EQ (bkSetDelimiter (ini, NULL), BK_EARG);
  BK_ASSERT_EQ (bkSetDelimiter (NULL, "="), BK_EARG);
  // A failed call keeps the delimiter.
  BK_ASSERT_STR_EQ (serialize (ini),
                    "[ls]\nArg\t: -l\nArg\t: -a\nDir\t: /opt\n");

  bkDestroy (ini);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Main
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

BKInt
main (void)
{
  BK_SUITE ("bikini");
  BK_RUN (testParseBasic);
  BK_RUN (testCase);
  BK_RUN (testContinuation);
  BK_RUN (testErrors);
  BK_RUN (testPartialParse);
  BK_RUN (testSections);
  BK_RUN (testRemove);
  BK_RUN (testSetRejects);
  BK_RUN (testWrite);
  BK_RUN (testSaveAndLoad);
  BK_RUN (testRead);
  BK_RUN (testReadErrors);
  BK_RUN (testFileErrors);
  BK_RUN (testMulti);
  BK_RUN (testDelimiter);
  BK_RUN (testNullSafety);
  BK_SUMMARY ();
}
