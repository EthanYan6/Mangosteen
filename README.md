# Mangosteen

[![Latest Release](https://img.shields.io/github/v/release/EthanYan6/Mangosteen?display_name=release&sort=semver)](https://github.com/EthanYan6/Mangosteen/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/EthanYan6/Mangosteen/total)](https://github.com/EthanYan6/Mangosteen/releases)
[![Stars](https://img.shields.io/github/stars/EthanYan6/Mangosteen)](https://github.com/EthanYan6/Mangosteen/stargazers)
[![License](https://img.shields.io/github/license/EthanYan6/Mangosteen)](https://github.com/EthanYan6/Mangosteen/blob/main/LICENSE)
[![Build](https://img.shields.io/github/actions/workflow/status/EthanYan6/Mangosteen/main.yml?branch=main&label=build)](https://github.com/EthanYan6/Mangosteen/actions/workflows/main.yml)

基于 [armel/uv-k1-k5v3-firmware-custom](https://github.com/armel/uv-k1-k5v3-firmware-custom)（F4HWN Fusion / UV-K1·K5V3）的定制固件。

上游：<https://github.com/armel/uv-k1-k5v3-firmware-custom.git>

---

## 特色功能

### 卡片主页

双 VFO 以卡片叠放显示；收到信号的信道自动置前，前方卡片即主信道（发射信道）。

* 圆形 S 表：弧形仪表 + 指针；信号上升立刻跟上，减弱时缓降
* 广播（WFM）同样显示信号强度
* 前方卡片顶部显示 DTMF 输入 / 呼叫 / 实时解码

### 广播收音（WFM）

广播频段并入主 VFO，无需独立收音机界面。切到广播频率自动进入 WFM，离开则退出。

### 设置菜单

列表式设置页：大字标题、五项一屏、当前值右对齐、选中反色；单项编辑使用独立编辑卡片。

### 弹出提示

键盘锁定、禁止发射等场景使用统一消息框覆盖层。

### FSK 短信 Messenger

与 [GOGUFW Messenger](https://github.com/Gogu-Qs/GOGUFW-UV-K1-Messenger) 互通（协议 `GGM2`）。

| 功能 | 说明 |
|---|---|
| 收发 | Inbox / Compose（T9）/ Sent / Drafts，支持 Reply、Delete、Resend |
| ACK | 送达确认与重试 |
| HEARD / Range Check | 邻居发现与测距 |
| 状态栏 | 主页后台收信，未读显示信封图标 |

快捷键：

| 按键 | 功能 |
|---|---|
| **F+MENU** | 打开 Messenger |
| **F+7** | 打开 HEARD / Range Check |

侧键 / MENU 长按也可分配 **MESSENGER**、**HEARD**。

相关菜单项：MsgRx、MsgCsg（呼号）、MsgAck、MsgBep、MsgLed、RngRsp、MsgHop 等。

配置与草稿保存在 Flash `0x012000`；收件箱 / 已发送仅在 RAM，断电清空。

为给短信让出 FSK，默认关闭 AirCopy 界面、BEAM 与小游戏。

---

## 编译

```bash
./compile-with-docker.sh Custom
```

产物名：`mangosteen_${VERSION_STRING_2}`（当前 Custom：`v2.0.0`，见 `CMakePresets.json`）。

---

## 许可与致谢

* 基础固件遵循上游 / DualTachyon / F4HWN / Egzumer 等 Apache-2.0 与署名要求。
* Messenger 参考 [Gogu-Qs/GOGUFW-UV-K1-Messenger](https://github.com/Gogu-Qs/GOGUFW-UV-K1-Messenger)。

按「原样」提供，请在合法频率与执照范围内使用，并自行承担刷机与操作风险。
