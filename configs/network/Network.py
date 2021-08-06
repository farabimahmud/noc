# Copyright (c) 2016 Georgia Institute of Technology
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from __future__ import print_function
from __future__ import absolute_import

import math
import m5
from m5.objects import *
from m5.defines import buildEnv
from m5.util import addToPath, fatal

def define_options(parser):
    # By default, ruby uses the simple timing cpu
    parser.set_defaults(cpu_type="TimingSimpleCPU")

    parser.add_option("--topology", type="string", default="Crossbar",
                      help="check configs/topologies for complete set")
    parser.add_option("--mesh-rows", type="int", default=0,
                      help="the number of rows in the mesh topology")
    parser.add_option("--network", type="choice", default="simple",
                      choices=['simple', 'garnet2.0'],
                      help="'simple'|'garnet2.0'")
    parser.add_option("--router-latency", action="store", type="int",
                      default=1,
                      help="""number of pipeline stages in the garnet router.
                            Has to be >= 1.
                            Can be over-ridden on a per router basis
                            in the topology file.""")
    parser.add_option("--link-latency", action="store", type="int", default=1,
                      help="""latency of each link the simple/garnet networks.
                            Has to be >= 1.
                            Can be over-ridden on a per link basis
                            in the topology file.""")
    parser.add_option("--link-width-bits", action="store", type="int",
                      default=128,
                      help="width in bits for all links inside garnet.")
    parser.add_option("--vcs-per-vnet", action="store", type="int", default=4,
                      help="""number of virtual channels per virtual network
                            inside garnet network.""")
    parser.add_option("--routing-algorithm", action="store", type="int",
                      default=0,
                      help="""routing algorithm in network.
                            0: weight-based table
                            1: XY (for Mesh. see garnet2.0/RoutingUnit.cc)
                            2: Custom (see garnet2.0/RoutingUnit.cc""")
    parser.add_option("--network-fault-model", action="store_true",
                      default=False,
                      help="""enable network fault model:
                            see src/mem/ruby/network/fault_model/""")
    parser.add_option("--garnet-deadlock-threshold", action="store",
                      type="int", default=50000,
                      help="network-level deadlock threshold.")
    # bypass queue enable options
    parser.add_option("--bypass_enabled", action="store_true",
            default=False, help="Enable Bypass Queue")
    parser.add_option("--bypass", type="choice", default="bypass_all",
                      choices=['bypass_all', 'bypass_none', 'jitter_all',
                          'bypass_x','optimized','dynamic_delay',
                          'bypass_all_out'],
                      help="'bypass_none normal'")
    parser.add_option("--flit_jitter_threshold", type="int", default=40,
                      help="Minimum flit jitter threshold, default 40")
    parser.add_option("--optimization_rate", type="int", default=50,
                      help="""Rate at which optimized queue will"
                       be used, default 50""")
    parser.add_option("--max-hpc", type="int", default=3,
                      help="""Maximum hops a bypass packet can travel per
                      cycles, default=3""")
    
    parser.add_option("--delta-s", type="int", default=4,
            help="""Number of cycles beyond which we cannot differentiate
            default is 4""")

    parser.add_option("--lower-limit", type="int", default=-1,
            help="""Number of cycles it takes to reach the farthest node 
            using bypass Default is 10""")

    parser.add_option("--upper-limit", type="int", default=40,
            help="""Number of cycles it takes to reach the farthest node 
            using normal path Default is 40 cycles""")
    parser.add_option("--target-latency", type="int", default=20,
    help="""Number of cycles it takes to reach the farthest node 
    using Camouflage Default is 20""")


def create_network(options, ruby):

    # Set the network classes based on the command line options
    if options.network == "garnet2.0":
        NetworkClass = GarnetNetwork
        IntLinkClass = GarnetIntLink
        ExtLinkClass = GarnetExtLink
        RouterClass = GarnetRouter
        InterfaceClass = GarnetNetworkInterface

    else:
        NetworkClass = SimpleNetwork
        IntLinkClass = SimpleIntLink
        ExtLinkClass = SimpleExtLink
        RouterClass = Switch
        InterfaceClass = None

    # Instantiate the network object
    # so that the controllers can connect to it.
    network = NetworkClass(ruby_system = ruby, topology = options.topology,
            routers = [], ext_links = [], int_links = [], netifs = [])

    return (network, IntLinkClass, ExtLinkClass, RouterClass, InterfaceClass)

def init_network(options, network, InterfaceClass):

    if options.network == "garnet2.0":
        network.num_rows = options.mesh_rows
        network.vcs_per_vnet = options.vcs_per_vnet
        network.ni_flit_size = options.link_width_bits / 8
        network.routing_algorithm = options.routing_algorithm
        network.garnet_deadlock_threshold = options.garnet_deadlock_threshold

        network.bypass_all = False
        network.bypass_none = False
        network.bypass_x = False
        network.jitter_all = False
        network.all_out_bypass = False 

        if options.bypass == "bypass_all":
            network.bypass_all = True
        elif options.bypass == "bypass_none":
            network.bypass_none = True
        elif options.bypass == "jitter_all":
            network.jitter_all = True
        elif options.bypass == "bypass_x":
            network.bypass_x = True 
        elif options.bypass == "optimized":
            network.optimized = True
            network.optimization_rate = max(0,min(100,
                    int(options.optimization_rate)))
        elif options.bypass == "dynamic_delay":
            network.dynamic_delay = True
        elif options.bypass == "bypass_all_out":
            network.all_out_bypass = True 

        network.attack_enabled = options.attack_enabled
        network.attack_node = options.attack_node

        network.fixed_target_enabled = options.fixed_target_enabled
        network.fixed_target_near = options.fixed_target_near
        network.fixed_target_far  = options.fixed_target_far
        network.randomly_selected_targets = options.randomly_selected_targets

        network.max_hpc = options.max_hpc
        network.lower_limit = options.lower_limit 
        network.upper_limit = options.upper_limit 
        network.target_latency = options.target_latency
        network.delta_s = options.delta_s

        if options.destination_list is not None:
            s = options.destination_list.split(",")
            print("Destinations of Attack/Secure LDs are")
            for d in s:
                print(d, end=" ")
            network.destination_list = options.destination_list



    if options.network == "simple":
        network.setup_buffers()
    if InterfaceClass != None:
        netifs = [InterfaceClass(id=i) \
                  for (i,n) in enumerate(network.ext_links)]

        if options.bypass == "jitter_all":
            fl = int(options.flit_jitter_threshold)
            netifs = [InterfaceClass(id=i, flit_jitter_threshold=fl) \
                    for (i,n) in enumerate(network.ext_links)]
        network.netifs = netifs

    if options.network_fault_model:
        assert(options.network == "garnet2.0")
        network.enable_fault_model = True
        network.fault_model = FaultModel()
