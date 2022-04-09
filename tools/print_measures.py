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

import matplotlib.pyplot as plt
import numpy as np

log = logging.getLogger(__name__)
log.setLevel(logging.DEBUG)

vm_name = ['victime', 'attacker', 'defender']
cmd_name = ["NONE", "PRIMITIVE_WAIT", "PRIMITIVE_MEASURE", "PRIMITIVE_READ", "PRIMITIVE_WRITE", "PRIMITIVE_PRINT_MEASURES", "PRIMITIVE_EXIT"]


def read_timestamp(filename: str):
    """ read timestamp from file

    :param filename:
    :return:
    """
    res = [[], [], []]
    struct_fmt = '<IBBIQQQQQ'
    struct_len = struct.calcsize(struct_fmt)
    # struct_unpack = struct.Struct(struct_fmt).unpack_from
    with open(filename, 'rb') as f:
        bytes = f.read(struct_len)
        while bytes:
            id, cmd, vm_id, wait, ts0, ts1, ts2, ts3, ts4 = struct.unpack(struct_fmt, bytes)
            res[vm_id].append({'id': id, 'cmd': cmd, 'wait': wait, 'ts':[ts0, ts1, ts2, ts3, ts4]})
            bytes = f.read(struct_len)

    return res

def print_mes(mes, vm):
    print("VM {} ({}) : {} ({})".format(vm_name[vm], mes['id'], mes['ts'][1]-mes['ts'][0], cmd_name[mes['cmd']]))

def distribution(mes, cmd):

    x = [i['ts'][1]-i['ts'][0] for i in mes if i['cmd'] == cmd]
    # x = [i for i in x if i > 1000]
    # y = list(range(0, len(x)))
    # plot
    # fig, ax = plt.subplots()

    # the histogram of the data
    n, bins, patches = plt.hist(x, 100)


    plt.xlabel('enlapsed time')
    plt.ylabel('Probability')
    plt.title('Histogram of {}'.format(cmd_name[cmd]))

    plt.show()


def main():
    """generate

    :return: exit status
    """
    parser = argparse.ArgumentParser(description='Metaheuristics for Optimization TP Series 5 : PSO.')
    parser.add_argument('-f', '--file', help='file to parse', type=str, required=True)
    args = parser.parse_args()

    res = read_timestamp(args.file)

    for vm in range(0,len(res)):
        for i in range(1,len(res[vm])):
            if res[vm][i]['cmd'] == 3:
                print_mes(res[vm][i], vm)

    distribution(res[1], 4)
    exit(EX_OK)


if __name__ == '__main__':
    exit(main())
