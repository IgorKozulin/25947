#!/usr/bin/env python3
from concurrent.futures import ThreadPoolExecutor
import os
from pathlib import Path
import select
import signal
import socket
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parent


def wait_for_output(output, expected):
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        if os.fstat(output.fileno()).st_size >= expected:
            return
        time.sleep(0.01)
    raise AssertionError('Server did not process data in time')


def send(client, path, data):
    result = subprocess.run([str(client), str(path)], input=data,
                            capture_output=True, timeout=10)
    assert result.returncode == 0, result.stderr


def check(number, directory):
    programs = {}
    for name in ('server', 'client'):
        executable = directory / f'{name}_{number}'
        subprocess.run(['cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                        str(ROOT / f'task_{number}' / f'{name}.c'),
                        '-o', str(executable)], check=True)
        programs[name] = executable
    path = directory / f'socket_{number}'
    with tempfile.TemporaryFile() as output:
        server = subprocess.Popen([str(programs['server']), str(path)],
                                  stdout=output, stderr=subprocess.PIPE)
        idle = None
        try:
            assert select.select([server.stderr], [], [], 5)[0], 'No startup message'
            assert str(path).encode() in server.stderr.readline()
            if number == 30:
                data = bytes(range(256)) * 4096
                send(programs['client'], path, data)
                assert server.wait(timeout=5) == 0
                output.seek(0)
                assert output.read() == data.upper()
            else:
                idle = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                idle.connect(str(path))
                first = b'Active client, idle peer!\n'
                send(programs['client'], path, first)
                wait_for_output(output, len(first))
                assert server.poll() is None
                output.seek(0)
                assert output.read() == first.upper()

                payloads = [bytes([ord('a') + i]) * 200000 for i in range(4)]
                with ThreadPoolExecutor(max_workers=4) as pool:
                    futures = [pool.submit(send, programs['client'], path, data)
                               for data in payloads]
                    for future in futures:
                        future.result()
                expected = sum(map(len, payloads)) + len(first)
                wait_for_output(output, expected)
                output.seek(len(first))
                mixed = output.read()
                assert len(mixed) == expected - len(first)
                for i in range(4):
                    assert mixed.count(bytes([ord('A') + i])) == 200000
                idle.close()
                idle = None
                last = b'Reconnected\n'
                send(programs['client'], path, last)
                wait_for_output(output, expected + len(last))
                output.seek(expected)
                assert output.read() == last.upper()
                server.send_signal(signal.SIGINT)
                assert server.wait(timeout=5) == 0
            assert not path.exists(), 'Socket path was not removed'
        finally:
            if idle is not None:
                idle.close()
            if server.poll() is None:
                server.terminate()
                try:
                    server.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    server.kill()
                    server.wait(timeout=2)
            server.stderr.close()

    path.write_text('Keep this file')
    result = subprocess.run([str(programs['server']), str(path)],
                            capture_output=True, timeout=5)
    assert result.returncode != 0 and path.read_text() == 'Keep this file'
    path.unlink()
    result = subprocess.run([str(programs['client']), str(path)],
                            input=b'', capture_output=True, timeout=5)
    assert result.returncode != 0
    for name in ('server', 'client'):
        result = subprocess.run([str(programs[name]), 'x' * 200],
                                input=b'', capture_output=True, timeout=5)
        assert result.returncode != 0
    print(f'Task {number}: OK')


def main():
    with tempfile.TemporaryDirectory(prefix='socket-check-', dir='/tmp') as temporary:
        check(30, Path(temporary))
        subprocess.run([sys.executable, str(ROOT / 'task_31' / 'check_clients.py')], check=True)
        if sys.platform == 'darwin':
            print('Task 32: run check_aio.py on Linux/Solaris (macOS socket AIO is unsupported)')
        else:
            subprocess.run([sys.executable, str(ROOT / 'task_32' / 'check_aio.py')], check=True)


if __name__ == '__main__':
    main()
