#!/usr/bin/env python
# -*- coding: utf-8 -*-

""" VMM side channel attack timestamp pretty printer

"""

__author__ = "Sebastien Chassot"
__email__ = "sebastien.chassot@etu.unige.ch"
__copyright__ = ""
__license__ = "GPL V2"
__status__ = "VMM side channel attack pretty printer"

import os
from os import EX_OK, path, getcwd
from sys import exit
import struct
import logging
import argparse


log = logging.getLogger(__name__)
log.setLevel(logging.DEBUG)


def read_timestamp(filename: str):
    """ read timestamp from file

    :param filename:
    :return:
    """
    res = []
    struct_fmt = '<Q'
    struct_len = struct.calcsize(struct_fmt)
    struct_unpack = struct.Struct(struct_fmt).unpack_from
    with open(filename, 'rb') as f:
        bytes = f.read(8)
        while bytes:
            res.append(struct.unpack('<Q', bytes)[0])
            bytes = f.read(8)

    return res


def main():
    """generate

    :return: exit status
    """
    parser = argparse.ArgumentParser(description='Metaheuristics for Optimization TP Series 5 : PSO.')
    parser.add_argument('-f', '--file', help='file to parse', type=str, required=True)
    args = parser.parse_args()

    res = read_timestamp(args.file)

    for i in range(1,len(res)):
        print("VM charlie ({}) : {} ({})".format(i, res[i], res[i]-res[i-1]))
    exit(EX_OK)


if __name__ == '__main__':
    exit(main())
