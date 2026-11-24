#!/usr/bin/env python3
from pathlib import Path
import re
import signal
import socket
import subprocess
import sys
import tempfile
import time


def wait_for(condition):
    deadline = time.monotonic() + 10
    while not condition():
        assert time.monotonic() < deadline, 'AIO server response timed out'
        time.sleep(0.01)


def main():
    source = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='aio-check-', dir='/tmp') as temporary:
        directory = Path(temporary)
        for name in ('server', 'client'):
            command = ['cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                       str(source / f'{name}.c'), '-o', str(directory / name)]
            if sys.platform.startswith('linux'):
                command += ['-lrt']
            elif sys.platform.startswith('sunos'):
                command += ['-lsocket', '-lnsl', '-lrt']
            subprocess.run(command, check=True)
        path = str(directory / 'socket')
        output_path = directory / 'output'
        log_path = directory / 'log'
        idle = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        children = []
        with output_path.open('wb') as output, log_path.open('wb') as log:
            server = subprocess.Popen([str(directory / 'server'), path], stdout=output, stderr=log)
            try:
                wait_for(lambda: 'AIO server listening' in log_path.read_text())
                idle.connect(path)
                for name in ('Alice', 'Bob'):
                    children.append(subprocess.Popen([str(directory / 'client'), name, path],
                                                      stderr=subprocess.PIPE))
                for child in children:
                    _, error = child.communicate(timeout=10)
                    assert child.returncode == 0, error
                wait_for(lambda: 'Clients remaining: 1' in log_path.read_text())
                lines = output_path.read_bytes().splitlines()
                assert len(lines) == 6, (lines, log_path.read_text())
                for name in ('ALICE', 'BOB'):
                    expected = [f'[{name}] HELLO FROM CLIENT',
                                f'[{name}] THIS IS A TEST MESSAGE',
                                f'[{name}] ASYNCHRONOUS IO IS AWESOME']
                    actual = [re.sub(r'^\[Client \d+\]: ', '', line.decode())
                              for line in lines if f'[{name}]'.encode() in line]
                    assert actual == expected
                assert b'[ALICE]' in lines[0] or b'[BOB]' in lines[0]
                assert server.poll() is None, log_path.read_text()
                idle.sendall(b'split ')
                idle.sendall(b'line\nlast')
                idle.close()
                wait_for(lambda: log_path.read_text().endswith('Clients remaining: 0\n'))
                lines = output_path.read_bytes().splitlines()
                assert lines[-2].endswith(b': SPLIT LINE')
                assert lines[-1].endswith(b': LAST')
                # Stop with an outstanding read from a new silent client.
                idle = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                idle.connect(path)
                wait_for(lambda: log_path.read_text().endswith('Total: 1\n'))
                server.send_signal(signal.SIGINT)
                assert server.wait(timeout=10) == 0
                assert 'aio_read:' not in log_path.read_text(), log_path.read_text()
            finally:
                idle.close()
                for child in children:
                    if child.poll() is None:
                        child.kill()
                        child.wait()
                if server.poll() is None:
                    server.terminate()
                    try:
                        server.wait(timeout=3)
                    except subprocess.TimeoutExpired:
                        server.kill()
                        server.wait()
        assert not Path(path).exists()
    print('POSIX AIO: concurrent named clients, idle client, EOF, cancellation: OK')


if __name__ == '__main__':
    main()
