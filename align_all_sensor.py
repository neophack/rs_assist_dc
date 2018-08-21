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
import shutil
import numpy as np


def align(tm_file, prefix, src_dirs):
    # src_dirs = ['data/camera-4070']
    tm_list = np.array([int(la.rstrip().split('\t')[1]) for la in open(tm_file) if la.startswith(prefix)])
    aligned_files = []
    for d in src_dirs:
        filelist = glob.glob(os.path.join(d, '15*.jpg'))
        if not filelist:
            print os.path.join(d, '15*.png')
            filelist = glob.glob(os.path.join(d, '15*.png'))
        assert filelist
        tms = np.array(sorted(map(lambda x: int(x.split('/')[-1].split('.')[0]), filelist)))
        inds = np.searchsorted(tms, tm_list, 'left')
        start_ind = 0

        assert start_ind >= 0
        print d
        print 'inds:', inds[start_ind:]
        print 'tms:', tms[inds[start_ind:]]
        print 'tms - 1:', tms[inds[start_ind:] - 1]
        print 'tm_list:', tm_list[start_ind:]
        assert np.all((abs(tms[inds[start_ind:]] - tm_list[start_ind:])) < 1000)
        aligned_files.append((d, tms[inds[start_ind:]]))
    print aligned_files
    return aligned_files


def reconstruct_event_data(tm_file, src_dirs, dst_dir='tmp'):

    aligned_files = align(tm_file, 'event', src_dirs)
    for i, (filename, files) in enumerate(aligned_files):
        if 'depth' not in filename:
            dirname = os.path.join(dst_dir, str(i))
            if not os.path.exists(dirname):
                os.makedirs(dirname)
            for j, _ in enumerate(files):
                filename = str(10000000 + (j + 1))[1:] + '.jpg'
                dst_filepath = os.path.join(dirname, filename)
                if 'color' not in filename:
                    pass
                else:
                    shutil.copy(filename, dst_filepath)


def reconstruct_calib_data(tm_file, src_dirs, dst_dir='tmp'):
    """Reformat calib data."""
    from xeye_calib import resize_xeye_image_file
    aligned_files = align(tm_file, 'calib', src_dirs)
    for i, (dirpath, files) in enumerate(aligned_files):
        if 'depth' in dirpath and 'color' not in dirpath:
            continue
        dirname = os.path.join(dst_dir, str(i), 'cam0')
        if not os.path.exists(dirname):
            os.makedirs(dirname)
        for j, fn in enumerate(files):
            if 'depth' in dirpath:
                filepath = os.path.join(dirpath, str(fn) + '.png')
                dst_filename = str(10000000 + (j + 1))[1:] + '.jpg'
                dst_filepath = os.path.join(dirname, dst_filename)
                shutil.copy(filepath, dst_filepath)
                
            else:
                filepath = os.path.join(dirpath, str(fn) + '.jpg')
                dst_filename = str(10000000 + (j + 1))[1:] + '.jpg'
                dst_filepath = os.path.join(dirname, dst_filename)
                resize_xeye_image_file(filepath, dst_filepath)


def test_calib():
    tm_file = './tm.txt'
    src_dirs = ['data/camera-4068',
                'data/camera-4069',
                'data/camera-4070',
                'data/camera-4071',
                'data/depth_cam1/color',
                'data/depth_cam2/color',
                'data/depth_cam1/depth',
                'data/depth_cam2/depth'
                ]
    dst_dir = 'data/test_calib'
    reconstruct_calib_data(tm_file, src_dirs, dst_dir)


if __name__ == '__main__':
    print test_calib()
