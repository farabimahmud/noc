# Copyright (c) 2008 Princeton University
# Copyright (c) 2009 Advanced Micro Devices, Inc.
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
#
# Author: Tushar Krishna
#

from m5.params import *
from m5.proxy import *
from m5.objects.Network import RubyNetwork
from m5.objects.BasicRouter import BasicRouter
from m5.objects.ClockedObject import ClockedObject

class GarnetNetwork(RubyNetwork):
    type = 'GarnetNetwork'
    cxx_header = "mem/ruby/network/garnet2.0/GarnetNetwork.hh"
    num_rows = Param.Int(0, "number of rows if 2D (mesh/torus/..) topology");
    ni_flit_size = Param.UInt32(16, "network interface flit size in bytes")
    vcs_per_vnet = Param.UInt32(4, "virtual channels per virtual network");
    buffers_per_data_vc = Param.UInt32(4, "buffers per data virtual channel");
    buffers_per_ctrl_vc = Param.UInt32(1, "buffers per ctrl virtual channel");
    routing_algorithm = Param.Int(0,
        "0: Weight-based Table, 1: XY, 2: Custom");
    enable_fault_model = Param.Bool(False, "enable network fault model");
    fault_model = Param.FaultModel(NULL, "network fault model");
    garnet_deadlock_threshold = Param.UInt32(5000000,
                              "network-level deadlock threshold")
    bypass_all = Param.Bool(False, "Enable bypass queue")
    bypass_none = Param.Bool(True, "Disable bypass for all aka baseline")
    jitter_all = Param.Bool(False, "Enable Jitter for all")
    bypass_x = Param.Bool(False, "Enable bypass for the x axis only")
    optimized = Param.Bool(False, "Enable optimized bypass")
    optimization_rate = Param.UInt32(50, "rate of optimization, default 50")
    attack_enabled = Param.Bool(False, "whether attack is enabled in"
            "this network,default is false")
    attack_node = Param.Int(-1, "ID of attack node, default -1 no attack node")
    min_cycles = Param.Int(4, "Minimum number of cycles we would allow")
    max_cycles = Param.Int(10, "Maximum number of cycles we would allow")
    dynamic_delay = Param.Bool(False, "Whether the optimization is dynamically \
        adjusted, default is False")

    fixed_target_enabled = Param.Bool(False, "Whether fixed target pair is \
        enabled for this simulation or not. Default is Not Enabled")
    fixed_target_near = Param.Int(-1, "Fixed Target Nearest Node")
    fixed_target_far = Param.Int(-1,"Fixed Target Farthest Node")
    randomly_selected_targets = Param.Bool(False, "Randomly selected target \
            default is False")

    all_out_bypass = Param.Bool(False, "All out bypass blocks all the output \
            units along the path of bypass until bypass is completed. \
            Default value false")

    max_hpc = Param.Int(3, "Maximum Hops Per Cycle one packet can bypass \
            depends on the technology. Default value is 3")
    lower_limit = Param.Int(-1, "Number of cycles it takes to reach the \
            farthest node using bypass.")
    upper_limit = Param.Int(40, "Number of cycles it takes to reach the \
            farthest node using Normal path. Default is 40")
    target_latency = Param.Int(20, "Number of cycles it takes to reach the \
            farthest node using Our Algorithm. Default is 20")

    delta_s  = Param.Int(4, "Number of cycles we cannot differentiate \
            the timing differences between. Default is 4")

    destination_list = Param.String("", "comma separated list of \
            destinations that the attacker going to use and we have \
            to protect from, Default is None")

class GarnetNetworkInterface(ClockedObject):
    type = 'GarnetNetworkInterface'
    cxx_class = 'NetworkInterface'
    cxx_header = "mem/ruby/network/garnet2.0/NetworkInterface.hh"

    id = Param.UInt32("ID in relation to other network interfaces")
    vcs_per_vnet = Param.UInt32(Parent.vcs_per_vnet,
                             "virtual channels per virtual network")
    virt_nets = Param.UInt32(Parent.number_of_virtual_networks,
                          "number of virtual networks")
    garnet_deadlock_threshold = Param.UInt32(Parent.garnet_deadlock_threshold,
                                      "network-level deadlock threshold")
    flit_jitter_threshold = Param.UInt32(20, "Minimum Jitter for flit")
  
class GarnetRouter(BasicRouter):
    type = 'GarnetRouter'
    cxx_class = 'Router'
    cxx_header = "mem/ruby/network/garnet2.0/Router.hh"
    vcs_per_vnet = Param.UInt32(Parent.vcs_per_vnet,
                              "virtual channels per virtual network")
    virt_nets = Param.UInt32(Parent.number_of_virtual_networks,
                          "number of virtual networks")
