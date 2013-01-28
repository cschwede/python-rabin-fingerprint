from distutils.core import setup, Extension

setup(name = "Indexer", version = "0.1", ext_modules = [Extension("rabin", ["src/rabin.c", ])])
