#!/usr/bin/python3

import re

# the run-time bounds checks
checks = 0
for line in open('gen.s'):
    if re.search(r'\] bounds check begin$', line):
        checks += 1

print('bounds checks:', checks)
