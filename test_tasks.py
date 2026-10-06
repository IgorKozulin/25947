"""Behavior checks for the first seven OS assignments (run on Linux)."""

import pathlib
import subprocess
import tempfile
import unittest


HERE = pathlib.Path(__file__).resolve().parent
SOURCES = {n: f"task{n}/main.c" for n in range(1, 8)}


class AssignmentTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.build = tempfile.TemporaryDirectory()
        cls.executables = {}
        for number, source in SOURCES.items():
            target = pathlib.Path(cls.build.name) / f"task{number}"
            subprocess.run(
                ["cc", "-std=c99", "-D_XOPEN_SOURCE=700", "-Wall", "-Wextra", "-Werror", str(HERE / source), "-o", str(target)],
                check=True,
                capture_output=True,
                text=True,
            )
            cls.executables[number] = target

    @classmethod
    def tearDownClass(cls):
        cls.build.cleanup()

    def run_task(self, number, *args, input=None, timeout=10, env=None):
        return subprocess.run(
            [str(self.executables[number]), *map(str, args)],
            input=input,
            capture_output=True,
            text=True,
            timeout=timeout,
            env=env,
        )

    def test_task1_process_options_in_reverse_order(self):
        result = self.run_task(1, "-p", "-i")
        self.assertEqual(result.returncode, 0)
        self.assertLess(result.stdout.index("UID:"), result.stdout.index("PID:"))

    def test_task1_accepts_many_combined_options(self):
        result = self.run_task(1, "-iiiiiiiiii")
        self.assertEqual(result.returncode, 0)
        self.assertEqual(sum(line.startswith("UID:") for line in result.stdout.splitlines()), 10)

    def test_task2_california_winter_and_summer(self):
        for utc, expected in [("2026-01-01T20:00:00Z", "2026-01-01 12:00:00 PST"),
                              ("2026-07-01T20:00:00Z", "2026-07-01 13:00:00 PDT")]:
            with self.subTest(utc=utc):
                result = self.run_task(2, utc)
                self.assertEqual(result.returncode, 0)
                self.assertEqual(result.stdout.strip(), expected)

    def test_task3_opens_owner_only_file_in_both_phases(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "private.txt"
            path.write_text("secret\n")
            path.chmod(0o600)
            result = self.run_task(3, path)
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout.count("open: success"), 2)

    def test_task4_preserves_lines_and_stops_on_leading_dot(self):
        result = self.run_task(4, input="alpha\n\nbeta\n.stop\ngamma\n")
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout, "alpha\n\nbeta\n")

    def test_task5_looks_up_lines_including_unterminated_last_line(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "lines.txt"
            path.write_bytes(b"alpha\n\nbeta")
            result = self.run_task(5, path, input="1\n2\n3\n4\n0\n")
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout, "Line number (0 to quit): alpha\n"
                                        "Line number (0 to quit): \n"
                                        "Line number (0 to quit): beta\n"
                                        "Line number (0 to quit): "
                                        "Line number (0 to quit): ")
        self.assertIn("No such line", result.stderr)

    def test_task5_and_7_accept_empty_file(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "empty.txt"
            path.write_bytes(b"")
            for number in (5, 7):
                with self.subTest(task=number):
                    result = self.run_task(number, path, input="1\n0\n")
                    self.assertEqual(result.returncode, 0)
                    self.assertIn("No such line", result.stderr)

    def test_task6_timeout_prints_entire_file(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "lines.txt"
            path.write_text("first\nsecond\n")
            process = subprocess.Popen([str(self.executables[6]), str(path)], stdin=subprocess.PIPE,
                                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            try:
                process.wait(timeout=8)
                out, err = process.communicate()
            finally:
                if process.poll() is None:
                    process.kill()
                    process.communicate()
        self.assertEqual(process.returncode, 0, err)
        self.assertIn("first\nsecond\n", out)

    def test_task7_mmap_lookup_and_timeout(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "lines.txt"
            path.write_bytes(b"one\ntwo")
            quick = self.run_task(7, path, input="2\n0\n")
            self.assertEqual(quick.returncode, 0)
            self.assertEqual(quick.stdout, "Line number (0 to quit): two\n"
                                           "Line number (0 to quit): ")
            process = subprocess.Popen([str(self.executables[7]), str(path)], stdin=subprocess.PIPE,
                                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            try:
                process.wait(timeout=8)
                out, err = process.communicate()
            finally:
                if process.poll() is None:
                    process.kill()
                    process.communicate()
        self.assertEqual(process.returncode, 0, err)
        self.assertIn("one\ntwo", out)


if __name__ == "__main__":
    unittest.main()
