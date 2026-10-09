# Stars!

![meme](docs/images/windows10stars.jpg)

Stars! 4X game rebuilt from decompiled C sources using a custom [stars-asm](https://github.com/sirgwain/stars-asm/tree/main) win16 disassembler the decompiler.

The original 2.6jrc3 stars.exe included ~1MB of debug symbols with function names, variable names, symbol definitions, and even line numbers. The stars-asm project used that information to rebuild the Stars! source to be as close to the original as possible.

## Branches

- `2.6j` (tag `2.6jrc3`): the faithful reconstruction, original bugs
  included. It matches the original game turn for turn in the fixed-seed
  regression.
- `2.8` (tag `v2.8.0`): the released 2.8 line, bug fixes only. It fixes
  original bugs and replaces the Win16 shims with native code. It reads
  2.6j files and writes the same formats, marked as version 2.84 so that
  2.6j doesn't load them.
- `main`: the 2.9 line, splitting the game into core, UI and host builds so
  the host (`stars-host`) runs on Linux and macOS.

## Documentation

- [Reconstruction](docs/RECONSTRUCTION.md): how the source was rebuilt from the
  decompiler's output, and the conventions it follows.
- [Native port](docs/NATIVE-PORT.md) and [Win16 parity](docs/WIN16-PARITY.md):
  what the Win32/Win64 build changes, and the original behavior it reproduces.
- [Known bugs](docs/KNOWN-BUGS.md): the original release bug list, mapped to
  source.
- [Roadmap](docs/ROADMAP.md): the 2.9 split and the bug fixes still open.
- [Changelog](CHANGELOG.md) and [versioning](docs/VERSIONING.md): what 2.8
  changes from 2.6j, and how versions and build numbers are assigned.

## Build

The game (`stars.exe`) uses Windows APIs. On macOS or Linux, build it as a
Windows executable with CMake 3.23 or later, Ninja, Python 3 with PyYAML, and
the x86_64 MinGW-w64 toolchain on PATH. The host alone (`stars-host`, below)
builds with the native compiler. The build runs `text/textgen.py` to compile
the game's strings, messages, tutorial and planet names from `text/*.txt`,
and `data/datagen.py` to compile the parts, races, default battle plans, AI
design recipes, part preferences and research orders from `data/**/*.yaml`.
On macOS these build dependencies can be installed with Homebrew:

```sh
brew install cmake ninja mingw-w64
```

Homebrew's Python doesn't let `pip` install into it, so put PyYAML in a
virtualenv and point CMake at it:

```sh
python3 -m venv ~/.venvs/stars && ~/.venvs/stars/bin/pip install pyyaml
cmake --preset mingw-debug -DPython3_EXECUTABLE=$HOME/.venvs/stars/bin/python
```

On Debian or Ubuntu, install `python3-yaml`.

From the project directory:

```sh
cmake --preset mingw-debug
cmake --build --preset mingw-debug
```

The executable is written to `dist/mingw-debug/bin/stars.exe`. CMake also compiles the
resources in `res/` and tracks their embedded files for rebuilds.

Wine is optional for building. If `wine` is on PATH when configuring, run with:

```sh
cmake --build --preset run-wine
```

For a release build:

```sh
cmake --preset mingw-release
cmake --build --preset mingw-release
```

This writes `dist/mingw-release/bin/stars.exe` with optimization enabled and
debug data stripped. Test hooks are disabled in ordinary builds. The MinGW
presets also build `stars-host.exe`.

On Windows with MSYS2 GCC, run these presets from the UCRT64 or MINGW64
environment, with CMake, Ninja and Python with PyYAML available. From
PowerShell, put `C:\msys64\ucrt64\bin` (or `C:\msys64\mingw64\bin`) first
on `PATH`. The toolchain uses MSYS2's unprefixed tools on Windows. Run
`ctest --test-dir dist/mingw-debug --output-on-failure` to execute the unit
tests directly on Windows.

### Clang with MinGW (MSYS2)

Use the MSYS2 **CLANG64** environment, with CMake, Ninja and Python with
PyYAML available. From PowerShell, put `C:\msys64\clang64\bin` first on
`PATH`. These presets use Clang's GNU Windows target and LLVM tools:

```sh
cmake --preset mingw-clang-debug
cmake --build --preset mingw-clang-debug
ctest --test-dir dist/mingw-clang-debug --output-on-failure
```

Use `mingw-clang-release` for Release. Each preset builds `stars.exe` and
`stars-host.exe` in `dist/<preset>/bin/`, plus the Windows unit tests.
GCC continues to use the `mingw-debug` and `mingw-release` presets, with
separate build directories so the compilers can be used side by side.

### Visual C++ (MSVC)

Install Visual Studio 2022 (17.0 or newer) or newer Visual Studio with the
Desktop development with C++ workload, CMake, Ninja, and Python with PyYAML.
From an **x64 Native Tools Command Prompt**, run:

```sh
cmake --preset msvc-debug
cmake --build --preset msvc-debug
```

This builds `dist/msvc-debug/bin/stars.exe` and `stars-host.exe`. Use
`msvc-release` in both commands for an optimized build in
`dist/msvc-release/bin/`. Visual Studio's Open Folder workflow can also
select these presets and supply the x64 compiler environment.

MSVC uses double precision for `long double`, so generated turns can differ
from the MinGW regression baseline; CMake warns about this. Keep using MinGW
when matching those turns is required. All Windows compilers build the same
unit tests by default and support the tutorial observer and regression trace
hooks. Test-only source variants replace calls without requiring GNU linker
wrapping.

Run `ctest --test-dir dist/msvc-debug --output-on-failure` for unit tests.
The regression runner accepts either MSVC executable with `--exe`; compare
against the unchanged native baseline to measure compiler differences.
Run `python tests/scaffold/tutorial/run.py --build-preset msvc-debug --download-ahk`
from the same developer prompt for the full tutorial. Use `--scenario
reject-generate` for its rejection check. The equivalent GCC and Clang
presets run the same tests.

### stars-host

`stars-host` is the game's host without its windows: it creates universes,
generates turns and writes the dumps from the same command line as
`stars.exe` (`-a game.def`, `-g[n] game.hst`, `-v game.hst`, `-b`, `-t`,
`-s<seed>`, `-dm`/`-dp`/`-df game.mN`). `--ini stars.ini` reads the
`stars.ini` settings a host uses (see below). It builds
with the native compiler on Linux and macOS, with only CMake and Ninja:

```sh
make host               # or: cmake --preset host-release && cmake --build --preset host-release
dist/host-release/bin/stars-host -g1 /path/to/game.hst
```

Turn generation rounds through x87 extended precision, which ARM lacks. On
Apple silicon, `make host` builds the `macos-host-release` preset, an x86_64
binary that Rosetta runs (`dist/macos-host-release/bin/stars-host`); it
generates the same turns as `stars.exe`. A native arm64 build works, but
its turns can differ, and CMake warns about it. `stars-host --version`
prints the version, and `make test-host` runs the unit tests natively.

Where `stars-host` differs from `stars.exe`:

- It reads `stars.ini` only when given `--ini`, and then only the settings
  a host uses: `[Files] Logging` and `[Misc] DefaultPassword`,
  `NewReports` (per-player `-d` files such as `game.p1`), `NoHostNames`
  and `Backups`. Without it they keep their defaults.
- It doesn't wait for turns (`-w`).
- On Linux and macOS it can't tell that another program has a game file
  open, as Windows file sharing does.
- Questions get the cautious answer (No, Cancel) and messages go to stderr.

## GitHub Actions

Every push to `main` builds the optimized MinGW Release executable and updates
the rolling [`latest` prerelease](https://github.com/sirgwain/stars/releases/tag/latest).
Both `stars.exe` and `stars!.hlp` are attached; download them into the same
directory. `stars-host-linux-x64.tar.gz` holds `stars-host` for x86-64 Linux,
built with gcc and linked statically after its unit tests pass. Superseded
main builds remain available as workflow artifacts.

Pushing a version tag (for example `v2.8.0`) builds that tag's source, checks
that it reports exactly that version, and publishes a release with the same
files (tags from before 2.9 have no `stars-host`). Other tags (such as `2.6jrc3`) publish releases named after the
tag. The rolling `latest` tag is excluded. See
[versioning](docs/VERSIONING.md) for how build numbers are derived:

```sh
git tag v2.8.0
git push origin v2.8.0
```

Pull requests build and run the complete tutorial and the unfinished-turn
rejection check under Wine/Xvfb. Diagnostic reports are retained even on failure.
The tutorial uses the same Release preset as the published builds, with the
read-only test observer enabled. The native regression workflow also builds in
Release mode on pull requests and main pushes, comparing
all checkpoints through turn 150 against the checked-in native baseline
(`tests/scaffold/fixtures/regression/native/`); it also runs every scenario
through `stars-host` built natively on Linux (`make regression-host`), and
the unit tests run natively there too (`make test-host`). The original game's
checkpoints are kept alongside it as the record of 2.6j behavior.
All workflows can also be run manually; the release workflow accepts `main`
or a tag. Publishing uses the built-in `GITHUB_TOKEN` with `contents: write`;
the tutorial uses read-only permissions.

## Regression save tooling

The standalone go module in `tests/savecli/` supplies the save
comparison and AI update commands used by checkpoint testing:

```sh
make regression        # every baseline scenario through turn 150
make regression-quick  # smallai4 through turn 10
make regression-export # after a full run, replace the baseline
make regression-host   # the same run through stars-host, without Wine
```

`make regression` builds the fixed-seed release `stars.exe` and
`dist/stars-save`, runs a fresh `dist/scaffold/regression/native`, and
compares it with the checked-in baseline in
`tests/scaffold/fixtures/regression/native/`; any difference fails.
`SCENARIOS="noai smallai4"` and `THROUGH=10` limit a run. See
[the save CLI README](tests/savecli/README.md) and
[regression instructions](tests/scaffold/REGRESSION.md).
