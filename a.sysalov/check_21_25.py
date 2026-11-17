#!/usr/bin/env python3
import os
from pathlib import Path
import select
import signal
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent


def read_until(process, expected):
    output = b''
    while not output.endswith(expected):
        ready, _, _ = select.select([process.stdout], [], [], 5)
        assert ready, (expected, output)
        part = os.read(process.stdout.fileno(), 1)
        assert part, (expected, output)
        output += part
    return output


def main():
    with tempfile.TemporaryDirectory(prefix='sysalov-21-25-') as temporary:
        programs = {}
        for number in (21, 25):
            source, = (ROOT / f'task_{number}').glob('*.c')
            programs[number] = Path(temporary) / source.stem
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                            str(source), '-o', str(programs[number])], check=True)

        for count in (0, 4):
            process = subprocess.Popen([str(programs[21])], stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE)
            try:
                read_until(process, b'print count and exit.\n')
                for _ in range(count):
                    process.send_signal(signal.SIGINT)
                    assert read_until(process, b'\a') == b'\a'
                process.send_signal(signal.SIGQUIT)
                output, error = process.communicate(timeout=5)
                assert process.returncode == 0 and not error
                assert output == f'\nBeep count: {count}\n'.encode()
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait()

        for data in (b'', b'Hello World!\nabcDEF 123\n\nlast',
                     bytes(range(256)) * 4096):
            result = subprocess.run([str(programs[25])], input=data,
                                    capture_output=True, timeout=5)
            assert result.returncode == 0 and not result.stderr
            assert result.stdout == data.upper()

        fd = os.open(temporary, os.O_RDONLY)
        try:
            result = subprocess.run([str(programs[25])], stdin=fd,
                                    capture_output=True, timeout=5)
            assert result.returncode != 0 and b'read:' in result.stderr
        finally:
            os.close(fd)
    print('Tasks 21 and 25: all checks passed.')


if __name__ == '__main__':
    main()
