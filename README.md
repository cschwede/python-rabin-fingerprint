python-rabin-fingerprint
========================

[![Build Status](https://travis-ci.org/cschwede/python-rabin-fingerprint.svg?branch=master)](https://travis-ci.org/cschwede/python-rabin-fingerprint)

Python C extension to find chunk boundaries using a Rabin-Karp rolling hash.
This is useful to slice data into variable sized chunks based on the content.
If a file is changed the modified chunk (and maybe the next one) is affected,
but not the following chunks. This makes it useful to apply data deduplication
before sending data over slow connections or storing multiple similar files
(like backups using tar snapshots).

Have a look at http://en.wikipedia.org/wiki/Rolling_hash for an introduction
to the Rabin-Karp rolling hash.

Installation
------------
Installation requires a working GCC compiler and Python development libraries.

    git clone git://github.com/cschwede/python-rabin-fingerprint.git
    cd python-rabin-fingerprint
    sudo python setup.py install
