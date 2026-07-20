# Docs 左侧版本时间线设计

**日期：** 2026-07-20  
**状态：** 已实现（待 Pages 部署验证）  
**目标：** 在山竹刷机站（`docs/`）左侧展示 GitHub Releases 版本更新历史，样式融入页面底色（非独立侧栏），Release body 正确渲染 Markdown。

## 1. 背景

现有代码已从叮咚鸡移植了时间线逻辑与样式，但目前：

- `index.html` 缺少左侧时间线 DOM
- `style.css` 用 `.timeline-sidebar { display: none !important; }` 强制隐藏
- 未引入 `marked` / `DOMPurify`，Markdown 会退化为转义纯文本 + `<br>`
- `lang.js` 文案仍写「叮咚鸡 / Dondji」

预览已确认方向：**左侧时间线与页面粉白底色一体，无白底面板、无右边框。**

## 2. 布局与视觉

| 项 | 决定 |
|---|---|
| 位置 | 桌面端左侧固定宽度约 340–400px；主操作区在右 |
| 外观 | 背景 `transparent`，与 `--bg-page` 及页面渐变一致；无 elevated 白底、无 `border-right` |
| 结构 | 竖线 + 圆点时间线；最新一条 accent 色圆点 |
| 内容 | tag（链到 Release）、日期、name、body（Markdown HTML） |
| 窄屏 | 时间线排到主内容下方，静态流式，不 sticky |

主区白卡片 / Tab 控件保持现状；仅左侧「历史轨」换肤为融入底色。

## 3. 数据与 Markdown

1. 请求 `https://api.github.com/repos/EthanYan6/Mangosteen/releases?per_page=20`
2. 每条用已有 `loadTimeline()` 渲染到 `#timeline`
3. `r.body` 经 `marked.parse`（GFM + breaks）→ `DOMPurify.sanitize` 后写入 `.timeline-body`
4. 若 CDN 未加载成功，回退为转义纯文本 + 换行转 `<br>`（现有逻辑保留）

在 `index.html` 中于业务脚本之前引入：

- `marked@14.1.4`
- `dompurify@3.2.4`

（与叮咚鸡刷机站一致，保证 Release notes 中列表、加粗、代码、链接、表格等可正常显示。）

## 4. 改动范围（实现时）

| 文件 | 改动 |
|---|---|
| `docs/index.html` | 在 `.page-layout` 内、`main` 前增加时间线 DOM；引入 marked / DOMPurify |
| `docs/css/style.css` | 删除「隐藏侧栏」覆盖规则；将 `.timeline-sidebar` 改为透明融入底色（可保留类名或改名为 rail，以实现时为准） |
| `docs/js/lang.js` | `timelineTitle` / `timelineLoading` 改为 Mangosteen 中英文案 |
| `docs/js/flash.js` | 原则上复用现有 `loadTimeline` / `timelineReleaseMarkdownToSafeHtml`，仅在需要时微调 |

**不做：** 新框架、新 API 代理、本地 changelog 文件、筛选/搜索、下载历史版本按钮（远程拉取最新固件仍走现有刷固件 Tab）。

## 5. 验收标准

- 桌面端左侧可见版本历史，视觉上不形成独立白底侧栏
- Release body 中 Markdown（至少列表、加粗、行内代码、链接）正确渲染且 XSS 经 DOMPurify 处理
- 中/英切换下标题文案为 Mangosteen，不再出现叮咚鸡
- 窄屏下时间线在主内容下方可读
- 无 Release 或 API 失败时有明确加载/失败提示

## 6. 风险

- GitHub API 未认证有速率限制；公开 Releases 列表通常足够，失败时显示错误文案即可
- CDN 不可达时 Markdown 降级为纯文本（可接受）
