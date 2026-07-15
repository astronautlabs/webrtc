cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)

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

assign_bool(use_custom_libcxx "TRUE")
assign_bool(is_arm32 "y$ENV{TARGET_ARCH}" STREQUAL "yarm")
assign_bool(is_arm64 "y$ENV{TARGET_ARCH}" STREQUAL "yarm64")

if (APPLE AND ${is_arm64})
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE INTERNAL "" FORCE)
endif()

if (WIN32 AND ${use_custom_libcxx})
  set(CMAKE_C_COMPILER "clang-cl" CACHE STRING "" FORCE)
  set(CMAKE_CXX_COMPILER "clang-cl" CACHE STRING "" FORCE)
elseif(NOT WIN32)
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

# Use ccache if requested and available. This caches by file contents instead of file timestamp.
# This produces a large amount of files under .cache/ccache which take a lot of time to hoover up in CI, so
# we don't currently use this in our CI, but it may be useful in the future. Instead we use git-restore-mtime to
# ensure timestamps will reflect last modified time, not checkout time (see download-webrtc.sh)
if(DEFINED ENV{USE_CCACHE})
    find_program(CCACHE_PROGRAM NAMES sccache ccache)
    if(CCACHE_PROGRAM)
        message(STATUS "Using ccache at: ${CCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        list(APPEND GN_GEN_ARGS "cc_wrapper=\"${CCACHE_PROGRAM}\"")
    else()
        message(WARNING "ccache is requested (environment USE_CCACHE), but ccache was not found on the system, it will be skipped")
    endif()
endif()

set(MODULE wrtc)
project(${MODULE})

parse_cmakejs_properties()

include(ExternalProject)
find_package(Git REQUIRED)
find_package(Threads REQUIRED)

# depot_tools
# -----------------------------------------------------------------------------

ExternalProject_Add(
  project_depot_tools

  GIT_REPOSITORY    https://chromium.googlesource.com/chromium/tools/depot_tools.git
  GIT_TAG           4ce8ba39a3488397a2d1494f167020f21de502f3

  PREFIX            ${CMAKE_BINARY_DIR}/external/depot_tools/prefix
  TMP_DIR           ${CMAKE_BINARY_DIR}/external/depot_tools/tmp
  STAMP_DIR         ${CMAKE_BINARY_DIR}/external/depot_tools/stamp
  DOWNLOAD_DIR      ${CMAKE_BINARY_DIR}/external/depot_tools/download
  SOURCE_DIR        ${CMAKE_BINARY_DIR}/external/depot_tools/depot_tools
  BINARY_DIR        ${CMAKE_BINARY_DIR}/external/depot_tools/build

  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
)

ExternalProject_Get_Property(project_depot_tools SOURCE_DIR)
set(depot_tools_install_dir ${SOURCE_DIR})

# libc++
# -----------------------------------------------------------------------------

set(libwebrtc_binary_dir ${CMAKE_BINARY_DIR}/external/libwebrtc/build/${CMAKE_BUILD_TYPE})
set(libwebrtc_src_dir ${CMAKE_BINARY_DIR}/external/libwebrtc/download/src)

add_library(libc++ OBJECT IMPORTED)
add_dependencies(libc++ libwebrtc)

if(WIN32)
  set(obj_ext .obj)
else()
  set(obj_ext .o)
endif()

# To update this set based on the built libc++ when upgrading the project, use the following command to obtain a new
# list:
# ls build/external/libwebrtc/build/Debug/obj/buildtools/third_party/libc++/libc++/*.o | xargs -n 1 basename | sed 's/\.o/${obj_ext}/'
set(libc++_objects
    algorithm${obj_ext}
    any${obj_ext}
    atomic${obj_ext}
    barrier${obj_ext}
    bind${obj_ext}
    call_once${obj_ext}
    charconv${obj_ext}
    chrono${obj_ext}
    condition_variable${obj_ext}
    condition_variable_destructor${obj_ext}
    d2fixed${obj_ext}
    d2s${obj_ext}
    directory_iterator${obj_ext}
    error_category${obj_ext}
    exception${obj_ext}
    f2s${obj_ext}
    filesystem_clock${obj_ext}
    filesystem_error${obj_ext}
    functional${obj_ext}
    future${obj_ext}
    hash${obj_ext}
    ios.instantiations${obj_ext}
    ios${obj_ext}
    iostream${obj_ext}
    locale${obj_ext}
    memory${obj_ext}
    mutex${obj_ext}
    mutex_destructor${obj_ext}
    new${obj_ext}
    new_handler${obj_ext}
    new_helpers${obj_ext}
    operations${obj_ext}
    optional${obj_ext}
    path${obj_ext}
    random${obj_ext}
    random_shuffle${obj_ext}
    regex${obj_ext}
    shared_mutex${obj_ext}
    stdexcept${obj_ext}
    string${obj_ext}
    strstream${obj_ext}
    system_error${obj_ext}
    thread${obj_ext}
    typeinfo${obj_ext}
    valarray${obj_ext}
    variant${obj_ext}
    vector${obj_ext}
    verbose_abort${obj_ext}
)

if(WIN32)
  list(APPEND libc++_objects
    locale_win32${obj_ext}
    support${obj_ext}
    thread_win32${obj_ext}
  )
else()
  list(APPEND libc++_objects
    directory_entry${obj_ext}
  )
endif()

list(TRANSFORM libc++_objects PREPEND ${libwebrtc_binary_dir}/obj/buildtools/third_party/libc++/libc++/)

set_property(TARGET libc++ APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(libc++ PROPERTIES IMPORTED_OBJECTS_DEBUG "${libc++_objects}" IMPORTED_OBJECTS "${libc++_objects}")

# NOTE(mroberts): I would like this to be INTERFACE.
#   https://gitlab.kitware.com/cmake/cmake/issues/15052
# target_include_directories(libc++ SYSTEM INTERFACE "${libc++_include_dir}")

# libc++abi
# -----------------------------------------------------------------------------

add_library(libc++abi OBJECT IMPORTED)
add_dependencies(libc++abi libwebrtc)

set(libc++abi_objects
  abort_message.o
  cxa_aux_runtime.o
  cxa_default_handlers.o
  cxa_demangle.o
  cxa_exception.o
  cxa_exception_storage.o
  cxa_guard.o
  cxa_handlers.o
  cxa_personality.o
  cxa_vector.o
  cxa_virtual.o
  fallback_malloc.o
  private_typeinfo.o
  stdlib_exception.o
  stdlib_stdexcept.o
  stdlib_typeinfo.o
)
list(TRANSFORM libc++abi_objects PREPEND ${libwebrtc_binary_dir}/obj/buildtools/third_party/libc++abi/libc++abi/)

set_property(TARGET libc++abi APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(libc++abi PROPERTIES IMPORTED_OBJECTS_DEBUG "${libc++abi_objects}" IMPORTED_OBJECTS "${libc++abi_objects}")

# NOTE(mroberts): I would like this to be INTERFACE.
#   https://gitlab.kitware.com/cmake/cmake/issues/15052
# target_include_directories(libc++abi SYSTEM INTERFACE "${libc++abi_include_dir}")

# libwebrtc
# -----------------------------------------------------------------------------

# https://chromiumdash.appspot.com/branches
# M87: branch-heads/4280
# M91: branch-heads/4472
# M92: branch-heads/4515
# M94: branch-heads/4606
# M98: branch-heads/4758
# M102: branch-heads/5005
# M106: branch-heads/5249
# M110: branch-heads/5481
# M114: branch-heads/5735
# M150: branch-heads/7871
set(WEBRTC_REVISION branch-heads/7871)

list(APPEND GN_GEN_ARGS
  rtc_build_examples=false
  rtc_use_x11=false
  rtc_enable_protobuf=false
  rtc_include_pulse_audio=false
  rtc_include_tests=false
  use_lld=true
)

if (NOT DEFINED ENV{TARGET_ARCH})
  set(ENV{TARGET_ARCH} "x64")
endif()

if (WIN32)
  # The escaping is very strange here
  list(APPEND GN_GEN_ARGS
    "target_cpu=\\\"$ENV{TARGET_ARCH}\\\""
  )
else()
  list(APPEND GN_GEN_ARGS
    "target_cpu=\"$ENV{TARGET_ARCH}\""
  )
endif()

if (${use_custom_libcxx})
  list(APPEND GN_GEN_ARGS
    use_custom_libcxx=true
  )
else()
  list(APPEND GN_GEN_ARGS
    use_custom_libcxx=false
  )
endif()

if (${is_arm32} OR ${is_arm64})
  list(APPEND GN_GEN_ARGS
    rtc_build_tools=true
  )
else()
  list(APPEND GN_GEN_ARGS
    rtc_build_tools=false
  )
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  list(APPEND GN_GEN_ARGS is_debug=true)
else()
  list(APPEND GN_GEN_ARGS is_debug=false)
endif()
string(REPLACE ";" " " GN_GEN_ARGS "${GN_GEN_ARGS}")

if(WIN32)
  set(suffix bat)
  set(PLATFORM windows)
else()
  set(suffix sh)
  if(APPLE)
    set(PLATFORM darwin)
  else()
    set(PLATFORM linux)
  endif()
endif()

set(byproducts
    ${libc++_objects}
    ${libc++abi_objects}
    ${libwebrtc_binary_dir}/obj/${CMAKE_STATIC_LIBRARY_PREFIX}webrtc${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${libwebrtc_binary_dir}/obj/api/video_codecs/${CMAKE_STATIC_LIBRARY_PREFIX}builtin_video_encoder_factory${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${libwebrtc_binary_dir}/obj/api/video_codecs/${CMAKE_STATIC_LIBRARY_PREFIX}builtin_video_decoder_factory${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${libwebrtc_binary_dir}/obj/api/video_codecs/${CMAKE_STATIC_LIBRARY_PREFIX}rtc_software_fallback_wrappers${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${libwebrtc_binary_dir}/obj/media/${CMAKE_STATIC_LIBRARY_PREFIX}rtc_internal_video_codecs${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${libwebrtc_binary_dir}/obj/media/${CMAKE_STATIC_LIBRARY_PREFIX}media_engine${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${libwebrtc_binary_dir}/obj/media/${CMAKE_STATIC_LIBRARY_PREFIX}rtc_simulcast_encoder_adapter${CMAKE_STATIC_LIBRARY_SUFFIX}
)

ExternalProject_Add(
  project_libwebrtc

  PREFIX            ${CMAKE_BINARY_DIR}/external/libwebrtc/prefix
  TMP_DIR           ${CMAKE_BINARY_DIR}/external/libwebrtc/tmp
  STAMP_DIR         ${CMAKE_BINARY_DIR}/external/libwebrtc/stamp
  DOWNLOAD_DIR      ${CMAKE_BINARY_DIR}/external/libwebrtc/download
  SOURCE_DIR        ${CMAKE_BINARY_DIR}/external/libwebrtc/download/src
  BINARY_DIR        ${CMAKE_BINARY_DIR}/external/libwebrtc/build/${CMAKE_BUILD_TYPE}

  BUILD_BYPRODUCTS  ${byproducts}

  DOWNLOAD_COMMAND  ${CMAKE_COMMAND} -E env DEPOT_TOOLS=${depot_tools_install_dir} PLATFORM=${PLATFORM} WEBRTC_REVISION=${WEBRTC_REVISION} ${CMAKE_SOURCE_DIR}/scripts/download-webrtc.${suffix}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env BINARY_DIR=<BINARY_DIR> DEPOT_TOOLS=${depot_tools_install_dir} GN_GEN_ARGS=${GN_GEN_ARGS} SOURCE_DIR=<SOURCE_DIR> ${CMAKE_SOURCE_DIR}/scripts/configure-webrtc.${suffix}
  BUILD_COMMAND     ${CMAKE_COMMAND} -E env CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} DEPOT_TOOLS=${depot_tools_install_dir} ${CMAKE_SOURCE_DIR}/scripts/build-webrtc.${suffix}
  INSTALL_COMMAND   ""

  # Need these to avoid timeouts during CI (and also to see more progress for these steps which are quite heavy)
  USES_TERMINAL_DOWNLOAD 1
  USES_TERMINAL_CONFIGURE 1
  USES_TERMINAL_BUILD 1
)

add_dependencies(project_libwebrtc project_depot_tools)

ExternalProject_Get_Property(project_libwebrtc DOWNLOAD_DIR)
set(libwebrtc_source_dir "${DOWNLOAD_DIR}")

ExternalProject_Get_Property(project_libwebrtc BINARY_DIR)
set(libwebrtc_binary_dir "${BINARY_DIR}")

add_library(libwebrtc STATIC IMPORTED)
add_dependencies(libwebrtc project_libwebrtc)

if(WIN32)
  set_property(TARGET libwebrtc PROPERTY IMPORTED_LOCATION "${libwebrtc_binary_dir}/obj/webrtc.lib")
else()
  set_property(TARGET libwebrtc PROPERTY IMPORTED_LOCATION "${libwebrtc_binary_dir}/obj/libwebrtc.a")
endif()

set(libc++_include_dir
  "${libwebrtc_source_dir}/src/third_party/libc++/src/include"
  "${libwebrtc_source_dir}/src/buildtools/third_party/libc++"
)
set(libc++abi_include_dir "${libwebrtc_source_dir}/src/third_party/libc++abi/src/include")

# NOTE(mroberts): I would like this to be INTERFACE.
#   https://gitlab.kitware.com/cmake/cmake/issues/15052
# target_include_directories(libwebrtc SYSTEM INTERFACE
#   ${libwebrtc_source_dir}
#   ${libwebrtc_source_dir}/webrtc
#   ${libwebrtc_source_dir}/webrtc/third_party/abseil-cpp
#   ${libwebrtc_source_dir}/webrtc/third_party/libyuv/include
# )

# catch2
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

function(import_webrtc_static_lib base_name sub_dir)
  add_library(${base_name} STATIC IMPORTED)
  set_target_properties(${base_name} PROPERTIES
    IMPORTED_LOCATION "${libwebrtc_binary_dir}/obj/${sub_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}${base_name}${CMAKE_STATIC_LIBRARY_SUFFIX}"
  )
  add_dependencies(${base_name} project_libwebrtc)
endfunction()

# ----------------------------
import_webrtc_static_lib(builtin_video_encoder_factory "api/video_codecs")
import_webrtc_static_lib(builtin_video_decoder_factory "api/video_codecs")
import_webrtc_static_lib(rtc_internal_video_codecs "media")
import_webrtc_static_lib(media_engine "media")
import_webrtc_static_lib(rtc_simulcast_encoder_adapter "media")
import_webrtc_static_lib(rtc_software_fallback_wrappers "api/video_codecs")

# node-webrtc
# -----------------------------------------------------------------------------

file(GLOB_RECURSE MODULE_SRC src/*.cc src/*.hh)
add_library(${MODULE} SHARED ${MODULE_SRC} ${CMAKE_JS_SRC})
set_target_properties(${MODULE} PROPERTIES PREFIX "" SUFFIX ".node")

set_property(TARGET ${MODULE} PROPERTY CXX_STANDARD 23)

# NOTE(mroberts): Workaround for
#   https://gitlab.kitware.com/cmake/cmake/issues/15052
# is to include all the header files here.
target_include_directories(${MODULE} SYSTEM PRIVATE
  ${CMAKE_JS_INC}
  ${CMAKE_SOURCE_DIR}/node_modules
  ${libwebrtc_source_dir}
  ${libwebrtc_source_dir}/webrtc
  ${libwebrtc_source_dir}/webrtc/third_party/abseil-cpp
  ${libwebrtc_source_dir}/webrtc/third_party/libyuv/include
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
  libwebrtc
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

  if (${use_custom_libcxx})
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

    target_link_libraries(${MODULE} PRIVATE
      libc++
    )

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
  endif()

  target_compile_definitions(${MODULE} PRIVATE
    -D_WINSOCKAPI_
    -DINCL_EXTRA_HTON_FUNCTIONS
    -DNOGDI
    -DNOMINMAX
    -DWEBRTC_WIN
    "$<$<CONFIG:Debug>:_HAS_ITERATOR_DEBUGGING=0>"
  )
else()
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

  if (${use_custom_libcxx})
    # NOTE(mroberts): Workaround for
    #   https://gitlab.kitware.com/cmake/cmake/issues/15052
    # is to include all the header files here.
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

  if (APPLE)
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
  else()
    target_compile_definitions(${MODULE} PRIVATE
      -DWEBRTC_LINUX
    )

    target_compile_options(${MODULE} PRIVATE
      -fpermissive
    )
  endif()
endif()

set(INCLUDE_DIRECTORIES
  ./
  ${CMAKE_SOURCE_DIR}/node_modules/node-addon-api
  ${libwebrtc_source_dir}
  ${libwebrtc_source_dir}/webrtc
  ${libwebrtc_source_dir}/webrtc/third_party/abseil-cpp
  ${libwebrtc_source_dir}/webrtc/third_party/libyuv/include
)
