#!/usr/bin/env python3
"""Collect GCC coverage for the floating-point function inventory, including UI.

Run the desired workload first in a fresh --coverage build. Counts are for
whole functions, not just floating operations; execution is not an assertion.
"""

import argparse
import gzip
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def collect(build, gcov, inventory):
    result = []
    files = {}
    for entry in inventory:
        files.setdefault(entry["file"], []).append(entry)
    with tempfile.TemporaryDirectory(prefix="stars-gcov-", dir=build) as temporary:
        if build not in Path(temporary).resolve().parents:
            raise ValueError("gcov temporary directory escaped the build")
        for filename, entries in sorted(files.items()):
            layer = entries[0]["layer"]
            directory = build / "CMakeFiles" / f"stars_{layer}.dir"
            objects = [directory / (filename + suffix) for suffix in (".obj", ".o")]
            obj = next((p for p in objects if p.is_file()), None)
            if obj is None:
                raise ValueError(f"Missing instrumented object for {filename}: {directory}")
            process = subprocess.run([gcov, "--json-format", "-b", "-c", obj.as_posix()],
                                     cwd=temporary, text=True, capture_output=True, check=True)
            if "cannot open" in process.stderr or "mismatch" in process.stderr:
                raise ValueError(process.stderr.strip())
            archive = Path(temporary) / (filename + ".gcov.json.gz")
            with gzip.open(archive, "rt") as stream:
                data = json.load(stream)
            source = next(f for f in data["files"] if Path(f["file"]).name == filename)
            text = (ROOT / filename).read_text()
            text = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',
                          lambda m: "\n" * m[0].count("\n"), text, flags=re.S)
            source_lines = text.splitlines()
            functions = {f["name"]: f for f in source["functions"]}
            for entry in entries:
                function = functions[entry["name"]]
                lines = [line for line in source["lines"]
                         if line.get("function_name") == entry["name"]]
                branches = [branch for line in lines for branch in line.get("branches", [])]
                body = "\n".join(source_lines[function["start_line"] - 1:function["end_line"]])
                variables = re.findall(r"\b(?:float|double)\s+\**\s*(\w+)", body)
                anchors = re.compile(r"\b(?:Sf(?:32|64|80)\w+|float|double|sqrt|pow|atan2|sin|cos|hypot|floor|DGetDistance|CalcPctSurvive"
                                     + "".join("|" + re.escape(v) for v in variables) + r")\b|\d+\.\d+")
                fp_lines = [line for line in lines if anchors.search(source_lines[line["line_number"] - 1])]
                result.append(dict(entry, start_line=function["start_line"],
                                   calls=function["execution_count"],
                                   lines=len(lines), lines_hit=sum(line["count"] > 0 for line in lines),
                                   branches=len(branches), branches_hit=sum(b["count"] > 0 for b in branches),
                                   fp_anchor_lines=len(fp_lines), fp_anchor_lines_hit=sum(line["count"] > 0 for line in fp_lines),
                                   missed_fp_anchors=[line["line_number"] for line in fp_lines if line["count"] == 0],
                                   missed_lines=[line["line_number"] for line in lines if line["count"] == 0]))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--gcov", default="gcov")
    parser.add_argument("--layer", choices=("all", "core", "ui"), default="all")
    parser.add_argument("--label", required=True, help="Workload actually run, not an assumed coverage level")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--require-all-fp", action="store_true",
                        help="Fail if an inventoried function or floating-point source anchor was not reached")
    args = parser.parse_args()
    inventory = json.loads((ROOT / "tests/scaffold/floating-functions.json").read_text())
    if args.layer != "all":
        inventory = [entry for entry in inventory if entry["layer"] == args.layer]
    functions = collect(args.build.resolve(), args.gcov, inventory)
    result = {"label": args.label, "build": str(args.build.resolve()),
              "gcov": subprocess.check_output([args.gcov, "--version"], text=True).splitlines()[0],
              "functions": functions}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    for layer in ("core", "ui"):
        selected = [f for f in functions if f["layer"] == layer]
        print(f"{layer}: {sum(f['calls'] > 0 for f in selected)}/{len(selected)} functions reached; "
              f"{sum(f['lines_hit'] for f in selected)}/{sum(f['lines'] for f in selected)} lines; "
              f"{sum(f['branches_hit'] for f in selected)}/{sum(f['branches'] for f in selected)} branch outcomes")
    if args.require_all_fp:
        missed = [f for f in functions if not f["calls"] or f["missed_fp_anchors"]]
        for function in missed:
            print(f"MISS {function['file']}:{function['name']}: "
                  f"calls={function['calls']}, anchors={function['missed_fp_anchors']}")
        if missed:
            raise SystemExit(1)


if __name__ == "__main__":
    main()
