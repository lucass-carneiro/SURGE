 SURGE: Super UnderRated Game Engine

 A prototype game engine made for fun (and profit ?).

# Example Debug build on Linux

```
git clone https://github.com/lucass-carneiro/SURGE && cd SURGE
git submodule init && git submodule update
cp vcpkg.json.linux vcpkg.json
cmake -B Debug -S . -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug
cmake --build Debug
```

# Example Release build on Linux

```
git clone https://github.com/lucass-carneiro/SURGE && cd SURGE
git submodule init && git submodule update
cp vcpkg.json.linux vcpkg.json
cmake -B Release -S . -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build Release
```

# Example Debug build on Windows

```
git clone https://github.com/lucass-carneiro/SURGE
cd SURGE
git submodule init
git submodule update
cp vcpkg.json.windows vcpkg.json
cmake -B Debug -S . -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_BUILD_TYPE=Debug
cmake --build Debug --config Debug
```

# Example Release build on Windows

```
git clone https://github.com/lucass-carneiro/SURGE
cd SURGE
git submodule init
git submodule update
cp vcpkg.json.windows vcpkg.json
cmake -B Release -S . -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_BUILD_TYPE=Release
cmake --build Release --config Release
```