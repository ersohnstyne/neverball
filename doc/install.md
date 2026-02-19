# Neverball 

## Requirements

| Name      |                                        |
| --------- | -------------------------------------- |
| SDL 2.0   | https://github.com/libsdl-org/SDL      |
| SDL2_ttf  | https://github.com/libsdl-org/SDL_ttf  |
| libvorbis | https://xiph.org/vorbis                |
| libpng    | https://libpng.org/pub/png/libpng.html |
| libjpeg   | https://ijg.org                        |
| libcurl   | https://www.gnu.org/software/gettext/  |
| libintl   | https://curl.se/                       |

You will also need to download OpenDriveAPI for each development platforms. This project code library is available from the OneDrive:

| Name           |                                                           |
| -------------- | --------------------------------------------------------- |
| Source code    | https://1drv.ms/u/s!Airrmyu6L5eynGj7HtYcQU_0ERtA?e=9XU5Zp |
| Windows 11 SDK | https://1drv.ms/u/s!Airrmyu6L5eynH3sBvBMPVLzPNEi?e=IEkpm8 |

By default during compilation, it is placed in the file, when running the script under Unix or Linux:

```shell
install-devel-linux.sh
```

Under Msys2 for Windows, it is placed in the file:

| Name      |                                          |
| --------- | ---------------------------------------- |
| Msys2     | `install-deps-msys2-<architecture>.sh`   |

Any kind of different platforms like WINE for Fedora does not supported.

### Ubuntu / Debian

```shell
sudo apt install build-essential libsdl2-dev libsdl2-ttf-dev libcurl4-openssl-dev libjpeg-dev libpng-dev libvorbis-dev
```

### Fedora

```shell
sudo dnf install @development-tools SDL2-devel SDL2_ttf-devel libcurl-devel libjpeg-turbo-devel libpng-devel libvorbis-devel
```

## Compilation

Package downloads is automatically disabled, if cURL package is not installed or missing.

Compiled source object file will automatically kicked, when building the game from Microsoft Visual Studio 2026.

Under Unix and Linux, simply run:

```shell
make
```

### Windows (Visual Studio 2026 / MSYS2)

Under Windows, either build from the Visual Studio 2026 solution, or install the MSYS2 environment and run:

```shell
make CRT_SECURE_NO_WARNINGS=1 ALTERNATIVE_BUILDENV=Msys2
```

### Cross-compilation for Windows (Fedora example)

Cross-compilation for Windows is also supported. On Fedora Linux, run:

```shell
make sols clean-src
mingw32-make -o sols PLATFORM=mingw
```

## Optional features

Optional features can be enabled at compile time by passing one or more additional arguments to make. Most of these features require additional libraries to be installed.

### Microsoft Elite Development Lifecycle for Windows

```shell
make CRT_SECURE_NO_WARNINGS=0
```

- Enabled by default.
- Requires Visual Studio 2026 Community or later.

### Filesystem V1

Time travel back to the original standard I/O without ZIP archives:

```shell
make FS_VERSION=1
```

- Payments, fetch and libcurl will ignored.

### Steam for Windows 11 and Linux

```shell
make ENABLE_STEAM=1
```

Dependency: Steamworks: https://partner.steamgames.com/

### PhysicsFS

Alternate file system backend that replaces standard I/O with PhysicsFS:

```shell
make ENABLE_FS=physfs
```

- Requires FS_VERSION=1.
- May causes some bugs or crashes.

Dependency: PhysicsFS: https://icculus.org/physfs/

### Native language support

Disable NLS:

```shell
make ENABLE_NLS=0
```

- Enabled by default.
- Requires an additional library on non-GLIBC systems.
- Dependency: libintl (GNU gettext): https://www.gnu.org/software/gettext/

### Editions for Windows and Linux

Recipes for Disaster Edition:

```shell
make ENABLE_RFD=1
```

- Must have provided filename: rfdconf.txt
- Requires an administrator permissions to modify recipes

### Paypal for all platforms

Enable payment game:

```shell
make ENABLE_IAP=paypal
```

- Requires libcurl for Windows or Linux (see below).
- Dependency: Paypal Developer: https://developer.paypal.com/, libcurl: https://curl.se/libcurl/

### Package downloads

Use libcurl for package downloads (enabled by default). Set this to any other value (e.g. `ENABLE_FETCH=0`) to disable downloads:

```shell
make ENABLE_FETCH=curl
```

- Dependency: libcurl: https://curl.se/libcurl

Use Google Drive with libcurl for package downloads on same time:

```shell
make ENABLE_FETCH=curl_gdrive
```

- Requires libcurl for Windows or Linux.
- Set this to Google Drive only, if you don't have custom website URL (e.g., ENABLE_FETCH=curl_gdrive_only).

### Tilt input devices

Nintendo Wii Remote support on Linux:

```shell
make ENABLE_TILT=wii
```

Depencency: BlueZ: https://www.bluez.org/, libwiimote https://libwiimote.sourceforge.net/

Hillcrest Labs Loop support:

```shell
make ENABLE_TILT=loop
```

Dependency: libusb-1.0: https://libusb.org/wiki/Libusb1.0, libfreespace: https://libfreespace.hillcrestlabs.com/

### Head-mounted display (HMD)

Head-mounted display support (including the Oculus Rift).

```shell
make ENABLE_HMD=openhmd
```

Dependency: OpenHMD: https://openhmd.net/

Oculus Rift support:

```shell
make ENABLE_HMD=libovr
```

Dependency: Oculus SDK: https://developer.oculusvr.com/

### Radiant console output

Map compiler output to Radiant console:

```shell
make ENABLE_RADIANT_CONSOLE=1
```

Dependency: SDL2_net: https://www.libsdl.org/projects/SDL_net/

## Installation

By default, an uninstalled build may be executed in place. A system-wide installation on Linux would probably copy the game to `/opt/pennyball`.

To be able to use the NetRadiant level editor, the game data and binaries must be installed in the same location. Distributions that wish to package Pennyball, and their shared data separately should take care to use symlinks and launcher scripts to support this.

## Distribution

The `dist` subdirectory contains some miscellaneous files:

- `.desktop` files
- high resolution icons in PNG, SVG and ICO formats

## Editions

If you've recently logged in into [Pennyball + Neverball Game Core Launcher](https://pennyball.stynegame.de), run this command:

```shell
make EDITION=home
```

Visit Discord Server [**Pennyball + Neverball**](https://discord.gg/qnJR263Hm2) and verify, to upgrade the game edition. Once upgraded, you can use by following the valid editions:

| Name              | Command line options             |
| ----------------- | -------------------------------- |
| Professional      | `make EDITION=pro`               |
| Enterprise        | `make EDITION=enterprise`        |
| Education         | `make EDITION=education`         |
| Server Essentials | `make EDITION=server_essentials` |
| Server Standard   | `make EDITION=server_standard`   |
| Server Datacenter | `make EDITION=server_datacenter` |

🌐 *Web: https://neverball.org/*