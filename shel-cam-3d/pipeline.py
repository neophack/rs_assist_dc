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
import commands
import glob
import os
import numpy as np
import cv2
import random


def run_shell_command(cmd):
    """Run gen shell command."""
    [status, output] = commands.getstatusoutput(cmd)
    # print output
    return status, output


def get_obj_region(intrinsic_path, extrinsic_path, rgb_path0, rgb_path1, depth_path0, depth_path1,
                   rgb_camid, depth_camid, show_result=False):
    """Retrun changed obj region."""
    bin_path = os.path.dirname(os.path.realpath(__file__)) + '/shel_cam_3d'
    cmd_line = ('%s --extrinsic_fp %s --intrinsic_fp %s --rgb_path0 %s --rgb_path1 %s '
                '--depth_path0 %s --depth_path1 %s --depth_camid %d --rgb_camid %d')
    cmd_line = cmd_line % (bin_path, extrinsic_path, intrinsic_path, rgb_path0,
                           rgb_path1, depth_path0, depth_path1, depth_camid, rgb_camid)
    print 'cmd_line:', cmd_line
    if show_result:
        cmd_line += ' --show_result'
    # print cmd_line
    status, output = run_shell_command(cmd_line)
    print 'run_shell_command done:'
    assert status == 0, str(status) + '\n' + output
    items = output.strip().split('\t')
    # print items[-1]
    if ':' not in items[-1]:
        return [], 0
    region = [tuple(e.split(',')) for e in items[-1].split(':') if e]
    print 'items:', items[: -1]
    region = [(float(x), float(y)) for x, y in region]
    region_diff = float(items[-2])
    return region, region_diff


def infer_object(region_info):
    """res_seq: [(rgb0, rgb1, depth0, depth1, region, depth_diff_accu),
          (rgb0, rgb1, depth0, depth1, region, depth_diff_accu),
          ...
          ]
    """
    print 'region_info', len(region_info)
    rgb0, rgb1, depth0, depth1, region_raw, sum_depth_diff = region_info
    if not region_raw:
        res = dict(rgb0=rgb0,
                   rgb1=rgb1,
                   depth0=depth0,
                   depth1=depth1,
                   region=region_raw,
                   contour=None,
                   poly=None,
                   rect=None,
                   rot_rect=None,
                   sum_depth_diff=sum_depth_diff)
        return res

    indices = [(x, y) for x, y in region_raw if x >= 0 and y >= 0 and x < 640 and y < 480]
    xs = [x for x, y in indices]
    ys = [y for x, y in indices]
    # return img
    rgb0_img = cv2.imread(rgb0)
    # rgb1_img = cv2.imread(rgb1)
    mask = np.zeros(rgb0_img.shape[0: 2], dtype=np.uint8)
    mask[ys, xs] = 255
    image, contours, hierarchy = cv2.findContours(mask, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
    csizes = [c.size for c in contours]
    print 'csizes:', csizes
    if not csizes:
        res = dict(rgb0=rgb0,
                   rgb1=rgb1,
                   depth0=depth0,
                   depth1=depth1,
                   region=region_raw,
                   contour=None,
                   poly=None,
                   rect=None,
                   rot_rect=None,
                   sum_depth_diff=sum_depth_diff)
        return res
    ind = np.argmax(csizes)
    contour = contours[ind]
    poly = cv2.approxPolyDP(contour, 1, closed=True)
    rect = cv2.boundingRect(np.array(poly))
    rot_rect = cv2.minAreaRect(poly)
    # print 'type of region_raw:', type(region_raw)
    # print region_raw
    res = dict(rgb0=rgb0,
               rgb1=rgb1,
               depth0=depth0,
               depth1=depth1,
               region=region_raw,
               contour=contour,
               poly=poly,
               rect=rect,
               rot_rect=rot_rect,
               sum_depth_diff=sum_depth_diff)
    return res


def get_region_sequence(intrinsic_path, extrinsic_path, rgb_seq, depth_seq, rgb_camid, depth_camid):
    """Describe this func with one line."""
    assert(len(rgb_seq) == len(depth_seq)), '{}\n{}'.format(rgb_seq, depth_seq)
    res_seq = []
    for i in range(1, len(rgb_seq)):
        region, region_diff = get_obj_region(intrinsic_path, extrinsic_path, rgb_seq[i - 1], rgb_seq[i],
                                             depth_seq[i - 1], depth_seq[i], rgb_camid, depth_camid)
        region = list(set([(int(x), int(y)) for x, y in region]))
        res = (rgb_seq[i - 1], rgb_seq[i], depth_seq[i - 1], depth_seq[i], region, region_diff)
        print len(region)
        res_seq.append(infer_object(res))
    return res_seq


def merge_multi_res_seqs(res_seq_list):
    s = len(res_seq_list[0])
    for i in range(1, len(res_seq_list)):
        assert len(res_seq_list[i]) == s
    output_res_seq = []
    for i in range(s):
        rs = [res_seq[i] for res_seq in res_seq_list]
        region_raw = rs[0]['region']
        rgb0 = rs[0]['rgb0']
        rgb1 = rs[0]['rgb1']
        depth0 = [rs[0]['depth0']]
        depth1 = [rs[0]['depth1']]
        sum_depth_diff = 0.0
        for r in rs[1:]:
            assert r['rgb0'] == rgb0
            assert r['rgb1'] == rgb1
            depth0.append(r['depth0'])
            depth1.append(r['depth1'])
            region_raw.extend(r['region'])
            sum_depth_diff += r['sum_depth_diff']
        sum_depth_diff /= len(rs)
        region_info = [rgb0, rgb1, depth0, depth1, region_raw, sum_depth_diff]
        output_res_seq.append(infer_object(region_info))
    return output_res_seq


def make_alpha_blend(img, indices, alpha, color):
    indices = [(x, y) for x, y in indices if x >= 0 and y >= 0]
    xs = [x for x, y in indices]
    ys = [y for x, y in indices]
    # return img
    mask = np.zeros(img.shape[0: 2], dtype=np.uint8)
    mask[ys, xs] = 255
    image, contours, hierarchy = cv2.findContours(mask, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
    # print image.shape, len(contours)
    mask1 = np.zeros(img.shape[0: 2], dtype=np.uint8)
    csizes = [c.size for c in contours]
    ind = np.argmax(csizes)
    nc = cv2.approxPolyDP(contours[ind], 1, closed=True)
    mask1[[y for x, y in contours[ind].reshape(-1, 2)],
          [x for x, y in contours[ind].reshape(-1, 2)]] = 255
    # print 'countours:', len(contours), contours[ind].shape
    cv2.drawContours(mask1, [nc], -1, 255, -1)
    cv2.imwrite('test.jpg', image)
    r = cv2.boundingRect(np.array(nc))
    forground = np.zeros(img.shape, dtype=np.float32)
    forground[ys, xs] = np.array(color).astype(np.float32) * alpha
    background = img.copy().astype(np.float32)
    background[ys, xs] = background[ys, xs] * (1 - alpha)
    background += forground
    background = background.astype(np.int8)
    cv2.rectangle(background, tuple(r[0: 2]), tuple([r[0] + r[2], r[1] + r[3]]), (0, 255, 0), 3)
    return background


def make_display_image(res_info, rot_rect, background_img=None):
    alpha = 0.1

    if res_info['sum_depth_diff'] > 0:
        img = cv2.imread(res_info['rgb0'])
    else:
        img = cv2.imread(res_info['rgb1'])
    if background_img is not None:
        img = background_img.copy()
    if not res_info['region'] or res_info['contour'] is None:
        return img
    # print res_info['poly']
    # draw region
    mask = np.zeros(img.shape[0: 2], dtype=np.uint8)
    cv2.drawContours(mask, [res_info['poly']], -1, 255, -1)
    # print mask
    ys, xs = np.where(mask == 255)

    forground = np.zeros(img.shape, dtype=np.float32)
    color = [255, 0, 0]
    forground[ys, xs] = np.array(color).astype(np.float32) / 255 * alpha
    background = img.copy().astype(np.float32) / 255
    background[ys, xs] = background[ys, xs] * (1 - alpha)
    background = background + forground
    background *= 255
    img_show = background.astype(np.uint8)

    if rot_rect:
        # draw rot rect
        r = res_info['rot_rect']
        box_pts = np.int16(cv2.boxPoints(r))
        color = tuple(random.sample(range(0, 255), 3))
        # color = (0, 255, 0)
        for i in range(1, 4):
            cv2.line(img_show, tuple(box_pts[i]), tuple(box_pts[i-1]), color)
        cv2.line(img_show, tuple(box_pts[3]), tuple(box_pts[0]), color)
        # cv2.drawContours(img_show, [box_pts], -1, (0, 255, 0), 4)
    else:
        # draw rect
        r = res_info['rect']
        color = tuple(random.sample(range(0, 255), 3))
        img_show = cv2.rectangle(img_show, tuple(r[0: 2]), tuple(
            [r[0] + r[2], r[1] + r[3]]), color, 3)
        # img_show = cv2.rectangle(img_show, tuple(r[0: 2]), tuple(
        #     [r[0] + r[2], r[1] + r[3]]), (0, 0, 255), 3)

    return img_show


def display_region_sequence(res_seq, suffix='', save_dir='./', rot_rect=False, draw_on_last=False,
                            show_each_step=True):
    import cv2
    count = 0
    display_image = None
    if draw_on_last:
        background_info = res_seq[-1]
        if background_info['sum_depth_diff'] > 0:
            display_image = cv2.imread(background_info['rgb0'])
        else:
            display_image = cv2.imread(background_info['rgb1'])
    for res in res_seq:
        # img0 = cv2.imread(rgb0_path)
        # img1 = cv2.imread(rgb1_path)
        # if region_diff < 0:
        #     img = img1
        # else:
        #     img = img0
        # print 'region', region
        # display_image = make_alpha_blend(img, region, 0.5, [255, 0, 0])
        if draw_on_last:
            display_image = make_display_image(res, rot_rect, display_image)
        else:
            display_image = make_display_image(res, rot_rect, None)
        name = str(1000000 + count)[1:]
        filepath = os.path.join(save_dir, '%s%s.jpg' % (name, suffix))
        if show_each_step:
            print 'show result'
            cv2.imshow('Show result', display_image)
            cv2.waitKey(0)
        cv2.imwrite(filepath, display_image)
        count += 1


def test_get_region_sequence():
    """Unit test."""
    intrinsic_path = "./data/leftIntrinsic.txt"
    extrinsic_path = "./data/transsWorldToCam.txt"
    rgb_dir = './data/test_data/cam0/'
    depth_dir = './data/test_data/cam2/'

    rgb_seq = sorted(glob.glob(os.path.join(rgb_dir, '*')))
    depth_seq = sorted(glob.glob(os.path.join(depth_dir, '*')))
    res_seq = get_region_sequence(intrinsic_path, extrinsic_path, rgb_seq, depth_seq,
                                  0, 2)
    # print res_seq
    display_region_sequence(res_seq, rot_rect=True)
    # print 'infer:', infer


def test():
    print get_obj_region("./data/leftIntrinsic.txt", "./data/transsWorldToCam.txt",
                         "./data/test_data/cam0/0001531476158412.jpg",
                         "./data/test_data/cam0/0001531476172966.jpg",
                         "./data/test_data/cam2/13-0_Depth.raw",
                         "./data/test_data/cam2/14-0_Depth.raw",
                         0, 2,
                         show_result=True)


if __name__ == '__main__':
    test_get_region_sequence()
