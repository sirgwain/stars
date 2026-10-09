"""Platform routing and launcher lifecycle checks; no Wine or desktop required."""
import importlib.util
import ctypes
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import Mock, patch


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


HERE = Path(__file__).resolve().parent
regression = load("regression", HERE / "regression.py")
tutorial = load("tutorial_runner", HERE / "tutorial/run.py")


class RegressionTests(unittest.TestCase):
    def test_platform_commands_and_paths(self):
        directory = HERE / "example"
        for platform, executable, wine in (("win32", "stars.exe", False),
                                            ("win32", "stars-host.exe", False),
                                            ("linux", "stars.exe", True),
                                            ("darwin", "stars.exe", True),
                                            ("linux", "stars-host", False)):
            with self.subTest(platform=platform, executable=executable), patch.object(sys, "platform", platform):
                command, cwd = regression.launch_command(executable, 12345, directory, 10, 1)
                self.assertEqual(command[:1], ["wine"] if wine else [executable])
                self.assertEqual(command[-2], "-g9")
                expected = regression.windows_path(directory) + "\\game.hst" if wine else str(directory / "game.hst")
                self.assertEqual(command[-1], expected)
                self.assertEqual(cwd, directory)


    def test_trace_environment_uses_platform_path(self):
        for platform, executable, wine in (("win32", "stars.exe", False),
                                            ("win32", "stars-host.exe", False),
                                            ("linux", "stars.exe", True),
                                            ("darwin", "stars.exe", True),
                                            ("linux", "stars-host", False)):
            with self.subTest(platform=platform, executable=executable), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary).resolve()
                source = root / "source"
                source.mkdir()
                (source / "game.hst").write_bytes(b"input host")
                manifest = {"exe": executable, "exe_sha256": "test", "seed": 12345}

                def launch(command, **kwargs):
                    directory = kwargs["cwd"]
                    expected = ("Z:" + str(directory).replace("/", "\\") + "\\trace.log"
                                if wine else str(directory / "trace.log"))
                    self.assertEqual(kwargs["env"]["STARS_TRACE"], expected)
                    (directory / "trace.log").write_text("trace output")
                    return Mock(returncode=1)

                with patch.object(sys, "platform", platform), \
                     patch.object(regression, "host_turn", side_effect=[0, 1]), \
                     patch.object(regression.subprocess, "run", side_effect=launch) as run:
                    directory = regression.generate(root, manifest, "noai", source, 1, 30, trace=True)
                run.assert_called_once()
                self.assertTrue((directory / "crossfeed.json").is_file())


class TutorialTests(unittest.TestCase):
    def test_cleanup_checks_executable_before_terminating(self):
        with tempfile.TemporaryDirectory() as temporary:
            run = Path(temporary)
            executable = run / "stars.exe"
            (run / "game.pid").write_text("1234")
            for matches in (False, True):
                with self.subTest(matches=matches):
                    kernel = Mock()
                    kernel.OpenProcess.return_value = 42

                    def query(handle, flags, name, length):
                        name.value = str(executable.resolve() if matches else run / "unrelated.exe")
                        return True

                    kernel.QueryFullProcessImageNameW.side_effect = query
                    with patch.object(ctypes, "WinDLL", create=True, return_value=kernel):
                        tutorial.stop_windows_game(run, executable)
                    if matches:
                        kernel.TerminateProcess.assert_called_once_with(42, 1)
                    else:
                        kernel.TerminateProcess.assert_not_called()
                    kernel.CloseHandle.assert_called_once_with(42)

    def run_launcher(self, platform, report=True, keep_failure=False):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary).resolve()
            exe = root / "stars.exe"
            exe.write_bytes(b"test executable")
            prefix = root / "prefix"
            (prefix / "drive_c/windows").mkdir(parents=True)
            work = root / "runs"
            commands = []

            def launch(command, **kwargs):
                commands.append((command, kwargs))
                run = Path(command[5 if platform != "win32" else 4])
                if report:
                    (run / "result.json").write_text(json.dumps({"status": "failed" if keep_failure else "passed"}))
                return Mock(poll=Mock(return_value=0))

            def output(command, **kwargs):
                return str(command[-1]) if command[0] == "winepath" else "wine-test"

            argv = ["run.py", "--exe", str(exe), "--ahk", str(exe), "--work", str(work)]
            if keep_failure:
                argv.append("--keep-game-on-failure")
            with patch.object(sys, "argv", argv), patch.object(sys, "platform", platform), \
                 patch.object(tutorial, "generate_catalog"), \
                 patch.object(tutorial.shutil, "which", return_value="tool"), \
                 patch.object(tutorial.tempfile, "mkdtemp", return_value=str(prefix)), \
                 patch.object(tutorial.subprocess, "Popen", side_effect=launch), \
                 patch.object(tutorial.subprocess, "check_output", side_effect=output), \
                 patch.object(tutorial.subprocess, "run") as run_command, \
                 patch.object(tutorial, "stop_windows_game") as stop:
                status = tutorial.main()
                self.assertEqual(status, 0 if report and not keep_failure else 1)
                run = next(work.iterdir())
                self.assertTrue((run / "game/Stars.ini").is_file())
                metadata = json.loads((run / "metadata.json").read_text())
                self.assertEqual(metadata["platform"], "windows" if platform == "win32" else "wine")
                if platform == "win32":
                    run_command.assert_not_called()
                    if keep_failure:
                        stop.assert_not_called()
                    else:
                        stop.assert_called_once()
                    self.assertIsNone(metadata["wine_prefix"])
                    self.assertEqual(commands[0][0][0], str(exe))
                else:
                    stop.assert_not_called()
                    self.assertEqual(commands[0][0][0], "wine")
                    self.assertEqual(commands[0][1]["env"]["WINEPREFIX"], str(prefix))
                    self.assertIn(["wineboot", "-u"], [call.args[0] for call in run_command.call_args_list])
                    self.assertIn(["wineserver", "-k"], [call.args[0] for call in run_command.call_args_list])

    def test_windows(self):
        self.run_launcher("win32")

    def test_wine(self):
        self.run_launcher("linux")

    def test_windows_missing_result_fails_and_cleans_up(self):
        self.run_launcher("win32", report=False)

    def test_windows_explicitly_retained_failure(self):
        self.run_launcher("win32", keep_failure=True)


if __name__ == "__main__":
    unittest.main()
