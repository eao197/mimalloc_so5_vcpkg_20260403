# Demo of problem with mimalloc heap in several threads

Steps to reproduce:

```sh
git clone https://github.com/microsoft/vcpkg
cd vcpkg
./bootstrap-vcpkg.sh
cd ..
git clone https://github.com/eao197/mimalloc_so5_vcpkg_20260403
cd mimalloc_so5_vcpkg_20260403
mkdir cmake_build
cd cmake_build
VCPKG_ROOT=../../vcpkg cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
./mimalloc_so5_case
```
