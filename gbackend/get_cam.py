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
import sys
import base64


def get_remote_json(ip):
    import requests
    url = 'http://{}:8950/realsense_api/'.format(ip)
    # url = 'http://172.24.24.253:8950/realsense_api/'
    r = requests.get(url)

    return r.json()


if __name__ == '__main__':
    if len(sys.argv) >= 2:
        ip = sys.argv[1]
    else:
        ip = '127.0.0.1'
    r = get_remote_json(ip)
    print('Response:')
    print(r.keys())