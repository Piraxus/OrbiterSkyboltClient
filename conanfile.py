from conans import ConanFile, CMake
import os

class SkyboltConan(ConanFile):
    name = "orbiter-skybolt-client"
    version = "1.0.3"
    settings = "os", "compiler", "arch", "build_type"
    options = {
		"shared": [True, False]
	}
    default_options = {
        "shared": False,
    }
    generators = ["cmake_paths", "cmake_find_package"]
    exports_sources = "*"
    no_copy_source = True

    requires = [
		"glew/2.2.0@_/_",
		"skybolt/1.4.0@_/_"
	]

    def configure(self):
        self.options["skybolt"].shared_plugins = False

    def build(self):
        cmake = CMake(self)

        cmake.definitions["GLEW_STATIC_LIBS"] = str(not self.options["glew"].shared)
        cmake.definitions["CMAKE_TOOLCHAIN_FILE"] = "conan_paths.cmake"
        cmake.configure()
        cmake.build()
		
    def package(self):
        cmake = CMake(self)
        cmake.install()