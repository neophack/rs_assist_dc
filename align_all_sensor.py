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
import sys
import shutil
import numpy as np
import logging
import commands


logger = logging.getLogger()
logger.setLevel(logging.DEBUG)
if not logger.handlers:
    logger.addHandler(logging.StreamHandler())


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


def init_cam_set(file_dict):
    """Init cam ids.

    All regular cam comes first, then all rgb cam of realsense, finnally
    all depth cam of realsense accoding to its rgb cam parnter's order.
    """
    depth_cam_num = 0
    for k in file_dict:
        if k.endswith('dept_file'):
            depth_cam_num += 1
    rgb_keys = [('xeye_image', i) for i in range(len(file_dict['xeye_image']))]
    depth_keys = []
    for k in range(depth_cam_num):
        rgb_keys.append('cam{}_color_file'.format(k + 1))

    for k in range(depth_cam_num):
        depth_keys.append('cam{}_dept_file'.format(k + 1))

    rgb_cam_list = range(len(rgb_keys))
    rgb_of_depth_cam_list = range(len(rgb_keys) - len(depth_keys), len(rgb_keys))
    src_keys = rgb_keys[:]
    src_keys.extend(depth_keys)
    return src_keys, rgb_cam_list, rgb_of_depth_cam_list


class Calibrator(object):
    """The line, one label."""

    def __init__(self, dst_dir, resize_xeye):
        self.dst_dir = dst_dir
        self.calib_data_dir = os.path.join(dst_dir, 'calib_data')
        self.src_keys = None
        self.counter = 1
        self.resize_xeye = resize_xeye
        self.record_path = os.path.join(dst_dir, 'calib_data.txt')

    def add_files(self, file_dict):
        """Parse files to add."""
        from xeye_calib import resize_xeye_image_file

        if self.src_keys is None:
            self.src_keys, self.rgb_cam_list, self.rgb_of_depth_cam_list = init_cam_set(file_dict)
            self.src_keys_dict = {v: i for i, v in enumerate(self.src_keys)}
            logger.info('Init Calibrator done.')
        for k, v in file_dict.iteritems():
            print 'k,v', k, v
            filename = str(10000000 + self.counter)[1:]
            if k.startswith('cam'):
                if 'dept' in k:
                    continue
                cam_id = self.src_keys_dict[k]
                dst_path = os.path.join(self.calib_data_dir, str(cam_id), 'cam0', filename + '.' + v.split('.')[-1]) 
                if not os.path.exists(os.path.dirname(dst_path)):
                    os.makedirs(os.path.dirname(dst_path))
                print 'calib data copy', v, dst_path
                print >> sys.stderr, 'calib data copy', v, dst_path
                with open(self.record_path, 'ab') as fout:
                     fout.write('cp ' + v + ' ' + dst_path + '\n')
            elif k.startswith('xeye'):
                for i, imgpath in enumerate(v):
                    cam_id = self.src_keys_dict[('xeye_image', i)]
                    dst_path = os.path.join(self.calib_data_dir, str(cam_id), 'cam0', filename + '.' + imgpath.split('.')[-1])
                    if not os.path.exists(os.path.dirname(dst_path)):
                        os.makedirs(os.path.dirname(dst_path))
                    if self.resize_xeye:
                         
                        with open(self.record_path, 'ab') as fout:
                             fout.write('resize ' + imgpath + ' ' + dst_path + '\n')
                        # resize_xeye_image_file(imgpath, dst_path)
                    else:
                        with open(self.record_path, 'ab') as fout:
                             fout.write('cp' + imgpath + ' ' + dst_path + '\n')
            else:
                logger.warn('Unrocognize key: {}'.format(k))
                return

        self.counter += 1

    def cp_files(self):
        from xeye_calib import resize_xeye_image_file
        f = open(self.record_path)
        for line in f:
            op, src_path, dst_path = line.strip().split()
            dir_path = os.path.dirname(dst_path)
            if not os.path.exists(dir_path):
                os.makedirs(dir_path)
            if op == 'resize':
                resize_xeye_image_file(src_path, dst_path)  
            elif op == 'cp':
                shutil.copy(src_path, dst_path)

    def gen_calib_parameters(self):
        logger.debug('dst_dir: {}'.format(self.dst_dir))
        cmd = 'sh calibrate.sh {}'.format(self.dst_dir)
        [status, output] = commands.getstatusoutput(cmd)
        assert os.path.exists(os.path.join(self.dst_dir, 'parameters/leftIntrinsic.txt')), output
        assert os.path.exists(os.path.join(self.dst_dir, 'parameters/transsWorldToCam.txt')), output
        return status, output


class MultiCamGrabLabeler(object):
    """One line, one label."""

    def __init__(self, dst_dir, resize_xeye):
        pass
        self.dst_dir = dst_dir
        self.param_dir = os.path.join(dst_dir, 'parameters')
        self.test_data_dir = os.path.join(dst_dir, 'test_data')
        self.intrinsic_path = os.path.join(self.param_dir, 'leftIntrinsic.txt')
        self.extrinsic_path = os.path.join(self.param_dir, 'transsWorldToCam.txt')
        self.result_dir = os.path.join(dst_dir, 'result')
        self.src_keys = None
        self.counter = 1
        self.labels = []
        self.resize_xeye = resize_xeye

    def add_files(self, file_dict):
        from xeye_calib import resize_xeye_image_file
        if self.src_keys is None:
            self.src_keys, self.rgb_cam_list, self.rgb_of_depth_cam_list = init_cam_set(file_dict)
            self.src_keys_dict = {v: i for i, v in enumerate(self.src_keys)}
            logger.info('Init Calibrator done.')
        for k, v in file_dict.iteritems():
            filename = str(10000000 + self.counter)[1:] + '.jpg'
            if k.startswith('cam'):
                cam_id = self.src_keys_dict[k]
                dst_path = os.path.join(self.test_data_dir, str(cam_id), filename)
                if not os.path.exists(os.path.dirname(dst_path)):
                    os.makedirs(os.path.dirname(dst_path))
                print v, dst_path
                shutil.copy(v, dst_path)
            elif k.startswith('xeye'):
                for i, imgpath in enumerate(v):
                    cam_id = self.src_keys_dict[('xeye_image', i)]
                    dst_path = os.path.join(self.test_data_dir, str(cam_id), filename)
                    if not os.path.exists(os.path.dirname(dst_path)):
                        os.makedirs(os.path.dirname(dst_path))
                    print imgpath, dst_path
                    if self.resize_xeye:
                        resize_xeye_image_file(imgpath, dst_path)
                    else:
                        shutil.copy(imgpath, dst_path)
            else:
                logger.warn('Unrocognize key: {}'.format(k))
                return
        self.counter += 1

    def gen_label(self):
        import infer_sequence
        if self.counter > 2:
            rgb_file_seqs = []
            depth_file_seqs = []

            for i, cam_id in enumerate(self.rgb_cam_list):
                if len(rgb_file_seqs) <= i:
                    rgb_file_seqs.append([])
                filename = str(10000000 + self.counter - 2)[1:] + '.jpg'
                rgb_file_seqs[i].append(os.path.join(self.test_data_dir, str(cam_id), filename))
                filename = str(10000000 + self.counter - 1)[1:] + '.jpg'
                rgb_file_seqs[i].append(os.path.join(self.test_data_dir, str(cam_id), filename))

            for i, cam_id in enumerate(self.rgb_of_depth_cam_list):
                depth_cam_id = cam_id + len(self.rgb_of_depth_cam_list)
                print 'i, cam_id', i, cam_id
                if len(depth_file_seqs) <= i:
                    depth_file_seqs.append([])
                filename = str(10000000 + self.counter - 2)[1:] + '.jpg'
                depth_file_seqs[i].append(os.path.join(self.test_data_dir, str(depth_cam_id), filename))
                filename = str(10000000 + self.counter - 1)[1:] + '.jpg'
                depth_file_seqs[i].append(os.path.join(self.test_data_dir, str(depth_cam_id), filename))

            return infer_sequence.infer(self.intrinsic_path, self.extrinsic_path,
                                        self.rgb_cam_list, self.rgb_of_depth_cam_list,
                                        rgb_file_seqs, depth_file_seqs, save_dir=self.result_dir,
                                        show_each_step=True)


class PerLineLabler(object):
    """One line, one label."""

    def __init__(self, dst_dir):
        pass
        self.src_dirs = None
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

    def init_by_file_dict(self, file_dict):
        """Init by file_dict.
        {'cam1_color_file': '/hohohoh/hohoh/1.png',,
         'cam1_dept_file': '/hohohoh/haha2222/1.png',
         'cam2_color_file': '/hohohoh/hohoh1/1.png',,
         'cam2_dept_file': '/hohohoh/haha222211/1.png',
         'xeye_image': ['/hmejjjfsdf/1.jpg', '/hohwoehfwe/jfdsf/2.jpg']
         }
        """
        self.depth_cam_num = 0
        self.rgb_cam_num = 0
        for k in file_dict:
            if k.endswith('dept_file'):
                self.depth_cam_num += 1
        rgb_dirs = sorted([os.path.dirname(k) for k in file_dict['xeye_image']])
        self.src_dirs = rgb_dirs[:]
        for k in range(self.depth_cam_num):
            self.src_dirs.append(os.path.dirname(file_dict['cam{}_color_file']))
        self.rgb_cam_list = [x for x in range(len(self.src_dirs))]
        self.rgb_cam_num = len(self.rgb_cam_list)
        for k in range(self.depth_cam_num):
            self.src_dirs.append(os.path.dirname(file_dict['cam{}_dept_file']))
        self.depth_cam_list = [x for x in range(self.rgb_cam_num, len(self.src_dirs))]
        self.align_offsets = [0] * len(self.src_dirs)
        self.aligned_sequence = []

    def run(self, line):
        import infer_sequence
        items = line.rstrip().split()
        prefix = items[0]
        assert prefix == 'event'
        timestamp = int(items[1])

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



def test_calibrator():
    """Generate simulate data_collection data and run calibrator."""
    base_dir = 'data/batch2/calib_data'
    calib = Calibrator('data/batch3')
    for i in range(0, 13):
        filename = str(10000000 + i + 1)[1:] + '.jpg'
        cam_data = {'cam1_color_file': os.path.join(base_dir, '4', 'cam0', filename),
                    'cam1_dept_file': os.path.join(base_dir, '4', 'cam0', filename),
                    'cam2_color_file': os.path.join(base_dir, '5', 'cam0', filename),
                    'cam2_dept_file': os.path.join(base_dir, '5', 'cam0', filename),
                    'xeye_image': [os.path.join(base_dir, '0', 'cam0', filename),
                                   os.path.join(base_dir, '1', 'cam0', filename),
                                   os.path.join(base_dir, '2', 'cam0', filename),
                                   os.path.join(base_dir, '3', 'cam0', filename),
                                   ]
                    }
        calib.add_files(cam_data)
    calib.gen_calib_parameters()
    

def test_multicamgrablabeler():
    """Generate simulate data_collection data and run calibrator."""
    base_dir = 'data/batch2/test_data'
    labeler = MultiCamGrabLabeler('data/batch3')
    for i in range(0, 13):
        filename = str(10000000 + i + 1)[1:] + '.jpg'
        cam_data = {'cam1_color_file': os.path.join(base_dir, '4', filename),
                    'cam1_dept_file': os.path.join(base_dir, '6', filename),
                    'cam2_color_file': os.path.join(base_dir, '5', filename),
                    'cam2_dept_file': os.path.join(base_dir, '7', filename),
                    'xeye_image': [os.path.join(base_dir, '0', filename),
                                   os.path.join(base_dir, '1', filename),
                                   os.path.join(base_dir, '2', filename),
                                   os.path.join(base_dir, '3', filename),
                                   ]
                    }
        labeler.add_files(cam_data)
        labeler.gen_label()


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
    # print test_calibrator()
    print test_multicamgrablabeler()
    # print test_event()
