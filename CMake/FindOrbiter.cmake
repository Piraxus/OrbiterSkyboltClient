# This module defines
#  Orbiter_FOUND, if false, do not try to link
#  Orbiter_LIBRARY, the library to link against
#  Orbiter_INCLUDE_DIR, where to find headers


FIND_PATH(Orbiter_INCLUDE_DIR GraphicsAPI.h
  PATHS
	$ENV{ORBITER_SDK_DIR}
  PATH_SUFFIXES
	include
)

FIND_LIBRARY(Orbiter_LIBRARY_R
  NAMES Orbiter
  PATHS
	$ENV{ORBITER_SDK_DIR}
  PATH_SUFFIXES
	lib
)

FIND_LIBRARY(Orbiter_LIBRARY_D
  NAMES Orbiterd Orbiter
  PATHS
	$ENV{ORBITER_SDK_DIR}
  PATH_SUFFIXES
	lib
)

FIND_LIBRARY(Orbiter_SDK_LIBRARY_R
  NAMES Orbitersdk
  PATHS
	$ENV{ORBITER_SDK_DIR}
  PATH_SUFFIXES
	lib
)

FIND_LIBRARY(Orbiter_SDK_LIBRARY_D
  NAMES Orbitersdkd Orbitersdk
  PATHS
	$ENV{ORBITER_SDK_DIR}
  PATH_SUFFIXES
	lib
)

SET(Orbiter_LIBRARIES
	optimized ${Orbiter_LIBRARY_R}
	debug ${Orbiter_LIBRARY_D}
	optimized ${Orbiter_SDK_LIBRARY_R}
	debug ${Orbiter_SDK_LIBRARY_D}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Orbiter DEFAULT_MSG Orbiter_LIBRARIES Orbiter_INCLUDE_DIR)