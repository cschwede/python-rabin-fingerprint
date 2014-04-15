from distutils.core import setup, Extension
import os

os.environ['CFLAGS'] = "-Qunused-arguments"
setup(name = "Indexer", version = "0.1", ext_modules = [Extension("rabin", ["src/rabin.c", ])])
