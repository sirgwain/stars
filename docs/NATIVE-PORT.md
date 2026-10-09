# Native port

The reconstructed sources build as a native Win32/Win64 executable with
MinGW (GCC or MSYS2 Clang) or MSVC, and the game code alone builds as
`stars-host` with any C11 compiler (see Platforms below). This document
covers what the port changes and what it must keep.
Each native-port shim is marked `NATIVE` in the source and described in
detail in [WIN16-PARITY.md](WIN16-PARITY.md).

- **Shims:** the `NATIVE` changes cover malformed player-message records,
  battle heap capacity, and two original uninitialized reads. Window
  procedures handle the Win32 `WM_CTLCOLORMSGBOX`…`WM_CTLCOLORSTATIC`
  messages directly where the original handled Win16 `WM_CTLCOLOR`.
- **Struct sizes:** `structs.h` layouts and `WriteRt`/`ReadRt` record sizes are
  the file format. 17 structs and `RECT` grow under Win64 (HB, MSGPLR, OBJ,
  PART, PLANET, FLEET, BTN, BTNT, DRAWCIR, RPT, SBAR, SEL, TILE, WN, INI,
  XFER, TUTOR). FLEET and PLANET records pack their fields explicitly, partial
  copies stop before the pointers, and other records keep their Win16 sizes.
  Native pointer or handle sizes never reach a save file.
- **Storage widths:** see
  [RECONSTRUCTION.md](RECONSTRUCTION.md#storage-widths-and-boolean-conventions).
  Don't widen `int16_t` to `BOOL`/`int` where storage, addresses or file I/O
  depend on the width.
- **POINT16:** Stars' `POINT` is `POINT16`. Win32 calls use the native `POINT`,
  converted through `PointFrom16`/`PointTo16`. `ShowScanSelChange` and
  `DrawBuildSelComp` pass 32-bit `RECT` fields through `POINT16`/`int16_t`
  locals (marked `NATIVE`). Keep this split when adding Win32 calls.
  `POINT16` is in `native.h`; its conversions (`GetCursorPos16`,
  `ScreenToClient16` and the like) are in `nativeui.h`.
- **Other helpers** (`nativeui.c`): `GetTextExtent` keeps Win16's packed
  width/height result, which about 140 callers split with `LOWORD`/`HIWORD`;
  `FrameWndProcDeferred` posts the frame's restore and maximize commands back
  to the message loop so Wine's macOS driver can't deadlock the
  load-or-unsubmit `MessageBox`.
- **Platform layer** (`native.h`/`native.c`): the game code reaches files,
  the clock and directories only through `HfOpenFile`, `CbReadFile`,
  `CbWriteFile`, `LSeekFile`, `CbFileSize`, `CloseFile`, `FFileExists`,
  `FFileReadOnly`, `MakeDir`, `DwTickCount` and `GetDateTimeSz`, and builds
  paths with `chDirSep`. On Win32 these are the original `OpenFile`,
  `_lread` and related calls; on POSIX they use `open`, `read` and the like,
  which have no share modes, so a file another program has open is not
  refused. The game's heaps (`memory.c`) come from `calloc` and `realloc`.
- **Formatting and rounding** (`native.c`): the game code formats with
  `CchSprintf`, which reads `%ld` as 32 bits like Win32 `wsprintf` (the
  string table passes `int32_t` to `%ld`; `long` is 64 bits on 64-bit POSIX)
  and, like `wsprintf`, writes at most 1024 bytes,
  and rounds with `LMulDiv`, which matches Win32 `MulDiv`. Every format in
  the string table formats the same through both.
- **Platforms:** the game code (`common.h`) builds without Windows headers;
  `stars-host` links it alone. Its turns match `stars.exe`'s where
  `long double` is x87 extended precision: x86-64 Linux and macOS (on
  Apple silicon, built for x86_64 and run by Rosetta). ARM's `long double`
  is 64 bits (macOS) or 128 bits (Linux), so the rounding casts round
  differently; CMake warns about such builds. MSVC's `long double` is also
  64-bit double precision, so its client and host can generate different
  turns from MinGW. The `msvc-debug` and `msvc-release` presets build both
  executables and the Windows unit tests, and the tutorial and trace hooks
  work with all three compilers through test-only source variants. The
  unused zero-length `_ctype` CRT placeholder was removed from
  `globalsui.c`/`globalsui.h` because MSVC rejects zero-length arrays.
  Regression trace output uses native paths for direct execution and Wine
  drive paths only for executables launched through Wine.
- **Toolchain parity (keep):** `qsort16` (`native.c`) reproduces the
  Win16 CRT's tie order, and the x87 rounding casts to `double`/`float` are
  deliberate, so don't simplify them.
- **Warnings kept** (`-Wall -Wextra -Wno-unused-parameter`, 118):
  - **Unused-but-set (79):** debug-info locals the original also stores to,
    probably for asserts or debug output that was compiled out.
  - **Sign-compare (23):** casts such as `(uint32_t)(dx * dx)` record the
    original's unsigned arithmetic. Two are `turn.c`'s `min(...)` of a
    signed and an unsigned value, reported since `min` moved from
    `windows.h` to `native.h`.
  - **Type-limits (14):** enum range checks on unsigned fields.
  - **Tautological compare (1):** `aiutil.c` `IroEnsureAi`
    `(iTechCur & 0xf) == 0x1a` is an original dead branch.
  - **Function cast (1):** `shipui.c` `TransferStuff` casts
    `FEnumCalcJettison`. Both signatures come from the debug info and are
    ABI-compatible.
- **Original uninitialized reads:** eight reads that MSVC Debug checks stop
  on now have defined values. `CreateChildWindows` gives the mine pane a
  temporary size before `RefitFrameChildren` lays out the windows.
  `ScoreXDlg` and `VCRDlg` initialize their unused close results to zero.
  `CommandHandler` starts the tutorial progress tick at the sampled base
  tick and saves the current cursor on the tutorial path before restoring
  it. `IdTargetFreighter` starts its mineral index at zero before copying
  it to the unused `iWorst2`. `LDrawGauge` starts its scale flag false
  because an empty gauge skips the scale calculation. `DrawShipWayPtOrders`
  assigns the mine-laying message ID before using it to choose the color.
  This is not an exhaustive audit of every execution path.
