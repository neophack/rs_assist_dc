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
    url = 'http://127.0.0.1:8950/realsense_api/'
    url = 'http://172.24.24.253:8950/realsense_api/'
    r = requests.get(url)
    print 'Response:'
    print r.json().keys()


if __name__ == '__main__':
    test()
