[suu面具网盘仓库](https://www.lanzouq.com/b0aenhnib)  
[Github Wiki](https://github.com/4accccc/vivo-Magisk-suu/wiki)
# 这是suu版本面具
# 这个版本的面具使用"suu"命令获得root授权
# 为了那些想要root自己vivo手机又不想让自己手机变砖的人
# 你可以跟着下面的`编译教程`来自己编译Magisk，或者去`Releases`界面下载现成的
# 好好享受! :)


![](docs/images/logo.png)

[![Downloads](https://img.shields.io/badge/dynamic/json?color=green&label=Downloads&query=totalString&url=https%3A%2F%2Fraw.githubusercontent.com%2Ftopjohnwu%2Fmagisk-files%2Fcount%2Fcount.json&cacheSeconds=1800)](https://raw.githubusercontent.com/topjohnwu/magisk-files/count/count.json)

#### 此软件并不受谷歌官方支持
#### 此版本的Magisk也不受Magisk官方支持

## Introduction & 简介

Magisk 是由一系列开源应用组成的用来自定义安卓的应用，支持安卓6.0+。<br>
一些突出的特点如下

- **MagiskSUU**: 为应用提供root访问权限
- **Magisk Modules**: 通过安装模块来修改只读分区
- **MagiskBoot**: 用来解包和重打包安卓boot镜像的最完善的工具
- **Zygisk**: 在每个安卓应用进程中运行自定义代码
- **MagiskHide**: 将Magisk从root检测 / 系统完整性检查中隐藏 **(此功能仅限Magisk 23.0之前的版本或Delta/Kitsune版本面具)**


## Bug Reports & 反馈Bugs

用这个版本就别想着反馈bug了，他们不会管的

## Building and Development & 编译教程

- 首先安装Android Studio以及git
- 打开cmd输入`git clone --recurse-submodules https://github.com/4accccc/vivo-Magisk-Delta-suu.git`
- 安装 Python 版本3.8+ \
  (Windows 用户: 选中 **'Add Python to PATH'**) \
  (Windows 可选)安装完成后命令行输入`pip install colorama`
- 调整系统设置使程序能够使用Android Studio自带的JDK:
  - macOS: `export JAVA_HOME="/Applications/Android Studio.app/Contents/jre/Contents/Home"`
  - Linux: `export PATH="/path/to/androidstudio/jre/bin:$PATH"`
  - Windows: 将 `C:\Path\To\Android Studio\jre\bin` 加入环境变量 `PATH` \
  也可在网上下载jdk17并安装，将 `X:\Path\To\Java\jdk-17\bin` 加入环境变量 `PATH`
- 新建名为 `ANDROID_SDK_ROOT` 的环境变量，变量值输入Android SDK 文件夹对应地址 (Android Studio设置里能找到)
- 运行`./build.py ndk`以下载ndk
- 运行`./build.py -r all`开始构建 Magisk

## Signing and Distribution & 签名以及分发

- 用来签名 Magisk 应用的证书也会被直接内嵌于一些可执行文件中。在发行版(release)中，Magisk 的 root 守护进程(daemon)将强制检查应用数字证书并且会卸载任何签名不符的Magisk软件以使用户远离恶意以及未经认证的 Magisk 应用。
- 若你想对 Magisk 本身作任何开发，请使用 **官方的debug版并且重新安装 Magisk** 以绕过签名检查
- 如果你想分发使用你自己签名证书的Magisk应用，请在配置文件`config.prop`中设置你的签名
- 请阅 [谷歌官方开发者文档](https://developer.android.com/studio/publish/app-signing.html#generate-key) 以获取更多关于生成你自己的签名的详情

## Translation Contributions & 贡献翻译

Magisk 应用的默认字符资源存储在以下位置:

- `app/src/main/res/values/strings.xml`
- `stub/src/main/res/values/strings.xml`

翻译他们然后放入以下位置 (`[module]/src/main/res/values-[语言]/strings.xml`).

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
