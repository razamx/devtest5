# /*******************************************************************************
# INTEL CONFIDENTIAL
# Copyright Intel Corporation.
# This software and the related documents are Intel copyrighted materials, and your use of them
# is governed by the express license under which they were provided to you (License).
# Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
# or transmit this software or the related documents without Intel's prior written permission.
# This software and the related documents are provided as is, with no express or implied warranties,
# other than those that are expressly stated in the License.
#
# *******************************************************************************/

option(CREATE_DSO_DB "Build data bases for DSO" ON)

#-------------------------------------------------------------------------------
# Internal compiler flags
#-------------------------------------------------------------------------------

if(CMAKE_COMPILER_IS_GNUCC)
  # Extra warning flags
  set(BASE_WARNING_FLAGS
    "-Wall \
    -Wextra \
    \
    -Waddress \
    -Wabi=12 \
    -Walloc-zero \
    -Walloca \
    -Warray-bounds \
    -Wbuiltin-macro-redefined \
    -Wcast-align=strict \
    -Wcast-qual \
    -Wcomment \
    -Wdate-time \
    -Wdisabled-optimization \
    -Wdouble-promotion \
    -Wduplicated-branches \
    -Wduplicated-cond \
    -Wendif-labels \
    -Wenum-compare \
    -Wfloat-equal \
    -Winit-self \
    -Winvalid-pch \
    -Wlogical-op \
    -Wlong-long \
    -Wmissing-field-initializers \
    -Wmissing-noreturn \
    -Wmultichar \
    -Wnormalized \
    -Wnull-dereference \
    -Woverlength-strings \
    -Wpacked \
    -Wpacked-bitfield-compat \
    -Wpointer-arith \
    -Wredundant-decls \
    -Wshadow \
    -Wshift-overflow=2 \
    -Wsign-compare \
    -Wstringop-overflow \
    -Wstringop-truncation \
    -Wsuggest-attribute=cold \
    -Wsuggest-attribute=const \
    -Wsuggest-attribute=malloc \
    -Wsuggest-attribute=noreturn \
    -Wsuggest-attribute=pure \
    -Wswitch-default \
    -Wtrampolines \
    -Wundef \
    -Wuninitialized \
    -Wunreachable-code \
    -Wunsafe-loop-optimizations \
    -Wvector-operation-performance \
    -Wvolatile-register-var \
    -Wwrite-strings")

  set(C_WARNING_FLAGS "${BASE_WARNING_FLAGS} \
    -Winline \
    -Woverride-init")

  set(CXX_WARNING_FLAGS "${BASE_WARNING_FLAGS} \
    -Wextra-semi \
    -Woverloaded-virtual \
    -Wdelete-non-virtual-dtor \
    -Wnoexcept \
    -Wnoexcept-type \
    -Wnon-virtual-dtor \
    -Wold-style-cast \
    -Wsign-promo \
    -Wstrict-null-sentinel \
    -Wsuggest-final-methods \
    -Wsuggest-final-types \
    -Wsuggest-override \
    -Wzero-as-null-pointer-constant \
    -Werror=literal-suffix \
    -Werror=register \
    -Werror=synth \
    -Werror=useless-cast \
    -Werror=write-strings")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_WARNING_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_WARNING_FLAGS}")

  # Setup flags for debug
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb3")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3")

  # Setup flags for GTEST
  set(GTEST_C_FLAGS
    "${SECURITY_COMPILATION_FLAGS} \
    -w -Wno-error -pthread")
  set(GTEST_CXX_FLAGS "${GTEST_C_FLAGS}")
endif(CMAKE_COMPILER_IS_GNUCC)

# Set up flags for clang-tidy
if(CLANG_COMPATIBLE_FLAGS)
  string(REPLACE "-fstack-clash-protection" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  string(REPLACE "-mindirect-branch-register" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} \
    -Wno-unknown-pragmas \
    -Wno-unknown-warning-option \
    -Wno-unknown-attributes \
    -Wno-missing-braces \
    -Wno-typedef-redefinition \
    -Wno-format-nonliteral")
endif()

#-------------------------------------------------------------------------------
# Package type setup
#-------------------------------------------------------------------------------

set(PACKAGE_TYPES PUBLIC NDA DEV)
set(PACKAGE_TYPE PUBLIC CACHE STRING "Set the package type")
set_property(CACHE PACKAGE_TYPE PROPERTY STRINGS ${PACKAGE_TYPES})
list(FIND PACKAGE_TYPES ${PACKAGE_TYPE} PACKAGE_TYPE_FOUND)
if(${PACKAGE_TYPE_FOUND} EQUAL -1)
    message(FATAL_ERROR "Error: ${PACKAGE_TYPE} is not a valid package type.\n"
        "Possible values are: ${PACKAGE_TYPES}")
endif()
string(TOLOWER ${PACKAGE_TYPE} LOWER_PACKAGE_TYPE)
function(add_subdirectory_check DIR_NAME)
    foreach(package IN LISTS ARGN)
        if(${PACKAGE_TYPE} STREQUAL ${package})
            add_subdirectory(${DIR_NAME})
            break()
        endif()
    endforeach()
endfunction(add_subdirectory_check)

# Set target platform
set(PUBLIC_PLATFORMS EHL TGL-U TGL-H)

if(PACKAGE_TYPE STREQUAL "PUBLIC")
  set(PLATFORMS "${PUBLIC_PLATFORMS}")
elseif(PACKAGE_TYPE STREQUAL "NDA")
  set(PLATFORMS "${PUBLIC_PLATFORMS};${NDA_PLATFORMS}")
endif()

MESSAGE(STATUS "PLATFORMS: ${PLATFORMS}")

#-------------------------------------------------------------------------------
# Macro for sample sources install
#-------------------------------------------------------------------------------

macro(install_folder_with_sources DEST_PREFIX NAME)
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src")
    install (DIRECTORY src DESTINATION ${DEST_PREFIX}/${NAME}
    PATTERN __pycache__* EXCLUDE)
  endif()
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include")
    install (DIRECTORY include DESTINATION ${DEST_PREFIX}/${NAME})
  endif()
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Makefile")
    install (FILES Makefile DESTINATION ${DEST_PREFIX}/${NAME})
  endif()
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    install (FILES README.md DESTINATION ${DEST_PREFIX}/${NAME})
  endif()
endmacro(install_folder_with_sources)

macro(install_sample_sources NAME)
  install_folder_with_sources(${SAMPLES_INSTALL_PREFIX} ${NAME})
endmacro(install_sample_sources)

macro(install_demo_sources NAME)
  install_folder_with_sources(${VALIDATION_INSTALL_PREFIX}/workloads/src ${NAME})
endmacro(install_demo_sources)

#-------------------------------------------------------------------------------
# Source package
#-------------------------------------------------------------------------------

if(HOST_STRUCTURE)
  install(FILES CMakeLists.txt
    DESTINATION ${SOURCES_INSTALL_PREFIX}
  )

  install(DIRECTORY config
    DESTINATION ${SOURCES_INSTALL_PREFIX}
  )

  install(DIRECTORY include
    DESTINATION ${SOURCES_INSTALL_PREFIX}
  )

  install(DIRECTORY src
    DESTINATION ${SOURCES_INSTALL_PREFIX}
    PATTERN "mock" EXCLUDE
    PATTERN "*test*" EXCLUDE
    PATTERN "CMakeLists.cmake" EXCLUDE
  )

  install(SCRIPT "replace.cmake")
endif()

#-------------------------------------------------------------------------------
# Create version file
#-------------------------------------------------------------------------------

if(REVISION)
  file(WRITE ${CMAKE_INSTALL_PREFIX}/${BASE_INSTALL_PREFIX}/version.txt "${DATE}\n")
  file(APPEND ${CMAKE_INSTALL_PREFIX}/${BASE_INSTALL_PREFIX}/version.txt "${REVISION}\n")
  file(APPEND ${CMAKE_INSTALL_PREFIX}/${BASE_INSTALL_PREFIX}/version.txt "${PROJ_VERSION}\n")
  if(PACKAGE_TYPE STREQUAL "NDA")
    file(APPEND ${CMAKE_INSTALL_PREFIX}/${BASE_INSTALL_PREFIX}/version.txt "for Yocto 3.3\n")
  endif()
endif()

################################################################################
# TESTS
################################################################################

enable_testing()

option(BUILD_TESTS "Build test programs" ON)
option(COVERAGE "Add coverage flags" OFF)

set(TESTS_INSTALL_PREFIX "tests" CACHE PATH "..." FORCE)
set(TESTS_BIN_INSTALL_PREFIX "${TESTS_INSTALL_PREFIX}/bin")

set(ALL_ROBOT_TESTS_LIST ${CMAKE_BINARY_DIR}/tests/all_robot_tests_list.txt)

file(REMOVE ${ALL_ROBOT_TESTS_LIST}) #remove list crated for previsious cmake processing

# This macro used to install dev files to test package
macro (install_dev)
  if(BUILD_TESTS)
    set(options OPTIONAL FAST)
    set(oneValueArgs DESTINATION)
    set(multiValueArgs PROGRAMS)
    # Prepare lists of robot tests with source and destination paths requared for tests lists generations
    cmake_parse_arguments(MY "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(DEFINED MY_DESTINATION AND NOT ${MY_DESTINATION} MATCHES "${TESTS_INSTALL_PREFIX}*")
      message(FATAL_ERROR "install_dev should install only to ${TESTS_INSTALL_PREFIX}*. You try install to ${MY_DESTINATION}")
    endif()

    install(${ARGN})
    foreach(MY_PROGRAM ${MY_PROGRAMS})
      get_filename_component(MY_PROGRAM_FILE_NAME ${MY_PROGRAM} NAME)
      if(${MY_PROGRAM_FILE_NAME} MATCHES "test_[^/]*\\.robot")
        string(REGEX REPLACE "^${CMAKE_BINARY_DIR}/*" "" MY_INSTALL_PATH ${CMAKE_INSTALL_PREFIX})
        set(PATH_TO_INSTALLED_ROBOT_TEST ${MY_INSTALL_PATH}/${MY_DESTINATION}/${MY_PROGRAM_FILE_NAME})
        set(PATH_TO_ROBOT_TEST ${CMAKE_CURRENT_SOURCE_DIR}/${MY_PROGRAM})
        file(APPEND
          ${ALL_ROBOT_TESTS_LIST}
          "${PATH_TO_ROBOT_TEST} ${PATH_TO_INSTALLED_ROBOT_TEST}\n"
        )
      endif()
    endforeach()
  endif(BUILD_TESTS)
endmacro(install_dev)

#-------------------------------------------------------------------------------
# gtest settings
#-------------------------------------------------------------------------------

if(BUILD_TESTS)
    set(GMOCK_VERSION "1.8.0")
    set(GMOCK_DIR "${CMAKE_CURRENT_BINARY_DIR}/gmock-${GMOCK_VERSION}")
    set(GMOCK_FILE_DIR "gmock-${GMOCK_VERSION}/src/googletest_github-build/googlemock/")
    ExternalProject_Add(googletest_github
        GIT_REPOSITORY    https://github.com/google/googletest.git
        GIT_TAG           release-1.8.0
        #URL https://github.com/google/googletest/archive/release-1.8.0.zip
        #URL_HASH SHA1=667f873ab7a4d246062565fad32fb6d8e203ee73
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                   -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
                   -DCMAKE_CXX_FLAGS=${GTEST_CXX_FLAGS}
                   -DCMAKE_C_FLAGS=${GTEST_C_FLAGS}
                   -DBUILD_GMOCK=ON
                   -DBUILD_GTEST=ON
        PREFIX ${GMOCK_DIR}
        # Disable update step
        UPDATE_COMMAND ""
        # Disable install step
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS "${GMOCK_FILE_DIR}gtest/libgtest_main.a"
        BUILD_BYPRODUCTS "${GMOCK_FILE_DIR}gtest/libgtest.a"
        BUILD_BYPRODUCTS "${GMOCK_FILE_DIR}libgmock_main.a"
        BUILD_BYPRODUCTS "${GMOCK_FILE_DIR}libgmock.a"
    )
    ExternalProject_Get_Property(googletest_github source_dir)
    ExternalProject_Get_Property(googletest_github binary_dir)

    include_directories(SYSTEM ${source_dir}/googletest/include)
    add_library(gtest STATIC IMPORTED)
    set_property(TARGET gtest PROPERTY IMPORTED_LOCATION ${binary_dir}/googlemock/gtest/libgtest.a)
    add_dependencies(gtest googletest_github)
    add_library(gtest_main STATIC IMPORTED)
    set_property(TARGET gtest_main PROPERTY IMPORTED_LOCATION ${binary_dir}/googlemock/gtest/libgtest_main.a)
    add_dependencies(gtest_main googletest_github)
    include_directories(SYSTEM ${source_dir}/googlemock/include)
    add_library(gmock STATIC IMPORTED)
    set_property(TARGET gmock PROPERTY IMPORTED_LOCATION ${binary_dir}/googlemock/libgmock.a)
    add_dependencies(gmock googletest_github)
    add_library(gmock_main STATIC IMPORTED)
    set_property(TARGET gmock_main PROPERTY IMPORTED_LOCATION ${binary_dir}/googlemock/libgmock_main.a)
    add_dependencies(gmock_main googletest_github)
endif()

#-------------------------------------------------------------------------------
# Helpers to create tests targets
#-------------------------------------------------------------------------------

add_custom_target(host_tests)
add_custom_target(target_tests ALL)

# Helper to add test's target
macro(add_tcc_test test_type target )
  add_executable(${target} EXCLUDE_FROM_ALL ${ARGN})
  add_dependencies(${test_type} ${target})
  if(${test_type} STREQUAL target_tests)
    set_property(TARGET ${target} PROPERTY LINK_FLAGS -Wl,-rpath=$ORIGIN)
    install_dev(PROGRAMS $<TARGET_FILE:${target}> DESTINATION tests)
  else(${test_type} STREQUAL target_tests)
    #TODO not copy host test on install dirrectory on build step
    set_property(TARGET ${target} PROPERTY LINK_FLAGS -Wl,-rpath=$ORIGIN)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/tests
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${target}> ${CMAKE_INSTALL_PREFIX}/tests
    )
  endif(${test_type} STREQUAL target_tests)
endmacro(add_tcc_test)

macro(add_test_library target)
  add_library(${target} ${ARGN})
  install_dev(TARGETS ${target} DESTINATION tests)
endmacro(add_test_library)

set(
  TCC_VALGRIND_PREFIX
  "valgrind --track-origins=yes --num-callers=200 --leak-check=full --error-exitcode=123 --trace-children=yes --show-leak-kinds=all"
  CACHE PATH "..."
  FORCE
)
message("${TCC_VALGRIND_PREFIX}")

macro(add_host_test target)

  if(NOT ${target} MATCHES "((test_)|(payload_)).*_host")
    message(FATAL_ERROR "Wrong test name")
  endif()

  add_executable(${target} EXCLUDE_FROM_ALL ${ARGN})
  set_property(TARGET ${target} PROPERTY LINK_FLAGS -Wl,-rpath=$ORIGIN)
  target_compile_options(${target} PRIVATE "-pthread")
  add_dependencies(host_tests ${target})

  install_dev(
    PROGRAMS $<TARGET_FILE:${target}>
    TARGETS ${target}
    DESTINATION tests
    OPTIONAL
  )

  if(${target} MATCHES "test_.*_host")
    make_directory(${CMAKE_BINARY_DIR}/gtest_xml_results)
    add_test(
      NAME ${target}_NONE
      COMMAND bash -c "
        TCC_LOG_LEVEL=NONE \
        TCC_LOG_TO_FILE=${target}.NONE.log \
        $<TARGET_FILE:${target}> \
          --gtest_output=xml:${CMAKE_BINARY_DIR}/gtest_xml_results/${target}.NONE.xml \
      "
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      CONFIGURATIONS host_test host_test_cpp
    )
    add_test(
      NAME ${target}_TRACE
      COMMAND bash -c "
        TCC_LOG_LEVEL=TRACE \
        TCC_LOG_TO_FILE=${target}.TRACE.log \
        $<TARGET_FILE:${target}> \
          --gtest_output=xml:${CMAKE_BINARY_DIR}/gtest_xml_results/${target}.TRACE.xml \
      "
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      CONFIGURATIONS host_test host_test_cpp
    )
    add_test(
      NAME ${target}_with_valgrind_NONE
      COMMAND bash -c "
        TCC_LOG_LEVEL=NONE \
        TCC_LOG_TO_FILE=${target}.with_valgrind.NONE.log \
        ${TCC_VALGRIND_PREFIX} \
        $<TARGET_FILE:${target}> \
          --gtest_output=xml:${CMAKE_BINARY_DIR}/gtest_xml_results/${target}_with_valgrind.NONE.xml \
      "
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      CONFIGURATIONS host_test_cpp_with_valgrind
    )
    add_test(
      NAME ${target}_with_valgrind_TRACE
      COMMAND bash -c "
        TCC_LOG_LEVEL=TRACE \
        TCC_LOG_TO_FILE=${target}.with_valgrind.TRACE.log \
        ${TCC_VALGRIND_PREFIX} \
        $<TARGET_FILE:${target}> \
          --gtest_output=xml:${CMAKE_BINARY_DIR}/gtest_xml_results/${target}.with_valgrind.TRACE.xml \
      "
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      CONFIGURATIONS host_test_cpp_with_valgrind
    )
  endif()
endmacro(add_host_test)

macro(add_target_test target)
  if(NOT ${target} MATCHES "((test)|(payload))_.*_target")
    message(FATAL_ERROR "Wrong test name")
  endif()
  add_tcc_test(target_tests ${target} ${ARGN})
endmacro(add_target_test)

# gtest dependencies
# Helper library to simplify configuring host/target tests which are use gtest
add_library(gtest_depend INTERFACE)
target_link_libraries(gtest_depend
  INTERFACE
    gtest_main
    gtest
    pthread
)

#-------------------------------------------------------------------------------
# Running tests
#-------------------------------------------------------------------------------

if(BUILD_TESTS)
  add_custom_target(run_host_tests
      COMMAND TCC_COVERAGE=${TCC_COVERAGE} ${CMAKE_SOURCE_DIR}/tests/run_host_tests.sh ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_INSTALL_PREFIX}/tests
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

  add_dependencies(run_host_tests
    host_tests
  )
endif()

#-------------------------------------------------------------------------------
# Coverage
#-------------------------------------------------------------------------------

if(COVERAGE)
    execute_process(COMMAND bash -c "command -v gcovr" OUTPUT_VARIABLE GCOVR_CMD)
    if (GCOVR_CMD STREQUAL "")
        raise_error("gcovr not instaled. To fix please run:'sudo pip install gcovr'")
    endif()

    execute_process(COMMAND bash -c "command -v coverage3" OUTPUT_VARIABLE COVERAGE3_CMD)
    if (COVERAGE3_CMD STREQUAL "")
        raise_error("coverage3 not instaled. To fix please run:'sudo pip3 install coverage'")
    endif()

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -DTCC_COVERAGE_ENABLED")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -DTCC_COVERAGE_ENABLED")

    add_custom_target(collect_coverage
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/collect_coverage.sh  ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_dependencies(collect_coverage run_host_tests)

    set(TCC_COVERAGE "1")
else()
    set(TCC_COVERAGE "")
endif()

#-------------------------------------------------------------------------------
# Create symlink
#-------------------------------------------------------------------------------

macro(install_symlink file_path symlink_path)
  string(REPLACE "/./" "/" tmp_file_path ${file_path})
  string(REPLACE "/./" "/" tmp_symlink_path ${symlink_path})
  install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${tmp_file_path} ${tmp_symlink_path})")
  install(CODE "message(\"-- Installed symlink: ${tmp_file_path} -> ${tmp_symlink_path}\")")
endmacro(install_symlink)

################################################################################
# Subdirs
################################################################################

add_subdirectory(doc)
add_subdirectory(benchmark)
if(BUILD_TESTS)
  add_subdirectory(tests)
endif()
add_subdirectory(samples)
add_subdirectory(scripts)
add_subdirectory(licensing)

if(HOST_STRUCTURE)
  add_subdirectory(target)
  install(DIRECTORY env DESTINATION ${BASE_INSTALL_PREFIX})
endif(HOST_STRUCTURE)

configure_file(CTestCustom.cmake ${CMAKE_BINARY_DIR}/CTestCustom.cmake COPYONLY)
