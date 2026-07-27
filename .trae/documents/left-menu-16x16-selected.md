# 左侧菜单：选中项16x16字体 + 滚动裁剪

## 背景
用户要求短信(messenger)主页和信息页面的左侧菜单：选中项用16x16字体，未选中项用小字体，超出虚线区域的内容被裁剪，实现滚动效果。

信息页面的设置菜单（`UI_MENU_DrawListStyle`）已有此行为（CN语言下选中项16x16），无需改动。仅需修改短信主页 `draw_home`。

## 修改文件
- `App/app/messenger_ui.c` — `draw_home` 函数

## 实现步骤

### 1. 新增 `msg_draw_str16_at_y` 函数
在 messenger_ui.c 中新增混合中英文16x16渲染函数，复用已有的 `msg_draw_cn16_char_at_y` 和 `msg_set_pixel`：
- 遍历字符串，GB2312字符用 `msg_draw_cn16_char_at_y`，ASCII用 `gFontBig`（8px宽16px高）
- 支持 `max_rows` 参数做Y轴裁剪
- 支持 `inverted` 参数（选中反色）

### 2. 改写 `draw_home` 函数
模仿 `UI_MENU_DrawListStyle` 的滚动逻辑：
- 计算可用区域：从 title 下方到虚线位置（CN: y=12..47, EN: y=10..46）
- 选中项高度 = 2+16+2 = 20px（pad + text + pad），未选中项 = 8px + 1px gap = 9px
- 计算 `first` 滚动偏移：确保选中项可见，超出顶部的项被裁剪
- 选中项用 `msg_draw_str16_at_y` 渲染 + 反色带
- 未选中项用现有小字体渲染
- Y轴裁剪：绘制时传入 `max_rows` 限制，确保不超过虚线

### 3. 调整图标位置
`draw_home_icon` 的Y坐标需根据新的选中行位置动态计算，而非固定坐标。

## 验证
- 编译通过，FLASH不超限
- 短信主页：选中项16x16，其余小字体
- 滚动到第4项时，顶部项被裁剪/不显示
- 中文和英文语言都正常显示
