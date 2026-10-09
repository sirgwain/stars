# Stars floating point and SoftFloat migration assessment

Implementation and measured migration results are now in
[SOFTFLOAT-IMPLEMENTATION.md](SOFTFLOAT-IMPLEMENTATION.md). The assessment below
records the pre-migration analysis.

Berkeley SoftFloat is a good fit for removing Stars' dependence on hardware x87 extended precision. The conversion is a **moderate, contained engineering task**, with compatibility testing as the largest risk. It is not a typedef replacement: C arithmetic expressions must become explicit operations, and every existing rounding boundary must survive. SoftFloat also leaves one important gap: the game's six calls to `pow`.

For planning, allow roughly **two to four engineering weeks** for a portable core implementation and compatibility validation, assuming the existing regression infrastructure is available. This is an estimate from the source inventory, not a measured implementation schedule. Reproducing a troublesome historical math-library result could extend it. A small pilot should resolve the biggest uncertainty before committing to the full conversion.

The initial assessment covered checkout `956ac6e` on `windows-compilers` on October 8, 2026. The October 9 extension audits the MinGW-only `softfloat` branch at `e7245dd`, with measured core/UI coverage and new numerical reference tests in [SoftFloat migration test coverage and validation](SOFTFLOAT-VALIDATION.md). MSVC references below describe the migration motivation and a possible future target; this branch does not currently support MSVC. The target is the native game baseline, preserving intentional fixes since 2.6j. The frozen original checkpoints remain historical evidence, not a reason to undo those fixes.

## What needs preserving

The compatibility requirement is more specific than having an 80-bit type. The code combines:

- Extended-precision intermediate arithmetic expressed through `long double` casts.
- Values deliberately stored or rounded to 64-bit `double` and 32-bit `float`.
- Conversion to integers by truncation, often after adding `0.5`, `0.999`, `0.9999`, or `0.99999`.
- Integer narrowing and wrapping before and after floating-point operations.
- `double` math-library arguments and results.

Thus the original's x87 arithmetic does not imply that every variable or saved number was an 80-bit `long double`. The reconstruction preserves a mixture of precisions. Keeping everything extended until the final result would change that mixture.

Microsoft documents that its `long double` has the same representation as `double`. CMake checks for a 64-bit significand on supported non-Windows builds and warns about divergent turns; the earlier `windows-compilers` branch extended that warning to MSVC. Compiler settings that constrain optimization do not provide a missing 80-bit type. [Microsoft long double documentation](https://learn.microsoft.com/en-us/cpp/c-language/type-long-double?view=msvc-170).

There is a concrete example at `ai4.c:757`, in `IdGetBestScannerDest`:

```c
dAdjust = Random((int32_t)((long double)iSize * 0.3));
```

For `iSize = 1200`, the binary64 constant `0.3` is slightly below exact three tenths. Multiplication in extended precision preserves a result slightly below 360, which truncates to **359**. Binary64 multiplication rounds to **360**, which truncates to **360**. The argument to `Random` changes, potentially changing AI decisions and later game state.

A standalone C11 MinGW probe using volatile inputs reproduced those two integer results on this machine. It reported `LDBL_MANT_DIG=64`, `LDBL_MAX_EXP=16384`, and `sizeof(long double)=16`. The 16-byte object size includes storage padding; it does not mean binary128 arithmetic. The first-turn Cybertron divergence is also recorded in `tests/scaffold/REGRESSION.md:170–173`.

This example makes **constant provenance** essential. Import the existing binary64 value of `0.3` into the extended format. Do not replace it with a freshly rounded extended-precision decimal constant, `0.3L`, or the rational expression `3/10`.

## Inventory scope and size

The production C inventory contains **49 functions in 19 files**: 36 functions in 14 `STARS_CORE_SOURCES` files and 13 functions in five `STARS_UI_SOURCES` files. These are functions with floating-point declarations, expressions, math calls, or direct calls to the floating-point interfaces `DGetDistance` and `CalcPctSurvive`. Transitive integer-only callers are not counted. The source contains **178 explicit `(long double)` casts**; this is a source-token count, not a runtime operation count.

The inventory checks floating types, literals, math functions, interfaces, and uses of the declared floating variables. It is a static source assessment, not a compiler AST census or runtime profile. Header searches identify only two public floating-point interfaces, both in `util.h`: `CalcPctSurvive`, which writes floats through pointers, and `DGetDistance`, which returns double. No floating-point fields were found in `structs.h` or floating globals in `globals.h`. No new disk representation is needed.

| Math function | Core call sites | UI call sites | Total |
| --- | ---: | ---: | ---: |
| `sqrt` | 18 | 9 | 27 |
| `pow` | 6 | 0 | 6 |
| `atan2` | 0 | 3 | 3 |
| `sin` | 0 | 2 | 2 |
| `cos` | 0 | 2 | 2 |
| `hypot` | 0 | 2 | 2 |
| `floor` | 0 | 6 | 6 |

Counts are syntactic calls, including both calls in a nested square root. There are no production `ceil` calls. Ordinary addition, subtraction, multiplication, division, comparisons, negation, assignments, and numeric conversions occur throughout the functions below. No production floating remainder, logarithm, exponential, floating text parser, or explicit floating-environment control was found. Decimal version numbers in comments and strings are excluded.

### Core function inventory

Locations identify relevant expressions or declarations at the audited revision. A file being in the core build does not mean every function in it generates turns: reports and ETA formatting also live there.

| File and lines | Function | Floating-point work and migration concern |
| --- | --- | --- |
| `ai4.c:503–644` | `DoCyberPackets` | Distance, attack modifier division, two fractional-exponent powers, packet mineral requirements, comparisons and truncation. Preserve double stores and unsigned integer inputs. |
| `ai4.c:757–758` | `IdGetBestScannerDest` | Extended multiplication by binary64 `0.3` and `0.15`, then integer truncation for random adjustments. Highest-value first test. |
| `ai4.c:1121–1158` | `FEnumNeedMinerals` | Double distance, extended products and comparison against squared range. |
| `ai4.c:1167–1214` | `FEnumPktAttack` | Distance, range comparison, two powers, packet decay and damage modifier division. |
| `aiutil.c:868,894,928` | `IdTargetFreighter` | Three square roots, truncated to integers for distance scoring. |
| `aiutil.c:1086` | `FSalvageTargetFreighter2` | Square root and integer distance scoring. |
| `aiutil.c:2826` | `IncreaseAIMinefieldSizes` | Square root plus extended `10.5`, then truncation. |
| `battle.c:1870` | `FAttack` | Extended multiply/divide for damage-related target valuation, truncated to integer. |
| `battle.c:2952–3114` | `DoBombing` | Float defense results, damage products, double half-effect calculation, comparisons and percentage messages. Preserve float-to-extended promotions. |
| `msg.c:389` | `PszFormatString` | Extended comparison of an already integer-divided message parameter against ten. Small, presentation-related site. |
| `planet.c:415` | `PctPlanetDesirability` | Extended division, explicit double conversion before square root, addition of `0.9`, truncation. |
| `planet.c:502` | `CMinesOperating` | Population square root and integer narrowing. |
| `planet.c:635` | `CResourcesAtPlanet` | Extended population/energy/efficiency formula, double square root, extended scaling and `0.999`. |
| `race.c:305–467` | `LInnateRaceHabitability` | Three double accumulators, extended additions/products/divisions, repeated rounding back to double, final rounded integer score. |
| `report.c:327–416` | `DumpPlanets` | Defense survival float, complement and manual decimal percentage extraction. |
| `ship.c:383–449` | `EstFuelUse` | Distance, approximate upward distance rounding, extended mass/fuel product and division. |
| `ship.c:620` | `IWarpForWaypoint` | Direct distance-result truncation and narrowing; easily missed by searching only for floating type names. |
| `ship.c:663–677` | `LFuelUseToWaypoint` | Distance offsets, double stores, sequential division by warp, years by truncation after offset. Do not reassociate divisions. |
| `ship.c:847,859,867` | `FleetTransferCargoBalance` | Three extended multiply/divide paths used for large fuel/cargo values. Keep thresholds and integer fallback branches. |
| `ship2.c:342` | `AutoRouteFleet` | Distance plus extended `0.999`, truncated for routing. |
| `ship2.c:642–710` | `PctCloakFromLpfl` | Double weighted totals, extended multiply/add and final division/truncation. |
| `thing.c:72` | `CPlanetsInCircle` | Square root plus extended `0.9999` for radius bound. |
| `turn.c:850–1183` | `MoveThings` | Double distances, float defense result, packet damage, distance comparison, double movement ratio and signed coordinate offsets. |
| `turn.c:1256–1616` | `MoveFleets` | Distance truncation, epsilon comparisons, travel/fuel decisions, double ratio, extended coordinate interpolation. Very sensitive at waypoint and integer boundaries. |
| `turn.c:1910` | `FTravelThroughMineFields` | Integer squared distance to double square root, extended `0.5`, truncation. |
| `turn2.c:773–841` | `DropColonists` | Float survival adjustment, extended attack scaling, comparisons and percentage messages. Preserve integer division before multiplication. |
| `turn2.c:1257–1282` | `UpdateGuesses` | Float defense survival to rounded integer estimate. |
| `util.c:87–118` | `CalcPctSurvive` | Two powers with integer-valued exponents; bases computed in extended precision, rounded to double; results rounded to float. |
| `util.c:406–476` | `FCalcFleetBombDamage` | Double smart-bomb accumulator, extended effectiveness formula and repeated multiplication with double stores, final integer damage. |
| `util.c:1232` | `PszGetDistance` | Double distance scaled in extended precision by 100, plus `0.5`, for display. |
| `util.c:1243–1251` | `DGetDistance` | Shared double square root of integer squared distance. Central numerical boundary. |
| `util.c:1613` | `GetPlanetScannerRange` | Population-based square root and integer narrowing. |
| `util.c:1730–1844` | `GetShdefScannerRange` | Fourth powers, weighted sums in double storage with extended intermediates, comparisons and two nested square roots per final range. |
| `util.c:2364` | `FCanFleetUseStargates` | Distance-result truncation/narrowing for gate range. |
| `util.c:2526–2587` | `CchGetETA` | Double distance stored and truncated before integer ETA arithmetic. |
| `utilgen.c:585,606,611` | `FIntersectCircleLine` | Extended geometric projection for large values, explicit double conversion followed by implicit integer conversion, two square roots. `dxI` is actually `int32_t`; its name is misleading here. |

### UI function inventory

| File and lines | Function | Floating-point work |
| --- | --- | --- |
| `mineui.c:460` | `DrawMineSurvey` | Square root for displayed minefield radius. |
| `planetui.c:360–518` | `DrawPlanetStats` | Defense float, complement and decimal percentage extraction. |
| `reportui.c:583` | `DrawVCReport` | Extended division by `1.4142` for drawing geometry. |
| `reportui.c:725` | `DrawScoreReport` | Same diagonal geometry calculation. |
| `reportui.c:1142–1231` | `DrawReportItem` | Defense float and manual percentage formatting. |
| `reportui.c:1704–1833` | `ICompReport` | Two defense float results, complements and ordering comparisons. |
| `scan.c:773,810` | `DrawScanner` | Two square roots for minefield radii. |
| `scan.c:1467,1472` | `DrawRadarCircle` | Square roots promoted for circle coverage comparisons. |
| `scan.c:1528–1553` | `DrawPathYearTicks` | Double lengths via two `hypot` calls, normalization, interpolation, comparisons and six `floor` calls. |
| `scan.c:1597–1688` | `DrawShipScanPath` | Slopes, two square roots, `atan2`, `sin`, `cos`, angle increments and pixel rounding. |
| `scan.c:2858–2865` | `GetDxDyOrientation` | `atan2`, extended angle scaling and double store, then direction-index conversion. |
| `scan.c:2987,3027` | `ShowScanSelChange` | Square roots plus one, converted to redraw bounds. |
| `utilgenui.c:207–277` | `DiaganolTextOut` | `atan2`, `sin`, `cos`, angle conversion and extended text geometry. Preserve the existing function spelling. |

UI geometry can remain native floating point for a host-determinism migration. Defense displays and sorting should continue to consume the same core result as combat, even if their final presentation arithmetic stays native. A project requiring identical pixels and report ordering across compilers would need to extend the scope to these UI functions.

The test-only arithmetic is separate: `tests/unit/test_ship.c:127–133`, `CYearsAtWarp`, calls `DGetDistance`, divides in double and calls `ceil`. `tests/unit/acutest.h` also uses double for elapsed-time reporting. Neither needs wholesale SoftFloat conversion; test oracles must nevertheless be reviewed when numerical interfaces change. Scripts and external runtimes used by the harness are outside this production C inventory.

## What SoftFloat provides

Release 3e provides `extFloat80_t`, basic arithmetic, square root, comparisons, and conversions between integer, binary32, binary64 and extended formats. Extended precision is selected with `extF80_roundingPrecision = 80`; nearest-even is the normal arithmetic rounding choice. Integer casts need the toward-zero conversion variants. Pointer-based `extF80M_*` interfaces are available across ports; by-value variants depend on the configuration. [SoftFloat interface](https://www.jhauser.us/arithmetic/SoftFloat-3/doc/SoftFloat.html).

| Stars requirement | Candidate API family |
| --- | --- |
| Integer inputs | `i32_to_extF80M`, `ui32_to_extF80M` |
| Float/double promotion | `f32_to_extF80M`, `f64_to_extF80M` |
| Extended arithmetic | `extF80M_add`, `extF80M_sub`, `extF80M_mul`, `extF80M_div` |
| Intentional storage rounding | `extF80M_to_f32`, `extF80M_to_f64` |
| Integer truncation | `extF80M_to_i32_r_minMag` |
| Comparisons | `extF80M_eq`, `extF80M_lt`, `extF80M_le` |
| Square root | `f64_sqrt` or extended square root with an explicit output conversion, according to the verified evaluation boundary |

These names are exposed in the [upstream public header](https://raw.githubusercontent.com/ucb-bar/berkeley-softfloat-3/master/source/include/softfloat.h). Keep the dependency behind a small Stars numerical module so game code does not manipulate SoftFloat state directly.

The key missing operations are `pow` and the UI transcendental functions. SoftFloat implements primitive IEEE arithmetic, not a complete C math library. Its published release is 3e, with square-root fixes made in 3d; use a pinned version containing those fixes. [Berkeley SoftFloat overview](https://www.jhauser.us/arithmetic/SoftFloat.html).

### The power function decision

Four calls in `DoCyberPackets` and `FEnumPktAttack` raise `0.875` or `0.75` to a distance divided by warp-squared. Those exponents are generally fractional. Repeated integer multiplication cannot replace them.

Two calls in `CalcPctSurvive` have integer-valued defense-count exponents. Repeated multiplication or exponentiation by squaring is mathematically suitable, but changes rounding order relative to the existing `pow`. It must not be assumed behavior-neutral.

Three practical choices exist:

| Choice | Effort and consequence |
| --- | --- |
| Keep native `pow` initially | Fastest pilot. Tests extended arithmetic independently, but platform math libraries remain a source of differences. Not sufficient to promise identical turns everywhere. |
| Pin a portable `pow` implementation and its evaluation rules | Preferred route if cross-platform identity is the objective. Must match the intended double boundaries and be compared with the current baseline. Merely compiling the same math source on every platform is not enough if contraction, excess precision or native operations still vary. |
| Reproduce the relevant original CRT algorithm | Potentially highest compatibility, but requires substantially more historical analysis. Reserve for demonstrated differences that the first two approaches cannot resolve. |

For defenses, a verified table may be an alternative if all allowed ability/count combinations can be enumerated. Prove the domain, including loaded games, before relying on it. The fractional AI powers still need an algorithm. SoftFloat itself supplies neither solution.

### Square root requires a deliberate boundary

All current source calls use `sqrt`, not `sqrtl`, with double arguments. For the current C-level contract, `f64_sqrt` is the natural first implementation. It is still necessary to verify whether a reference result reflects a double-rounded function return or an x87 extended result retained until the caller's next operation. Extended square root followed by conversion and directly rounded binary64 square root are not interchangeable in every boundary case.

Examples worth testing first are `DGetDistance`, `CResourcesAtPlanet`, and the nested roots in `GetShdefScannerRange`. If original instruction-level fidelity is needed, inspect the relevant original call and store sequences. This assessment did not re-disassemble the original executable; it does not establish the original CRT's result bits or control word at every site.

## Integration approach

Use a small core numerical module, with Windows-independent C declarations, to own extended values, constant import, arithmetic, narrowing, comparisons, and math-library boundaries. Preserve existing debug-symbol names in callers. Prefer explicit helper calls over elaborate macros that hide order or evaluate arguments twice.

Start with a hybrid representation: existing native `float` and `double` locals can remain as **storage and interface carriers**, with `memcpy` of their bits into SoftFloat types. Arithmetic requiring extended precision then runs through the module and converts explicitly back at existing stores. Require the expected binary32/binary64 representations. Do not numerically cast native double to a SoftFloat struct or copy an entire padded native `long double` object into an extended struct.

A later fully software core could use software binary32/binary64 values internally as well. This gives stronger control over all evaluations but expands interface changes, especially the `float *` outputs of `CalcPctSurvive`. Neither approach requires changing `structs.h`, disk records, or save-file versions.

The implementation rules that matter most are:

1. Translate expression trees in their existing order. Preserve intermediate `(double)` and `(float)` casts and stores. Do not reassociate divisions, collapse multiplications into powers, or fuse multiply-add operations.
2. Import unsuffixed floating literals with their binary64 bit patterns. Integer literals converted to extended values should remain exact integer conversions. Preserve unary signs and operand signedness.
3. Retain integer arithmetic performed before promotion, including `int16_t`, `uint16_t`, `uint32_t`, `LOWORD`, and existing wrapping behavior. SoftFloat does not emulate those integer semantics.
4. Use truncation for C integer conversions, keeping any existing fractional offset as a separate preceding operation. `+0.5` followed by truncation is not a general nearest-integer operation, especially for negative coordinates.
5. Fix arithmetic mode and extended precision centrally. Check invalid conversions and exceptional inputs against the chosen compatibility contract; do not accidentally turn an invalid value into a different integer sentinel. If hosting becomes concurrent, isolate numerical mode and exception state per thread or context.

For the library build, add a separate CMake target. The supplied build examples use GNU make and include MinGW configurations, not a ready-made MSVC CMake target. Adapt `platform.h`, integer primitives, endian settings and inline conventions; start with portable integer implementations instead of copying GCC-only optimizations. Select a consistent x86-compatible specialization across hosts. The source documentation describes both `8086` and `8086-SSE`, platform configuration, and thread-local state. Retain upstream license notices in source and distributions. [SoftFloat source documentation](https://www.jhauser.us/arithmetic/SoftFloat-3/doc/SoftFloat-source.html).

Once software arithmetic is verified, replace the blanket extended-`long double` warning with checks appropriate to the selected backend. Keep a hardware reference backend available for differential testing, even if the software backend eventually becomes the normal path on every architecture.

## Effort and migration sequence

These estimates assume one engineer familiar with the repository. They include review and numerical tests, but not an open-ended reconstruction of the original CRT.

| Work | Estimated engineering time | Exit condition |
| --- | --- | --- |
| Pin/build SoftFloat, establish wrappers and constants | 1–2 days | MSVC and MinGW build; conversion and operation tests pass. |
| Pilot the AI rounding example, distance and defenses | 1–3 days | Known divergence removed; decision on `pow` and square-root boundaries supported by bit comparisons. |
| Convert remaining core expressions and audit stores | 3–5 days | All 36 inventoried core functions covered or explicitly classified as presentation-only. |
| Resolve math differences and validate across platforms | 4–10 days | Full suites and boundary corpus agree with the intended baseline; timing measured. |

Total: approximately **9–20 engineering days**. Complete UI arithmetic migration would be additional work and would require a solution for trigonometry. Keeping native UI drawing is a reasonable scope choice when the goal is deterministic hosting and correct gameplay under MSVC and ARM.

Software arithmetic will cost more per operation than hardware x87/SSE. The small number of source sites makes acceptable overall performance plausible, but source counts do not reveal execution frequency. Benchmark full turn generation, especially AI, habitability loops, scanning and bombing. No end-to-end slowdown estimate is justified before that measurement.

## Validation needed before adoption

The pilot should compare result bits and integer outputs, not only decimal printouts. Include the `1200 * 0.3` example, nearby integers, negative movement coordinates, exact-square and near-square distances, large cargo ratios, float defense boundaries, repeated double accumulation, and both families of `pow` calls. Use a hardware extended-precision reference under controlled compiler settings, then compare MSVC software results and ARM software results.

Existing `test_utilgen.c` geometry cases and `test_ship.c` fuel/warp tests provide useful coverage, but they are not an exhaustive numerical boundary suite. Upstream TestFloat can help validate the chosen library build; it cannot establish Stars gameplay parity. [Berkeley SoftFloat testing guidance](https://www.jhauser.us/arithmetic/SoftFloat.html).

For each implementation batch, follow the repository's build, full native regression, unit, tutorial, host regression and host-unit requirements. Include Debug and Release for MSVC and MinGW, plus the intended native ARM host. Preserve the native baseline for a behavior-neutral migration. Investigate the first unexpected difference; do not regenerate fixtures just to accept a new arithmetic implementation. An intentional gameplay correction needs its own commit, changelog entry and baseline update. Leave the original fixtures unchanged.

The initial report changed no game source or baseline and used a static inventory, source/API review and isolated numerical probe. The October 9 [validation audit](SOFTFLOAT-VALIDATION.md) adds numerical tests and measured core/UI coverage, and runs the unit, regression and tutorial suites with MinGW GCC. SoftFloat itself has not been integrated or benchmarked.

The recommended next step is a bounded pilot covering `IdGetBestScannerDest`, `DGetDistance`, and `CalcPctSurvive`. Together they exercise the known precision failure, the most widely shared math helper, and the missing power-function dependency. A successful pilot would make the remaining migration largely a careful expression conversion and regression exercise.

The subsequent [gap-closure report](SOFTFLOAT-GAP-VALIDATION.md) adds tests for the 14 previously missed floating-point source lines and a replaceable `pow` backend tested against both native and MPFR references.
