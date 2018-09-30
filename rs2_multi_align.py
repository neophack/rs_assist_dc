# -*- coding: utf-8 -*-
###############################################################################
#
# Copyright (c) 2016 Baidu.com, Inc. All Rights Reserved
#
###############################################################################
"""
Give out .

Authors: wangxiao05(wangxiao05@baidu.com)
Date:    2018/08/06 15:32:43
"""
import pyrealsense2 as rs
import numpy as np
import cv2
import os
import time


# get devices
ctx = rs.context()
devices = ctx.query_devices()
sn_list = []

for d in devices:
    sn = d.get_info(rs.camera_info.serial_number)
    try:
        sn_num = int(sn)
        sn_list.append(sn)
        if len(sn_list) >= 2:
            break
    except:
        continue
assert len(sn_list) >= 2, 'len sn_list: {}'.format(len(sn_list))


WIDTH = 640
HEIGHT = 480
# Configure depth and color streams...
# ...from Camera 1
pipeline_1 = rs.pipeline()
config_1 = rs.config()
config_1.enable_device(sn_list[0])
config_1.enable_stream(rs.stream.depth, WIDTH, HEIGHT, rs.format.z16, 30)
config_1.enable_stream(rs.stream.color, WIDTH, HEIGHT, rs.format.bgr8, 30)
# ...from Camera 2
pipeline_2 = rs.pipeline()
config_2 = rs.config()
config_2.enable_device(sn_list[1])
config_2.enable_stream(rs.stream.depth, WIDTH, HEIGHT, rs.format.z16, 30)
config_2.enable_stream(rs.stream.color, WIDTH, HEIGHT, rs.format.bgr8, 30)


# Start streaming from both cameras
pipeline_1.start(config_1)
pipeline_2.start(config_2)

align1 = rs.align(rs.stream.color)
align2 = rs.align(rs.stream.color)

if not os.path.exists('tmp'):
    os.makedirs('tmp')
cam_dir_1 = 'tmp/depth_cam1'
cam_dir_2 = 'tmp/depth_cam2'
if not os.path.exists(cam_dir_1):
    os.makedirs(os.path.join(cam_dir_1, 'depth'))
    os.makedirs(os.path.join(cam_dir_1, 'color'))
if not os.path.exists(cam_dir_2):
    os.makedirs(os.path.join(cam_dir_2, 'depth'))
    os.makedirs(os.path.join(cam_dir_2, 'color'))
with open(os.path.join(cam_dir_1, 'sn.txt'), 'wb') as fout:
    fout.write(sn_list[0] + '\n')
with open(os.path.join(cam_dir_2, 'sn.txt'), 'wb') as fout:
    fout.write(sn_list[1] + '\n')


print 'start saving images ...'
try:
    last_tm = time.time()
    while True:

        tm = str(int(time.time() * 1000))
        current_tm = time.time()
        if current_tm - last_tm > 1:
            last_tm = current_tm
            print 'saving: ', current_tm
        # Camera 1
        # Wait for a coherent pair of frames: depth and color
        frames_1 = pipeline_1.wait_for_frames()
        aligned_frames_1 = align1.process(frames_1)
        depth_frame_1 = aligned_frames_1.get_depth_frame()
        color_frame_1 = aligned_frames_1.get_color_frame()
        if not depth_frame_1 or not color_frame_1:
            continue
        # Convert images to numpy arrays
        depth_image_1 = np.asanyarray(depth_frame_1.get_data())
        color_image_1 = np.asanyarray(color_frame_1.get_data())

        # Camera 2
        # Wait for a coherent pair of frames: depth and color
        frames_2 = pipeline_2.wait_for_frames()
        aligned_frames_2 = align1.process(frames_2)

        depth_frame_2 = aligned_frames_2.get_depth_frame()
        color_frame_2 = aligned_frames_2.get_color_frame()
        if not depth_frame_2 or not color_frame_2:
            continue
        # Convert images to numpy arrays
        depth_image_2 = np.asanyarray(depth_frame_2.get_data())
        color_image_2 = np.asanyarray(color_frame_2.get_data())
        # Apply colormap on depth image (image must be converted to 8-bit per pixel first)
        depth_path_1 = os.path.join(cam_dir_1, 'depth', '%s.png' % tm)
        depth_flag_1 = os.path.join(cam_dir_1, 'depth', '%s.flag' % tm)
        color_path_1 = os.path.join(cam_dir_1, 'color', '%s.png' % tm)
        color_flag_1 = os.path.join(cam_dir_1, 'color', '%s.flag' % tm)
        depth_path_2 = os.path.join(cam_dir_2, 'depth', '%s.png' % tm)
        depth_flag_2 = os.path.join(cam_dir_2, 'depth', '%s.flag' % tm)
        color_path_2 = os.path.join(cam_dir_2, 'color', '%s.png' % tm)
        color_flag_2 = os.path.join(cam_dir_2, 'color', '%s.flag' % tm)

        with open(depth_path_1, 'wb') as fout:
            fout.write(depth_image_1.astype(np.uint16).tobytes())
        cv2.imwrite(color_path_1, color_image_1)
        with open(depth_flag_1, 'wb') as fout:
            pass
        with open(color_flag_1, 'wb') as fout:
            pass
        with open(depth_path_2, 'wb') as fout:
            fout.write(depth_image_2.astype(np.uint16).tobytes())
        cv2.imwrite(color_path_2, color_image_2)
        with open(depth_flag_2, 'wb') as fout:
            pass
        with open(color_flag_2, 'wb') as fout:
            pass
        time.sleep(0.1)

finally:
    # Stop streaming
    pipeline_1.stop()
    pipeline_2.stop()
