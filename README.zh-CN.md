# MetricBar

[English](README.md) | [绠€浣撲腑鏂嘳(README.zh-CN.md)

**鎶婁换鎰?JSON 鎺ュ彛鐨勬寚鏍囷紝浠ュ疄鏃舵枃瀛楃洿鎺ユ樉绀哄湪 Windows 10 浠诲姟鏍忎笂銆?*

![鎴浘鍗犱綅](docs/screenshot-placeholder.svg)

> 鎴浘/GIF 鍗犱綅锛氬彂甯?GitHub Release 鍓嶏紝璇锋崲鎴?Windows 10 浠诲姟鏍忕殑瀹為檯鎴浘鎴栧綍灞忋€?
## 鍔熻兘鐗规€?
- 鍘熺敓 x64 Windows DeskBand锛屾寚鏍囩洿鎺ュ祵鍏ヤ换鍔℃爮锛岃€屼笉鏄尋鍦?16脳16 鎵樼洏鍥炬爣閲屻€?- 鎶婂祵濂?JSON 灞曞钩涓?`status.online` 绛夌偣鍙疯矾寰勶紱鏁扮粍鍙敤 `items.0.value`銆?- 鎸囨爣椤哄簭銆佹ā鏉裤€佸垎闅旂鍙婂師濮嬪€?瀛楄妭鏍煎紡鍧囩敱 INI 閰嶇疆銆?- 浠庢棫鐗?`vps_net`锛堝 `鈫?.7Mbps 鈫?.5Mbps`锛夋淳鐢?`net.up` 涓?`net.down`銆?- 瀛楁缂哄け鎴栨牸寮忛敊璇椂浠呭搴旀寚鏍囨樉绀?`--`锛屼笉褰卞搷鍏朵粬鎸囨爣銆?- 娌℃湁 `[Metrics]` 娈垫椂鍥為€€鍒?V2 鐨?`DisplayMode` 琛屼负銆?- WinHTTP 鍦ㄥ悗鍙扮嚎绋嬭疆璇紝鍗曢樁娈佃秴鏃?8 绉掞紝骞跺湪缃戠粶銆佺嚎绋嬪拰绐楀彛杈圭晫闅旂寮傚父銆?- 淇濈暀 V2 鐨?CLSID 鍜?`VpsTraySpeed.dll` 鏂囦欢鍚嶏紝鍙師鍦板崌绾с€?
## 鏀寔鑼冨洿

浠呮敮鎸?**64 浣?Windows 10**銆俉indows 11 宸茬Щ闄や紶缁熶换鍔℃爮 DeskBand UI锛屽洜姝や笉鏀寔 Windows 11銆?
## 瀹夎

1. 鎶婂帇缂╁寘瀹屾暣瑙ｅ帇鍒伴暱鏈熶繚鐣欑殑鐩綍锛屼緥濡?`C:\Tools\MetricBar`銆?2. 鍙抽敭 `install.bat`锛岄€夋嫨鈥滀互绠＄悊鍛樿韩浠借繍琛屸€濄€?3. 濡傛灉宸ュ叿鏍忓垪琛ㄦ湭鍒锋柊锛岄噸鍚?Explorer 鎴栨敞閿€鍚庨噸鏂扮櫥褰曘€?4. 鍙抽敭浠诲姟鏍忥紝鎵撳紑鈥滃伐鍏锋爮鈥濓紝鍕鹃€?**MetricBar**銆?
鍗曞嚮鏂囧瓧鍙墦寮€ `HomepageUrl`銆傝嫢鏁版嵁涓瓨鍦ㄦ棫鐗?VPS 瀛楁锛屾偓鍋滄樉绀轰笂涓嬭銆丆PU 鍜屽湪绾挎暟锛涢€氱敤 JSON 鍒欐樉绀烘覆鏌撳悗鐨勬寚鏍囥€?
### 浠?V2 鍗囩骇

COM CLSID 鏈敼鍙樸€傚厛鍙栨秷鍕鹃€夊伐鍏锋爮骞堕噸鍚?Explorer锛屼娇鏃?DLL 鍗歌浇锛涢殢鍚庤鐩栧師鐩綍涓殑 `VpsTraySpeed.dll`銆傚啀杩愯涓€娆℃柊鐗?`install.bat`锛屾妸娉ㄥ唽琛ㄦ樉绀哄悕浠?`VpsTraySpeed` 鎴?`TaskbarJsonMonitor` 鏇存柊涓?`MetricBar`锛屾渶鍚庨噸鏂板惎鐢ㄥ伐鍏锋爮銆?
## 閰嶇疆

`config.ini` 涓?DLL 浣嶄簬鍚屼竴鐩綍銆備慨鏀瑰悗闇€閲嶅惎 Explorer 鎴栨敞閿€閲嶈繘銆?
榛樿閰嶇疆锛?
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

鏍蜂緥鏁堟灉锛歚鈫?.5M 鈫?.1M CPU 16.4%`銆?
### 鎸囨爣瀛楁

- `Count`锛氭寚鏍囩粍鏁伴噺锛岃寖鍥?1鈥?6锛涢潪娉曞€煎洖閫€鍒?3銆?- `Separator`锛氭寚鏍囦箣闂寸殑鏂囧瓧锛涚┖鍊奸粯璁や竴涓┖鏍笺€傝嚜瀹氫箟鍊煎闇€淇濈暀涓よ竟绌烘牸锛屽彲鍐欎负 `Separator=" | "`銆?- `Path`锛氬睍骞冲悗鐨勫瓧娈靛悕銆傚璞＄敤鐐瑰彿锛屾暟缁勪娇鐢ㄦ暟瀛楁銆?- `Template`锛氭墍鏈?`{value}` 浼氳鏇挎崲涓烘牸寮忓寲缁撴灉锛涢潪娉曟ā鏉夸細鍥為€€鍒板畨鍏ㄩ粯璁ゅ€笺€?- `Format=raw`锛氫繚鐣?JSON 瀛楃涓插強鏁板瓧鍘熸枃銆?- `Format=bytes`锛氭寜 1024 杩涘埗鎹㈢畻锛屼竴浣嶅皬鏁帮紱`1536` 鈫?`1.5K`锛宍2097152` 鈫?`2.0M`銆?
椤跺眰鏍囬噺 JSON 浣跨敤璺緞 `$`銆傚竷灏斿€煎拰 null 鍒嗗埆鏄剧ず `true`銆乣false`銆乣null`銆傚瓧娈电己澶憋紝鎴栨妸闈炴暟瀛楀瓧娈电敤浜?`bytes` 鏃讹紝鏄剧ず `--`銆?
### V2 鍥為€€

鍒犻櫎瀹屾暣鐨?`[Metrics]` 娈典互鍙?`[M1]`銆乣[M2]` 绛夌粍锛屽嵆鎸?`DisplayMode=download|upload|both` 娓叉煋鏃х増 `vps_net`銆備负鍏煎鍗囩骇锛屾棫 `[TaskbarJsonMonitor]` 鍜?`[VpsTraySpeed]` 涓婚厤缃浠嶅彲璇诲彇锛涙柊閰嶇疆搴斾娇鐢?`[MetricBar]`銆?
## 绗笁鏂?JSON 绀轰緥

### GitHub REST API 闄愰

GitHub 鍦╗瀹樻柟 REST API 鏂囨。](https://docs.github.com/zh/rest/rate-limit/rate-limit)涓鏄庝簡 `GET /rate_limit` 鍙婂祵濂楃殑 `resources.core` 瀵硅薄銆?
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

### Open-Meteo 褰撳墠澶╂皵

Open-Meteo 鍦╗瀹樻柟棰勬姤 API 鏂囨。](https://open-meteo.com/en/docs)涓鏄庝簡褰撳墠娓╁害鍜岀浉瀵规箍搴﹀瓧娈点€?
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
Template=婀垮害 {value}%
Format=raw
```

## 鍗歌浇

1. 鍦ㄤ换鍔℃爮鈥滃伐鍏锋爮鈥濊彍鍗曞彇娑堝嬀閫?**MetricBar**銆?2. 鍙抽敭 `uninstall.bat`锛岄€夋嫨鈥滀互绠＄悊鍛樿韩浠借繍琛屸€濄€?3. 閲嶅惎 Explorer 鎴栨敞閿€鍚庯紝鍐嶅垹闄ゅ畨瑁呯洰褰曘€?
## 鏋勫缓涓庢祴璇?
浜や粯鐗堟湰浣跨敤 WSL2 Ubuntu 22.04 鍜?MinGW-w64锛?
```bash
sudo apt-get install g++-mingw-w64-x86-64 binutils-mingw-w64-x86-64 make
```

PowerShell 涓繍琛岋細

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\build.ps1
```

鏋勫缓鑴氭湰浼氳繍琛岃В鏋?鎸囨爣娴嬭瘯銆丆OM 鎺ュ彛鍚堢害娴嬭瘯銆佹棤闇€娉ㄥ唽琛ㄧ殑 DeskBand 闆嗘垚瀹夸富娴嬭瘯锛屽苟楠岃瘉 DLL 蹇呴』涓?x64 PE锛涙祴璇曡繃绋嬩笉鍐欐敞鍐岃〃銆?
## 椤圭洰鍛藉悕

鍛藉悕璁板綍锛?
1. **MetricBar**锛堝凡閫夛級锛氱畝鐭€佸ソ璁帮紝閫傚悎浠诲姟鏍忔寚鏍囨潯銆?2. **JsonTaskbar**锛氳兘浣撶幇 JSON锛屼絾鐩戞帶鍚箟涓嶅鐩存帴銆?3. **TaskbarJsonMonitor**锛氭弿杩板噯纭紝浣嗗洜杩囬暱鑰屽純鐢ㄣ€?
## GitHub 鍙戝竷

鏈湴浠撳簱宸插噯澶囧ソ锛屼絾涓嶄細鑷姩鎺ㄩ€併€傚畬鏁村懡浠よ [PUBLISHING.md](PUBLISHING.md)銆?
## 璁稿彲璇?
[MIT](LICENSE)
