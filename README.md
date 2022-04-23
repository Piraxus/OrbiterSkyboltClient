# Orbiter Skybolt Client
A 3d graphics client plugin for the [Orbiter](https://github.com/orbitersim/orbiter) space flight simulator which allows Orbiter to render with the [Skybolt](https://github.com/Piraxus/Skybolt) engine.

This project is in early development and many Orbiter graphics features are not yet supported.

## Build
### Using conan
[Conan](https://conan.io) is a C++ package manager which makes builds easier.
If you do not already have conan installed, run `pip install conan` to install conan using [pip](https://pypi.org/project/pip).

Then build with the following commands:
```
git clone https://github.com/piraxus/Skybolt
conan export Skybolt

git clone https://github.com/Piraxus/OrbiterSkyboltClient
conan install OrbiterSkyboltClient --install-folder OrbiterSkyboltClientBuild --build=missing
conan build --build-folder OrbiterSkyboltClientBuild
```

The plugin is built to `OrbiterSkyboltClientBuild/bin/Release/OrbiterSkyboltClient.dll`.

## Contact
Skybolt and the Orbiter Skybolt Client created and maintained by Matthew Reid. To submit a bug report, please [raise an issue on the GitHub repository](https://github.com/Piraxus/OrbiterSkyboltClient/issues).

## License
This project is licensed under the MIT license - see the [License.txt](License.txt) file for details.