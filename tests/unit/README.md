# Unit tests

Each `test_<name>.c` here is an [acutest](https://github.com/mity/acutest)
program linked with the game code (the `stars_core` and `stars_ui` object
libraries) and its resources. They build as Windows console programs with
the normal MinGW build, and CTest runs them through Wine:

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
(`--wrap` is GNU ld's) and the dialog tests in `test_battle.c` and
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

## Floating point migration references

`test_floating_point.c` freezes the current MinGW arithmetic in sixteen test
cases, including Windows UI geometry and sorting. Its fourteen
`golden/floating-*.txt` files use LF and record float/double bits, integer
results and text. They are read-only expectations; failures leave
`floating-*.actual.txt` in the test working directory. Do not replace these
references to make a SoftFloat migration pass. See
[the validation audit](../../docs/SOFTFLOAT-VALIDATION.md) for provenance,
coverage limits, the deliberately failing double-precision control, and
the full before/after procedure.

The AI case observes `Random` and the bombing case observes `FSendPlrMsg`
with GNU ld's `--wrap`; CMake omits those two cases on Apple hosts. Windows additionally wraps text output and supplies
fixed text metrics to isolate diagonal-text arithmetic from font rendering.
These wrappers belong only to this test executable. UI tests are omitted
from non-Windows builds.

`test_pow.c` adds four cases: exact native compatibility, MPFR-referenced
accuracy, packet integer truncation, and special values. The 2,117-row
`floating-pow.txt` stores binary64 input, exponent, native result and correctly
rounded result bits. It is not a runtime-generated expectation. The offline
capture program is `tests/scaffold/pow_reference.c` (requires MPFR/GMP).
MPFR/GMP are restricted to offline test tooling. They must not be linked,
bundled, or copied into the game, host or production `pow` implementation,
including as a fallback. Normal builds and unit tests use the frozen numeric
references and require neither library.

The default backend is now `Sf64Pow`. To test a candidate, configure a separate build with
`-DSTARS_TEST_POW_SOURCE=/absolute/path/to/adapter.c`. The adapter defines
`double DStarsTestPow(double x, double y)`; preserve the binary64 interface
using bit conversions when calling SoftFloat. `STARS_TEST_POW_LIBRARIES` can
supply its link dependencies. This replaces the backend in `test_pow` and,
with GNU ld, wraps game `Sf64Pow` calls in `test_floating_point` too. It does not
change the production executables. An adapter must not call wrapped `Sf64Pow`
recursively; GNU adapters forwarding to the software backend use `__real_Sf64Pow`
in the game test (only `test_pow` defines `STARS_TEST_POW_CUSTOM`).

`STARS_TEST_POW_MAX_ULP` defaults to 0. The compatibility test requires exact
native bits except for the single explicitly identified GCC rounding error,
where it requires the oracle's exact result. Both original values remain in
the frozen file. Do not loosen tolerances or rewrite references to hide differences.

`test_sfnum.c` adds three cases: 14,000 frozen independent reference results
for all seven mathematical functions, 10,000 extended-precision operation
sequences compared with native x87 when available, and special-value handling.
The offline generator is `tests/scaffold/softfloat_reference.c`. Neither it
nor MPFR/GMP is part of CMake's production or unit-test build. Run
`tests/scaffold/check_software_float.py --build BUILD --objdump OBJDUMP` to
reject native x86 floating arithmetic in the compiled game/UI objects.
See [the implementation report](../../docs/SOFTFLOAT-IMPLEMENTATION.md).

## Bug fixes

A bug fix in [docs/ROADMAP.md](../../docs/ROADMAP.md) step 5 adds a test that
fails on the code before the fix. Name the test after the behavior it
checks and put it in the file for the source file that holds the fix.
The game's message boxes are recorded in `cStarsTestAlert`/`szStarsTestAlert`
instead of shown (Yes/No boxes answer Yes): on Windows every test links with
`--wrap=AlertSz`, and natively `stars_test.c` supplies `IdAlertBox`. `test_native_ports.c` also wraps file I/O (see
`CMakeLists.txt`); give other tests that need wraps the same treatment.
