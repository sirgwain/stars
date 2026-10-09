#!/usr/bin/env python3
"""Stage, run, and compare fixed-seed native Stars! AI regression scenarios."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SAVE_CLI = ROOT / "dist" / ("stars-save.exe" if sys.platform == "win32" else "stars-save")
SCENARIOS = ("noai", "oneai1", "oneai2", "oneai3", "oneai4", "oneai5", "oneai6", "smallai4", "smallai6")
CHECKPOINTS = (0, 1, 10, 25, 50, 80, 100, 150)
SAVE_NAME = re.compile(r"game\.(xy|hst|[mhx](?:[1-9]|1[0-6]))$", re.I)


def digest(path):
    """digest hashes an input or output for the run manifest."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(args):
    """prepare stages identical definitions and race data without overwriting runs."""
    work = args.work.resolve()
    if any(character.isspace() for character in str(work)):
        raise ValueError("work path must not contain whitespace; Stars! cannot quote filenames")
    if work.exists():
        raise ValueError(f"work directory already exists: {work}; choose a fresh directory")
    if not 0 <= args.seed <= 0xFFFFFFFF:
        raise ValueError("seed must fit uint32")
    exe = args.exe.resolve()
    if not exe.is_file():
        raise ValueError(f"missing executable: {exe}")
    work.mkdir(parents=True)
    # Every launch passes -s<seed>, so any build of stars.exe repeats exactly.
    manifest = {"engine": "native", "seed": args.seed, "exe": str(exe),
                "exe_sha256": digest(exe), "scenarios": {}, "fixtures": {}}
    manifest["race_sha256"] = digest(ROOT / "tests/scaffold/fixtures/newgame/tiny/humanoid.r1")
    for name in SCENARIOS:
        dest = work / name
        dest.mkdir()
        lines = (ROOT / f"tests/scaffold/fixtures/regression/{name}.def").read_text().splitlines()
        lines[1] = lines[1].rsplit(" ", 1)[0] + f" {args.seed}"
        manifest["fixtures"][name] = hashlib.sha256("\n".join(lines).encode("ascii")).hexdigest()
        lines[4] = game_path(exe, dest, "human.r1")
        lines[-1] = game_path(exe, dest, "game.xy")
        (dest / "game.def").write_bytes(("\r\n".join(lines) + "\r\n").encode("ascii"))
        shutil.copyfile(ROOT / "tests/scaffold/fixtures/newgame/tiny/humanoid.r1", dest / "human.r1")
        manifest["scenarios"][name] = {p.name: digest(p) for p in dest.iterdir()}
    (work / "run.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Prepared run: {work}")


def host_turn(path):
    """host_turn validates the unencrypted host header and returns its turn."""
    data = path.read_bytes()
    if len(data) < 18 or data[:2] != b"\x10\x20" or data[2:6] != b"J3J3" or data[16] != 2:
        raise ValueError(f"invalid host header: {path}")
    return struct.unpack_from("<H", data, 12)[0]


def checked_files(directory, checksums, label):
    """checked_files verifies saved files against their recorded hashes."""
    for filename, checksum in checksums.items():
        if digest(directory / filename) != checksum:
            raise ValueError(f"{label} changed: {directory / filename}")


def windows_path(path):
    """windows_path converts a host path to Wine's Z: drive."""
    return "Z:" + str(path).replace("/", "\\")


def runs_in_wine(exe):
    """Windows runs all binaries directly; other hosts use Wine for .exe files."""
    return sys.platform != "win32" and Path(exe).suffix.lower() == ".exe"


def game_path(exe, path, filename):
    """game_path names a file in a game directory the way exe reads paths."""
    if runs_in_wine(exe):
        return windows_path(path) + "\\" + filename
    return str(Path(path) / filename)


def copy_files(source, destination, filenames):
    """copy_files copies the named checkpoint files into a directory."""
    for filename in filenames:
        shutil.copyfile(source / filename, destination / filename)


def start_from_baseline(args, manifest, name, directory, snapshots, has_data):
    """start_from_baseline imports a verified turn-zero checkpoint."""
    if has_data or args.resume:
        raise ValueError("--baseline requires a fresh scenario and cannot be combined with --resume")
    baseline = args.baseline.resolve()
    reference = json.loads((baseline / "run.json").read_text())
    if any(reference[key] != manifest[key] for key in ("seed", "fixtures", "race_sha256")):
        raise ValueError("baseline seed or fixtures differ from this run")
    source = baseline / name / "checkpoints/000"
    saved = json.loads((source / "checkpoint.json").read_text())
    checked_files(source, saved["files"], "baseline checkpoint file")
    if host_turn(source / "game.hst") != 0:
        raise ValueError("baseline must start at turn zero")
    checkpoint = snapshots / "000"
    checkpoint.mkdir()
    copy_files(source, checkpoint, saved["files"])
    copy_files(source, directory, saved["files"])
    saved["origin"] = "baseline"
    saved["source_checkpoint"] = str(source)
    (checkpoint / "checkpoint.json").write_text(json.dumps(saved, indent=2) + "\n")
    print(f"{name}: using reference creation files from {source}; native creation is not tested")


def restore_checkpoint(args, name, directory, snapshots):
    """restore_checkpoint verifies the latest checkpoint and preserves live files."""
    completed = [t for t in CHECKPOINTS if (snapshots / f"{t:03}" / "checkpoint.json").is_file()]
    if not completed or completed != list(CHECKPOINTS[:len(completed)]):
        raise ValueError(f"{name}: resume requires consecutive completed checkpoints starting at 000")
    previous = completed[-1]
    if previous >= args.through:
        print(f"{name}: already completed through turn {previous}")
        return completed
    checkpoint = snapshots / f"{previous:03}"
    saved = json.loads((checkpoint / "checkpoint.json").read_text())
    checked_files(checkpoint, saved["files"], "checkpoint file")
    if host_turn(checkpoint / "game.hst") != previous:
        raise ValueError(f"{name}: checkpoint host turn is incorrect")
    preserved = Path(tempfile.mkdtemp(prefix="before-resume-", dir=directory))
    for path in directory.iterdir():
        if SAVE_NAME.fullmatch(path.name):
            shutil.move(path, preserved / path.name)
        elif path.name.startswith("run-") and path.suffix == ".log":
            shutil.copyfile(path, preserved / path.name)
    copy_files(checkpoint, directory, saved["files"])
    print(f"{name}: restored turn {previous}; previous live files preserved in {preserved}")
    return completed


def launch_command(exe, seed, directory, turn, previous):
    """launch_command builds the creation or incremental generation command.

    -s<seed> replaces the clock as the startup seed, so a launch repeats
    exactly; the seed resets on every launch.
    """
    flags = ["-a", game_path(exe, directory, "game.def")] if turn == 0 else [f"-g{turn - previous}", game_path(exe, directory, "game.hst")]
    launcher = ["wine"] if runs_in_wine(exe) else []
    return [*launcher, str(exe), f"-s{seed}", *flags], directory


def capture_checkpoint(directory, snapshots, name, turn, command, exit_code):
    """capture_checkpoint validates generated saves and records their hashes."""
    saves = {p.name.lower(): p for p in directory.iterdir() if SAVE_NAME.fullmatch(p.name)}
    if not {"game.xy", "game.hst"} <= saves.keys():
        raise ValueError(f"{name}: missing universe or host file at turn {turn}; see run log")
    actual = host_turn(saves["game.hst"])
    if actual != turn:
        raise ValueError(f"{name}: expected turn {turn}, found {actual}; stopping before snapshot")
    checkpoint = snapshots / f"{turn:03}"
    checkpoint.mkdir()
    for filename, path in saves.items():
        shutil.copyfile(path, checkpoint / filename)
    report = {"turn": turn, "year": 2400 + turn, "command": command, "exit_code": exit_code,
              "files": {filename: digest(path) for filename, path in saves.items()}}
    (checkpoint / "checkpoint.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"Saved {checkpoint}", flush=True)


def run(args):
    """run executes matching launch boundaries and saves verified checkpoints."""
    work = args.work.resolve()
    manifest = json.loads((work / "run.json").read_text())
    exe = Path(manifest["exe"])
    if digest(exe) != manifest["exe_sha256"]:
        raise ValueError("executable changed since prepare; prepare a fresh run")
    for name in args.scenario or SCENARIOS:
        directory = work / name
        snapshots = directory / "checkpoints"
        has_data = (snapshots.exists() and any(snapshots.iterdir())) or any(SAVE_NAME.fullmatch(p.name) for p in directory.iterdir())
        if has_data and not args.resume:
            raise ValueError(f"{name} already has run data; prepare a fresh run")
        checked_files(directory, manifest["scenarios"][name], "fixture")
        snapshots.mkdir(exist_ok=True)
        completed = []
        if args.baseline:
            start_from_baseline(args, manifest, name, directory, snapshots, has_data)
            completed = [0]
        if args.resume:
            completed = restore_checkpoint(args, name, directory, snapshots)
            if completed[-1] >= args.through:
                continue
        previous = completed[-1] if completed else 0
        for turn in CHECKPOINTS:
            if turn in completed:
                continue
            if turn > args.through:
                break
            command, cwd = launch_command(exe, manifest["seed"], directory, turn, previous)
            print(f"{name}: turn {turn}: {' '.join(command)}", flush=True)
            with (directory / f"run-{turn:03}.log").open("w") as log:
                process = subprocess.run(command, cwd=cwd, stdout=log, stderr=subprocess.STDOUT,
                                         check=False, timeout=args.timeout)
            # FGenerateTurn sets vretExitValue=1 on successful command-line generation.
            expected_exit = 1 if turn > 0 else 0
            if process.returncode != expected_exit:
                raise ValueError(f"{name}: exit {process.returncode}, expected {expected_exit}; see run-{turn:03}.log")
            if turn == 0:
                generated = {p.name.lower(): p for p in directory.iterdir()}
                if "game.m1" not in generated or "game.hst" not in generated:
                    raise ValueError(f"{name}: missing player turn or host file after game creation")
                player_file = generated["game.m1"]
                subprocess.run([str(args.cli.resolve()), "save", "update", str(player_file), "--ai", "maid"],
                               cwd=ROOT, check=True)
                subprocess.run([str(args.cli.resolve()), "save", "update", str(generated["game.hst"]),
                                "--ai", "maid", "--player", "1"], cwd=ROOT, check=True)
            capture_checkpoint(directory, snapshots, name, turn, command, process.returncode)
            previous = turn


def compare(args):
    """compare checks every checkpoint and reports missing or differing saves."""
    cli = args.cli.resolve()
    manifests = [json.loads((p / "run.json").read_text()) for p in (args.left, args.right)]
    if any(m["seed"] != manifests[0]["seed"] or m["fixtures"] != manifests[0]["fixtures"]
           or m["race_sha256"] != manifests[0]["race_sha256"] for m in manifests):
        raise ValueError("runs have different seeds or fixtures")
    results = []
    for name in args.scenario or SCENARIOS:
        for turn in CHECKPOINTS:
            if turn > args.through:
                break
            dirs = [p / name / "checkpoints" / f"{turn:03}" for p in (args.left, args.right)]
            if any(not d.is_dir() for d in dirs):
                results.append({"scenario": name, "turn": turn, "error": "missing checkpoint"})
                continue
            if turn == 0 and any(json.loads((d / "checkpoint.json").read_text()).get("origin") == "baseline" for d in dirs):
                results.append({"scenario": name, "turn": turn, "skipped": "reference input; creation not tested"})
                continue
            results.extend(compare_saves(cli, dirs, {"scenario": name, "turn": turn}))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(results, indent=2) + "\n")
    return summarize(results, args.report)


def export(args):
    """export copies completed scenarios into a baseline fixture directory.

    Only the named scenarios are written; others already in the baseline are
    kept, so unchanged scenarios don't churn with native stale bytes.
    """
    work = args.work.resolve()
    dest = args.dest.resolve()
    manifest = json.loads((work / "run.json").read_text())
    names = args.scenario or SCENARIOS
    for name in names:
        snapshots = work / name / "checkpoints"
        missing = [t for t in CHECKPOINTS if not (snapshots / f"{t:03}" / "checkpoint.json").is_file()]
        if missing:
            raise ValueError(f"{name}: missing checkpoints {missing}; run through 150 first")
    baseline = {}
    if (dest / "run.json").is_file():
        baseline = json.loads((dest / "run.json").read_text())
        if any(baseline[key] != manifest[key] for key in ("seed", "fixtures", "race_sha256")):
            raise ValueError("baseline seed or fixtures differ from this run")
    scenarios = dict(baseline.get("scenarios", {}))
    present = [name for name in names if name in scenarios or (dest / name).exists()]
    if present and not args.replace:
        raise ValueError(f"baseline already has {', '.join(present)}; pass --replace to regenerate")
    for name in names:
        if (dest / name).exists():
            shutil.rmtree(dest / name)
        for turn in CHECKPOINTS:
            source = work / name / "checkpoints" / f"{turn:03}"
            saved = json.loads((source / "checkpoint.json").read_text())
            checked_files(source, saved["files"], "checkpoint file")
            target = dest / name / "checkpoints" / f"{turn:03}"
            target.mkdir(parents=True)
            copy_files(source, target, saved["files"])
            saved.pop("command", None)
            saved.pop("source_checkpoint", None)
            (target / "checkpoint.json").write_text(json.dumps(saved, indent=2) + "\n")
        scenarios[name] = manifest["exe_sha256"]
    keep = {key: manifest[key] for key in ("engine", "seed", "fixtures", "race_sha256")}
    # Executable that produced each scenario's checkpoints.
    keep["scenarios"] = {name: scenarios[name] for name in SCENARIOS if name in scenarios}
    (dest / "run.json").write_text(json.dumps(keep, indent=2) + "\n")
    print(f"Exported {', '.join(names)} to {dest}")


def summarize(results, report):
    """summarize prints result counts and reports whether any comparison failed."""
    failures = sum(not r.get("match", False) and "skipped" not in r for r in results)
    matches = sum(r.get("match", False) for r in results)
    warnings = sum(r.get("warning", False) for r in results)
    skipped = sum("skipped" in r for r in results)
    print(f"{matches} matches ({warnings} with warnings); {failures} differences/errors; "
          f"{skipped} reference inputs skipped. Report: {report}")
    return bool(failures)


def compare_saves(cli, dirs, base):
    """compare_saves runs save compare on every save in either directory."""
    files = [{p.name.lower(): p for p in d.iterdir() if SAVE_NAME.fullmatch(p.name)} for d in dirs]
    results = []
    for filename in sorted(files[0].keys() | files[1].keys() | {"game.xy", "game.hst"}):
        result = dict(base, file=filename)
        if any(filename not in f for f in files):
            result["error"] = "missing file"
        else:
            proc = subprocess.run([str(cli), "save", "compare", str(files[0][filename]), str(files[1][filename])],
                                  cwd=ROOT, text=True, capture_output=True)
            result["match"] = proc.returncode == 0
            result["detail"] = proc.stdout + proc.stderr
            if result["match"] and proc.stdout.startswith("MATCH with warnings"):
                # Unused storage holds stale native heap and stack bytes that
                # change between runs; keep the report stable and leave the
                # values to save compare.
                result["warning"] = True
                result["detail"] = proc.stdout.splitlines()[0] + "\n"
        results.append(result)
    return results


def load_work(path):
    """load_work reads a prepared run and verifies its executable is unchanged."""
    work = path.resolve()
    manifest = json.loads((work / "run.json").read_text())
    if digest(Path(manifest["exe"])) != manifest["exe_sha256"]:
        raise ValueError(f"executable changed since prepare; prepare a fresh run: {work}")
    return work, manifest


def generate(work, manifest, scenario, source, turns, timeout, trace=False):
    """generate runs one launch of turns from source saves and returns the output directory.

    Output goes to <work>/<scenario>/xfeed/<from>_<to>. An existing output is
    reused only when it came from the same input files and executable.
    """
    saved = source / "checkpoint.json"
    if saved.is_file():
        checked_files(source, json.loads(saved.read_text())["files"], "input checkpoint file")
    start = host_turn(source / "game.hst")
    end = start + turns
    directory = work / scenario / "xfeed" / f"{start:03}_{end:03}"
    inputs = sorted(p.name.lower() for p in source.iterdir() if SAVE_NAME.fullmatch(p.name))
    input_files = {f: digest(source / f) for f in inputs}
    if directory.exists():
        previous = json.loads((directory / "crossfeed.json").read_text()) if (directory / "crossfeed.json").is_file() else {}
        if (previous.get("input_files") != input_files or previous.get("exe_sha256") != manifest["exe_sha256"]
                or (trace and not (directory / "trace.log").is_file())):
            raise ValueError(f"output exists from different input, executable, or without a trace: {directory}; remove it to rerun")
        print(f"{scenario}: reusing {directory}", flush=True)
        return directory
    directory.mkdir(parents=True)
    copy_files(source, directory, inputs)
    command, cwd = launch_command(Path(manifest["exe"]), manifest["seed"], directory, end, start)
    env = dict(os.environ)
    if trace:
        env["STARS_TRACE"] = game_path(Path(manifest["exe"]), directory, "trace.log")
    print(f"{scenario}: turn {start} -> {end} from {source}: {' '.join(command)}", flush=True)
    with (directory / "run.log").open("w") as log:
        process = subprocess.run(command, cwd=cwd, stdout=log, stderr=subprocess.STDOUT,
                                 check=False, timeout=timeout, env=env)
    if process.returncode != 1:
        raise ValueError(f"exit {process.returncode}, expected 1; see {directory / 'run.log'}")
    if host_turn(directory / "game.hst") != end:
        raise ValueError(f"expected turn {end} in {directory / 'game.hst'}")
    if trace and not (directory / "trace.log").is_file():
        raise ValueError(f"no trace.log written; was {manifest['exe']} built with -DSTARS_TEST_TRACE=ON?")
    report = {"exe_sha256": manifest["exe_sha256"], "input": str(source),
              "input_files": input_files, "turn": end, "command": command,
              "files": {p.name.lower(): digest(p) for p in directory.iterdir() if SAVE_NAME.fullmatch(p.name)}}
    (directory / "crossfeed.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"Saved {directory}", flush=True)
    return directory


def crossfeed(args):
    """crossfeed generates turns from another run's saves with this run's executable.

    Feeding another build's checkpoint to this build tests turn-generation
    logic on identical input, separating a logic difference from inherited
    state.
    """
    work, manifest = load_work(args.work)
    directory = generate(work, manifest, args.scenario, args.input.resolve(), args.turns, args.timeout, args.trace)
    if not args.expect:
        return False
    expect = args.expect.resolve()
    end = host_turn(directory / "game.hst")
    if host_turn(expect / "game.hst") != end:
        raise ValueError(f"expected saves are not at turn {end}: {expect}")
    results = compare_saves(args.cli.resolve(), [expect, directory], {"scenario": args.scenario, "turn": end})
    report_path = args.report or directory / "comparison.json"
    report_path.write_text(json.dumps(results, indent=2) + "\n")
    return summarize(results, report_path)


def bisect(args):
    """bisect finds the first turn where two runs' executables diverge.

    Both run -gK from the same input. The startup seed resets only at launch,
    so -gK reproduces the first K turns of a longer launch; a binary search
    over K needs about log2(turns) launches per executable.
    """
    cli = args.cli.resolve()
    reference, reference_manifest = load_work(args.reference)
    native, native_manifest = load_work(args.native)
    source = args.input.resolve()
    start = host_turn(source / "game.hst")

    def diverges(k):
        """diverges reports whether the two outputs differ after k turns."""
        expect = generate(reference, reference_manifest, args.scenario, source, k, args.timeout)
        actual = generate(native, native_manifest, args.scenario, source, k, args.timeout, args.trace)
        results = compare_saves(cli, [expect, actual], {"scenario": args.scenario, "turn": start + k})
        (actual / "comparison.json").write_text(json.dumps(results, indent=2) + "\n")
        failed = any(not r.get("match", False) for r in results)
        print(f"{args.scenario}: after {k} turns (turn {start + k}): {'DIFFERS' if failed else 'matches'}", flush=True)
        return failed

    if not diverges(args.turns):
        print(f"{args.scenario}: no divergence through turn {start + args.turns}")
        return False
    low, high = 0, args.turns
    while high - low > 1:
        mid = (low + high) // 2
        if diverges(mid):
            high = mid
        else:
            low = mid
    first = native / args.scenario / "xfeed" / f"{start:03}_{start + high:03}"
    print(f"{args.scenario}: first divergent turn {start + high} (generated from turn {start + high - 1}); "
          f"report: {first / 'comparison.json'}")
    return True


def trace(args):
    """trace annotates a regression trace with the source line of each caller."""
    exe = args.exe.resolve()
    headers = subprocess.run(["x86_64-w64-mingw32-objdump", "-p", str(exe)], text=True, capture_output=True, check=True).stdout
    base = int(re.search(r"^ImageBase\s+([0-9a-fA-F]+)", headers, re.M).group(1), 16)
    labels = {"R": "Random({0})={1}", "P": "PctPlanetCapacity(planet {0})={1}"}
    rows = []
    for index, line in enumerate(args.file.read_text().splitlines()):
        tag, turn, player, ai, arg, result, rva = line.split()[:7]
        if (args.turn is None or int(turn) == args.turn) and (args.player is None or int(player) == args.player):
            rows.append((index, turn, player, ai, labels[tag].format(arg, result), int(rva, 16)))
    # A return address follows its call; one byte earlier lies within the call.
    addresses = sorted({row[-1] for row in rows})
    lookup = subprocess.run(["x86_64-w64-mingw32-addr2line", "-f", "-e", str(exe)] + [hex(base + a - 1) for a in addresses],
                            text=True, capture_output=True, check=True).stdout.splitlines()
    names = {a: f"{lookup[2 * i]} {Path(lookup[2 * i + 1].split(' ')[0]).name}" for i, a in enumerate(addresses)}
    for index, turn, player, ai, call, rva in rows:
        print(f"{index} turn={turn} player={player} ai={ai} {call} {names[rva]}")
    return False


def main():
    """main dispatches regression staging, execution, and comparison."""
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="action", required=True)
    stage = sub.add_parser("prepare")
    stage.add_argument("--work", type=Path, required=True)
    stage.add_argument("--exe", type=Path, required=True)
    stage.add_argument("--seed", type=int, default=12345)
    execute = sub.add_parser("run")
    execute.add_argument("--work", type=Path, required=True)
    execute.add_argument("--scenario", choices=SCENARIOS, action="append")
    execute.add_argument("--through", type=int, choices=CHECKPOINTS, default=150)
    execute.add_argument("--resume", action="store_true", help="restore the latest checkpoint and continue; preserve current saves")
    execute.add_argument("--baseline", type=Path, help="start from this run's creation checkpoint to test turn generation separately")
    execute.add_argument("--timeout", type=int, default=900, help="seconds per game launch")
    execute.add_argument("--cli", type=Path, default=SAVE_CLI, help="stars-save CLI used to update the human player")
    diff = sub.add_parser("compare")
    diff.add_argument("left", type=Path)
    diff.add_argument("right", type=Path)
    diff.add_argument("--scenario", choices=SCENARIOS, action="append")
    diff.add_argument("--through", type=int, choices=CHECKPOINTS, default=150)
    diff.add_argument("--cli", type=Path, default=SAVE_CLI)
    diff.add_argument("--report", type=Path, default=ROOT / "tests/scaffold/fixtures/regression/regression-comparison.json")
    save = sub.add_parser("export", help="store a completed run as a checked-in baseline fixture")
    save.add_argument("--work", type=Path, required=True, help="completed run to export")
    save.add_argument("--dest", type=Path, default=ROOT / "tests/scaffold/fixtures/regression/native")
    save.add_argument("--scenario", choices=SCENARIOS, action="append")
    save.add_argument("--replace", action="store_true", help="replace an existing baseline")
    feed = sub.add_parser("crossfeed", help="generate turns from another run's saves with this run's executable")
    feed.add_argument("--work", type=Path, required=True, help="prepared run whose executable generates")
    feed.add_argument("--scenario", choices=SCENARIOS, required=True, help="scenario directory to hold the output")
    feed.add_argument("--input", type=Path, required=True, help="directory of input saves, such as a checkpoint")
    feed.add_argument("--turns", type=int, required=True, help="turns to generate in one launch (-gN)")
    feed.add_argument("--expect", type=Path, help="directory of saves to compare the generated turn against")
    feed.add_argument("--timeout", type=int, default=900, help="seconds for the game launch")
    feed.add_argument("--cli", type=Path, default=SAVE_CLI)
    feed.add_argument("--report", type=Path, help="comparison report (default: comparison.json in the output)")
    feed.add_argument("--trace", action="store_true", help="write trace.log (native built with -DSTARS_TEST_TRACE=ON)")
    search = sub.add_parser("bisect", help="find the first turn where two runs' executables diverge")
    search.add_argument("--reference", type=Path, required=True, help="prepared run of the reference build")
    search.add_argument("--native", type=Path, required=True, help="prepared run of the build under test")
    search.add_argument("--scenario", choices=SCENARIOS, required=True)
    search.add_argument("--input", type=Path, required=True, help="directory of saves both runs start from")
    search.add_argument("--turns", type=int, required=True, help="the launch span to search (-gN)")
    search.add_argument("--timeout", type=int, default=900, help="seconds per game launch")
    search.add_argument("--cli", type=Path, default=SAVE_CLI)
    search.add_argument("--trace", action="store_true", help="write trace.log for native launches")
    annotate = sub.add_parser("trace", help="annotate a trace.log with caller source lines")
    annotate.add_argument("file", type=Path)
    annotate.add_argument("--exe", type=Path, required=True, help="the traced native executable")
    annotate.add_argument("--turn", type=int)
    annotate.add_argument("--player", type=int, help="zero-based player index")
    args = parser.parse_args()
    if args.action in ("crossfeed", "bisect") and args.turns < 1:
        parser.error("--turns must be at least 1")
    try:
        return {"prepare": prepare, "run": run, "compare": compare, "export": export, "crossfeed": crossfeed,
                "bisect": bisect, "trace": trace}[args.action](args) or 0
    except (ValueError, OSError, subprocess.SubprocessError, struct.error) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
