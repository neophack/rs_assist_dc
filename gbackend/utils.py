# -*- coding: utf-8 -*-
###############################################################################
#
# Copyright (c) 2018 wbnupku. All Rights Reserved
#
###############################################################################
"""
This module provides common utils

Authors: wbnupku(wbnupku@outlook.com)
Date:    2018/09/17 15:10:19
"""
import yaml
import logging
import logging.config


def create_logger(filepath='config/logging.yml'):
    """Return logger defined by yml."""
    logging.config.dictConfig(yaml.load(open('config/logging.yml')))
    logger = logging.getLogger()
    return logger


def create_multiprocessing_logger(filepath='config/logging.yml'):
    """Return logger defined by yml."""
    import multiprocessing_logging
    logging.config.dictConfig(yaml.load(open('config/logging.yml')))
    logger = logging.getLogger()
    multiprocessing_logging.install_mp_handler()
    return logger

# gunicorn_logger = create_logger()
