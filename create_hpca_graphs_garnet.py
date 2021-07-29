import os 
import math 

gem5_binary     = "/build/NULL/gem5.debug"
base_dir        = "/home/farabi/noc"
results_dir     = os.path.join(base_dir, "hpca_results")

injection_rate  = [0.001, 0.01, 0.05, 0.1, 0.15, 0.2, 0.25 ]
traffic_type    = ["uniform_random"]
attack_rate     = [0.1]
policy          = ["bypass_none", "bypass_all_out","jitter_all"]
sim_cycles      = 1000000
max_hpc         = 5
lower_limit     = 20
upper_limit     = 40

n_cpus          = 64
n_dirs          = 64
n_l2caches      = 64
n_rows          = int(math.sqrt(n_dirs))

attacker_node   = 0

lower_limit     = 20
upper_limit     = 40
delta_s         = 4


def get_out_dir(dir_name, ir, p, t):
    test_case = os.path.join(results_dir,"{:s}-{:03.0f}".format(p,ir*1000))
    print(test_case)    
    if not os.path.exists(test_case):
        try:
            os.makedirs(test_case)
        except(Exception):
            print(Exception)
    return test_case



for ir in injection_rate:
    for p in policy:
        for t in traffic_type:
            outdir = get_out_dir(base_dir, ir, p, t)
            s = os.path.join(base_dir,gem5_binary)
            s += "configs/example/garnet_synth_traffic.py "
            s += "outdir={} ".format(outdir)
            s += "--num-cpus={} ".format(n_cpus)
            s += "--num-dirs={} ".format(n_dirs)
            s += "--network=garnet2.0 "
            s += "--topology=Mesh_XY "
            s += "--mesh-row={} ".format(n_rows)
            s += "--sim-cycles={} ".format(sim_cycles)
            s += "--synthetic={} ".format(t)
            s += "--bypass={} ".format(p)
            s += "--injectionrate={} ".format(ir)
            s += "--attack-enabled "
            s += "--attack-node={} ".format(attacker_node)
            s += "--max-hpc={} ".format(max_hpc)
            s += "--lower-limit={} ".format(lower_limit)
            s += "--upper-limit={} ".format(upper_limit)
            s += "--delta-s={} ".format(delta_s) 
            print("{}".format(  s))