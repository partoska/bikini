Bikini (bikini)

Installed files:
  /usr/bin/bikini                  The bikini command-line tool.
  /usr/lib/libbikini.a             The INI library, with its pkg-config file.
  /usr/include/bikini.h            The library headers.

Quick start:
  bikini server.port app.ini       Print one value.
  bikini server app.ini            Print one section, as INI text.
  bikini -k . app.ini              List the sections.
  bikini -w -s 9090 server.port app.ini
                                   Set a value in place.
  cat app.ini | bikini server.host Read from standard input.

Exit status: 0 found, 1 not found, 2 usage or input error.

Run "bikini -h" for full usage, or see https://lab.partoska.com/bikini.

Bikini is licensed under the MIT License.
