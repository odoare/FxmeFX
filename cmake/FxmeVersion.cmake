# Reads the single source-of-truth series version from Source/Common/Version.h
# and exposes it as FXMEFX_VERSION (e.g. "0.3.1"). Included by the root and the
# per-plugin CMakeLists so the C++ header shown on the top bar and the built
# plugin (VST3/AU) metadata never drift apart. Bump the version in Version.h.

file(READ "${CMAKE_CURRENT_LIST_DIR}/../Source/Common/Version.h" _fxmefx_version_header)
string(REGEX MATCH "FXMEFX_VERSION_STRING[ \t]+\"([0-9]+\\.[0-9]+\\.[0-9]+)\""
       _fxmefx_version_match "${_fxmefx_version_header}")

if(NOT CMAKE_MATCH_1)
    message(FATAL_ERROR
        "FxmeVersion: could not parse FXMEFX_VERSION_STRING from Source/Common/Version.h")
endif()

set(FXMEFX_VERSION "${CMAKE_MATCH_1}")
