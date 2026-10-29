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

add_executable(bikini-cli src/plat.h src/plat.c src/main.c)
set_target_properties(bikini-cli PROPERTIES OUTPUT_NAME bikini)
target_compile_options(bikini-cli PRIVATE ${PL_WARNINGS} ${PL_SANITIZERS_STRICT})
target_link_libraries(bikini-cli PRIVATE bikini)

if(WIN32)
    set(BIKINI_ICON "${PROJECT_SOURCE_DIR}/assets/installer/bikini.ico")
    configure_file(src/bikini.rc.in "${CMAKE_CURRENT_BINARY_DIR}/bikini.rc" @ONLY)
    target_sources(bikini-cli PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/bikini.rc")
endif()

install(TARGETS bikini-cli RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
