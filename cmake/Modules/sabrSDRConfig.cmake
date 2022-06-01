INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_SABRSDR sabrSDR)

FIND_PATH(
    SABRSDR_INCLUDE_DIRS
    NAMES sabrSDR/api.h
    HINTS $ENV{SABRSDR_DIR}/include
        ${PC_SABRSDR_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    SABRSDR_LIBRARIES
    NAMES gnuradio-sabrSDR
    HINTS $ENV{SABRSDR_DIR}/lib
        ${PC_SABRSDR_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/sabrSDRTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SABRSDR DEFAULT_MSG SABRSDR_LIBRARIES SABRSDR_INCLUDE_DIRS)
MARK_AS_ADVANCED(SABRSDR_LIBRARIES SABRSDR_INCLUDE_DIRS)
