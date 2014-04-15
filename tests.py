#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
import random
import tempfile
import unittest

from rabin import rabin


class TestFingerprint(unittest.TestCase):

    def setUp(self):
        self.filenames = []

    def tearDown(self):
        for filename in self.filenames:
            try:
                os.remove(filename)
            except:
                pass

    def test_fingerprint(self):
        # make the test reproducable
        random.seed(0)
        byte = [chr(i) for i in range(256)]

        # create two files: both are identical except 1 KiB in the middle
        data1 = ''.join(random.choice(byte) for _ in range(512 * 1024))
        data2 = ''.join(random.choice(byte) for _ in range(1 * 1024))
        data3 = ''.join(random.choice(byte) for _ in range(512 * 1024))

        tmpf = tempfile.NamedTemporaryFile(delete=False)
        tmpf.write(data1 + data3)
        self.filenames.append(tmpf.name)
        tmpf.close()

        tmpf2 = tempfile.NamedTemporaryFile(delete=False)
        tmpf2.write(data1 + data2 + data3)
        self.filenames.append(tmpf2.name)
        tmpf2.close()

        old_chunks = rabin(tmpf.name)
        reference = [97374, 78240, 80059, 35852, 61484, 89381,
                     139592, 73169, 36567, 34204, 130637, 192017]
        self.assertEqual(reference, old_chunks)

        # only one chunk differs
        new_chunks = rabin(tmpf2.name)
        reference[6] = 140616
        reference[7] = 73169
        self.assertEqual(reference, new_chunks)

    def test_empty_file(self):
        f = tempfile.NamedTemporaryFile(delete=False)
        self.filenames.append(f.name)
        empty_chunks = rabin(f.name)
        self.assertEqual([], empty_chunks)

    def test_missing_file(self):
        self.assertRaises(IOError, rabin, 'noneexistentfile')


if __name__ == '__main__':
    unittest.main()
