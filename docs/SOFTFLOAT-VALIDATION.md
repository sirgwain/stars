# SoftFloat migration test coverage and validation

**Follow-up:** the 14 missed source anchors listed in this initial audit are now exercised and asserted by additional tests. See [gap closure and the future pow contract](SOFTFLOAT-GAP-VALIDATION.md) for the current results. The measurements below preserve the initial audit for comparison.

The MinGW GCC build now has a reproducible numerical reference for a future SoftFloat migration, supported by passing full-game regressions and the tutorial. This is substantially stronger than the previous tests, but it is **not yet evidence that every floating-point boundary is protected**. Reaching a function, taking its floating-point branch, and asserting the resulting value are three different levels of coverage.

This audit uses branch `softfloat`, game-source revision `e7245dd`, on October 9, 2026, with MSYS2 UCRT64 GCC 16.2.0 on Windows x64. MSVC is outside this branch and was not used. The companion [migration assessment](SOFTFLOAT-ASSESSMENT.md) describes the arithmetic and library choices. No game arithmetic, disk layouts, or existing native regression fixtures changed during this audit.

## Evidence and test boundaries

Before adding tests, all 13 CTest executables passed, containing 44 registered Windows test cases. The new `test_floating_point` executable adds ten cases, bringing the total to 14 executables and 54 registered Windows cases. Parameter loops perform additional comparisons within each case.

Both the normal release client and `stars-host.exe` completed all nine native scenarios through turn 150. Each comparison checked 519 files, with zero mismatches and no reported warnings. The GCC coverage-instrumented host also matched all 519 comparisons. These are decrypted semantic comparisons using the existing save comparator; game IDs and encryption salts are excluded by that comparator. They are not raw-file byte equality claims.

The tutorial completed all 80 pages through year 2437, with 346 actions, on both full runs. The early-Generate rejection check also passed. Most tutorial turns load canned files, so this establishes that the UI workflow still operates; it does not replace the 150-turn simulation regression. The second tutorial run saved GCC's coverage counters before the harness terminated the game, because forced termination otherwise loses UI execution evidence.

The Debug and Release numerical results must match the **same** checked-in reference files. The Windows `host-release` preset also builds Windows UI tests; its unit-test pass does not establish Linux or ARM portability. Those platforms remain future migration acceptance targets.

The original `host-release` output directory contained a different generator's cache. This audit used the fresh `dist/fp-host-release` directory, leaving the old build intact. Initial coverage setup also exposed the branch toolchain's assumption that every MinGW utility has a target-prefixed filename. Temporary harness adaptations selected the installed native tools and launched Windows executables directly.

## New numerical reference tests

`tests/unit/test_floating_point.c` calls actual game functions and compares their outputs with nine read-only `tests/unit/golden/floating-*.txt` files. There are **2,518 frozen values and strings**, plus five independently specified AI random-bound expectations and invariants such as cargo conservation. Failed tests leave `floating-*.actual.txt` in the build's test directory and report differing rows' actual and expected values. Missing or extra reference rows fail. There is no automatic reference-update switch. `floating-provenance.json` records the compiler, game-source hashes and reference hashes.

| Test | Inputs and asserted outcomes |
| --- | --- |
| Distance result bits | 14 coordinate pairs, all sign combinations and reversal: zero, exact integer distances, irrational roots, near-square values and large valid coordinates. Exact binary64 result bits. |
| Defense power result bits | Six technology levels, defense counts 0 through 101, population cap boundaries, unowned planets and optional smart-defense output. Exact binary32 results, null-output consistency and probability bounds. |
| Planet and race rounding | Six predefined races with and without Total Terraforming; environmental values 0 through 100 and a mixed-environment grid; AR populations around squares and thresholds at four energy levels. Exact integer scores, resources and mines. |
| Scanner and cloak rounding | All 16 scanner types at four multiplicities; stealing and detection outputs; fleets below, at and above the 500,000 mass switch, with uneven cargo. Exact integer outputs. |
| AI random bound rounding | Real `IdGetBestScannerDest` calls at all five universe sizes. A linker wrapper observes the bound passed to `Random` while still executing the real RNG. Expected bounds are 119, 239, 359, 479 and 599. |
| Large circle projection | Long diagonal routes that take the floating projection branch, offset centers, endpoint crossings and reversed travel. Exact intersection flags and entry/exit distances. Complements the pre-existing horizontal, vertical, diagonal and miss tests. |
| Large cargo balance | Real capacity loss after a ship transfer, cargo quantities 44,999, 45,000 and 45,001, uneven minerals and colonists, and large fuel quantities. Exact allocation and conservation assertions. The fixture uses a hull with real cargo capacity and asserts that cargo actually moves. |
| Route and distance display | Zero, exact and fractional distances around travel thresholds; actual auto-routing warp and ETA text; both distance-label forms; the tutorial built-in scanner case. Exact integers and strings. |
| UI orientation and radar boundaries | Three zoom settings, quadrants and values either side of an angular sector boundary; contained circles at tangency and one pixel either side, in both insertion orders. Exact sprite geometry and circle-batch contents. |
| UI diagonal text and defense sorting | Four translated/aspect-ratio rectangles and defense ties, neighboring values and caps in both sort directions. Text metrics are fixed by a test wrapper; output position and rotation come from the real UI function. This isolates arithmetic from installed fonts. It is not a raster or font-rendering test. |

The frozen outputs were captured from the unchanged GCC implementation, then checked against Debug, Release and instrumented builds. They preserve current behavior, including formatting quirks, rather than asserting that every historical result is mathematically ideal. The AI bounds and conservation checks provide additional expectations independent of the captured files.

The `Random` wrapper uses GNU linker support on Windows and non-Apple native hosts. The wrapper-specific case is omitted on Apple hosts, whose linker does not provide that interface. Other numerical cases remain available; UI cases are Windows-only.

## Negative control

A separate build in `dist/fp-double-control` used `-mlong-double-64`, deliberately removing extended precision without changing the source. The AI test failed at every universe size: 120 instead of 119, 240 instead of 239, **360 instead of 359**, 480 instead of 479, and 600 instead of 599.

The other nine cases passed under that negative control. This is useful evidence about the tests' limits: many calculations are insensitive to this particular precision change for the selected inputs. A large number of matching results must not be mistaken for proof that all rounding boundaries are covered. During migration, additional negative controls should remove an intentional float/double narrowing, alter a power result by one ULP, and change a coordinate truncation rule. Each should fail a targeted test before relying on that test as a safeguard.

## Measured coverage

The inventory remains 49 production functions: 36 core and 13 UI. `tests/scaffold/floating-functions.json` records their names, and `tests/scaffold/floating_coverage.py` reads GCC JSON coverage for those functions. It reports whole-function line and branch-outcome coverage separately from floating-point source anchors.

An anchor is an executable source line containing a floating type, math call, literal, or locally declared floating variable. This lexical measure helps locate missed expressions; it is not an AST operation count, a proof of all dataflow, or complete coverage of multiline expression evaluation. Whole-function branch counts also include unrelated integer and control-flow branches.

Before the new tests, the existing unit suite plus full host regression reached **32/36 core functions**, with **2,368/3,011 executable lines** and **1,850/2,459 branch outcomes** in those functions. It reached **0/13 UI functions** in the floating-point inventory. Existing dialog tests do not necessarily reach these numerical UI functions.

After the new numerical tests, and before adding the tutorial's counters, coverage reached **35/36 core functions**, **2,442/3,011 lines**, and **1,896/2,459 branch outcomes**. The new UI tests reached **4/13 functions**, **121/2,654 lines**, and **59/1,552 branch outcomes**. Floating-point anchors at that stage were **157/167 core** and **21/54 UI**.

Coverage counts below apply only to the inventoried functions, not the entire game.

Final combined coverage reaches **36/36 core functions and 12/13 UI functions**. Core coverage is **2,557/3,011 lines** and **1,951/2,459 branch outcomes**; UI coverage is **1,565/2,654 lines** and **816/1,552 branch outcomes**. The source-anchor measure reaches **158/167 core** and **49/54 UI** lines: **207/221 overall**. These are execution measures, not 207 independently asserted results.

`DrawVCReport` is the only inventoried function never called by the combined workload. Several reached functions still miss their floating-point branches. The following table separates the new direct numerical expectations from integration coverage.

| Function | Layer | Reached before additions and tutorial | Final FP anchors | Final branch outcomes | Numerical protection |
| --- | --- | --- | ---: | ---: | --- |
| `DoCyberPackets` | core | Yes | 8/8 | 87/104 | Existing integration or bug tests |
| `IdGetBestScannerDest` | core | Yes | 2/2 | 44/51 | New independent RNG bounds |
| `FEnumNeedMinerals` | core | Yes | 3/3 | 27/28 | Existing integration or bug tests |
| `FEnumPktAttack` | core | Yes | 8/8 | 24/26 | Existing integration or bug tests |
| `IdTargetFreighter` | core | Yes | 2/3 | 165/180 | Existing integration or bug tests |
| `FSalvageTargetFreighter2` | core | Yes | 1/1 | 37/38 | Existing integration or bug tests |
| `IncreaseAIMinefieldSizes` | core | Yes | 1/1 | 4/4 | Existing integration or bug tests |
| `FAttack` | core | Yes | 0/1 | 132/158 | Existing integration or bug tests |
| `DoBombing` | core | Yes | 10/14 | 92/126 | Existing integration or bug tests |
| `DrawMineSurvey` | ui | No | 0/1 | 131/220 | Existing integration or bug tests |
| `PszFormatString` | core | No | 1/1 | 36/77 | Existing integration or bug tests |
| `PctPlanetDesirability` | core | Yes | 1/1 | 20/20 | New frozen outputs |
| `CMinesOperating` | core | Yes | 1/1 | 5/6 | New frozen outputs |
| `CResourcesAtPlanet` | core | Yes | 1/1 | 16/16 | New frozen outputs |
| `DrawPlanetStats` | ui | No | 4/4 | 37/50 | Existing integration or bug tests |
| `LInnateRaceHabitability` | core | Yes | 11/11 | 83/94 | New frozen outputs |
| `DumpPlanets` | core | Yes | 4/4 | 60/84 | Existing dump golden files |
| `DrawVCReport` | ui | No | 0/1 | 0/62 | Not exercised |
| `DrawScoreReport` | ui | No | 1/1 | 37/44 | Existing integration or bug tests |
| `DrawReportItem` | ui | No | 4/4 | 55/177 | Existing integration or bug tests |
| `ICompReport` | ui | No | 8/8 | 30/214 | New frozen outputs |
| `DrawScanner` | ui | No | 1/2 | 340/541 | Existing integration or bug tests |
| `DrawRadarCircle` | ui | No | 2/2 | 49/54 | New frozen outputs |
| `DrawPathYearTicks` | ui | No | 10/10 | 9/12 | Existing integration or bug tests |
| `DrawShipScanPath` | ui | No | 8/8 | 89/124 | Existing integration or bug tests |
| `GetDxDyOrientation` | ui | No | 3/3 | 8/8 | New frozen outputs |
| `ShowScanSelChange` | ui | No | 0/2 | 18/26 | Existing integration or bug tests |
| `EstFuelUse` | core | Yes | 3/3 | 54/58 | Existing integration or bug tests |
| `IWarpForWaypoint` | core | Yes | 1/1 | 84/92 | Existing integration or bug tests |
| `LFuelUseToWaypoint` | core | Yes | 4/4 | 33/36 | Existing integration or bug tests |
| `FleetTransferCargoBalance` | core | Yes | 3/3 | 71/82 | New frozen outputs |
| `AutoRouteFleet` | core | No | 1/1 | 11/46 | New frozen outputs |
| `PctCloakFromLpfl` | core | Yes | 8/8 | 40/52 | New frozen outputs |
| `CPlanetsInCircle` | core | Yes | 1/1 | 19/20 | Existing integration or bug tests |
| `MoveThings` | core | Yes | 10/10 | 179/204 | Existing integration or bug tests |
| `MoveFleets` | core | Yes | 10/11 | 174/262 | Existing integration or bug tests |
| `FTravelThroughMineFields` | core | Yes | 1/1 | 152/188 | Existing integration or bug tests |
| `DropColonists` | core | Yes | 5/5 | 87/118 | Existing integration or bug tests |
| `UpdateGuesses` | core | Yes | 2/2 | 14/16 | Existing integration or bug tests |
| `CalcPctSurvive` | core | Yes | 8/9 | 11/12 | New frozen outputs |
| `FCalcFleetBombDamage` | core | Yes | 4/4 | 42/52 | Existing integration or bug tests |
| `PszGetDistance` | core | No | 1/1 | 2/2 | New frozen outputs |
| `DGetDistance` | core | Yes | 2/2 | 0/0 | New frozen outputs |
| `GetPlanetScannerRange` | core | Yes | 1/1 | 15/22 | Existing integration or bug tests |
| `GetShdefScannerRange` | core | Yes | 33/33 | 57/66 | New frozen outputs |
| `FCanFleetUseStargates` | core | Yes | 0/1 | 35/59 | Existing integration or bug tests |
| `CchGetETA` | core | No | 3/3 | 11/30 | New frozen outputs |
| `FIntersectCircleLine` | core | Yes | 3/3 | 28/30 | New frozen outputs |
| `DiaganolTextOut` | ui | No | 8/8 | 13/20 | New frozen outputs |

The 14 remaining unexecuted anchors are:

| Location | Function | Missed expression or path |
| --- | --- | --- |
| `aiutil.c:868` | `IdTargetFreighter` | One freighter-selection square-root branch. |
| `battle.c:1870` | `FAttack` | Extended damage-ratio target valuation. |
| `battle.c:3049,3106,3113,3114` | `DoBombing` | Terraform damage and some survival/message branches. |
| `mineui.c:460` | `DrawMineSurvey` | Square root for minefield radius text. |
| `reportui.c:583` | `DrawVCReport` | Division for diagonal victory-graph geometry; entire function unvisited. |
| `scan.c:810` | `DrawScanner` | One of the minefield-radius drawing paths. |
| `scan.c:2987,3027` | `ShowScanSelChange` | Both minefield selection/redraw radius calculations. |
| `turn.c:1422` | `MoveFleets` | Direct distance truncation in one movement path. |
| `util.c:111` | `CalcPctSurvive` | Fallback to full survival when no defense part is available. |
| `util.c:2364` | `FCanFleetUseStargates` | Distance truncation in the gate eligibility path. |

## Remaining work before claiming migration parity

The ten new cases directly protect numerical outputs from 14 core functions and four UI functions. Other functions retain integration or existing bug-test coverage. Even a fully reached function may have an untested floating branch or a branch whose numerical result is not asserted directly.

| Area | Remaining validation requirement |
| --- | --- |
| Fractional `pow` in packet AI | The regression executes the calls, but there is no dedicated frozen corpus for bases 0.75/0.875 with fractional distance exponents. Record boundary inputs and result bits for both packet formulas, including both mass-driver configurations. |
| Battle valuation and bombing | Force the floating target-valuation path and smart/ordinary/terraform bombing branches. Check integer casualties, buildings, terraforming and message parameters immediately, as well as saved turns. |
| Movement, gates and interception | Exercise all quadrants, near-integer distances, low fuel, gate distance/weight boundaries and the explicit distance-truncation paths. Existing pursuit and minefield tests cover useful behavior, but not every threshold. |
| Colonist drops and guesses | Assert survival-adjusted power and displayed/remembered percentages around float and integer boundaries, including old-format loaded state. |
| Cloak and scanner accumulation | Add multiple weighted designs, larger slot combinations, mass/point threshold combinations, and adversarial sums near a fourth-root or percentage boundary. Current cases cover representative switches, not all combinations. |
| UI drawing beyond the four direct cases | Add deterministic draw-command or geometry assertions for mine surveys, planet stats, report items, graphs, scanner paths/year ticks and selection redraws. Tutorial screenshots and actions are useful integration evidence, but do not establish each coordinate or percentage. |
| UI environment | Compare images only under controlled font, DPI, zoom, theme and window geometry. Across systems, prefer numeric coordinates, draw-command arguments and text assertions over raw pixels. |
| Original-file compatibility | Keep the existing old-version load tests and frozen original files; additionally run selected original 2.83 checkpoints through the migrated host and compare against the pre-migration native host using identical inputs. The native 150-turn scenarios alone do not exhaust historical loaded states. |

The highest-priority migration risks remain constant import, intermediate narrowing, integer conversion, the power-function implementation and the square-root return boundary. These are numerical contracts, not just library-integration details.

## Before and after acceptance procedure

1. Preserve the current native regression fixtures and the new numerical references. Save compiler version, flags, source revision, fixture hashes, executable hashes and the chosen SoftFloat version/configuration. Keep the native reference executable or an isolated reference checkout.
2. Run the numerical corpus and full unit suite against hardware and software backends in Debug and Release. Compare floating results by their defined format bits and final integer/string results exactly. Do not introduce epsilon tolerances for gameplay thresholds.
3. Run all nine regression scenarios through turn 150 for client and host, using the same seed and save comparator. Compare against the existing native baseline and investigate the first unexpected difference. Use the existing `generate` and `bisect` harness commands to locate its first turn; preserve the input checkpoint and failing arithmetic case.
4. Run the full tutorial and the early-Generate rejection test. Require an explicit full-pass result, not just an exit code or an instruction-dispatch inventory. Compare before/after orders and numerical UI outputs, with controlled screenshots as supplementary evidence.
5. Run fresh GCC coverage workloads and compare function/anchor coverage, especially the previously missed paths. Never combine stale `.gcda` files from a different binary or mistake repeated execution for an additional assertion.
6. Run the same semantic corpus and saves on intended ARM/Linux hosts. A same-machine GCC pass cannot establish portability. Measure end-to-end turn generation under equivalent optimized builds; instrumented runtime is not a performance baseline.

Acceptance means unchanged expected gameplay and UI results, no unexplained regression differences, all deliberately added negative controls detected, and any remaining coverage exclusions explicitly reviewed. New intentional behavior changes must follow the repository's separate-commit, changelog and baseline-update rules. Do not export a baseline merely to make the software arithmetic pass.

## Running the checks

The normal presets discover `test_floating_point.c` automatically. For this Windows installation, put `C:\msys64\ucrt64\bin` and the installed Ninja directory on `PATH`, then run the existing MinGW configure/build and CTest commands. `ctest --test-dir dist/mingw-debug -R test_floating_point --output-on-failure` isolates the new tests. The complete suite must also pass.

For coverage, configure a fresh build with `CMAKE_C_FLAGS=--coverage` and `CMAKE_EXE_LINKER_FLAGS=--coverage`, run the intended workload, then collect:

```text
python tests/scaffold/floating_coverage.py --build dist/fp-coverage --label "describe the workloads actually completed" --output dist/softfloat-audit/coverage.json
```

Use `--gcov` to select the gcov executable matching the compiler, and `--layer core` for a build without the Windows UI objects. A missing build object, mismatched profile or invalid gcov result must be resolved before trusting the report. The tutorial requires a test-only coverage flush before forced termination; ordinary exit-time collection is insufficient for this harness.

Windows-native runner adaptations used for this audit were restored, as requested. The retained `dist/softfloat-audit/windows-harness.patch` applies cleanly to this working tree; its tutorial coverage hook requires a `--coverage` build. The source tree keeps the numerical tests, fixtures, collector and documentation. The branch's unadapted runners still assume Wine. Reapply the retained adapter for an instrumented Windows run, or use the existing Wine workflow.

Local evidence is retained in `dist/softfloat-audit/results.json`, the three `*-comparison.json` files, `coverage-before.json`, `coverage-after-tests.json`, `coverage-final.json`, build/test logs and the timestamped tutorial directories. The game-source hash check confirmed that all 19 inventoried production files remain identical to the audited revision. Only tests, test-target build settings, fixture line-ending policy and reports remain changed.

No SoftFloat implementation has been installed or migrated by this work. These results establish the before-migration reference and expose the work needed to substantiate the after-migration result.
