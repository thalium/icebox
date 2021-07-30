#!/usr/bin/env python

from distutils.core import setup

setup(
    name="icebox",
    version="1.0",
    description="Icebox VMI bindings",
    author="Benoît Amiaux",
    author_email="benoit.amiaux@gmail.com",
    url="https://github.com/thalium/icebox",
    packages=["icebox"],
    package_data={
        "icebox": ["icebox/libicebox.pyd"],
    },
)
