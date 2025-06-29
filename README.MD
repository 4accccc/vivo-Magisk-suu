# IMPORTANT NOTE: I(@4accccc) have no passion for this project anymore.@Winkmoon will take my place.
# plz go and follow @Winkmoon :3

[中文用户往这里看过来](https://github.com/4accccc/vivo-Magisk-suu/blob/master/README_Chinese.MD)  
[Github Wiki](https://github.com/4accccc/vivo-Magisk-suu/wiki)  
[my suu magisk netdisk](https://www.lanzouq.com/b0aenhnib)  
# This is the "suu" version of Magisk.
# This version of Magisk use "suu" command to get root permission. 
# For those vivo users who want to root their vivo phones without being bricked.
# You can follow `Building and Development` below to build your own Magisk(suu) or download the prebuilt apk file in `Release`
# Enjoy! :)


![](docs/images/logo.png)

[![Downloads](https://img.shields.io/badge/dynamic/json?color=green&label=Downloads&query=totalString&url=https%3A%2F%2Fraw.githubusercontent.com%2Ftopjohnwu%2Fmagisk-files%2Fcount%2Fcount.json&cacheSeconds=1800)](https://raw.githubusercontent.com/topjohnwu/magisk-files/count/count.json)

#### This is not an officially supported Google product
#### ALSO IT IS NOT AN OFFICIALLY SUPPORTED VERSION OF MAGISK

## Introduction

Magisk is a suite of open source software for customizing Android, supporting devices higher than Android 6.0.<br>
Some highlight features:

- **MagiskSUU**: Provide root access for applications
- **Magisk Modules**: Modify read-only partitions by installing modules
- **MagiskBoot**: The most complete tool for unpacking and repacking Android boot images
- **Zygisk**: Run code in every Android applications' processes
- **MagiskHide**: Hide Magisk from root detections / system integrity checks **(Only avaliable before Magisk 23.0 or Delta/Kitsune Mask)**

## Downloads

[Github](https://github.com/4accccc/vivo-Magisk-suu/) is NOT the only source where you can get official Magisk (suu) information and downloads.

## Useful Links

- [Installation Instruction](https://topjohnwu.github.io/Magisk/install.html)
- [Magisk Documentation](https://topjohnwu.github.io/Magisk/)
- [Libsuu](https://github.com/4accccc/libsuu)

## Bug Reports

DO NOT REPORT BUGS IF YOU ARE USING THIS VERSION OF MAGISK!
THE OFFICAL MAGISK DOESNT HAVE A SUU VERSION SO PLZ STOP! 

## Building and Development

- Magisk builds on any OS Android Studio supports. Install Android Studio and do the initial setups. \
  (Windows only: **Enable developer mode**. This is required because we need symbolic link support.)
- Install Git:
  On Unix, install git with your favorite package manager \
  On Windows, download the install the latest Git version on the official website.Make sure to **“Enable symbolic links”** during installation.
- Clone sources: `git clone --recurse-submodules https://github.com/4accccc/vivo-Magisk-suu.git`
- Install Python 3.8+ \
  (Windows only: select **'Add Python to PATH'** in installer.) \
  (Optional on Windows): Run **pip install colorama** to install the colorama python package.
- Configure to use the JDK bundled in Android Studio: \
  The recommended option is to set environment variable **ANDROID_STUDIO** to the path where your Android Studio is installed. The build script will automatically find and use the bundled JDK. \
  You can also setup JDK 17 yourself, but this guide will not cover the instructions.
- Set environment variable **ANDROID_HOME** to the Android SDK folder. This path can be found in Android Studio settings.
- Run `./build.py ndk` to let the script download and install NDK for you
- To start building, run `build.py -r all`
- To start development, open the project with Android Studio. The IDE can be used for both app (Kotlin/Java) and native sources.
- Optionally, set custom configs with `config.prop`. A sample `config.prop.sample` is provided.

## Signing and Distribution

- The certificate of the key used to sign the final Magisk APK product is also directly embedded into some executables. In release builds, Magisk's root daemon will enforce this certificate check and reject and forcefully uninstall any non-matching Magisk apps to protect users from malicious and unverified Magisk APKs.
- To do any development on Magisk itself, switch to an **official debug build and reinstall Magisk** to bypass the signature check.
- To distribute your own Magisk builds signed with your own keys, set your signing configs in `config.prop`.
- Check [Google's Documentation](https://developer.android.com/studio/publish/app-signing.html#generate-key) for more details on generating your own key.

## Translation Contributions

Default string resources for the Magisk app and its stub APK are located here:

- `app/src/main/res/values/strings.xml`
- `stub/src/main/res/values/strings.xml`

Translate each and place them in the respective locations (`[module]/src/main/res/values-[lang]/strings.xml`).

## License

    Magisk, including all git submodules are free software:
    you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software Foundation,
    either version 3 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
