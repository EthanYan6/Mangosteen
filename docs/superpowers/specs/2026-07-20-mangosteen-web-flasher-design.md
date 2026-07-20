# Mangosteen Web 刷机站设计

**日期：** 2026-07-20  
**状态：** 待用户审阅  
**目标：** 为 Mangosteen 固件提供 GitHub Pages 可爱风 Web 刷机站（Web Serial）

## 1. 背景与目标

模仿 Dondji 的 GitHub Pages 刷机能力，但采用**方案 2：全新精简站点**——UI/结构重写，仅移植并对照验证 Web Serial 协议层；写频逻辑以 **Mangosteen 固件源码**为准，不照搬叮咚鸡写频行为。

### 1.1 已确认需求

| 项 | 决定 |
|---|---|
| 实现路径 | 方案 2：新站点 + 移植协议 |
| 品牌 | 左上大号可爱山竹 SVG +「山竹刷机网站」 |
| 山竹图源 | SVG/CSS 手绘（不做位图素材依赖） |
| 开发者 | 标明由 **BD1AHN** 开发 |
| 访客统计 | busuanzi，**仅统计本站** |
| 打赏 | 复用叮咚鸡收款码 + **同一套打赏榜单机制/数据** |
| 语言 | 中英双语 |
| 视觉 | 可爱、偏亮；山竹紫 + 浅粉/白 |

### 1.2 功能范围（Tab 顺序固定）

1. **备份校准**
2. **刷固件**（须支持从 GitHub Releases **直接下载**最新固件）
3. **校验校准**
4. **恢复校准**
5. **写频**

### 1.3 明确不做

刷字库、备份/恢复配置、版本时间线、应急工具箱、社交侧栏、叮咚鸡加载小鸡动画等与本站无关的 UI。

## 2. 架构

### 2.1 部署

- 仓库目录：`docs/` 作为 GitHub Pages 根
- Pages 设置：Deploy from branch → `/docs`
- 预期 URL：`https://ethanyan6.github.io/Mangosteen/`
- README 增加刷机站入口链接

### 2.2 技术栈

纯静态：HTML + CSS + JS（Web Serial）。可选 minify；不引入 SPA 框架。

### 2.3 建议目录结构

```
docs/
  index.html
  css/style.css
  js/
    serial.js       # Web Serial + 协议帧
    flash.js        # 刷固件（含 Releases 下载）
    calib.js        # 备份 / 校验 / 恢复校准
    writefreq.js    # 写频（对齐 Mangosteen）
    lang.js         # 中英
    app.js          # 品牌、Tab、打赏、Toast、统计
  images/           # 微信/支付宝收款码（自 Dondji 复用）
  data/donations.csv
  firmware/         # 可选镜像兜底
  superpowers/specs/  # 本设计文档（可不对 Pages 暴露强调）
```

协议语义参考 Dondji 已验证实现，但文件与 UI **全新编写**；写频字段以 Mangosteen `settings.c` / `misc.c` / `radio.c` 为准。

## 3. 页面与交互

### 3.1 布局

| 区域 | 内容 |
|---|---|
| 顶栏跑马灯 | 累计 UV + 今日 UV（busuanzi） |
| 左上品牌 | 大号山竹 SVG（轻量动效）+「山竹刷机网站」+ 短副标题（如 Mangosteen · UV-K1 / K5V3） |
| 开发者行 | 「由 BD1AHN 开发」+「请他喝杯咖啡」 |
| 主区 | 五个 Tab + 当前操作区 + 日志/进度 |
| 右上 | 语言切换（中/EN）；主题切换可选，首版可仅语言 |
| 弹层 | 打赏（收款码 + 榜单）、机型/BOOT 警告、Toast |

### 3.2 视觉原则

- 首屏品牌为主，功能区在下，避免仪表盘式堆砌
- 可爱轻盈，避免深色「黑客终端」默认风与通用紫渐变 AI 模板感
- 至少 2–3 处有意动效（山竹微动、Tab/按钮反馈、加载进度）

## 4. 功能细节

### 4.1 备份校准

- 正常开机进入使用界面后连 USB（非 BOOT）
- 按设备信息解析校准基址（UVTools2 兼容：`0x1E00` / `0xB000`）
- 导出 `.dat` 供下载

### 4.2 刷固件

- **直接下载**：从 GitHub Releases（`EthanYan6/Mangosteen` latest）拉取 `mangosteen_*.bin`，成功后自动填入刷写区
- **本地选择**：保留选文件，便于刷自定义/历史版本
- 可选：`docs/firmware/` 作镜像兜底；优先 Releases
- 刷写前：机型确认 + BOOT 模式说明（按住 PTT 开机）
- 不做字库 / 开机图刷写

### 4.3 校验校准

- 对比设备当前校准、官方区、第三方区、本地备份
- 表格展示；支持读设备、加载备份、导出

### 4.4 恢复校准

- 选择 `.dat` 写回；二次确认
- 提示兼容本站（及同格式）备份；开机状态操作

### 4.5 写频（Mangosteen 对齐要点）

| 项 | Mangosteen 行为 |
|---|---|
| MR 数据 | `channel * 16` |
| 信道名 | `0x004000 + ch*16`，**最多 10 字节 ASCII**（固件 `SETTINGS_FetchChannelName` 读 10 字节并校验 32–127） |
| 中文旧区 `0x020000` | **不使用**（与叮咚鸡 UTF-8 中文名逻辑不同） |
| 信道属性 | `0x8000 + ch*2`（`FLASH_CHANNEL_ATTR_BASE`） |
| 调制 | 含 **FM / AM / USB / WFM**（按固件枚举；不可把 `modulation > 2` 一律当无效） |
| 信道数 | 前 1024 槽（`MR_CHANNELS_MAX`） |
| 导出前缀 | `Mangosteen_channels_export` |

UI：分页表格、读/写设备、导入导出；名称输入限制 ASCII ≤10。

### 4.6 打赏与统计

- 收款码：复用叮咚鸡微信/支付宝图（拷贝到本仓库 `docs/images/`）
- 榜单：同一套机制；`docs/data/donations.csv` **与叮咚鸡内容保持同步拷贝**（更新榜单时两边一起改，不跨站远程拉 CSV）
- busuanzi：仅本站页面 ID/URL，与叮咚鸡隔离

## 5. 错误处理

- 无 Web Serial / 非 Chrome·Edge：明确换浏览器提示
- 刷固件前机型 + BOOT 确认；刷写中禁止危险切页，失败可重试
- 校准恢复 / 写频写入前二次确认
- 写频名称非 ASCII 或超长拦截
- Releases 下载失败：提示网络问题并引导本地选文件
- 操作结果：Toast + 日志区

## 6. 验收标准

- [ ] 五 Tab 顺序：备份校准 → 刷固件 → 校验校准 → 恢复校准 → 写频
- [ ] 可从 Releases 直接下载最新 `mangosteen_*.bin` 并刷入
- [ ] 校准备份 / 校验 / 恢复可用
- [ ] 写频符合 Mangosteen（ASCII 名 10 字节、含 WFM、无中文旧区逻辑）
- [ ] 中英切换可用
- [ ] 打赏弹层（收款码 + 榜单）可用
- [ ] 本站访客统计与叮咚鸡分离
- [ ] GitHub Pages 可打开，README 有入口

## 7. 实现备注

- 实现前对照 Mangosteen 源码复核 MR 16 字节字段布局、功率档位、步进表，避免仅凭叮咚鸡 JS 假设
- 本设计经分段确认：§1 结构（含刷固件与 Tab 序）、§2 视觉、§3 功能（含下载固件）、§4 错误与部署
