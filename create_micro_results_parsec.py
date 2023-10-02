#!/bin/python3

import os
import math



attack_rate     = [1]
policy          = ["bypass_none", "jitter_all", "bypass_all_out"]
policy          = ['jitter_all']

debug_flag_lists= ["JitterAllStats"]
debug_flags     = ','.join(map(str, debug_flag_lists))
debug_file      = "debug.out"

input_size      = 'simmedium'

sim_cycles      = 1000000000
max_hpc         = 5
lower_limit     = 20
upper_limit     = 40
create_checkpoint = True

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
bypass_target_latency = 40
jitter_target_latency = 100


run_case        = "parsec_{}_all_all_{}_{}_{}_{}".format(input_size,
        jitter_target_latency, l1d_size, l2_size,sim_cycles)
gem5_binary     = "build/X86_MESI_Two_Level/gem5.opt"
benchmark_dir   = ""

base_dir        = os.path.abspath(os.getcwd())
results_dir     = os.path.join(base_dir,
        os.path.join("micro_results",run_case))
checkpoint_dir  = os.path.join(base_dir, os.path.join("ckpts",run_case))
bash_scripts_dir = os.path.join(base_dir, os.path.join(
    "micro_scripts",run_case))
config_file     = os.path.join(base_dir, "configs/example/fs.py")
rcs_script_path = os.path.join(base_dir, "parsec_script_generator")

list_of_application    = ['blackscholes', 'bodytrack',
        'canneal', 'dedup', 'facesim',
        'ferret', 'fluidanimate', 'freqmine', 'streamcluster',
        'swaptions', 'vips', 'x264']

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

    s += "--checkpoint-dir={} ".format(checkpoint_dir)
    s += "--kernel={} ".format('x86_64-vmlinux-2.6.28.4-smp')
    s += "--disk-image={} ".format('x86root-parsec.img')
    if create_checkpoint:
        s += "--cpu-type={} ".format("X86KvmCPU")
        # s += "-n 1 "  # blackscholes_64c_simdev_ckpts.rcS
        s += "--script={} ".format(
               os.path.join(rcs_script_path,"hack_back_ckpt.rcS"))

        # s += "script={}_{}c_{}_ckpts.rcS ".format(
        #        os.path.join(rcs_script_path,a),
        #        n_cpus,
        #        input_size)

    else:

        s += "--maxinsts={} ".format(sim_cycles)

        s += "--cpu-type={} ".format("DerivO3CPU")
        s += "--script={}_{}c_{}.rcS ".format(
                os.path.join(rcs_script_path,a),
                n_cpus,
                input_size)
        s += "--checkpont-restore 1 "

    s += "--ruby "
    s += "--caches "
    s += "--l2cache "
    s += "--num-l2caches={} ".format(n_cpus)
    s += "--l1d_size={} ".format(l1d_size)
    s += "--l2_size={} ".format(l2_size)
    s += "--bypass={} ".format(p)
    s += "--attack-enabled "
    s += "--attack-node={} ".format(attacker_node)
    s += "--max-hpc={} ".format(max_hpc)
    s += "--upper-limit={} ".format(upper_limit)

    # s += "--fast-forward=9223372036854775807 "
    # s += "--cmd={} ".format(application_cmd[a][0])
    # s += "--options=\"{}\" ".format(application_cmd[a][1])

    s += "--destination-list={} ".format(destination_list[0])
    if p == "jitter_all":
        s += "--target-latency={} ".format(jitter_target_latency)
    elif p == "bypass_all_out":
        s += "--target-latency={} ".format(bypass_target_latency)
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
                print("export M5_PATH={} ".format(base_dir), file=f)
                # print("#SBATCH --exclude=compute012,compute013,compute014",
                #     file=f)
                # print("#SBATCH --partition=ada", file=f)
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
                print("sh {} &".format(bash_script_file), file=f)


#print(get_out_dir(results_dir, "bfs","bypass_none"))
create_bash_script()
create_slurm_job()
