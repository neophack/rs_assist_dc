# -*- coding: utf-8 -*-
###############################################################################
#
# Copyright (c) 2016 Baidu.com, Inc. All Rights Reserved
#
###############################################################################
"""
align all sensor data by timestamp.

Authors: wangxiao05(wangxiao05@baidu.com)
Date:    2018/08/21 15:49:10
"""
import glob
import os
import numpy as np


def align(tm_file, target_dirs=None):
    target_dirs = ['data/camera-4070']
    tm_list = np.array([int(float(la.rstrip()) * 1000) for la in open(tm_file)])
    for d in target_dirs:
        filelist = glob.glob(os.path.join(d, '15*.jpg'))
        tms = np.array(sorted(map(lambda x: int(x.split('/')[-1].rstrip('.jpg')), filelist)))
        inds = np.searchsorted(tms, tm_list, 'left')
        print tms[inds]
        print tm_list
        assert np.all((tms[inds] - tm_list) < 200)


if __name__ == '__main__':
    align('./tm.txt')
