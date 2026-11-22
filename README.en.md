<div align="center">

# qt_PicRope · PicRope (Qt Rebuild)
English | [简体中文](README.md)

</div>

> **Qt secondary-development version**, based on [PicRope](https://github.com/Renly1994/PicRope) by [Renly1994](https://github.com/Renly1994) — a Windows screenshot staging and desktop-pin tool. This repository rebuilds and extends the original **C# / WPF** project in **C++ / Qt Widgets**.

**PicRope (图绳)** is a Windows screenshot staging and desktop-pin tool. It automatically collects screenshots onto a "rope" along the top edge of the screen: move the mouse to the top to expand it, and it collapses when the mouse leaves. Click an image to copy it, then drag it into chat windows, documents or any other target app; double-click to open it in the built-in viewer. It is handy for writing docs, making tutorials and organizing reference images.

---

## 1. Origin

- **Upstream repository (original)**： <https://github.com/Renly1994/PicRope> (Windows screenshot staging and desktop-pin tool, C#/WPF)
- **This repository (Qt secondary development)**： <https://github.com/2058763315/qt_PicRope>
- **Positioning**： a Qt secondary-development version of the author's C#/WPF implementation, rewritten in C++17, with UI and interactions aligned to the original and reusing its feature design (screenshot collection, rope, basket, text notes, built-in viewer, tray, multi-monitor, UI language and themes).
- Comments at the top of each source file state the mapping to the original module (e.g. `app.h ↔ App.cs DesktopController`, `ui/shelf.h ↔ ShelfWindow`, `core/gallery.h ↔ Gallery`), making it easy to compare against the original design.
- The in-app update check is based on this repository's GitHub Releases (`2058763315/qt_PicRope`).

---

## 2. Features

| Feature | Description |
| --- | --- |
| **Screenshot collection** | Automatically scans the Windows Screenshots folder for new images; also supports clipboard images and text, plus manual import |
| **Rope staging** | New images go onto the rope (up to 50 most recent) or into the image basket according to the "auto-hang" switch; cards overlap automatically to fit the visible width when there are many |
| **Copy & drag** | Click an image to copy it and drag it into a target app (drag copies only); right-click to "take off the rope" or batch take-off |
| **Built-in viewer** | Double-click opens the built-in viewer: switch among images in the same folder with ← / →, zoom with the wheel, press `0` to reset zoom, close with Esc; the bottom bar shows the file name, zoom and index |
| **Image basket** | Search by file name, right-click to rename, open the containing folder; right-click "copy to clipboard", "open with the system editor", "hang on rope / take off rope", "delete"; double-click preview, rubber-band or Ctrl/Shift multi-select |
| **Text notes** | Collects clipboard text (called "text envelope" in the original); supports search, full-text view, copy and multi-delete, preserving original line breaks and indentation |
| **Settings** | Opened from the tray "Settings…", centered on screen, with five sections: **Features / Shortcuts / Appearance / Data / About** |
| **UI language** | Simplified Chinese / English, applied by an automatic restart; UI strings are loaded from Qt translation files (`translations/PicRope_en.qm`) |
| **Custom hotkey** | The show/hide global hotkey can be changed (default `Ctrl+Alt+T`) and must include Ctrl/Alt/Shift/Win; a failed registration is reported and rolls back automatically |
| **Appearance tuning** | Card size small / medium / large, rope opacity 20%–100%, and auto-collapse delay 300 / 800 / 1200 / 1500 ms, applied immediately |
| **Data management** | Export / import a data backup (Zip with settings, images and text notes), open the data folder, restore default settings (keeping images and notes) |
| **Launch at startup** | A switch in Settings that writes the current user's registry Run key; the auto-start option ticked in the installer wizard is reflected here as well |
| **In-app update** | Silently checks the latest GitHub release after startup; when a new version is found it asks, then automatically downloads the installer and launches it (auto check can be disabled in Settings) |
| **Region capture** | Launches the system screenshot tool (`ms-screenclip:`, default shortcut `Win + Shift + S`); the tray "More → Region capture…" item also triggers it |
| **Tray control** | Show/hide, Image basket…, Text notes…, clipboard image/text collection switches, auto pop-up, auto-hang new images, Settings, More (region capture / open collection folder / clear the rope keeping files), Exit |
| **Multi-monitor** | Monitor selection (follow mouse or fixed), panels shown on the monitor under the cursor, auto-collapse at the top, click-through |

---

## 3. Project Structure

### 3.1 Directory layout

```
qt_PicRope/
├── main.cpp              Entry: font, single-instance lock, state loading, UI translation
│                         by setting, start controller
├── app.h / app.cpp       Desktop controller (AppController): assembles modules, handles
│                         actions, listens to clipboard, binds tray/settings/hotkeys, update scheduling
├── system/               Windows platform capability wrappers
│   ├── win32.h/.cpp      Global hotkey (RegisterHotKey, custom combinations), monitor info,
│   │                     topmost/click-through window, cursor position, foreground-window detection
│   └── clipboard.h/.cpp  Clipboard read/write: image, text, file references, own-format marker
├── core/                 Core business logic
│   ├── state.h/.cpp      State persistence: settings, language, card size, opacity, hide delay
│   │                     and collection paths in data/state.json
│   ├── image.h/.cpp      Image read/validation, thumbnail generation, pixel fingerprint (dedup)
│   ├── gallery.h/.cpp    Collection management: rope items + collected list, shared items
│   │                     managed with QSharedPointer
│   ├── texts.h/.cpp      Text notes (text collection) data source
│   ├── capture.h/.cpp    Capture pipeline: folder scan, clipboard collection, dedup,
│   │                     manual import, inbox copy management
│   └── updater.h/.cpp    Update check: request latest GitHub release, version compare,
│                         download installer (Qt Network)
├── ui/                   UI layer
│   ├── themes.h/.cpp     Five themes (Classic Silver / Linen / Porcelain / Soft Color / Graphite)
│   ├── shelf.h/.cpp      Rope main window: topmost transparent strip, auto expand/collapse at
│   │                     the edge, click-through areas, card overlap layout, size/opacity/delay
│   ├── card.h/.cpp       Single image card: click to copy, double-click to open, long-press to edit,
│   │                     context menu, drag
│   ├── viewer.h/.cpp     Built-in viewer: ←/→ browsing, wheel zoom, 0 to reset, drag pan
│   ├── pinned.h/.cpp     Desktop pin window (source kept, currently not compiled)
│   ├── panels.h/.cpp     Image basket + text notes (rounded borderless panels)
│   ├── settings.h/.cpp   Settings: Features / Shortcuts / Appearance / Data / About
│   └── tray.h/.cpp       System tray menu and switches
├── docs/                 Project website (static, can be hosted on GitHub Pages)
│   ├── index.html        Home page: feature overview and download entry
│   ├── guide.html        User guide: installer/portable, operations
│   ├── style.css         Site styles
│   ├── site.js           Page interactions
│   └── i18n.js           Chinese/English copy and language switch
├── resources.qrc         Resource manifest (app.ico)
├── app.ico               Application icon
├── translations/         UI translations (Qt Linguist project)
│   ├── PicRope_en.ts     English translation source
│   └── PicRope_en.qm     Compiled translation loaded at runtime
└── qt_PicRope.pro        qmake project file (core/gui/widgets/network, TRANSLATIONS, .qm copy)
```

### 3.2 Three-layer architecture

| Layer | Directory | Responsibility | Typical types |
| --- | --- | --- | --- |
| System layer | `system/` | Interacts with Windows capabilities: hotkeys, monitors, clipboard, window effects | `Win32`、`Clip`、`SystemEvents` |
| Core layer | `core/` | UI-independent business logic and data: state, collection, capture, dedup, update | `StateStore`、`Gallery`、`TextCollection`、`CapturePipeline`、`ImageContent`、`UpdateChecker` |
| UI layer | `ui/` | All visible windows and widgets, interactions and appearance | `ShelfWindow`、`ShotCard`、`ImageViewer`、`PanelWindow`、`BasketWindow`、`TextsWindow`、`SettingsDialog`、`TrayController` |

- **Dependency direction**：`ui → core → system`; `core` does not depend on the UI, which keeps it reusable and testable.
- **Assembly hub**：`AppController` (`app.cpp`) glues the three layers together — it owns the module instances, wires "UI action → business handling" through signals/slots, and listens to the system clipboard, global hotkeys and update results.

### 3.3 Key design points

- **Shared item model**：`Gallery` uses `QSharedPointer<CaptureItem>` so the "rope" and "collected" lists share the same objects; cards and the built-in viewer can safely extend an item's lifetime, avoiding dangling pointers.
- **Clipboard echo prevention**：images written back to the clipboard carry a "PicRope own format" marker, so the collector skips its own writes; a pixel fingerprint (sampled SHA-256) is used for image deduplication.
- **Atomic persistence**：`state.json` is written atomically with `QSaveFile`; every image is copied into the `data/Captures` inbox next to the program.
- **Card overlap layout**：the rope lays cards out with overlap as needed within the container width (each card advances by a bounded distance), so the whole row fits the visible width without horizontal scrolling; changing card size recomputes the rope height.
- **Built-in viewer**：`ImageViewer` is a fixed-size window that browses images in the same folder in order, zooms with the wheel, resets with `0`, pans when zoomed, and shows the file name, zoom and index in its bottom bar.
- **Panel pre-creation & multi-monitor placement**：the basket / text notes panels are pre-created while idle after startup (with native handles pre-created); when shown, they anchor to the rope tool dock if it is on the monitor under the cursor, otherwise to that monitor's top-right corner.
- **Custom hotkey with rollback**：`registerToggleHotkey(sequence)` maps a Qt key combination to Win32 modifiers and a virtual key code, accepting only combinations with modifiers; if registration fails it warns and rolls back to the last working hotkey, so Settings never shows a hotkey that cannot actually summon the rope.
- **UI language**：the UI language is stored as `Language` in `state.json` (default `zh_CN`); at startup, if it is `en`, a `QTranslator` loads `translations/PicRope_en.qm` next to the program, and switching the language in Settings restarts the app automatically.
- **Data backup & restore**：export packs `data/` into a Zip; import overwrites current settings and data and prompts for a restart; "restore default settings" only resets settings and keeps images, text notes and collected files.
- **Launch at startup**：the Settings switch reads/writes the current user's registry key `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`; installer and portable builds share the same state.
- **In-app update**：`UpdateChecker` requests this repository's latest release JSON, compares versions segment by segment, prefers an exe whose asset name contains `Setup`, downloads it to a temp folder and then launches the installer; it checks silently about 4 seconds after startup (controlled by `autoCheckUpdate`, enabled by default).
- **Single instance & fault tolerance**：`QLockFile` guarantees a single instance and stale locks left by a crash can be cleaned up; startup failures are written to `data/error.log`.

---

## 4. Build & Run

**Requirements**

- Qt 6 (**Widgets + Network** modules), a C++17 compiler (MSVC or MinGW; MSVC is built with `/utf-8`)
- Windows 10 / 11 64-bit
- Building the English UI needs Qt Linguist tools (`lrelease`) to generate `.qm` (a ready-made `PicRope_en.qm` is already included)

**Build steps**

```bash
# Open qt_PicRope.pro in Qt Creator, pick a kit and build/run; or build from the command line:
qmake qt_PicRope.pro
make          # MinGW: mingw32-make; MSVC: nmake / jom
```

**Translations**

- The English UI is compiled from `translations/PicRope_en.ts` into `PicRope_en.qm` (`lrelease PicRope_en.ts`).
- The project configures `TRANSLATIONS` and a post-build copy step: the `.qm` is copied to `translations/` next to the build output and loaded at runtime according to the `Language` setting; without the `.qm` the UI falls back to Simplified Chinese.

**Runtime directory layout**

- State & settings：`data/state.json` next to the program (settings, theme, UI language, card size, opacity, hide delay, hotkey, rope and collection paths, text notes)
- Image inbox：`data/Captures/` next to the program (copies of collected and manually imported images)
- UI translation：`translations/PicRope_en.qm` next to the program
- Error log：`data/error.log` (hotkey registration failures, update errors, etc.)

**Release, website & update**

- Windows installers (`*Setup*.exe`) are published via GitHub Releases; the in-app updater reads that release and launches the installer after download.
- `docs/` is the accompanying project website (home page + user guide, Chinese/English switch); it is fully static and can be hosted directly, e.g. on GitHub Pages.

**Shortcuts & operations**

| Operation | How |
| --- | --- |
| Show / hide the rope | Default `Ctrl+Alt+T` (changeable in Settings → Shortcuts), or click the tray icon |
| Region capture | `Win + Shift + S` (default shortcut of the system screenshot tool, not editable in the app; the tray "More → Region capture…" item also triggers it); the result goes to the clipboard |
| Open an image | Double-click a card to use the built-in viewer (← / → in the same folder, wheel zoom, `0` to reset, Esc to close) |
| Copy / edit | Click to copy; long-press or right-click → "Edit" (system default image editor) |
| Image basket | Search by name, right-click rename, open folder, copy to clipboard, open with the system editor, hang/take off the rope, double-click preview, rubber-band or Ctrl/Shift multi-select, delete |
| Text notes | Search, full-text view, copy, rubber-band multi-select, "delete selected" |
| Settings / theme / language | Tray "Settings…" or the gear button on the rope tool dock; switch themes, adjust card size / opacity / hide delay, change the hotkey, and switch the UI language (a language change restarts the app) |
| Data management | Settings → Data: export/import a backup, open the data folder, restore default settings |
| Tray management | Show/hide, Image basket…, Text notes…, clipboard image/text collection switches, auto pop-up (persisted), auto-hang, Settings, under "More": region capture / open collection folder / clear the rope (keeping files), Exit |
