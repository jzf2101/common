#!/usr/bin/env python

from distutils.core import setup
from distutils.extension import Extension
from distutils.version import LooseVersion
from distutils.sysconfig import parse_makefile
from Cython.Distutils import build_ext
from Cython.Build import cythonize
from cython import __version__ as cython_version

import numpy
import sys
import os

clang = False
if sys.platform.lower().startswith('darwin'):
    clang = True

# make sure C shared library exists
libmicroscopes_common = 'out/libmicroscopes_common.dylib' if clang else 'out/libmicroscopes_common.so'
if not os.path.isfile(libmicroscopes_common):
    raise ValueError(
        "could not locate libmicroscopes_common shared object. make sure to run `make' first")

# append to library path
os.environ['LIBRARY_PATH'] = os.environ.get('LIBRARY_PATH', '') + ':out'

min_cython_version = '0.20.2b1' if clang else '0.20.1'
if LooseVersion(cython_version) < LooseVersion(min_cython_version):
    raise ValueError(
        'cython support requires cython>={}'.format(min_cython_version))

distributions_inc, distributions_lib = None, None
try:
    config = parse_makefile('../config.mk')
    distributions_inc = config.get('DISTRIBUTIONS_INC', None)
    distributions_lib = config.get('DISTRIBUTIONS_LIB', None)
except IOError:
    pass

if distributions_inc is not None:
    print 'Using distributions_inc:', distributions_inc
if distributions_lib is not None:
    print 'Using distributions_lib:', distributions_lib

extra_compile_args = ['-std=c++0x']
if clang:
    extra_compile_args.extend([
        '-mmacosx-version-min=10.7',  # for anaconda
        '-stdlib=libc++',
    ])

extra_include_dirs = []
if distributions_inc is not None:
    extra_include_dirs.append(distributions_inc)

extra_link_args = []
if distributions_lib is not None:
    extra_link_args.extend([
        '-L' + distributions_lib,
        '-Wl,-rpath,' + distributions_lib
    ])

def make_extension(module_name):
    sources = [module_name.replace('.', '/') + '.pyx']
    return Extension(
        module_name,
        sources=sources,
        libraries=["microscopes_common", "protobuf", "distributions_shared"],
        language="c++",
        include_dirs=[numpy.get_include(), 'include'] + extra_include_dirs,
        extra_compile_args=extra_compile_args,
        extra_link_args=extra_link_args)

extensions = cythonize([
    make_extension('microscopes.cxx.models'),
    make_extension('microscopes.cxx._models'),
    make_extension('microscopes.cxx.common.dataview'),
    make_extension('microscopes.cxx.common._dataview'),
    make_extension('microscopes.cxx.common.rng'),
    make_extension('microscopes.cxx.common._rng'),
])

setup(ext_modules=extensions)
