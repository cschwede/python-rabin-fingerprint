from distutils.core import setup, Extension
import os
from sys import platform 

os.environ['CFLAGS'] = "-std=c99"
if platform == "darwin":
    os.environ['CFLAGS'] += " -Qunused-arguments"

setup(name = "Indexer", version = "0.1", ext_modules = [Extension("rabin", ["src/rabin.c", ])])
