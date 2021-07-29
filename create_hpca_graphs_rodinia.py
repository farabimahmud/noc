import os 
import math 

gem5_binary     = "build/NULL/gem5.debug"
base_dir        = "/home/farabi/noc"
results_dir     = os.path.join(base_dir, "hpca_results")
bash_scripts_dir = os.path.join(base_dir, "hpca_scripts/garnet_1/") 
config_file     = os.path.join(base_dir,
        "configs/example/se.py")
injection_rate  = [ 0.001*i + 0.00 for i in range(200) ]
traffic_type    = ["uniform_random"]
attack_rate     = [0.1]
policy          = ["bypass_none", "jitter_all"]

debug_flag_lists= ["JitterAllStats"]
debug_flags     = ','.join(map(str, debug_flag_lists))
debug_file      = "debug.out"

sim_cycles      = 1000000
max_hpc         = 5
lower_limit     = 20
upper_limit     = 40

n_cpus          = 64
n_dirs          = 64
n_l2caches      = 64
n_rows          = int(math.sqrt(n_dirs))

attacker_node   = 0


def get_test_case(p, ir):
    return "{:s}-{:03.0f}".format(p,ir*1000)

def get_out_dir(dir_name, ir, p, t):
    test_case = os.path.join(results_dir,get_test_case(p,ir))
    # print(test_case)    
    if not os.path.exists(test_case):
        try:
            os.makedirs(test_case)
        except(Exception):
            pass
            # print(Exception)
    return test_case


def get_gem5_command(ir, p, t):
    outdir = get_out_dir(base_dir, ir, p, t)
    s = os.path.join(base_dir,gem5_binary)
    s += " --debug-flags={} ".format(debug_flags)
    s += " --debug-file={} ".format(os.path.join(outdir,debug_file))
    s += " --outdir={} ".format(outdir)         
    s += " --redirect-stdout "         
    s += " --redirect-stderr "         
    s += " --stdout-file={} ".format(os.path.join(outdir,"stdout.log"))         
    s += " --stderr-file={} ".format(os.path.join(outdir,"stderr.log"))         

    s += " {} ".format(config_file)
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
    return s

def create_bash_script():
    for ir in injection_rate:
        for p in policy:
            for t in traffic_type:
                gem5_command = get_gem5_command(ir,p,t)
                bash_script_filename = "{}.sh".format(get_test_case(p,ir))
                bash_script_file = os.path.join(bash_scripts_dir,
                        bash_script_filename)

                print(gem5_command)
                with open(bash_script_file, "w") as f:
                    print("#!/bin/bash", file=f)
                    print("source ~/.bashrc", file=f)
                    print("mkdir -p {}".format(get_out_dir(base_dir,ir,p,t)),
                            file=f)
                    print("{}".format(gem5_command), file=f)
    
def create_slurm_job():
    with open("garnet_1.sh", "w") as f:
        print("#!/bin/bash",file=f)
        for ir in injection_rate:
            for p in policy:
                for t in traffic_type:
                    bash_script_filename = "{}.sh".format(get_test_case(p,ir))
                    bash_script_file = os.path.join(bash_scripts_dir,
                        bash_script_filename)

                    print("chmod +x {} &&".format(bash_script_file), file=f)
                    print("sbatch {}".format(bash_script_file), file=f)

               


    
create_bash_script()
create_slurm_job()
