""" VMM side channel attack experiments

"""

__author__ = "Sebastien Chassot"
__email__ = "sebastien.chassot@etu.unige.ch"
__copyright__ = ""
__license__ = "GPL V2"
__status__ = "VMM side channel attack commands generator"

import random
from enum import Enum


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

def max_nb_of_measures(data):
    nb = []
    for mes_lst in data.values():
        res = len([_ for _ in mes_lst if _['cmd'] == Prim_sca.PRIMITIVE_MEASURE.value])
        res += len([_ for _ in mes_lst if _['cmd'] == Prim_sca.PRIMITIVE_READ.value])*2
        res += len([_ for _ in mes_lst if _['cmd'] == Prim_sca.PRIMITIVE_WRITE.value])*2
        nb.append(res)
    return max(nb)

def shared_cow(nb_mes, nb_pages, addr):
    _data = {
        'cmd0': [
            cmd_measure(Role.VICTIM, 0),
            cmd_read(Role.VICTIM, 0, addr),  # load measures page in cache
            cmd_exit(Role.VICTIM, 0)],
        'cmd1': [
            cmd_measure(Role.ATTACKER, 0),
            cmd_read(Role.ATTACKER, 0, addr),  # load measures page in cache
        ],
        'cmd2': [
            cmd_measure(Role.DEFENDER, 0),
            cmd_read(Role.DEFENDER, 0, addr),  # load measures page in cache
            cmd_exit(Role.DEFENDER, 0)],
    }
    for i in range(nb_mes):
        _data['cmd1'].append(cmd_write(Role.ATTACKER, 0, addr+(random.randrange(0,nb_pages)), random.randint(0, 0xffffffffffffffff)))
    _data['cmd1'].append(cmd_exit(Role.ATTACKER, 0))

    return _data, max_nb_of_measures(_data)


def read_own(nb_mes, nb_pages, addr):
    _data = {
        'cmd0': [
            cmd_measure(Role.VICTIM, 0),
            cmd_read(Role.VICTIM, 0, addr),  # load measures page in cache
            cmd_exit(Role.VICTIM, 0)],
        'cmd1': [
            cmd_measure(Role.ATTACKER, 0),
            cmd_read(Role.ATTACKER, 0, addr),  # load measures page in cache
        ],
        'cmd2': [
            cmd_measure(Role.DEFENDER, 0),
            cmd_read(Role.DEFENDER, 0, addr),  # load measures page in cache
            cmd_exit(Role.DEFENDER, 0)],
    }
    for i in range(nb_mes):
        _data['cmd1'].append(cmd_read(Role.ATTACKER, 0, addr+(random.randrange(0, nb_pages))))
    _data['cmd1'].append(cmd_exit(Role.ATTACKER, 0))

    return _data, max_nb_of_measures(_data)
