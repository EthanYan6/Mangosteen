<div align="center">

# Mangosteen

**Quansheng UV-K1 / K5V3 定制固件**

卡片主页 · 广播并入 VFO · FSK 短信 · 全新菜单

[![Latest Release](https://img.shields.io/github/v/release/EthanYan6/Mangosteen?display_name=release&sort=semver&style=for-the-badge)](https://github.com/EthanYan6/Mangosteen/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/EthanYan6/Mangosteen/total?style=for-the-badge)](https://github.com/EthanYan6/Mangosteen/releases)
[![Stars](https://img.shields.io/github/stars/EthanYan6/Mangosteen?style=for-the-badge)](https://github.com/EthanYan6/Mangosteen/stargazers)
[![License](https://img.shields.io/github/license/EthanYan6/Mangosteen?style=for-the-badge)](https://github.com/EthanYan6/Mangosteen/blob/main/LICENSE)
[![Build](https://img.shields.io/github/actions/workflow/status/EthanYan6/Mangosteen/main.yml?branch=main&label=build&style=for-the-badge)](https://github.com/EthanYan6/Mangosteen/actions/workflows/main.yml)

[下载固件](https://github.com/EthanYan6/Mangosteen/releases/latest) ·
[山竹刷机网站](https://ethanyan6.github.io/Mangosteen/) ·
[提交 Issue](https://github.com/EthanYan6/Mangosteen/issues) ·
[上游 F4HWN](https://github.com/armel/uv-k1-k5v3-firmware-custom)

</div>

---

## 简介

Mangosteen 基于 [armel/uv-k1-k5v3-firmware-custom](https://github.com/armel/uv-k1-k5v3-firmware-custom)（F4HWN Fusion），面向泉盛 **UV-K1 / K5V3** 手台，在保留 F4HWN 核心能力的同时，重新设计了主页与菜单交互，并集成与 GOGUFW 互通的 FSK 短信。

| | |
|---|---|
| **适用机型** | Quansheng UV-K1、UV-K5(8) / K5V3 等兼容机 |
| **当前版本** | 见上方 Latest Release 徽章 |
| **产物命名** | `mangosteen_<version>.bin` |
| **许可** | Apache-2.0 |

---

## 亮点一览

| 模块 | 一句话 |
|---|---|
| **卡片主页** | 双 VFO 卡片叠放，RX 跟随置前，圆形 S 表 + DTMF |
| **广播 WFM** | 广播频段直接并入主 VFO，无需独立收音机界面 |
| **设置菜单** | 列表式大字菜单 + 单项编辑卡片 |
| **消息提示** | 键盘锁定、禁止发射等统一弹出提示 |
| **FSK 短信** | 与 GOGUFW Messenger 互通（`GGM2`），含 HEARD / Range Check |
| **Yan ID** | 松手尾音发呼号；对方主页显示 `CALL SIGN:`（双方均需本固件） |

---

## 特色功能

### 卡片主页

双 VFO 以卡片叠放显示；收到信号的信道自动置前，前方卡片即主信道（发射信道）。

- 圆形 S 表：弧形仪表 + 指针；信号上升立刻跟上，减弱时缓降
- 广播（WFM）同样显示信号强度
- 前方卡片顶部显示 DTMF 输入 / 呼叫 / 实时解码

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

**快捷键**

| 按键 | 功能 |
|---|---|
| **F+MENU** | 打开 Messenger |
| **F+7** | 打开 HEARD / Range Check |

侧键 / MENU 长按也可分配 **MESSENGER**、**HEARD**。

相关菜单项：MsgRx、MsgCsg（呼号）、MsgAck、MsgBep、MsgLed、RngRsp、MsgHop 等。

配置与草稿保存在 Flash `0x012000`；收件箱 / 已发送仅在 RAM，断电清空。为给短信让出 FSK，默认关闭 AirCopy 界面、BEAM 与小游戏。

### Yan ID 呼号尾音（使用手册）

两台都刷了 Mangosteen 后，可在松手结束发射时用 FSK 发出自己的呼号；对方主页 DTMF 行优先显示 `CALL SIGN:` + 呼号（约数秒后消失）。与 Messenger 短信呼号（`MsgCsg`）分开存储，互不影响。

**双方都要设置：**

| 步骤 | 菜单项 | 怎么设 |
|:---:|--------|--------|
| 1 | **MsgRx** | 设为 **ON**（打开 FSK 后台接收；关则收不到 Yan ID / 短信） |
| 2 | **Yan ID** | 输入自己的呼号：字母 + 数字，最多 **6** 位（空则不会发射） |
| 3 | **Roger** | 选 **YAN ID**（尾音发呼号；选 OFF / ROGER / MDC 则不发 Yan ID） |

**使用：**

1. 两台同频、能正常对讲。
2. 按上表设好 MsgRx、Yan ID、Roger。
3. 按 PTT 说话，**松手**后发送端发出 FSK 呼号包。
4. 接收端主页应出现 `CALL SIGN:你的呼号`。

**建议自检：** 先确认 Messenger 短信能互通（证明 FSK 正常），再测 Yan ID。若短信也收不到，先查 MsgRx 是否打开、是否同频。

**说明：**

- 仅 Mangosteen ↔ Mangosteen；原厂或其它固件无法解析显示。
- Yan ID 包不会进短信收件箱，也不会进 HEARD。
- Yan ID 为空却选了 Roger=`YAN ID` 时，等同尾音关闭，不发任何 roger。

---

## 快速开始

### 1. 下载固件

前往 [Releases](https://github.com/EthanYan6/Mangosteen/releases/latest) 下载最新 `mangosteen_*.bin`。

### 2. 刷写

使用 k5prog、CHIRP、官方写频软件或其它支持 UV-K1 / K5 系列的刷机工具，将固件写入手台。刷机前请备份原厂固件与信道数据。

### 3. 本地编译（可选）

环境：Docker Desktop。

```bash
./compile-with-docker.sh Custom
```

产物位于编译输出目录，文件名形如 `mangosteen_<VERSION_STRING_2>.bin`（版本见 `CMakePresets.json`）。

也可用其它 preset（Bandscope、Fusion 等）；本仓库默认开启 Messenger，并关闭 AirCopy UI / BEAM / Game。

---

## 项目结构（简要）

```
Mangosteen/
├── App/                 # 应用与 UI（主页卡片、菜单、Messenger…）
├── Core/ Drivers/       # 芯片与外设驱动
├── cmake/               # CMake 工具链与选项
├── CMakePresets.json    # 构建 preset 与版本号
├── compile-with-docker.sh
└── .github/workflows/   # CI 构建
```

---

## 链接

| 资源 | 地址 |
|---|---|
| 本仓库 | https://github.com/EthanYan6/Mangosteen |
| 最新下载 | https://github.com/EthanYan6/Mangosteen/releases/latest |
| 山竹刷机网站 | https://ethanyan6.github.io/Mangosteen/ |
| 上游 F4HWN | https://github.com/armel/uv-k1-k5v3-firmware-custom |
| GOGUFW Messenger | https://github.com/Gogu-Qs/GOGUFW-UV-K1-Messenger |

---

## 许可与致谢

- 基础固件遵循上游 / DualTachyon / F4HWN / Egzumer 等 **Apache-2.0** 与署名要求。
- Messenger 协议与实现参考 [Gogu-Qs/GOGUFW-UV-K1-Messenger](https://github.com/Gogu-Qs/GOGUFW-UV-K1-Messenger)。

本固件按「原样」提供，请在合法频率与执照范围内使用，并自行承担刷机与操作风险。

<div align="center">

**若本项目对你有帮助，欢迎 Star 支持**

</div>
