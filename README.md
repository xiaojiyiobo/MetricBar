# MetricBar

[English](README.md) | [绠€浣撲腑鏂嘳(README.zh-CN.md)

**Show metrics from any JSON endpoint as live text in the Windows 10 taskbar.**

![MetricBar in the Windows 10 taskbar](docs/screenshot.png)



## Features

- Native x64 Windows DeskBand: metrics appear directly in the taskbar, not in a tiny tray icon.
- Flattens nested JSON into dot paths such as `status.online` and array paths such as `items.0.value`.
- Configurable metric order, templates, separators, and raw/byte formatting.
- Derives `net.down` and `net.up` from a legacy `vps_net` value such as `鈫?.7Mbps 鈫?.5Mbps`.
- A missing or malformed field renders as `--` without affecting other metrics.
- Without a `[Metrics]` section, the component falls back to the V2 `DisplayMode` behavior.
- WinHTTP polling runs on a background thread with an 8-second timeout and exception isolation.
- Keeps the V2 CLSID and `VpsTraySpeed.dll` filename for in-place upgrades.

## Platform support

MetricBar supports **64-bit Windows 10 only**. Windows 11 removed the classic taskbar DeskBand UI, so it is intentionally unsupported.

## Install

1. Extract the package to a permanent directory, for example `C:\Tools\MetricBar`.
2. Right-click `install.bat` and choose **Run as administrator**.
3. Restart Explorer or sign out and back in if the toolbar list has not refreshed.
4. Right-click the taskbar, open **Toolbars**, and enable **MetricBar**.

Clicking the text opens `HomepageUrl`. Hovering shows the legacy VPS details when those fields exist, or the rendered generic metrics otherwise.

### Upgrade from V2

The COM CLSID is unchanged. Disable the toolbar, restart Explorer so the old DLL is unloaded, and replace `VpsTraySpeed.dll` in the existing installation directory. Run the new `install.bat` once to refresh the toolbar display name from `VpsTraySpeed` or `TaskbarJsonMonitor` to `MetricBar`, then enable it again.

## Configuration

`config.ini` lives beside the DLL. Restart Explorer or sign out after editing it.

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
Template=鈫搟value}
Format=raw

[M2]
Path=net.up
Template=鈫憑value}
Format=raw

[M3]
Path=vps_cpu
Template=CPU {value}%
Format=raw
```

This renders `鈫?.5M 鈫?.1M CPU 16.4%` for the sample payload.

### Metric settings

- `Count`: number of metric groups, from 1 to 16. Invalid values fall back to 3.
- `Separator`: text inserted between metrics. An empty value uses one space. Use quotes to preserve surrounding spaces in a custom value, for example `Separator=" | "`.
- `Path`: flattened field name. Nested objects use dots; arrays use numeric segments.
- `Template`: replaces every `{value}` token with the formatted field. An invalid template falls back to a safe default.
- `Format=raw`: preserves JSON strings and number text.
- `Format=bytes`: converts numeric byte values using 1024-based units and one decimal place: `1536` 鈫?`1.5K`, `2097152` 鈫?`2.0M`.

Top-level scalar JSON values are available as `$`. Boolean and null leaves render as `true`, `false`, and `null`. Missing fields and non-numeric values used with `bytes` render as `--`.

### V2 fallback

Delete the complete `[Metrics]` section and all `[M1]`, `[M2]`, 鈥?groups to use the original `DisplayMode=download|upload|both` rendering of `vps_net`. The older `[TaskbarJsonMonitor]` and `[VpsTraySpeed]` main sections are still accepted for upgrade compatibility; new configurations should use `[MetricBar]`.

## Third-party JSON examples

### GitHub REST API rate limit

GitHub documents `GET /rate_limit` and its nested `resources.core` object in the [official REST API documentation](https://docs.github.com/en/rest/rate-limit/rate-limit).

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

### Open-Meteo current weather

Open-Meteo documents current temperature and relative humidity in its [official forecast API documentation](https://open-meteo.com/en/docs).

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
Template={value}掳C
Format=raw

[M2]
Path=current.relative_humidity_2m
Template=RH {value}%
Format=raw
```

## Uninstall

1. Disable **MetricBar** in the taskbar's **Toolbars** menu.
2. Right-click `uninstall.bat` and choose **Run as administrator**.
3. Restart Explorer or sign out before deleting the installation directory.

## Build and test

The release build uses WSL2 Ubuntu 22.04 and MinGW-w64:

```bash
sudo apt-get install g++-mingw-w64-x86-64 binutils-mingw-w64-x86-64 make
```

Then run from PowerShell:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\build.ps1
```

The build runs parser/metric tests, COM contract tests, an unregistered DeskBand integration host, and an x64 PE check. No registry changes are made by the test suite.

## Project name

Naming history:

1. **MetricBar** 鈥?selected: short, memorable, and appropriate for a taskbar metric strip.
2. **JsonTaskbar** 鈥?explicit about JSON but less clear about monitoring.
3. **TaskbarJsonMonitor** 鈥?descriptive, but rejected because it is too long.

## Publishing

This repository is prepared for GitHub but is not pushed automatically. See [PUBLISHING.md](PUBLISHING.md) for the exact commands.

## License

[MIT](LICENSE)
