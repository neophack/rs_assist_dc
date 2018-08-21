# -*- coding: utf-8 -*-
###############################################################################
#
# Copyright (c) 2016 Baidu.com, Inc. All Rights Reserved
#
###############################################################################
"""
Gen label and timstamps.

Authors: wangxiao05(wangxiao05@baidu.com)
Date:    2018/08/21 18:09:54
"""

import time
import sys

if len(sys.argv) == 1:
    filename = 'test_gen_tm.txt'
else:
    filename = sys.argv[1]

with open(filename, 'ab') as fout:
    items = []
    prefix = 'event'
    while True:
        print 'input PRODUCT_ID, "calib", "event", "e" or type Return ...'

        inp = raw_input()
        if inp == '':
            tm = str(int(time.time() * 1000))
            items.insert(0, tm)
            items.insert(0, prefix)
            line = '\t'.join(items)
            print line
            print >> fout, line
            items = []
        elif inp == 'calib':
            items = []
            line = 'start recording calibration data ...'
            print line
            prefix = 'calib'
            print >> fout, line
        elif inp == 'event':
            items = []
            line = 'start recording event data ...'
            prefix = 'event'
            print line
            print >> fout, line
        elif inp != 'e':
            items.append(inp)
        else:
            print 'Program end'
            sys.exit(0)
