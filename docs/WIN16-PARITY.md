# Win16 Parity Repairs

The `2.6j` branch reproduces behavior of the original Stars! 2.6jrc3 that
depends on Win16 memory layout, uninitialized stack bytes, or the original
compiler and runtime, so the fixed-seed regression
(`tests/scaffold/REGRESSION.md`) could show native and original matching
turn for turn. On `main` those emulated bugs are fixed (see
`CHANGELOG.md`); this document lists what remains: guards against native
corruption, native record and message boundaries, and toolchain behavior
the game's results still depend on.

Change the `.c`/`.h` files directly when modifying one of these, and update
this document in the same change with the affected site, original behavior,
and new behavior. Build and run the regression comparisons after each change.

## Corruption guards (keep)

These stop native memory corruption where the original corrupted dead
storage harmlessly. They don't change game behavior and should stay.

- **Cybertron recycle writes for absent designs** (`ai4.c` DoCyberAiTurn):
  `rgRecycleShdef[iLatestDestroyer|iLatestCargo] = 0` with index -1
  overwrote a byte of a dead `lppl` pointer in Win16. Native skips the store
  when the index is -1.
- **Macinti late-game splits** (`ai3.c` DoMacintiAiTurn): turn>80 code
  copied from the 16-entry ship logic marked entries 10–15 of the 10-entry
  `rgRecycleSBShdef`, writing into `l` and `fTonsOfMinerals`. SplitOutShdefs
  read all 16. Native uses a 16-entry `rgSplitShdef` scratch array. The stale
  bytes the original read between calls are not reproduced.
- **Starbase attack mask index** (`battle.c` CplrBattle, marked `NATIVE`):
  in the starbase `iplrAttackEveryone` and `default` (attack one player) cases,
  the original wrote `rggrfAttack[iplrCur]` (BP-0x4c, battle.c:902
  and 917) before `iplrCur` was assigned. The slot held stale stack data, so
  the original wrote the mask to an arbitrary entry, or past the 16-entry
  array, and left the starbase's own entry unset. Native indexes with
  `iplrStarbase`, which the surrounding code evidently intended. This can
  change battle outcomes from the original's. The regression results are
  unchanged. **Revert to:** `rggrfAttack[iplrCur]` at both sites
  (undefined behavior in native).
- **Give-order label width** (`ship.c` DrawShipWayPtOrders, `grTaskGive`,
  marked `NATIVE`): the original measured the "To" label with
  `GetTextExtent(hdc, psz, cch)` while `psz` was still unset (BP-0x32,
  ship.c:377), even though the text is built in and drawn from `szT`. Native
  measures `szT`. Display only. **Revert to:** `psz` (a wild pointer read in
  native).

- **PszFromLong count pointer** (`utilgen.c`): the original tested `*pcch`
  instead of `pcch` (`utilgen.c:462` in the binary; `PszFromInt` tests the
  pointer). The report dumps pass NULL, so Win16 read, and could write,
  DS:0. Native tests the pointer, which also always stores the count.
  **Revert to:** nothing; a NULL dereference crashes natively.

## Native record and message boundaries (keep)

- **Malformed player-message records** (`msg.c` LpmsgplrFromRt, used by
  `log.c` FLoadLogFile and `msg.c` ReadPlayerMessages, marked `NATIVE`): a
  record shorter than its 12-byte header plus abs(cLen) text bytes is
  skipped. The original copied `cb` bytes and later wrote abs(cLen)+12 from
  the heap, over-reading harmlessly; native would compute a negative copy
  length (cb < 4) or overflow the 1024-byte write buffer. Valid records (text is limited to 968 characters) are
  unaffected. **Revert to:** nothing; keep the guard.
- **Battle heap capacity** (`file.c` FLoadGame and `battle.c` FDoCoolBattle,
  marked `NATIVE`): original LOWORD(pointer) was an offset within a Win16
  heap segment, whose HB header occupied 16 bytes. Native finds the owning
  HB and subtracts its base, translating sizeof(HB) back to 16 before
  applying the unchanged 0xffc8 limit. Native address low bits previously
  selected rollover arbitrarily, changing battle record order or risking
  overruns. **Revert to:** LOWORD(pointer) only on the original segmented
  heap; keep the relative-offset calculation for native heaps.
## Compiler and runtime behavior matched (keep unless parity is dropped)

These follow the original toolchain rather than a bug, and the game's
results depend on them.

- **qsort tie order:** the game sorts with `qsort16`
  (implemented in `native.c`), rebuilt from the Win16 CRT, so
  elements with equal keys end up in the same order. Native libc qsort
  orders ties differently.
- **x87 precision:** `sfnum.h` uses SoftFloat's extended format and preserves
  the explicit narrowing to binary64/binary32. Native x87 instructions are
  no longer required. See [SOFTFLOAT-IMPLEMENTATION.md](SOFTFLOAT-IMPLEMENTATION.md).

## Regression harness tolerances

`dist/stars-save save compare` treats storage the game never reads as warnings,
not differences: order `fUnused`, task-union words past those the task reads,
`tlm.cTimeOld`, AIHIST freighter slots past `cFreighter`, and the reserved
CYBERINFO byte. See `tests/scaffold/REGRESSION.md`.
