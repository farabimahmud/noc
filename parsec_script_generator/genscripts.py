#!/bin/python3

import subprocess

applications    = ['blackscholes', 'bodytrack', 'canneal', 'dedup', 'facesim',
        'ferret', 'fluidanimate', 'freqmine', 'streamcluster',
        'swaptions', 'vips', 'x264', 'rtview']
nthreads        = [64]
inputs          = ['test', 'simdev','simmedium','simlarge']


for a in applications:
    for n in nthreads:
        cmdstring = ['./writescripts.pl', a, str(n)]
        subprocess.Popen(cmdstring)
        cmdstring.append('--ckpts')
        subprocess.Popen(cmdstring)


