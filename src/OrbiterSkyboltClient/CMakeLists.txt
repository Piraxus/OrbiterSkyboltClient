add_source_group_tree(. SOURCE)

include_directories("../")
include_directories("../../Skybolt")

find_package(Orbiter REQUIRED)
include_directories(${Orbiter_INCLUDE_DIR})

find_package(OpenGL REQUIRED)
find_package(GLEW)
include_directories(${GLEW_INCLUDE_DIR})

set(LIBS
	SkyboltEngine
	${Orbiter_LIBRARIES}
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
)

add_library(OrbiterSkyboltClient SHARED ${SOURCE} SkyboltClient.rc)
target_link_libraries (OrbiterSkyboltClient ${LIBS})

set(ORBITER_PLUGINS_DIRECTORY "" CACHE STRING "Orbiter Plugins Directory")

install(TARGETS OrbiterSkyboltClient
	DESTINATION ${ORBITER_PLUGINS_DIRECTORY}
)