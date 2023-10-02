#!/bin/python3

import os
import math

exclude_nodes_list = ["compute{:03}".format(x) for x in range(50,118)]
exclude_nodes    = ",".join(exclude_nodes_list)

partitions_list = ["bigmo","isen","dwc","normal","ada"]
partitions       = ",".join(partitions_list)

attack_rate     = [1]
policy          = ["bypass_none", "jitter_all", "bypass_all_out","bypass_baseline"]
# policy          = ['jitter_all']

debug_flag_lists= ["JitterAllStats"]
debug_flags     = ','.join(map(str, debug_flag_lists))
debug_file      = "debug.out"
input_sizes     = ["simmedium"]
input_size      = 'simmedium'

sim_cycles      = 1000000000
max_hpc         = 5
lower_limit     = 20
upper_limit     = 100
create_checkpoint = False

l1d_size        = '32kB'
l2_size         = '2MB'
n_cpus          = 64
n_dirs          = n_cpus
n_l2caches      = n_cpus
n_rows          = int(math.sqrt(n_dirs))

dest_list_all = ",".join([ str(i) for i in range(n_cpus) ])
attacker_node = dest_list_all
# attacker_node   = 0
destination_list= [dest_list_all]
target_latency  = upper_limit
bypass_target_latency = 20
jitter_target_latency = 100



run_case        = "parsec_timing_{}_all_all_{}_{}_{}_{}".format(input_size,
        jitter_target_latency, l1d_size, l2_size,sim_cycles)
gem5_binary     = "build/X86_MESIF_Two_Level/gem5.opt"
benchmark_dir   = ""

base_dir        = os.path.abspath(os.getcwd())
results_dir     = os.path.join(base_dir,
        os.path.join("usenix23_results",run_case))
checkpoint_dir  = os.path.join(base_dir,
    "ckpts/parsec_simmedium_all_all_100_32kB_2MB_ckpts")

bash_scripts_dir = os.path.join(base_dir, os.path.join(
    "usenix23_scripts",run_case))
if not os.path.exists(bash_scripts_dir):
    try:
        os.makedirs(bash_scripts_dir)
    except(Exception):
        pass


config_file     = os.path.join(base_dir, "configs/example/fs.py")
rcs_script_path = os.path.join(base_dir, "parsec_script_generator")

list_of_application    = ['blackscholes', 'bodytrack',
        'canneal', 'dedup', 'facesim',
        'ferret', 'fluidanimate', 'freqmine', 'streamcluster',
        'swaptions', 'vips', 'x264']

# list_of_application    = ['bodytrack']

job_command = "sh"
if base_dir == "/home/grads/f/farabi/noc":
    job_command = "sbatch" 


def get_test_case(a,p,i=input_size):
    return "{}-{}-{}".format(a,p,i)

def get_out_dir(dir_name, a,p,i=input_size):
    test_case = os.path.join(dir_name,get_test_case(a,p,i))
    #print(test_case)
    if not os.path.exists(test_case):
        try:
            os.makedirs(test_case)
        except(Exception):
            pass
        # print(Exception)
    return test_case


def get_gem5_command(a, p ,i=input_size):
    outdir = get_out_dir(results_dir, a, p,i)
    
    cur_ckpt_dir = get_out_dir(checkpoint_dir,a,p,i)
    if p == "bypass_baseline":
        cur_ckpt_dir = get_out_dir(checkpoint_dir,a,"bypass_all_out",i)
    s = os.path.join(base_dir,gem5_binary)
    #s += " --debug-flags={} ".format(debug_flags)
    #s += " --debug-file={} ".format(os.path.join(outdir,debug_file))
    s += " \\\n --outdir={} \\\n".format(outdir)
    s += " --redirect-stdout \\\n"
    s += " --redirect-stderr \\\n"
    s += " --stdout-file={} \\\n".format(
            os.path.join(outdir,"stdout.log"))
    s += " --stderr-file={} \\\n".format(
            os.path.join(outdir,"stderr.log"))

    s += " {} \\\n".format(config_file)

    s += "--num-cpus={} \\\n".format(n_cpus)
    s += "--num-dirs={} \\\n".format(n_dirs)
    s += "--network=garnet2.0 \\\n"
    s += "--topology=Mesh_XY \\\n"
    s += "--mesh-row={} \\\n".format(n_rows)

    s += "--checkpoint-dir={} \\\n".format(cur_ckpt_dir)
    s += "--kernel={} \\\n".format('x86_64-vmlinux-2.6.28.4-smp')
    s += "--disk-image={} \\\n".format('x86root-parsec.img')
    if create_checkpoint:
        s += "--cpu-type={} \\\n".format("AtomicSimpleCPU")
        s += "--script={} \\\n".format(
          os.path.join(rcs_script_path, "hack_back_ckpt.rcS")
        )
        # s += "--script={}_{}c_{}_ckpts.rcS \\\n".format(
        #         os.path.join(rcs_script_path,a),
        #         n_cpus,
        #         input_size)
        # s += "script={}_{}c_{}_ckpts.rcS ".format(
        #        os.path.join(rcs_script_path,a),
        #        n_cpus,
        #        input_size)

    else:

        if isinstance(sim_cycles,int):
            s += "--maxinsts={} \\\n".format(sim_cycles)

        s += "--restore-with-cpu={} \\\n".format("TimingSimpleCPU")
        s += "--script={}_{}c_{}.rcS \\\n".format(
                os.path.join(rcs_script_path,a),
                n_cpus,
                input_size)
        s += '--checkpoint-restore=1 \\\n'
    s += "--ruby \\\n"
    s += "--caches \\\n"
    s += "--l2cache \\\n"
    s += "--num-l2caches={} \\\n".format(n_cpus)
    s += "--l1d_size={} \\\n".format(l1d_size)
    s += "--l2_size={} \\\n".format(l2_size)
    s += "--bypass={} \\\n".format(p)
    s += "--attack-enabled \\\n"
    s += "--attack-node={} \\\n".format(attacker_node)
    s += "--max-hpc={} \\\n".format(max_hpc)

    # s += "--fast-forward=9223372036854775807 "
    # s += "--cmd={} ".format(application_cmd[a][0])
    # s += "--options=\"{}\" ".format(application_cmd[a][1])

    s += "--destination-list={} \\\n".format(destination_list[0])
    if p == "jitter_all":
        s += "--target-latency={} \\\n".format(jitter_target_latency)
        s += "--upper-limit={} \\\n".format(jitter_target_latency)
    elif p == "bypass_all_out":
        s += "--target-latency={} \\\n".format(bypass_target_latency)
        s += "--upper-limit={} \\\n".format(bypass_target_latency)
    elif p == "bypass_baseline":
        s += "--target-latency={} \\\n".format(0)
        s += "--upper-limit={} \\\n".format(bypass_target_latency)

    s += "--attack-rate={} ".format(attack_rate[0])
    return s

def create_bash_script():
    for a in list_of_application:
        for p in policy:
          for i in input_sizes:
              gem5_command = get_gem5_command(a,p,i)
              bash_script_filename = "{}.sh".format(get_test_case(a,p,i))
              # get_out_dir(bash_scripts_dir, a,p)
              bash_script_file = os.path.join(bash_scripts_dir,
                      bash_script_filename)
              print(gem5_command)
              with open(bash_script_file, "w") as f:
                  print("#!/bin/bash", file=f)
                  if job_command == "sbatch":
                      print("#SBATCH --exclude={}".format(exclude_nodes),
                      file=f)
                      print("#SBATCH --partition={}".format(partitions), file=f)
                  print("\n", file=f)
                  print("export M5_PATH={} ".format(base_dir), file=f)

                  print("source ~/.bashrc", file=f)
                  # print("mkdir -p {}".format(os.path.join(
                  #     bash_scripts_dir,
                  #     get_test_case(a,p)),
                  #     file=f)
                  print("{}".format(gem5_command), file=f)

def create_slurm_job():
    job_filename = "{}.sh".format(run_case)
    with open(job_filename, "w") as f:
        print("#!/bin/bash",file=f)
        print("\n\n", file=f)

        for a in list_of_application:
            for p in policy:
                bash_script_filename = "{}.sh".format(get_test_case(a,p))
                bash_script_file = os.path.join(bash_scripts_dir,
                        bash_script_filename)
                print("chmod +x {} &&".format(bash_script_file), file=f)
                print("{} {} ".format(job_command, bash_script_file), file=f)

    print(job_filename)


#print(get_out_dir(results_dir, "bfs","bypass_none"))
create_bash_script()
create_slurm_job()
