import sys
import os

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../'))

import unittest
from unittest import mock
from ddt import ddt, data, unpack
import create_bom # pylint: disable=import-error

@ddt
class TestCheckSum(unittest.TestCase):
    @data(
        ("files/README.md",  1594567860),
        ("files/bin_file",   1742489887),
        ("files/bin_file_2", 3128462852),
        ("files/bin_file_3", 1052468430),
        ("files/text_file.txt",   2610763910),
        ("files/text_file_2.txt", 3060373916),
        ("files/text_file_3.txt", 1774879241),
    )
    @unpack
    def test_check_sum_for(self, file, expected):
        self.assertEqual(create_bom.checksum_file(file), expected)

@ddt
class TestProcessFile(unittest.TestCase):
    @data(
        ("files/README.md",    ['<deliverydir>/README.md',
                                '<installdir>/tcc_tools/<version>/README.md',
                                1594567860,
                                'internal',
                                '664']),
        ("files/bin_file",     ['<deliverydir>/bin_file',
                                '<installdir>/tcc_tools/<version>/bin_file',
                                1742489887,
                                'internal',
                                '664']),
        ("files/bin_file_2",  ['<deliverydir>/bin_file_2',
                                '<installdir>/tcc_tools/<version>/bin_file_2',
                                3128462852,
                                'internal',
                                '764']),
        ("files/bin_file_3",  ['<deliverydir>/bin_file_3',
                                '<installdir>/tcc_tools/<version>/bin_file_3',
                                1052468430,
                                'internal',
                                '775']),
        ("files/text_file.txt",   ['<deliverydir>/text_file.txt',
                                    '<installdir>/tcc_tools/<version>/text_file.txt',
                                    2610763910,
                                    'internal',
                                    '666']),
        ("files/text_file_2.txt", ['<deliverydir>/text_file_2.txt',
                                    '<installdir>/tcc_tools/<version>/text_file_2.txt',
                                    3060373916,
                                    'internal',
                                    '644']),
        ("files/text_file_3.txt", ['<deliverydir>/text_file_3.txt',
                                    '<installdir>/tcc_tools/<version>/text_file_3.txt',
                                    1774879241,
                                    'internal',
                                    '400']),
        ("files/symlinks_bin_file", ['<N/A>',
                                    '<installdir>/tcc_tools/<version>/symlinks_bin_file',
                                    '',
                                    'internal',
                                    '664',
                                    'bin_file']),
        ("files/child_dir/symlinks_bin_file_2", ['<N/A>',
                                                 '<installdir>/tcc_tools/<version>/child_dir/symlinks_bin_file_2',
                                                 '',
                                                 'internal',
                                                 '764',
                                                 '../bin_file_2']),
    )
    @unpack
    def test_process_file_for(self, file, expected):
        self.assertEqual(create_bom.process_file(file, "files"), expected)
