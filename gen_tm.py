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
import os
import commands


def run():
    """Generate label and timestamp."""
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



def run1():
    """Data collection pipeline."""
    from align_all_sensor import reconstruct_calib_data
    from align_all_sensor import PerLineLabler
    labeler = PerLineLabler()

    tm_file = __file__
    src_dirs = ['./data/camera-4068',
                './data/camera-4069',
                './data/camera-4070',
                './data/camera-4071',
                './data/depth_cam1/color',
                './data/depth_cam2/color',
                './data/depth_cam1/depth',
                './data/depth_cam2/depth'
                ]
    align_offsets = [0, 0, 0, 0, 0, 0, 0, 0]
    base_dir = './data/batch3'
    calib_data_dir = os.path.join(base_dir, 'calib_data')
    test_data_dir = os.path.join(base_dir, 'test_data')
    parameter_dir = os.path.join(base_dir, 'parameters')

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
                labeler.run(line)
                items = []
            elif inp == 'calib':
                items = []
                line = 'start recording calibration data ...'
                print line
                prefix = 'calib'
                print >> fout, line
            elif inp == 'event':
                if prefix == 'calib':
                    reconstruct_calib_data(tm_file, src_dirs, align_offsets, calib_data_dir)
                    cmd = 'sh calibrate.sh {}'.format(base_dir)
                    [status, output] = commands.getstatusoutput(cmd)
                    print status
                    print output
                    assert status == 0

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