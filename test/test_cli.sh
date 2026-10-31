#!/bin/sh
#
# Bikini - Small, minimal INI library and query tool.
# Copyright (C) 2026 Fabrika Charvat s.r.o. All rights reserved.
# Developed by Partoska Laboratory team, <https://lab.partoska.com>
#
# MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# You can contact the author(s) via email at ask <at> partoska.com.
#

BIKINI="$1"
FAILURES=0
COUNT=0

if [ -z "$BIKINI" ] || [ ! -x "$BIKINI" ]; then
    echo "Usage: $0 PATH/TO/bikini" >&2
    exit 2
fi

TMP=$(mktemp -d) || exit 2
trap 'rm -rf "$TMP"' EXIT

cat > "$TMP/app.ini" <<'INI'
; Don't Panic
[ship]
name = Heart of Gold
drive: improbability

[guide.entry]
earth-rating = harmless
towel = shield
	weapon
INI

cat > "$TMP/local.ini" <<'INI'
[ship]
drive = warp
alert = red

[crew]
size = 4
INI

cat > "$TMP/dots.ini" <<'INI'
[deep.thought]
the.answer = 42

[deep]
thought.the.answer = still computing
INI

cat > "$TMP/multi.ini" <<'INI'
[process]
name = ls
arg = -l
arg = -a
dir = /opt

[deep.thought]
the.answer = 42

[process]
name = sleep
arg = 30
INI

cat > "$TMP/multi2.ini" <<'INI'
[process]
name = pwd
INI

cat > "$TMP/emoji.ini" <<'INI'
[mood]
smile = 😀
family = 👨‍👩‍👧
heart = ❤️, warm 🎉

[🎉 party]
guests = 42
INI

# edit NAME [SOURCE]: a fresh copy of app.ini, or SOURCE, to edit in place
edit() {
    cp "$TMP/${2:-app}.ini" "$TMP/$1.ini"
    echo "$TMP/$1.ini"
}

# expect NAME STATUS EXPECTED-OUTPUT COMMAND...
expect() {
    name="$1"
    status="$2"
    expected="$3"
    shift 3
    COUNT=$((COUNT + 1))
    actual=$("$@" 2>/dev/null)
    code=$?
    if [ "$code" -ne "$status" ] || [ "$actual" != "$expected" ]; then
        echo "  $name: FAIL (status $code, expected $status)"
        echo "    expected: $expected"
        echo "    actual:   $actual"
        FAILURES=$((FAILURES + 1))
    else
        echo "  $name: ok"
    fi
}

expect "value from file" 0 "Heart of Gold" "$BIKINI" ship.name "$TMP/app.ini"
expect "value from stdin" 0 "improbability" sh -c "\"$BIKINI\" ship.drive < \"$TMP/app.ini\""
expect "value from pipe" 0 "improbability" sh -c "cat \"$TMP/app.ini\" | \"$BIKINI\" ship.drive -"
expect "dotted section" 0 "harmless" "$BIKINI" guide.entry.earth-rating "$TMP/app.ini"
expect "multi-line value" 0 "shield
weapon" "$BIKINI" guide.entry.towel "$TMP/app.ini"
expect "section" 0 "[ship]
name = Heart of Gold
drive = improbability" "$BIKINI" ship "$TMP/app.ini"
expect "whole document" 0 "[ship]
name = Heart of Gold
drive = improbability

[guide.entry]
earth-rating = harmless
towel = shield
	weapon" "$BIKINI" . "$TMP/app.ini"
expect "ignore case" 0 "Heart of Gold" "$BIKINI" -i SHIP.Name "$TMP/app.ini"
expect "missing key" 1 "" "$BIKINI" ship.captain "$TMP/app.ini"
expect "missing section" 1 "" "$BIKINI" crew "$TMP/app.ini"
expect "missing file" 2 "" "$BIKINI" ship.name "$TMP/magrathea.ini"
expect "syntax error" 2 "" sh -c "printf 'smeg\n' | \"$BIKINI\" ."
expect "NUL from stdin" 2 "" sh -c "printf '[romulan]\\nship = bird\\000\\n' | \"$BIKINI\" romulan.ship"
expect "unreadable stdin" 2 "" sh -c "\"$BIKINI\" . < /"

# Listing
expect "list sections" 0 "ship
guide.entry" "$BIKINI" -k . "$TMP/app.ini"
expect "list keys" 0 "earth-rating
towel" "$BIKINI" --keys guide.entry "$TMP/app.ini"
expect "list missing section" 1 "" "$BIKINI" -k crew "$TMP/app.ini"
expect "list a value" 2 "" "$BIKINI" -k ship.name "$TMP/app.ini"
expect "list empty document" 0 "" sh -c "printf '' | \"$BIKINI\" -k"

# Quoting
expect "quoted section" 0 "42" "$BIKINI" '"deep.thought"."the.answer"' "$TMP/dots.ini"
expect "quoted key" 0 "still computing" "$BIKINI" 'deep."thought.the.answer"' "$TMP/dots.ini"
expect "bare dots still split" 1 "" "$BIKINI" '"deep.thought".the.answer' "$TMP/dots.ini"
expect "quoted whole section" 0 "[deep]
thought.the.answer = still computing" "$BIKINI" '"deep"' "$TMP/dots.ini"
expect "quotes are exact" 1 "" "$BIKINI" '"deep"."thought.the"' "$TMP/dots.ini"
expect "escaped quote" 0 "9" sh -c "printf '[say \"engage\"]\\nwarp = 9\\n' | \"$BIKINI\" '\"say \\\"engage\\\"\".warp'"
expect "unterminated quote" 2 "" "$BIKINI" '"deep.thought' "$TMP/dots.ini"
expect "quote inside a name" 2 "" "$BIKINI" 'deep"thought".the' "$TMP/dots.ini"
expect "text after a quote" 2 "" "$BIKINI" '"deep"thought' "$TMP/dots.ini"
expect "empty quoted name" 2 "" "$BIKINI" '"".answer' "$TMP/dots.ini"
expect "trailing dot" 2 "" "$BIKINI" '"deep".' "$TMP/dots.ini"

# Several files
expect "merged value" 0 "warp" "$BIKINI" ship.drive "$TMP/app.ini" "$TMP/local.ini"
expect "merged order" 0 "improbability" "$BIKINI" ship.drive "$TMP/local.ini" "$TMP/app.ini"
expect "merged section" 0 "[ship]
name = Heart of Gold
drive = warp
alert = red" "$BIKINI" ship "$TMP/app.ini" "$TMP/local.ini"
expect "merged sections" 0 "ship
guide.entry
crew" "$BIKINI" -k . "$TMP/app.ini" "$TMP/local.ini"
expect "merged stdin" 0 "4" sh -c "\"$BIKINI\" crew.size \"$TMP/app.ini\" - < \"$TMP/local.ini\""
expect "merged missing file" 2 "" "$BIKINI" ship.drive "$TMP/app.ini" "$TMP/magrathea.ini"

# Editing to standard output
expect "set existing" 0 "[ship]
name = Heart of Gold
drive = warp" sh -c "\"$BIKINI\" -s warp ship.drive \"$TMP/app.ini\" | \"$BIKINI\" ship"
expect "set new section" 0 "[ship]
name = Heart of Gold
drive = improbability

[guide.entry]
earth-rating = harmless
towel = shield
	weapon

[crew]
size = 5" "$BIKINI" --set 5 crew.size "$TMP/app.ini"
expect "set dotted section" 0 "mostly harmless" sh -c "\"$BIKINI\" -s 'mostly harmless' guide.entry.earth-rating \"$TMP/app.ini\" | \"$BIKINI\" guide.entry.earth-rating"
expect "set quoted" 0 "unknown" sh -c "\"$BIKINI\" -s unknown 'deep.\"the.question\"' \"$TMP/dots.ini\" | \"$BIKINI\" 'deep.\"the.question\"'"
expect "set multi-line" 0 "stardate
41153.7" sh -c "\"$BIKINI\" -s 'stardate
41153.7' log.entry \"$TMP/app.ini\" | \"$BIKINI\" log.entry"
expect "set value like option" 0 "-1" sh -c "\"$BIKINI\" -s -1 ship.shields \"$TMP/app.ini\" | \"$BIKINI\" ship.shields"
expect "set no key" 2 "" "$BIKINI" -s 1 ship "$TMP/app.ini"
expect "set no value" 2 "" "$BIKINI" ship.drive -s
expect "set invalid key" 2 "" "$BIKINI" -s 1 'ship.[x' "$TMP/app.ini"
expect "delete key" 0 "[ship]
drive = improbability" sh -c "\"$BIKINI\" -d ship.name \"$TMP/app.ini\" | \"$BIKINI\" ship"
expect "delete section" 0 "ship" sh -c "\"$BIKINI\" -d guide.entry \"$TMP/app.ini\" | \"$BIKINI\" -k"
expect "delete section of merged" 0 "crew" sh -c "\"$BIKINI\" -d ship \"$TMP/app.ini\" \"$TMP/local.ini\" | \"$BIKINI\" -d guide.entry | \"$BIKINI\" -k"
expect "delete missing" 1 "[ship]
name = Heart of Gold
drive = improbability

[guide.entry]
earth-rating = harmless
towel = shield
	weapon" "$BIKINI" -d ship.captain "$TMP/app.ini"
expect "delete document" 2 "" "$BIKINI" -d . "$TMP/app.ini"
expect "exclusive modes" 2 "" "$BIKINI" -k -d ship "$TMP/app.ini"

# Editing in place
FILE=$(edit set)
expect "write set" 0 "[ship]
name = Heart of Gold
drive = warp" sh -c "\"$BIKINI\" -w -s warp ship.drive \"$FILE\" && \"$BIKINI\" ship \"$FILE\""
FILE=$(edit delete)
expect "write delete" 0 "ship" sh -c "\"$BIKINI\" -d -w guide.entry \"$FILE\" && \"$BIKINI\" -k . \"$FILE\""
FILE=$(edit missing)
expect "write delete missing" 1 "" "$BIKINI" -d -w ship.captain "$FILE"
COUNT=$((COUNT + 1))
# cksum, not cmp: Git for Windows on arm64 has no cmp
if [ "$(cksum < "$TMP/app.ini")" = "$(cksum < "$FILE")" ]; then
    echo "  write delete missing leaves file: ok"
else
    echo "  write delete missing leaves file: FAIL"
    FAILURES=$((FAILURES + 1))
fi
expect "write without edit" 2 "" "$BIKINI" -w ship.drive "$TMP/app.ini"
expect "write stdin" 2 "" sh -c "\"$BIKINI\" -w -s 1 ship.towels < \"$TMP/app.ini\""
expect "write two files" 2 "" "$BIKINI" -w -s 1 ship.towels "$TMP/app.ini" "$TMP/local.ini"
expect "write unwritable" 2 "" "$BIKINI" -w -s 1 ship.towels /

# Repeated sections and keys
expect "repeats without multi" 2 "" "$BIKINI" . "$TMP/multi.ini"
expect "multi value" 0 "-a" "$BIKINI" --multi 'process[0].arg[1]' "$TMP/multi.ini"
expect "multi all values" 0 "-l
-a" "$BIKINI" --multi 'process[0].arg[*]' "$TMP/multi.ini"
expect "multi across sections" 0 "ls
sleep" "$BIKINI" --multi 'process[*].name[0]' "$TMP/multi.ini"
expect "multi section" 0 "[process]
name = sleep
arg = 30" "$BIKINI" --multi 'process[1]' "$TMP/multi.ini"
expect "multi all sections" 0 "[process]
name = ls
arg = -l
arg = -a
dir = /opt

[process]
name = sleep
arg = 30" "$BIKINI" --multi 'process[*]' "$TMP/multi.ini"
expect "multi dotted names" 0 "42" "$BIKINI" --multi 'deep.thought[0].the.answer[0]' "$TMP/multi.ini"
expect "multi quoted names" 0 "42" "$BIKINI" --multi '"deep.thought"[0]."the.answer"[0]' "$TMP/multi.ini"
expect "multi missing index" 1 "" "$BIKINI" --multi 'process[2]' "$TMP/multi.ini"
expect "multi missing value" 1 "" "$BIKINI" --multi 'process[1].arg[1]' "$TMP/multi.ini"
expect "multi needs section index" 2 "" "$BIKINI" --multi process.name "$TMP/multi.ini"
expect "multi needs key index" 2 "" "$BIKINI" --multi 'process[0].name' "$TMP/multi.ini"
expect "multi invalid index" 2 "" "$BIKINI" --multi 'process[-1]' "$TMP/multi.ini"
expect "multi list sections" 0 "process[0]
deep.thought[0]
process[1]" "$BIKINI" --multi -k . "$TMP/multi.ini"
expect "multi list keys" 0 "name[0]
arg[0]
arg[1]
dir[0]" "$BIKINI" --multi -k 'process[0]' "$TMP/multi.ini"
expect "multi list all" 2 "" "$BIKINI" --multi -k 'process[*]' "$TMP/multi.ini"
expect "multi files concatenated" 0 "ls
sleep
pwd" "$BIKINI" --multi 'process[*].name[0]' "$TMP/multi.ini" "$TMP/multi2.ini"
expect "multi set" 0 "-h" sh -c "\"$BIKINI\" --multi -s -h 'process[0].arg[1]' \"$TMP/multi.ini\" | \"$BIKINI\" --multi 'process[0].arg[1]'"
expect "multi set missing" 1 "" sh -c "\"$BIKINI\" --multi -s x 'process[1].arg[1]' \"$TMP/multi.ini\" > /dev/null"
expect "multi set all" 2 "" "$BIKINI" --multi -s x 'process[0].arg[*]' "$TMP/multi.ini"
expect "multi add" 0 "[process]
name = ls
arg = -l
arg = -a
arg = -h
dir = /opt" sh -c "\"$BIKINI\" --multi --add -h 'process[0].arg' \"$TMP/multi.ini\" | \"$BIKINI\" --multi 'process[0]'"
expect "multi add section" 0 "pwd" sh -c "\"$BIKINI\" --multi --add pwd 'process[2].name' \"$TMP/multi.ini\" | \"$BIKINI\" --multi 'process[2].name[0]'"
expect "multi add beyond" 1 "" sh -c "\"$BIKINI\" --multi --add x 'process[3].name' \"$TMP/multi.ini\" > /dev/null"
expect "multi add with index" 2 "" "$BIKINI" --multi --add x 'process[0].arg[0]' "$TMP/multi.ini"
expect "add needs multi" 2 "" "$BIKINI" --add x ship.crew "$TMP/app.ini"
expect "multi delete value" 0 "-a" sh -c "\"$BIKINI\" --multi -d 'process[0].arg[0]' \"$TMP/multi.ini\" | \"$BIKINI\" --multi 'process[0].arg[*]'"
expect "multi delete all values" 0 "name[0]
dir[0]" sh -c "\"$BIKINI\" --multi -d 'process[*].arg[*]' \"$TMP/multi.ini\" | \"$BIKINI\" --multi -k 'process[0]'"
expect "multi delete section" 0 "process[0]
deep.thought[0]" sh -c "\"$BIKINI\" --multi -d 'process[1]' \"$TMP/multi.ini\" | \"$BIKINI\" --multi -k ."
expect "multi delete all sections" 0 "deep.thought[0]" sh -c "\"$BIKINI\" --multi -d 'process[*]' \"$TMP/multi.ini\" | \"$BIKINI\" --multi -k ."
expect "multi delete missing" 1 "" sh -c "\"$BIKINI\" --multi -d 'process[0].env[*]' \"$TMP/multi.ini\" > /dev/null"
FILE=$(edit multi-write multi)
expect "multi write" 0 "-l
-a
-h" sh -c "\"$BIKINI\" --multi -w --add -h 'process[0].arg' \"$FILE\" && \"$BIKINI\" --multi 'process[0].arg[*]' \"$FILE\""

# Delimiter
expect "delimiter" 0 "[process]
name=sleep
arg=30" "$BIKINI" --multi --delimiter = 'process[1]' "$TMP/multi.ini"
expect "delimiter colon" 0 "[ship]
name: Heart of Gold
drive: improbability" "$BIKINI" --delimiter ': ' ship "$TMP/app.ini"
expect "delimiter invalid" 2 "" "$BIKINI" --delimiter '->' . "$TMP/app.ini"
expect "delimiter no value" 2 "" "$BIKINI" . "$TMP/app.ini" --delimiter
FILE=$(edit delimiter multi)
expect "delimiter write" 0 "[process]
name=ls
arg=-l
arg=-a
arg=-h
dir=/opt" sh -c "\"$BIKINI\" --multi --delimiter = -w --add -h 'process[0].arg' \"$FILE\" && sed -n 1,6p \"$FILE\""

# Emoji pass through as bytes, given in files and pipes; queries name
# ASCII sections, as Windows hands arguments over in its ANSI code page.
EMOJI="$TMP/emoji.ini"
expect "emoji value" 0 "👨‍👩‍👧" "$BIKINI" mood.family "$EMOJI"
expect "emoji value from stdin" 0 "❤️, warm 🎉" sh -c "\"$BIKINI\" mood.heart < \"$EMOJI\""
expect "emoji section" 0 "[mood]
smile = 😀
family = 👨‍👩‍👧
heart = ❤️, warm 🎉" "$BIKINI" mood "$EMOJI"
expect "emoji list sections" 0 "mood
🎉 party" "$BIKINI" -k . "$EMOJI"
expect "emoji list keys" 0 "smile
family
heart" "$BIKINI" -k mood "$EMOJI"
expect "emoji round trip" 0 "$(cat "$EMOJI")" "$BIKINI" . "$EMOJI"
expect "unknown option" 2 "" "$BIKINI" --nope
expect "version" 0 "bikini 2.0.1" "$BIKINI" --version
expect "help banner" 0 "Bikini v2.0.1, <https://lab.partoska.com/bikini>
Copyright (C) 2026 Fabrika Charvat s.r.o. All rights reserved.
Licensed under the MIT License." sh -c "\"$BIKINI\" --help | sed -n '6,8p'"

echo
echo "$COUNT tests, $FAILURES failed"
[ "$FAILURES" -eq 0 ]
