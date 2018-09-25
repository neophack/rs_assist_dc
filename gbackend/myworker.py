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
import logging
import cv2
import numpy as np
import base64


class ImageTeller(object):

    def __init__(self):
        self.logger = logging.getLogger()
        self.logger.debug('Init ImageTeller')
        print 'Init ImageTeller'
        print self.logger.name, self.logger.handlers

    def run(self, args):
        try:
            # print >> sys.stderr, 'args:', args
            image_b64 = args['image_b64']
            # print >> sys.stderr, 'image_b64:', image_b64
            imgdata = np.fromstring(base64.b64decode(image_b64), dtype='uint8')
            image = cv2.imdecode(imgdata, 1)
            h, w = image.shape[0: 2]
            return dict(image_width=w, image_height=h)
        except:
            return dict(msg='Decode failed, may be not a image')
