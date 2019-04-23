# -*- coding: utf-8 -*-
###############################################################################
#
# Copyright (c) 2016 Baidu.com, Inc. All Rights Reserved
#
###############################################################################
"""
Proved tools to deal with kv.

Authors: wangxiao05(wangxiao05@baidu.com)
Date:    2017/01/23 15:59:21
"""

from io import open
from struct import pack
from struct import unpack
import traceback
import sys
import os
import io
import typing
import logging


logger = logging.getLogger(__name__)

VERINFO = sys.version_info
PYTHON_VER = VERINFO.major


def is_stream(s):
    """Compatable code."""
    if VERINFO.major == 2 and isinstance(s, file):
        return True
    return isinstance(s, io.IOBase)


def convert_to_stream_in(s):
    """Convert bytes and files to python 2 and 3 compatable stream."""
    if isinstance(s, io.IOBase):
        if s.name == '<stdin>':
            return s.buffer
        return s
    if VERINFO.major == 2 and isinstance(s, file):
        return s
    if VERINFO.major == 3 and isinstance(s, bytes):
        return io.BytesIO(s)
    return open(s, 'rb')


def convert_to_stream_out(s):
    """Convert bytes and files to python 2 and 3 compatable stream."""
    if isinstance(s, io.IOBase):
        if s.name == '<stdout>':
            return s.buffer
        return s
    if VERINFO.major == 2 and isinstance(s, file):
        return s
    if VERINFO.major == 3 and isinstance(s, bytes):
        return io.BytesIO(s)
    return open(s, 'wb')


class KvIter(object):
    """Kv iterator on stream."""
    def __init__(self, arg=None):
        self.arg = arg
        self.count = 0

    def __call__(self, arg=None):
        if arg is None:
            f = convert_to_stream_in(self.arg)
        else:
            f = convert_to_stream_in(arg)
        len_buf = f.read(4)
        while True:
            s = len(len_buf)
            if s == 0:
                break
            assert len(len_buf) == 4, \
                'wrong len_buf size: {}, total kv read: {}'.format(len(len_buf), self.count)
            try:
                key_len = unpack('I', len_buf)[0]
                key = f.read(key_len)
                len_buf = f.read(4)
                val_len = unpack('I', len_buf)[0]
                val = f.read(val_len)
                self.count += 1
                yield key, val
                len_buf = f.read(4)
            except GeneratorExit:
                raise GeneratorExit()
            except:
                traceback.print_exc()
                pass


def iter_kv(arg):
    """Kv iterator on stream."""
    f = convert_to_stream_in(arg)
    len_buf = f.read(4)
    count = 0
    while True:
        s = len(len_buf)
        if s == 0:
            logger.debug('Func: iter_kv has seen {} kv pairs.'.format(count))
            break
        assert len(len_buf) == 4, \
            'wrong len_buf size: {}, total kv read: {}'.format(len(len_buf), count)
        try:
            key_len = unpack('I', len_buf)[0]
            key = f.read(key_len)
            len_buf = f.read(4)
            val_len = unpack('I', len_buf)[0]
            val = f.read(val_len)
            count += 1
            yield key, val
            len_buf = f.read(4)
        except GeneratorExit:
            raise GeneratorExit()
        except:
            traceback.print_exc()
            pass


def unpack_kv(stream):
    """Unpack stream to kv list."""
    return [(k, v) for k, v in iter_kv(stream)]



class KvBatchIter(object):
    """Kv iterator on stream."""

    def __init__(self, arg, batch_size=1, drop_last=False):
        self.arg = arg
        self.count = 0
        self.batch_count = 0
        self.batch_size = batch_size
        self.drop_last = drop_last

    def __call__(self, arg=None, batch_size=None, drop_last=None):
        arg = self.arg if arg is None else arg
        f = convert_to_stream_in(arg)
        batch_size = self.batch_size if batch_size is None else batch_size
        drop_last = self.drop_last if drop_last is None else drop_last

        len_buf = f.read(4)
        batch_key = []
        batch_val = []
        while True:
            s = len(len_buf)
            if s == 0:
                break
            assert len(len_buf) == 4, \
                'wrong len_buf size: {}, total kv read: {}'.format(len(len_buf), self.count)
            try:
                key_len = unpack('I', len_buf)[0]
                key = f.read(key_len)
                len_buf = f.read(4)
                val_len = unpack('I', len_buf)[0]
                val = f.read(val_len)
                self.count += 1
                batch_key.append(key)
                batch_val.append(val)
                assert len(batch_key) <= self.batch_size
                if len(batch_key) == self.batch_size:
                    self.batch_count += 1
                    yield batch_key, batch_val
                    batch_key = []
                    batch_val = []

                len_buf = f.read(4)
            except GeneratorExit:
                raise GeneratorExit()
            except:
                traceback.print_exc()
                pass
        if len(batch_key) > 0 and not drop_last:
            self.batch_count += 1
            yield batch_key, batch_val


def iter_batch(arg, batch_size=1, drop_last=False):
    """Generator of batch kv."""
    batchiter = KvBatchIter(arg, batch_size, drop_last)
    return batchiter()


def test_iter_batch(arg):
    batch_iter = KvBatchIter(arg, 10, False)
    for keys, vals in batch_iter():
        assert(len(keys) == len(vals))
        assert(len(keys) <= 10)
    print(batch_iter.batch_count)
    print(batch_iter.count)


if PYTHON_VER == 2:
    def append_kv(k, v, stream):
        """Pack kv and append it to stream."""
        stream.write(pack('I', len(k)) + k + pack('I', len(v)) + v)
        return stream

    def write_kv(k, v, stream):
        """Pack kv and append it to stream."""
        stream.write(pack('I', len(k)) + k + pack('I', len(v)) + v)
        stream.flush()
        return stream

    def pack_kv(kv_list, stream):
        """Pack a list of kvs and append the result to stream."""
        for k, v in kv_list:
            append_kv(str(k), str(v), stream)
        return stream
else:
    def append_kv(k, v, stream):
        """Pack kv and append it to stream."""
        if isinstance(k, typing.Text):
            k = k.encode('utf-8')
        stream.write(pack('I', len(k)) + k + pack('I', len(v)) + v)
        return stream

    def write_kv(k, v, stream):
        """Pack kv and append it to stream."""
        if isinstance(k, typing.Text):
            k = k.encode('utf-8')
        stream.write(pack('I', len(k)) + k + pack('I', len(v)) + v)
        stream.flush()
        return stream

    def pack_kv(kv_list, stream):
        """Pack a list of kvs and append the result to stream."""
        for k, v in kv_list:
            append_kv(str(k).encode('utf-8'), str(v).encode('utf-8'), stream)
        return stream





def sort_kv(input_stream, output_stream, cmp=None, key=None, reverse=False):
    """Sort kv from input_stream.

    Sort kv from input_stream by cmp function and append to output_stream
    cmp, key and reverse args used by sort are provided.
    """
    def mycmp(x, y):
        """self defined cmp."""
        if x == y:
            return 0
        if x < y:
            return 1
        if x > y:
            return -1
        return 0
    if cmp is None:
        cmp = mycmp
    kv_list = unpack_kv(input_stream)
    return pack_kv(sorted(kv_list, cmp=cmp, key=key, reverse=reverse),
                   output_stream)


def iter_kvgroup(stream, filter_func=None):
    """Iter kv group.

    All kvs with the which are adjacent and have the same k are deemed
        as a group,
    the group list are returned instread of the kv list
    filter_func can be set to filter out unintereting kvs
    """
    def _default_filter_func():
        return True

    if filter_func is None:
        filter_func = _default_filter_func
    pre_key = None
    kv_list = []
    for k, v in iter_kv(stream):
        if pre_key is None:
            kv_list.append((k, v))
        elif k != pre_key:
            if filter_func(kv_list) is True:
                yield kv_list
            kv_list = [(k, v)]
        else:
            kv_list.append((k, v))
        pre_key = k
    if len(kv_list) > 0 and filter_func(kv_list) is True:
        yield kv_list


def iter_uniq_kv(stream, filter_func=None, pick=0):
    """Iter on uniq kv.

    iterator on kv group, but return the picked element in the group.
    by default, the iterator will return the first element of the group
    """
    def _default_filter_func():
        return True

    if filter_func is None:
        filter_func = _default_filter_func
    for kv_list in iter_kvgroup(stream):
        if filter_func(kv_list) is True:
            yield kv_list[pick]


def iter_onekey_kv(stream):
    """Iter only on groups with one single key.

    Iterator on kv group, omitting the group with multiple kvs, and
    yield the only element
    """
    for k, v in iter_uniq_kv(stream, filter_func=lambda g: len(g) == 1):
        yield (k, v)


def iter_multikey_kv(stream, pick=0):
    """Iterate on kv group with multiple keys.

    Iterator on kv group, omitting the group with only one kv, and
    yield the picked element
    """
    for k, v in iter_uniq_kv(stream, lambda g: len(g) > 1, pick=pick):
        yield (k, v)


def traverse_dir(rootdir, filter_func=None):
    """Traverse the rootdir and return the filtered filelist."""
    def _default_filter_func():
        return True

    if filter_func is None:
        filter_func = _default_filter_func
    filelist = []
    for parent, dirnames, filenames in os.walk(rootdir):
        for filename in filenames:
            if filter_func(filename):
                filelist.append(os.path.join(parent, filename))
    return filelist


def iter_file(rootdir, func=None, endswith=None):
    """Iterator on the files in rootdir."""
    assert(endswith is None or type(endswith) is list
           or type(endswith) is set)

    def _filter_suffix(filename, endswith):
        if not endswith:
            return True
        suffix = os.path.splitext(filename)[1]
        if suffix in endswith or '.' + suffix in endswith:
            return True
        return False

    for parent, dirnames, filenames in os.walk(rootdir):
        for filename in filenames:
            if not _filter_suffix(filename, endswith):
                continue
            filepath = os.path.join(parent, filename)
            if func is None:
                yield filepath
            else:
                ret = func(filepath)
                yield filepath, ret


def truncate_kv(num, input_stream=None, output_stream=None):
    """Return front 'num' kv."""
    if input_stream is None:
        input_stream = sys.stdin

    if output_stream is None:
        output_stream = sys.stdout
    count = 0
    num = int(num)
    for k, v in iter_kv(input_stream):
        if count > num:
            return
        append_kv(k, v, output_stream)
        count += 1


def parallel_run(work_func, args_list, worker_num):
    """Simple parallel call func."""
    import multiprocessing
    from contextlib import contextmanager
    from inspect import getfullargspec

    def work_func_unpack(args):
        return work_func(*args)

    @contextmanager
    def poolcontext(*args, **kwargs):
        pool = multiprocessing.Pool(*args, **kwargs)
        yield pool
        pool.terminate()
    with poolcontext(processes=worker_num) as pool:
        args = getfullargspec(work_func)
        if len(args.args) > 1:
            results = pool.starmap(work_func, args_list)
        elif len(args.args) == 1:
            results = pool.map(work_func, args_list)
        else:
            results = pool.map(work_func)
    return results


if __name__ == '__main__':
    if len(sys.argv) > 1:
        func = getattr(sys.modules[__name__], sys.argv[1])
        func(*sys.argv[2:])
    else:
        print(sys.argv[0] + 'cmd [cmd_args]')
