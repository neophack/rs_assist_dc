# -*- coding: utf-8 -*-
###############################################################################
#
# Copyright (c) 2016 Baidu.com, Inc. All Rights Reserved
#
###############################################################################
"""
Wrapper for actual worker.

Authors: wangxiao05(wangxiao05@baidu.com)
Date:    2018/07/23 14:55:56
"""
import os
import base64
import time


def get_deep_file(src_path, color_image_dir):
    tim_inv = 9999999999999
    now_tim = int(time.time() * 1000)
    src_pic = ""
    for root, dirs, files in os.walk(src_path):
        for f in files:
            # print f
            if len(f) < 3:
                continue
            if not f.endswith('jpg') and not f.endswith('png'):
                continue
            print 'f:', f
            fn = long(f.split('.')[0])
            _abs = abs(fn - now_tim)
            if _abs < tim_inv:
                tim_inv = _abs
                if os.path.exists(src_pic):
                    os.remove(src_pic)
                    # print 'remove',src_pic
                src_pic = os.path.realpath("%s/%s" % (root, f))
                # print src_pic,tim_inv
            else:
                os.remove("%s/%s" % (root, f))
    color_pic = ''
    if src_pic:
        color_pic = os.path.join(color_image_dir, os.path.basename(src_path))
    return src_pic, color_pic


class RealsenseImageProvider(object):

    def __init__(self, depth_dir='./'):
        depth_dir = '/Users/wangxiao05/data/rs_assist_dc'
        self.depth_dir = depth_dir

    def run(self):
        d = {}
        fp_depth, fp_color = get_deep_file(os.path.join(self.depth_dir, 'depth_cam1/depth'),
                                           os.path.join(self.depth_dir, 'depth_cam1/color'))
        d['cam1_depth_file'] = base64.b64encode(open(fp_depth).read())
        d['cam1_color_file'] = base64.b64encode(open(fp_color).read())
        fp_depth, fp_color = get_deep_file(os.path.join(self.depth_dir, 'depth_cam2/depth'),
                                           os.path.join(self.depth_dir, 'depth_cam2/color'))
        d['cam2_depth_file'] = base64.b64encode(open(fp_depth).read())
        d['cam2_color_file'] = base64.b64encode(open(fp_color).read())
        return d
