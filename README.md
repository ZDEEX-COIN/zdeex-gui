# ZdeeX

## What is ZdeeX?

![Logo](https://explorer.zdeex.org/assets/images/logo/logo.png "Logo")

## Compiling from source

SilentDragon is written in C++ 14, and can be compiled with g++/clang++/visual
c++. It also depends on Qt5, which you can get from [here](https://www.qt.io/download)
or within your operating system package manager. Note that if you are compiling
from source, you won't get the embedded hushd by default. You can either run an external
hushd, or compile hushd as well.

### Building on Linux

#### Linux Troubleshooting
If you run into an error with OpenGL, you may need to install extra deps. More details [here](https://gist.github.com/shamiul94/a632f7ab94cf389e08efd7174335df1c)

**Error**
```
/usr/bin/ld: cannot find -lGL
clang: error: linker command failed with exit code 1 (use -v to see invocation)
```
**Solution**
```
sudo apt-get -y install libglu1-mesa-dev freeglut3-dev mesa-common-dev
```

#### Ubuntu 18.04 and 20.04

You can install the pre-reqs and build on Ubuntu 18.04 & 20.04 with:

```shell script
sudo apt-get -y install qt5-default qt5-qmake qtcreator
git clone https://github.com/ZDEEX-COIN/zdeex-gui
cd zdeex-gui
./build.sh linguist
./build.sh
./silentzdeex
```

#### Arch Linux

You can install the pre-reqs and build on Arch Linux with:

```shell script
sudo pacman -S qt5-base qt5-tools qtcreator rust
git clone https://github.com/ZDEEX-COIN/zdeex-gui
cd zdeex-gui
./build.sh linguist
./build.sh release
./silentzdeex
```

### Building on Windows
You need Visual Studio 2017 (The free C++ Community Edition works just fine).

From the VS Tools command prompt
```shell script
git clone  https://github.com/ZDEEX-COIN/zdeex-gui
cd zdeex-gui
c:\Qt5\bin\qmake.exe silentzdeex.pro -spec win32-msvc CONFIG+=debug
nmake

debug\SilentZdeex.exe
```

To create the Visual Studio project files so you can compile and run from Visual Studio:
```shell script
c:\Qt5\bin\qmake.exe silenzdeex.pro -tp vc CONFIG+=debug
```

### Building on macOS

You need to install the Xcode app or the Xcode command line tools first, and then install Qt. 

TODO: Suggestions on installing qt5 deps on a Mac

```shell script
git clone https://github.com/ZDEEX-COIN/zdeex-gui
cd zdeex-gui
# These commands require qmake to be installed
./build.sh linguist
./build.sh
make

./SilentDragon.app/Contents/MacOS/SilentZdeex
```

### Building SilentZdeeX


```
git clone https://github.com/ZDEEX-COIN/zdeex-gui
cd zdeex-gui
./build-sdx.sh
./build-sdx.sh linguist # update translations, might not be needed
```

The binary will be called `silentzdeex`

### Emulating the embedded node

In binary releases, SilentZdeex will use node binaries in the current directory to sync a node from scratch.
It does not attempt to download them, it bundles them. To simulate this from a developer setup, you can symlink
these four files in your Git repo:

```shell script
ln -s ../zdeex/src/zdeexd
ln -s ../zdeex/src/zdeex-cli
```

The above assumes silentdragon and hush3 git repos are in the same directory. File names on Windows will need to be tweaked.

## Where is my wallet stored?

Linux: `~/.hush/ZDEEX`

Windows 10: `C:\Documents and Settings\%user\Application Data\Hush\ZDEEX` or `C:\Users\%user\AppData\Roaming\Hush\ZDEEX`

Mac : `~/Library/Application Support/Hush/ZDEEX`

## Support

For support or other questions, join us on [ZdeeX](https://zdeex.org)

## License

GPLv3 or later
