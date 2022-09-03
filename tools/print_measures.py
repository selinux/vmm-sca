#!/usr/bin/env python
# -*- coding: utf-8 -*-

""" VMM side channel attack timestamp pretty printer

"""

__author__ = "Sebastien Chassot"
__email__ = "sebastien.chassot@etu.unige.ch"
__copyright__ = ""
__license__ = "GPL V2"
__status__ = "VMM side channel attack pretty printer"

import math
import os
from os import EX_OK, path, getcwd
from sys import exit
import struct
import logging
import argparse
from enum import Enum
from statistics import mean, stdev

import matplotlib.pyplot as plt
import numpy as np

log = logging.getLogger(__name__)
log.setLevel(logging.DEBUG)

vm_name = ['victime', 'attacker', 'defender']
cmd_name = ["NONE", "PRIMITIVE_WAIT", "PRIMITIVE_MEASURE", "PRIMITIVE_READ", "PRIMITIVE_WRITE", "PRIMITIVE_PRINT_MEASURES", "PRIMITIVE_EXIT"]

class Prim_sca(Enum):
    PRIMITIVE_WAIT = 1
    PRIMITIVE_MEASURE = 2
    PRIMITIVE_READ = 3
    PRIMITIVE_WRITE = 4
    PRIMITIVE_PRINT_MEASURES = 5
    PRIMITIVE_EXIT = 6


def print_mes(mes, vm):
    print("VM {} ({}) : {} ({})".format(vm_name[vm], mes['id'], mes['ts'][1]-mes['ts'][0], cmd_name[mes['cmd']]))


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
            res[vm_id].append({'id': id, 'cmd': cmd, 'wait': wait, 'ts': [ts0, ts1, ts2, ts3, ts4]})
            bytes = f.read(struct_len)

    return res


def plot_exp0(mes, vm):

    freq = 1/3.5

    x_meas = [i['ts'][0] for i in mes if i['cmd'] == Prim_sca.PRIMITIVE_MEASURE.value]
    x_read = [i['ts'][1]-i['ts'][0] for i in mes if i['cmd'] == Prim_sca.PRIMITIVE_READ.value]

    # plot
    # fig, (ax1, ax2, ax3) = plt.subplots(3, 1, sharey=True)
    fig, ax = plt.subplots(1, 1, sharey=True)
    n, bins, patches = ax.hist(x_read, 3000)



    plt.title('Random read (own) pages (VM {})'.format(vm_name[vm]))
    plt.xlabel('enlapsed time')
    plt.ylabel('histogram')

    print("="*40+"\nnumber of cycle at 3.5Ghz\n"+"-"*40)
    print("read : means {:.2f}, stdev {:.2f}".format(mean(x_read), stdev(x_read)))
    low_r = [i*freq for i in x_read if i < 1000]
    high_r = [i*freq for i in x_read if i > 1000]
    print("read in cache : means {:.2f}, stdev {:.2f}".format(mean(low_r), stdev(low_r)))
    print("read not in cache : means {:.2f}, stdev {:.2f}, min {}".format(mean(high_r), stdev(high_r), min(high_r)))

    plt.xlim([0, 15000])
    plt.show()


def plot_exp1(mes, vm):

    freq = 1/3.5

    x_meas = [i['ts'][0] for i in mes if i['cmd'] == Prim_sca.PRIMITIVE_MEASURE.value]
    x_read = [i['ts'][1]-i['ts'][0] for i in mes if i['cmd'] == Prim_sca.PRIMITIVE_READ.value]
    x_write = [i['ts'][1]-i['ts'][0] for i in mes if i['cmd'] == Prim_sca.PRIMITIVE_WRITE.value]

    # plot
    # fig, (ax1, ax2, ax3) = plt.subplots(3, 1, sharey=True)
    fig, ax = plt.subplots(1, 1, sharey=True)
    n, bins, patches = ax.hist(x_read, 1000)

    plt.title('Two VMs randomly read own pages (VM {})'.format(vm_name[vm]))
    plt.xlabel('enlapsed time')
    plt.ylabel('histogram')

    print("="*40+"\nnumber of cycle at 3.5Ghz\n"+"-"*40)
    print("read : means {:.2f}, stdev {:.2f}".format(mean(x_read), stdev(x_read)))
    low_r = [i*freq for i in x_read if i < 1000]
    high_r = [i*freq for i in x_read if i > 1000]
    print("read in cache : means {:.2f}, stdev {:.2f}".format(mean(low_r), stdev(low_r)))
    print("read not in cache : means {:.2f}, stdev {:.2f}, min {}".format(mean(high_r), stdev(high_r), min(high_r)))

    plt.xlim([0, 11000])
    plt.show()


def plot_exp2(mes, vm):

    freq = 1/3.5

    x_meas = [i['ts'][0] for i in mes if i['cmd'] == Prim_sca.PRIMITIVE_MEASURE.value]
    x_read = [i['ts'][1]-i['ts'][0] for i in mes if i['cmd'] == Prim_sca.PRIMITIVE_READ.value]
    x_write = [i['ts'][1]-i['ts'][0] for i in mes if i['cmd'] == Prim_sca.PRIMITIVE_WRITE.value]

    # plot
    # fig, (ax1, ax2, ax3) = plt.subplots(3, 1, sharey=True)
    fig, ax = plt.subplots(1, 1, sharey=True)
    n, bins, patches = ax.hist(x_read, 1000)

    plt.title('Two VMs randomly read shared pages (VM {})'.format(vm_name[vm]))
    plt.xlabel('enlapsed time')
    plt.ylabel('histogram')

    print("="*40+"\nnumber of cycle at 3.5Ghz\n"+"-"*40)
    print("read : means {:.2f}, stdev {:.2f}".format(mean(x_read), stdev(x_read)))
    low_r = [i*freq for i in x_read if i < 1000]
    high_r = [i*freq for i in x_read if i > 1000]
    print("read in cache : means {:.2f}, stdev {:.2f}".format(mean(low_r), stdev(low_r)))
    print("read not in cache : means {:.2f}, stdev {:.2f}, min {}".format(mean(high_r), stdev(high_r), min(high_r)))

    plt.xlim([0, 11000])
    plt.show()


def plot_exp3(mes, vm):

    freq = 1/3.5

    x_meas = [i['ts'][0] for i in mes if i['cmd'] == Prim_sca.PRIMITIVE_MEASURE.value]
    x_read = [i['ts'][1]-i['ts'][0] for i in mes if i['cmd'] == Prim_sca.PRIMITIVE_READ.value]
    x_write = [i['ts'][1]-i['ts'][0] for i in mes if i['cmd'] == Prim_sca.PRIMITIVE_WRITE.value]

    # plot
    # fig, (ax1, ax2, ax3) = plt.subplots(3, 1, sharey=True)
    fig, ax = plt.subplots(1, 1, sharey=True)
    n, bins, patches = ax.hist(x_write, 1000)

    # the histogram of the data
    # n, bins, patches = ax1.hist(x_meas, 100)
    # n, bins, patches = ax2.hist(x_read, 100)
    # n, bins, patches = ax3.hist(x_write, 100)


    plt.title('Attacker randomly modify shared pages - pure COW (VM {})'.format(vm_name[vm]))
    plt.xlabel('enlapsed time')
    # ax2.xlabel('enlapsed time')
    # ax3.xlabel('enlapsed time')
    plt.ylabel('Probability')
    # ax2.ylabel('Probability')
    # ax3._label('Probability')
    # ax1.title('Histogram Measures')
    # ax2.title('Histogram Read')
    # ax3.title('Histogram Write')

    print("="*40+"\nnumber of cycle at 3.5Ghz\n"+"-"*40)
    # print("read : means {:.2f}, stdev {:.2f}".format(mean(x_read), stdev(x_read)))
    # low_r = [i*freq for i in x_read if i < 1000]
    # high_r = [i*freq for i in x_read if i > 1000]
    # print("read in cache : means {:.2f}, stdev {:.2f}".format(mean(low_r), stdev(low_r)))
    # print("read not in cache : means {:.2f}, stdev {:.2f}, min {}".format(mean(high_r), stdev(high_r), min(high_r)))

    print("write : means {:.2f}, stdev {:.2f}".format(mean(x_write), stdev(x_write)))
    low_w = [i for i in x_write if i < 1000]
    mid_w = [i for i in x_write if 1000 < i < 15000]
    high_w = [i for i in x_write if i > 15000]
    print("write in cache : means {:.2f}, stdev {:.2f}, min {}".format(mean(low_w), stdev(low_w), min(low_w)))
    # print("write not in cache : means {:.2f}, stdev {:.2f}, min {}".format(mean(mid_w), stdev(mid_w), min(mid_w)))
    print("copy on write  : means {:.2f}, stdev {:.2f}, min {}".format(mean(high_w), stdev(high_w), min(high_w)))

    plt.xlim([18000, 40000])
    plt.show()


def plot_exp4(mes, vm):

    freq = 1/3.5

    x_meas = [i['ts'][0] for i in mes if i['cmd'] == Prim_sca.PRIMITIVE_MEASURE.value]
    x_read = [i['ts'][1]-i['ts'][0] for i in mes if i['cmd'] == Prim_sca.PRIMITIVE_READ.value]
    x_write = [i['ts'][1]-i['ts'][0] for i in mes if i['cmd'] == Prim_sca.PRIMITIVE_WRITE.value]

    # plot
    # fig, (ax1, ax2, ax3) = plt.subplots(3, 1, sharey=True)
    fig, ax = plt.subplots(1, 1, sharey=True)
    n, bins, patches = ax.hist(x_write, 100)

    # the histogram of the data
    # n, bins, patches = ax1.hist(x_meas, 100)
    # n, bins, patches = ax2.hist(x_read, 100)
    # n, bins, patches = ax3.hist(x_write, 100)


    plt.title('Random write both own and shared pages - COW evidence (VM {})'.format(vm_name[vm]))
    plt.xlabel('enlapsed time')
    # ax2.xlabel('enlapsed time')
    # ax3.xlabel('enlapsed time')
    plt.ylabel('Probability')
    # ax2.ylabel('Probability')
    # ax3._label('Probability')
    # ax1.title('Histogram Measures')
    # ax2.title('Histogram Read')
    # ax3.title('Histogram Write')

    print("="*40+"\nnumber of cycle at 3.5Ghz\n"+"-"*40)
    # print("read : means {:.2f}, stdev {:.2f}".format(mean(x_read), stdev(x_read)))
    # low_r = [i*freq for i in x_read if i < 1000]
    # high_r = [i*freq for i in x_read if i > 1000]
    # print("read in cache : means {:.2f}, stdev {:.2f}".format(mean(low_r), stdev(low_r)))
    # print("read not in cache : means {:.2f}, stdev {:.2f}, min {}".format(mean(high_r), stdev(high_r), min(high_r)))

    print("write : means {:.2f}, stdev {:.2f}".format(mean(x_write), stdev(x_write)))
    low_w = [i for i in x_write if i < 1000]
    mid_w = [i for i in x_write if 1000 < i < 15000]
    high_w = [i for i in x_write if i > 15000]
    print("write in cache : means {:.2f}, stdev {:.2f}, min {}".format(mean(low_w), stdev(low_w), min(low_w)))
    print("write not in cache : means {:.2f}, stdev {:.2f}, min {}".format(mean(mid_w), stdev(mid_w), min(mid_w)))
    print("copy on write  : means {:.2f}, stdev {:.2f}, min {}".format(mean(high_w), stdev(high_w), min(high_w)))

    plt.show()


def plot_exp5(mes):

    freq = 1/3.5

    y_v = [i['ts'][0] for i in mes[0] if i['cmd'] == Prim_sca.PRIMITIVE_MEASURE.value]
    y_a = [i['ts'][0] for i in mes[1] if i['cmd'] == Prim_sca.PRIMITIVE_MEASURE.value]
    y_d = [i['ts'][0] for i in mes[2] if i['cmd'] == Prim_sca.PRIMITIVE_MEASURE.value]

    y_v = [y_v[i]-y_v[i-1] for i in range(1, len(y_v))]
    y_a = [y_a[i]-y_a[i-1] for i in range(1, len(y_a))]
    y_d = [y_d[i]-y_d[i-1] for i in range(1, len(y_d))]
    # plot
    # fig, (ax1, ax2, ax3) = plt.subplots(3, 1, sharey=True)
    fig, ax = plt.subplots(1, 1, sharey=True)
    x = list(range(len(y_v)))
    ax.plot(x, y_v)
    ax.plot(x, y_a)
    ax.plot(x, y_d)

    # the histogram of the data
    # n, bins, patches = ax1.hist(x_meas, 100)
    # n, bins, patches = ax2.hist(x_read, 100)
    # n, bins, patches = ax3.hist(x_write, 100)


    plt.title('Time (time counter) from VMs point of view')
    plt.xlabel('measures in time (period ~0.35ms)')
    # ax2.xlabel('enlapsed time')
    # ax3.xlabel('enlapsed time')
    plt.ylabel('number of cycle from previous measures')
    # ax2.ylabel('Probability')
    # ax3._label('Probability')
    # ax1.title('Histogram Measures')
    # ax2.title('Histogram Read')
    # ax3.title('Histogram Write')

    print("="*40+"\nnumber of cycle at 3.5Ghz\n"+"-"*40)
    # print("read : means {:.2f}, stdev {:.2f}".format(mean(x_read), stdev(x_read)))
    # low_r = [i*freq for i in x_read if i < 1000]
    # high_r = [i*freq for i in x_read if i > 1000]
    # print("read in cache : means {:.2f}, stdev {:.2f}".format(mean(low_r), stdev(low_r)))
    # print("read not in cache : means {:.2f}, stdev {:.2f}, min {}".format(mean(high_r), stdev(high_r), min(high_r)))

    print("write : means {:.2f}, stdev {:.2f}".format(mean(y_a), stdev(y_a)))
    low_w = [i for i in y_a if i < 2000000]
    high_w = [i for i in y_a if i > 2000000]
    print("common measures  : means  {:.0f}, stdev    {:.0f}, min  {}".format(mean(low_w), stdev(low_w), min(low_w)))
    print("context changes  : means {:.0f}, stdev {:.0f}, min {}".format(mean(high_w), stdev(high_w), min(high_w)))

    # plt.xlim([1000, 19000])
    # plt.ylim([0, 110])
    plt.show()


def plot_exp6(filename):

    freq = 1/3.5
    y = []
    struct_fmt = '<Q'
    struct_len = struct.calcsize(struct_fmt)
    with open(filename, 'rb') as f:
        bytes = f.read(struct_len)
        while bytes:
            y.append(struct.unpack(struct_fmt, bytes)[0]/1e6)
            bytes = f.read(struct_len)

    # y = y[1:]
    output = [idx for idx, element in enumerate(y) if element > 2]
    o2 = [y[i] for i in output]
    print(output)
    print(o2)
    a = [y[0]]
    for i in range(1, len(y)):
        a.append(math.log(a[i-1]+y[i]))
    # y = [sum(y[:i]) for i in range(0, len(y))]
    # plot
    # fig, (ax1, ax2, ax3) = plt.subplots(3, 1, sharey=True)
    fig, ax = plt.subplots(1, 1, sharey=True)
    x = list(range(len(y)))
    ax.plot(x, a)

    plt.title('Time (time counter) from VMs point of view Qemu')
    plt.xlabel('measures in time (period ~3.5ms)')
    plt.ylabel('number of cycle from previous measures')

    print("="*40+"\nnumber of cycle at 3.5Ghz\n"+"-"*40)
    print("write : means {:.2f}, stdev {:.2f}".format(mean(y), stdev(y)))
    plt.xlim([40000, 41000])
    # plt.ylim([0.715e11, 0.717e11])
    plt.show()


def plot_exp7(filename1, filename2):

    freq = 1/3.5
    y = []
    with open(filename1, 'rb') as f:
        lines = f.readlines()
        y1 = [int(line.rstrip()) for line in lines]
    with open(filename2, 'rb') as f:
        lines = f.readlines()
        y2 = [int(line.rstrip()) for line in lines]

    # y = [sum(y[:i]) for i in range(0, len(y))]
    # plot
    # fig, (ax1, ax2, ax3) = plt.subplots(3, 1, sharey=True)
    fig, ax = plt.subplots(1, 1, sharey=True)

    l = len(y1) if len(y1) < len(y2) else len(y2)
    y1 = y1[:l] if len(y1) > l else y1
    y2 = y2[:l] if len(y2) > l else y2
    x = list(range(l))
    ax.plot(x, y1, y2)

    plt.title('Shared pages in time from two identical VMs (alpine)')
    plt.xlabel('time (s)')
    plt.ylabel('shared pages')
    ax.legend(['amd Ryzen', 'intel i7'], loc='center right')
    plt.xlim([0, 410])
    # plt.ylim([0.715e11, 0.717e11])
    plt.show()


def plot_exp8(filename1, filename2):

    freq = 1/3.5
    y = []
    with open(filename1, 'rb') as f:
        lines = f.readlines()
        y1 = [int(line.rstrip()) for line in lines]
    with open(filename2, 'rb') as f:
        lines = f.readlines()
        y2 = [int(line.rstrip()) for line in lines]

    fig, ax = plt.subplots(1, 1, sharey=True)

    l = len(y1) if len(y1) < len(y2) else len(y2)
    y1 = y1[:l] if len(y1) > l else y1
    y2 = y2[:l] if len(y2) > l else y2
    x = list(range(l))
    ax.plot(x, y1, y2)

    plt.title('Shared pages in time Qemu only in BIOS mode or EFI mode')
    plt.xlabel('time (s)')
    plt.ylabel('shared pages')
    ax.legend(['EFI', 'BIOS'], loc='center right')
    plt.xlim([0, 200])
    # plt.ylim([0.715e11, 0.717e11])
    plt.show()


def plot_exp9(filename1):

    freq = 1/3.5
    y = []
    with open(filename1, 'rb') as f:
        lines = f.readlines()
        y = [int(line.rstrip()) for line in lines]

    fig, ax = plt.subplots(1, 1, sharey=True)

    x = list(range(len(y)))
    ax.plot(x, y)

    plt.title('Shared pages in time Qemu activity effect')
    plt.xlabel('time (s)')
    plt.ylabel('shared pages')
    # ax.legend(['EFI', 'BIOS'], loc='center right')
    plt.xlim([0, 200])
    # plt.ylim([0.715e11, 0.717e11])
    plt.show()


def main():
    """generate

    :return: exit status
    """
    parser = argparse.ArgumentParser(description='Metaheuristics for Optimization TP Series 5 : PSO.')
    parser.add_argument('file', help='file to parse', type=str)
    args = parser.parse_args()

    # try:
    res = read_timestamp(args.file)
    # except:
    #     pass

    # for vm in range(0, len(res)):
    #     for i in range(0, len(res[vm])):
    #         if res[vm][i]['cmd'] == 3:
    #             print_mes(res[vm][i], vm)

    if 'exp0' in args.file:
        plot_exp0(res[1], 1)
    if 'exp1' in args.file:
        plot_exp1(res[1], 1)
    if 'exp2' in args.file:
        plot_exp2(res[1], 1)
    if 'exp3' in args.file:
        plot_exp3(res[1], 1)
    if 'exp4' in args.file:
        plot_exp4(res[1], 1)
    if 'exp5' in args.file:
        plot_exp5(res)
    if 'exp6' in args.file:
        plot_exp6("../results/exp6_qemu_single_vm.dat")
    if 'exp7' in args.file:
        plot_exp7("../results/exp7-qemu_two_vm_page_shared_in_time.mesures.t480", "../results/exp7-qemu_two_vm_page_shared_in_time.mesures.r3700")
    if 'exp8' in args.file:
        plot_exp8("../results/exp8-qemu_EFI_two_empty_vm_page_shared_in_time.measure.r3700", "../results/exp8-qemu_BIOS_two_empty_vm_page_shared_in_time.measure.r3700")
    if 'exp9' in args.file:
        plot_exp9("../results/exp9-qemu_two_VM_stress_exp9.2.r3700")
    # distribution(res[2], 2)
    exit(EX_OK)


if __name__ == '__main__':
    exit(main())
