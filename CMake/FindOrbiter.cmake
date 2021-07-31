# This module defines
#  Orbiter_FOUND, if false, do not try to link
#  Orbiter_LIBRARY, the library to link against
#  Orbiter_INCLUDE_DIR, where to find headers


FIND_PATH(Orbiter_INCLUDE_DIR GraphicsAPI.h
  PATHS
	$ENV{ORBITER_SOURCE_DIR}
  PATH_SUFFIXES
	include
)

FIND_LIBRARY(Orbiter_LIBRARY_R
  NAMES Orbiter_server
  PATHS
	$ENV{ORBITER_LIB_DIR}
  PATH_SUFFIXES
	lib
)

FIND_LIBRARY(Orbiter_LIBRARY_D
  NAMES Orbiter_serverd Orbiter_server
  PATHS
	$ENV{ORBITER_LIB_DIR}
  PATH_SUFFIXES
	lib
)

FIND_LIBRARY(Orbiter_SDK_LIBRARY_R
  NAMES Orbitersdk
  PATHS
	$ENV{ORBITER_LIB_DIR}
  PATH_SUFFIXES
	lib
)

FIND_LIBRARY(Orbiter_SDK_LIBRARY_D
  NAMES Orbitersdkd Orbitersdk
  PATHS
	$ENV{ORBITER_LIB_DIR}
  PATH_SUFFIXES
	lib
)

SET(Orbiter_LIBRARIES optimized ${Orbiter_LIBRARY_R} ${Orbiter_SDK_LIBRARY_R} debug ${Orbiter_LIBRARY_D} ${Orbiter_SDK_LIBRARY_D})

IF(Orbiter_LIBRARY AND Orbiter_INCLUDE_DIR)
  SET(Orbiter_FOUND "YES")
ENDIF()
