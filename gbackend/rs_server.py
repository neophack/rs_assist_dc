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
import subprocess
import glob
import sys



def get_rgb_cam_file():
    tm = str(int(time.time() * 1000))
    cmd = 'cd getpic && bash getpic.sh ../tmp/ {}'.format(tm)
    subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    fps = sorted(glob.glob('tmp/{}_*.jpg'.format(tm)))
    print('fps: {}'.format(fps), file=sys.stderr)
    assert(len(fps) == 1)
    return fps
    
    
    
def get_deep_file(src_path, color_image_dir):
    tim_inv = 9999999999999
    now_tim = int(time.time() * 1000)
    src_pic = ""
    for root, dirs, files in os.walk(src_path):
        for f in files:
            # print f
            if len(f) < 3 or f.startswith('.'):
                continue
            
            if not f.endswith('.flag'):
                continue

            fn = int(f.split('.')[0])
            _abs = abs(fn - now_tim)
            if _abs < tim_inv:
                tim_inv = _abs
                if os.path.exists(src_pic):
                    os.remove(src_pic)
                    # print 'remove',src_pic
                src_pic = os.path.realpath("%s/%s" % (root, str(fn) + '.png'))
                # print src_pic,tim_inv
            else:
                try:
                    os.remove("%s/%s" % (root, str(fn) + '.flag'))
                except:
                    pass
                try:
                    os.remove("%s/%s" % (root, str(fn) + '.png'))
                except:
                    pass
    color_pic = ''
    if src_pic:
        color_pic = os.path.realpath(os.path.join(color_image_dir, os.path.basename(src_pic)))

    return src_pic, color_pic


class RealsenseImageProvider(object):

    def __init__(self, depth_dir='./'):
        depth_dir = './tmp'
        self.depth_dir = depth_dir

    def run(self):
        d = {}
        fp_depth, fp_color = get_deep_file(os.path.join(self.depth_dir, 'depth_cam1/depth'),
                                           os.path.join(self.depth_dir, 'depth_cam1/color'))
        import sys
        d['cam1_dept_file'] = base64.b64encode(open(fp_depth, 'rb').read()).decode('utf-8')
        d['cam1_color_file'] = base64.b64encode(open(fp_color, 'rb').read()).decode('utf-8')
        fp_depth, fp_color = get_deep_file(os.path.join(self.depth_dir, 'depth_cam2/depth'),
                                           os.path.join(self.depth_dir, 'depth_cam2/color'))
        d['cam2_dept_file'] = base64.b64encode(open(fp_depth, 'rb').read()).decode('utf-8')
        d['cam2_color_file'] = base64.b64encode(open(fp_color, 'rb').read()).decode('utf-8')
        fps = get_rgb_cam_file()
        for i, fp in enumerate(fps):
            d['rgb{}_color_file'.format(i)] = base64.b64encode(open(fp, 'rb').read()).decode('utf-8')
        return d


if __name__ == '__main__':
    get_rgb_cam_file()
