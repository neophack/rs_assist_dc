# -*- coding: utf-8 -*-
###############################################################################
#
# Copyright (c) 2016 Baidu.com, Inc. All Rights Reserved
#
###############################################################################
"""
Simple flask-restful example.

Authors: wangxiao05(wangxiao05@baidu.com)
Date:    2018/07/23 14:55:56
"""
from flask import Flask
from flask_restful import reqparse
from flask_restful import Api
from flask_restful import Resource
from myworker import ImageTeller as ActualWoker
from utils import create_logger


def create_app():
    """Create flask simple api."""
    app = Flask(__name__)
    create_logger()
    api = Api(app)
    # define request

    # Init a obj and add it to app.
    # This can be put into another file.
    app.my_worker = ActualWoker()
    return app, api


parser = reqparse.RequestParser()
parser.add_argument('image_b64', type=str, help='base64-ed image')


class SimpleAPI(Resource):

    def __init__(self):
        from flask import current_app
        self.logger = current_app.logger
        self.parser = parser
        self.logger.debug("init simpleapi")

    def get(self):
        return dict(info='This is a simple api for wrapping calculation.')

    def post(self):
        from flask import current_app
        args = self.parser.parse_args()
        self.logger.info('args:' + repr(args.keys()))
        return current_app.my_worker.run(args), 201


class RealsenseAPI(Resource):

    def __init__(self):
        from flask import current_app
        self.logger = current_app.logger

    def get(self):
        import rs_server
        return rs_server.RealsenseImageProvider().run()


##
# Actually setup the Api resource routing here
##
if __name__ == '__main__':
    import logging
    logger = logging.getLogger()
    logger.addHandler(logging.StreamHandler())
    app, api = create_app()
    # api.add_resource(SimpleAPI, '/simple_api/')
    api.add_resource(RealsenseAPI, '/realsense_api/')
    app.run(debug=True, port=8950)
