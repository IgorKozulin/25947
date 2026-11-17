#!/usr/bin/env python3
import errno
import os
from pathlib import Path
import pty
import select
import signal
import subprocess
import tempfile
import termios
import time

ROOT = Path(__file__).resolve().parent


def run(executable, *args, data=None, cwd=None, env=None):
    return subprocess.run([str(executable), *map(str, args)], input=data,
                          text=True, capture_output=True, timeout=5,
                          cwd=cwd, env=env)


def read_until(fd, expected, process):
    result = b''
    deadline = time.monotonic() + 5
    while expected not in result:
        assert time.monotonic() < deadline, (expected, result)
        ready, _, _ = select.select([fd], [], [], 0.1)
        if ready:
            try:
                part = os.read(fd, 65536)
            except OSError as error:
                if error.errno == errno.EIO:
                    break
                raise
            if not part:
                break
            result += part
        elif process.poll() is not None:
            break
    assert expected in result, (expected, result)
    return result


def check_terminal(executable, editor=False, interrupt=False):
    master, slave = pty.openpty()
    before = termios.tcgetattr(slave)
    process = subprocess.Popen([str(executable)], stdin=slave, stdout=slave,
                               stderr=slave)
    try:
        deadline = time.monotonic() + 5
        while termios.tcgetattr(slave)[3] & termios.ICANON:
            assert time.monotonic() < deadline
            select.select([], [], [], 0.01)
        if interrupt:
            process.send_signal(signal.SIGTERM)
        elif editor:
            os.write(master, b'a' * 35 + b' ' + b'hello')
            read_until(master, b'\r\nhello', process)
            os.write(master, b'\x17')
            read_until(master, b'a' * 35 + b' ', process)
            erase = before[6][termios.VERASE]
            kill = before[6][termios.VKILL]
            os.write(master, b'x' + erase)
            read_until(master, b'a' * 35 + b' ', process)
            os.write(master, kill)
            read_until(master, b'\x1b[J', process)
            os.write(master, b'\x01')
            read_until(master, b'\x07', process)
            os.write(master, b'\x04')
        else:
            read_until(master, b'(y/n): ', process)
            os.write(master, b'y')
            read_until(master, b'Answer: y', process)
        assert process.wait(timeout=5) == (128 + signal.SIGTERM if interrupt else 0)
        after = termios.tcgetattr(slave)
        # PENDIN is a transient kernel flag on macOS pseudo terminals.
        after[3] &= ~getattr(termios, 'PENDIN', 0)
        before[3] &= ~getattr(termios, 'PENDIN', 0)
        assert after == before
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()
        os.close(master)
        os.close(slave)


def main():
    with tempfile.TemporaryDirectory(prefix='sysalov-check-') as temporary:
        directory = Path(temporary)
        programs = {}
        for number in range(8, 21):
            source, = (ROOT / f'task_{number}').glob('*.c')
            programs[number] = directory / f'task_{number}'
            subprocess.run(['cc', '-std=c99', '-O2', '-Wall', '-Wextra',
                            '-Werror', str(source), '-o', str(programs[number])],
                           check=True)

        sample = directory / "text with 'quote.txt"
        sample.write_text('first\nlast\n')
        environment = dict(os.environ, EDITOR='cat')
        result = run(programs[8], sample, env=environment)
        assert result.returncode == 0 and 'first\nlast\n' in result.stdout
        holder = subprocess.Popen([str(programs[8]), str(sample)],
                                  env=dict(os.environ, EDITOR='sleep 2 #'),
                                  stdout=subprocess.PIPE, text=True)
        try:
            assert 'File locked' in holder.stdout.readline()
            result = run(programs[8], sample, env=environment)
            assert result.returncode != 0 and 'locked' in result.stderr
        finally:
            holder.wait(timeout=5)

        result = run(programs[9], sample)
        assert result.returncode == 0
        assert result.stdout.index('last\n') < result.stdout.index('Child process finished.')
        result = run(programs[10], '/bin/sh', '-c', 'exit 7')
        assert 'Exit code: 7' in result.stdout
        result = run(programs[10], '/bin/sh', '-c', 'kill -TERM $$')
        assert 'Terminated by signal:' in result.stdout
        assert 'Exit code: 127' in run(programs[10], 'no-such-sysalov-command').stdout
        result = run(programs[11], 'env')
        assert set(result.stdout.splitlines()) == {'PATH=/usr/bin:/bin', 'TASK=11'}
        assert run(programs[11], 'no-such-sysalov-command').returncode == 127

        result = run(programs[12], data='cat < input > output &\n')
        assert 'Command: [cat]' in result.stdout and 'Background: yes' in result.stdout
        for number in range(13, 16):
            result = run(programs[number], data='missing-sysalov-command\necho OK\nexit\n')
            assert result.stdout.strip() == 'OK' and result.stderr
            result = run(programs[number], data='echo "hello world"; echo ">"\n')
            assert result.stdout == 'hello world\n>\n'
            result = run(programs[number], data='echo first >\necho after\n')
            assert 'after' in result.stdout and result.stderr
        for number in (14, 15):
            with tempfile.TemporaryFile(mode='w+') as output:
                process = subprocess.Popen([str(programs[number])], stdin=subprocess.PIPE,
                                           stdout=output, stderr=output, text=True)
                process.stdin.write('sleep 2 &\necho ready\nexit\n')
                process.stdin.close()
                try:
                    assert process.wait(timeout=1) == 0
                finally:
                    if process.poll() is None:
                        process.kill()
                        process.wait()
                output.seek(0)
                result = output.read()
                assert 'Background PID:' in result and 'ready' in result
        result = run(programs[15], data='echo one>out\necho two>>out\ncat<out\nexit\n', cwd=directory)
        assert result.stdout == 'one\ntwo\n'
        assert (directory / 'out').read_text() == 'one\ntwo\n'
        assert run(programs[13], data='cat<out\n', cwd=directory).stderr
        assert run(programs[14], data='cat<out\n', cwd=directory).stderr

        check_terminal(programs[16])
        check_terminal(programs[16], interrupt=True)
        check_terminal(programs[17], editor=True)
        check_terminal(programs[17], interrupt=True)

        sample.chmod(0o640)
        result = run(programs[18], sample, directory)
        assert result.returncode == 0 and result.stdout.startswith('-rw-r-----')
        assert sample.name in result.stdout and str(sample) not in result.stdout
        assert run(programs[18], directory / 'missing').returncode != 0
        fixtures = directory / 'patterns'
        fixtures.mkdir()
        for name in ('alpha.txt', 'beta.c', 'literal[1]', '.hidden'):
            (fixtures / name).touch()
        (fixtures / 'sub').mkdir()
        (fixtures / 'sub' / 'file.txt').touch()
        for pattern, expected in [('*.txt', ['alpha.txt']), ('?eta.*', ['beta.c']),
                                  ('a**?ha.t?t', ['alpha.txt']),
                                  ('literal[1]', ['literal[1]']), ('none*', ['none*'])]:
            result = run(programs[19], data=pattern+'\n', cwd=fixtures)
            assert result.returncode == 0 and result.stdout.split(': ', 1)[1].splitlines() == expected
        assert run(programs[19], data='sub/*\n', cwd=fixtures).returncode != 0
        for pattern, expected in [('s*/*.txt', 'sub/file.txt'), ('*/*', 'sub/file.txt'),
                                  ('sub/', 'sub/'), ('./sub//*.txt', './sub/file.txt'),
                                  ('missing/*', 'missing/*')]:
            result = run(programs[20], data=pattern+'\n', cwd=fixtures)
            assert result.returncode == 0 and result.stdout.split(': ', 1)[1].strip() == expected
        pattern = str(fixtures / 'sub' / '*.txt')
        assert str(fixtures / 'sub' / 'file.txt') in run(programs[20], data=pattern+'\n').stdout
    print('Tasks 8-20: all checks passed.')


if __name__ == '__main__':
    main()
