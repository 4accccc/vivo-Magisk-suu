# 这是suu版本面具
# 这个版本的面具使用"suu"命令获得root授权
# 为了那些想要root自己vivo手机又不想让自己手机变砖的人
# 你可以跟着下面的`Building and Development`来自己编译Magisk，或者去`Releases`界面下载现成的
# 好好享受! :)


![](docs/images/logo.png)

[![Downloads](https://img.shields.io/badge/dynamic/json?color=green&label=Downloads&query=totalString&url=https%3A%2F%2Fraw.githubusercontent.com%2Ftopjohnwu%2Fmagisk-files%2Fcount%2Fcount.json&cacheSeconds=1800)](https://raw.githubusercontent.com/topjohnwu/magisk-files/count/count.json)

#### 此软件并不受谷歌官方支持
#### 此版本的Magisk也不受Magisk官方支持

## 反馈Bugs

用这个版本就别想着反馈bug了，他们不会管的

## Building and Development
## 编译教程

- 首先安装Android Studio
- 安装git，打开cmd输入`git clone --recurse-submodules https://github.com/4accccc/vivo-Magisk-suu.git`
- 安装 Python 版本3.8+ \
  (Windows 用户: 选中 **'Add Python to PATH'**, 安装完成后命令行输入`pip install colorama`)
- 调整系统设置使程序能够使用Android Studio自带的JDK:
  - macOS: `export JAVA_HOME="/Applications/Android Studio.app/Contents/jre/Contents/Home"`
  - Linux: `export PATH="/path/to/androidstudio/jre/bin:$PATH"`
  - Windows: 将`C:\Path\To\Android Studio\jre\bin` 加入环境变量 `PATH`
- 新建名为 `ANDROID_SDK_ROOT` 的环境变量，变量值输入Android SDK 文件夹对应地址 (Android Studio设置里能找到)
- 运行`./build.py ndk`以下载ndk
- 运行`./build.py -r all`开始构建 Magisk

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
