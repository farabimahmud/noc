#!/bin/python3

import os
import math



attack_rate     = [1]
policy          = ["bypass_none", "jitter_all", "bypass_all_out", "bypass_baseline"]
#policy          = ['jitter_all']

debug_flag_lists= ["JitterAllStats"]
debug_flags     = ','.join(map(str, debug_flag_lists))
debug_file      = "debug.out"

sim_cycles      = 10000000
max_hpc         = 5
lower_limit     = 20
upper_limit     = 100

l1d_size        = '32kB'
l2_size         = '2MB'
n_cpus          = 64
n_dirs          = 64
n_l2caches      = 64
n_rows          = int(math.sqrt(n_dirs))

dest_list_all = ",".join([ str(i) for i in range(64) ])
attacker_node = dest_list_all
# attacker_node   = 0
destination_list= [dest_list_all]
target_latency  = upper_limit
bypass_target_latency = 20
jitter_target_latency = 100


run_case        = "rodinia_all_all_{}_{}_{}_{}".format(
        jitter_target_latency, l1d_size, l2_size,sim_cycles)
gem5_binary     = "build/X86_MESIF_Two_Level/gem5.opt"
benchmark_dir   = "/home/grads/f/farabi/benchmarks/rodinia_3.0/"
base_dir        = os.path.abspath(os.getcwd())
results_dir     = os.path.join(base_dir, os.path.join("usenix23_results",run_case))
bash_scripts_dir = os.path.join(base_dir, os.path.join(
                    "usenix23_scripts",run_case))
config_file     = os.path.join(base_dir, "configs/example/se.py")

list_of_application = [
    'backprop',
    'b+tree',
     #'heartwall', (clobbered?)
    'kmeans',
    'lud',
    # 'nn',
    ##'particlefilter',
    'srad_v1',
    'srad_v2',
    'bfs',
    'cfd',
    'hotspot',
    ## 'lavaMD', #runtime error
    'myocyte',
    'nw',
    'pathfinder',
    'streamcluster',
    ]


application_cmd= {
    'kmeans'    : [benchmark_dir+"openmp/kmeans/kmeans_openmp/kmeans",
    "-n 64 -i "+benchmark_dir+"data/kmeans/kdd_cup"],
    'bfs'       :   [benchmark_dir+"openmp/bfs/bfs",
    "64 "+benchmark_dir+"data/bfs/graph1MW_6.txt"],
    'lavaMD'    :    [benchmark_dir+"openmp/lavaMD/lavaMD",
    "-cores 64 -boxes1d 10"],
    'lud'       :    [benchmark_dir+"openmp/lud/omp/lud_omp",
    "-n 64 -i "+benchmark_dir+"data/lud/512.dat"],
    'nn'        : [benchmark_dir+"openmp/nn/nn",
    "filelist_4 5 30 90"],
    'srad_v2'   :
        [benchmark_dir+"openmp/srad/srad_v2/srad",
    "2048 2048 0 127 0 127 64 0.5 2"],
    'srad_v1'   :
        [benchmark_dir+"openmp/srad/srad_v1/srad",
    "100 0.5 502 458 64"],
    'streamcluster' :
        [benchmark_dir+"openmp/streamcluster/sc_omp",
    "10 20 256 65536 65536 1000 none output.txt 64"],
    'nw'        : [benchmark_dir+"openmp/nw/needle",
    "2048 10 64"],
    'particlefilter' :
        [benchmark_dir+"openmp/particlefilter/particle_filter",
    "-x 128 -y 128 -z 10 -np 10000"],
    'cfd'       :
        [benchmark_dir+"openmp/cfd/euler3d_cpu",
    benchmark_dir+"data/cfd/fvcorr.down.097K"],
    'pathfinder':
        [benchmark_dir+"openmp/pathfinder/pathfinder",
    "100000 100 64"],
    'heartwall' :
        [benchmark_dir+"openmp/heartwall/heartwall",
    benchmark_dir+"data/heartwall/test.avi 20 64"],
    'backprop'  :
        [benchmark_dir+"openmp/backprop/backprop",
    "65536 64"],
    'b+tree'    :
        [benchmark_dir+"openmp/b+tree/b+tree.out",
    "core 64 file "+benchmark_dir+"data/b+tree/mil.txt command "
        +benchmark_dir+"data/b+tree/command.txt"],
    'hotspot'   :
        [benchmark_dir+"openmp/hotspot/hotspot",
    "512 512 2 64 "+benchmark_dir+"data/hotspot/temp_512  "
    +benchmark_dir+"data/hotspot/power_512"],
    'myocyte'   :
        [benchmark_dir+"openmp/myocyte/myocyte.out","100 1 0 64"]
}



def get_test_case(a,p):
    return "{}-{}".format(a,p)

def get_out_dir(dir_name, a,p):
    test_case = os.path.join(dir_name,get_test_case(a,p))
    #print(test_case)
    if not os.path.exists(test_case):
        try:
            os.makedirs(test_case)
        except(Exception):
            pass
            # print(Exception)
    return test_case


def get_gem5_command(a, p):
    outdir = get_out_dir(results_dir, a, p)
    s = os.path.join(base_dir,gem5_binary)
    #s += " --debug-flags={} ".format(debug_flags)
    #s += " --debug-file={} ".format(os.path.join(outdir,debug_file))
    s += " --outdir={} ".format(outdir)
    s += " --redirect-stdout "
    s += " --redirect-stderr "
    s += " --stdout-file={} ".format(
            os.path.join(outdir,"stdout.log"))
    s += " --stderr-file={} ".format(
            os.path.join(outdir,"stderr.log"))

    s += " {} ".format(config_file)

    s += "--num-cpus={} ".format(n_cpus)
    s += "--num-dirs={} ".format(n_dirs)
    s += "--network=garnet2.0 "
    s += "--topology=Mesh_XY "
    s += "--mesh-row={} ".format(n_rows)
    s += "--maxinsts={} ".format(sim_cycles)

    s+= "--cpu-type={} ".format("DerivO3CPU")

    s += "--ruby "
    s += "--caches "
    s += "--l2cache "
    s += "--num-l2caches={} ".format(n_cpus)
    s += "--l1d_size={} ".format(l1d_size)
    s += "--l2_size={} ".format(l2_size)
    s += "--fast-forward=9223372036854775807 "
    s += "--bypass={} ".format(p)
    s += "--attack-enabled "
    s += "--attack-node={} ".format(attacker_node)
    s += "--max-hpc={} ".format(max_hpc)
    # s += "--lower-limit={} ".format(lower_limit)
    s += "--cmd={} ".format(application_cmd[a][0])
    s += "--options=\"{}\" ".format(application_cmd[a][1])
    s += "--destination-list={} ".format(destination_list[0])
    if p == "jitter_all":
        s += "--target-latency={} ".format(jitter_target_latency)
        s += "--upper-limit={} ".format(jitter_target_latency)

    elif p == "bypass_all_out":
        s += "--target-latency={} ".format(bypass_target_latency)
        s += "--upper-limit={} ".format(bypass_target_latency)

    s += "--attack-rate={} ".format(attack_rate[0])
    return s

def create_bash_script():
    for a in list_of_application:
        for p in policy:
            gem5_command = get_gem5_command(a,p)
            bash_script_filename = "{}.sh".format(get_test_case(a,p))
            get_out_dir(bash_scripts_dir, a,p)
            bash_script_file = os.path.join(bash_scripts_dir,
                    bash_script_filename)
            print(gem5_command)
            with open(bash_script_file, "w") as f:
                print("#!/bin/bash", file=f)
                print("#SBATCH --exclude=compute012,compute013,compute014",
                        file=f)
                print("#SBATCH --partition=bigmo,dwc", file=f)
                print("\n", file=f)
                print("source ~/.bashrc", file=f)
                print("mkdir -p {}".format(
                    get_out_dir(bash_scripts_dir, a,p)),
                    file=f)
                print("{}".format(gem5_command), file=f)

def create_slurm_job():
    with open("{}.sh".format(run_case), "w") as f:
        print("#!/bin/bash",file=f)
        print("\n\n", file=f)

        for a in list_of_application:
            for p in policy:
                bash_script_filename = "{}.sh".format(get_test_case(a,p))
                bash_script_file = os.path.join(bash_scripts_dir,
                        bash_script_filename)
                print("chmod +x {} &&".format(bash_script_file), file=f)
                print("sbatch {}".format(bash_script_file), file=f)


#print(get_out_dir(results_dir, "bfs","bypass_none"))
create_bash_script()
create_slurm_job()
