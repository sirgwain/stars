#!/usr/bin/env python3
"""Compile positive/negative controls for the software-floating-point audit.

Run with the compiler and objdump used by the audited build. The CI workflow
uses MinGW, including COFF imported symbols and data-only function pointers.
"""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--cc", default="gcc")
parser.add_argument("--objdump", default="objdump")
parser.add_argument("--work", type=Path, default=ROOT / "dist/scaffold/software-float-audit",
                    help="Directory for temporary compiled controls")
args, remaining = parser.parse_known_args()


class SoftwareFloatAuditTests(unittest.TestCase):
    def audit(self, source):
        work = args.work.resolve()
        work.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="stars-fp-audit-", dir=work) as temporary:
            build = Path(temporary)
            self.assertEqual(build.resolve().parent, work)
            directory = build / "CMakeFiles/stars_core.dir"
            directory.mkdir(parents=True)
            source_file = build / "probe.c"
            source_file.write_text(source, encoding="utf-8")
            # Disabling builtins forces an external call, isolating the hole
            # where the old opcode-only audit incorrectly reported success.
            subprocess.run([args.cc, "-std=gnu11", "-O2", "-fno-builtin", "-c", str(source_file),
                            "-o", str(directory / "probe.c.obj")], check=True, capture_output=True, text=True)
            return subprocess.run([sys.executable, str(ROOT / "tests/scaffold/check_software_float.py"),
                                   "--build", str(build), "--objdump", args.objdump],
                                  capture_output=True, text=True)

    def test_native_calls(self):
        for function, binary in (("pow", True), ("atan2", True), ("hypot", True),
                                 ("sin", False), ("cos", False), ("sqrt", False),
                                 ("floor", False), ("exp", False), ("log", False)):
            for suffix, ctype in (("", "double"), ("f", "float"), ("l", "long double")):
                name = function + suffix
                with self.subTest(function=name):
                    call = f"{name}(x, y)" if binary else f"{name}(x)"
                    result = self.audit(f"#include <math.h>\n{ctype} Probe({ctype} x, {ctype} y) {{ return {call}; }}\n")
                    self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
                    self.assertIn("native math reference " + name, result.stdout)

    def test_function_pointer_in_data(self):
        result = self.audit("#include <math.h>\ndouble (*pfnNativePow)(double, double) = pow;\n")
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("native math reference pow", result.stdout)
        self.assertIn("0 native floating instructions", result.stdout)

    def test_imported_call(self):
        predefined = subprocess.check_output([args.cc, "-dM", "-E", "-x", "c", "-"], input="", text=True)
        if "#define _WIN32 " not in predefined:
            self.skipTest("dllimport control requires a Windows target")
        result = self.audit("__declspec(dllimport) double pow(double, double);\n"
                            "double Probe(double x, double y) { return pow(x, y); }\n")
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertRegex(result.stdout, r"native math reference __imp__?pow\b")
        self.assertIn("0 native floating instructions", result.stdout)

    def test_software_calls_and_bit_carriers(self):
        result = self.audit("extern double Sf64Pow(double, double);\n"
                            "double Probe(double x, double y) { return Sf64Pow(x, y); }\n"
                            "double Carry(double x) { return x; }\n"
                            "unsigned int Bits(unsigned int u) { return u ^ 0x80000000u; }\n")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("0 native math references", result.stdout)


if __name__ == "__main__":
    unittest.main(argv=[str(Path(__file__).name), *remaining])
