#!/usr/bin/env python3
"""Reject native x86 floating arithmetic and libm references in game/UI objects.

Native float/double registers may carry bits (moves are allowed), but math,
comparisons and numeric conversions must call the software implementation.
This checks the game objects, not CRT libraries or test-only native oracles.
"""
import argparse
from pathlib import Path
import re
import subprocess

# Include the C math families, not only the seven functions used before the
# migration. Float/long-double variants and platform decorations are handled
# below; software helpers such as Sf64Pow must remain distinct.
MATH_FUNCTIONS = frozenset("""
acos acosh asin asinh atan atan2 atanh cbrt ceil copysign cos cosh erf erfc
exp exp2 expm1 fabs fdim floor fma fmax fmin fmod frexp hypot ilogb ldexp
lgamma llrint llround log log10 log1p log2 logb lrint lround modf nan
nearbyint nextafter nexttoward pow remainder remquo rint round scalbln
scalbn sin sinh sqrt tan tanh tgamma trunc
""".split())


def native_math_symbol(symbol):
    # COFF imports can be __imp_pow (x64) or __imp__pow (x86). ELF symbol
    # versions and x86 stdcall suffixes both start at '@'. GCC also uses
    # __pow_finite under some optimization modes; older CRTs expose _CIpow.
    name = symbol.split("@", 1)[0]
    if name.startswith("__imp_"):
        name = name[len("__imp_"):]
    name = name.lstrip("_")
    if name.startswith("CI"):
        name = name[2:]
    if name.endswith("_finite"):
        name = name[:-len("_finite")]
    return any(name == base + suffix for base in MATH_FUNCTIONS for suffix in ("", "f", "l"))


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--build", type=Path, required=True)
parser.add_argument("--objdump", default="objdump")
args = parser.parse_args()
objects = sorted(p for layer in ("core", "ui")
                 for p in (args.build / "CMakeFiles" / f"stars_{layer}.dir").rglob("*")
                 if p.suffix in (".o", ".obj"))
if not objects:
    raise SystemExit("No compiled game objects found")
bad = []
references = []
for obj in objects:
    # Undefined symbols cover direct/tail calls and addresses stored in data,
    # which have no floating opcode and can be outside the disassembled text.
    data = subprocess.check_output([args.objdump, "-dt", str(obj)], text=True)
    if not re.search(r"file format (?:pe|elf)(?:i)?(?:-x86-64|64-x86-64|32-i386)", data):
        raise SystemExit(f"Unsupported architecture (this audit is for x86): {obj}")
    for line in data.splitlines():
        # GNU objdump uses *UND* for ELF and section zero for COFF symbols.
        if "*UND*" in line or re.match(r"\[\s*\d+\]\(sec\s+0\)", line):
            symbol = line.split()[-1]
            if native_math_symbol(symbol):
                references.append(f"{obj}: native math reference {symbol}")
        match = re.match(r"\s*[0-9a-f]+:\s+(?:[0-9a-f]{2}\s+)+\s*([a-z0-9]+)\b", line)
        if not match:
            continue
        op = match[1]
        if (re.fullmatch(r"v?(?:(?:add|sub|mul|div|sqrt|min|max|round)(?:ss|sd|ps|pd)|"
                         r"(?:u?comi)(?:ss|sd)|cvt\w+|fm(?:add|sub)\w+)", op)
                or re.match(r"f(?:ld|st|add|sub|mul|div|com|ucom|ist|ild|sqrt|sin|cos|ptan|patan|scale|rndint|2xm1|yl2x)", op)):
            bad.append(f"{obj}: {line.strip()}")
print(f"{len(objects)} game/UI objects; {len(bad)} native floating instructions; "
      f"{len(references)} native math references")
if bad or references:
    print("\n".join(bad + references))
    raise SystemExit(1)
