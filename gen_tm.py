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
    import get_cam
    base_dir = 'data/batch4'
    calib = Calibrator(base_dir, resize_xeye=True)
    labeler = MultiCamGrabLabeler(base_dir, resize_xeye=True)




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
                file_dict = get_cam()
     
                if prefix == 'calib':
                    calib.add_files(file_dict)
                else:
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