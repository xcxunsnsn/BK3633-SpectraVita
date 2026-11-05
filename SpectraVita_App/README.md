# SpectraVita 上位机源码

版本：1.0.0+1。支持 Windows 和 Android。

两端共用 `lib/` 中的 Flutter 界面、BLE 控制、协议解析和数据导出逻辑。`windows/` 与 `android/` 分别提供平台工程，无需维护两份业务代码。用 VS Code 或 Android Studio 打开本目录（含 `pubspec.yaml`），即可继续开发。

## 工程目录

| 路径 | 内容 |
| --- | --- |
| `lib/main.dart` | 应用入口、主题与中文字体 |
| `lib/screens/home_page.dart` | 首页、五类传感器页面、导航、图表和表格 |
| `lib/services/ble_controller.dart` | BLE 扫描连接、命令响应、周期计划、单次采集及历史上传 |
| `lib/protocol/spectravita_protocol.dart` | V4 帧、命令构造、数据模型、单位换算和 FIFO 主频计算 |
| `lib/services/data_exporter.dart` | CSV 生成、Windows 另存为、Android 系统分享 |
| `test/` | 协议、采集控制、CSV 和界面测试 |
| `assets/` | 思源黑体 Regular/Bold 字体和字体许可证 |
| `android/` | Android 权限、原生入口、Gradle 工程及 Wrapper |
| `windows/` | Windows 原生窗口与 CMake 工程 |
| `pubspec.yaml` / `pubspec.lock` | 项目依赖声明及锁定版本 |

## 开发环境

工程原验证环境为 Flutter 3.47.2 stable、Dart 3.13.2；`pubspec.yaml` 要求 Dart `^3.13.2`。建议先使用相同版本，避免同时升级依赖和构建工具。

- Windows：安装 Visual Studio 2022，选择“使用 C++ 的桌面开发”工作负载和 Windows SDK；在 Windows 设置中开启开发人员模式，以支持 Flutter 插件符号链接。
- Android：安装 Android Studio、Android SDK 和 Command-line Tools。工程使用 Gradle 9.3.1、Android Gradle Plugin 9.1.0、Kotlin 2.4.0；原验证环境使用 Android Studio 自带 JDK 25.0.2，源码编译目标为 Java 17。
- 将 Flutter 的 `bin` 目录加入 PATH。SDK 可以放在任意磁盘，不需要与原开发电脑路径一致。
- 首次配置需要网络下载 Pub、Gradle 和 Android 依赖。

在本目录执行：

```powershell
flutter --version
flutter doctor -v
flutter pub get
flutter analyze
flutter test
```

若 Flutter 未识别 Android SDK，可执行 `flutter config --android-sdk "实际SDK目录"`。Android 许可证按 `flutter doctor --android-licenses` 的提示阅读并处理。

压缩包不包含 `android/local.properties`、`.dart_tool` 和本机插件路径；通过 Flutter 命令配置/构建时会按当前电脑环境生成。首次打开时先运行 `flutter pub get`，不要直接使用原电脑的缓存或单独运行 Gradle。

## Windows 运行与构建

```powershell
flutter run -d windows
flutter build windows --release
```

构建输出位于 `build/windows/x64/runner/Release/`。分发程序时需要整个 Release 目录，包括 EXE、DLL 和 `data/`，不能只复制 `spectravita_app.exe`。

## Android 运行与构建

手机打开开发者选项和 USB 调试，连接电脑后执行：

```powershell
flutter devices
flutter build apk --debug
```

安装包位于 `build/app/outputs/flutter-apk/app-debug.apk`。也可用 `flutter run -d 设备ID` 直接在真机运行。BLE 功能需在支持蓝牙的真机上验证；Android 12 及以上需允许“附近的设备”权限，旧版本按系统提示允许蓝牙扫描所需权限。

当前 Android Release 配置仍使用 debug 签名。用于正式分发前，应另行配置自己的签名文件和版本号；签名密钥不在本包内。

## 使用流程

1. 扫描并连接广播名为 `SpectraVita`、提供 `FEE0` 服务的设备。
2. 首页可启动五类周期计划；已有类型运行时只启动剩余类型。传感器页可单独启动对应类型。
3. 普通单次测量作用于当前页面，结果显示在“最近一次单次结果”中，不混入 Flash 历史或 CSV。
4. 多光谱、FIFO、紫外及接近支持增益选择；启动对应周期计划前，会先做一次单次预测量，再发送周期命令。接近 B 通道若饱和，会逐级降低增益后重测。
5. 在首页停止全部周期采集后，按类型逐一上传 Flash 历史。上传期间不要同时执行采集命令。
6. 上传数据可查看图表和表格并导出 CSV。Windows 弹出“另存为”，Android 打开系统分享面板，由用户选择文件管理器、邮件等可用目标。

周期采集可在 BLE 断开后继续。连接断开时界面保留最后已知运行类型并标记状态未知；重新连接后先在首页停止全部，再操作采集或上传。

## 通信与数据

设备使用 BK3633 和 V4 应用协议。APP 为 BLE Central / GATT Client，`FEE2` 用于 Write Without Response 下发命令，`FEE4` 用于 Notify 接收。

帧格式为 `AA | CMD | DATA | XOR | BB`，XOR 覆盖 CMD 和 DATA，多字节数值使用小端序。

| 类型 | 单次请求/响应 | 单次数据长度 | 历史记录长度 | 周期/速率 |
| --- | --- | ---: | ---: | --- |
| 多光谱 | `0x42 / 0x43` | 20 字节 | 22 字节（含增益） | 60 s |
| 多光谱 FIFO | `0x44 / 0x45` | 256 字节 | 256 字节 | 600 s |
| 紫外 | `0x46 / 0x47` | 8 字节 | 8 字节 | 60 s |
| 接近 | `0x48 / 0x49` | 4 字节 | 4 字节 | 600 s |
| 加速度 | `0x4A / 0x4B` | 6 字节 | 7 字节（含 LOW/HIGH 状态） | LOW 1 Hz / HIGH 50 Hz |

- 周期计划为 `0x4E / 0x4F`；上传为 `0x50 / 0x51`；全局停止为 `0x52 / 0x53`。
- 多光谱：`ATIME=29`、`ASTEP=599`，默认增益512×，可选0.5×～512×。两次通道测量组合为F1～F8、Clear和NIR。
- FIFO：`FD_TIME=720`，默认增益16×，可选0.5×～512×；128个原始计数，点间隔2.0016 ms。峰峰值小于5 count时不显示主频。历史记录不包含增益及异常时的有效点数。
- 紫外：默认512×，可选1×～2048×；UVA/UVB/UVC由固件换算为整数 `µW/cm²`，有符号温度除以10后为℃。历史记录不包含增益代码。
- 接近：A固定1×，B默认8×，可选1×/2×/4×/8×；`PS_TIME=7`时满量程2047，`0xFFFF`表示无效数据。原始计数未经距离标定，不能直接换算为毫米。
- 加速度：有符号原始值乘0.0039得到g，再乘9.80665得到 `m/s²`。
- CSV为UTF-8 BOM，保留包Unix时间和原始HEX，并附逐条估算Unix时间及本地时间。HIGH加速度显示毫秒，其余显示到秒；逐条时间根据包时间和采样间隔重建，并非每条独立硬件时间戳。

`0x51` 不带传感器类型、包序号、总包数或结束标志，必须串行上传；当前按末包记录数或2秒通知静默判断结束。大帧依赖足够的ATT MTU。完成上传后固件会清除该类型历史，使用时应及时保存导出数据。

## 第三方资源

中文字体为 Adobe Source Han Sans CN，许可为 SIL Open Font License 1.1，见 `assets/licenses/SourceHanSans-LICENSE.txt`。字体及许可一并保留。Flutter 插件依赖和锁定版本见 `pubspec.yaml` 与 `pubspec.lock`，第三方组件遵循各自许可证。
