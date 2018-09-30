# -*- coding: utf-8 -*-
################################################################################
#
# Copyright (c) 2018 Baidu.com, Inc. All Rights Reserved
#
################################################################################
"""
This module provides.

Filename: simulate_cam_cap.py
Authors:  wangxiao(wangxiao05@baidu.com)
Date:     2018/09/30 14:26:53
"""
import os
import glob


root_dir = 'data/batch4'
calib_data_dir = os.path.join(root_dir, 'calib_data')
parameters_dir = os.path.join(root_dir, 'parameters_dir')
target_dir = os.path.join(root_dir, 'test_data')
rgb_num = len(glob.glob(os.path.join(calib_data_dir, '*')))
total_cam_num = len(glob.glob(os.path.join(target_dir, '*')))
depth_cam_num = total_cam_num - rgb_num
calib_data_counter = 1
test_data_counter = 0

def get_calib_image():
    d = {}
    global calib_data_counter
    data_dir = calib_data_dir
    filename = str(10000000 + calib_data_counter)[1:]
    d['cam1_dept_file'] = os.path.join(data_dir, '4', 'cam0', filename + '.png')
    d['cam1_color_file'] = os.path.join(data_dir, '2', 'cam0', filename + '.png')
    d['cam2_dept_file'] = os.path.join(data_dir, '5', 'cam0', filename + '.png')
    d['cam2_color_file'] = os.path.join(data_dir, '3', 'cam0', filename + '.png')
    xeye = []
    for i in range(2):
        xeye.append(os.path.join(data_dir, str(i), 'cam0', filename + '.jpg'))
    d['xeye_image'] = xeye
    calib_data_counter += 1
    return d


def get_collection_image():
    d = {}
    global test_data_counter
    data_dir = target_dir
    filename = str(10000000 + test_data_counter)[1:]
    d['cam1_dept_file'] = os.path.join(data_dir, '4', filename + '.jpg')
    d['cam1_color_file'] = os.path.join(data_dir, '2', filename + '.jpg')
    d['cam2_dept_file'] = os.path.join(data_dir, '5', filename + '.jpg')
    d['cam2_color_file'] = os.path.join(data_dir, '3', filename + '.jpg')
    xeye = []
    for i in range(2):
        xeye.append(os.path.join(data_dir, str(i), filename + '.jpg'))
    d['xeye_image'] = xeye
    test_data_counter += 1
    return d


def run():
    for i in range(10):
        print get_calib_image()

    for i in range(10):
        print get_collection_image()

if __name__ == '__main__':
    run()