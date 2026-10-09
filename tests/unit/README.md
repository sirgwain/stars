# Unit tests

Each `test_<name>.c` here is an [acutest](https://github.com/mity/acutest)
program linked with the game core, Windows UI and resources. They build as Windows console programs with
GCC, Clang/MinGW or MSVC. CTest runs them directly on Windows and through
Wine when cross-compiling:

```sh
make test-unit                           # build mingw-debug and run them all
make test-unit CTEST_ARGS='-R test_turn' # one file
dist/mingw-debug/tests/test_turn.exe --list   # under wine: the tests in a file
```

They also build natively with the host presets, linked with the game code
and `hostui.c` instead of the Windows interface, and run without Wine:

```sh
make test-host                           # macos-host-release on Apple silicon, host-release elsewhere
make test-host CTEST_ARGS='-R test_turn'
```

The native build leaves out what needs Windows: `test_native_ports.c`
(it also tests Windows controls) and the dialog tests in `test_battle.c` and
`test_race.c` (`#ifdef _WIN32`). Use `szDirSep` for paths and `getcwd` for
the working directory so a test builds on both.

Acutest runs every test in its own process, so each test starts from the
game's initial globals. Tests run in `<build>/tests`, where CMake copies the
fixture race to `data/humanoid.r1`.

## Two kinds of test

- **Function tests** call one game function with crafted inputs, such as
  `FIntersectCircleLine` in `test_utilgen.c`.
- **Turn tests** build a game and generate turns with the helpers in
  `stars_test.h`: `FStarsTestInit`, `FStarsTestDir` (a fresh
  `work/<name>` directory), `FStarsTestNewGame` (a tiny universe with the
  test race and any AI players), `FStarsTestLoadHost` and
  `FStarsTestGenerate`. Between loading the host and generating, a test can
  change the loaded game (`LpplStarsTestHomeworld`, `LpflStarsTestAddFleet`,
  or directly) and save it with `FStarsTestSaveHost`. AI players give their
  orders while one turn generates and the next turn carries them out, so AI
  behavior needs two generations. `test_turn.c` shows the plumbing and
  `test_ai4.c` a full example.

## Golden files

`test_report.c` checks the universe, planet and fleet dumps against
`tests/unit/golden`, so the Windows build and `stars-host` keep writing
the same files. If a change to the dumps is intended, regenerate them from
the Windows build and commit them with the change:

```sh
cd dist/mingw-debug/tests && STARS_UPDATE_GOLDEN=1 wine ./test_report.exe
```

## Bug fixes

A bug fix in [docs/ROADMAP.md](../../docs/ROADMAP.md) step 5 adds a test that
fails on the code before the fix. Name the test after the behavior it
checks and put it in the file for the source file that holds the fix.
The game's message boxes are recorded in `cStarsTestAlert`/`szStarsTestAlert`
instead of shown (Yes/No boxes answer Yes): on Windows every test replaces
cross-file calls to `AlertSz`, and natively
`stars_test.c` supplies `IdAlertBox`. `test_native_ports.c` also replaces file
I/O. `cmake/test-hooks.cmake` generates test-only translation units that rename
real functions and their same-file calls; the harness supplies the public
names. This preserves GNU `--wrap` semantics without depending on a linker.
Product objects are unchanged. Register new replacements in that CMake helper
and select them for the appropriate target in `CMakeLists.txt`.

For MSVC, run in an x64 Native Tools command prompt:

```sh
cmake --preset msvc-debug -DSTARS_BUILD_TESTS=ON
cmake --build --preset msvc-debug
ctest --test-dir dist/msvc-debug --output-on-failure
```

Use `mingw-debug` or `mingw-clang-debug` in the corresponding compiler environment
for the same suites, or the matching `-release` preset to test optimized code.
