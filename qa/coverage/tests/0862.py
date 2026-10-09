#!/usr/bin/python3

import re

# the labels of the noinline bodies and the loads of the receiver register
# that load the register into itself, none is expected
labels = []
self_loads = 0
for line in open('gen.s'):
    m = re.match(r'^(func\.\S+):$', line)
    if m:
        labels.append(m.group(1))
    if re.match(r'^\s+(lea r11, \[r11\]|mv s11, s11|addi s11, s11, 0)$', line):
        self_loads += 1

print('bodies:', ' '.join(sorted(labels)))
print('receiver loaded into itself:', self_loads)
