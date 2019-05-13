# -*- coding: utf-8 -*-
###############################################################################
#
# Copyright (c) 2016 Baidu.com, Inc. All Rights Reserved
#
###############################################################################
"""
Gen label and timstamps.

Authors: wangxiao05(wangxiao05@baidu.com)
Date:    2018/08/21 18:09:54
"""

import time
import sys
import os
import json
import importlib
import logging
import shutil
from io import open


logger = logging.getLogger()
if not logger.handlers:
    logger.addHandler(logging.StreamHandler())
logger.setLevel(logging.INFO)


def get_remote_images(ip):
    import requests
    url = 'http://{}:8950/realsense_api/'.format(ip)
    # url = 'http://172.24.24.253:8950/realsense_api/'
    r = requests.get(url)

    d = r.json()
    return d


def check_calib_done(base_dir):
    """Return True if calib data exists."""
    if os.path.exists(os.path.join(base_dir, 'parameters')):
        return True
    else:
        return False


def check_calib_and_train(base_dir, collect_id):
    """Retrun True if calib data and train data have the same parameters."""
    if not check_calib_done(base_dir):
        logger.error('not calirated. run calibration first!')
        return False
    collect_data_dir = os.path.join(base_dir, 'collect_data', collect_id)

    def md5(fname):
        import hashlib
        if not os.path.exists(fname):
            return b'0'
        hash_md5 = hashlib.md5()
        with open(fname, "rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
                hash_md5.update(chunk)
        return hash_md5.hexdigest()
    train_data_dir = os.path.join(collect_data_dir, 'train_data')
    if not os.path.exists(train_data_dir):
        logger.error('training data not collected.')
        return False
    base_calib_file = os.path.join(base_dir, 'parameters', 'leftIntrinsic.txt')
    collect_para_file = os.path.join(collect_data_dir, 'parameters', 'leftIntrinsic.txt')
    if not os.path.exists(collect_para_file) or md5(base_calib_file) != md5(collect_para_file):
        logger.error('Calib data and collect calib data not the same.')
        return False
    return True


def calibrate(root_dir, *args):
    from functools import partial
    from align_all_sensor import Calibrator
    if root_dir == 'simulate':
        base_dir = '_simulate.data'
        sim_mod = importlib.import_module('simulate_cam_cap')
        get_calib_image = sim_mod.get_calib_image
        get_image = get_calib_image
    else:
        ip = args[0]
        get_image = partial(get_remote_images, ip=ip)
        get_calib_image = get_image

    if not os.path.exists(base_dir):
        os.makedirs(base_dir)
    calib = Calibrator(base_dir, resize_xeye=True)

    # throw garbage image
    throw_away = get_image()
    logger.debug('throw away: {}'.format(throw_away))
    throw_away = get_image()
    logger.debug('throw away: {}'.format(throw_away))

    logger.info('Enter calibration mode')
    if not check_calib_done(base_dir):
        logger.info('Camsets already calibrated, Recalibrating...')

    prefix = 'calib'
    filename = os.path.join(base_dir, 'gen_tm.txt')

    with open(filename, 'a') as fout:
        inp = None
        items = []
        line = 'start recording calibration data ...'
        logger.info('line: {}'.format(line))
        fout.write(line + '\n')
        fout.flush()
        while True:

            logger.info('You can choose:')
            logger.info('  1. input "e" to end calib')
            logger.info('  2. type Return to capture images...')

            inp = input()
            if inp == '':
                tm = str(int(time.time() * 1000))
                items.insert(0, tm)
                items.insert(0, prefix)
                line = '\t'.join(items)
                logger.info('line: {}'.format(line))
                file_dict = get_calib_image()

                logger.info('check file_dict: {}'.format(file_dict.keys()))
                calib.add_files(file_dict)
                items = []
            elif inp == 'e':
                logger.info('start calibration ...')
                [status, output] = calib.gen_calib_parameters()
                logger.info('calibration status: {}'.format(status))
                # logger.info('calibration output: {}'.format(output))
                # fout.write('calibration output: {}'.format(output))
                assert(status == 0)
                logger.info('Calibration done!')
                return


def collect_train(root_dir, *args):
    """Data collection pipeline with label"""
    from functools import partial
    from align_all_sensor import MultiCamGrabLabeler
    if root_dir == 'simulate':
        root_dir = '_simulate.data'
        sim_mod = importlib.import_module('simulate_cam_cap')
        get_collection_image = sim_mod.get_collection_image
        get_image = get_collection_image
        current_label = '00000000'
    else:
        ip, current_label = args[:2]
        get_image = partial(get_remote_images, ip=ip)
        get_collection_image = get_image

    if not check_calib_done(root_dir):
        logger.info('Error. Camera not calibrated. Run calib first.')
        return
    assert(current_label != '')
    base_dir = os.path.join(root_dir, 'collect_data', current_label)
    if os.path.exists(base_dir):
        shutil.rmtree(base_dir)
        while os.path.exists(base_dir):
            time.sleep(0.1)
    os.makedirs(base_dir)
    shutil.copytree(os.path.join(root_dir, 'calib_data'), os.path.join(base_dir, 'calib_data'))
    shutil.copytree(os.path.join(root_dir, 'parameters'), os.path.join(base_dir, 'parameters'))
    labeler = MultiCamGrabLabeler(base_dir, data_dir='train_data', resize_xeye=True)
    # throw garbage image
    throw_away = get_image()
    logger.debug('throw away: {}'.format(throw_away))
    throw_away = get_image()
    logger.debug('throw away: {}'.format(throw_away))

    background_image_taken = False

    items = []
    line = 'start recording event data ...'
    prefix = 'collection'
    logger.info('line: {}'.format(line))
    valid_image_count = 0
    while True:

        if background_image_taken:
            logger.info('----------------------------------------------------------')
            logger.info('current label: {}. Collected: {}'.format(current_label, valid_image_count))
            if current_label:
                logger.info('  * input "e" to end program.')
                logger.info(
                    '  * press Return to collect image of current label.'.format(current_label))
                logger.info(
                    '  * input "-" to indicate last box is not right.'.format(current_label))
        else:
            while True:
                logger.info('Press Return to take the first background image...')
                inp = input()
                if inp != '':
                    logger.info('Press Return to take the first background image...')
                    continue
                else:
                    file_dict = get_collection_image()
                    labeler.add_files(file_dict, inp)
                    results = labeler.gen_label()
                    items = []
                    background_image_taken = True
                    break
            continue
        inp = input()
        if inp == '-':
            labeler.mark_that_last_box_prediction_is_wrong()
            if valid_image_count > 0:
                valid_image_count -= 1
        elif inp == '':
            tm = str(int(time.time() * 1000))
            items.insert(0, tm)
            items.insert(0, prefix)
            line = '\t'.join(items)
            logger.info('line: {}'.format(line))
            file_dict = get_collection_image()
            labeler.add_files(file_dict, current_label)
            results = labeler.gen_label()
            images = []
            for r in results:
                images.extend(r['images'])
            labeler.show_images(images)
            items = []
            valid_image_count += 1
        elif inp == 'e':
            logger.info('Program end')
            return
        else:
            logger.info('Invalid input, omit!')


def collect_test(root_dir, *args):
    """Data collection pipeline with label"""
    from functools import partial
    from align_all_sensor import MultiCamGrabLabeler
    if root_dir == 'simulate':
        root_dir = '_simulate.data'
        sim_mod = importlib.import_module('simulate_cam_cap')
        get_collection_image = sim_mod.get_collection_image
        get_image = get_collection_image
        current_label = '00000000'
    else:
        ip, current_label = args[:2]
        get_image = partial(get_remote_images, ip=ip)
        get_collection_image = get_image
    assert(current_label != '')
    base_dir = os.path.join(root_dir, 'collect_data', current_label)

    if not check_calib_and_train(root_dir, current_label):
        return

    labeler = MultiCamGrabLabeler(base_dir, data_dir='test_data', resize_xeye=True)
    labeler.mode = 3
    # throw garbage image
    throw_away = get_image()
    logger.debug('throw away: {}'.format(throw_away))
    throw_away = get_image()
    logger.debug('throw away: {}'.format(throw_away))

    items = []
    line = 'start recording event data ...'
    prefix = 'collection'
    logger.info('line: {}'.format(line))
    valid_image_count = 0
    while True:
        logger.info('----------------------------------------------------------')
        logger.info('current label: {}. Collected: {}'.format(current_label, valid_image_count))
        if not labeler.background_taken:
            logger.info(
                '  * input "-" to indicate last collected product is not right.')
            logger.info('  * input Return to take the background image...')
        else:
            logger.info('  * input Return to take the product image...')
        logger.info('  * input "e" to end program.')

        inp = input()
        if inp == '-':
            if not labeler.background_taken:
                labeler.mark_that_last_box_prediction_is_wrong()
                if valid_image_count > 0:
                    valid_image_count -= 1
            else:
                logger.info('Invalid input, omit!')
        elif inp == '':
            tm = str(int(time.time() * 1000))
            items.insert(0, tm)
            items.insert(0, prefix)
            line = '\t'.join(items)
            logger.info('line: {}'.format(line))
            file_dict = get_collection_image()
            labeler.add_files(file_dict, current_label)
            if not labeler.background_taken:
                results = labeler.gen_label()
                images = []
                for r in results:
                    images.extend(r['images'])
                labeler.show_images(images)
            items = []
            valid_image_count += 1
        elif inp == 'e':
            logger.info('Program end')
            return
        else:
            logger.info('Invalid input, omit!')


def collect(*args):
    """Data collection pipeline with label"""
    logger.info('Start collecting training data...')
    collect_train(*args)
    logger.info('Start collecting testing data...')
    collect_test(*args)


if __name__ == '__main__':

    if len(sys.argv) > 1:
        func = getattr(sys.modules[__name__], sys.argv[1])
        func(*sys.argv[2:])
    else:
        print(sys.argv[0] + 'python {} calibrate root_dir ip ')


    # collect_test()