#!/usr/bin/env python
# -*- coding: utf-8 -*-

""" VMM side channel attack commands generator

"""

__author__ = "Sebastien Chassot"
__email__ = "sebastien.chassot@etu.unige.ch"
__copyright__ = ""
__license__ = "GPL V2"
__status__ = "VMM side channel attack commands generator"

import os
from os import EX_OK, path, getcwd
from sys import exit
import struct
import logging
from enum import Enum
import re
import argparse

import experiments

log = logging.getLogger(__name__)
log.setLevel(logging.DEBUG)

VM_MEM_MMIO_ADDR = 0
VM_MEM_MMIO_SIZE = 0
VM_MEM_PT_ADDR = 0
VM_MEM_PT_SIZE = 0
VM_MEM_RUN_ADDR = 0
VM_MEM_RUN_SIZE = 0
NB_SAMPLES = 0
VM_MEM_MEASURES_ADDR = 0
VM_MEM_MEASURES_SIZE = 0
VM_MEM_OWNPAGES_ADDR = 0
VM_MEM_OWNPAGES_SIZE = 0
VM_MEM_SHAREDPAGES_ADDR = 0
VM_MEM_SHAREDPAGES_SIZE = 0


class Role(Enum):
    VICTIM = 0
    ATTACKER = 1
    DEFENDER = 2
    NUMBEROFROLE = 3


class Prim_sca(Enum):
    PRIMITIVE_WAIT = 1
    PRIMITIVE_MEASURE = 2
    PRIMITIVE_READ = 3
    PRIMITIVE_WRITE = 4
    PRIMITIVE_PRINT_MEASURES = 5
    PRIMITIVE_EXIT = 6



def read_c_header(filename: str) -> dict:
    """ import constants from C header file

    :param filename: header_file.h
    :return: dict const : value
    """
    defines = {}
    with open(filename) as header_file:
        for line in header_file.readlines():
            if line.startswith("#define"):
                line.rstrip()
                line = line.split('//')[0]
                m = re.search('#define\s+([A-Za-z]\w+)\s+(.*)', line)
                if m:
                    defines[m.group(1)] = m.group(2).replace(' ', '').replace('LL', '').replace('/', '//')

    return defines


def set_header(role, nb):
    """ return a packed header

    :param role: VM role
    :param nb: number of commands
    :return: packed struct
    """
    return struct.pack('<IQIQBI', role, nb, 0, 0, 0, 0)


def set_cmd(role, cmd, wait, addr=0, value=0, repeat=0):
    """ return a packed command

    :param role: VM role
    :param cmd: command type
    :param wait: delay from previous command
    :param addr: address if meaningful (read/write)
    :param value: value if meaningful (write)
    :return: packed struct
    """
    return struct.pack('<IBIQQI', role, cmd, wait, addr, value, repeat)


def write_cmd(filename: str, data: {}):
    """ write header+commands to file

    :param filename:
    :param data:
    :return:
    """
    with open(filename, 'wb') as writer:
        # writer.write(set_header(Role.VICTIM.value, len(data['cmd0'])))
        writer.write(set_header(Role.VICTIM.value, len(data['cmd0'])))
        writer.write(set_header(Role.ATTACKER.value, len(data['cmd1'])))
        writer.write(set_header(Role.DEFENDER.value, len(data['cmd2'])))
        for r in ['cmd0', 'cmd1', 'cmd2']:
            for c in data[r]:
                role, cmd, wait, addr, value, repeat = c.keys()
                log.debug("{} {} {} 0x{:02x} {} ".format(c[role], c[cmd], c[wait], c[addr], c[value], c[repeat]))
                writer.write(set_cmd(c[role], c[cmd], c[wait], c[addr], c[value], c[repeat]))


def main():
    """generate

    :return: exit status
    """
    parser = argparse.ArgumentParser(description='Metaheuristics for Optimization TP Series 5 : PSO.')
    parser.add_argument('file', help='file to parse', type=str)
    parser.add_argument('exp', help='experiment', type=int)
    args = parser.parse_args()

    """parse C header to reflect global variables states"""
    res = read_c_header("common/common.h")
    for k, v in res.items():
        # print("input : "+k+" = "+v)
        try:
            exec("global {}; {} = {}".format(k, k, v))
            # print(hex(VM_MEM_SHAREDPAGES_ADDR), flush=True)

        except :
            # print("fail : global "+k+" = "+v)
            pass
    # read alone
    if args.exp == 0:
        data, nb_of_timestamps = experiments.exp0(1000, VM_MEM_OWNPAGES_SIZE, VM_MEM_OWNPAGES_ADDR)
    # read both independently
    if args.exp == 1:
        data, nb_of_timestamps = experiments.exp1(1000, VM_MEM_OWNPAGES_SIZE, VM_MEM_OWNPAGES_ADDR)
    # read both in concurrence
    if args.exp == 2:
        data, nb_of_timestamps = experiments.exp1(1000, VM_MEM_SHAREDPAGES_SIZE, VM_MEM_SHAREDPAGES_ADDR)
    # COW only
    if args.exp == 3:
        data, nb_of_timestamps = experiments.exp2(1000, VM_MEM_SHAREDPAGES_SIZE, VM_MEM_SHAREDPAGES_ADDR)
    # COW with interactions
    if args.exp == 4:
        data, nb_of_timestamps = experiments.exp3(1000, VM_MEM_SHAREDPAGES_SIZE, VM_MEM_SHAREDPAGES_ADDR, VM_MEM_OWNPAGES_SIZE, VM_MEM_OWNPAGES_ADDR)
    # VM time
    if args.exp == 5:
        data, nb_of_timestamps = experiments.exp4(100000, VM_MEM_SHAREDPAGES_ADDR)

    assert nb_of_timestamps < VM_MEM_MEASURES_SIZE/8, 'Too many measures increase VMs memory'

    log.debug("save file : {}".format(args.file))

    write_cmd(args.file, data)

    exit(EX_OK)


if __name__ == '__main__':
    exit(main())
