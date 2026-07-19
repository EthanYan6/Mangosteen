# Mangosteen

基于 [armel/uv-k1-k5v3-firmware-custom](https://github.com/armel/uv-k1-k5v3-firmware-custom)（F4HWN Fusion / UV-K1·K5V3）的定制固件仓库。

本文说明 **本仓库相对上游不一致的改动**。未列出的部分，大体仍沿用 F4HWN 行为。

上游仓库：<https://github.com/armel/uv-k1-k5v3-firmware-custom.git>

---

## 1. 项目标识

| 项目 | 上游 | 本仓库 |
|---|---|---|
| 固件产物名 | 多为 `f4hwn.*` / preset `TARGET` | `mangosteen_${VERSION_STRING_2}`（见根目录 `CMakeLists.txt`） |
| 版本展示 | 随 F4HWN 发行 | 如 `VERSION_STRING_2=v1.0.0` 等（见 `CMakePresets.json`） |
| GitHub | armel/uv-k1-k5v3-firmware-custom | EthanYan6/Mangosteen |

---

## 2. 主页卡片 UI（相对上游主界面）

上游主页为经典双 VFO / 列表式布局；本仓库启用 **Home Card** 卡片主页（`UI_USE_HOME_CARD`，实现见 `App/ui/home_card.*`、`App/ui/main.c`），并引入精简 U8G2 绘制支持。

### 设计规则

1. **RxMode** 选择 `DUAL RX RESPOND` / `MAIN TX DUAL RX` 时，才会真正双信道接收。
2. 主页面哪个信道收到信号，哪个信道的卡片就显示在最前方；**前方卡片即主信道**，发射也在该信道进行。

---

## 3. 广播收音（WFM）并入主页

上游常见路径是独立 FM 收音机界面（`ENABLE_FMRADIO` + `app/fm.c`）。

本仓库改为：

* 默认 **`ENABLE_FMRADIO=false`**，不再打包独立收音机 UI。
* 默认 **`ENABLE_BK1080=true`**，在 VFO 上使用 **WFM** 调制（`MODULATION_WFM`）。
* 在主页切换到广播频段时自动进入 WFM 接收；离开频段则退出 WFM（见 `App/radio.c`、`App/app/main.c`）。
* 部分失效/无关菜单项已隐藏（与收音机打包调整一并做过）。

---

## 4. 通用弹出提示（Message Box）

新增通用消息框覆盖层（`UI_ShowMessageBox` / `UI_DrawMessageBox`，见 `App/ui/helper.*`、`App/ui/ui.c`）。

当前用途包括：

* 键盘锁定提示：`Key Locked`
* 禁止发射提示：`TX Disabled!`

---

## 5. FSK 短信 Messenger（相对上游最大功能差）

上游 F4HWN **没有** GOGUFW 短信栈。本仓库从 [GOGUFW-UV-K1-Messenger](https://github.com/Gogu-Qs/GOGUFW-UV-K1-Messenger) 移植并接线，目标与 GOGUFW **互通**（协议 `GGM2`）。

### 功能范围

| 包含 | 不包含（相对 GOGUFW 全量） |
|---|---|
| Inbox / Compose（T9）/ Sent / Drafts | CALLTX 呼叫音（F+9） |
| Reply / Delete / Resend | Survival Mode |
| ACK / 重试 | FM Naming（`0x013000`） |
| HEARD、Range Check | CHIRP 专用模块 |
| 主页后台收信、未读信封图标 | — |

### 快捷键（与上游不同）

| 按键 | 上游典型行为 | 本仓库 |
|---|---|---|
| **F+MENU** | 进入菜单相关逻辑 | 打开 **Messenger** |
| **F+7** | 常为 Breakout 小游戏 | 打开 **HEARD / Range Check** |

侧键 / MENU 长按也可分配 **MESSENGER**、**HEARD**。

### 菜单项（`Msg*`）

MsgRx、MsgCsg（呼号）、MsgAck、MsgBep、MsgLed、RngRsp、MsgHop、MsgDbg（隐藏）等。

### 存储

* 配置 / 呼号 / 草稿：私有 Flash 扇区 **`0x012000`**（与 GOGUFW 一致）
* EEPROM 兼容别名：`0x00E000` → `0x012000`
* Inbox / Sent：**仅 RAM**，断电清空

### 编译与资源取舍（相对上游 preset）

为把 FSK 留给短信，各 preset 默认：

| 开关 | 本仓库 | 说明 |
|---|---|---|
| `ENABLE_MESSENGER` | **true** | 新增 |
| `ENABLE_AIRCOPY` | **false** | 关闭 AirCopy **界面**；仍链接 `app/aircopy.c` 提供 FSK 缓冲 |
| `ENABLE_FEAT_F4HWN_BEAM` | **false** | 关闭 BEAM |
| `ENABLE_FEAT_F4HWN_GAME` | **false** | 关闭小游戏，腾出 F+7 |

FSK 缓冲长度为 **`g_FSK_Buffer[50]`**（对齐 GOGUFW；AirCopy/Beam 若再启用仍只用前 36 字）。

### 源码位置

`App/app/messenger*.c/h`，以及 `app.c` / `main.c` / `action.c` / `radio.c` / `menu` / `status` 等接线。

---

## 6. 与上游关系的一句话对照

```
armel/uv-k1-k5v3-firmware-custom  (F4HWN Fusion)
        │
        ▼
   Mangosteen
        ├── 卡片主页 + 主信道跟随 RX
        ├── WFM 并入主 VFO（取消独立收音机 UI 默认打包）
        ├── 弹出提示框
        ├── GOGUFW 兼容 Messenger / HEARD / Range Check
        └── 关闭 AirCopy UI / BEAM / Game（FSK 专供短信）
```

---

## 7. 编译（与上游相同方式）

Docker Desktop 运行后：

```bash
./compile-with-docker.sh Custom
```

也可用其它 preset（Bandscope、Fusion 等）；本仓库下它们同样默认开启 Messenger、关闭 AirCopy/BEAM/Game。

---

## 8. 许可与致谢

* 基础固件与许可仍遵循上游 / DualTachyon / F4HWN / Egzumer 等 Apache-2.0 与署名要求。
* Messenger 协议与实现参考 [Gogu-Qs/GOGUFW-UV-K1-Messenger](https://github.com/Gogu-Qs/GOGUFW-UV-K1-Messenger)。

本固件按「原样」提供，请在合法频率与执照范围内使用，并自行承担刷机与操作风险。
