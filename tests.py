#!/usr/bin/python
# -*- coding: utf-8 -*-
import hashlib
import os
import random
import sys
import tempfile
import unittest

from rabin import chunksizes_from_filename, chunksizes_from_fd


class TestFingerprint(unittest.TestCase):

    def setUp(self):
        random.seed(0)
        self.filenames = []

        if sys.version > '3':
            byte = [chr(i) for i in range(256)]
        else:
            byte = [unichr(i) for i in range(256)]

        # create two files: both are identical except 1 KiB in the middle
        self.data1 = ''.join(self._choice(byte) for _ in range(512 * 1024))
        self.data2 = ''.join(self._choice(byte) for _ in range(1 * 1024))
        self.data3 = ''.join(self._choice(byte) for _ in range(512 * 1024))

        self.tmpf = tempfile.NamedTemporaryFile(mode="wb", delete=False)
        s = self.data1 + self.data3
        s = s.encode('utf-8')
        self.tmpf.write(s)
        self.filenames.append(self.tmpf.name)
        self.tmpf.close()

        # Ensure that sample data is always the same
        m1 = hashlib.md5(s)
        self.assertEqual('c91de0700e243bd2aedefddb0a5b1845', m1.hexdigest())

        self.tmpf2 = tempfile.NamedTemporaryFile(mode="wb", delete=False)
        s = self.data1 + self.data2 + self.data3
        s = s.encode('utf-8')
        self.tmpf2.write(s)
        self.filenames.append(self.tmpf2.name)
        self.tmpf2.close()

        # Ensure that sample data is always the same
        m2 = hashlib.md5(s)
        self.assertEqual('c9601df3a8afc1bb5ee61dcbc7ebfc42', m2.hexdigest())

        self.reference = [55284, 225345, 34119, 39188, 120699, 97026, 120605,
                          72303, 35120, 43389, 63216, 46086, 112801, 98696,
                          45160, 82568, 95650, 35648, 78397, 71123]

    def tearDown(self):
        for filename in self.filenames:
            try:
                os.remove(filename)
            except:
                pass

    def _choice(self, seq):
        """Choose a random element from a non-empty sequence.

        Behaviour of random.choice changed in Python 3.2:
        https://hg.python.org/cpython/rev/c2f8418a0e14/

        This method is used in Python2 and thus returns the
        same result in Python3.
        """
        return seq[int(random.random() * len(seq))]

    def test_fingerprint_from_filename(self):
        chunks = chunksizes_from_filename(self.tmpf.name)
        self.assertEqual(self.reference, chunks)

        # only a few chunks differ
        chunks = chunksizes_from_filename(self.tmpf2.name)
        self.reference[6] = 120605
        self.reference[7] = 72303
        self.reference[8] = 36679
        self.assertEqual(self.reference, chunks)

    def test_fingerprint_from_fd(self):
        with open(self.tmpf.name) as fd:
            old_chunks = chunksizes_from_fd(fd.fileno())
        self.assertEqual(self.reference, old_chunks)

    def test_empty_file(self):
        f = tempfile.NamedTemporaryFile(delete=False)
        self.filenames.append(f.name)
        empty_chunks = chunksizes_from_filename(f.name)
        self.assertEqual([], empty_chunks)

    def test_missing_file(self):
        self.assertRaises(IOError, chunksizes_from_filename, 'noneexistentfile')


if __name__ == '__main__':
    unittest.main()
