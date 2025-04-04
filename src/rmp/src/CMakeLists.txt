###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, The Regents of the University of California
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
###############################################################################

include("openroad")

project(rmp)

swig_lib(NAME      rmp
         NAMESPACE rmp
         I_FILE    rmp.i
         SCRIPTS   rmp.tcl
)

target_sources(rmp
  PRIVATE
    blifParser.cpp
    blif.cpp
    Restructure.cpp
    MakeRestructure.cpp
)

messages(
TARGET rmp
OUTPUT_DIR ..
)

target_include_directories(rmp
  PUBLIC
    ../include
  PRIVATE
    .
 )

target_link_libraries(rmp 
  PRIVATE
    odb
    dbSta
    OpenSTA
    rsz
    utl_lib
    ${ABC_LIBRARY}
 )

add_library(rmp_abc_library 
  abc_library_factory.cpp
  logic_extractor.cpp
  logic_cut.cpp
  delay_optimization_strategy.cpp
  zero_slack_strategy.cpp
)

target_include_directories(rmp_abc_library
  PUBLIC
    ../include
  PRIVATE
    .
)

target_link_libraries(rmp_abc_library
  PUBLIC
    OpenSTA
    dbSta_lib
    libabc
    utl_lib
    ${ABC_LIBRARY}
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          rmp_py
           NAMESPACE     rmp
           LANGUAGE      python
           I_FILE        rmp-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/../include
                         ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/rmp_py.py
  )

  target_include_directories(rmp_py
    PUBLIC
      ../include
  )

  target_link_libraries(rmp_py
    PUBLIC
      rmp
      odb
      dbSta
      OpenSTA
      rsz
  )

endif()
