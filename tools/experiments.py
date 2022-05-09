""" VMM side channel attack experiments

"""

__author__ = "Sebastien Chassot"
__email__ = "sebastien.chassot@etu.unige.ch"
__copyright__ = ""
__license__ = "GPL V2"
__status__ = "VMM side channel attack commands generator"

import random
from enum import Enum
from struct import pack, unpack


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


def data_init(addr):
    return {'cmd0': [cmd_measure(Role.VICTIM, 0), cmd_read(Role.VICTIM, 0, addr)],
            'cmd1': [cmd_measure(Role.ATTACKER, 0), cmd_read(Role.ATTACKER, 0, addr)],
            'cmd2': [cmd_measure(Role.DEFENDER, 0), cmd_read(Role.DEFENDER, 0, addr)]}


def exp0(nb_mes, nb_pages, addr):
    """Attacker read his own memory region

    how  : attacker randomly read his own pages.
    goal : evaluate time access to data without interactions

    :param nb_mes:
    :param nb_pages: addresses range (offset+size)
    :param addr:  base address (offset)
    """
    # preamble
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


def exp1(nb_mes, nb_pages, addr):
    """Attacker and Victim read memory region

    how  : attacker and victim randomly read shared pages.
    goal : evaluate time access to data with random interactions

    :param nb_mes:
    :param nb_pages: addresses range (offset+size)
    :param addr:  base address (offset)
    """
    _data = {
        'cmd0': [
            cmd_measure(Role.VICTIM, 0),
            cmd_measure(Role.VICTIM, 0),
            cmd_measure(Role.VICTIM, 0),
            cmd_measure(Role.VICTIM, 0),
            cmd_measure(Role.VICTIM, 0),
            cmd_measure(Role.VICTIM, 0),
            cmd_measure(Role.VICTIM, 0),
            cmd_measure(Role.VICTIM, 0),
            cmd_measure(Role.VICTIM, 0),
            cmd_measure(Role.VICTIM, 0),
            cmd_measure(Role.VICTIM, 0),
            cmd_measure(Role.VICTIM, 0),
            cmd_measure(Role.VICTIM, 0),
            cmd_measure(Role.VICTIM, 0),
            cmd_measure(Role.VICTIM, 0),
            cmd_read(Role.VICTIM, 0, addr)],
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

    for i in range(nb_mes):
        _data['cmd0'].append(cmd_read(Role.VICTIM, 0, addr+(random.randrange(0, nb_pages))))
    _data['cmd0'].append(cmd_exit(Role.VICTIM, 0))

    return _data, max_nb_of_measures(_data)


def exp2(nb_mes, nb_pages, addr):
    """Attacker write shared memory region (COW)

    how  : attacker randomly write shared pages.
    goal : evaluate COW time without interactions

    :param nb_mes:
    :param nb_pages: addresses range (offset+size)
    :param addr:  base address (offset)
    """
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
        _data['cmd1'].append(cmd_write(Role.ATTACKER, 0, addr+(random.randrange(0, nb_pages)), random.randint(0, 0xffffffffffffffff)))
    _data['cmd1'].append(cmd_exit(Role.ATTACKER, 0))

    return _data, max_nb_of_measures(_data)


def exp3(nb_mes, nb_pages0, addr0, nb_pages1, addr1):
    """Attacker write own and shared memory region

    how         : attacker randomly write on pages.
    goal        : evaluate difference while writing own data and shared data
    hypothesis  : COW should take longer to perform

    :param nb_mes:
    :param nb_pages0: addresses range (offset+size)
    :param addr0:  base address (offset)
    :param nb_pages1: addresses range (offset+size)
    :param addr1:  base address (offset)
    """
    _data = {
        'cmd0': [
            cmd_measure(Role.VICTIM, 0),
            cmd_read(Role.VICTIM, 0, addr0),  # load measures page in cache
            cmd_exit(Role.VICTIM, 0)],
        'cmd1': [
            cmd_measure(Role.ATTACKER, 0),
            cmd_read(Role.ATTACKER, 0, addr0),  # load measures page in cache
        ],
        'cmd2': [
            cmd_measure(Role.DEFENDER, 0),
            cmd_read(Role.DEFENDER, 0, addr0),  # load measures page in cache
            cmd_exit(Role.DEFENDER, 0)],
    }
    for i in range(nb_mes):
        _data['cmd1'].append(cmd_write(Role.ATTACKER, 0, addr0+(random.randrange(0, nb_pages0)), random.randint(0, 0xffffffffffffffff)))
    for i in range(nb_mes):
        _data['cmd1'].append(cmd_write(Role.ATTACKER, 0, addr1+(random.randrange(0, nb_pages1)), random.randint(0, 0xffffffffffffffff)))
    _data['cmd1'].append(cmd_exit(Role.ATTACKER, 0))

    return _data, max_nb_of_measures(_data)


def exp4(nb_mes, addr):
    """measure VMs runtime

    how  : take many measures
    goal : see if we detect VM exit

    :param nb_mes:
    :param addr:  base address (offset)
    """
    # preamble
    _data = data_init(addr)

    #
    wait = 287
    for i in range(nb_mes):
        _data['cmd0'].append(cmd_measure(Role.VICTIM, wait))
        _data['cmd1'].append(cmd_measure(Role.ATTACKER, wait))
        _data['cmd2'].append(cmd_measure(Role.DEFENDER, wait))

    # exit
    _data['cmd0'].append(cmd_exit(Role.VICTIM, 0))
    _data['cmd1'].append(cmd_exit(Role.ATTACKER, 0))
    _data['cmd2'].append(cmd_exit(Role.DEFENDER, 0))

    return _data, max_nb_of_measures(_data)


def set_cmd(role, cmd, wait, addr=0, value=0, repeat=0):
    """ return a packed command

    :param role: VM role
    :param cmd: command type
    :param wait: delay from previous command
    :param addr: address if meaningful (read/write)
    :param value: value if meaningful (write)
    :param repeat: command can be repeated (unused)
    :return: packed struct
    """
    return pack('<IBIQQI', role, cmd, wait, addr, value, repeat)


def cmd_measure(role, wait, repeat=1):
    """ create a measure command

    :param role:
    :param wait:
    :param repeat: command can be repeated (unused)
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
    """return the number of timestamps needed by data experiment

    """
    nb = []
    for mes_lst in data.values():
        res = len([_ for _ in mes_lst if _['cmd'] == Prim_sca.PRIMITIVE_MEASURE.value])
        res += len([_ for _ in mes_lst if _['cmd'] == Prim_sca.PRIMITIVE_READ.value])*2
        res += len([_ for _ in mes_lst if _['cmd'] == Prim_sca.PRIMITIVE_WRITE.value])*2
        nb.append(res)
    return max(nb)
