> [init submodules]
> mdkir build
> cd build
> conan install -pr=../profiles/gcc_rel ..
> cmake ..

For Clang:
> conan install -pr=../profiles/clang_rel ..
> export CC=/usr/bin/clang
> export CXX=/usr/bin/clang++
> cmake ..

For Release:
> -DCMAKE_BUILD_TYPE=Release