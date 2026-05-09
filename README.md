# **TXLib**
**A light C/C++ library for game, graphics, and general purposes.**

## Installation
Download `add_txlib.cmake`:

[TXLib: add_txlib.cmake](https://raw.githubusercontent.com/TXWD1234/TXLib/add_txlib.cmake)

or

```bash
cd "<your_project_directory>"
curl -O https://raw.githubusercontent.com/TXWD1234/TXLib/add_txlib.cmake
```

Place it in your project directory, then include it in your project CMakeLists.txt, before `add_executable()`:

```cmake
include("add_txlib.cmake")
tx_add_txlib()
```

Then link TXLib to your project:

```cmake
target_link_libraries(<your_project_name> PUBLIC TXLib)
```