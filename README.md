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
conan build OrbiterSkyboltClient --build-folder OrbiterSkyboltClientBuild
```

The plugin is built to `OrbiterSkyboltClientBuild/bin/Release/OrbiterSkyboltClient.dll`.

## Install
1. Copy `OrbiterSkyboltClient.dll` to `<Orbiter repo>/Modules/Plugin`
2. Copy `<Skybolt repo>/Assets/Core` to `<Orbiter repo>/Modules/Plugin/OrbiterSkyboltClient/Assets/Core`
3. `cd` to the `SkyboltOrbiterClient` repository root and run `dvc pull` to fetch the remote binary assets (images etc required at runtime). If you do not already have DVC installed, first run `pip install dvc[s3]` to install with [pip](https://pypi.org/project/pip)
4. Copy all folders under `<SkyboltOrbiterClient repo>/Assets` to `<Orbiter repo>/Modules/Plugin/OrbiterSkyboltClient/Assets`

## Contact
Skybolt and the Orbiter Skybolt Client created and maintained by Matthew Reid. To submit a bug report, please [raise an issue on the GitHub repository](https://github.com/Piraxus/OrbiterSkyboltClient/issues).

## License
This project is licensed under the MIT license - see the [License.txt](License.txt) file for details.