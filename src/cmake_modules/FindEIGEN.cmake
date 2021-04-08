# Find the EIGEN package. This includes the libraries and include
# files.
#
# This code defines the following variables:
#
#  EIGEN_FOUND          - TRUE if all components are found.
#  EIGEN_INCLUDE_DIRS   - Full paths to all include dirs.
#  EIGEN_LIBRARIES      - Full paths to all libraries.
#
# Example Usages:
#  find_package(EIGEN)
#
# The location of EIGEN can be specified using the environment variable
# or cmake parameter PATH_EIGEN. If different installations
# for 32-/64-bit versions and release/debug versions are available,
# they can be specified with
#   PATH_EIGEN
#   PATH_EIGEN_RELEASE
#   PATH_EIGEN_DEBUG
# More specific paths are preferred over less specific ones when searching
# for libraries and 64 bit over 32 bit.
#
# Note that the standard FIND_PACKAGE features are supported
# (QUIET, REQUIRED, etc.).

set(EIGEN_SUPPRESS_WARNINGS "FLAG_SUPPRESS_WARNINGS")

foreach(BUILDMODE "RELEASE" "DEBUG")
    set(EIGEN_HINT_PATHS_${BUILDMODE}
        ${PATH_EIGEN_${BUILDMODE}}
        $ENV{PATH_EIGEN_${BUILDMODE}}
        ${PATH_EIGEN}
        $ENV{PATH_EIGEN}
    )
endforeach()

find_path(EIGEN_INCLUDE_DIRS
    NAMES unsupported
    HINTS ${EIGEN_HINT_PATHS_RELEASE} ${EIGEN_HINT_PATHS_DEBUG}
    PATH_SUFFIXES include/eigen3
)


# Check for consistency and handle arguments like QUIET, REQUIRED, etc.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    EIGEN
    REQUIRED_VARS EIGEN_INCLUDE_DIRS
)

# Do not show internal variables in cmake GUIs like ccmake.
mark_as_advanced(EIGEN_INCLUDE_DIRS
                 EIGEN_LIBRARIES
                 EIGEN_HINT_PATHS_RELEASE EIGEN_HINT_PATHS_DEBUG)
