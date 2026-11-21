/* 图绳 PicRope 官网通用脚本：外观切换、滚动入场、侧栏高亮 */
(function () {
  var root = document.documentElement;
  var saved = localStorage.getItem('picrope-theme');
  var prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
  if (saved === 'dark' || (!saved && prefersDark)) root.classList.add('dark');

  var btn = document.getElementById('themeBtn');
  var icon = document.getElementById('themeIcon');
  function syncIcon() {
    if (!icon) return;
    icon.innerHTML = root.classList.contains('dark')
      ? '<path d="M20.5 14.2A8.6 8.6 0 019.8 3.5 8.7 8.7 0 1020.5 14.2z"/>'
      : '<circle cx="12" cy="12" r="4.2"/><path d="M12 2.4v2.2M12 19.4v2.2M4.3 4.3l1.6 1.6M18.1 18.1l1.6 1.6M2.4 12h2.2M19.4 12h2.2M4.3 19.7l1.6-1.6M18.1 5.9l1.6-1.6"/>';
  }
  syncIcon();
  if (btn) {
    btn.addEventListener('click', function () {
      root.classList.toggle('dark');
      localStorage.setItem('picrope-theme', root.classList.contains('dark') ? 'dark' : 'light');
      syncIcon();
    });
  }

  // 滚动入场
  var items = document.querySelectorAll('.reveal');
  if (!('IntersectionObserver' in window)) {
    items.forEach(function (el) { el.classList.add('in'); });
  } else if (items.length) {
    var io = new IntersectionObserver(function (entries) {
      entries.forEach(function (e) {
        if (e.isIntersecting) { e.target.classList.add('in'); io.unobserve(e.target); }
      });
    }, { threshold: 0.12 });
    items.forEach(function (el) { io.observe(el); });
  }

  // 顶部导航：首页锚点高亮
  var navLinks = Array.prototype.slice.call(document.querySelectorAll('.nav-links a[href^="#"]'));
  if (navLinks.length && 'IntersectionObserver' in window) {
    var targets = navLinks.map(function (a) { return document.querySelector(a.getAttribute('href')); }).filter(Boolean);
    var navIO = new IntersectionObserver(function (entries) {
      entries.forEach(function (e) {
        if (!e.isIntersecting) return;
        navLinks.forEach(function (a) { a.classList.toggle('active', a.getAttribute('href') === '#' + e.target.id); });
      });
    }, { rootMargin: '-45% 0px -50% 0px' });
    targets.forEach(function (t) { navIO.observe(t); });
  }

  // 文档页：侧栏随滚动高亮
  var sideLinks = Array.prototype.slice.call(document.querySelectorAll('.doc-side a[href^="#"]'));
  if (sideLinks.length && 'IntersectionObserver' in window) {
    var heads = sideLinks.map(function (a) { return document.querySelector(a.getAttribute('href')); }).filter(Boolean);
    var sideIO = new IntersectionObserver(function (entries) {
      entries.forEach(function (e) {
        if (!e.isIntersecting) return;
        sideLinks.forEach(function (a) { a.classList.toggle('active', a.getAttribute('href') === '#' + e.target.id); });
      });
    }, { rootMargin: '-20% 0px -70% 0px' });
    heads.forEach(function (h) { sideIO.observe(h); });
  }
  // 下载按钮：自动指向最新发行版的安装包 / 压缩包，无需改版本号
  var dlLinks = document.querySelectorAll('a[data-asset]');
  if (dlLinks.length && window.fetch) {
    fetch('https://api.github.com/repos/2058763315/qt_PicRope/releases/latest')
      .then(function (r) { return r.ok ? r.json() : null; })
      .then(function (rel) {
        if (!rel || !rel.tag_name) return;
        var ver = String(rel.tag_name).replace(/^v/, '');
        dlLinks.forEach(function (a) {
          var suffix = a.getAttribute('data-asset') === 'setup' ? '-Setup.exe' : '-win64.zip';
          a.href = 'https://github.com/2058763315/qt_PicRope/releases/download/' +
            encodeURIComponent(rel.tag_name) + '/PicRope-' + encodeURIComponent(ver) + suffix;
        });
      })
      .catch(function () {});
  }
})();
