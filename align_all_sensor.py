# -*- coding: utf-8 -*-
###############################################################################
#
# Copyright (c) 2016 Baidu.com, Inc. All Rights Reserved
#
###############################################################################
"""
align all sensor data by timestamp.

Authors: wangxiao05(wangxiao05@baidu.com)
Date:    2018/08/21 15:49:10
"""
import glob
import os
import shutil
import numpy as np


def align(tm_data, prefix, src_dirs, align_offsets):
    """tm_data"""
    # src_dirs = ['data/camera-4070']
    if isinstance(tm_data, int):
        tm_label = np.array([tm_data])
    elif isinstance(tm_data, list):
        tm_label = np.array(tm_data)
    else:
        tm_label = np.array([int(la.rstrip().split('\t')[1])
                             for la in open(tm_data) if la.startswith(prefix)])
    aligned_files = []
    for i, d in enumerate(src_dirs):
        filelist = glob.glob(os.path.join(d, '15*.jpg'))
        if not filelist:
            print os.path.join(d, '15*.png')
            filelist = glob.glob(os.path.join(d, '15*.png'))
        assert filelist, d
        tms = np.array(sorted(map(lambda x: int(x.split('/')[-1].split('.')[0]), filelist)))
        tm_list = tm_label + align_offsets[i]
        inds = np.searchsorted(tms, tm_list, 'left')
        start_ind = 0

        assert start_ind >= 0
        # print d
        # print 'inds:', inds[start_ind:]
        # print 'tms:', tms[inds[start_ind:]]
        # print 'tms - 1:', tms[inds[start_ind:] - 1]
        # print 'tm_list:', tm_list[start_ind:]
        # print np.array(tms[inds[start_ind:]]) - np.array(tm_list[start_ind:])
        assert np.all((abs(tms[inds[start_ind:]] - tm_list[start_ind:])) < 250)
        aligned_files.append((d, tms[inds[start_ind:]]))
    return aligned_files


def reconstruct_event_data(tm_file, src_dirs, align_offsets, dst_dir='tmp'):
    """Recontruct data."""
    from xeye_calib import resize_xeye_image_file
    aligned_files = align(tm_file, 'event', src_dirs, align_offsets)
    for i, (dirpath, files) in enumerate(aligned_files):
        dirname = os.path.join(dst_dir, str(i))
        if not os.path.exists(dirname):
            os.makedirs(dirname)
        for j, fn in enumerate(files):
            if 'depth' in dirpath:
                filepath = os.path.join(dirpath, str(fn) + '.png')
                dst_filename = str(10000000 + (j + 1))[1:] + '.jpg'
                dst_filepath = os.path.join(dirname, dst_filename)
                shutil.copy(filepath, dst_filepath)
            else:
                filepath = os.path.join(dirpath, str(fn) + '.jpg')
                dst_filename = str(10000000 + (j + 1))[1:] + '.jpg'
                dst_filepath = os.path.join(dirname, dst_filename)
                resize_xeye_image_file(filepath, dst_filepath)


def reconstruct_calib_data(tm_file, src_dirs, align_offsets, dst_dir='tmp'):
    """Reformat calib data."""
    from xeye_calib import resize_xeye_image_file
    aligned_files = align(tm_file, 'calib', src_dirs, align_offsets)
    for i, (dirpath, files) in enumerate(aligned_files):
        if 'depth' in dirpath and 'color' not in dirpath:
            continue
        dirname = os.path.join(dst_dir, str(i), 'cam0')
        if not os.path.exists(dirname):
            os.makedirs(dirname)
        for j, fn in enumerate(files):
            if 'depth' in dirpath:
                filepath = os.path.join(dirpath, str(fn) + '.png')
                dst_filename = str(10000000 + (j + 1))[1:] + '.jpg'
                dst_filepath = os.path.join(dirname, dst_filename)
                shutil.copy(filepath, dst_filepath)
            else:
                filepath = os.path.join(dirpath, str(fn) + '.jpg')
                dst_filename = str(10000000 + (j + 1))[1:] + '.jpg'
                dst_filepath = os.path.join(dirname, dst_filename)
                resize_xeye_image_file(filepath, dst_filepath)


class PerLineLabler(object):
    """One line, one label."""

    def __init__(self):
        pass
        self.default_init()


    def default_init(self):
        self.src_dirs = ['./data/camera-4068',
                         './data/camera-4069',
                         './data/camera-4070',
                         './data/camera-4071',
                         './data/depth_cam1/color',
                         './data/depth_cam2/color',
                         './data/depth_cam1/depth',
                         './data/depth_cam2/depth'
                         ]
        self.depth_cam_num = 2
        self.rgb_cam_list = [x for x in range(len(self.src_dirs) - self.depth_cam_num)]
        self.depth_cam_list = [x for x in range(len(self.rgb_cam_list) - self.depth_cam_num, len(self.src_dirs))]
        self.align_offsets = [0] * len(self.src_dirs)
        self.aligned_sequence = []
        self.intrinsic_path = 'data/batch2/parameters/leftIntrinsic.txt'
        self.extrinsic_path = 'data/batch2/parameters/transsWorldToCam.txt'
        return self

    def run(self, line):
        import infer_sequence
        items = line.rstrip().split()
        prefix = items[0]
        assert prefix == 'event'
        timestamp = int(items[1])
        product_id = int(items[2]) if len(items) > 2 else None

        aligned_files = align(timestamp, prefix, self.src_dirs, self.align_offsets)
        self.aligned_sequence.append(aligned_files)
        if self.aligned_sequence >= 2:
            rgb_file_seqs = []
            for i in self.rgb_cam_list:
                src_dir = self.src_dirs[i]
                file_seq = []
                for seq in self.aligned_sequence[-2:]:

                    for tm in seq[src_dir]:
                        file_seq.append(os.path.join(src_dir, tm + '.jpg'))
                assert len(file_seq) == 2, len(file_seq)
                rgb_file_seqs.append(file_seq)

            depth_file_seqs = []
            for i in self.depth_cam_list:
                src_dir = self.src_dirs[i]
                file_seq = []
                for seq in self.aligned_sequence[-2:]:

                    for tm in seq[src_dir]:
                        file_seq.append(os.path.join(src_dir, tm + '.jpg'))
                assert len(file_seq) == 2, len(file_seq)
                depth_file_seqs.append(file_seq)

            return infer_sequence.infer(self.intrinsic_path, self.extrinsic_path,
                                        self.rgb_cam_list, self.depth_cam_list,
                                        rgb_file_seqs, depth_file_seqs)


def test_calib():
    import commands
    tm_file = './data/test_gen_tm.txt'
    src_dirs = ['./data/camera-4068',
                './data/camera-4069',
                './data/camera-4070',
                './data/camera-4071',
                './data/depth_cam1/color',
                './data/depth_cam2/color',
                './data/depth_cam1/depth',
                './data/depth_cam2/depth'
                ]
    align_offsets = [25000, 25000, 24600, 25000, 0, 0, 0, 0]
    dst_dir = './data/batch2/calib_data'
    reconstruct_calib_data(tm_file, src_dirs, align_offsets, dst_dir)
    cmd = 'sh calibrate.sh {}'.format('data/batch2')
    [status, output] = commands.getstatusoutput(cmd)
    print status
    print output


def test_event():
    tm_file = './data/test_gen_tm.txt'
    src_dirs = ['./data/camera-4068',
                './data/camera-4069',
                './data/camera-4070',
                './data/camera-4071',
                './data/depth_cam1/color',
                './data/depth_cam2/color',
                './data/depth_cam1/depth',
                './data/depth_cam2/depth'
                ]
    align_offsets = [25000, 25000, 24600, 25000, 0, 0, 0, 0]
    dst_dir = './data/batch2/test_data'
    reconstruct_event_data(tm_file, src_dirs, align_offsets, dst_dir)


if __name__ == '__main__':
    print test_calib()
    # print test_event()
