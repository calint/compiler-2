#!/usr/bin/python3

import os
import re
import subprocess

# compiles '7540.baz' for the target of the case with the register report and
# compares the footer and the report with the listing: the most scratch
# registers and the most registers held at once, counted from the
# 'allocate' and 'free' comments, and per callee the most registers one call
# holds itself and the calls, counted from the labels of the inlined calls and
# the 'call' instructions; prints what differs, else what agrees
compiler = os.path.abspath('../../../baz')
machine = os.environ['MACHINE']
options = [f'--target={machine}', '--report=registers']
if machine != 'x86_64':
    options.append('--bin=gen-rv32i.bin')

result = subprocess.run([compiler, '7540.baz', *options],
                        capture_output=True, text=True)
lines = result.stdout.splitlines()
comment = ';' if machine == 'x86_64' else '#'
variables_base = 'rbp' if machine == 'x86_64' else 's0'

# the listing: what each register held, the frame that allocated it
live = []
frames = []
frame_names = []
own = {}
inlined_calls = {}
noinline_calls = {}
max_scratch = 0
max_held = 0
for line in lines:
    text = line.strip()
    label = re.match(r'^(func\.\S+):$', text)
    if label and not line.startswith(' '):
        frames = [len(frame_names)]
        frame_names.append(f'{label.group(1)[5:]} (noinline body)')
        continue
    if text == 'main:':
        frames = [len(frame_names)]
        frame_names.append('main')
        inlined_calls['main'] = 1
        continue
    if label and label.group(1).endswith('.end'):
        frames.pop()
        continue
    if label:
        name = re.sub(r'\.\.[\d.]+$', '', label.group(1)[5:])
        frames.append(len(frame_names))
        frame_names.append(name)
        inlined_calls[name] = inlined_calls.get(name, 0) + 1
        continue
    call = re.match(r'^call\s+func\.(\S+)$', text)
    if call:
        saved = sum(1 for held in live if held[0] != variables_base)
        noinline_calls.setdefault(call.group(1), []).append(saved)
        continue
    if not text.startswith(comment):
        continue
    allocated = re.search(r'allocate (scratch|named) register (?:-> )?(\w+)$',
                          text)
    if allocated:
        # note: the backend's own registers have no source location
        kind = allocated.group(1) if '[' in text else 'backend'
        frame = frames[-1] if frames else None
        live.append((allocated.group(2), kind, frame))
        listed = [held for held in live if held[1] != 'backend']
        max_scratch = max(max_scratch,
                          sum(1 for held in listed if held[1] == 'scratch'))
        max_held = max(max_held, len(listed))
        if frame is not None and kind != 'backend':
            count = sum(1 for held in listed if held[2] == frame)
            own[frame] = max(own.get(frame, 0), count)
        continue
    freed = re.search(r'free (?:scratch|named) register (\w+)$', text)
    if freed:
        index = max(i for i, held in enumerate(live)
                    if held[0] == freed.group(1))
        del live[index]

callees = {}
for frame, name in enumerate(frame_names):
    if frame in own:
        callees[name] = max(callees.get(name, 0), own[frame])
listed_callees = {
    name: (callees.get(name, 0), calls)
    for name, calls in inlined_calls.items()
}
for name, saved in noinline_calls.items():
    body = f'{name} (noinline body)'
    listed_callees[body] = (callees.get(body, 0), len(saved))

# the footer and the report
footer_scratch = None
peak = None
reported_callees = {}
reported_saved = {}
table = None
for line in lines:
    found = re.match(r'^. +max scratch registers in use: (\d+)$', line)
    if found:
        footer_scratch = int(found.group(1))
    found = re.match(r'^. register use at the peak: (\d+) of', line)
    if found:
        peak = int(found.group(1))
    if 'own  calls  callee' in line:
        table = reported_callees
        continue
    if 'saved  calls  callee' in line:
        table = reported_saved
        continue
    found = re.match(r'^. +(\d+) +(\d+)  (.+?)(?: \d+:\d+)?$', line)
    if table is not None and found:
        table[found.group(3)] = (int(found.group(1)), int(found.group(2)))

listed_saved = {
    name: (max(saved), len(saved))
    for name, saved in noinline_calls.items()
}

checks = [
    ('most scratch registers', footer_scratch, max_scratch),
    ('most registers held', peak, max_held),
    ('per callee', reported_callees, listed_callees),
    ('saved at noinline calls', reported_saved, listed_saved),
]
for title, reported, listed in checks:
    if reported == listed:
        print(f'{title}: the report agrees with the listing')
    else:
        print(f'{title}: the report says {reported}, the listing {listed}')
print(result.returncode)
