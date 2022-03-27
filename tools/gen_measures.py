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

log = logging.getLogger(__name__)
log.setLevel(logging.DEBUG)


class Role(Enum):
    VICTIM = 0
    ATTACKER = 1
    DEFENDER = 2
    NUMBEROFROLE = 3


class Prim_sca(Enum):
    PRIMITIVE_WAIT = 0
    PRIMITIVE_MEASURE = 1
    PRIMITIVE_READ = 2
    PRIMITIVE_WRITE = 3
    PRIMITIVE_PRINT_MEASURES = 4
    PRIMITIVE_EXIT = 5


OWN_PAGES_ADDR = 0xa09000
SHARED_PAGES_ADDR = 0xa29000


def set_header(role, nb):
    """ return a packed header

    :param role: VM role
    :param nb: number of commands
    :return: packed struct
    """
    return struct.pack('<IQIQB', role, nb, 0, 0, 0)


def set_cmd(role, cmd, wait, addr=0, value=0):
    """ return a packed command

    :param role: VM role
    :param cmd: command type
    :param wait: delay from previous command
    :param addr: address if meaningful (read/write)
    :param value: value if meaningful (write)
    :return: packed struct
    """
    return struct.pack('<IBIQQ', role, cmd, wait, addr, value)


def cmd_measure(role, wait):
    """ create a measure command

    :param role:
    :param wait:
    :return:
    """
    return {'role': role.value, 'cmd': Prim_sca.PRIMITIVE_MEASURE.value, 'wait': wait, 'addr': 0, 'value': 0}


def cmd_read(role, wait, addr):
    """ create a read command (read at guest address)

    :param role:
    :param wait:
    :param addr:
    :return:
    """
    return {'role': role.value, 'cmd': Prim_sca.PRIMITIVE_READ.value, 'wait': wait, 'addr': addr, 'value': 0}


def cmd_write(role, wait, addr, value):
    """ create a write command (write value at address)

    :param role:
    :param wait:
    :param addr:
    :param value:
    :return:
    """
    return {'role': role.value, 'cmd': Prim_sca.PRIMITIVE_WRITE.value, 'wait': wait, 'addr': addr, 'value': value}


def cmd_exit(role, wait):
    """ create an exit command (exit guest loop)

    :param role:
    :param wait:
    :return:
    """
    return {'role': role.value, 'cmd': Prim_sca.PRIMITIVE_EXIT.value, 'wait': wait, 'addr': 0, 'value': 0}


def cmd_print_mes(role, wait):
    """ create a print command

    :param role:
    :param wait:
    :return:
    """
    return {'role': role.value, 'cmd': Prim_sca.PRIMITIVE_PRINT_MEASURES.value, 'wait': wait, 'addr': 0, 'value': 0}


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
                role, cmd, wait, addr, value = c.keys()
                log.debug("{} {} {} 0x{:02x} {} ".format(c[role], c[cmd], c[wait], c[addr], c[value]))
                writer.write(set_cmd(c[role], c[cmd], c[wait], c[addr], c[value]))


def main():
    """generate

    :return: exit status
    """
    data = {
        'cmd0': [
            # cmd_measure(Role.VICTIM, 1000),
            # cmd_measure(Role.VICTIM, 500000),
            # cmd_measure(Role.VICTIM, 1000),
            # cmd_measure(Role.VICTIM, 1000),
            cmd_measure(Role.VICTIM, 150000),
            # cmd_print_mes(Role.VICTIM, 100000),
            cmd_exit(Role.VICTIM, 1000000)],
        'cmd1': [
            cmd_read(Role.ATTACKER, 1000, 0x203000),
            cmd_read(Role.ATTACKER, 0, 0x203000),
            cmd_read(Role.ATTACKER, 0, 0x203000),
            cmd_read(Role.ATTACKER, 0, 0x203000),
            cmd_read(Role.ATTACKER, 0, 0x203000),
            cmd_read(Role.ATTACKER, 0, 0x204000),
            cmd_read(Role.ATTACKER, 0, 0x204000),
            cmd_write(Role.ATTACKER, 0, 0x204000, 0x20),
            cmd_measure(Role.ATTACKER, 1000),
            # cmd_measure(Role.ATTACKER, 1000),
            # cmd_measure(Role.ATTACKER, 1000),
            # cmd_measure(Role.ATTACKER, 1000),
            # cmd_measure(Role.ATTACKER, 200000),
            # cmd_measure(Role.ATTACKER, 1000),
            # cmd_measure(Role.ATTACKER, 300000),
            # cmd_measure(Role.ATTACKER, 1000),
            # cmd_measure(Role.ATTACKER, 400000),
            # cmd_measure(Role.ATTACKER, 1000),
            # cmd_measure(Role.ATTACKER, 500000),
            # cmd_measure(Role.ATTACKER, 500000),
            # cmd_measure(Role.ATTACKER, 500000),
            cmd_print_mes(Role.ATTACKER, 500000),
            cmd_exit(Role.ATTACKER, 500000)],
        'cmd2': [
            cmd_measure(Role.DEFENDER, 500000),
            cmd_measure(Role.DEFENDER, 500000),
            cmd_measure(Role.DEFENDER, 500000),
            cmd_measure(Role.DEFENDER, 500000),
            cmd_measure(Role.DEFENDER, 500000),
            cmd_measure(Role.DEFENDER, 500000),
            cmd_measure(Role.DEFENDER, 500000),
            cmd_measure(Role.DEFENDER, 500000),
            cmd_measure(Role.DEFENDER, 500000),
            cmd_measure(Role.DEFENDER, 500000),
            # cmd_print_mes(Role.DEFENDER, 100000),
            cmd_exit(Role.DEFENDER, 1000000)],
    }

    write_cmd("test_bench.dat", data)

    exit(EX_OK)


if __name__ == '__main__':
    exit(main())
