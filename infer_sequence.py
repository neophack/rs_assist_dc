# -*- coding: utf-8 -*-
###############################################################################
#
# Copyright (c) 2016 Baidu.com, Inc. All Rights Reserved
#
###############################################################################
"""
Pipeline to get labeled region from depth assisted data..

Authors: wangxiao05(wangxiao05@baidu.com)
Date:    2018/07/23 13:59:19
"""
import glob
import os
import numpy as np
import cv2
from multiprocessing.dummy import Pool as ThreadPool
import sys

sys.path.insert(0, './shel-cam-3d')
from pipeline import get_region_sequence
from pipeline import display_region_sequence
from pipeline import get_obj_region
from pipeline import merge_multi_res_seqs


def test_get_region_sequence():
    """Unit test."""
    intrinsic_path = "data/batch2/parameters/leftIntrinsic.txt"
    extrinsic_path = "data/batch2/parameters/transsWorldToCam.txt"
    depth_cam_id_list = [4, 5]
    rgb_cam_id_list = [0, 1, 2, 3]

    for i in rgb_cam_id_list:
        depth_res_seqs = {}
        for depth_cam_id in depth_cam_id_list:
            rgb_dir = './data/batch2/test_data/%d/' % i
            depth_dir = './data/batch2/test_data/%d' % (depth_cam_id + 2)

            rgb_seq = sorted(glob.glob(os.path.join(rgb_dir, '*')))
            depth_seq = sorted(glob.glob(os.path.join(depth_dir, '*')))
            res_seq = get_region_sequence(intrinsic_path, extrinsic_path, rgb_seq, depth_seq,
                                          i, depth_cam_id)
            print('depth_cam_id:', depth_cam_id)
            depth_res_seqs.setdefault(depth_cam_id, res_seq)
            # cam_res_seqs.append(res_seq)
            # print res_seq
        # print 'depth_res_seqs:', depth_res_seqs.keys()
        # print 'depth_res_seq [4]', depth_res_seqs[4]
        depth_res_seqs['combined'] = merge_multi_res_seqs(depth_res_seqs.values())
        for k in depth_res_seqs:
            display_region_sequence(depth_res_seqs[k],
                                    suffix='_%d_%s' % (i, str(k)),
                                    rot_rect=True,
                                    draw_on_last=True)


def show_multi_images(display_images, resize_ratio=0.5, cols=2):
    if not display_images:
        return None
    s = len(display_images)
    if s % cols != 0:
        target_length = s + (cols - s % cols)
    else:
        target_length = s
    col_images = []
    row_images = []
    h, w = display_images[0].shape[0: 2]
    dst_h = int(h * resize_ratio)
    dst_w = int(w * resize_ratio)

    for i in range(target_length):
        if i != 0 and i % cols == 0:
            col_images.append(np.hstack(row_images))
            row_images = []
        if i >= s:
            append_image = np.zeros((dst_h, dst_w, 3), dtype=display_images[0].dtype)
        else:
            append_image = cv2.resize(display_images[i], (dst_w, dst_h))
        row_images.append(append_image)
    if row_images:
        col_images.append(np.hstack(row_images))
    img = np.vstack(col_images)
    print(img.shape)
    cv2.imshow('test', img)
    cv2.waitKey(0)


def infer_parallel(intrinsic_path, extrinsic_path, rgb_cam_id_list, depth_cam_id_list,
                   rgb_file_seqs, depth_file_seqs, save_dir=None, show_each_step=True):
    """Unit test."""
    display_images = []

    def _infer(intrinsic_path, extrinsic_path, i, depth_cam_id_list, rgb_file_seqs,
               depth_file_seqs):
        result_imgs = []
        depth_res_seqs = {}
        for j, depth_cam_id in enumerate(depth_cam_id_list):
            rgb_seq = rgb_file_seqs[i]
            depth_seq = depth_file_seqs[j]
            res_seq = get_region_sequence(intrinsic_path, extrinsic_path,
                                          rgb_seq, depth_seq,
                                          i, depth_cam_id)
            depth_res_seqs.setdefault(depth_cam_id, res_seq)
        depth_res_seqs['combined'] = merge_multi_res_seqs(list(depth_res_seqs.values()))

        for k in depth_res_seqs:
            if k != 'combined':
                continue
            images = display_region_sequence(depth_res_seqs[k],
                                             suffix='_%d_%s' % (i, str(k)),
                                             rot_rect=True,
                                             draw_on_last=True,
                                             save_dir=save_dir,
                                             show_each_step=show_each_step)
            result_imgs.extend(images)
        return result_imgs

    pool = ThreadPool(6)
    args = []
    for i in rgb_cam_id_list:
        args.append([intrinsic_path, extrinsic_path, i, depth_cam_id_list, rgb_file_seqs,
                     depth_file_seqs])
    print('args num', len(args))
    results = pool.map(lambda x: _infer(*x), args)
    pool.close()
    pool.join()
    display_images = []
    for imgs in results:
        print('imgs in result:', len(imgs))
        display_images.extend(imgs)
    return display_images


def infer(intrinsic_path, extrinsic_path, rgb_cam_id_list, depth_cam_id_list,
          rgb_file_seqs, depth_file_seqs, save_dir=None, show_each_step=True):
    """Unit test."""
    display_images = []
    for i in rgb_cam_id_list:
        depth_res_seqs = {}
        for j, depth_cam_id in enumerate(depth_cam_id_list):
            rgb_seq = rgb_file_seqs[i]
            depth_seq = depth_file_seqs[j]
            res_seq = get_region_sequence(intrinsic_path, extrinsic_path,
                                          rgb_seq, depth_seq,
                                          i, depth_cam_id)
            depth_res_seqs.setdefault(depth_cam_id, res_seq)
        depth_res_seqs['combined'] = merge_multi_res_seqs(depth_res_seqs.values())

        for k in depth_res_seqs:
            if k != 'combined':
                continue
            images = display_region_sequence(depth_res_seqs[k],
                                             suffix='_%d_%s' % (i, str(k)),
                                             rot_rect=True,
                                             draw_on_last=True,
                                             save_dir=save_dir,
                                             show_each_step=show_each_step)
            display_images.extend(images)
    return display_images


def test():
    region = get_obj_region("data/batch1/parameters/leftIntrinsic.txt",
                            "data/batch1/parameters/transsWorldToCam.txt",
                            "data/batch1/test_data/cam0/0001531476158412.jpg",
                            "data/batch1/test_data/cam0/0001531476172966.jpg",
                            # "data/batch1/test_data/cam2.bk/13_Color.png",
                            # "data/batch1/test_data/cam2.bk/14_Color.png",
                            "data/batch1/test_data/cam2/13-0_Depth.raw",
                            "data/batch1/test_data/cam2/14-0_Depth.raw",
                            0, 2,
                            show_result=True)
    return region


def test_1():
    depth_cam_id = 5
    rgb_cam_id = 1
    region = get_obj_region("./data/batch2/parameters/leftIntrinsic.txt",
                            "./data/batch2/parameters/transsWorldToCam.txt",
                            "./data/batch2/test_data/{}/0000001.jpg".format(rgb_cam_id),
                            "./data/batch2/test_data/{}/0000002.jpg".format(rgb_cam_id),
                            "./data/batch2/test_data/{}/0000001.jpg".format(depth_cam_id + 2),
                            "./data/batch2/test_data/{}/0000002.jpg".format(depth_cam_id + 2),
                            rgb_cam_id, depth_cam_id,
                            show_result=True)
    return region


if __name__ == '__main__':
    test_get_region_sequence()
