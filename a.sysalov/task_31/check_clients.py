#!/usr/bin/env python3
import os
from pathlib import Path
import re
import socket
import subprocess
import tempfile
import time


def wait_for(condition):
    deadline = time.monotonic() + 5
    while not condition():
        assert time.monotonic() < deadline, 'Server response timed out'
        time.sleep(0.01)


def main():
    source = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='client-lines-', dir='/tmp') as temporary:
        directory = Path(temporary)
        for name in ('server', 'client'):
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                            str(source / f'{name}.c'), '-o', str(directory / name)], check=True)
        path = str(directory / 'socket')
        output_path = directory / 'output'
        log_path = directory / 'log'
        with output_path.open('wb') as output, log_path.open('wb') as log:
            server = subprocess.Popen([str(directory / 'server'), path],
                                      stdout=output, stderr=log)
            peer = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                wait_for(lambda: 'Server listening' in log_path.read_text())
                peer.connect(path)
                wait_for(lambda: 'Total: 1' in log_path.read_text())
                peer.sendall(b'hel')
                time.sleep(0.05)
                assert output_path.read_bytes() == b''
                result = subprocess.run([str(directory / 'client'), '-n', 'CLIENT_A', path],
                                        input=b'hello from client!\nthis is a test message.\nlast',
                                        capture_output=True, timeout=5)
                assert result.returncode == 0, result.stderr
                wait_for(lambda: b'LAST\n' in output_path.read_bytes())
                assert re.sub(rb'(?m)^\[Client \d+\]: ', b'', output_path.read_bytes()) == (
                    b'[CLIENT_A] HELLO FROM CLIENT!\n'
                    b'[CLIENT_A] THIS IS A TEST MESSAGE.\n[CLIENT_A] LAST\n')
                peer.sendall(b'lo\nsecond\n' + b'x' * 10000)
                peer.close()
                wait_for(lambda: log_path.read_text().endswith('Clients remaining: 0\n'))
                lines = output_path.read_bytes().splitlines()
                assert len(lines) == 6
                assert re.fullmatch(rb'\[Client \d+\]: HELLO', lines[3])
                assert re.fullmatch(rb'\[Client \d+\]: SECOND', lines[4])
                assert re.fullmatch(rb'\[Client \d+\]: X{10000}', lines[5])
                assert lines[3].split(b']:')[0] == lines[4].split(b']:')[0]
            finally:
                peer.close()
                server.terminate()
                server.wait(timeout=5)
        assert not os.path.exists(path)
    print('Client names, split/multiple lines, long lines and disconnect: OK')


if __name__ == '__main__':
    main()
