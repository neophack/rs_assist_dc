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

import base64


def test():
    import requests
    img_path = './1.jpg'
    imgdata = open(img_path).read()
    print 'image data len:', len(imgdata)
    url = 'http://127.0.0.1:8950/simple_api/'
    req_dict = dict(image_b64=base64.b64encode(imgdata))
    r = requests.post(url, json=req_dict)
    print 'Response:'
    print r.json()


if __name__ == '__main__':
    test()
