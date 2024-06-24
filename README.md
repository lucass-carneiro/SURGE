 # Welcome to SURGE
 
 **SURGE** is the **S**uper **U**nder**R**ated **G**ame **E**ngine

 A prototype game engine made for fun (*and profit ?*).

# Table of Contents
1. [Philosophy](#phylosophy)
2. [Obtaining and Building](#obtaining-and-building)
    1. [Dependencies](#dependencies)
    2. [Cloning](#cloning)
    3. [Build Configurations](#build-configurations)
    4. [Build Options](#build-options)
    5. [Build Commands](#build-commands)
    6. [Example Debug Build On Linux](#example-debug-build-on-linux)
    7. [Example Debug Build On Windows](#example-debug-build-on-windows)
3. [`SCOMP`](#scomp)
    1. [`Creating Stagings`](#creating-stagings)

# Philosophy

**SURGE** is divided in three logical components: A *player*, a *core* and *modules*. The **SURGE** core contains all of the engine's facilities condensed in a single shared object (DLL on Windows). The core can create an engine window, set up the graphics renderer, handle memory allocations, file IO, user inputs and other services.

After its initial setup, the *player* loads and executes a *module*. Modules utilize the services provided by the *core* to draw objects on the screen or to read from the user's mouse, keyboard and controllers. Each module is compiled as a dynamic library object. This allows for modules to be *hot reloaded*, that is, reloaded while the engine is running. The *player* takes care of loading, unloading and refreshing modules.

A SURGE game is thus a sequence of modules, loaded one after the other, each producing custom behavior that can be hot reloaded whenever the users wishes to do so.

# Obtaining and building

This section describes how to obtain and compile **SURGE**.

## Dependencies

**SURGE** depends on only two software components being preinstalled in the user's machine:

1. `git`
2. `CMake`

Additionally, **SURGE** expects that a C++ compiler toolchain such as `GCC`, `Clang` or `MSVC` is installed and working.

All other dependencies are obtained automatically at compile time via `vcpkg`, which is a submodule of this repository

## Cloning

The first step in compiling **SURGE** is to clone this repository. With `git` installed, issue the following commands in a terminal

```
clone https://github.com/lucass-carneiro/SURGE
cd SURGE
git submodule init
git submodule update --remote
```
The `--remote in the last` command ensures that all submodules will be checked out in their latest commits

## Build Configurations

**SURGE** can be built in the following configurations:

1. `Debug`
2. `Release`
3. `Profile`

In the `Debug` configuration, extra debug information is provided and optimizations are turned off.
In the `Release` config, optimizations are turned on but debug checks and information are turned off.
In the `Profile` config, optimization are turned on but debug symbols and information are also produced.

For developing games, the `Debug` configuration is recommended.
For profiling games and making them run faster, the `Profile` configuration is recommended.
For final builds and game releases, the `Release` configuration is recommended.

## Build Options

**SURGE** uses `CMake` to generate its build system. Several engine options and behaviors can be customized in the `CMake` configuration step via flags passed with the `-D[FLAG NAME]=[SETTING]` options in the `CMake` command line invocation. The available build options are detailed bellow

Option                         | Description                                           | Default Value                             |
-------------------------------|-------------------------------------------------------|-------------------------------------------|
SURGE_USE_LOG                  | Enable log messages                                   | ON                                        |
SURGE_USE_LOG_COLOR            | Use colors on log outputs                             | ON                                        |
SURGE_ENABLE_SANITIZERS        | Compiles code with sanitizers                         | ON (`Debug`) / OFF (`Release`, `Profile`) |
SURGE_ENABLE_OPTIMIZATIONS     | Compiles code with optimizations                      | OFF (`Debug`) / ON (`Release`, `Profile`) |
SURGE_ENABLE_LTO               | Compiles code with link time optimizations            | OFF (`Debug`) / ON (`Release`, `Profile`) |
SURGE_ENABLE_FAST_MATH         | Compiles code with fast math mode                     | OFF (`Debug`) / ON (`Release`, `Profile`) |
SURGE_ENABLE_TUNING            | Compiles code with architecture tuning                | OFF (`Debug`) / ON (`Release`, `Profile`) |
SURGE_DEBUG_MEMORY             | Enable custom allocators debug facilities             | ON (`Debug`) / OFF (`Release`, `Profile`) |
SURGE_ENABLE_HR                | Enable module hot reloading when pressing LCTRL + F5  | ON (`Debug`, `Release`, Profile)          |
SURGE_OPENGL_ERROR_BUFFER_SIZE | Buffer size (Bytes) for storing OpenGL error messages | 1024. Must be >= 1024                     |

## Build Commands

To create a `CMake` build system, issue

```
cmake -B [CONFIG] -S . -DVCPKG_TARGET_TRIPLET=[VCPKG TARGET TRIPLET] -DCMAKE_BUILD_TYPE=[CONFIG] -D[OTHER BUILD  OPTIONS]=[OPTION VALUES]
```

Where

* `[CONFIG]` is either `Debug`, `Release` or `Profile`
* `[VCPKG TARGET TRIPLET]` is the target `vcpkg` triplet for the machine. For example, 64-bit Linux builds will use `x64-linux` and 64-bit Windows builds will use `x64-windows`
* `[OTHER BUILD OPTIONS]` are options from the table above or options such as `-DCMAKE_CXX_COMPILER` for using different `C++` compilers

To build the configuration, issue

```
cmake --build [CONFIG] --config [CONFIG]
```

Where `[CONFIG]` matches the previous command. If multithreading is available in the machine compiling the code, `-j[NUM THREADS]` can be passed as well to speed up the build process.

## Example Debug Build On Linux

```
git clone https://github.com/lucass-carneiro/SURGE
cd SURGE
git submodule init
git submodule update --remote
cmake -B Debug -S . -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug
cmake --build Debug --config Debug -j20
```
Most used build command

```
cmake -B Debug -S . -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -DSURGE_DEBUG_MEMORY=OFF -DSURGE_LOG_GL_NOTIFICATIONS=OFF
```

## Example Debug Build On Windows

```
git clone https://github.com/lucass-carneiro/SURGE
cd SURGE
git submodule init
git submodule update --remote
cmake -B Debug -S . -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_BUILD_TYPE=Debug
cmake --build Debug --config Debug -j20
```
