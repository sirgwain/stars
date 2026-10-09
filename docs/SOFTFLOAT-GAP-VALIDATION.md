# Floating-point gap closure and the future pow contract

This is the pre-migration test report. See
[the implementation results](SOFTFLOAT-IMPLEMENTATION.md) for the software backend
and its explicitly documented correction to the GCC pow result.

Follow-up to [the initial coverage audit](SOFTFLOAT-VALIDATION.md), October 9, 2026, on branch `softfloat`, game-source revision `e7245dd`. This work adds tests and reference data; it does not implement SoftFloat or change production arithmetic.

## What is now protected

The six additional game/UI cases exercise all 14 previously missed floating-point source anchors. The combined unit/regression/tutorial workload now reaches **49/49 functions and 221/221 identified floating-point executable source lines**. This closes the specific execution gaps in the first report. It does **not** mean 100% branch coverage, exhaustive input coverage, or proof of original Win16 parity.

There are now **15 CTest executables and 64 registered Windows cases**: the original 44 cases, 16 numerical game/UI cases, and four `pow` cases. The game/UI references contain **3,180 frozen values and strings** in 14 files, up from 2,518. The separate power corpus contains **2,117 input pairs**, each with both a native result and a mathematically rounded reference. Existing numerical fixtures and native/original regression baselines were not replaced.

| New case | Previously missed paths | Assertions |
| --- | --- | --- |
| Freighter destination thresholds | `IdTargetFreighter`, `aiutil.c:868` | Real freighter and unowned candidate, distances around 25- and 50-light-year boundaries with a nonzero second coordinate; destination ID, inserted order and warp. |
| Unavailable defense fallback | `CalcPctSurvive`, `util.c:111` | AR race with stored defenses but no available defense part; both survival outputs are exactly one and the current player is restored. |
| Gate distance and movement | `FCanFleetUseStargates`, `util.c:2364`; `MoveFleets`, `turn.c:1422` | Real source/destination gates, distances 249/250/251 and 499/500/501 with a one-light-year offset, ship masses 99/100/101; eligibility, actual arrival, remaining ships and damage. |
| Bombing boundaries | `DoBombing`, `battle.c:3049,3106,3113,3114` | Retro, LBU, ordinary and smart bombs at 0/50/100 defenses; exact population, installations and environment. GNU wrappers record the real message IDs and casualty/installation/protection parameters while still sending the messages. Defended LBU cases explicitly assert no colonist deaths. |
| Large beam damage carryover | `FAttack`, `battle.c:1870` | An attack exceeding 65,535 damage, beam mitigation and two targets; the first target is destroyed and damage carries to the second. Exact target order, hit count, remaining ships, damage and losses. This path is damage carryover, more specifically than the initial report's “target valuation” description. |
| UI minefield and victory geometry | `DrawVCReport`, `reportui.c:583`; `DrawMineSurvey`, `mineui.c:460`; `DrawScanner`, `scan.c:810`; `ShowScanSelChange`, `scan.c:2987,3027` | Actual UI methods, fixed text metrics, mine counts 99/100/101 and 9,999/10,000/10,001. Frozen text, text positions, ellipse bounds and selection-redraw rectangles. Hidden owned windows supply drawing contexts; tests do not depend on screenshots or installed font metrics. |

New fixtures were captured from unchanged Debug game code and matched the instrumented and Release builds. Independent invariants supplement the captures, but a captured value remains a compatibility reference, not independent proof of mathematical correctness. The AI RNG and bombing-message wrapper cases are omitted on Apple linkers; UI cases are Windows-only.

## Testing a future pow implementation

**Implementation constraint:** MPFR is permitted only as a testing oracle or
offline reference generator. The shipped game, host and future `pow`
implementation must not depend on, link to, bundle, or incorporate MPFR code.
GMP is likewise confined to the offline oracle tooling. Implement `pow` using
SoftFloat operations and independently written or suitably licensed code;
MPFR must not serve as a production fallback for difficult rounding cases.
Correct rounding remains a numerical target, not a requirement to use MPFR.

The six production `pow` call sites have a binary64 argument/result interface, even where surrounding calculations use extended precision. The adapter intentionally preserves that boundary:

```c
double DStarsTestPow(double x, double y);
```

Configure a separate build with:

```text
-DSTARS_TEST_POW_SOURCE=/absolute/path/to/adapter.c
-DSTARS_TEST_POW_LIBRARIES=<required libraries>
```

The default backend calls the current native `pow` through a volatile function pointer, preventing constant folding from bypassing the call. A supplied adapter is used by `test_pow`. With GNU ld it also replaces game `pow` calls in `test_floating_point` through `pow_game_adapter.c` and `--wrap=pow`. Production executables remain unaffected. An adapter that forwards to native `pow` must use `__real_pow` in the game test to avoid recursion; only the standalone power test defines `STARS_TEST_POW_CUSTOM`.

For SoftFloat, convert the adapter's inputs/results by their binary64 bit representations. This test interface does not authorize replacing the game's intentional double/float narrowing with extended precision. Full-client and host regression builds will need the actual production backend integration when migration begins.

The four power tests establish separate contracts:

1. **Native compatibility:** every finite result must match the captured GCC bits exactly. There is no epsilon tolerance.
2. **Mathematical accuracy:** compare against an MPFR reference. `STARS_TEST_POW_MAX_ULP=1` is the default to accommodate the measured native result; set it to `0` to demand correct rounding. Overflow classification is checked exactly.
3. **Packet integer boundaries:** both multiplication and division formulas, ten mineral quantities, both packet bases, and fractional exponents. Compare final integer truncation against the frozen native power result, skipping division cases outside the signed 32-bit range. These are formula-level tests; they supplement, rather than replace, the regression's real AI execution.
4. **Special values:** exact elementary results, integer powers of negative bases, invalid fractional powers, NaNs, infinities and signed zero. NaNs are checked by classification, not a platform-specific payload. `errno` and exception flags are not specified by this test contract.

The 2,117-row corpus includes all five defense abilities, both defense divisors, counts 0–101, packet bases 0.75/0.875, distance/range ratios, neighboring representable exponents, integer exponents and their neighbors, values near unity, and underflow/overflow boundaries. It is substantial targeted coverage, not an exhaustive binary64 power-function test.

### Independent reference and an actual rounding disagreement

`tests/scaffold/pow_reference.c` is an **offline** capture program using installed MPFR 4.2.2 and GMP. It starts at 128-bit precision, evaluates directed lower and upper bounds, and increases precision until both bounds round to the same binary64 answer. It fails if that cannot be established by 4,096 bits. CTest never runs this generator or rewrites its output. Ordinary tests have no MPFR runtime dependency.

The corpus has exactly one native/reference disagreement:

```text
x             = 0x1.8000000000000p-1       (0.75)
y             = 0x1.9ffffffffffffp+3       (the predecessor of 13)
native result = 0x1.853d300000004p-6
rounded result= 0x1.853d300000003p-6
```

The native result is one ULP high. Therefore a correctly rounded replacement cannot also pass the exact-native test at this row. Keep both expectations visible and investigate any gameplay consequence before deciding whether to preserve the native result or accept an intentional change. The corpus is a GCC compatibility reference, not a claim about the original Win16 math library's exact result.

To reproduce the offline capture into a review file, not the checked-in fixture:

```text
gcc tests/scaffold/pow_reference.c -o dist/pow_reference.exe -lmpfr -lgmp
dist/pow_reference.exe > dist/pow-reference-review.txt
```

Input/output hex words and source/fixture hashes are recorded in `tests/unit/golden/floating-provenance.json`.

## Controls and validation

The deliberate `-mlong-double-64` build fails the AI RNG-bound case, as before; the other 15 numerical cases pass that particular perturbation. A candidate adapter that moves every power result one ULP upward fails all four `test_pow` cases, including packet integer truncation. The numerical game cases still pass that small perturbation. A separate adapter multiplying power results by 1.001 fails the defense and bombing cases, confirming that the candidate reaches real game calls. Neither adapter is part of the normal build.

Debug, Release, host-release and instrumented unit suites pass all 15 executables. All nine scenarios through turn 150 match the native baseline: 519 comparisons each for the release game, release host and coverage host. The release game/host each report 22 matches with unused-storage warnings; the instrumented host reports 32. There are zero semantic differences or comparison errors. These warnings are reported rather than treated as raw-file equality.

The complete tutorial passes all 80 pages, 346 actions and year 2437. The early-Generate rejection check also passes. Counters were cleared before this follow-up workload; no previous audit counters were carried into the final measurement. The observer explicitly flushed coverage before the tutorial runner terminated the game.

| Final fresh coverage | Core | UI | Combined |
| --- | ---: | ---: | ---: |
| Inventoried functions reached | 36/36 | 13/13 | 49/49 |
| Floating-point source anchors reached | 167/167 | 54/54 | 221/221 |
| Whole-function executable lines | 2,644/3,011 | 1,748/2,654 | 4,392/5,665 |
| Whole-function branch outcomes | 2,016/2,459 | 905/1,552 | 2,921/4,011 |

The optional enforcement command is:

```text
python tests/scaffold/floating_coverage.py --build dist/fp-coverage --label "full verified workload" --output dist/softfloat-gap-audit/coverage-final.json --require-all-fp
```

It exits successfully for the completed workload. Before the tutorial flushed its counters, it correctly exited 1 and identified the missing formatting/report/path expressions. Thus the full tutorial remains necessary for this combined coverage claim. The anchor definition and its lexical limitations are unchanged from the initial audit.

Temporary native Windows runner/toolchain and coverage-flush adaptations were restored. The retained `dist/softfloat-audit/windows-harness.patch` still applies to the final source tree; use it only for an instrumented tutorial build, as its coverage hook requires GCC coverage support. The final MinGW Debug configure/build and full unit suite pass after restoration.

Local evidence is in `dist/softfloat-gap-audit/results.json`, `coverage-final.json`, the three regression comparison reports, build/test logs and timestamped tutorial folders. `floating-provenance.json` records permanent source and fixture hashes. The 19 inventoried production source files still match the original audit hashes. No gameplay, file format, original/native baseline or production backend was changed.

## Limits that remain

Closing source-line gaps does not remove the initial audit's broader migration requirements: adversarial inputs throughout battle, movement, scanner accumulation and colonist drops; selected historical 2.83 saves; cross-platform runs; and end-to-end performance measurements. Some functions are still protected mainly by integration results rather than dedicated numerical output tests. Arbitrary `pow` inputs, non-default rounding modes, floating exception flags and exact historical transcendental behavior need separate decisions if they enter the intended contract.

Keep the existing fixtures immutable during migration. Run the same cases against native and software backends, investigate the first discrepancy, and use both the mathematical oracle and the gameplay baseline to explain it. Reaching every inventoried line is useful evidence; it is not a guarantee that every possible game behaves identically.
