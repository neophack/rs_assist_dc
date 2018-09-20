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
import json
from aicam_server.cam_cap import get_image


def run():
    """Generate label and timestamp."""
    if 1 or len(sys.argv) == 1:
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
    from align_all_sensor import Calibrator
    from align_all_sensor import MultiCamGrabLabeler
    base_dir = sys.argv[1]
    calib = Calibrator(base_dir, resize_xeye=True)
    labeler = MultiCamGrabLabeler(base_dir, resize_xeye=True)
    # throw garbage image
    throw_away = get_image()
    print 'throw away:', throw_away
    throw_away = get_image()
    print 'throw away:', throw_away

    if 1 or len(sys.argv) == 1:
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
                file_dict = get_image()
                # print 'file_dict', json.dumps(file_dict, indent=2)
                if prefix == 'calib':
                    print 'file_dict', json.dumps(file_dict, indent=2)
                    calib.add_files(file_dict)
                else:
                    print 'add file_dict to labeler'
                    print 'file_dict', json.dumps(file_dict, indent=2)
                    labeler.add_files(file_dict)
                    labeler.gen_label()
                items = []
            elif inp == 'calib':
                items = []
                line = 'start recording calibration data ...'
                print line
                prefix = 'calib'
                print >> fout, line
            elif inp == 'event':
                if prefix == 'calib':
                    calib.cp_files()
                    [status, output] = calib.gen_calib_parameters()
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


if __name__ == '__main__':
    run1()
