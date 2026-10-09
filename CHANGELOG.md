# Changelog

Changes from the original Stars! 2.6jrc3 (tag `2.6jrc3`, branch `2.6j`).
Entries marked **host results** change turn generation, so a 2.8 host can
produce different results from a 2.6j host for the same turn.

## Unreleased

### Added

- The mouse wheel zooms the scanner at the cursor, one zoom level a notch;
  Ctrl+wheel (a touchpad pinch) zooms too. The tilt wheel and Shift+wheel
  scroll the scanner sideways.
- The mouse wheel scrolls the report tables by rows, as many a notch as the
  mouse settings say; the tilt wheel and Shift+wheel scroll a column a
  notch.
- Alt+click (Cmd+click on a Mac) with a fleet selected adds a waypoint at
  the fastest useful speed: the speed a fleet with a colonize task gets,
  the fastest up to warp 9 whose fuel fits, slowed to the lowest that
  arrives as soon. Alt held while dropping a dragged waypoint does the
  same. Players set colonize tasks on scouts to get this speed, at the cost
  of a warning every turn. Turn generation is unchanged.
- The selected fleet's path in the scanner has a tick where each year of
  travel ends: a fleet moves its warp squared in light years a year, each
  leg at its own warp, and stops at every waypoint.
- Dragging the scanner with the left button pans it. A press still selects
  what is under it, as a click does; the view moves once the mouse moves
  past the system's drag distance. Dragging a waypoint, Shift and Ctrl
  clicks and the measuring tape (Shift+right-drag) are unchanged.
- The wheel goes to the window under the cursor rather than the window with
  the focus, so the panes and lists scroll where the player points, as with
  Windows 10's "scroll inactive windows" setting. A drop-down list the player
  only points at ignores the wheel, so it can't change an order by accident.

### Changed

- The game's text and static data are now built from source files rather
  than checked-in C tables: the strings, messages, tutorial and planet names
  from `text/*.txt`, and the hulls, parts, starting designs, predefined and
  AI races, default battle plans and the AI's designs, part preferences and
  research orders from `data/**/*.yaml`. Building needs Python 3 and PyYAML.
  The text, data, turns and files are unchanged.

### Fixed

- Regression `generate()` uses native paths for trace output on Windows;
  it previously supplied a Wine drive path that prevented `trace.log` from
  being created. Turn generation and regression baselines are unchanged.
- Eight uninitialized reads from the original no longer stop MSVC Debug
  builds: the mine pane size in `CreateChildWindows`, the tutorial progress
  tick and saved cursor in `CommandHandler`, the dialog close results in
  `ScoreXDlg` and `VCRDlg`, the mineral index in `IdTargetFreighter`, the
  scale flag for empty gauges in `LDrawGauge`, and the mine-laying message
  ID that `DrawShipWayPtOrders` uses to choose its text color. Turn
  generation and saved files are unchanged.
- The scanner no longer leaves a band drawn out of place when it scrolls
  while a redraw is still pending (`ScrollScanner`), as after a wheel zoom
  followed at once by a trackpad scroll. The original drew the pending
  area at the new position and then scrolled it with the rest.

## 2.9.0 (2026-10-05)

### Added

- `stars-host`, the game's host without windows, for Windows, Linux and
  macOS. It takes `stars.exe`'s host command line (`-a`, `-g`, `-b`, `-t`,
  `-v`, `-s`, `-p`, `-l`) and generates the same turns where `long double`
  is x87 extended precision (x86-64; on Apple silicon, an x86_64 build run
  by Rosetta). Messages go to stderr; it never waits for a player.
  `stars-host --version` prints its version, which names its platform
  (`2.9 linux-x64`, `2.9 macos-x64`); the Windows build keeps `2.9x64`.
  It writes the universe, planet and fleet dumps (`-dm`, `-dp`, `-df`) as
  `stars.exe` does, and `--ini <file>` reads the `stars.ini` settings a
  host uses (`[Files] Logging`; `[Misc] DefaultPassword`, `NewReports`,
  `NoHostNames`, `Backups`).

### Fixed

- Creating a universe from a definition file no longer writes the race
  file's path past the end of a 16-byte name buffer (`FWasRaceFile`). Only
  the file name is kept, as the race dialogs keep it. Results are unchanged;
  macOS stopped the program at the overrun.

## 2.8.1

### Changed

- Removed serial number requirement and various cheater checks during turn
  generation. Still writing empty serial bytes in place of old rgbConfig/lSerial
  in rtlog records for backwards compatibility with tools that load it.

### Fixed

- Patch releases now display `Version 2.8.1x64` instead of
  `Version 2.8x64 (2.8.1)` (`SzVersion`, `cmake/version.cmake`). Initial
  releases still omit `.0`, and development builds retain their full version.

## 2.8.0 (2026-10-04)

### Changed

- Files written by 2.8 (games, turns, orders, histories and races) carry
  file format version 2.84 instead of 2.83, so Stars! 2.6j and 2.7 refuse
  them. 2.8 still loads 2.6j and 2.7 files, so a 2.8 host can take over a
  2.6j game. The records are unchanged (`WriteBOF`). Opening a file from a
  later version now says it is newer; the original called 2.84 files older
  (`FOpenFile`).

### Added

- Product version from git tags and build numbers, stored in a
  `VERSIONINFO` resource. It is shown as `Version 2.8x64` (or
  `2.8x64 (2.8.1-dev.N+gSHA)` between releases) in the About box, splash
  screen and report headers, marking the 64-bit Win32 build.
  The save-file format version is separate (see Changed). See
  [docs/VERSIONING.md](docs/VERSIONING.md).

### Fixed

- Macinti AI: deciding whether to build mine layers no longer reads past
  the start of its design list when it has no miner design; a missing design
  counts as zero ships (`DoMacintiAiTurn`). **Host results.**
- Robotoid and Macinti AI: deciding whether an attack fleet joins up with
  others no longer reads a fleet's position as a ship count when the AI has
  no destroyer design (`DoRobotoidAiTurn`, `DoMacintiAiTurn`). **Host
  results.**
- Cybertron AI: mine-laying fleets sent to a random planet now lay mines
  there indefinitely, like the AI's other mine-laying orders. The original
  left the order's countdown as uninitialized stack bytes, so whether the
  fleet laid at all depended on the save path and, after turn 80, it
  usually stopped on arrival (`DoCyberAiTurn`). **Host results.**
- Macinti AI: the same fix for its mine-laying fleets, whose order's
  countdown was uninitialized stack bytes (`DoMacintiAiTurn`). **Host
  results.**
- Macinti AI: an armada too weak to count as a war fleet is now judged by
  its actual strength when choosing whether to attack, retreat or wait. The
  original compared an uninitialized value (`FPotentMacWarFleet`,
  `TargetMacArmada`). **Host results.**
- Message text: a quantity formatted without a preceding mineral no longer
  reads a unit string from before the unit table; it gets no unit
  (`PszFormatString`). Display only.
- Minefields: fleets travelling due north or south now hit minefields along
  their route. The route/field intersection measured a vertical route from
  its start point instead of its closest point to the field
  (`FIntersectCircleLine`). **Host results.**
- Cybertron AI: no longer scraps the ships of its newest starbase-defender
  design once that design passes the recycle age. After checking its
  defender designs it protected the newest destroyer design again instead
  (`DoCyberAiTurn`). **Host results.**
- Ship design: with Regenerating Shields, a large armor slot (such as 24
  Superlatanium on a Space Dock) no longer overflows into a huge armor
  value; it gets half its armor like any other (`UpdateShdefCost`).
  **Host results.**
- Scanning: scanners and stargates with ranges above about 463 ly now see
  cloaked starbases (such as ISB races') within their reduced range. The
  range-times-visibility product overflowed 32 bits, so lightly cloaked
  starbases were often only obscured (`SetVisPFFleets`, `SetVisPFPlanets`,
  `SetVisPFThings`). **Host results.**
- Colonizing: a ship needs an installed colonization or orbital
  construction module. A design slot that once held a module but now holds
  none no longer counts (`SatisfyOrders`). **Host results.**
- Native build: the Battle Plans drop-downs (targets, attack who, tactic)
  saved 0 (None/Disengage, Nobody) whatever was chosen, and the production
  dialog's Required Minerals panel ignored the selected item. Both sent the
  Win16 message numbers for `CB_GETCURSEL`/`LB_GETCURSEL`, which Win32
  controls ignore (`BattlePlansDlg`, `DrawProductionDlg`). Now as in the
  original.
- Native build: the planet and fleet dumps no longer crash. `PszFromLong`
  tested the value behind its optional count pointer instead of the
  pointer, which the dumps pass as NULL; Win16 silently read the start of
  its data segment.
- Native build: the planet dump's Factories column printed 0 and shifted
  the factory count into Def %. The mine and factory counts were passed as
  two 16-bit words each, as the Win16 binary pushed them (`DumpPlanets`).
- File copies (`StarsCopyFile`, used for turn-file backups) close the copy
  when done. The original closed the wrong handle, leaving each backup
  open with exclusive sharing for the rest of the session, and a copy whose
  destination couldn't be created left the error-recovery state pointing at
  a finished call.
- Turn generation keeps a backup of the turn file of a player who submitted
  no turn, as the original did. The reconstruction had the copy's source
  and destination reversed (`FGenerateTurn`).
- Claim Adjuster turn files carry only the other players' habitat. To give
  a CA player their habitat, the original sent every visible player's full
  record, including tech levels, research spending and target, racial
  traits, production template and diplomatic relations (`FWriteDataFile`).
  **Host results** for AI Claim Adjusters, which plan from their own turn
  file.
- Stealing: a waypoint transport order can no longer load colonists or fuel
  from a planet or fleet it may only rob with a Robber Baron or Pick Pocket
  scanner; the player gets the "attempt was unsuccessful" message instead.
  Minerals can still be stolen (`SatisfyOrders`). **Host results.**
- Race files: shortening a race name no longer saves a race file that Stars!
  rejects as corrupt. The checksum covered leftover characters of the
  longer name in memory, which aren't saved (`IRaceChecksum`). Race files
  already saved with a bad checksum still have to be recreated.
- Pursuit: when a pursued fleet splits or disappears, every pursuer
  retargets the heaviest matching fleet where it was last seen, as in 2.6j
  RC4. RC3 (the reconstructed release) spread pursuers over fleets nobody
  else was chasing (`ValidateWaypoints`). **Host results.**
- `FCheckFile`'s AI-player check reads the requested player's record. The
  original stopped at the first player record, or at any earlier record
  whose first byte matched the player number, and read its AI bit. No
  caller uses this check yet, so nothing changes in play.
- Loading a corrupt player record no longer writes past the player's data.
  The original trusted the record's relation count and name lengths, so a
  count above 16 or a name of 32 or more characters overran the relations
  and names (`ReadRtPlr`). Valid files load as before.
- Native build: a fleet's "has completed its assigned orders" message is
  removed again when the fleet is used up colonizing, scrapping or meeting
  the Mystery Trader, or is replaced when it completes new orders, as in
  the original. The stored goto was compared as unsigned and the fleet's
  goto, which has its high bit set, as signed, so they never matched
  (`FRemovePlayerMessage`, `FFindPlayerMessage`).
- Healing: ships that gated, fought or hit mines no longer heal by merging
  into a fleet that didn't; the fleet they join doesn't heal that turn
  either (`Merge2Fleets`). **Host results.**
- Native build: a detonating minefield no longer crashes turn generation
  when a fleet is inside it. `FTravelThroughMineFields` read the travel
  distance through the NULL pointer `ThingDecay` passes for a detonation,
  which Win16 read harmlessly from the start of its data segment.
- Minefields: a fleet immune to a detonating minefield (its owner's mine
  layers in their own field) can still be hit by another player's
  detonating field that turn. The immune check used up the fleet's one
  detonation check (`ThingDecay`, `FTravelThroughMineFields`). **Host
  results.**
- Fleets: merging can no longer pass 32767 ships of one design in a
  fleet. A waypoint or AI merge wrapped the count negative; now the ships
  that don't fit stay in their own fleet (`Merge2Fleets`). Merging all
  fleets at a location clamped the count to 32766 and lost the rest; now a
  fleet that doesn't fit is left out (`FFleetMergeAll`). **Host results.**
- Pursuit: a fleet chasing another no longer stops when a third fleet
  chasing it catches up. The pursuer marked the caught fleet as done
  moving; now it keeps following with the rest of its move (`MoveFleets`).
  **Host results.**
- Cargo: minerals, fuel or colonists sent to another player's fleet that
  don't fit in its hold stay with the sender, as the "unable to transfer"
  message says. The original took them from the sender and destroyed them
  (`FRunLogRecord`). **Host results.**
- Turn files no longer tell a player which other players have seen a
  minefield, a wormhole or the Mystery Trader, who has travelled through a
  wormhole, or which part the trader carries. Each player's file keeps only
  that player's bit of these masks (`FWriteDataFile`). Turn files only; AI
  play is unchanged.
- Production: changing or deleting a ship or starbase design clears the
  progress of queued items of that design. The original kept the
  completion percentage and charged only the rest at the new design's cost,
  so a mostly built cheap starbase could be finished as an expensive one
  (`FRunLogRecord`). **Host results.**
- Scores: a planet's population and resources in the published scores are
  taken as it produced and grew that year. Loading colonists onto ships at
  waypoint 1, after production, no longer lowers them (`Produce`,
  `CalcPlayerScore`). Scores only; play is unchanged.
- Popups: right-clicking a location on the map, and choosing a waypoint's
  target, list every fleet and object there. The original stopped at 100
  entries, so objects past them couldn't be selected or targeted
  (`ScannerWndProc`, `ClickInShipOrders`, `PopupMenu`). Client only.
- Race wizard: a race that was Random and is then customized is no longer
  replaced by a random race when the game is created. Choosing Custom kept
  the random-race setting (`RaceWizardDlg1`).
