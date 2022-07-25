 SqUirRel Game Engine

# Debug Build instructions

```
mkdir Debug && cd Debug

conan install ../conan --remote=conancenter --build missing --profile ../conan/toolchain-gcc-12-release
cmake .. -G "Unix Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug -DSURGE_BUILD_TESTING=ON

OR

conan install ../conan --remote=conancenter --build missing --profile ../conan/toolchain-clang-13-release
cmake .. -G "Unix Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug -DSURGE_BUILD_TESTING=ON -DCMAKE_CXX_COMPILER=/usr/bin/clang++

cmake --build . -j20
```

# Sanitizer supression

```
export ASAN_OPTIONS=suppressions=../configs/asan_supressions.sup
```

# Valgrind supression

```
valgrind --leak-check=full --show-reachable=yes --show-leak-kinds=all --error-limit=no --suppressions=../configs/sdl_valgrind_suppressions.sup ./bin/surge main.sqr --config-file=../configs/surge_config.yaml
```

# Perf with firefox
```
perf record -g -F 999 ./bin/surge ../configs/config.nut
perf script -F +pid > firefox.perf
```
View with (https://profiler.firefox.com/)

# TODO:
* Switch to memory pool/arena model
* Load and display images using scripts. Idea: Use a singleton image record