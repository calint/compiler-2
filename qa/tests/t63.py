#!/usr/bin/python3

import subprocess

def send_and_receive(input_line, process):
    process.stdin.write(input_line)
    process.stdin.flush()
    output = process.stdout.readline()
    print(output, end='')

process = subprocess.Popen(['./gen'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)

send_and_receive('bob\n', process)
send_and_receive('charlie\n', process)
send_and_receive('\n', process)

process.stdin.close()
process.stdout.close()
process.stderr.close()
process.wait()
