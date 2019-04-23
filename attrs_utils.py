# -*- coding: utf-8 -*-
"""
This module provides interfacing paradigm.

Filename: attrs_utils.py
Authors:  wangxiao(wangxiao05@baidu.com)
Date:     2018/12/10 21:20:21
"""

from __future__ import absolute_import
from __future__ import print_function
from __future__ import unicode_literals


import importlib
import typing
import attr
import builtins
from cattr import Converter
import numpy as np
import functools as f

import json
import base64
from struct import pack
from struct import unpack
from io import open
import sys
import logging


VERINFO = sys.version_info
if VERINFO.major == 2:
    import cPickle as pickle
    pickle_loads = pickle.loads
    pickle_load = pickle.load
    pickle_dumps = f.partial(pickle.dumps, protocol=2)
    pickle_dump = f.partial(pickle.dump, protocol=2)
else:
    import pickle
    pickle_loads = f.partial(pickle.loads, encoding='bytes')
    pickle_load = f.partial(pickle.load, encoding='bytes')
    pickle_dumps = f.partial(pickle.dumps, protocol=2)
    pickle_dump = f.partial(pickle.dump, protocol=2)


def u2c(value):
    """Convert underscore string to capitalized string."""
    # Make a list of capitalized words and underscores to be preserved
    capitalized_words = [w.capitalize() if w else '_' for w in value.split('_')]
    return "".join(capitalized_words)


CUSTOM_CVT = Converter()
CUSTOM_CVT.register_unstructure_hook(
    np.ndarray, lambda a: dict(dtype=a.dtype.name, data=a.tobytes(), shape=list(a.shape)))


CUSTOM_CVT.register_structure_hook(
    np.ndarray, lambda a, _: np.frombuffer(a['data'], dtype=a['dtype']).reshape(tuple(a['shape'])))


def to_dict(obj):
    """Convert object to dict"""
    global CUSTOM_CVT
    return CUSTOM_CVT.unstructure(obj)


def from_dict(d, cls, compatible=True):
    """Convert dict to obj of class."""
    global CUSTOM_CVT
    if compatible and hasattr(cls, '__attrs_attrs__'):
        nd = {}
        for a in cls.__attrs_attrs__:
            if a.name not in d:
                continue
            nd[a.name] = d[a.name]
        # nd = {a.name: d[a.namee] for a in cls.__attrs_attrs__ if a.name in d}
        return CUSTOM_CVT.structure(nd, cls)
    return CUSTOM_CVT.structure(d, cls)


class BytesEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, builtins.bytes):
            return {'b64': base64.b64encode(obj).decode('utf-8')}
        # Let the base class default method raise the TypeError
        return json.JSONEncoder.default(self, obj)


def _json_load_as_bytes(dct):
    if 'b64' in dct:
        return base64.b64decode(dct['b64'].encode('utf-8'))
    return dct


def _json_dumpu_py2(d, *args, **kwargs):
    """Return unicode in py2 and str in py3."""
    return json.dumps(d).decode('utf-8')


if VERINFO.major == 2:
    _json_dumpu = _json_dumpu_py2
else:
    _json_dumpu = f.partial(json.dumps)


def to_json(obj):
    """To json."""
    return _json_dumpu(to_dict(obj), cls=BytesEncoder)


def dict_from_json(data):
    """Return a dict."""
    return json.loads(data, object_hook=_json_load_as_bytes)


def dict_to_json(d):
    """Dumps dict as json with bytes as {'b64': ...}."""
    return _json_dumpu(d, cls=BytesEncoder)


def from_json(data, cls, compatible=True):
    """from json."""
    return from_dict(dict_from_json(data), cls, compatible=compatible)


def to_bytes(obj):
    """To bytes."""
    return pickle_dumps(to_dict(obj))


def dict_from_bytes(b):
    """Return a dict."""
    return pickle_loads(b)


def dict_to_bytes(b):
    """Return a dict."""
    return pickle_dumps(b)


def from_bytes(b, cls, compatible=True):
    """Return obj of cls parsed from bytes."""
    return from_dict(dict_from_bytes(b), cls, compatible)


def gen_class_code(key, d, c=''):
    """Return code string."""
    assert isinstance(d, dict)
    if c == '':
        add_head = 'import builtins\nimport typing\nimport attr\n'
    else:
        add_head = ''
    content = ''
    content += '\n@attr.s\n'
    content += 'class {}(object):\n'.format(key)
    content += '    """Define stucture."""\n'
    embed_cls_info = []
    for k, v in d.items():
        if isinstance(v, dict):
            if 'b64' in v:
                content += '    {} = attr.ib(type=builtins.bytes)\n'.format(k)
            else:
                classname = key + u2c(k)
                content += '    {} = attr.ib(type={})\n'.format(k, classname)
                embed_cls_info.append((classname, v))
        elif isinstance(v, int):
            content += '    {} = attr.ib(type=int)\n'.format(k)
        elif isinstance(v, float):
            content += '    {} = attr.ib(type=float)\n'.format(k)
        elif isinstance(v, str):
            content += '    {} = attr.ib(type=typing.Text)\n'.format(k)
        elif isinstance(v, list):
            assert v
            v0 = v[0]
            if isinstance(v0, dict):
                if 'b64' in v:
                    classname = 'builtins.bytes'
                    typename = 'typing.List[{}]'.format(classname)
                else:
                    classname = key + u2c(k)
                    typename = 'typing.List[{}]'.format(classname)
                    embed_cls_info.append((classname, v0))
            elif isinstance(v0, int):
                typename = 'int'
            elif isinstance(v0, float):
                typename = 'float'
            elif isinstance(v0, str):
                typename = 'str'
            else:
                raise TypeError("typename: {}, content: {}".format(repr(type(v0)), repr(v0)))
            content += '    {} = attr.ib(type={})\n'.format(k, typename)
        else:
            raise TypeError("typename: {}, content: {}".format(repr(type(v0)), repr(v0)))
    c = content + '\n\n' + c

    for k, v in reversed(embed_cls_info):
        c = gen_class_code(k, v, c)
    return add_head + c


def create_class_from_dict(key, d):
    """Return a attrs-defined object."""
    # TODO
    #   list of list
    logger = logging.getLogger(__name__)
    attribute_dict = {}
    for k, v in d.items():
        print('key: {}, type v: {}'.format(key, type(v)))
        logger.debug('key: {}, type v: {}'.format(key, type(v)))
        if isinstance(v, dict):
            if 'b64' in v:
                attribute_dict[k] = attr.ib(type=builtins.bytes)
            else:
                classname = key + u2c(k)
                attribute_dict[k] = attr.ib(type=create_class_from_dict(classname, v))
        elif isinstance(v, int):
            attribute_dict[k] = attr.ib(type=int)
        elif isinstance(v, float):
            attribute_dict[k] = attr.ib(type=float)
        elif isinstance(v, str):
            attribute_dict[k] = attr.ib(type=typing.Text)
        elif isinstance(v, list):
            assert v
            v0 = v[0]
            if isinstance(v0, dict):
                if 'b64' in v:
                    embed_class = builtins.bytes
                else:
                    classname = key + u2c(k)
                    embed_class = create_class_from_dict(classname, v0)
            elif isinstance(v0, int):
                embed_class = int
            elif isinstance(v0, float):
                embed_class = float
            elif isinstance(v0, str):
                embed_class = str
            else:
                raise TypeError("typename: {}, content: {}".format(repr(type(v0)), repr(v0)))
            attribute_dict[k] = attr.ib(type=typing.List[embed_class])
        else:
            raise TypeError("typename: {}, content: {}".format(repr(type(v)), repr(v)))
    c = attr.make_class(key, attribute_dict)
    print('attribute_dict: {}'.format(attribute_dict))
    return c


def _attr_decoder(arg):
    import functools as f
    return f.partial(from_bytes, cls=arg)


def iter_obj(arg, key_decoder=None, val_decoder=None):
    """Return iterator of objects."""
    from .kvtools import iter_kv

    def _decode_as_utf8(s):
        return s.decode('utf-8')

    if key_decoder is None and val_decoder is None:
        for k, v in iter_kv(arg):
            yield k, v
    if hasattr(key_decoder, '__attrs_attrs__'):
        key_decoder = _attr_decoder(key_decoder)
    elif key_decoder == 'str':
        key_decoder = _decode_as_utf8
    elif key_decoder is None:
        key_decoder = bytes
    else:
        raise TypeError('Not supported decoder: {}'.format(key_decoder))
    if hasattr(val_decoder, '__attrs_attrs__'):
        val_decoder = _attr_decoder(val_decoder)
    elif val_decoder == 'str':
        val_decoder = _decode_as_utf8
    elif val_decoder is None:
        val_decoder = bytes
    else:
        raise TypeError('Not supported decoder: {}'.format(val_decoder))

    for k, v in iter_kv(arg):
        nk, nv = key_decoder(k), val_decoder(v)
        yield nk, nv


def _create_test_case():
    test_case_req = {
        "imagelist_b64":
        {
            "signature_name": "classify_objects",
            "context": {
                'topk': 5
            },
            "examples": [
                {
                    "image": {"b64": "aW1hZ2UgYnl0ZXM="},
                },
                {
                    "image": {"b64": "YXdlc29tZSBpbWFnZSBieXRlcw=="},
                }
            ]
        }
    }

    test_case_resp = {
        "result": [
            [["6901898886636", "1.00", "華旗-山楂果茶-经典原味-400ml"],
             ["6910160313513", "0.00", "丘比-草莓果酱-170g"]],
            [["6901898886636", "1.00", "華旗-山楂果茶-经典原味-400ml"],
             ["6910160313513", "0.00", "丘比-草莓果酱-170g"]],
        ]
    }
    return test_case_req, test_case_resp


@attr.s
class RespJsonObject(object):
    """Self defined Resp attrs class."""
    result = attr.ib(type=typing.List[typing.List[typing.List[str]]])


def test_py2n3():
    """Unit test."""
    a = {'x': b'b'.decode('utf-8'), 'y': 'b', 'z': 1}
    # result = json.dumps(a, indent=2).decode('utf-8')
    # print(type(result))
    # b = json.loads(result)
    # for k, v in b.items():
    #     print('k, type:{}, {}'.format(type(k), k))
    #     print('v, type:{}, {}'.format(type(v), v))

    result = bytes(pickle.dumps(a))
    if VERINFO.major == 2:
        result = bytes(pickle.dumps(a))
        with open('py2.pkl', 'wb') as fout:
            fout.write(result)
    else:
        result = bytes(pickle.dumps(a, protocol=2))
        with open('py3.pkl', 'wb') as fout:
            fout.write(result)
    print(type(result))
    if VERINFO.major == 2:
        b = pickle.load(open('py3.pkl', 'rb'))
        print(bytes == str)
        for k, v in b.items():
            print('k, type:{}, {}'.format(type(k), k))
            print('v, type:{}, {}'.format(type(v), v))
    else:
        b = pickle.load(open('py2.pkl', 'rb'), encoding='bytes')
        print(bytes == str)
        for k, v in b.items():
            print('k, type:{}, {}'.format(type(k), k))
            print('v, type:{}, {}'.format(type(v), v))


def test_create():
    """Unit test."""
    test_case_req, _ = _create_test_case()
    C = create_class_from_dict('RequestJsonObject', test_case_req)
    c = from_json(json.dumps(test_case_req), C)
    assert to_json(c) == json.dumps(test_case_req)
    assert from_json(json.dumps(test_case_req), C) == c


def test_gen_class_code():
    """Unit test."""
    test_case_req, _ = _create_test_case()
    with open('temp_request_json_object.py', 'w') as fout:
        print(gen_class_code('RequestJsonObject', test_case_req), file=fout)
    mod = importlib.import_module('temp_request_json_object')
    C = mod.RequestJsonObject
    assert to_json(from_json(json.dumps(test_case_req), C)) == json.dumps(test_case_req)


def test_resp():
    """Unit test."""
    _, test_case_resp = _create_test_case()
    resp_obj = from_json(json.dumps(test_case_resp), RespJsonObject)
    assert to_json(resp_obj) == json.dumps(test_case_resp)


if __name__ == '__main__':
    test_py2n3()
