# Repository Instructions

## Goal

These sources began as a reconstruction of the original Stars! 2.6jrc3 C
source, made from the debug-rich Win16 `stars.exe`. The faithful
reconstruction is finished: it is tagged `2.6jrc3` and kept on the `2.6j`
branch, which still follows the reconstruction rules in its own `AGENTS.md`
(`git show 2.6j:AGENTS.md`).

This is the `2.6j-linux` branch: the former 2.8 line (tag `v2.8.0`). It fixes
the original's bugs and turns the Win16 shims into native Win32 code, while
staying a game that plays like 2.6j and reads its files. The branch takes
bug fixes for patch releases (`v2.8.1`, ...) only: no restructuring, new
features or cleanup. New work, starting with the 2.9 split of the core, UI
and host, happens on `main`. Make a fix on `main` first when it applies
there too, and bring it here with `git cherry-pick -x`.

When goals conflict, apply them in this order:

1. **Compatibility:** save, turn, history and race files from Stars!
   2.6j/2.7 (file format version 2.83) stay readable. 2.8 writes version
   2.84, which 2.6j/2.7 refuse, so 2.8 files are for 2.8 only. On-disk
   formats and record sizes do not change.
2. **Behavior:** game behavior changes only on purpose. Each intentional
   change is its own commit, moves the native regression baseline in that
   commit, and is recorded in `CHANGELOG.md`.
3. **Readability:** clear native C. Prefer the existing structure and idiom;
   change it when that makes the code clearer or removes Win16 baggage.

## Code rules

- Names from the debug symbols (functions, params, locals, globals, struct
  fields, enums) stay. They tie the code to the original's debug symbols
  and `docs/KNOWN-BUGS.md`. New names use the same Hungarian style: `c` count,
  `i` index, `f` flag, `lp` pointer, `rg` array, `h` handle, `psz`/`sz`
  strings, `id`/`ish`/`ipl` and the like. Check `structs.h` and nearby code
  for existing prefixes before you invent one.
- Build as C11 (GNU extensions on). Keep the surrounding style: locals at the
  top of a function, `//` or `/* */` comments, no reformatting beyond the
  lines you change. Helpers are fine when they replace a Win16 shim or remove
  real duplication; keep them small and next to their callers.
- Truth tests stay bare for flags (`f…`), `F…()` calls, pointers, handles and
  bit tests; keep explicit `== 0`/`!= 0` for counts, indices and IDs.
- Comments explain what a reader can't see in the code: why a fix exists,
  what the original did, a file-format constraint.
- Don't mix kinds of change in one commit. Behavior-neutral cleanup (shim
  removal, warnings) must leave the regression baseline unchanged.

## Bug fixes

- Fix one bug per commit. Name the function and the original behavior in the
  commit message, add a `CHANGELOG.md` entry, and update or remove its entry
  in `docs/KNOWN-BUGS.md`, `docs/WIN16-PARITY.md` or `docs/ROADMAP.md`.
- A fix that changes turn generation changes host results; note it in
  `CHANGELOG.md`. A 2.8 host can still pick up a game from 2.6j files, so
  a fix must work on state written by 2.6j.
- Regenerate the native baseline in the same commit (see Verification) and
  state which scenarios moved and from which turn.

## Marking code

- `/* NATIVE: ... */` marks code needed only because the build is Win32 or
  64-bit (pointer size, `POINT16`, heap headers, CRT differences). As shims
  are replaced with plain Win32 code, the marker and its
  `docs/WIN16-PARITY.md` or `docs/NATIVE-PORT.md` entry go with them.
- Don't add `/* PARITY: ... */` markers on `main`. Reproducing Win16 bugs
  for the regression belongs on the `2.6j` branch.
- `#ifdef STARS_TEST_*` marks test-harness code.

## Things that must not change

- `structs.h` layouts that reach disk, `WriteRt`/`ReadRt` record sizes and
  on-disk formats stay at their Win16 sizes. Native pointer or handle sizes
  must never reach a save file. See `docs/NATIVE-PORT.md`.
- Element widths of fields, arrays and file records stay as they are. Don't
  widen `int16_t` to `BOOL`/`int` where storage, addresses (`&f…`) or file
  I/O depend on the width. See `docs/RECONSTRUCTION.md`, "Storage widths and
  boolean conventions".
- `qsort16` and the x87 rounding casts decide tie order and rounding in turn
  generation. Changing them is a behavior change, not a cleanup.
- The tutorial observer depends on `InitInstance`, `ScannerWndProc`, the
  globals and struct layouts it reads, and the literal `tutor.c` signatures
  `int16_t FTutorTaskDone() {` and `int16_t FCheck`. Update the observer and
  parser in the same change if any of these move.

## Versioning

- The product version comes from git: release tags are `vMAJOR.MINOR.PATCH`
  (`v2.8.0`), and builds between tags are numbered from the last tag. CMake
  generates `version.h`; see `docs/VERSIONING.md`.
- The file format version written to saves is separate: 2.84. 2.8 reads
  2.49 through 2.84.

## Reference material

- If present, read [AGENTS-private.md](AGENTS-private.md) for additional local
  instructions. It is intentionally untracked; keep its contents and the
  materials it describes private.
- `structs.h` defines every struct. Use it to interpret offsets, bitfields and
  `LOWORD`/`HIWORD` splits.
- To see what the original did before changing it, especially for a bug
  fix, list the function from the binary with `stars-asm dasm asm|sem -n
  <Func>`; the `2.6j` branch keeps the faithful reconstruction.
- `docs/KNOWN-BUGS.md` maps reported original bugs to source.
  `docs/WIN16-PARITY.md` lists the remaining Win16 guards and boundaries.
  `docs/NATIVE-PORT.md` covers the native port. `docs/ROADMAP.md` lists the
  work left. Update them when an item is finished or found.

## Verification

After each batch of edits:

1. Build with `cmake --preset mingw-debug && cmake --build --preset mingw-debug`.
   Don't introduce new warnings in the files you touched.
2. Run the native regression and compare it against the native baseline in
   `tests/scaffold/fixtures/regression/native/`: `make regression` (details
   in `tests/scaffold/REGRESSION.md`). A quick check is `make
   regression-quick`, one scenario through checkpoint 10; a batch is done
   only after the full suite.
3. Run the unit tests (`make test-unit`, `tests/unit/README.md`).
4. Run the tutorial (`make tutorial`).

Behavior-neutral changes must match the baseline exactly (unused-storage
warnings excepted). A behavior change regenerates the baseline with
`make regression-export` in the same commit. Report regression
results as they are: if a scenario diverges unexpectedly, show the first diff.

The original game's checkpoints in `fixtures/regression/original/` are the
frozen record of 2.6j behavior. The DOSBox setup that made them is gone, so
they can't be regenerated; don't change them.

Keep commits small and focused on one kind of change so that a regression
can be bisected to a single commit.

## After changes

Review the diff against these rules. In particular, check for renamed
debug-symbol names, file-format or record-size changes, behavior changes
without a baseline update and `CHANGELOG.md` entry, and stale entries in
`docs/WIN16-PARITY.md`, `docs/NATIVE-PORT.md` and `docs/ROADMAP.md`.
