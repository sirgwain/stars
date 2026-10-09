# Tutorial UI tests

The suite runs AutoHotkey **v2.0.28** and the rebuilt Stars! executable directly on Windows, or inside the same isolated Wine prefix on Linux/macOS. It drives normal Windows controls, menus, keyboard shortcuts and mouse gestures. The test build adds a hidden, read-only observation window; it does not complete tasks or modify game orders.

## Run

Install Python 3, CMake, Ninja and an x64 compiler (MinGW GCC, MSYS2 Clang or MSVC), plus Wine outside Windows.
For MSVC, use an x64 Native Tools command prompt and pass
`--build-preset msvc-debug` (or `msvc-release`). All compilers use the same
observer and tutorial assertions. MSVC runtime-error dialogs are reported as
test failures with their diagnostic text; the runner never dismisses them
to continue a walkthrough.

On Windows, from a shell with native Python, CMake, Ninja and MSYS2 GCC on PATH:

```powershell
python tests/scaffold/tutorial/run.py --ahk "C:/Program Files/AutoHotkey/v2/AutoHotkey64.exe"
python tests/scaffold/tutorial/run.py --download-ahk --scenario reject-generate
```

The installed interpreter must match the pinned version. `--download-ahk`
uses a verified portable copy. Clang users can select
`--build-preset mingw-clang-debug` or `mingw-clang-release` from CLANG64.
Native runs use Windows 10/11 DPI APIs and operate on your interactive desktop: leave mouse and keyboard
idle and provide room for the 1280×960 game and its adjacent tutorial window
(the Wine desktop is 1600×1200). Do not run multiple UI suites concurrently.
Each test build reads and writes `Stars.ini` in its working directory;
the runner stages that file beside the test game, leaving your Windows INI
untouched. Rebuild older `--exe` tutorial binaries before using them natively.
Native logs are named `autohotkey.log`, and cleanup checks the recorded
game PID and executable path before terminating it. Native continuation
attaches only to that retained game; close a retained game normally when done.

```sh
make tutorial
make tutorial-reject
```

`--download-ahk` fetches the official portable release and verifies its pinned SHA-256 before extracting it under `dist/tools/autohotkey`. To supply an existing interpreter:

```sh
python3 tests/scaffold/tutorial/run.py --ahk /path/to/AutoHotkey64.exe
```

The runner builds `dist/tutorial-mingw-debug/bin/stars.exe` with `STARS_TEST_TUTORIAL=ON` using `mingw-debug` by default. To test the optimized Release build, as the GitHub workflow does:

```sh
make tutorial TUTORIAL_ARGS='--build-preset mingw-release'
make tutorial-reject TUTORIAL_ARGS='--build-preset mingw-release'
```

This uses `dist/tutorial-mingw-release/bin/stars.exe` and records the selected preset in `metadata.json`. The observer option defaults to OFF for normal builds. `--exe PATH` accepts a prebuilt executable with that option enabled. Each run copies the executable and scripts into a fresh directory and uses its own INI settings and game files, plus a Wine prefix outside Windows. Wine prefixes are created under `/tmp/stars-tutorial-*`, outside the repository so Wine’s filesystem symlinks are not scanned by workspace tools. The absolute prefix path is printed at startup and recorded as `wine_prefix` in `metadata.json`. The original registration and game files are never modified.

Linux runs need an X11 display. For unattended runs:

```sh
xvfb-run -a -s '-screen 0 1600x1200x24' make tutorial
```

The virtual Wine desktop is 1600×1200. The game occupies 1280×960, and the tutor sits beside it so it cannot intercept scanner clicks. macOS Wine can execute the controls and gestures, but its GDI screen capture can return blank images. The suite logs that limitation instead of saving misleading screenshots or failing an otherwise valid walkthrough.

## Coverage and pass conditions

`run.py` reads all 640 fragments (80 pages) from `text/tutorial.txt`. It exports numeric IDs from `res/resource.h`, and checks that every highlighted instruction assigned by `FTutorTaskDone` (a `TutorId` name or number, resolved through `text/tutorial.txt`) has a dispatcher action. Missing actions fail before launching Wine. `coverage.json` records that inventory.

`steps.ahk` maps highlighted instructions to actions for years 2400–2436: navigation, message filtering, exploration routes, production and templates, colonization and transport, research, ship and starbase design, fleet splitting/merging, reports, battle playback and the final score report. `actions.ahk` contains the reusable UI operations. It uses resource IDs and observed HWNDs for standard controls, and current layout rectangles for custom controls. No RC caption changes are needed. The source tutorial calls planet 7 “Moholdi”; the actual planet name is “Mohlodi”, which the action uses.

A full pass requires all 80 pages, every required task before each Generate, no panic auto-completion, the final completion message and departure from tutorial mode at turn 37. Most tutorial Generate operations load canned turn files, so the suite checks the live task state and records the orders **before** generation. The final generation runs the normal simulation. Early victory messages do not count as tutorial completion.

The separate `reject-generate` scenario tries Generate before the first task is complete, checks that the turn does not advance, and verifies that the tutorial remains usable afterward.

For incremental development:

```sh
python3 tests/scaffold/tutorial/run.py --until-year 2403 --timeout 180
```

An intentional early stop reports `partial`, produces a skipped JUnit case, and exits nonzero. It cannot be mistaken for full coverage. Unknown instructions, covered click targets, unexpected tutorial errors, stalls, crashes, syntax failures and timeouts also produce nonzero results.

For debugging later actions without replaying every earlier year, launch with `--keep-game-on-failure`. After fixing the action, use `--continue-run /absolute/path/to/the/original/run --keep-game-on-failure` to attach to that live game. Windows uses its recorded PID and executable path; Wine uses the existing prefix recorded in `metadata.json`. Wine prefixes must still exist; clearing `/tmp` prevents Wine continuation. Older runs without the required metadata need a fresh run. A continuation can report failures or `partial`; it cannot report a full walkthrough pass. It requires the game to remain running, and does not reopen a saved game. Finish debugging with a fresh normal run. Close a retained native game normally. Under Wine, run `WINEPREFIX=/absolute/prefix/path/from/metadata.json wineserver -k`.

## Artifacts

Each run is retained under `dist/scaffold/tutorial/<UTC timestamp>/`:

- `result.json` and `results.xml`: explicit outcome and failure location.
- `events.jsonl`: instruction text, action times, queue contents and diagnostics.
- `year-2400.txt`, etc.: live orders and state before each generation.
- `last-state.txt` and `windows.txt`: observer snapshot and window/control inventory on failure.
- `screenshots/`: BMP captures of the Windows desktop, or where supported by the Wine display driver.
- `wine.log` (Wine) or `autohotkey.log` (Windows), and `metadata.json`: interpreter diagnostics, versions and executable/source hashes.
- `game/coverage.json`: required instruction inventory.

By default, the runner closes only its own game and, when used, its Wine server. The explicit debugging option retains a failed game. Prefixes contain registration data, remain in `/tmp` for debugging, and may be removed by the operating system; the GitHub workflow uploads diagnostic files only. It runs the full walkthrough and rejection check on pull requests and supports manual runs.

A populated instruction dispatcher is a coverage inventory, not proof that every gesture works on every Wine driver. `result.json` is the runtime authority; a full walkthrough is validated only when it reports `passed` with 80 pages at the final year.
