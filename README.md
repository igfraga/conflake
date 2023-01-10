I wanted to try out LLVM, so I made this small, incomplete functional programming language.

To build:

- (init submodules)
- mdkir build
- cd build
- conan install -pr=../profiles/gcc_rel ..
- cmake ..

For Clang:
- conan install -pr=../profiles/clang_rel ..
- export CC=/usr/bin/clang
- export CXX=/usr/bin/clang++
- cmake ..

For Release:
- -DCMAKE_BUILD_TYPE=Release


For QtCreator, as of Sept 2021, I had to:
- run conan manually from QtCreator's generated build folder
- add QT_CREATOR_SKIP_PACKAGE_MANAGER_SETUP=ON cmake flag (this was failing)
