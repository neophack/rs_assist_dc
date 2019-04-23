# -*- coding: utf-8 -*-
################################################################################
#
# Copyright (c) wbnupku@outlook.com, Inc. All Rights Reserved
#
################################################################################
"""
This module provides.

Filename: air_build.py
Authors:  wbnupku(wbnupku@outlook.com)
Date:     2019/04/15 16:49:48
"""

from __future__ import absolute_import
from __future__ import print_function
from __future__ import unicode_literals


import os
import attr
import typing


@attr.s
class AirProductFrame(object):
    """Frame."""
    filepath = attr.ib(type=str)
    data = attr.ib(type=bytes)


@attr.s
class AirProductWeightEvent(object):
    """Weigth"""
    samples = attr.ib(type=str)
    cells = attr.ib(type=str)


@attr.s
class AirProductDetectInfo(object):
    """Detect."""
    url = attr.ib(type=str, default='')
    image_data = attr.ib(type=bytes, default=b'')
    left = attr.ib(type=int, default=-1)
    right = attr.ib(type=int, default=-1)
    top = attr.ib(type=int, default=-1)
    bottom = attr.ib(type=int, default=-1)
    tag = attr.ib(type=str, default='')
    score = attr.ib(type=float, default=0.0)
    flag = attr.ib(type=bool, default=False)


@attr.s
class RealsenseAssistInfo(object):
    poly = attr.ib(type=typing.List[typing.Tuple[float, float]])
    sum_depth_diff = attr.ib(type=int)
    rect = attr.ib(type=typing.Tuple[float, float, float, float])
    rot_rect = attr.ib(type=typing.Tuple[float, float, float, float, float])
    region = attr.ib(type=typing.List[typing.Tuple[float, float]])
    contour = attr.ib(type=typing.List[typing.Tuple[float, float]])


@attr.s
class AirRecordOfBuildSearchUpdate(object):
    """Record of event."""
    start_frame = attr.ib(type=AirProductFrame)
    end_frame = attr.ib(type=AirProductFrame)
    goods_num = attr.ib(type=int)
    product_id = attr.ib(type=str)
    record_id = attr.ib(type=str)
    weight = attr.ib(type=typing.Optional[AirProductWeightEvent], default=None)

    detect_info = attr.ib(type=typing.Optional[AirProductDetectInfo], default=None)
    product_name = attr.ib(type=typing.Optional[typing.Text], default=None)
    action = attr.ib(type=int, default=0)
    key = attr.ib(type=typing.Text, default='')
    rs_assist_info = attr.ib(type=typing.Optional[RealsenseAssistInfo], default=None)


def convert_to_airupdate(base_dir, num, output_arg):
    from align_all_sensor import MultiCamGrabLabeler
    import infer_sequence
    from kvtools import append_kv
    from kvtools import convert_to_stream_out
    from attrs_utils import to_bytes
    # base_dir = 'data/batch4'
    collection_dir = 'test_data'
    labeler = MultiCamGrabLabeler(base_dir).init_from_test_data(collection_dir)
    out = convert_to_stream_out(output_arg)
    num = int(num)
    for i in range(2, num):
        results = labeler.gen_label(counter=i)
        images = []
        for r in results:
            images.extend(r['images'])
        infer_sequence.show_multi_images(images, resize_ratio=0.5, cols=2)
        f = open(os.path.join(base_dir, 'test_data', '0', str(10000000 + i)[1:] + '.txt'))
        product_id = f.read().strip()
        for res in results:
            for r in res['combined']:
                print('r:', r.keys())
                x, y, degree = r['rot_rect']
                start_frame = AirProductFrame(filepath=r['rgb0'], data=open(r['rgb0'], 'rb').read())
                end_frame = AirProductFrame(filepath=r['rgb1'], data=open(r['rgb1'], 'rb').read())
                rs_info = RealsenseAssistInfo(poly=r['poly'], sum_depth_diff=r['sum_depth_diff'],
                                              rect=r['rect'], rot_rect=(
                                                  x[0], x[1], y[0], y[1], degree),
                                              region=r['region'],
                                              contour=r['contour'])
                action = 1 if r['sum_depth_diff'] > 0 else -1
                record_id = base_dir + '_' + str(i)
                airup = AirRecordOfBuildSearchUpdate(start_frame=start_frame,
                                                     end_frame=end_frame,
                                                     goods_num=action,
                                                     action=action,
                                                     product_id=product_id,
                                                     record_id=record_id,
                                                     key=record_id,
                                                     rs_assist_info=rs_info)
                v = to_bytes(airup)
                append_kv(airup.key.encode('utf-8'), v, out)


if __name__ == '__main__':
    import sys
    convert_to_airupdate(sys.argv[1], sys.argv[2], sys.argv[3])
