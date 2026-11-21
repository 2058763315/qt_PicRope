/* 中英文切换：按原文查表替换文本节点，无需给每个元素加标记 */
(function () {
  var DICT = {
    /* ---------- 通用 ---------- */
    "图绳": "PicRope",
    "功能": "Features",
    "篮子": "Basket",
    "文档": "Docs",
    "下载": "Download",
    "常见问题": "FAQ",
    "更新记录": "Release notes",
    "回到顶部": "Back to top",
    "64 位": "64-bit",
    "图片篮子": "Image basket",
    "文字笔记": "Text notes",
    "图片篮子…": "Image basket…",
    "文字笔记…": "Text notes…",
    "设置…": "Settings…",
    "显示/收起": "Show/Hide",
    "自动弹出": "Auto pop-up",
    "退出图绳": "Exit PicRope",
    "剪贴板收集 · 图片": "Clipboard capture · Images",
    "剪贴板收集 · 文字": "Clipboard capture · Text",
    "重命名…": "Rename…",
    "挂上绳子": "Hang on rope",
    "取下绳子": "Take off rope",
    "打开文件夹": "Open folder",
    "检查更新": "Check for updates",
    "自动检查更新": "Check automatically",
    "data": "data",
    "。": ".",
    "。勾选": ". Tick",
    "、": ", ",

    /* ---------- 首页 · 英雄区 ---------- */
    "截图 · 上绳": "shot · hang",
    "自定义挂绳热键 · 数据备份": "Customizable hotkey · Data backup",
    "截图，本该随手一放。": "Screenshots, right where you left them.",
    "截图自动收集到屏幕顶部的一条绳子上，鼠标移到顶部即展开，离开自行收起。 单击即可复制，双击用内置查看器打开，暂时用不上的丢进图片篮子。":
      "New screenshots collect on a rope at the top of your screen. Move the cursor up and it unfolds; move away and it hides. Click to copy, double-click to open in the viewer, or park the ones you don't need in the basket.",
    "GitHub 下载": "Download on GitHub",
    "查看文档": "Read the docs",
    "设计稿": "Design draft",
    "已复制": "Copied",
    "会议白板": "Whiteboard",
    "待整理": "To sort",
    "图标素材": "Icon set",

    /* ---------- 首页 · 功能 ---------- */
    "把「截图之后」的那一步省掉": "Skip everything after the screenshot",
    "不用先找文件夹、不用开图床。截完就在绳子上了，什么时候用，什么时候取。":
      "No hunting for folders, no image host. The shot lands on the rope, ready whenever you need it.",
    "自动上绳": "Auto-hang",
    "剪贴板里的新图片与「屏幕截图」文件夹的内容自动收集，按图片内容去重，同一张不会重复挂两次。":
      "New clipboard images and anything saved by the screenshot tool are collected automatically, de-duplicated by content so the same shot never hangs twice.",
    "顶部热区": "Top-edge hotspot",
    "绳子常驻在屏幕顶部边缘。鼠标移上去自动展开，离开后收起，不占桌面也不挡视线。":
      "The rope lives at the top edge. Hover and it slides out, move away and it hides. No clutter, no blocked view.",
    "内置查看器": "Built-in viewer",
    "双击卡片直接用内置查看器打开，自动读取同目录图片，方向键翻页，滚轮缩放，按 0 复位。":
      "Double-click a card to open it in the built-in viewer. It reads the rest of the folder, flips with the arrow keys, zooms with the wheel and resets with 0.",
    "暂时不用的图片收进篮子，按文件名搜索、拖框多选、右键重命名，要用时一键挂回绳子。":
      "Park images you don't need yet: search by name, drag to select, rename from the context menu, then hang them back with one click.",
    "复制的文字也会被收集，和图片一起管理：搜索、多选、一键复制，临时素材不再散落各处。":
      "Copied text is captured too and managed alongside images: search, multi-select, copy in one go, so scratch material stops scattering.",
    "五套外观主题": "Five themes",
    "银灰、棉麻木夹、奶白、柔彩、石墨金属，界面与卡片风格一起切换，深浅色都不刺眼。":
      "Silver grey, linen with wooden pegs, porcelain, soft colour and graphite metal. Interface and cards switch together, in light or dark.",

    /* ---------- 首页 · 展示 ---------- */
    "图片篮子，临时素材的中转站": "The basket: a layover for scratch material",
    "绳子只放「现在要用」的，其他的先丢进篮子。需要时再挂回去，不打断手头的工作。":
      "Keep only what you need right now on the rope; everything else waits in the basket until it's needed.",
    "按文件名实时搜索，支持清空": "Search by file name as you type, clear in one click",
    "鼠标拖动框选，Ctrl / Shift 多选": "Drag to box-select, Ctrl / Shift to multi-select",
    "右键重命名，同步更新卡片与记录": "Rename from the context menu; cards and records follow",
    "底部状态栏实时显示张数与选中数": "The status bar shows the total and the selection count",
    "搜索图片文件名…": "Search image file name…",
    "复制过的文字，也一并收好": "Copied text, kept in the same place",
    "写文档时复制过的段落、聊天里摘下来的句子，会自动进入文字笔记，随时回看、随时复制。":
      "Paragraphs copied while writing and lines lifted from a chat land in Text notes automatically, ready to review or copy again.",
    "左侧列表浏览，右侧查看全文": "List on the left, full text on the right",
    "多选后一次性复制或删除": "Multi-select, then copy or delete in one go",
    "与图片同一套搜索与选择逻辑": "Same search and selection logic as images",
    "搜索文字…": "Search text…",
    "刚刚 14:22": "Just now 14:22",
    "截图随手一放，要用时再取 —— 这是本次改版的核心。":
      "Take the shot, grab it when you need it. That is the whole idea.",
    "今天 11:08": "Today 11:08",
    "双击卡片用内置查看器打开，方向键翻页，滚轮缩放，按 0 复位。":
      "Double-click a card for the built-in viewer: arrow keys flip, the wheel zooms, 0 resets.",
    "昨天 18:40": "Yesterday 18:40",
    "篮子里的图片都已挂上绳子。": "Everything in the basket is back on the rope.",

    /* ---------- 首页 · 快捷键条 ---------- */
    "呼出 / 收起挂绳": "Show / hide the rope",
    "查看器切换图片": "Switch image in viewer",
    "缩放复位 100%": "Reset zoom to 100%",
    "关闭查看器": "Close the viewer",

    /* ---------- 首页 · 下载 ---------- */
    "下载图绳": "Download PicRope",
    "支持 Windows 10 / 11（64 位）。安装版与绿色版功能一致，任选其一。":
      "Windows 10 / 11 (64-bit). The installer and the portable build are feature-identical; pick either.",
    "推荐": "Recommended",
    "安装版": "Installer",
    "双击安装，自动创建开始菜单与桌面快捷方式，可随向导选择是否开机自启。":
      "Run the wizard; Start menu and desktop shortcuts are created, and you can opt into launch at login.",
    "适合长期使用，升级直接覆盖安装": "Best for long-term use; upgrade by installing over it",
    "卸载时保留图片与文字笔记数据": "Uninstalling keeps your images and notes",
    "开机自启可在设置中随时关闭": "Launch at login can be turned off in Settings",
    "下载安装包": "Download installer",
    "绿色版": "Portable",
    "解压即用，不写系统目录，也不需要管理员权限，适合放在 U 盘里随身携带。":
      "Unzip and run. Nothing is written outside its own folder and no admin rights are needed, so it travels happily on a USB stick.",
    "免安装，删除目录即卸载干净": "No install; deleting the folder removes it completely",
    "数据保存在程序旁的 data 文件夹": "Data lives in the data folder next to the program",
    "开机自启可在「设置 → 功能」中开启": "Launch at login is under Settings → Features",
    "下载压缩包": "Download ZIP",
    "本次更新": "What's new",
    "全部更新记录 →": "All release notes →",
    "新增英文界面，设置中可切换简体中文 / English，重启生效":
      "New English interface; switch between Chinese and English in Settings, then restart",
    "设置新增剪贴板收集（图片 / 文字）与新图片自动上绳，和托盘菜单双向同步":
      "Settings now include clipboard capture (images / text) and auto-hang for new images, kept in sync with the tray menu",
    "新增卡片尺寸（小 / 中 / 大）、挂绳不透明度（20% ~ 100%）、收起延迟":
      "New card size (Small / Medium / Large), rope opacity (20%–100%) and auto-hide delay",
    "新增「快捷键」栏目，呼出 / 收起挂绳的热键可自定义，默认 Ctrl+Alt+T":
      "New Shortcuts section; the show / hide rope hotkey is customizable, default Ctrl+Alt+T",
    "新增数据备份（把 data 目录打包成 zip）、打开数据目录与恢复默认设置":
      "New data backup (packs the data folder into a zip), open data folder and reset to defaults",
    "设置界面支持上下滚动，窗口高度不再为内容让路":
      "The Settings window now scrolls instead of growing taller with its content",
    "呼出 / 收起挂绳（默认，可在设置 →「快捷键」中修改）":
      "Show / hide rope (default; change it under Settings → Shortcuts)",
    "设置新增剪贴板收集、新图片自动上绳、卡片尺寸、挂绳不透明度、收起延迟、自定义挂绳热键，以及数据备份 / 打开目录 / 恢复默认设置；设置界面支持上下滚动":
      "New Settings: clipboard capture, auto-hang, card size, rope opacity, auto-hide delay, customizable hotkey, data backup / open folder / reset. The Settings window now scrolls.",
    "设置新增「功能」栏目：顶部自动弹出、界面语言、开机自启":
      "New Features section in Settings: top-edge auto pop-up, interface language, launch at login",
    "托盘菜单与设置界面的自动弹出状态实时双向同步":
      "The auto pop-up switch stays in sync between the tray menu and Settings",
    "修复切换语言取消后，下拉框状态与展开高亮不一致的问题":
      "Fixed the language dropdown keeping the cancelled option highlighted after choosing No",

    /* ---------- 首页 · FAQ ---------- */
    "还有一些没提到的细节，可以直接到 GitHub 提 issue。":
      "Anything we missed? Open an issue on GitHub.",
    "它和系统截图工具有什么不同？": "How is it different from the system screenshot tool?",
    "系统截图是「截完就结束」，图绳负责的是之后那一段：自动收集、按内容去重、排在顶部随时取用，还能把暂时不用的图片收进篮子。":
      "The system tool stops when the shot is taken. PicRope handles what comes next: collecting, de-duplicating by content, keeping everything at the top edge, and parking the rest in the basket.",
    "绳子会挡住屏幕内容吗？": "Does the rope cover what's on screen?",
    "不会。绳子贴在屏幕顶部边缘，平时隐藏，鼠标移到顶部才展开，离开就收起。也可以在托盘菜单里临时关掉自动弹出。":
      "No. It hugs the top edge, stays hidden by default, unfolds when the cursor reaches the top and hides when you leave. Auto pop-up can also be switched off from the tray menu.",
    "图片保存在哪里？": "Where are images stored?",
    "保存在程序目录下的 data 文件夹中，升级与卸载都会保留。取下图片时可选择保留原文件。":
      "In the data folder next to the program. Upgrades and uninstalls keep it, and taking an image off the rope can leave the file in place.",
    "绿色版怎么设置开机自启？": "How do I enable launch at login in the portable build?",
    "打开「设置 → 功能」，勾选「开机时自动启动图绳」即可。它与安装包写入的是同一个注册表项，因此安装版在向导里勾选的自启，也能在这里关闭。":
      "Open Settings → Features and tick “Start PicRope at login”. It writes the same registry entry as the installer, so a launch-at-login choice made during setup can be turned off here too.",
    "为什么切换语言后需要重启？": "Why does changing the language need a restart?",
    "当前版本采用重启生效的方式加载翻译文件，切换时会询问是否立即重启，选「否」则保持原语言不变。":
      "This version loads its translation files at startup. You'll be asked to restart right away; choosing No keeps the current language.",
    "Copyright © 图绳 PicRope · 基于 Qt 6 重构": "Copyright © PicRope · Rebuilt with Qt 6",

    /* ---------- 文档页 ---------- */
    "首页": "Home",
    "/ 使用文档": "/ Documentation",
    "使用文档": "Documentation",
    "托盘菜单": "Open it from the tray menu's",
    "从安装到日常使用，按顺序读一遍大约十分钟。适用于 1.1.4 版本，Windows 10 / 11 64 位。":
      "From install to daily use, about ten minutes read in order. Written for 1.1.4 on Windows 10 / 11 64-bit.",
    "目录": "Contents",
    "快速开始": "Quick start",
    "截图收集": "Capturing screenshots",
    "顶部绳子": "The top rope",
    "设置项": "Settings",
    "快捷键": "Shortcuts",
    "没找到答案？可以到 GitHub 提 issue。": "Not covered here? Open an issue on GitHub.",
    "图绳由托盘图标常驻运行，界面主要是一条贴在屏幕顶部的绳子。启动后：":
      "PicRope runs from the tray; its main surface is a rope pinned to the top edge of the screen. After launch:",
    "托盘区出现图绳图标，右键可打开全部菜单": "An icon appears in the tray; right-click it for the full menu",
    "把鼠标移到屏幕上边缘，绳子自动展开": "Move the cursor to the top edge and the rope unfolds",
    "之后每次截图，图片会自动挂到绳子上": "Every screenshot after that hangs on the rope automatically",
    "安装版与绿色版": "Installer vs. portable",
    "两种包功能完全一致，任选其一：": "The two packages are identical in features; pick either:",
    "版本": "Package",
    "说明": "Notes",
    "双击": "Run",
    "按向导安装，可选开机自启；升级与卸载都会保留": "through the wizard; launch at login is optional. Upgrades and uninstalls keep the",
    "解压": "Unzip",
    "后直接运行": "and run",
    "，不写系统目录": "; nothing is written outside the folder",
    "图绳使用单实例锁。已经有一个实例在运行时，再次启动会提示「图绳已在运行」，不会有第二个图标出现。":
      "PicRope is single-instance. If it is already running, launching it again says “PicRope is already running” and no second icon appears.",
    "图绳不替换系统截图工具，它负责截图之后的部分。目前会从两个来源收集图片：":
      "PicRope doesn't replace the system screenshot tool; it takes over afterwards. Images come from two sources:",
    "剪贴板": "Clipboard",
    "：复制图片、或用": ": copy an image, or press",
    "截图后，图片自动保存并挂上绳子": "and the shot is saved and hung on the rope",
    "「屏幕截图」文件夹": "The Screenshots folder",
    "：系统截图工具存入的图片同样会被收集": ": images saved by the system tool are collected too",
    "同一张图片经多个来源出现时，图绳按": "When the same image arrives from several sources, PicRope de-duplicates by",
    "图片内容": "image content",
    "去重，只保留一张，不会重复挂两次。": "and keeps a single copy, so nothing hangs twice.",
    "收集行为可以在托盘菜单里分别关闭：": "Collection can be switched off separately from the tray menu:",
    "绳子贴在屏幕顶部边缘，平时隐藏。鼠标移到上边缘时展开，离开后自动收起，也可以通过托盘菜单的":
      "The rope hugs the top edge and stays hidden. Hover the top edge to unfold it, move away and it hides; you can also toggle it from the tray menu's",
    "或快捷键": "or with the shortcut",
    "手动切换。": "to toggle it.",
    "卡片操作": "Card actions",
    "单击卡片复制图片；双击用内置查看器打开；右键菜单包含：":
      "Click a card to copy the image, double-click to open it in the viewer. The context menu offers:",
    "菜单项": "Menu item",
    "作用": "Action",
    "复制图片": "Copy image",
    "把图片放入剪贴板": "Put the image on the clipboard",
    "打开图片": "Open image",
    "调用系统默认看图软件打开": "Open with the default image app",
    "保存副本…": "Save a copy…",
    "另存到指定位置": "Save to a location you choose",
    "在文件夹中显示": "Show in folder",
    "在资源管理器中定位文件": "Reveal the file in Explorer",
    "编辑…": "Edit…",
    "用系统图片编辑器打开": "Open with the system image editor",
    "取下 / 取下左侧 / 取下右侧 / 取下其他 / 全部取下":
      "Take off / Take off left / right / others / all",
    "从绳子移除卡片，文件仍保留": "Remove cards from the rope; files stay on disk",
    "删除（移入回收站）": "Delete (to Recycle Bin)",
    "删除磁盘文件并移入回收站": "Delete the file and send it to the Recycle Bin",
    "「取下」只影响绳子上的卡片，源文件不会被删除；需要真正删除文件时用最后一项。":
      "“Take off” only removes cards from the rope; the file stays. Use the last item to delete for real.",
    "绳子只放当前要用的图片，其余的可以先收进篮子。从托盘菜单打开":
      "Keep only what you need on the rope; park the rest in the basket. Open it from the tray menu's",
    "标题栏搜索框按文件名实时筛选，右侧叉号一键清空":
      "The title-bar search filters by file name as you type; the × clears it",
    "鼠标拖动可框选，": "Drag to box-select,",
    "点击可多选": "+ click to multi-select",
    "右键": "Right-click",
    "直接改磁盘文件名，收藏记录与绳子卡片同步更新":
      "to rename the file on disk; records and cards update with it",
    "把选中的图片送回绳子，": "returns the selected images to the rope;",
    "则相反": "does the reverse",
    "底部状态栏显示总数与选中数，左侧": "The status bar shows the total and the selection count;",
    "直接定位图片目录": "jumps to the image folder",
    "复制过的文字也会被收集，从托盘菜单的": "Copied text is captured as well; open it from the tray menu's",
    "打开。左侧是列表，选中后在右侧查看全文；支持搜索、多选后一次性复制或删除。":
      "entry. The list is on the left, the full text on the right. Search, multi-select, then copy or delete in one go.",
    "双击绳子上的卡片即可打开，查看器会自动读取同目录下的图片，方便前后翻看。":
      "Double-click a card to open it. The viewer reads the rest of the folder, so you can flip back and forth.",
    "切换上一张 / 下一张": "Previous / next image",
    "滚轮以鼠标位置为中心缩放，范围 100% ~ 800%": "The wheel zooms around the cursor, from 100% to 800%",
    "放大后按住图片拖动平移，按": "When zoomed in, drag the image to pan; press",
    "复位到 100%": "to reset to 100%",
    "未放大时按住图片或底部信息栏，可以拖动整个窗口":
      "At 100%, drag the image or the bottom bar to move the window",
    "打开。设置分为五组：": "entry. There are five sections:",
    "外观": "Appearance",
    "内置五套主题：经典 · 银灰、棉麻 · 木夹、奶白 · 轻盈、柔彩 · 日常、石墨 · 金属。切换后界面与卡片风格同步变化。":
      "Five built-in themes: Classic silver, Linen with wooden pegs, Porcelain, Soft colour and Graphite metal. Interface and cards change together.",
    "设置": "Setting",
    "鼠标移到屏幕顶部自动弹出挂绳": "Pop the rope open when the cursor reaches the top edge",
    "与托盘菜单的": "Stays in sync with the tray menu's",
    "实时双向同步，状态会保存": "in both directions; the state is saved",
    "与托盘菜单同步；关闭后复制的新图片不再收集":
      "Synced with the tray menu; when off, newly copied images are not collected",
    "与托盘菜单同步；关闭后复制的新文字不再收集":
      "Synced with the tray menu; when off, newly copied text is not collected",
    "与托盘菜单同步；关闭后新图先收进图片篮子，需要时再挂上绳子":
      "Synced with the tray menu; when off, new images go to the basket first and can be hung on demand",
    "界面语言": "Interface language",
    "简体中文 / English，切换后提示重启，重启生效":
      "Chinese / English. You'll be asked to restart; the change takes effect after that",
    "开机时自动启动图绳": "Start PicRope at login",
    "绿色版可直接勾选；与安装包写入同一注册表项，安装时勾选的自启也能在这里关闭":
      "Portable builds can tick it directly. It writes the same registry entry as the installer, so a launch-at-login choice made during setup can be turned off here too",
    "若注册表里的自启项指向旧版本路径，取消勾选再重新勾选一次，即可更新为当前程序的路径。":
      "If the entry still points at an older install, untick and tick it again to repoint it at the current program.",
    "「显示 / 收起挂绳」的热键可以自定义：点进输入框，按下新的组合键即可，默认 ":
      "The Show / hide rope hotkey is customizable: click the field and press a new combination; the default is",
    "热键必须包含 Ctrl、Alt、Shift 或 Win 之一；修改后立即生效并保存。若与其它程序冲突，会提示并回滚到上一个可用热键。":
      "The hotkey must include Ctrl, Alt, Shift or Win; changes apply immediately and are saved. If another program already uses it, PicRope warns you and restores the previous one.",
    "卡片尺寸": "Card size",
    "小 / 中 / 大，改变挂绳上缩略图的大小，切换后立即生效":
      "Small / Medium / Large; changes the thumbnail size on the rope and applies immediately",
    "挂绳不透明度": "Rope opacity",
    "20% ~ 100%，每档 20%；调低后对桌面内容的遮挡更少":
      "20% to 100% in 20% steps; lower values block less of the desktop",
    "收起延迟": "Auto-hide delay",
    "鼠标离开顶部后 300 / 800 / 1200 / 1500 毫秒自动收起，默认 1200 毫秒":
      "Hides 300 / 800 / 1200 / 1500 ms after the cursor leaves the top edge; the default is 1200 ms",
    "数据": "Data",
    "导出数据备份…": "Export data backup…",
    "把": "Pack the",
    "目录打包成 zip，方便迁移到另一台电脑":
      "folder into a zip archive for moving to another computer",
    "打开数据目录": "Open data folder",
    "直接打开": "Open the",
    "文件夹，查看图片与配置": "folder to browse images and settings",
    "恢复默认设置…": "Reset to defaults…",
    "重置所有设置项，保留图片与文字笔记；可选择立即重启":
      "Resets every setting but keeps images and text notes; you can restart right away",
    "关于": "About",
    "显示当前版本号，并可手动": "Shows the current version and lets you",
    "后，启动时会自动查询新版本；发现新版本时可下载并启动安装程序。":
      "makes it look for a new release at startup; when one is found you can download it and start the installer.",
    "快捷键": "Shortcuts",
    "查看器缩放复位到 100%": "Reset viewer zoom to 100%",
    "绳子没有自动弹出？": "The rope doesn't pop up?",
    "确认托盘菜单里的": "Make sure the tray menu's",
    "已勾选；也可以在设置 →「功能」中检查同一项。部分全屏应用会独占屏幕边缘，退出全屏后再试。":
      "is ticked, or check the same switch under Settings → Features. Some full-screen apps grab the screen edge; leave full screen and try again.",
    "截图没有出现在绳子上？": "A screenshot didn't appear on the rope?",
    "检查": "Check whether",
    "是否被关闭。若图片来自其他程序的文件复制（而非图片数据），系统不会写入剪贴板图片，此时需要把文件拖入或在篮子中打开所在文件夹。":
      "has been switched off. If another app copied the image as a file rather than as image data, the clipboard holds no bitmap; drag the file in, or open its folder from the basket.",
    "切换语言后界面没变？": "The interface didn't change after switching language?",
    "语言切换为重启生效。在设置中选择语言后按提示立即重启，或手动退出（托盘菜单":
      "Languages load at startup. After choosing one, restart when prompted, or quit from the tray menu's",
    "）再启动。": "entry and start again.",
    "数据存在哪里？": "Where is my data?",
    "程序目录下的": "In the",
    "文件夹，包含配置与收集的图片、文字。升级与卸载都会保留。":
      "folder next to the program, with settings and everything collected. Upgrades and uninstalls keep it.",
    "新增英文界面；设置新增「功能」栏目（自动弹出、界面语言、开机自启）；托盘与设置状态同步；修复语言下拉取消后状态错位":
      "English interface; new Features section (auto pop-up, interface language, launch at login); tray and Settings stay in sync; fixed the language dropdown keeping the cancelled option highlighted",
    "稳定性与细节优化": "Stability and polish",
    "内置图片查看器；图片篮子支持搜索与重命名；截图按内容去重":
      "Built-in viewer; search and rename in the basket; de-duplication by image content",
    "返回": "Back",
    "功能总览": "Feature overview",
    "下一步": "Next"
  };

  var ATTR = {
    "aria-label": {
      "切换外观": "Toggle appearance",
      "界面语言": "Interface language",
      "图绳 PicRope 图标": "PicRope icon"
    },
    "alt": {
      "图绳 PicRope 图标": "PicRope icon"
    }
  };

  var TITLES = {
    "图绳 PicRope · 截图随手一放，要用时再取": "PicRope · Screenshots, right where you left them",
    "使用文档 · 图绳 PicRope": "Documentation · PicRope",
    "图绳 PicRope · 使用文档": "PicRope · Documentation"
  };

  var EXTRA = {
    toc: "Contents",
    folder: "folder"
  };

  var STORE = "picrope-lang";
  var texts = [];
  var attrs = [];
  var titleText = document.title;

  function norm(s) { return String(s).replace(/\s+/g, " ").trim(); }

  function collect() {
    var walker = document.createTreeWalker(document.body, NodeFilter.SHOW_TEXT, {
      acceptNode: function (n) {
        if (!n.nodeValue || !norm(n.nodeValue)) return NodeFilter.FILTER_REJECT;
        var p = n.parentElement;
        if (!p || /^(SCRIPT|STYLE)$/.test(p.tagName)) return NodeFilter.FILTER_REJECT;
        return NodeFilter.FILTER_ACCEPT;
      }
    });
    var node;
    while ((node = walker.nextNode())) texts.push({ node: node, text: node.nodeValue });
    ["aria-label", "alt", "title", "placeholder"].forEach(function (name) {
      var list = document.querySelectorAll("[" + name + "]");
      for (var i = 0; i < list.length; i++) {
        attrs.push({ el: list[i], name: name, value: list[i].getAttribute(name) });
      }
    });
  }

  function apply(lang) {
    var en = lang === "en";
    texts.forEach(function (o) {
      var raw = o.text;
      var key = norm(raw);
      var tag = o.node.parentElement && o.node.parentElement.getAttribute
        ? o.node.parentElement.getAttribute("data-i18n") : null;
      var hit = en ? (tag && EXTRA[tag] ? EXTRA[tag] : DICT[key]) : null;
      if (hit == null) { o.node.nodeValue = raw; return; }
      var lead = raw.match(/^\s*/)[0];
      var trail = raw.match(/\s*$/)[0];
      o.node.nodeValue = lead + hit + trail;
    });
    attrs.forEach(function (a) {
      var map = ATTR[a.name];
      var hit = en && map ? map[norm(a.value)] : null;
      a.el.setAttribute(a.name, hit || a.value);
    });
    document.title = (en && TITLES[norm(titleText)]) || titleText;
    document.documentElement.lang = en ? "en" : "zh-CN";

    var btns = document.querySelectorAll(".lang-switch button");
    for (var i = 0; i < btns.length; i++) {
      var on = btns[i].getAttribute("data-lang") === lang;
      btns[i].classList.toggle("active", on);
      btns[i].setAttribute("aria-pressed", on ? "true" : "false");
    }
  }

  function setLang(lang) {
    try { localStorage.setItem(STORE, lang); } catch (e) {}
    apply(lang);
  }

  collect();

  var saved = null;
  try { saved = localStorage.getItem(STORE); } catch (e) {}
  var m = /[?&]lang=(en|zh)\b/.exec(location.search);
  if (m) { saved = m[1]; try { localStorage.setItem(STORE, saved); } catch (e) {} }
  apply(saved === "en" ? "en" : "zh");

  window.setPicRopeLang = setLang;

  var sw = document.querySelector(".lang-switch");
  if (sw) {
    sw.addEventListener("click", function (e) {
      var btn = e.target.closest("button[data-lang]");
      if (btn) setLang(btn.getAttribute("data-lang"));
    });
  }
})();
