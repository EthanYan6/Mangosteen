/**
 * Mangosteen user manual page: TOC + markdown content + PDF print.
 */
(function () {
  'use strict';

  var articleEl = null;
  var tocNavEl = null;
  var headingObserver = null;
  var loadToken = 0;

  function t(key, params) {
    if (typeof window.t === 'function') return window.t(key, params);
    return key;
  }

  function getLang() {
    if (typeof window.getCurrentLang === 'function') return window.getCurrentLang();
    return 'zh';
  }

  function manualMdPath() {
    return getLang() === 'en' ? 'data/manual.en.md' : 'data/manual.zh.md';
  }

  function slugify(text) {
    var base = String(text || '')
      .trim()
      .toLowerCase()
      .replace(/[^\w\u4e00-\u9fff\- ]+/g, '')
      .replace(/\s+/g, '-')
      .replace(/-+/g, '-')
      .replace(/^-|-$/g, '');
    return base || 'section';
  }

  function uniqueId(used, text) {
    var id = slugify(text);
    var candidate = id;
    var n = 2;
    while (used[candidate]) {
      candidate = id + '-' + n;
      n += 1;
    }
    used[candidate] = true;
    return candidate;
  }

  function configureMarked() {
    if (typeof marked === 'undefined' || typeof marked.setOptions !== 'function') return;
    marked.setOptions({ breaks: true, gfm: true });
  }

  function markdownToSafeHtml(md) {
    var text = (md || '').trim();
    if (!text) return '';
    var hasMarked = typeof marked !== 'undefined' && typeof marked.parse === 'function';
    var hasPurify = typeof DOMPurify !== 'undefined' && typeof DOMPurify.sanitize === 'function';
    if (!hasMarked || !hasPurify) {
      return text
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/\n/g, '<br>');
    }
    var html = '';
    try {
      html = marked.parse(text);
    } catch (e) {
      return text.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/\n/g, '<br>');
    }
    return DOMPurify.sanitize(html, {
      ADD_ATTR: ['id', 'target', 'rel']
    });
  }

  function decorateHeadings(root) {
    var used = Object.create(null);
    var headings = root.querySelectorAll('h1, h2, h3');
    var items = [];
    headings.forEach(function (el) {
      var level = Number(el.tagName.charAt(1));
      var title = (el.textContent || '').trim();
      if (!title) return;
      var id = uniqueId(used, title);
      el.id = id;
      items.push({ id: id, title: title, level: level });
    });
    return items;
  }

  function renderToc(items) {
    if (!tocNavEl) return;
    if (!items.length) {
      tocNavEl.innerHTML = '<div class="help-toc__empty">' + t('helpTocEmpty') + '</div>';
      return;
    }
    var html = '<ul class="help-toc__list">';
    items.forEach(function (item) {
      html +=
        '<li class="help-toc__item help-toc__item--h' + item.level + '">' +
        '<a class="help-toc__link" href="#' + item.id + '" data-toc-id="' + item.id + '">' +
        item.title +
        '</a></li>';
    });
    html += '</ul>';
    tocNavEl.innerHTML = html;
  }

  function setActiveToc(id) {
    if (!tocNavEl) return;
    tocNavEl.querySelectorAll('.help-toc__link').forEach(function (a) {
      a.classList.toggle('is-active', a.getAttribute('data-toc-id') === id);
    });
  }

  function setupScrollSpy(items) {
    if (headingObserver) {
      headingObserver.disconnect();
      headingObserver = null;
    }
    if (!items.length || !('IntersectionObserver' in window)) return;

    var visible = Object.create(null);
    headingObserver = new IntersectionObserver(
      function (entries) {
        entries.forEach(function (entry) {
          if (entry.isIntersecting) visible[entry.target.id] = true;
          else delete visible[entry.target.id];
        });
        var active = null;
        for (var i = 0; i < items.length; i++) {
          if (visible[items[i].id]) {
            active = items[i].id;
            break;
          }
        }
        if (active) setActiveToc(active);
      },
      {
        root: null,
        rootMargin: '-20% 0px -65% 0px',
        threshold: [0, 1]
      }
    );

    items.forEach(function (item) {
      var el = document.getElementById(item.id);
      if (el) headingObserver.observe(el);
    });
  }

  function bindTocClicks() {
    if (!tocNavEl) return;
    tocNavEl.addEventListener('click', function (ev) {
      var link = ev.target.closest('a.help-toc__link');
      if (!link) return;
      var id = link.getAttribute('data-toc-id');
      var target = id ? document.getElementById(id) : null;
      if (!target) return;
      ev.preventDefault();
      target.scrollIntoView({ behavior: 'smooth', block: 'start' });
      setActiveToc(id);
      try {
        history.replaceState(null, '', '#' + id);
      } catch (e) {}
    });
  }

  function showError(message, hint) {
    if (!articleEl) return;
    articleEl.innerHTML =
      '<div class="help-error">' +
      '<p class="help-error__title">' + message + '</p>' +
      (hint ? '<p class="help-error__hint">' + hint + '</p>' : '') +
      '</div>';
    if (tocNavEl) {
      tocNavEl.innerHTML = '<div class="help-toc__empty">' + t('helpTocError') + '</div>';
    }
  }

  function loadManual() {
    var token = ++loadToken;
    if (articleEl) {
      articleEl.innerHTML = '<div class="help-content-loading">' + t('helpContentLoading') + '</div>';
    }
    if (tocNavEl) {
      tocNavEl.innerHTML = '<div class="help-toc__loading">' + t('helpTocLoading') + '</div>';
    }

    var path = manualMdPath() + '?t=' + Date.now();
    fetch(path)
      .then(function (resp) {
        if (!resp.ok) throw new Error('HTTP ' + resp.status);
        return resp.text();
      })
      .then(function (md) {
        if (token !== loadToken) return;
        if (!articleEl) return;
        articleEl.innerHTML = markdownToSafeHtml(md);
        var items = decorateHeadings(articleEl);
        renderToc(items);
        setupScrollSpy(items);

        if (location.hash) {
          var hashId = decodeURIComponent(location.hash.slice(1));
          var hashEl = document.getElementById(hashId);
          if (hashEl) {
            setTimeout(function () {
              hashEl.scrollIntoView({ behavior: 'smooth', block: 'start' });
              setActiveToc(hashId);
            }, 60);
          } else if (items[0]) {
            setActiveToc(items[0].id);
          }
        } else if (items[0]) {
          setActiveToc(items[0].id);
        }
      })
      .catch(function () {
        if (token !== loadToken) return;
        showError(t('helpContentError'), t('helpContentErrorHint'));
      });
  }

  function downloadPdf() {
    try {
      document.body.classList.add('help-print');
      window.print();
    } finally {
      setTimeout(function () {
        document.body.classList.remove('help-print');
      }, 300);
    }
  }

  function init() {
    articleEl = document.getElementById('helpArticle');
    tocNavEl = document.getElementById('helpTocNav');
    if (!articleEl || !tocNavEl) return;

    configureMarked();
    bindTocClicks();

    var pdfBtn = document.getElementById('helpPdfBtn');
    if (pdfBtn) pdfBtn.addEventListener('click', downloadPdf);

    loadManual();
    window.addEventListener('langchange', loadManual);
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }
})();
