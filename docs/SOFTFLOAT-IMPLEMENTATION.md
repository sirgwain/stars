# Software floating-point implementation

The core and Windows UI now use Berkeley SoftFloat 3e. `sfnum.h` preserves
the previous GCC expression types: `Sf80` uses the x87 extended format
(64 significand bits and a 15-bit exponent), while `Sf64` and `Sf32` preserve
the explicit binary64 and binary32 rounding boundaries. Integer arithmetic
and its original grouping remain intact. Integer conversions truncate
toward zero; arithmetic rounds to nearest, ties to even.

Native `float` and `double` remain in existing signatures and local variables
as bit carriers, copied with `memcpy`. They do not perform game arithmetic.
There are no changes to disk structures, record sizes or file versions.
No production `long double` or system `pow`, `sqrt`, `floor`, `hypot`, `atan2`,
`sin` or `cos` calls remain. The optimized object-code audit checks this
independently of source spelling and runs in the regression CI workflow.
It checks both floating instructions and undefined native math symbols,
including imported functions and addresses stored in data. Compiled negative
controls verify rejection of direct calls, float/long-double variants,
imported calls and function pointers; software calls and bit carriers pass.

## Numerical implementation

`third_party/softfloat` contains unmodified upstream sources, the upstream
license and the pinned archive hash. CMake builds portable integer primitives
with the `8086-SSE` specialization. Its state is process-global, matching the
single-threaded game; callers must not change the nearest-even mode or
`extF80_roundingPrecision == 80`. Exception flags are not consumed by the game.
The license is staged beside the executable and included in release assets.

`sfnum.c` implements the functions SoftFloat does not supply using its
binary128 arithmetic:

- `pow`: exponentiation by squaring for small integer exponents; otherwise
  a range-reduced logarithm (atanh series) and exponential (Taylor series).
  Arguments near one avoid cancellation against an approximation to ln(2).
  The integer path preserves exact midpoint cases lost by a log/exp round trip.
- `atan2`: quadrant handling and a reduced atan series.
- `sin` / `cos`: angle reduction and series, for the UI's angle domain.
  The supported range is `|angle| <= 2^20` radians; larger inputs return NaN.
  The game calls these with angles from `atan2` and a small number of revolutions.
- `hypot`: binary128 squares and square root, avoiding binary64 intermediate
  overflow/underflow. `sqrt` and `floor` use SoftFloat primitives directly.

These are independently written algorithms. **MPFR and GMP are neither
production dependencies nor runtime fallbacks, and none of their implementation
code is included.** Only the optional offline oracle generators link them.
Ordinary builds and unit tests consume frozen reference data and need neither.

Binary128 guard digits and the tests establish accuracy for the tested inputs;
they are **not a proof of correctly rounded transcendental results for every
binary64 input**. This is a game numerical layer, not a general replacement for
every libm contract: errno, floating exceptions for transcendentals, alternate
rounding modes and huge-angle reduction are not supported contracts.

## Compatibility and rounding

The existing 3,180 game/UI reference records and all original `floating-pow.txt`
rows are unchanged. Of the 2,117 original pow inputs, the implementation matches
GCC exactly on 2,116. One result intentionally follows the correctly rounded
oracle instead:

| Input | GCC result bits | Software/oracle result bits |
|---|---|---|
| `pow(0.75, 0x1.9ffffffffffffp+3)` | `3f9853d300000004` | `3f9853d300000003` |

The compatibility test identifies that exact input and both historical values;
it does not grant a general one-ULP tolerance. The accuracy test requires zero
ULP difference throughout the corpus. Packet integer-boundary tests still match
the historical results. The game regression baseline is not regenerated.

## Validation on Windows x64 / GCC 16.2.0

- Debug, Release and Windows host configurations: 16 unit executables,
  67 test cases, including all 16 game/UI numerical cases.
- Original pow corpus: 2,117 input pairs, exact oracle rounding, special
  values and 10,860 packet mass/input combinations for multiplication and division.
- New independent corpus: 14,000 exact expected results (2,000 each for sqrt,
  floor, hypot, atan2, sin, cos and pow). MPFR directed lower/upper results
  must round to the same binary64 value before a reference is accepted.
- 10,000 software extended-precision multiply/add/divide sequences compared
  bit-for-bit with native x87, including narrowing to float/double, plus the
  AI boundary `400 * (long double)0.3 -> 119`.
- Release game and Windows command-line host: nine scenarios through turn
  150; each passes all 519 saved-state comparisons, with 22 existing
  unused-storage warnings and zero differences/errors.
- Optimized game/UI object-code audit: 65 objects, zero native floating
  arithmetic, comparison or numeric-conversion instructions.
- Reduced-native-precision control (`-mlong-double-64`): all 16 test
  executables pass, including the AI boundary that failed before migration.
- Tutorial: all 80 pages and 346 actions through year 2437; premature
  generation rejection also passes.
- Combined unit/host-regression/tutorial coverage: 49/49 inventoried functions
  and 228/228 floating-point executable source anchors; 4,404/5,677 function
  lines and 2,923/4,013 branch outcomes. Source anchors increased from 221
  because software calls are explicit and long expressions span more lines.
  This is full execution of the identified anchors, not exhaustive input or
  branch coverage. The instrumented host also passes all 519 comparisons
  (32 existing unused-storage warnings).

Local logs are in `dist/softfloat-implementation/`. Temporary native-Windows
regression/tutorial harness changes were restored after validation.

A small local timing sample (three paired runs of `smallai4`, checkpoint 100
through 150) measured median wall times of 5.779 seconds for the saved GCC
host and 5.755 seconds for SoftFloat. There was no measurable whole-turn cost
in this sample; it ran alongside the tutorial and is not a performance
guarantee for other workloads.

The regression checks caught an intermediate conversion mistake after a
parenthesis simplification: `oneai1`, turn 25, first differed in `game.hst`
(player record and fleet orders). Restoring the original integer grouping
removed the divergence. Both full regressions were rerun successfully.

## Running the checks

Use the normal build, unit, regression, host and tutorial targets described
in `AGENTS.md`. On x86, additionally run:

```sh
python tests/scaffold/check_software_float.py --build dist/mingw-release --objdump x86_64-w64-mingw32-objdump
python tests/scaffold/test_check_software_float.py --cc x86_64-w64-mingw32-gcc --objdump x86_64-w64-mingw32-objdump
```

The new offline oracle can be reproduced separately (MPFR 4.2.2 in this run):

```sh
gcc -std=gnu11 -O2 tests/scaffold/softfloat_reference.c -lmpfr -lgmp -o softfloat_reference
./softfloat_reference > /tmp/floating-transcendentals.txt
```

Compare this output with the checked-in file; ordinary test runs never rewrite
it. See its adjacent provenance JSON for hashes, seed and domains.

Linux, macOS and ARM execution have not been validated in this Windows run.
The former native-long-double restriction is removed, but each platform still
needs its full regression and UI checks before claiming platform parity.
