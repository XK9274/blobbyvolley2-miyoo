# Blobby Volley 2 [Miyoo Mini & Miyoo Mini Plus Edition] ![build workflow](https://github.com/danielknobe/blobbyvolley2/actions/workflows/main.yaml/badge.svg)

Ported to Miyoo Mini retro gaming devices by [@XK9274](https://github.com/XK9274) and [@Vitty85](https://github.com/Vitty85)

Bundled with the Beach Mod by [@lordelan](https://github.com/lordelan)

**The head-to-head multiplayer ball game**

### Website
 http://blobbyvolley.de

### System requirements
Either Windows 2000 or later, Linux or MacOS

### Dedicated Server
The "Dedicaded Server" runs with a Gamespeed of 100%, which means 75 FPS

The Port for the Server is 1234.

### Source Code
Clone the git repository:
```bash
git clone https://github.com/danielknobe/blobbyvolley2.git
```

### Build under Linux
Blobby Volley compiles with GCC 5 or newer, or Clang 10 or newer.
Other compilers may work but are currently untested. The minimum supported
CMake version is 3.7.

1. Install dependencies:

Debian-based Distros:
```bash
apt-get install g++ cmake libsdl2-dev libboost-dev libphysfs-dev
```
Arch-based Distros:
```bash
pacman -S gcc cmake sdl2 boost physfs
```
2. Compile:
```bash
cmake .
make
```
3. Run:
```bash
src/blobby
```

### Build under Windows 7 or newer and Visual Studio 2015 Update 3 or newer
1. Install vcpkg by following the instructions:
https://github.com/microsoft/vcpkg/blob/master/README.md

2. Install dependencies in project directory:
```powershell
.\vcpkg install
```

3. Configure, build and run:
https://github.com/microsoft/vcpkg/blob/master/docs/users/buildsystems/cmake-integration.md


### Build under MacOS
1. Install homebrew by following the instructions:
https://brew.sh

2. Install dependencies:
```bash
brew install sdl2 physfs boost
```
3. Compile:
```bash
cmake .
make
```
4. Run:
```bash
src/blobby
```

### Build for Nintendo Switch

1. [devkitPro](https://switchbrew.org/wiki/Setting_up_Development_Environment) needs to be installed and completely configured.

2. Install dependencies through devkitPro's package manager:

- SDL2
- PhysFS
- OpenGL

2. Compile:

```bash
cmake . -DCMAKE_TOOLCHAIN_FILE=NintendoSwitchToolchain.cmake -DCMAKE_INSTALL_PREFIX=blobby -DSWITCH=true -DCMAKE_BUILD_TYPE=Debug
make && make install
```

3. Copy the `blobby` folder to the `/switch/` directory on your SD card.

### Credits
See AUTHORS
