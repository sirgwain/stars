# Fixed-seed AI regression runs

The runner's Windows/Wine routing and tutorial cleanup checks can be run
without a desktop or Wine: `python tests/scaffold/test_runners.py`.

This harness runs the game directly on Windows and under Wine on other
platforms with a fixed seed. It keeps
separate saves at creation and turns 1, 10, 25, 50, 80, 100, and 150, then
compares decrypted records with the standalone CLI in `tests/savecli/`.
`make regression` runs it against the checked-in baseline.

## Baselines

There are two sets of checked-in checkpoints in `fixtures/regression/`:

- `original/`: the original Win16 game, run under DOSBox with a seed-patched
  copy of its `stars.exe`. This is the record of 2.6j behavior. The native
  build at the `2.6jrc3` tag matches it for `noai`, `oneai1`–`oneai4` and
  `smallai4`. The DOSBox bundle and patcher are no longer kept; these
  checkpoints are frozen.
- `native/`: the native build's own checkpoints, the baseline for `main`.
  Behavior-neutral changes must match it. A change that is meant to alter
  game behavior regenerates it in the same commit (see
  [Update the native baseline](#update-the-native-baseline)). Its `run.json`
  lists the scenarios it covers.

GitHub Actions runs the native baseline's scenarios on pull requests and main
pushes using `mingw-release` with the fixed seed. It generates native
checkpoints at turns 0, 1, 10, 25, 50, 80, 100, and 150 and compares them
against `fixtures/regression/native/`. They contain save files, checkpoint manifests and the run
metadata needed by `compare`, with no executables or logs. See the
[fixture README](fixtures/regression/README.md) for provenance and test
registration.

## Scenarios

The active scenarios are `noai`, `oneai1`–`oneai6`, `smallai4`, and `smallai6`.
Their definitions live in `tests/scaffold/fixtures/regression/`; the names and
checkpoint boundaries are defined in `regression.py`.

The human slot uses the existing `fixtures/newgame/tiny/humanoid.r1`. The first
slot must be a race file: the original definition parser accepts `#` AI entries
only in subsequent slots. After creation, the runner updates the human's `.m1`
player record to Maid AI (ID 7), so that slot takes basic actions during forced
generation. Do not use `-w` or `-t` for these runs.

The six AI entries are `#1 4` through `#6 4`: Robotoid, Turindrone, Automitron,
Rototill, Cybertron, and Macinti, all at Expert difficulty. Type and difficulty
zero would choose randomly, so neither is used. Victory conditions are disabled,
player positions use the same setting (1), and all seven game-option flags are
zero. Random events remain enabled to exercise more simulation behavior.

## Fixed seed

The seed on line 2 of a `.def` initializes universe creation through `Randomize`.
A new process normally seeds `Randomize2` from `GetTickCount()` in `WinMain`,
so a definition seed alone cannot reproduce later turns. `stars.exe -s<seed>`
seeds it with a decimal uint32 instead; the algorithms use only its low 14
bits. The runner passes the run's seed on every launch, so any build of
`stars.exe` repeats exactly and no special build is needed.

## Build and stage

Run from the repository root. Python 3, Go, a Windows C compiler, CMake, Ninja, and Wine
are needed (Wine only outside Windows). `make regression` does all of this; by hand:

```sh
cmake --preset mingw-release
cmake --build --preset mingw-release
make save-cli

python3 tests/scaffold/regression.py prepare --seed 12345 \
  --exe dist/mingw-release/bin/stars.exe \
  --work dist/scaffold/regression/native
```

On Windows, Go builds the helper with `cd tests/savecli` followed by
`go build -o ../../dist/stars-save.exe .`. Use native Windows Python to run
the commands above; no Wine installation is required. The default helper
path includes `.exe` on Windows. CMake, Ninja and the chosen compiler
must be on PATH. MSVC runs use an x64 Native Tools command prompt and
`msvc-release`; Clang runs use `mingw-clang-release`. Pass each build's
`stars.exe` or `stars-host.exe` to `prepare --exe`, with a separate work
directory for each compiler and executable. Compare all runs against the
same native baseline; a compiler difference is a test result, not a reason
to change that baseline. PowerShell does not need `make` for these individual commands.

Choose a **fresh output directory** for each run; staging never overwrites
prior results. The runner uses native paths on Windows and Wine's `Z:`
host-filesystem mapping elsewhere. Keep work
paths free of spaces because the Stars! parser cannot quote them. Staging
writes CRLF definitions and absolute Windows paths for the race and output
files, and records the executable's hash, the seed and the fixture hashes in
`run.json`.

## Run and capture checkpoints

```sh
python3 tests/scaffold/regression.py run --work dist/scaffold/regression/native
```

For a quicker check, select one scenario and/or an earlier endpoint
(`make regression-quick`):

```sh
python3 tests/scaffold/regression.py run \
  --work dist/scaffold/regression/native --scenario smallai4 --through 10
```

The timeout is per launch (default 900 seconds), not for the entire suite. A
failed run stops immediately and preserves logs and files. By default, runs with
saves or completed checkpoints are not overwritten. To continue after a timeout
or an abbreviated run, use `--resume`: it verifies the latest checkpoint, moves
the current saves into a `before-resume-*` directory (also preserving logs),
restores the checkpoint, and repeats the next launch from that exact state.
A failure before any saves are written can be retried in place; a failure before
the first checkpoint that leaves partial saves requires a fresh staged run.

```sh
python3 tests/scaffold/regression.py run \
  --work dist/scaffold/regression/native --scenario smallai4 --resume
```

Each scenario launches once per checkpoint, in this exact order, each with
`-s<seed>`:

```text
-a   <absolute game.def>   -> checkpoint 000, year 2400
-g1  <absolute game.hst>   -> checkpoint 001
-g9  <absolute game.hst>   -> checkpoint 010
-g15 <absolute game.hst>   -> checkpoint 025
...                           050, 080, 100, 150
```

The counts are incremental: `-g15` immediately after `-g9` reaches turn 25.
Keep launch boundaries identical across builds because the startup seed resets
on each launch. Do not replace the sequence with a single `-g100` run and expect
the same results. Logging switches can also change execution paths; the runner
uses the same simulation options for every run.

Before saving a checkpoint, the runner checks the host file's actual turn. A
successful process exit without output is a failure. Native command-line turn
generation returns **1 on success** (`FGenerateTurn` sets `vretExitValue`);
creation returns 0. The runner checks those expected codes
as well as the output files. At turn zero it runs
`dist/stars-save save update game.m1 --ai maid` and
`dist/stars-save save update game.hst --ai maid --player 1` before copying the checkpoint.
Use `--cli` on `regression.py run` if the CLI is elsewhere. Under each scenario:

```text
run-000.log                 Wine output for universe creation
run-001.log                 output for the first turn
run-010.log ... run-150.log
checkpoints/000/game.*      all .xy, .hst, .mN, .hN, and .xN files present
checkpoints/000/checkpoint.json
checkpoints/001/... through checkpoints/150/...
```

Checkpoint manifests contain commands, years, and raw SHA-256 hashes. Backups
and diagnostic logs are not treated as game-state files.

## stars-host

`make regression-host` runs the same scenarios and launches through
`stars-host` built with the native compiler, without Wine, and compares
them with the same baseline. On Windows all executables run directly;
elsewhere `regression.py` runs executables without an `.exe` suffix directly
and gives them POSIX paths. On Apple silicon the
target builds `stars-host` for x86_64 (Rosetta), because turn generation
needs x87 extended precision: a native arm64 build diverges in the
Cybertron AI from turn 1 (`IdGetBestScannerDest` truncates
`(long double)iSize * 0.3`, which is just under 360 with x87 and exactly
360 in a 64-bit `long double`).

## Compare

To compare a native run against the checked-in native baseline:

```sh
python3 tests/scaffold/regression.py compare \
  tests/scaffold/fixtures/regression/native dist/scaffold/regression/native \
  --report dist/scaffold/regression/comparison.json
```

Use the same `--scenario` arguments on `run` and `compare` to limit both.
Compare against `fixtures/regression/original` the same way to see where a
run departs from the original game.

The command writes `tests/scaffold/fixtures/regression/regression-comparison.json` and returns nonzero
for any difference, invalid save, missing file, or missing checkpoint. Use
`--scenario smallai4 --through 10` when comparing abbreviated runs. For one file:

```sh
dist/stars-save save compare baseline/game.hst run/game.hst
```

Raw hashes will differ because game IDs and file encryption salts include clock
values. `save compare` decrypts each file first, then excludes only:

- `RTBOF.lidGame` and `GAME.lid` (the clock-derived game identity).
- `RTBOF.lSaltTime` (the encryption salt).

It retains all other record bytes, ordering, version fields, flags, player data,
and simulation state. It handles the `.xy` file's raw packed star coordinates
separately from its encrypted `GAME` record. A mismatch reports the first
differing record with its type, file offsets, sizes, and decrypted bytes.

AI history (`rtAiData` in `.hN`) is decoded using the owning player's AI,
read from the companion `.mN` beside the history. Histories omit PLAYER records.
Cybertron histories decode as `CYBERINFO`; Robotoid, Turindrone, Automitron,
and Rototill histories decode as `AIHIST`. Without a companion turn file, the
history is reported only as a changed payload. Some storage is never read:
`AIHIST` freighter slots at or past `cFreighter` on both sides (stale heap data
moved by `ValidateStarbaseHistory`) and the reserved `CYBERINFO` byte. In fleet
orders (`rtOrderA`/`rtOrderB`), `fUnused` is never read and `fNoAutoTrack` is
cleared when fleets load. Task-union words past those the order's `grTask`
reads are stale too. Xfer reads all five, Patrol two, and LayMines and Give one;
`tlm.cTimeOld` is write-only. Changes in unused storage are listed as `unused ...`. If they are a file's only differences, `save compare`
exits 0 with `MATCH with warnings`. The regression report then records
`"match": true, "warning": true` and counts these files as warnings, not failures.
Its detail keeps only the summary line: native unused storage holds stale heap
and stack bytes that change between runs, so the values would make the
checked-in report shift. Run `save compare` on the files to see them.

This is strict record comparison, not a complete semantic interpretation. A
reported difference needs inspection: padding or environment-specific fields
can differ too. No additional bytes are silently discarded to make tests pass.
The fixed seed makes simulation randomness reproducible, but it does not
establish that the implementation is correct.

### Update the native baseline

A commit that changes game behavior on purpose regenerates the native
baseline from a complete release run of every scenario it covers. `make
regression` followed by `make regression-export` does this; by hand:

```sh
cmake --preset mingw-release
cmake --build --preset mingw-release
python3 tests/scaffold/regression.py prepare --seed 12345 \
  --exe dist/mingw-release/bin/stars.exe --work dist/scaffold/baseline
python3 tests/scaffold/regression.py run --work dist/scaffold/baseline \
  --scenario noai --scenario oneai1 ...
python3 tests/scaffold/regression.py compare \
  tests/scaffold/fixtures/regression/native dist/scaffold/baseline --scenario noai ...
python3 tests/scaffold/regression.py export --work dist/scaffold/baseline \
  --scenario noai --scenario oneai1 ... --replace
```

Run `compare` before `export` and record in the commit message which
scenarios moved and their first differing turn. A scenario that moves
without a reason in the change is a regression, not a baseline update.

### Test turn generation independently of universe creation

If creation is broken, a fresh scenario can start from another run's
verified turn-zero files, such as the baseline:

```sh
python3 tests/scaffold/regression.py run \
  --work dist/scaffold/regression/native --scenario smallai4 \
  --baseline tests/scaffold/fixtures/regression/native
```

This checks the seed and fixture hashes, copies the reference creation files,
then runs the remaining launches. The checkpoint manifest marks turn zero as
a reference input. Comparisons skip that checkpoint rather than count it as
successful creation. Use `--resume` for a subsequent attempt from a completed
checkpoint; `--baseline` is only for a fresh scenario.

### Separate logic differences from inherited state

`crossfeed` copies saves from any directory and generates `--turns` turns with
the executable of a prepared `--work` run. It writes them to
`<work>/<scenario>/xfeed/<from>_<to>/`, and `--expect` compares the result
against another directory of saves. To tell whether a change's difference
comes from its turn-generation logic or from state inherited from earlier
turns, feed the new build the old build's checkpoint:

```sh
python3 tests/scaffold/regression.py crossfeed --work dist/scaffold/regression/native \
  --scenario oneai6 --input tests/scaffold/fixtures/regression/native/oneai6/checkpoints/025 \
  --turns 25 --expect tests/scaffold/fixtures/regression/native/oneai6/checkpoints/050
```

The startup seed resets on every launch, so match a checkpoint's launch span
(`--turns` from the previous checkpoint) when comparing against it. An
existing output is reused only when its input files and executable match;
otherwise remove it to rerun.

`bisect` runs two prepared runs' executables (say, builds of two commits)
from the same input and binary-searches `-gK` for the first divergent turn.
A `-gK` launch reproduces the first K turns of a longer launch from the same
input, so a search needs about log2(turns) launches per executable:

```sh
python3 tests/scaffold/regression.py bisect --reference dist/scaffold/old \
  --native dist/scaffold/regression/native --scenario oneai6 \
  --input tests/scaffold/fixtures/regression/native/oneai6/checkpoints/025 --turns 25
```

Stars! keeps the last generated turn's inputs and AI order logs (`.xN`) in
`backup/`. Compare them between builds. Matching inputs with differing logs
place the divergence in that AI's decisions.

### Trace native RNG draws

Configure a separate build with `-DSTARS_TEST_TRACE=ON` and prepare a native run
from it. The linker wraps `Random` and `PctPlanetCapacity`
(`tests/scaffold/regression_trace.c`). The trace build behaves identically.
With `--trace`, `crossfeed` and `bisect` set `STARS_TRACE` and write
`trace.log` beside the output. Each `Random` line records the turn, player, AI flag,
range, result, caller address, and RNG seeds before the draw. Seeds allow replaying
the stream at any offset. `trace` resolves caller addresses to source lines:

```sh
cmake --preset mingw-debug -B dist/regression-trace -DSTARS_TEST_TRACE=ON
cmake --build dist/regression-trace
python3 tests/scaffold/regression.py prepare --seed 12345 \
  --exe dist/regression-trace/bin/stars.exe --work dist/scaffold/trace
python3 tests/scaffold/regression.py trace <dir>/trace.log \
  --exe dist/regression-trace/bin/stars.exe --turn 32 --player 1
```

Trace both builds and compare, then replay the seeds at nearby offsets to
locate an extra or missing draw.

## Divergences from the original

The native build at `2.6jrc3` matched the original in every scenario except
three, each caused by the original reading uninitialized stack memory:
`oneai5` (the Cybertron mine-laying order's countdown, from the t80
checkpoint) and `oneai6`/`smallai6` (`TargetMacArmada`'s `cshWar`, from t57
and t68). 2.8 fixes those reads, so all nine scenarios are deterministic and
in the native baseline. Comparisons against `fixtures/regression/original/`
are expected to differ from the point each 2.8 behavior change takes effect;
see `CHANGELOG.md`. For bisecting a native change, use `crossfeed`/`bisect`
(above), a `-DSTARS_TEST_TRACE=ON` build with `STARS_TRACE=trace.log`, and
the `.xN` AI logs in each save directory.

## Harness tests

The native-port checks (player-message serialization and both readers,
legacy link bytes, recipient filtering, maximum text length, static-control
color dispatch, and the Win16 battle heap rollover boundary) are unit tests
now: `tests/unit/test_native_ports.c`, run with `make test-unit`. See
[tests/unit/README.md](../unit/README.md).

The save CLI's own tests check salt/ID normalization, retention of
coordinate changes, and rejection of truncated universe files:

```sh
cd tests/savecli && go test ./...
```

Run the tutorial separately from all Wine verification runs.
