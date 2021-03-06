#!/usr/bin/python

from setuptools import setup
from distutils.extension import Extension
from Pyrex.Distutils import build_ext

setup(
    name="PyMoira",
    version="4.3.0",
    description="PyMoira - Python bindings for the Athena Moira library",
    author="Evan Broder",
    author_email="broder@mit.edu",
    license="MIT",
    py_modules=['moira'],
    ext_modules=[
        Extension("_moira",
                  ["_moira.pyx"],
                  libraries=["moira", "krb5"]),
        Extension("mrclient",
                  ["mrclient.pyx"],
                  libraries=["mrclient", "moira"]),
        ],
    scripts=['qy'],
    cmdclass= {"build_ext": build_ext}
)
