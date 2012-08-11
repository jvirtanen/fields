#!/usr/bin/env python

from distutils.core import setup

setup(
    name='fields',
    version='0.4.0',
    description='Python binding to Fields, ' \
        + 'the fast C library for reading CSV and other tabular text formats',
    url='https://github.com/jvirtanen/fields',
    license='MIT',
    author='Jussi Virtanen',
    author_email='jussi.k.virtanen@gmail.com',
    packages=['fields']
)
