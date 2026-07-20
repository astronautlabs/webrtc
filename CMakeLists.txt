cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)

set(webrtc_dir "${CMAKE_BINARY_DIR}/external/webrtc")

# Ensure node.js is installed
# -----------------------------------------------------------------------------

find_program(NODE_EXECUTABLE NAMES node)
if(NOT NODE_EXECUTABLE)
    message(FATAL_ERROR "Node.js must be installed.")
endif()

# CLion support for cmake-js
# https://github.com/cmake-js/cmake-js/issues/23#issuecomment-134265017
# -----------------------------------------------------------------------------

function(PARSE_CMAKEJS_PROPERTIES)
  # A much cleaner parser using Regex to target '-DVAR=VALUE'
  function(GET_VARIABLE INPUT_STRING VARIABLE_TO_SELECT OUTPUT_VARIABLE)
    # Matches: '-D[VARIABLE_TO_SELECT]=[capture everything up to the next single quote]'
    string(REGEX MATCH "'-D${VARIABLE_TO_SELECT}=([^']*)'" _discard "${INPUT_STRING}")

    # CMAKE_MATCH_1 contains the captured group inside the parentheses
    set("${OUTPUT_VARIABLE}" "${CMAKE_MATCH_1}" PARENT_SCOPE)
  endfunction(GET_VARIABLE)

  # Setup the command arguments dynamically
  set(CMAKE_JS_ARGS print-configure -l error)
  string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
  if (CMAKE_BUILD_TYPE_LOWER STREQUAL "debug")
    list(APPEND CMAKE_JS_ARGS "--debug")
  endif()

  # Ensure CMake can find Node
  find_program(NODE_EXECUTABLE NAMES node)
  if(NOT NODE_EXECUTABLE)
      message(FATAL_ERROR "Node.js not found! Please ensure 'node' is in your system PATH.")
  endif()

  execute_process(
      COMMAND ${NODE_EXECUTABLE} ./node_modules/cmake-js/bin/cmake-js ${CMAKE_JS_ARGS}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_VARIABLE CMAKE_JS_OUTPUT
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # Extract the variables
  get_variable("${CMAKE_JS_OUTPUT}" "CMAKE_JS_INC" LOCAL_CMAKE_JS_INC)
  get_variable("${CMAKE_JS_OUTPUT}" "CMAKE_LIBRARY_OUTPUT_DIRECTORY" LOCAL_CMAKE_LIBRARY_OUTPUT_DIRECTORY)

  # Export them to the main CMakeLists scope
  set(CMAKE_JS_INC "${LOCAL_CMAKE_JS_INC}" PARENT_SCOPE)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${LOCAL_CMAKE_LIBRARY_OUTPUT_DIRECTORY}" PARENT_SCOPE)

endfunction(PARSE_CMAKEJS_PROPERTIES)

# -----------------------------------------------------------------------------

macro(assign_bool var)
  if (${ARGN})
    set(${var} ON)
  else()
    set(${var} OFF)
  endif()
endmacro()

assign_bool(is_arm64 "y$ENV{TARGET_ARCH}" STREQUAL "yarm64")

if (APPLE AND ${is_arm64})
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE INTERNAL "" FORCE)
endif()

# Setup compiler (prefer Clang unless explicitly specified)
# -----------------------------------------------------------------------------

if (WIN32)
  if(NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER "clang-cl" CACHE STRING "")
  endif()
  if(NOT CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER "clang-cl" CACHE STRING "")
  endif()
else()
  # Do not override if the user explicitly passed CC/CXX via environment variables
  if(NOT DEFINED ENV{CC} AND NOT DEFINED ENV{CXX})
    find_program(SYSTEM_CLANG_C NAMES clang)
    find_program(SYSTEM_CLANG_CXX NAMES clang++)

    if(SYSTEM_CLANG_C AND SYSTEM_CLANG_CXX)
      set(CMAKE_C_COMPILER "${SYSTEM_CLANG_C}")
      set(CMAKE_CXX_COMPILER "${SYSTEM_CLANG_CXX}")
      message(STATUS "Found system Clang: ${SYSTEM_CLANG_CXX}")
    else()
      message(FATAL_ERROR "This project requires Clang. Please install clang and clang++ on your system.")
    endif()
  endif()
endif()

set(MODULE wrtc)
project(${MODULE})

parse_cmakejs_properties()

include(ExternalProject)
find_package(Git REQUIRED)
find_package(Threads REQUIRED)

# libc++/libc++abi
# -----------------------------------------------------------------------------
add_library(libc++ STATIC IMPORTED)
set_target_properties(libc++ PROPERTIES
    IMPORTED_LOCATION "${webrtc_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}c++${CMAKE_STATIC_LIBRARY_SUFFIX}"
)
add_library(libc++abi STATIC IMPORTED)
set_target_properties(libc++abi PROPERTIES
    IMPORTED_LOCATION "${webrtc_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}c++abi${CMAKE_STATIC_LIBRARY_SUFFIX}"
)
set(libc++_include_dir
  "${webrtc_dir}/include/webrtc/third_party/libc++/src/include"
  "${webrtc_dir}/include/webrtc/buildtools/third_party/libc++"
)
set(libc++abi_include_dir
  "${webrtc_dir}/include/webrtc/third_party/libc++abi/src/include"
)

if (NOT DEFINED ENV{TARGET_ARCH})
  set(ENV{TARGET_ARCH} "x64")
endif()

# catch2 (test framework)
# -----------------------------------------------------------------------------

ExternalProject_Add(
  project_catch2

  GIT_REPOSITORY    https://github.com/catchorg/Catch2.git
  GIT_TAG           ee1450f268dfd5c13aa8670ba97e93cabaf2e15d

  PREFIX            ${CMAKE_BINARY_DIR}/external/catch2/prefix
  TMP_DIR           ${CMAKE_BINARY_DIR}/external/catch2/tmp
  STAMP_DIR         ${CMAKE_BINARY_DIR}/external/catch2/stamp
  DOWNLOAD_DIR      ${CMAKE_BINARY_DIR}/external/catch2/download
  SOURCE_DIR        ${CMAKE_BINARY_DIR}/external/catch2/src
  BINARY_DIR        ${CMAKE_BINARY_DIR}/external/catch2/build

  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
)

ExternalProject_Get_Property(project_catch2 SOURCE_DIR)
set(catch2_install_dir ${SOURCE_DIR})

# libwebrtc
# -----------------------------------------------------------------------------

function(declare_webrtc_lib base_name)
  add_library(${base_name} STATIC IMPORTED)
  set_target_properties(${base_name} PROPERTIES
    IMPORTED_LOCATION "${webrtc_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${base_name}${CMAKE_STATIC_LIBRARY_SUFFIX}"
  )
endfunction()

declare_webrtc_lib(webrtc)
declare_webrtc_lib(builtin_video_encoder_factory)
declare_webrtc_lib(builtin_video_decoder_factory)
declare_webrtc_lib(rtc_internal_video_codecs)
declare_webrtc_lib(media_engine)
declare_webrtc_lib(rtc_simulcast_encoder_adapter)
declare_webrtc_lib(rtc_software_fallback_wrappers)

# node-webrtc
# -----------------------------------------------------------------------------

file(GLOB_RECURSE MODULE_SRC src/*.cc src/*.hh)
add_library(${MODULE} SHARED ${MODULE_SRC} ${CMAKE_JS_SRC})
set_target_properties(${MODULE} PROPERTIES PREFIX "" SUFFIX ".node")
set_property(TARGET ${MODULE} PROPERTY CXX_STANDARD 23)
target_include_directories(${MODULE} SYSTEM PRIVATE
  ${CMAKE_JS_INC}
  ${CMAKE_SOURCE_DIR}/node_modules
  ${webrtc_dir}/include
  ${webrtc_dir}/include/webrtc
  ${webrtc_dir}/include/webrtc/third_party/abseil-cpp
  ${webrtc_dir}/include/webrtc/third_party/libyuv/include
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_dependencies(${MODULE} project_catch2)
  target_include_directories(${MODULE} SYSTEM PRIVATE
    ${catch2_install_dir}/single_include
  )
endif()

target_include_directories(${MODULE} PRIVATE
  ${CMAKE_SOURCE_DIR}
)

target_link_libraries(${MODULE} PRIVATE
  ${CMAKE_THREAD_LIBS_INIT}
  webrtc
  builtin_video_encoder_factory
  builtin_video_decoder_factory
  rtc_internal_video_codecs
  media_engine
  rtc_simulcast_encoder_adapter
  rtc_software_fallback_wrappers
  ${CMAKE_JS_LIB}
)

target_compile_definitions(${MODULE} PRIVATE
  -DNAPI_VERSION=9
  -DNODE_ADDON_API_CPP_EXCEPTIONS=1
  -DNODE_ADDON_API_CPP_EXCEPTIONS_ALL=1
  -DUSE_BUILTIN_SW_CODECS
  -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE
)

if(WIN32)
    target_link_libraries(${MODULE} PRIVATE
        libc++
    )
    target_link_options(${MODULE} PRIVATE
        "/WHOLEARCHIVE:$<TARGET_FILE:libc++>"
    )
  set_property(TARGET ${MODULE}
    PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
  )

  target_link_libraries(${MODULE} PRIVATE
    dmoguids.lib
    msdmo.lib
    secur32.lib
    winmm.lib
    wmcodecdspuuid.lib
    ws2_32.lib
    Iphlpapi.lib
    delayimp.lib
  )

    # ignore MSVC's C++ standard library headers
    target_compile_options(${MODULE} PRIVATE
        "SHELL:-Xclang -nostdinc++"
    )

    # inject libc++ as system headers before clang's builtins
    foreach(DIR IN LISTS libc++_include_dir)
      target_compile_options(${MODULE} PRIVATE
        "SHELL:-Xclang -isystem -Xclang \"${DIR}\""
      )
    endforeach()

    target_link_options(${MODULE} PRIVATE
      /defaultlib:libcmt
      /defaultlib:libcpmt
      /nodefaultlib:libc++
      /nodefaultlib:c++
    )

    target_compile_definitions(${MODULE} PRIVATE
      -D_LIBCPP_ABI_NAMESPACE=Cr
      -D_LIBCPP_ABI_VERSION=2
      -D_LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS
      -D_LIBCPP_HAS_NO_VENDOR_AVAILABILITY_ANNOTATIONS
    )

  target_compile_definitions(${MODULE} PRIVATE
    -D_WINSOCKAPI_
    -DINCL_EXTRA_HTON_FUNCTIONS
    -DNOGDI
    -DNOMINMAX
    -DWEBRTC_WIN
    "$<$<CONFIG:Debug>:_HAS_ITERATOR_DEBUGGING=0>"
  )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(${MODULE} PRIVATE
        "-Wl,--whole-archive"
        libc++
        libc++abi
        "-Wl,--no-whole-archive"
    )
    target_compile_definitions(${MODULE} PRIVATE
      -DWEBRTC_LINUX
    )

    target_compile_options(${MODULE} PRIVATE
      -fpermissive
    )
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-Wl,-force_load,$<TARGET_FILE:libc++>"
        "-Wl,-force_load,$<TARGET_FILE:libc++abi>"
    )
    target_link_libraries(${MODULE} PRIVATE
      "-framework AppKit"
      "-framework AVFoundation"
    )
    target_compile_options(${MODULE} PRIVATE
      -Weverything
      -Wno-c++98-compat
      -Wno-c++98-compat-pedantic
      -Wno-covered-switch-default
      -Wno-documentation
      -Wno-exit-time-destructors
      -Wno-float-conversion
      -Wno-unused-parameter
      -Wno-global-constructors
      -Wno-padded
      -Wno-shadow
      -Wno-shadow-field-in-constructor
      -Wno-shorten-64-to-32
      -Wno-sign-conversion
      -Wno-thread-safety-negative
      -Wno-unused-template
      -Wno-weak-vtables
    )

    target_compile_definitions(${MODULE} PRIVATE
      -DWEBRTC_MAC
      -DWEBRTC_IOS
    )
endif()

if(NOT WIN32)
  target_link_libraries(${MODULE} PRIVATE
    -lc
    -lm
  )
  target_link_options(${MODULE} PRIVATE
    "-B${libwebrtc_source_dir}/webrtc/third_party/llvm-build/Release+Asserts/bin" # prefer the lld built by webrtc
    "-fuse-ld=lld" # force LLVM linker, required because libwebrtc is using Compact Relocation and GNU LD does not support it
  )
  target_compile_options(${MODULE} PRIVATE
    -fno-rtti
    -fvisibility=hidden
    -fsafe-buffer-usage-suggestions
    -Wall
    -Wextra
    -Wpedantic
    -Wno-missing-field-initializers
    -Wno-uninitialized
    -Wno-unused-local-typedefs
    -Wno-unused-parameter
    -Wno-deprecated-declarations
    -Wno-unused-variable
    "$<$<CONFIG:Debug>:-g>"
  )

  target_compile_definitions(${MODULE} PRIVATE
    -DWEBRTC_POSIX=1
    "$<$<CONFIG:Debug>:DEBUG>"
  )

    target_include_directories(${MODULE} SYSTEM PRIVATE
      ${libc++_include_dir}
      ${libc++abi_include_dir}
    )

    # We statically link the same libc++ and libc++abi libraries that libwebrtc builds against.
    target_link_libraries(${MODULE} PRIVATE
      libc++
      libc++abi
      -nostdlib++ # (clang specific)
    )

    target_compile_options(${MODULE} PRIVATE
      -nostdlib++
      -nostdinc++
      -nodefaultlibs
    )

    target_compile_definitions(${MODULE} PRIVATE
      -D_LIBCPP_ABI_NAMESPACE=Cr
      -D_LIBCPP_ABI_VERSION=2
    )
endif()
