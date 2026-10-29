# Bikini Library - Small, minimal INI reader and writer.
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

add_executable(test_bikini test_bikini.c)
target_compile_options(test_bikini PRIVATE ${PL_WARNINGS})
target_link_libraries(test_bikini PRIVATE bikini)
add_test(NAME test_bikini COMMAND test_bikini)

# test_cli is a POSIX shell script; on Windows it needs one on the PATH,
# such as the sh of Git for Windows or MSYS2.
if(WITH_CLI)
    find_program(PL_SH NAMES sh bash)
    if(PL_SH)
        add_test(NAME test_cli
            COMMAND "${PL_SH}" "${CMAKE_CURRENT_SOURCE_DIR}/test_cli.sh" "$<TARGET_FILE:bikini-cli>")
    else()
        message(STATUS "No POSIX shell found, test_cli is not registered")
    endif()
endif()
