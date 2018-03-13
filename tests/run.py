#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import re
import sys
import subprocess
import random

CLIENT = '../build/bin/comp_client'

testMatcher = re.compile(r'^[0-9]+\.[0-9]+_.+$')

tests = [ ]
for test in os.listdir('.'):
    if test.endswith('.nut') and testMatcher.match(test):
        tests.append(test)

random.shuffle(tests)

passCount = 0
failCount = 0

for test in tests:
    sys.stdout.write(test + ': ')

    try:
        subprocess.check_output([CLIENT, test], stderr=subprocess.STDOUT)
        passCount = passCount + 1
        print('PASS')
    except subprocess.CalledProcessError:
        failCount = failCount + 1
        print('FAIL')

print('')
print('FINISHED %d/%d: %.2f%%' % (passCount, passCount + failCount, \
    (100.0 * passCount) / (passCount + failCount)))
