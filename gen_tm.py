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
# from aicam_server.cam_cap import get_image
import importlib
import logging
import shutil

logger = logging.getLogger()
if not logger.handlers:
    logger.addHandler(logging.StreamHandler())
logger.setLevel(logging.INFO)


def run():
    """Generate label and timestamp."""
    if 1 or len(sys.argv) == 1:
        filename = 'test_gen_tm.txt'
    else:
        filename = sys.argv[1]

    with open(filename, 'ab') as fout:
        items = []
        prefix = 'event'
        while True:
            print('input PRODUCT_ID, "calib", "event", "e" or type Return ...')

            inp = input()
            if inp == '':
                tm = str(int(time.time() * 1000))
                items.insert(0, tm)
                items.insert(0, prefix)
                line = '\t'.join(items)
                print(line)
                print(line, file=fout)
                items = []
            elif inp == 'calib':
                items = []
                line = 'start recording calibration data ...'
                print(line)
                prefix = 'calib'
                print(line, file=fout)
            elif inp == 'event':
                items = []
                line = 'start recording event data ...'
                prefix = 'event'
                print(line)
                print(line, file=fout)
            elif inp != 'e':
                items.append(inp)
            else:
                print('Program end')
                sys.exit(0)


def run1():
    """Data collection pipeline."""
    from align_all_sensor import reconstruct_calib_data
    from align_all_sensor import Calibrator
    from align_all_sensor import MultiCamGrabLabeler
    aicam_server = importlib.import_module('aicam-server')
    get_image = aicam_server.cam_cap.get_image
    base_dir = sys.argv[1]
    calib = Calibrator(base_dir, resize_xeye=True)
    labeler = MultiCamGrabLabeler(base_dir, resize_xeye=True)
    # throw garbage image
    throw_away = get_image()
    print('throw away:', throw_away)
    throw_away = get_image()
    print('throw away:', throw_away)

    if 1 or len(sys.argv) == 1:
        filename = 'test_gen_tm.txt'
    else:
        filename = sys.argv[1]

    with open(filename, 'ab') as fout:
        items = []
        prefix = 'event'
        while True:
            print('input PRODUCT_ID, "calib", "event", "e" or type Return ...')

            inp = input()
            if inp == '':
                tm = str(int(time.time() * 1000))
                items.insert(0, tm)
                items.insert(0, prefix)
                line = '\t'.join(items)
                print(line)
                print(line, file=fout)
                file_dict = get_image()
                if prefix == 'calib':
                    print('file_dict', json.dumps(file_dict, indent=2))
                    calib.add_files(file_dict)
                else:
                    print('add file_dict to labeler')
                    print('file_dict', json.dumps(file_dict, indent=2))
                    labeler.add_files(file_dict)
                    images = labeler.gen_label()
                    labeler.show_images(images)
                items = []
            elif inp == 'calib':
                items = []
                line = 'start recording calibration data ...'
                print(line)
                prefix = 'calib'
                print(line, file=fout)
            elif inp == 'event':
                if prefix == 'calib':
                    calib.cp_files()
                    [status, output] = calib.gen_calib_parameters()
                    print(status)
                    print(output)
                    assert status == 0

                items = []
                line = 'start recording event data ...'
                prefix = 'event'

                print(line)
                print(line, file=fout)
            elif inp != 'e':
                items.append(inp)
            else:
                print('Program end')
                sys.exit(0)


def run2():
    """Data collection pipeline with label"""
    from align_all_sensor import Calibrator
    from align_all_sensor import MultiCamGrabLabeler
    if sys.argv[1] == 'simulate':
        base_dir = '_simulate.data'
        sim_mod = importlib.import_module('simulate_cam_cap')
        get_calib_image = sim_mod.get_calib_image
        get_collection_image = sim_mod.get_collection_image
        get_image = get_collection_image
    else:
        base_dir = sys.argv[1]
        cam_cap = importlib.import_module('aicam-server.cam_cap')
        get_image = cam_cap.get_image
        get_calib_image = get_image
        get_collection_image = get_image

    if not os.path.exists(base_dir):
        os.makedirs(base_dir)
    calib = Calibrator(base_dir, resize_xeye=True)
    labeler = MultiCamGrabLabeler(base_dir, resize_xeye=True)
    # throw garbage image
    throw_away = get_image()
    logger.debug('throw away: {}'.format(throw_away))
    throw_away = get_image()
    logger.debug('throw away: {}'.format(throw_away))

    filename = os.path.join(base_dir, 'gen_tm.txt')
    if not os.path.exists(os.path.join(base_dir, 'parameters')) or \
            len(sys.argv) > 2 and sys.argv[2] == 'calib':
        logger.info('Enter calibration mode')
        calib_dir = os.path.join(base_dir, 'calib_data')
        if os.path.exists(calib_dir):
            shutil.rmtree(calib_dir)
        param_dir = os.path.join(base_dir, 'parameters')
        if os.path.exists(param_dir):
            shutil.rmtree(param_dir)
        prefix = 'calib'
    else:
        prefix = 'collection'
        logger.info('Camsets already calibrated, enter data collection mode')
        logger.info('If you want to recalibrate, restart with "python gen_tm $dst_folder calib"')
    background_image_taken = False
    current_label = ''
    with open(filename, 'a') as fout:
        items = []
        while True:
            if prefix == 'calib':
                logger.info('You can choose:')
                logger.info('  1. input "e" to end calib')
                logger.info('  2. type Return to capture images...')
            else:
                if background_image_taken:
                    logger.info('----------------------------------------------------------')
                    logger.info('current label: {}. You should do ...'.format(current_label))
                    logger.info('  * input scan a PRODUCT_ID to set label,')
                    if current_label:
                        logger.info('  * input "e" to end program.')
                        logger.info('  * press Return to collect image of current label.'.format(current_label))
                        logger.info('  * input "-" to indicate last box is not right.'.format(current_label))
                else:
                    while True:
                        logger.info('Press Return to take the first background image...')
                        inp = input()
                        if inp != '':
                            continue
                        else:
                            file_dict = get_collection_image()
                            labeler.add_files(file_dict, inp)
                            images = labeler.gen_label()

                            items = []
                            background_image_taken = True
                            break
                    continue
            inp = input()
            if inp == '-':
                labeler.mark_that_last_box_prediction_is_wrong()
            if (len(inp) > 5 and inp.isdigit()):
                current_label = inp
                logger.info('set current_label to: {}'.format(current_label))
            elif inp == '':
                tm = str(int(time.time() * 1000))
                items.insert(0, tm)
                items.insert(0, prefix)
                line = '\t'.join(items)
                logger.info('line: {}'.format(line))
                print(line, file=fout)
                if prefix == 'calib':
                    file_dict = get_calib_image()
                else:
                    file_dict = get_collection_image()
                # print 'file_dict', json.dumps(file_dict, indent=2)
                if prefix == 'calib':
                    # print 'calib add file_dict', json.dumps(file_dict, indent=2)
                    calib.add_files(file_dict)
                else:

                    # print 'file_dict', json.dumps(file_dict, indent=2)

                    labeler.add_files(file_dict, current_label)
                    images = labeler.gen_label()
                    labeler.show_images(images)
                items = []
            elif inp == 'calib':
                items = []
                line = 'start recording calibration data ...'
                logger.info('line: {}'.format(line))
                prefix = 'calib'
                print(line, file=fout)
            elif inp == 'collection':
                if prefix == 'calib':
                    logger.info('If you want to end calibration, input "e" and Return.')
                    continue

                items = []
                line = 'start recording event data ...'
                prefix = 'collection'

                logger.info('line: {}'.format(line))
                print(line, file=fout)
            elif inp != 'e':
                items.append(inp)
            else:
                if prefix == 'calib':
                    logger.info('start calibration ...')
                    calib.cp_files()
                    [status, output] = calib.gen_calib_parameters()
                    logger.info('calibration status: {}'.format(status))
                    logger.info('calibration output: {}'.format(output))
                    assert status == 0
                    items = []
                    line = 'start recording event data ...'
                    prefix = 'collection'

                    continue
                logger.info('Program end')
                sys.exit(0)


if __name__ == '__main__':
    run2()
