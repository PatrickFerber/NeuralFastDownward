# Find the Tensorflow package. This includes the libraries and include
# files.
#
# Attention: I got annoyed by the always changing and complicated setup of
# Tensorflow. I prefer now PyTorch.
#
# Attention: Tensorflow is regularly changing in how to build its
# libraries and what is the output of the
# builds. It might be that for your version, this script has to be
# adapted.
# Last time I checked the script tensorflow did not allow compiling
# a static library, thus, we have to use a hack to find the shared
# library even in static linking mode.
#
#
# This code defines the following variables:
#
#  TENSORFLOW_FOUND             - TRUE if all components are found.
#  TENSORFLOW_INCLUDE_DIRS      - Full paths to all include dirs.
#  TENSORFLOW_LIBRARIES         - Full paths to all libraries.
#  TENSORFLOW_SUPPRESS_WARNINGS - Flag to ignore compiler warnings
#
# Example Usages:
#  find_package(TENSORFLOW)
#
# The location of TENSORFLOW can be specified using the environment variable
# or cmake parameter PATH_TENSORFLOW. If different installations
# for 32-/64-bit versions and release/debug versions are available,
# they can be specified with
#   PATH_TENSORFLOW
#   PATH_TENSORFLOW_RELEASE
#   PATH_TENSORFLOW_DEBUG
# More specific paths are preferred over less specific ones
#
# Note that the standard FIND_PACKAGE features are supported
# (QUIET, REQUIRED, etc.).

set(TENSORFLOW_SUPPRESS_WARNINGS "FLAG_SUPPRESS_WARNINGS")

foreach(BUILDMODE "RELEASE" "DEBUG")
    set(TENSORFLOW_HINT_PATHS_${BUILDMODE}
        ${PATH_TENSORFLOW_${BUILDMODE}}
        $ENV{PATH_TENSORFLOW_${BUILDMODE}}
        ${PATH_TENSORFLOW}
        $ENV{PATH_TENSORFLOW}
    )
endforeach()

find_path(TENSORFLOW_INCLUDE
    NAMES tensorflow
    HINTS ${TENSORFLOW_HINT_PATHS_RELEASE} ${TENSORFLOW_HINT_PATHS_DEBUG}
    PATH_SUFFIXES include
)

# A new version of tensorflow required this directory too
find_path(TENSORFLOW_NSYNC
    NAMES nsync.h
    HINTS ${TENSORFLOW_HINT_PATHS_RELEASE} ${TENSORFLOW_HINT_PATHS_DEBUG}
    PATH_SUFFIXES include/nsync/public
)
set(TENSORFLOW_INCLUDE_DIRS ${TENSORFLOW_INCLUDE} ${TENSORFLOW_NSYNC})


find_library(TENSORFLOW_CC_LIBRARY_RELEASE
    NAMES tensorflow_cc
    HINTS ${TENSORFLOW_HINT_PATHS_RELEASE}
    PATH_SUFFIXES lib
)

find_library(TENSORFLOW_FRAMEWORK_LIBRARY_RELEASE
    NAMES tensorflow_framework
    HINTS ${TENSORFLOW_HINT_PATHS_RELEASE}
    PATH_SUFFIXES lib
)

find_library(TENSORFLOW_CC_LIBRARY_DEBUG
    NAMES tensorflow_cc
    HINTS ${TENSORFLOW_HINT_PATHS_DEBUG}
    PATH_SUFFIXES lib
)

find_library(TENSORFLOW_FRAMEWORK_LIBRARY_DEBUG
    NAMES tensorflow_framework
    HINTS ${TENSORFLOW_HINT_PATHS_DEBUG}
    PATH_SUFFIXES lib
)


# HACK: If the framework library is missing skip it, because older TF
# versions do not have it.
set(TENSORFLOW_LIBRARIES
    optimized ${TENSORFLOW_CC_LIBRARY_RELEASE}
    debug ${TENSORFLOW_CC_LIBRARY_DEBUG})

if(TENSORFLOW_FRAMEWORK_LIBRARY_RELEASE)
    list(APPEND TENSORFLOW_LIBRARIES optimized ${TENSORFLOW_FRAMEWORK_LIBRARY_RELEASE})
endif()
if(TENSORFLOW_FRAMEWORK_LIBRARY_DEBUG)
    list(APPEND TENSORFLOW_LIBRARIES debug ${TENSORFLOW_FRAMEWORK_LIBRARY_DEBUG})
endif()


# Check for consistency and handle arguments like QUIET, REQUIRED, etc.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    TENSORFLOW
    REQUIRED_VARS TENSORFLOW_INCLUDE_DIRS TENSORFLOW_LIBRARIES
)

# Do not show internal variables in cmake GUIs like ccmake.
mark_as_advanced(TENSORFLOW_INCLUDE_DIRS
                 TENSORFLOW_LIBRARY_RELEASE TENSORFLOW_LIBRARY_DEBUG
                 TENSORFLOW_LIBRARIES
                 TENSORFLOW_HINT_PATHS_RELEASE TENSORFLOW_HINT_PATHS_DEBUG)
