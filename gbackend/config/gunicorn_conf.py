# -*- coding: utf-8 -*-
###############################################################################
#
# Copyright (c) 2018 wbnupku. All Rights Reserved
#
###############################################################################
"""
Gunicorn configure file.

Authors: wbnupku(wbnupku@outlook.com)
Date:    2018/09/17 16:53:00
"""


# The socket to bind. A string of the form: HOST, HOST:PORT, unix:PATH. An IP is a valid HOST
# default = ['127.0.0.1:8000']
bind = '0.0.0.0:8950'

# logging
# The Access log file to write to. '-' means log to stdout.
# accesslog = None
#
# access_log_format = %(h)s %(l)s %(u)s %(t)s "%(r)s" %(s)s %(b)s "%(f)s" "%(a)s"
accesslog = 'log/gbackend.access.log'
access_log_format = ("[gbackend] %(p)s %(h)s %(l)s %(u)s %(t)s %(D)sms .%(r)s. %(U)s "
                     "%(q)s %(s)s %(b)s .%(f)s. .%(a)s. conn=\"%({Connection}i)s\" "
                     "%({Header}i)s %({Header}o)s")

# error log
errorlog = 'log/gbackend.error.log'
loglevel = 'INFO'
capture_output = True
logger_class = 'utils.gunicorn_logger'
logconfig = None


# pidfile
# default: None
pidfile = 'pid.txt'

preload_app = True
daemon = True

# workers
# alternatives: sync, eventlet, gevent
worker_class = 'gthread'

# The number of worker processes for handling requests.
# default: 1
workers = 1

# The number of worker threads for handling requests. Only affects the Gthread worker type
# default: 1
threads = 1

# Workers silent for more than this many seconds are killed and restarted.
# default: 30
timeout = 30
