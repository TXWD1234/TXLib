## Require:
- Arch Linux:
```shell
sudo pacman -S glfw
cd <projectDir>
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg_root]/scripts/buildsystems/vcpkg.cmake
```
- Windows:
```shell
vcpkg install glfw3
```