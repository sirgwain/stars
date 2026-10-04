# Win16 Parity Repairs

The native build reproduces some behavior of the original Stars! 2.6jrc3 that
depends on Win16 memory layout, uninitialized stack bytes, or the original
compiler and runtime. These repairs exist only so fixed-seed regression runs
(`tests/scaffold/REGRESSION.md`) can show that native and original match turn
for turn, which in turn shows the reconstruction is faithful.

Once parity is no longer needed, the items under **Original bugs emulated**
can be reverted to the behavior the code evidently intended. Each entry lists
where the repair lives, what the original did, and what a revert should do.

The sources in this checkout are maintained manually. Change the `.c`/`.h`
files directly when modifying or reverting a repair, and update this document
in the same change with the affected site, original behavior, and new behavior.
Build and run the regression comparisons after each change; retain any accepted
divergences explicitly rather than hiding them in comparison rules.

## Original bugs emulated (revert when parity is no longer needed)

### Negative-index reads return the Win16 neighbor

- **Original:** a local that may be -1 indexes an array without a check.
  Win16 read whatever its layout put just before the array.
- **Parity repair:** each unguarded read becomes `i != -1 ? path : <alias>`,
  where `<alias>` is the global or field the Win16 layout put before the array.
  Current sites:
  - `ai3.c` DoMacintiAiTurn: `rgshdef[iLatestMiner].cExist` with
    iLatestMiner -1 reads `vtimer.mdForce | vtimer.fAutoGenWhenIn << 16`.
  - `ai3.c` DoMacintiAiTurn and `ai.c` DoRobotoidAiTurn:
    `lpfl->rgcsh[iLatestDestroyer]` with -1 reads `lpfl->pt.y`.
- **Revert to:** drop the alias and handle -1 as "no such design" (count 0),
  or skip the condition, whichever each caller intends.

### Cybertron mine-laying order carries leftover stack bytes

- **Where:** `ai4.c` DoCyberAiTurn, `rgbOrdFrame`.
- **Original:** DoCyberAiTurn's random mine-laying move builds a local
  `ORDER ord` without setting its task union, so `tlm.cTime` (also read as
  `tsell.iPlrX`, the laying countdown: 0 stops, 5 lays forever) is whatever
  the stack slot held. MSVC overlapped block-scoped `shdef`,
  `rgRecycleSBShdef`, and `ord` in the frame. After turn 80, the recycle
  clears zero that slot and the fleet stops laying on arrival.
- **Parity repair:** the three locals share one `rgbOrdFrame` byte array laid
  out like the Win16 frame (`shdef` +0, `ord` +0x84, `rgRecycleSBShdef` +0x86).
- **Revert to:** remove the overlay and set `ord.tlm.cTime = 5` and
  `ord.tlm.cTimeOld = 5`, like the AI's other LayMines orders (lay forever).
- **Not reproduced:** at turn 80 or earlier, with no design scrapped that
  turn, the slot holds residue from functions called before DoCyberAiTurn.
  In the original it is a stack address that shifts with the save path
  length (for example 0xa2e2), and native's frame is uninitialized. The
  regression report shows this as a `tlm.cTime` difference (oneai5, turn 80).

### Tutorial ship builder falls off the end of the function

- **Where:** `tutor.c` FTutorialEnabledShipBuilder, `case tutsbEdit`, marked
  `PARITY`.
- **Original:** the success test was `if (FCheckShipBuilder(0, 2)) break;`
  (tutor.c:3557). The `switch` is the function's last statement and there is
  no final `return`, so the jump goes straight to the epilogue (tutor.c:3732)
  and the function returns whatever AX holds: FCheckShipBuilder's result,
  which is always TRUE on that path.
- **Parity repair:** `if (FCheckShipBuilder(0, 2) != 0) return TRUE;`. Falling
  off the end of a non-void function is undefined in native C, so the return
  value is made explicit.
- **Revert to:** nothing; this is already the evident intent. Drop the marker
  if parity tracking is dropped.

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

- **PszFromLong count pointer** (`utilgen.c`, marked `NATIVE`): the
  original tested `*pcch` instead of `pcch` (`utilgen.c:462`; `PszFromInt`
  tests the pointer). The report dumps pass NULL, so Win16 read, and could
  write, DS:0. Native tests the pointer. **Revert to:** `*pcch != 0`
  (a NULL dereference in native).

## Native record and message boundaries (keep)

- **Player-message links** (`log.c` FWriteLogFile/FLoadLogFile and `msg.c`
  WritePlayerMessages/ReadPlayerMessages, marked `NATIVE`): the original
  wrote the four-byte lpmsgplrNext far pointer followed by four int16_t
  fields and abs(cLen) text bytes. Readers immediately replaced that link.
  Native writes four zero bytes and the same payload, then skips the disk
  link when reading. The old `(uint8_t *)&iPlrFrom - 4` writer exposed native
  pointer bits; the reader needlessly copied those bytes into its link.
  Record size remains abs(cLen)+12, and old nonzero links remain readable.
  **Revert to:** raw MSGPLR serialization only with a genuine four-byte
  pointer layout; never restore native pointer serialization.
- **Malformed player-message records** (`log.c` FLoadLogFile and `msg.c`
  ReadPlayerMessages, marked `NATIVE`): a record shorter than its 12-byte
  header plus abs(cLen) text bytes is skipped. The original copied `cb` bytes
  and later wrote abs(cLen)+12 from the heap, over-reading harmlessly; native
  would compute a negative copy length (cb < 4) or overflow the 1024-byte
  write buffer. Valid records (text is limited to 968 characters) are
  unaffected. **Revert to:** nothing; keep the guard.
- **Battle heap capacity** (`file.c` FLoadGame and `battle.c` FDoCoolBattle,
  marked `NATIVE`): original LOWORD(pointer) was an offset within a Win16
  heap segment, whose HB header occupied 16 bytes. Native finds the owning
  HB and subtracts its base, translating sizeof(HB) back to 16 before
  applying the unchanged 0xffc8 limit. Native address low bits previously
  selected rollover arbitrarily, changing battle record order or risking
  overruns. **Revert to:** LOWORD(pointer) only on the original segmented
  heap; keep the relative-offset calculation for native heaps.
- **Static-control colors** (26 dialog checks across battle.c, create.c,
  msg.c, produce.c, race.c, report.c, research.c, scan.c, ship2.c, stars.c,
  tutor.c and utilgen.c, marked `NATIVE`): original WM_CTLCOLOR carried
  CTLCOLOR_STATIC (6) in HIWORD(lParam). Win32 carries the control HWND in
  lParam and the control type in the message ID; these sites now compare
  message/msg with WM_CTLCOLORSTATIC. **Revert to:** the high-word check only
  when using actual Win16 WM_CTLCOLOR dispatch.
- **WM_CTLCOLOR dispatch** (38 window and dialog procedures across
  battle.c, build.c, create.c, mdi.c, mine.c, msg.c, planet.c, produce.c,
  race.c, report.c, research.c, scan.c, ship2.c, stars.c, tb.c, tutor.c and
  utilgen.c, marked `NATIVE`): each original procedure dispatched every
  message, WM_CTLCOLOR included, through one `switch`. Win32 replaced that
  single message with WM_CTLCOLORMSGBOX through WM_CTLCOLORSTATIC, so the
  switch expression maps that range back to WM_CTLCOLOR
  (`switch (IS_WM_CTLCOLOR(message) ? WM_CTLCOLOR : message)`), keeping
  `case WM_CTLCOLOR:` in its original source position. Every other message
  dispatches unchanged. **Revert to:** `switch (message)` with Win16
  WM_CTLCOLOR dispatch.

## Compiler and runtime behavior matched (keep unless parity is dropped)

These follow the original toolchain rather than a bug, and the game's
results depend on them.

- **qsort tie order:** `qsort` maps to `qsort16`
  (implemented in `win16defines.h`), rebuilt from the Win16 CRT, so
  elements with equal keys end up in the same order. Native libc qsort
  orders ties differently.
- **x87 precision:** floating arithmetic keeps the original's extended
  precision. Casts that round an x87 result to double or float are preserved.

## Known Win16 behavior not reproduced

- **Macinti armada strength** (`ai3.c` TargetMacArmada): `FPotentMacWarFleet`
  returns 0 without writing `*pcEquiv` for a weak fleet, and TargetMacArmada
  ignores the result and compares `cshWar` (BP-0x20) against the armada
  potency thresholds anyway. The original's value is whatever the fleet loop
  in DoMacintiAiTurn last left at that stack address. TargetMacArmada's own
  callees run below it and never touch it, but other calls from the loop do.
  The value chooses between returning and targeting, and it also sets how many
  `Random(10)` draws are made, so a wrong guess shifts the RNG stream for the
  rest of the turn. This is the cause of the oneai6 and smallai6 divergences
  (first seen at smallai6 t57, oneai6 t68). In a test build, setting
  `cshWar = 1000` (above every threshold) matched smallai6 through t150 and
  oneai6 through t84. Oneai6 then drifted at t85: the draw counts differed but
  the decisions didn't, and the wormhole jumps showed the shifted RNG.
  Making it `static` did not match either. **Revert to:** initialize `cshWar`
  to 0, or have FPotentMacWarFleet always store `cEquiv`.
- **Macinti mine-laying order** (`ai3.c` DoMacintiAiTurn): same unset task
  union as Cybertron. Its `tlm.cTime` slot (BP-0xba) overlaps the far-pointer
  segment of block local `lpplBest` and the tail of `shdef`, so the original
  value depends on a Win16 selector and cannot be reproduced deterministically.
- **PszFormatString** `vrgszUnits[-1]`: display text only. The original read a
  Win16 pointer.

## Regression harness tolerances

`dist/stars-save save compare` treats storage the game never reads as warnings,
not differences: order `fUnused`, task-union words past those the task reads,
`tlm.cTimeOld`, AIHIST freighter slots past `cFreighter`, and the reserved
CYBERINFO byte. See `tests/scaffold/REGRESSION.md`.
