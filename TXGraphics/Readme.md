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

# Notes

## Convensions
In `gl_core`, things should be wrapper oriented, tenplated to get the highest base level performance, and use disable copy + move semantics everywhere.
In `render_engine`, things should be API oriented, allowing type erasure, and function `init()`.