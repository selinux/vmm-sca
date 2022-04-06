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


def cmd_measure(role, wait, repeat=1):
    """ create a measure command

    :param role:
    :param wait:
    :return:
    """
    return {'role': role.value, 'cmd': Prim_sca.PRIMITIVE_MEASURE.value, 'wait': wait, 'addr': 0, 'value': 0, 'repeat': repeat}


def cmd_read(role, wait, addr, repeat=1):
    """ create a read command (read at guest address)

    :param role:
    :param wait:
    :param addr:
    :return:
    """
    return {'role': role.value, 'cmd': Prim_sca.PRIMITIVE_READ.value, 'wait': wait, 'addr': addr, 'value': 0, 'repeat': repeat}


def cmd_write(role, wait, addr, value, repeat=1):
    """ create a write command (write value at address)

    :param role:
    :param wait:
    :param addr:
    :param value:
    :return:
    """
    return {'role': role.value, 'cmd': Prim_sca.PRIMITIVE_WRITE.value, 'wait': wait, 'addr': addr, 'value': value, 'repeat': repeat}


def cmd_exit(role, wait):
    """ create an exit command (exit guest loop)

    :param role:
    :param wait:
    :return:
    """
    return {'role': role.value, 'cmd': Prim_sca.PRIMITIVE_EXIT.value, 'wait': wait, 'addr': 0, 'value': 0, 'repeat': 1}


def cmd_print_mes(role, wait):
    """ create a print command

    :param role:
    :param wait:
    :return:
    """
    return {'role': role.value, 'cmd': Prim_sca.PRIMITIVE_PRINT_MEASURES.value, 'wait': wait, 'addr': 0, 'value': 0, 'repeat': 1}


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
    data = {
        'cmd0': [
            cmd_measure(Role.VICTIM, 0),
             cmd_measure(Role.VICTIM, 150000),
             # cmd_read(Role.VICTIM, 0, VM_MEM_OWNPAGES_ADDR+(0x1000*0)),
             # cmd_read(Role.VICTIM, 0, VM_MEM_OWNPAGES_ADDR+(0x1000*1)),
             # cmd_read(Role.VICTIM, 0, VM_MEM_OWNPAGES_ADDR+(0x1000*3)),
            # cmd_print_mes(Role.VICTIM, 1000),
            cmd_exit(Role.VICTIM, 0)],
        'cmd1': [
            cmd_measure(Role.ATTACKER, 0),
            cmd_read(Role.ATTACKER, 0, VM_MEM_MEASURES_ADDR),                       # load measures page in cache
            # cmd_read(Role.ATTACKER, 0, VM_MEM_SHAREDPAGES_ADDR+(0x1000*0), 4),
            # cmd_print_mes(Role.ATTACKER, 1000),
            # cmd_exit(Role.ATTACKER, 0)
            ],
        'cmd2': [
            cmd_measure(Role.DEFENDER, 0),

            cmd_exit(Role.DEFENDER, 0)],
    }

    for i in range(0x100):
        data['cmd1'].append(cmd_read(Role.ATTACKER, 0, VM_MEM_SHAREDPAGES_ADDR+(0x1000*i)))
        # data['cmd1'].append(cmd_read(Role.ATTACKER, 0, VM_MEM_OWNPAGES_ADDR+(0x1000*i)))
        data['cmd1'].append(cmd_write(Role.ATTACKER, 0, VM_MEM_SHAREDPAGES_ADDR+(0x1000*i), 0xa5a5a5))
        data['cmd1'].append(cmd_write(Role.ATTACKER, 0, VM_MEM_OWNPAGES_ADDR+(0x1000*i), 0xa5a5a5))
        # data['cmd1'].append(cmd_write(Role.ATTACKER, 0, VM_MEM_OWNPAGES_ADDR+(0x1000*i), 0x5a5a5a))
        # data['cmd1'].append(cmd_write(Role.ATTACKER, 0, VM_MEM_OWNPAGES_ADDR+(0x1000*i), 0x5a5a5a))
        # data['cmd1'].append(cmd_write(Role.ATTACKER, 0, VM_MEM_OWNPAGES_ADDR+(0x1000*i), 0x5a5a5a))
    # data['cmd1'].append(cmd_read(Role.ATTACKER, 0, VM_MEM_SHAREDPAGES_ADDR+(0x1000*i), 2))
    data['cmd1'].append(cmd_exit(Role.ATTACKER, 0))


    res = read_c_header("common/common.h")
    for k,v in res.items():
        # print("input : "+k+" = "+v)
        try:
            exec("global {}; {} = {}".format(k, k, v))
            # exec(k+' = '+eval(v))
            # print("yep : global "+k+" = "+v)
            # print(hex(VM_MEM_SHAREDPAGES_ADDR), flush=True)

        except :
            # print("fail : global "+k+" = "+v)
            pass

    # print(hex(VM_MEM_RUN_ADDR), flush=True)
    # print(hex(VM_MEM_RUN_SIZE), flush=True)
    # print(hex(VM_MEM_MMIO_ADDR), flush=True)
    # print(hex(VM_MEM_MMIO_SIZE), flush=True)
    # print(hex(VM_MEM_PT_ADDR), flush=True)
    # print(hex(VM_MEM_PT_SIZE), flush=True)
    # print("samples : "+str(NB_SAMPLES), flush=True)
    # print(hex(VM_MEM_MEASURES_ADDR), flush=True)
    # print(hex(VM_MEM_MEASURES_SIZE), flush=True)
    # print(hex(VM_MEM_OWNPAGES_ADDR), flush=True)
    # print(hex(VM_MEM_OWNPAGES_SIZE), flush=True)
    # print(hex(VM_MEM_SHAREDPAGES_ADDR), flush=True)
    # print(hex(VM_MEM_SHAREDPAGES_SIZE), flush=True)

    # if VM_MEM_MEASURES_SIZE//8 < NB_SAMPLES:
    #     log.error("Measure memory too small")

    log.debug("save file : test_bench.dat")
    write_cmd("test_bench.dat", data)

    exit(EX_OK)


if __name__ == '__main__':
    exit(main())
