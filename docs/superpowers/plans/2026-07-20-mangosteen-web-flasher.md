# Mangosteen Web 刷机站 Implementation Plan

> **For agentic workers:** Implement task-by-task. Steps use checkbox syntax.

**Goal:** Ship a cute GitHub Pages flasher at `docs/` with calib dump/check/restore, firmware flash (Releases download), and Mangosteen-aligned writefreq.

**Architecture:** New HTML/CSS brand shell; adapt Dondji Web Serial protocol JS with Mangosteen constants; bilingual lang.js; shared donation assets.

**Tech Stack:** Static HTML/CSS/JS, Web Serial, busuanzi, SheetJS/Sortable for writefreq, GitHub Releases API.

---

### Task 1: Scaffold pages + cute theme
- Create `docs/index.html`, `docs/css/style.css`, brand SVG, copy donation images/csv
- Tabs order: dump → flash → calib-check → restore → writefreq

### Task 2: Adapt protocol JS
- Copy/adapt `flash.js`: repo Mangosteen, Releases firmware fetch, null-safe unused UI
- Writefreq: ASCII name ≤10, WFM, no 0x020000 CN zone, export prefix Mangosteen

### Task 3: i18n + README
- `lang.js` CN/EN; README Pages link
