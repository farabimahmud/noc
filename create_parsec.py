#!/usr/bin/python
from __future__ import print_function


import sys
import os
import math

schemes = [
         'optimized',
        # 'bypass_none'
        #'optimized',
        #'SpectreSafeInvisibleSpec',
        #'FuturisticSafeInvisibleSpec',
        ]

isas = [
        #'arm',
        'x86'
        ]
num_cpus = num_threads = [
        #'4',
        # '8',
        '16',
        #'64',
        ]

data_size = 'simmedium'
runtype = 'restore'  #'restore' 'ckpts'
l1_victim_sizes = [16]
l2_victim_sizes = [16]
#victim_sizes = l2_victim_sizes 
# victim_sizes=[(4,4)]
#test_case = 'vc-conservative-spec-try3-upto-100m'
#test_case = 'vc-unsafe-parsec-arm-l1-100m'
test_case = 'week-jan-26-100m'

m5_root = '/home/grads/f/farabi/noc/m5out'

benches = [
          'blackscholes',
         #   'bodytrack',
          'canneal',
          'dedup',
          'facesim',
          # 'ferret',      # Segmentation fault with 16, 64 cpus
          'fluidanimate',
          'freqmine',
          'streamcluster',
          'swaptions',
          'x264',
            ]
os.environ["GEM5_PATH"] = "/home/grads/f/farabi/noc/"
parsec_dir = "/home/grads/f/farabi/parsec/"
rates = [0,25, 50, 75, 100]

def create_command(params):

    bench = params['bench']
    num_cpu = params['num_cpu']
    num_thread = params['num_cpu']
    scheme = params['scheme']
    data_size = params['data_size']
    isa = params['isa']
    runtype = params['runtype']
    rate = params['rate']

    gem5_options = []
    script_options = []

    if isa == 'x86':
        os.environ['M5_PATH'] = '/home/grads/f/farabi/gem5-kernel/x86-parsec'

        gem5_options.append(('bin-path'    ,
            '/home/grads/f/farabi/noc/build/X86_MESI_Two_Level/gem5.opt'))

        script_options.append(('--script',
            ('/home/grads/f/farabi/noc/scripts_x86/%s_%sc_%s_ckpts.rcS' % \
                    (bench, num_thread, data_size))))

        script_options.append(('--kernel',
            os.environ['M5_PATH'] + \
                    '/binaries/x86_64-vmlinux-2.6.28.4-smp'))

        script_options.append(('--disk-image',
            os.environ['M5_PATH'] + \
                    '/disks/x86root.img'))
    else:
        print ("unknown isa" + isa)
        exit()

    if runtype == 'restore':
        gem5_options.append(('--outdir',
            ('%s/restore/%s/%s-%s-%s-%s-%s-%d' % \
                (m5_root, test_case, bench, \
                num_cpu, data_size, isa, scheme, rate))))
        outdir = '%s/restore/%s/%s/%s-%s-%s-%s-%s-%d' % \
                (m5_root,isa, test_case, bench, \
                num_cpu, data_size, isa, scheme, rate)
        script_options.append(('--cpu-type', 'DerivO3CPU'))
        script_options.append(('--checkpoint-dir',
            ('/home/grads/f/farabi/nuca/m5out/ckpts/%s-%s-%s-%s' % ( bench, \
                    num_cpu, data_size, isa))))
        script_options.append(('--checkpoint-restore', '1'))

    elif runtype == 'ckpts':
        gem5_options.append(('--outdir',
            ('%s/ckpts/%s-%s-%s-%s-%d' % (m5_root, bench, \
                    num_cpu, data_size, isa,rate))))
        outdir = '%s/ckpts/%s-%s-%s-%s-%d' % \
                (m5_root, bench, num_cpu, data_size, isa, rate)
        script_options.append(('--cpu-type',
            'AtomicSimpleCPU'))
#        script_options.append(('--take-checkpoints',
#            '100000000 --at-instruction'))
        script_options.append(('--checkpoint-dir',
            ('%s/ckpts/%s-%s-%s-%s' % \
                    (m5_root, bench, num_cpu, data_size, isa))))


    elif runtype == 'full':
        gem5_options.append(('--outdir',
            ('%s/full/%s-%s-%s-%s-%s' % \
                    (m5_root, bench, num_cpu, data_size, isa, scheme))))
        outdir = '%s/full/%s-%s-%s-%s-%s' % \
                (m5_root, bench, num_cpu, data_size, isa, scheme)
        script_options.append(('--cpu-type',
            'DerivO3CPU'))
    else:
        print ("unknown runtype " + runtype)
        exit()

    # common options
    #gem5_options.append(('--debug-flags', \
    #        'ProtocolTrace,RubyQueue,O3CPUAll,HWSecurity'))
    # gem5_options.append(('--debug-flags', 'Naive'))
    #gem5_options.append(('--debug-flags', 'HWDebug'))
    #gem5_options.append(('--debug-flags', 'RubyVictim'))
    #gem5_options.append(('--debug-flags', 'ROB,LSQUnit'))
    #gem5_options.append(('--debug-flags','ProtocolTrace,RubySliccVictim'))
    #gem5_options.append(('--debug-file', 'debug.out'))
    # canneal vc4 - dealdlock 11825289228000
    #gem5_options.append(('--debug-start', '11825000228000'))
    #gem5_options.append(('--debug-start', '8741728504000'))
    #gem5_options.append(('--redirect-stdout', ''))
    #gem5_options.append(('--redirect-stderr', ''))
    script_options.insert(0,
            ('config', os.environ['GEM5_PATH'] + \
                    'configs/example/fs.py'))
    script_options.append(('--num-cpus', num_cpu))

    # memory options
    script_options.append(('--mem-size', '2GB'))
    if runtype == 'restore':
        script_options.append(('--ruby', ''))
        script_options.append(('--num-l2caches', num_cpu))
        script_options.append(('--num-dirs', num_cpu))
        script_options.append(('--l1d_assoc', '8'))
        script_options.append(('--l2_assoc', '16'))
        script_options.append(('--l1i_assoc', '4'))


        # NOC options
        script_options.append(('--network', 'garnet2.0'))
        script_options.append(('--topology', 'Mesh_XY'))

        if num_cpu == '8':
            mesh_rows = 4
        elif num_cpu == '64':
            mesh_rows = 8
        else:
            mesh_rows = int(math.log(int(num_cpu), 2))

        assert(type(mesh_rows) == int)
        script_options.append(('--mesh-rows', str(mesh_rows)))

        # InvisiSpec
        script_options.append(('--bypass', scheme))
        script_options.append(('--flit_jitter_threshold', 40 ))
        script_options.append(('--optimization_rate', rate))
        #script_options.append(('--needsTSO', '1'))
        script_options.append(('--maxinsts', '100000000'))

    print(outdir)
    return gem5_options, script_options, outdir

counter = 0
jobs_filename = '500.slurm.jobs'
with open(jobs_filename, 'w') as f:
    print('#!/bin/sh', file=f)
    print('\n\n\n', file=f)

print(benches)
print(schemes)
print(isas)
print(num_cpus)
script_dir = "parsec_scripts"
if not os.path.exists(script_dir):
    print(script_dir, "created")
    os.makedirs(script_dir)

for rate in rates:
    for bench in benches:
        for scheme in schemes:
            for isa in isas:
                for num_cpu in num_cpus:
                    params = {}
                    params['bench'] = bench
                    params['num_cpu'] = num_cpu
                    params['data_size'] = data_size
                    params['scheme'] = scheme
                    params['isa'] = isa
                    params['runtype'] = runtype
                    params['rate'] = rate
                    gem5_opts, script_opts, outdir = create_command(params)

                    command = ''
                    debug_command = ''
                    for opts in [gem5_opts, script_opts]:
                         for opt, val in opts:
                         # manage single value options
                              if opt in ['bin-path', 'config']:
                                  command += val + ' '
                                  debug_command += val + '\n'
                              elif opt in ['--ruby', '--has-victim', \
                                           '--redirect-stdout', \
                                           '--redirect-stderr']:
                                  command += opt + ' '
                                  debug_command += opt + '\n'
                              else:
                                  command += '%s=%s ' % (opt, val)
                                  debug_command += '%s=%s\n' % (opt, val)

                              # create folder if this
                              # options specifies folder path
                              if opt in ['--outdir', '--checkpoint-dir']:
                                  try:
                                      os.makedirs(val)
                                  except OSError:
                                      # print("OSError")
                                      pass
                    create_dir_command = 'mkdir -p %s' % (outdir)
                    filename = "%s/%d-%s-%s-%d.sh" % (script_dir, counter, isa,
                            bench, rate)
                    with open(filename, "w") as f:
                        print('#!/bin/bash', file=f)
                        print("\n", file=f)
                        print(create_dir_command, file=f)
                        print(command, file=f )
                        #print command
                        #print ('\n')
                    with open(jobs_filename, 'a' ) as f:
                        print('chmod +x /home/grads/f/farabi/noc/%s && '%(
                            filename), file=f)
                        print('sbatch  /home/grads/f/farabi/noc/%s'%(
                            filename), file=f)

                        counter+=1
