# /*******************************************************************************
# Copyright Intel Corporation.
# This software and the related documents are Intel copyrighted materials, and your use of them
# is governed by the express license under which they were provided to you (License).
# Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
# or transmit this software or the related documents without Intel's prior written permission.
# This software and the related documents are provided as is, with no express or implied warranties,
# other than those that are expressly stated in the License.
#
# *******************************************************************************/

cmake_minimum_required(VERSION 3.0.2)

find_package(Git)

include(GNUInstallDirs)
include(ExternalProject)

set(PROJECT_NAME TCC_TOOLS)

project(${PROJECT_NAME})

set(MAJOR_VERSION 2022)
set(MINOR_VERSION 1)
set(PATCH_VERSION 0)

set(PROJ_VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})

if(NOT GIT_FOUND OR NOT EXISTS "${CMAKE_SOURCE_DIR}/.git")
  add_definitions(-DGIT_COMMIT_HASH=local_build)
else()
  EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} --git-dir=${CMAKE_SOURCE_DIR}/.git rev-parse HEAD OUTPUT_VARIABLE GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE)
  SET(REVISION ${GIT_HASH} CACHE STRING "git short sha" FORCE)
  add_definitions(-DGIT_COMMIT_HASH=${REVISION})
endif()

EXECUTE_PROCESS(COMMAND date --rfc-3339=seconds OUTPUT_VARIABLE CURRENT_DATE
OUTPUT_STRIP_TRAILING_WHITESPACE)
SET(DATE ${CURRENT_DATE} CACHE STRING "current date" FORCE)
add_definitions(-DCURRENT_DATE=${DATE})

add_definitions(-DTCC_VER_MAJOR=${MAJOR_VERSION})
add_definitions(-DTCC_VER_MINOR=${MINOR_VERSION})
add_definitions(-DTCC_VER_PATCH=${PATCH_VERSION})
add_definitions(-DTCC_BUILD_ID=${BUILD_ID})

option(HOST_STRUCTURE "Set package structure for host usage" OFF)
option(TCC_LOGGING_DISABLED "Exclude logging from sources" OFF)

#-------------------------------------------------------------------------------
# Default permissions
#-------------------------------------------------------------------------------

set(SCRIPT_INSTALL_PERMISSIONS
    OWNER_WRITE OWNER_READ OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE)

#-------------------------------------------------------------------------------
# Setup install path
#-------------------------------------------------------------------------------

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "/opt/intel/tcc_tools" CACHE PATH "..." FORCE)
endif()

set(CMAKE_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")

if(HOST_STRUCTURE)
  set(BASE_INSTALL_PREFIX "." CACHE PATH "Location for Linux non common files")
else(HOST_STRUCTURE)
  set(BASE_INSTALL_PREFIX "share/tcc_tools" CACHE PATH "Location for Linux non common files")
endif(HOST_STRUCTURE)

set(BINARIES_INSTALL_PREFIX "bin" CACHE PATH "..." FORCE)
# Yocto and Installation script use lib64 as default library path
set(CMAKE_INSTALL_LIBDIR "lib64" CACHE PATH "Location to install library files" FORCE)
set(LICENCE_INSTALL_PREFIX  "${BASE_INSTALL_PREFIX}/licensing" CACHE PATH "..." FORCE)
set(CONFIG_INSTALL_PREFIX   "${BASE_INSTALL_PREFIX}/config" CACHE PATH "..." FORCE)
set(DOC_INSTALL_PREFIX      "${BASE_INSTALL_PREFIX}/documentation/api" CACHE PATH "..." FORCE)
set(TOOLS_INSTALL_PREFIX    "${BASE_INSTALL_PREFIX}/tools" CACHE PATH "..." FORCE)
set(SAMPLES_INSTALL_PREFIX  "${BASE_INSTALL_PREFIX}/samples" CACHE PATH "..." FORCE)
set(SOURCES_INSTALL_PREFIX  "${BASE_INSTALL_PREFIX}/sources" CACHE PATH "..." FORCE)
set(SCRIPTS_INSTALL_PREFIX  "${BASE_INSTALL_PREFIX}/scripts" CACHE PATH "..." FORCE)
set(TARGET_INSTALL_PREFIX  "${BASE_INSTALL_PREFIX}/target" CACHE PATH "..." FORCE)

set(VALIDATION_INSTALL_PREFIX  "${TOOLS_INSTALL_PREFIX}/demo" CACHE PATH "..." FORCE)

#Add documentation to main target
set(ADD_ALL ALL)

#-------------------------------------------------------------------------------
# Compiler options
#-------------------------------------------------------------------------------

if(CMAKE_COMPILER_IS_GNUCC)
  # Strip full path in the binaries for the Release
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} \
    -fdebug-prefix-map=${CMAKE_SOURCE_DIR}/= \
    -ffile-prefix-map=${CMAKE_SOURCE_DIR}/= ")

  set(CMAKE_CXX_FLAGS_RELEASE
    "${CMAKE_CXX_FLAGS_RELEASE} \
    -fdebug-prefix-map=${CMAKE_SOURCE_DIR}/= \
    -ffile-prefix-map=${CMAKE_SOURCE_DIR}/= ")


  # Common flags
  set(SECURITY_COMPILATION_FLAGS
    "-fPIE -fPIC -pie")

  set(C_SECURITY_COMPILATION_FLAGS
    "-fno-strict-overflow \
    -fno-delete-null-pointer-checks \
    -fwrapv \
    -Wformat=2 \
    -D_FORTIFY_SOURCE=2 \
    -fstack-protector-strong \
    ${SECURITY_COMPILATION_FLAGS} \
    -fstack-clash-protection \
    -mindirect-branch-register")

  set(C_INTEGER_OVERFLOW_AND_COMPARISON_FLAGS
    "-Wtype-limits \
    -Wstrict-overflow=5 \
    -fstrict-overflow \
    -Wsign-compare")

  set(SECURITY_LINK_FLAGS
     "-Wl,-z,relro \
     -Wl,-z,now \
     -z noexecstack")

  set(SECURITY_EXE_LINK_FLAGS "-Wl,-pie")

  set(CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} \
    ${SECURITY_LINK_FLAGS} \
    ${SECURITY_EXE_LINK_FLAGS}")

  set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} \
    ${SECURITY_LINK_FLAGS}")

  set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} \
    -std=gnu99 \
    ${C_SECURITY_COMPILATION_FLAGS} \
    ${C_INTEGER_OVERFLOW_AND_COMPARISON_FLAGS} \
    -Werror")

  set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} \
    -std=gnu++17 \
    ${SECURITY_COMPILATION_FLAGS}")

endif(CMAKE_COMPILER_IS_GNUCC)

if(TCC_LOGGING_DISABLED)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DTCC_LOGGING_DISABLED")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTCC_LOGGING_DISABLED")
endif()

if(DEFINED NO_TCC_MEASUREMENT)
  add_definitions(-DNO_TCC_MEASUREMENT)
endif()

macro(raise_error)
	execute_process(COMMAND
	    ${CMAKE_COMMAND} -E env CLICOLOR_FORCE=1
	    ${CMAKE_COMMAND} -E cmake_echo_color --red --bold "ERROR: ${ARGN}"
	)
	message(SEND_ERROR)
endmacro(raise_error)

include(CMakeLists.cmake)

################################################################################
# Subdirs
################################################################################

add_subdirectory(src)
add_subdirectory(config)
add_subdirectory(tools)
