add_source_group_tree(. SOURCE)

include_directories("../")

find_package(Orbiter REQUIRED)
include_directories(${Orbiter_INCLUDE_DIR})

find_package(Skybolt REQUIRED)
include_directories(${Skybolt_INCLUDE_DIR}/Skybolt)

find_package(OpenGL REQUIRED)

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIR})

OPTION(GLEW_STATIC_LIBS "GLEW use static libs")
if (GLEW_STATIC_LIBS)
	add_definitions(-DGLEW_STATIC)
endif()

set(LIBS
	${GLEW_LIBRARIES}
	${Orbiter_LIBRARIES}
	${OPENGL_LIBRARIES}
	${Skybolt_LIBRARIES}
)

add_library(OrbiterSkyboltClient SHARED ${SOURCE} SkyboltClient.rc)
target_link_libraries (OrbiterSkyboltClient ${LIBS})

set(ORBITER_PLUGINS_DIRECTORY "" CACHE STRING "Orbiter Plugins Directory")

install(TARGETS OrbiterSkyboltClient
	DESTINATION ${ORBITER_PLUGINS_DIRECTORY}
)