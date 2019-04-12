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
import subprocess
import base64


logger = logging.getLogger()

if not logger.handlers:
    logger.setLevel(logging.DEBUG)
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
            print(os.path.join(d, '15*.png'))
            filelist = glob.glob(os.path.join(d, '15*.png'))
        assert filelist, d
        tms = np.array(sorted(map(lambda x: int(x.split('/')[-1].split('.')[0]), filelist)))
        tm_list = tm_label + align_offsets[i]
        inds = np.searchsorted(tms, tm_list, 'left')
        start_ind = 0

        assert start_ind >= 0
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
    # rgb_keys = [('xeye_image', i) for i in range(len(file_dict['xeye_image']))]
    rgb_keys = sorted([k for k in file_dict.keys() if k.startswith('rgb')])
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


def test_init_cam_set():
    keys = ['cam1_dept_file', 'cam1_color_file', 'cam2_dept_file', 'cam2_color_file', 'rgb0_color_file']
    d = dict(zip(keys, keys))
    print(init_cam_set(d))

class Calibrator(object):
    """The line, one label."""

    def __init__(self, dst_dir, resize_xeye):
        self.dst_dir = dst_dir
        self.calib_data_dir = os.path.join(dst_dir, 'calib_data')
        self.src_keys = None
        self.counter = 1
        self.resize_xeye = resize_xeye
        self.record_path = os.path.join(dst_dir, 'calib_data.txt')
        if os.path.exists(self.record_path):
            with open(self.record_path, 'w') as _:
                pass

    def add_files(self, file_dict):
        """Parse files to add."""
        from xeye_calib import resize_rgb_b64
        if self.src_keys is None:
            self.src_keys, self.rgb_cam_list, self.rgb_of_depth_cam_list = init_cam_set(file_dict)
            self.src_keys_dict = {v: i for i, v in enumerate(self.src_keys)}
            logger.info('Init Calibrator done.')
            logger.info('src_keys_dict, {}'.format(self.src_keys_dict))
            logger.info('file_dict.keys, {}'.format(file_dict.keys()))
        for k, v in file_dict.items():
            filename = str(10000000 + self.counter)[1:]
            if k.startswith('cam'):
                if 'dept' in k:
                    continue
                print(self.src_keys_dict.keys())
                cam_id = self.src_keys_dict[k]
                dst_path = os.path.join(self.calib_data_dir, str(
                    cam_id), 'cam0', filename + '.' + v.split('.')[-1])
                if not os.path.exists(os.path.dirname(dst_path)):
                    os.makedirs(os.path.dirname(dst_path))
                print('calib data copy', v, dst_path)
                print('calib data copy', v, dst_path, file=sys.stderr)
                # with open(self.record_path, 'a') as fout:
                #     fout.write('cp ' + v + ' ' + dst_path + '\n')
                with open(dst_path, 'wb') as fout:
                    fout.write(base64.b64decode(v))
            elif k.startswith('rgb'):
                cam_id = self.src_keys_dict[k]
                dst_path = os.path.join(self.calib_data_dir, str(
                    cam_id), 'cam0', filename + '.' + imgpath.split('.')[-1])
                if not os.path.exists(os.path.dirname(dst_path)):
                    os.makedirs(os.path.dirname(dst_path))
                if self.resize_xeye:
                    resize_rgb_b64(v, dst_path)
                else:
                    with open(dst_path, 'wb') as fout:
                        fout.write(base64.b64decode(v))

            else:
                logger.warn('Unrocognize key: {}'.format(k))
                return
        self.counter += 1


    def add_base64_files(self, file_dict):
        """Parse files to add."""
        if self.src_keys is None:
            self.src_keys, self.rgb_cam_list, self.rgb_of_depth_cam_list = init_cam_set(file_dict)
            self.src_keys_dict = {v: i for i, v in enumerate(self.src_keys)}
            logger.info('Init Calibrator done.')
        for k, v in file_dict.iteritems():
            filename = str(10000000 + self.counter)[1:]
            if k.startswith('cam'):
                if 'dept' in k:
                    continue
                cam_id = self.src_keys_dict[k]
                dst_path = os.path.join(self.calib_data_dir, str(
                    cam_id), 'cam0', filename + '.jpg')
                if not os.path.exists(os.path.dirname(dst_path)):
                    os.makedirs(os.path.dirname(dst_path))
                print('calib data copy', v, dst_path)
                # print >> sys.stderr, 'calib data copy', v, dst_path
                with open(dst_path, 'w') as fout:
                    fout.write(base64.b64decode(v))
            elif k.startswith('xeye'):
                for i, imgb64 in enumerate(v):
                    cam_id = self.src_keys_dict[('xeye_image', i)]
                    dst_path = os.path.join(self.calib_data_dir, str(
                        cam_id), 'cam0', filename + '.png')
                    if not os.path.exists(os.path.dirname(dst_path)):
                        os.makedirs(os.path.dirname(dst_path))
                    if self.resize_xeye:
                        with open(self.record_path, 'a') as fout:
                            fout.write('resize ' + imgb64 + ' ' + dst_path + '\n')
                        # resize_xeye_image_file(imgpath, dst_path)
                    else:
                        with open(dst_path, 'w') as fout:
                            fout.write(base64.b64decode(imgb64))
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
        # [status, output] = commands.getstatusoutput(cmd)
        output = subprocess.check_output(cmd, shell=True)
        assert os.path.exists(os.path.join(self.dst_dir, 'parameters/leftIntrinsic.txt')), output
        assert os.path.exists(os.path.join(self.dst_dir, 'parameters/transsWorldToCam.txt')), output
        return 0, output


class MultiCamGrabLabeler(object):
    """One line, one label."""

    def __init__(self, base_dir, test_data_dir_name='test_data', resize_xeye=True):
        pass
        self.base_dir = base_dir
        self.param_dir = os.path.join(base_dir, 'parameters')
        self.test_data_dir = self.gen_test_data_dir_name()
        self.intrinsic_path = os.path.join(self.param_dir, 'leftIntrinsic.txt')
        self.extrinsic_path = os.path.join(self.param_dir, 'transsWorldToCam.txt')
        self.result_dir = os.path.join(base_dir, 'result')
        self.src_keys = None
        self.counter = 0
        self.labels = []
        self.resize_xeye = resize_xeye

    def gen_test_data_dir_name(self):
        dir_name = os.path.join(self.base_dir, 'test_data')
        if os.path.exists(dir_name):
            namelist = [os.path.basename(e) for e in glob.glob(dir_name + '*')]
            residuals = [e[len('test_data'):] for e in namelist]
            nums = [int(e) for e in residuals if e != '']
            new_num = max(nums) + 1 if nums else 0
            dir_name += str(new_num)
        return dir_name

    def init_from_test_data(self, test_data_dir_name):
        """Init from test data after calibration."""
        assert(os.path.exists(self.intrinsic_path))
        self.test_data_dir = os.path.join(self.base_dir, test_data_dir_name)
        rgb_num = int(open(self.intrinsic_path).readline().rstrip())
        total_cam_num = len(glob.glob(os.path.join(self.test_data_dir, '*')))
        depth_cam_num = total_cam_num - rgb_num
        self.rgb_cam_list = range(rgb_num)
        self.rgb_of_depth_cam_list = range(rgb_num - depth_cam_num, rgb_num)
        self.counter = len(glob.glob(os.path.join(self.test_data_dir, '0', '*.jpg')))
        return self

    def mark_that_last_box_prediction_is_wrong(self):
        if self.counter < 2:
            return
        total_cam_num = len(self.rgb_cam_list) + len(self.rgb_of_depth_cam_list)
        for cam_id in range(total_cam_num):
            label_filename = str(10000000 + self.counter)[1:] + '.txt'
            with open(os.path.join(self.test_data_dir, str(cam_id), label_filename), 'a') as fout:
                fout.write('wrong_box+\n')
    def add_files(self, file_dict, label=''):
        from xeye_calib import resize_xeye_image_file
        if self.src_keys is None:
            self.src_keys, self.rgb_cam_list, self.rgb_of_depth_cam_list = init_cam_set(file_dict)
            self.src_keys_dict = {v: i for i, v in enumerate(self.src_keys)}
            logger.info('Init MultiCamGrabLabeler done.')
        filename = str(10000000 + self.counter)[1:] + '.jpg'
        label_filename = str(10000000 + self.counter)[1:] + '.txt'
        for k, v in file_dict.iteritems():
            
            if k.startswith('cam'):
                cam_id = self.src_keys_dict[k]
                dst_path = os.path.join(self.test_data_dir, str(cam_id), filename)
                if not os.path.exists(os.path.dirname(dst_path)):
                    os.makedirs(os.path.dirname(dst_path))
                # print v, dst_path
                shutil.copy(v, dst_path)
                if label:
                    with open(os.path.join(self.test_data_dir, str(cam_id), label_filename), 'w') as fout:
                        fout.write(label)
            elif k.startswith('xeye'):
                for i, imgpath in enumerate(v):
                    cam_id = self.src_keys_dict[('xeye_image', i)]
                    dst_path = os.path.join(self.test_data_dir, str(cam_id), filename)
                    if not os.path.exists(os.path.dirname(dst_path)):
                        os.makedirs(os.path.dirname(dst_path))
                    print(imgpath, dst_path)
                    if self.resize_xeye:
                        resize_xeye_image_file(imgpath, dst_path)
                    else:
                        shutil.copy(imgpath, dst_path)
                    if label:
                        with open(os.path.join(self.test_data_dir, str(cam_id), label_filename), 'w') as fout:
                            fout.write(label + '\n')
            else:
                logger.warn('Unrocognize key: {}'.format(k))
                return

        self.counter += 1

    def add_base64_files(self, file_dict, label=''):
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
                shutil.copy(v, dst_path)
            elif k.startswith('xeye'):
                for i, imgpath in enumerate(v):
                    cam_id = self.src_keys_dict[('xeye_image', i)]
                    dst_path = os.path.join(self.test_data_dir, str(cam_id), filename)
                    if not os.path.exists(os.path.dirname(dst_path)):
                        os.makedirs(os.path.dirname(dst_path))
                    print(imgpath, dst_path)
                    if self.resize_xeye:
                        resize_xeye_image_file(imgpath, dst_path)
                    else:
                        shutil.copy(imgpath, dst_path)
            else:
                logger.warn('Unrocognize key: {}'.format(k))
                return
        self.counter += 1

    def gen_label(self, counter=None):
        import infer_sequence
        if counter is None:
            counter = self.counter
        if counter > 1:
            rgb_file_seqs = []
            depth_file_seqs = []

            for i, cam_id in enumerate(self.rgb_cam_list):
                if len(rgb_file_seqs) <= i:
                    rgb_file_seqs.append([])
                filename = str(10000000 + counter - 2)[1:] + '.jpg'
                rgb_file_seqs[i].append(os.path.join(self.test_data_dir, str(cam_id), filename))
                filename = str(10000000 + counter - 1)[1:] + '.jpg'
                rgb_file_seqs[i].append(os.path.join(self.test_data_dir, str(cam_id), filename))

            for i, cam_id in enumerate(self.rgb_of_depth_cam_list):
                depth_cam_id = cam_id + len(self.rgb_of_depth_cam_list)
                print('i, cam_id', i, cam_id)
                if len(depth_file_seqs) <= i:
                    depth_file_seqs.append([])
                filename = str(10000000 + counter - 2)[1:] + '.jpg'
                depth_file_seqs[i].append(os.path.join(
                    self.test_data_dir, str(depth_cam_id), filename))
                filename = str(10000000 + counter - 1)[1:] + '.jpg'
                depth_file_seqs[i].append(os.path.join(
                    self.test_data_dir, str(depth_cam_id), filename))

            return infer_sequence.infer_parallel(self.intrinsic_path, self.extrinsic_path,
                                                 self.rgb_cam_list, self.rgb_of_depth_cam_list,
                                                 rgb_file_seqs, depth_file_seqs,
                                                 save_dir=self.result_dir,
                                                 show_each_step=False)

    def show_images(self, images, resize_ratio=0.5, cols=2):
        import infer_sequence
        logger.info('show image. image size: {}'.format(len(images)))
        return infer_sequence.show_multi_images(images, resize_ratio, cols)


def test_calib():
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
    output = subprocess.check_output(cmd)
    print(output)


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
    import infer_sequence
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
        images = labeler.gen_label()
        if images is None:
            continue
        infer_sequence.show_multi_images(images, resize_ratio=0.5, cols=4)


def rerun_multicamgrablabeler():
    """Generate simulate data_collection data and run calibrator."""
    import infer_sequence
    base_dir = 'data/batch4'
    test_dir = 'test_data'
    labeler = MultiCamGrabLabeler(base_dir).init_from_test_data(test_dir)

    images = labeler.gen_label(counter=2)
    print('images lengh', len(images))
    infer_sequence.show_multi_images(images, resize_ratio=0.5, cols=4)


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
    test_init_cam_set()
    # rerun_multicamgrablabeler()
    # print(test_multicamgrablabeler())
    # print(test_event())
