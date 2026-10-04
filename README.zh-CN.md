# MetricBar

[English](README.md) | [简体中文](README.zh-CN.md)

**把任意 JSON 接口的指标，以实时文字直接显示在 Windows 10 任务栏上。**

![截图占位](docs/screenshot-placeholder.svg)

> 截图/GIF 占位：发布 GitHub Release 前，请换成 Windows 10 任务栏的实际截图或录屏。

## 功能特性

- 原生 x64 Windows DeskBand，指标直接嵌入任务栏，而不是挤在 16×16 托盘图标里。
- 把嵌套 JSON 展平为 `status.online` 等点号路径；数组可用 `items.0.value`。
- 指标顺序、模板、分隔符及原始值/字节格式均由 INI 配置。
- 从旧版 `vps_net`（如 `↑2.7Mbps ↓2.5Mbps`）派生 `net.up` 与 `net.down`。
- 字段缺失或格式错误时仅对应指标显示 `--`，不影响其他指标。
- 没有 `[Metrics]` 段时回退到 V2 的 `DisplayMode` 行为。
- WinHTTP 在后台线程轮询，单阶段超时 8 秒，并在网络、线程和窗口边界隔离异常。
- 保留 V2 的 CLSID 和 `VpsTraySpeed.dll` 文件名，可原地升级。

## 支持范围

仅支持 **64 位 Windows 10**。Windows 11 已移除传统任务栏 DeskBand UI，因此不支持 Windows 11。

## 安装

1. 把压缩包完整解压到长期保留的目录，例如 `C:\Tools\MetricBar`。
2. 右键 `install.bat`，选择“以管理员身份运行”。
3. 如果工具栏列表未刷新，重启 Explorer 或注销后重新登录。
4. 右键任务栏，打开“工具栏”，勾选 **MetricBar**。

单击文字可打开 `HomepageUrl`。若数据中存在旧版 VPS 字段，悬停显示上下行、CPU 和在线数；通用 JSON 则显示渲染后的指标。

### 从 V2 升级

COM CLSID 未改变。先取消勾选工具栏并重启 Explorer，使旧 DLL 卸载；随后覆盖原目录中的 `VpsTraySpeed.dll`。再运行一次新版 `install.bat`，把注册表显示名从 `VpsTraySpeed` 或 `TaskbarJsonMonitor` 更新为 `MetricBar`，最后重新启用工具栏。

## 配置

`config.ini` 与 DLL 位于同一目录。修改后需重启 Explorer 或注销重进。

默认配置：

```ini
[MetricBar]
DataUrl=http://23.94.171.107:18099/homepage.json
RefreshSeconds=10
DisplayMode=both
TextColor=auto
ErrorColor=#FF4848
FontSize=10
Width=220
HomepageUrl=http://23.94.171.107:3002/

[Metrics]
Count=3
Separator=

[M1]
Path=net.down
Template=↓{value}
Format=raw

[M2]
Path=net.up
Template=↑{value}
Format=raw

[M3]
Path=vps_cpu
Template=CPU {value}%
Format=raw
```

样例效果：`↓2.5M ↑1.1M CPU 16.4%`。

### 指标字段

- `Count`：指标组数量，范围 1–16；非法值回退到 3。
- `Separator`：指标之间的文字；空值默认一个空格。自定义值如需保留两边空格，可写为 `Separator=" | "`。
- `Path`：展平后的字段名。对象用点号，数组使用数字段。
- `Template`：所有 `{value}` 会被替换为格式化结果；非法模板会回退到安全默认值。
- `Format=raw`：保留 JSON 字符串及数字原文。
- `Format=bytes`：按 1024 进制换算，一位小数；`1536` → `1.5K`，`2097152` → `2.0M`。

顶层标量 JSON 使用路径 `$`。布尔值和 null 分别显示 `true`、`false`、`null`。字段缺失，或把非数字字段用于 `bytes` 时，显示 `--`。

### V2 回退

删除完整的 `[Metrics]` 段以及 `[M1]`、`[M2]` 等组，即按 `DisplayMode=download|upload|both` 渲染旧版 `vps_net`。为兼容升级，旧 `[TaskbarJsonMonitor]` 和 `[VpsTraySpeed]` 主配置段仍可读取；新配置应使用 `[MetricBar]`。

## 第三方 JSON 示例

### GitHub REST API 限额

GitHub 在[官方 REST API 文档](https://docs.github.com/zh/rest/rate-limit/rate-limit)中说明了 `GET /rate_limit` 及嵌套的 `resources.core` 对象。

```ini
[MetricBar]
DataUrl=https://api.github.com/rate_limit
RefreshSeconds=60
Width=230
HomepageUrl=https://github.com/

[Metrics]
Count=2
Separator=" / "

[M1]
Path=resources.core.remaining
Template=GitHub {value}
Format=raw

[M2]
Path=resources.core.limit
Template={value}
Format=raw
```

### Open-Meteo 当前天气

Open-Meteo 在[官方预报 API 文档](https://open-meteo.com/en/docs)中说明了当前温度和相对湿度字段。

```ini
[MetricBar]
DataUrl=https://api.open-meteo.com/v1/forecast?latitude=52.52&longitude=13.41&current=temperature_2m,relative_humidity_2m
RefreshSeconds=300
Width=210
HomepageUrl=https://open-meteo.com/

[Metrics]
Count=2
Separator="  "

[M1]
Path=current.temperature_2m
Template={value}°C
Format=raw

[M2]
Path=current.relative_humidity_2m
Template=湿度 {value}%
Format=raw
```

## 卸载

1. 在任务栏“工具栏”菜单取消勾选 **MetricBar**。
2. 右键 `uninstall.bat`，选择“以管理员身份运行”。
3. 重启 Explorer 或注销后，再删除安装目录。

## 构建与测试

交付版本使用 WSL2 Ubuntu 22.04 和 MinGW-w64：

```bash
sudo apt-get install g++-mingw-w64-x86-64 binutils-mingw-w64-x86-64 make
```

PowerShell 中运行：

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\build.ps1
```

构建脚本会运行解析/指标测试、COM 接口合约测试、无需注册表的 DeskBand 集成宿主测试，并验证 DLL 必须为 x64 PE；测试过程不写注册表。

## 项目命名

命名记录：

1. **MetricBar**（已选）：简短、好记，适合任务栏指标条。
2. **JsonTaskbar**：能体现 JSON，但监控含义不够直接。
3. **TaskbarJsonMonitor**：描述准确，但因过长而弃用。

## GitHub 发布

本地仓库已准备好，但不会自动推送。完整命令见 [PUBLISHING.md](PUBLISHING.md)。

## 许可证

[MIT](LICENSE)
