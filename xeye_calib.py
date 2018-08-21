# -*- coding: utf-8 -*-
###############################################################################
#
# Copyright (c) 2016 Baidu.com, Inc. All Rights Reserved
#
###############################################################################
"""
Calib xeye with realsense2.

Authors: wangxiao05(wangxiao05@baidu.com)
Date:    2018/08/21 14:06:23
"""

import cv2
import numpy as np


def resize_xeye_image(data):
    """Resize and crop xeye image to 640 * 480."""
    imgdata = np.fromstring(data, dtype='uint8')
    img = cv2.imdecode(imgdata, 1)
    rimg = cv2.resize(img, (720, 480))
    rimg = rimg[:, 40: 680, :]
    _, imagedata = cv2.imencode('.jpg', rimg)
    return imagedata


def resize_xeye_image_file(filepath, dst_filepath):
    """Resize and crop xeye image to 640 * 480."""
    data = open(filepath).read()
    imgdata = np.fromstring(data, dtype='uint8')
    img = cv2.imdecode(imgdata, 1)
    rimg = cv2.resize(img, (720, 480))
    rimg = rimg[:, 40: 680, :]
    _, imagedata = cv2.imencode('.jpg', rimg)
    with open(dst_filepath, 'wb') as fout:
        fout.write(imagedata)


def test():
    imgpath = '1534509152869.jpg'
    data = open(imgpath).read()
    rimgdata = resize_xeye_image(data)
    open('output.jpg', 'wb').write(rimgdata)


if __name__ == '__main__':
    test()