/*
 * Copyright (c) 2008 Princeton University
 * Copyright (c) 2016 Georgia Institute of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "mem/ruby/network/garnet2.0/GarnetNetwork.hh"

#include <cassert>
#include <string>
#include <sstream>

#include "base/cast.hh"
#include "debug/AttackPacketGenerator.hh"
#include "debug/Naive.hh"
#include "debug/SK.hh"
#include "debug/Vanilla.hh"
#include "mem/ruby/common/NetDest.hh"
#include "mem/ruby/network/MessageBuffer.hh"
#include "mem/ruby/network/garnet2.0/CommonTypes.hh"
#include "mem/ruby/network/garnet2.0/CreditLink.hh"
#include "mem/ruby/network/garnet2.0/GarnetLink.hh"
#include "mem/ruby/network/garnet2.0/NetworkInterface.hh"
#include "mem/ruby/network/garnet2.0/NetworkLink.hh"
#include "mem/ruby/network/garnet2.0/Router.hh"
#include "mem/ruby/system/RubySystem.hh"

using namespace std;
int GarnetNetwork::PACKETID = 0;
/*
 * GarnetNetwork sets up the routers and links and collects stats.
 * Default parameters (GarnetNetwork.py) can be overwritten from command line
 * (see configs/network/Network.py)
 */


    GarnetNetwork::GarnetNetwork(const Params *p)
: Network(p)
{
    m_num_rows = p->num_rows;
    m_ni_flit_size = p->ni_flit_size;
    m_vcs_per_vnet = p->vcs_per_vnet;
    m_buffers_per_data_vc = p->buffers_per_data_vc;
    m_buffers_per_ctrl_vc = p->buffers_per_ctrl_vc;
    m_routing_algorithm = p->routing_algorithm;

    m_enable_fault_model = p->enable_fault_model;
    if (m_enable_fault_model)
        fault_model = p->fault_model;

    m_vnet_type.resize(m_virtual_networks);

    for (int i = 0 ; i < m_virtual_networks ; i++) {
        if (m_vnet_type_names[i] == "response")
            m_vnet_type[i] = DATA_VNET_; // carries data (and ctrl) packets
        else
            m_vnet_type[i] = CTRL_VNET_; // carries only ctrl packets
    }

    // record the routers
    for (vector<BasicRouter*>::const_iterator i =  p->routers.begin();
            i != p->routers.end(); ++i) {
        Router* router = safe_cast<Router*>(*i);
        m_routers.push_back(router);

        // initialize the router's network pointers
        router->init_net_ptr(this);
    }
    all_out_bypass = p->all_out_bypass;

    // record the network interfaces
    int tmp_i = 0;
    for (vector<ClockedObject*>::const_iterator i = p->netifs.begin();
            i != p->netifs.end(); ++i) {
        NetworkInterface *ni = safe_cast<NetworkInterface *>(*i);
        m_nis.push_back(ni);
        ni->init_net_ptr(this);
        DPRINTF(SK, "creating %d nis: %s\n", tmp_i++, *ni);
    }
    jitter_all = p->jitter_all;
    optimized = p->optimized;
    bypass_all = p->bypass_all;
    bypass_x = p->bypass_x;

    if (p->optimized){
        optimization_rate = p->optimization_rate;
    }

    // attack node information
    attack_enabled = p->attack_enabled;
    if (attack_enabled){
        attack_node = p->attack_node;

        DPRINTF(AttackPacketGenerator, "[GN] Attack Node is set to %d\n",
                attack_node);
    }
    min_cycles = p->min_cycles;
    max_cycles = p->max_cycles;
    dynamic_delay = p->dynamic_delay; 

    // initialize expected_delay if scheme is dynamic
    if (all_out_bypass){
        DPRINTF(Vanilla, "All out bypass enabled\n");
    }
    max_hpc = p->max_hpc;
    upper_limit = Cycles(p->upper_limit);
    delta_s = Cycles(p->delta_s);
    target_latency = Cycles(p->target_latency);

    if (p->destination_list.length() > 0){
        DPRINTF(Vanilla, "Setting Up Destination List\n");
        std::stringstream stream(p->destination_list);
        std::string token; 
        while(std::getline(stream, token, ',')){
            int dest_node = std::stoi(token);
            assert(0 < dest_node &&  dest_node < m_routers.size());
            destination_list.push_back(dest_node);
        }
        DPRINTF(Vanilla, "[GN] Destination List is - \n");
        for (auto d:destination_list){
            DPRINTF(Vanilla, "%d\n", d);
        }

       
    }
    if (p->lower_limit != -1){
        DPRINTF(Vanilla, "Overriding lower limit from the destinations\n");
        lower_limit = Cycles(p->lower_limit);
    }
    closest_rt = Cycles(p->target_latency);
}

int
GarnetNetwork::get_bypass_cost(int src, int dest){
    int x_src = src % m_num_cols;
    int y_src = src / m_num_cols;
    int x_dest = dest % m_num_cols;
    int y_dest = dest / m_num_cols;  
    int x_cost = abs(x_src - x_dest);
    int y_cost = abs(y_src - y_dest);      
    int bypass_cost = 0; 
    bypass_cost = int(ceil( (float) (x_cost+y_cost)/ (float) max_hpc)); 
    DPRINTF(Vanilla,"Bypass cost from src %d to dest %d is %d\n",
            src,
            dest,
            bypass_cost);
    assert(bypass_cost != 0 && "bypass cost cannot be 0");
    return bypass_cost;
}

Cycles 
GarnetNetwork::get_farthest_node_bypass_cost(int src, std::vector<int> dlist){
    std::vector<int> bypass_cost_from_source;   
    for (auto d:dlist){
        bypass_cost_from_source.push_back(get_bypass_cost(src,d));
     }
    int max_value = *std::max_element(
            bypass_cost_from_source.begin(), 
            bypass_cost_from_source.end());

    int closest_index = std::min_element(
            bypass_cost_from_source.begin(), 
            bypass_cost_from_source.end()
            ) - bypass_cost_from_source.begin();
    closest_dest  = dlist[closest_index];

    int farthest_index = std::max_element(
            bypass_cost_from_source.begin(), 
            bypass_cost_from_source.end()
            ) - bypass_cost_from_source.begin();
    farthest_dest = dlist[farthest_index];
    DPRINTF(Vanilla, "Closest %d Farthest %d lower %d\n", closest_dest, farthest_dest, max_value);
    return Cycles(max_value);
}

    void
GarnetNetwork::init()
{
    Network::init();

    for (int i=0; i < m_nodes; i++) {
        m_nis[i]->addNode(m_toNetQueues[i], m_fromNetQueues[i]);
    }

    // The topology pointer should have already been initialized in the
    // parent network constructor
    assert(m_topology_ptr != NULL);
    m_topology_ptr->createLinks(this);

    // Initialize topology specific parameters
    if (getNumRows() > 0) {
        // Only for Mesh topology
        // m_num_rows and m_num_cols are only used for
        // implementing XY or custom routing in RoutingUnit.cc
        m_num_rows = getNumRows();
        m_num_cols = m_routers.size() / m_num_rows;
        assert(m_num_rows * m_num_cols == m_routers.size());
    } else {
        m_num_rows = -1;
        m_num_cols = -1;
    }

    // FaultModel: declare each router to the fault model
    if (isFaultModelEnabled()) {
        for (vector<Router*>::const_iterator i= m_routers.begin();
                i != m_routers.end(); ++i) {
            Router* router = safe_cast<Router*>(*i);
            int router_id M5_VAR_USED =
                fault_model->declare_router(router->get_num_inports(),
                        router->get_num_outports(),
                        router->get_vc_per_vnet(),
                        getBuffersPerDataVC(),
                        getBuffersPerCtrlVC());
            assert(router_id == router->get_id());
            router->printAggregateFaultProbability(cout);
            router->printFaultVector(cout);
        }
    }

    // Reset router entries if dynamic delay is enabled
    if(dynamic_delay){
        for (int i=0; i<m_routers.size(); i++){
            m_routers[i]->resetExpectedDelay();
        }
    }

    createOutputUnitTable(m_routers.size());
    lower_limit  = get_farthest_node_bypass_cost(attack_node, destination_list);

}

/*
 * This function creates a link from the Network Interface (NI)
 * into the Network.
 * It creates a Network Link from the NI to a Router and a Credit Link from
 * the Router to the NI
 */

    void
GarnetNetwork::makeExtInLink(NodeID src, SwitchID dest, BasicLink* link,
        const NetDest& routing_table_entry)
{
    assert(src < m_nodes);

    GarnetExtLink* garnet_link = safe_cast<GarnetExtLink*>(link);

    // GarnetExtLink is bi-directional
    NetworkLink* net_link = garnet_link->m_network_links[LinkDirection_In];
    net_link->setType(EXT_IN_);
    CreditLink* credit_link = garnet_link->m_credit_links[LinkDirection_In];

    m_networklinks.push_back(net_link);
    m_creditlinks.push_back(credit_link);

    PortDirection dst_inport_dirn = "Local";
    m_routers[dest]->addInPort(dst_inport_dirn, net_link, credit_link);
    m_nis[src]->addOutPort(net_link, credit_link, dest);
}

/*
 * This function creates a link from the Network to a NI.
 * It creates a Network Link from a Router to the NI and
 * a Credit Link from NI to the Router
 */

    void
GarnetNetwork::makeExtOutLink(SwitchID src, NodeID dest, BasicLink* link,
        const NetDest& routing_table_entry)
{
    assert(dest < m_nodes);
    assert(src < m_routers.size());
    assert(m_routers[src] != NULL);

    GarnetExtLink* garnet_link = safe_cast<GarnetExtLink*>(link);

    // GarnetExtLink is bi-directional
    NetworkLink* net_link = garnet_link->m_network_links[LinkDirection_Out];
    net_link->setType(EXT_OUT_);
    CreditLink* credit_link = garnet_link->m_credit_links[LinkDirection_Out];

    m_networklinks.push_back(net_link);
    m_creditlinks.push_back(credit_link);

    PortDirection src_outport_dirn = "Local";
    m_routers[src]->addOutPort(src_outport_dirn, net_link,
            routing_table_entry,
            link->m_weight, credit_link);
    m_nis[dest]->addInPort(net_link, credit_link);
}

/*
 * This function creates an internal network link between two routers.
 * It adds both the network link and an opposite credit link.
 */

    void
GarnetNetwork::makeInternalLink(SwitchID src, SwitchID dest, BasicLink* link,
        const NetDest& routing_table_entry,
        PortDirection src_outport_dirn,
        PortDirection dst_inport_dirn)
{
    GarnetIntLink* garnet_link = safe_cast<GarnetIntLink*>(link);

    // GarnetIntLink is unidirectional
    NetworkLink* net_link = garnet_link->m_network_link;
    net_link->setType(INT_);
    CreditLink* credit_link = garnet_link->m_credit_link;

    m_networklinks.push_back(net_link);
    m_creditlinks.push_back(credit_link);

    m_routers[dest]->addInPort(dst_inport_dirn, net_link, credit_link);
    m_routers[src]->addOutPort(src_outport_dirn, net_link,
            routing_table_entry,
            link->m_weight, credit_link);
    src_r_link_dest_r_map[std::make_pair(src,net_link)] = dest;
    // DPRINTF(Vanilla, "Router %d connected via %#x to Router %d\n",
    //         src, net_link, dest);
}

// Total routers in the network
    int
GarnetNetwork::getNumRouters()
{
    return m_routers.size();
}

// Get ID of router connected to a NI.
    int
GarnetNetwork::get_router_id(int ni)
{
    return m_nis[ni]->get_router_id();
}



    void
GarnetNetwork::regStats()
{
    Network::regStats();

    // Packets

    m_packet_network_latency_dist
        .init(0, 50, 2 )
        .name(name() + ".packet_network_latency_dist")
        .flags(Stats::oneline)
        .precision(12)       
        ;
    
    m_closest_dest_attack_packet_latency
        .init(0,100,5)
        .name(name() + ".closest_dest_attack_packet_latency")
        .flags(Stats::oneline)
        .precision(12)
        ;

    m_farthest_dest_attack_packet_latency
        .init(0,100,5)
        .name(name() + ".farthest_dest_attack_packet_latency")
        .flags(Stats::oneline)
        .precision(12)
       ; 

    m_attack_packet_latency
        .init(0, 100, 5 )
        .name(name() + ".attack_packet_latency")
        .flags(Stats::oneline)
        .precision(12)
        ;

    m_regular_packet_latency
        .init(0, 100, 5 )
        .name(name() + ".regular_packet_latency")
        .flags(Stats::oneline)
        .precision(12)        
        ;

    m_packets_received
        .init(m_virtual_networks)
        .name(name() + ".packets_received")
        .flags(Stats::pdf | Stats::total | Stats::nozero | Stats::oneline)
        ;

    m_packets_injected
        .init(m_virtual_networks)
        .name(name() + ".packets_injected")
        .flags(Stats::pdf | Stats::total | Stats::nozero | Stats::oneline)
        ;

    m_packet_network_latency
        .init(m_virtual_networks)
        .name(name() + ".packet_network_latency")
        .flags(Stats::oneline)
        ;

    m_packet_queueing_latency
        .init(m_virtual_networks)
        .name(name() + ".packet_queueing_latency")
        .flags(Stats::oneline)
        ;

    for (int i = 0; i < m_virtual_networks; i++) {
        m_packets_received.subname(i, csprintf("vnet-%i", i));
        m_packets_injected.subname(i, csprintf("vnet-%i", i));
        m_packet_network_latency.subname(i, csprintf("vnet-%i", i));
        m_packet_queueing_latency.subname(i, csprintf("vnet-%i", i));
    }

    m_avg_packet_vnet_latency
        .name(name() + ".average_packet_vnet_latency")
        .flags(Stats::oneline);
    m_avg_packet_vnet_latency =
        m_packet_network_latency / m_packets_received;

    m_avg_packet_vqueue_latency
        .name(name() + ".average_packet_vqueue_latency")
        .flags(Stats::oneline);
    m_avg_packet_vqueue_latency =
        m_packet_queueing_latency / m_packets_received;

    m_avg_packet_network_latency
        .name(name() + ".average_packet_network_latency");
    m_avg_packet_network_latency =
        sum(m_packet_network_latency) / sum(m_packets_received);

    m_avg_packet_queueing_latency
        .name(name() + ".average_packet_queueing_latency");
    m_avg_packet_queueing_latency
        = sum(m_packet_queueing_latency) / sum(m_packets_received);

    m_avg_packet_latency
        .name(name() + ".average_packet_latency");
    m_avg_packet_latency
        = m_avg_packet_network_latency + m_avg_packet_queueing_latency;

    // Flits
    m_flits_received
        .init(m_virtual_networks)
        .name(name() + ".flits_received")
        .flags(Stats::pdf | Stats::total | Stats::nozero | Stats::oneline)
        ;

    m_flits_injected
        .init(m_virtual_networks)
        .name(name() + ".flits_injected")
        .flags(Stats::pdf | Stats::total | Stats::nozero | Stats::oneline)
        ;

    m_flit_network_latency
        .init(m_virtual_networks)
        .name(name() + ".flit_network_latency")
        .flags(Stats::oneline)
        ;

    m_flit_queueing_latency
        .init(m_virtual_networks)
        .name(name() + ".flit_queueing_latency")
        .flags(Stats::oneline)
        ;

    for (int i = 0; i < m_virtual_networks; i++) {
        m_flits_received.subname(i, csprintf("vnet-%i", i));
        m_flits_injected.subname(i, csprintf("vnet-%i", i));
        m_flit_network_latency.subname(i, csprintf("vnet-%i", i));
        m_flit_queueing_latency.subname(i, csprintf("vnet-%i", i));
    }

    m_avg_flit_vnet_latency
        .name(name() + ".average_flit_vnet_latency")
        .flags(Stats::oneline);
    m_avg_flit_vnet_latency = m_flit_network_latency / m_flits_received;

    m_avg_flit_vqueue_latency
        .name(name() + ".average_flit_vqueue_latency")
        .flags(Stats::oneline);
    m_avg_flit_vqueue_latency =
        m_flit_queueing_latency / m_flits_received;

    m_avg_flit_network_latency
        .name(name() + ".average_flit_network_latency");
    m_avg_flit_network_latency =
        sum(m_flit_network_latency) / sum(m_flits_received);

    m_avg_flit_queueing_latency
        .name(name() + ".average_flit_queueing_latency");
    m_avg_flit_queueing_latency =
        sum(m_flit_queueing_latency) / sum(m_flits_received);

    m_avg_flit_latency
        .name(name() + ".average_flit_latency");
    m_avg_flit_latency =
        m_avg_flit_network_latency + m_avg_flit_queueing_latency;


    // Hops
    m_avg_hops.name(name() + ".average_hops");
    m_avg_hops = m_total_hops / sum(m_flits_received);

    // Links
    m_total_ext_in_link_utilization
        .name(name() + ".ext_in_link_utilization");
    m_total_ext_out_link_utilization
        .name(name() + ".ext_out_link_utilization");
    m_total_int_link_utilization
        .name(name() + ".int_link_utilization");
    m_average_link_utilization
        .name(name() + ".avg_link_utilization");
    
    m_total_bypass_count 
        .name(name() + ".total_bypass_count");
    m_total_jitter_count
      .name(name() + ".total_jitter_count");
    m_total_num_attack_packets 
        .name(name() + ".total_num_attack_packet");
    m_total_normal_count
        .name(name() + ".total_normal_count");
    m_total_jitter_amount 
        .name(name() + ".total_jitter_amount");

    m_average_vc_load
        .init(m_virtual_networks * m_vcs_per_vnet)
        .name(name() + ".avg_vc_load")
        .flags(Stats::pdf | Stats::total | Stats::nozero | Stats::oneline)
        ;
}

    void
GarnetNetwork::collateStats()
{
    RubySystem *rs = params()->ruby_system;
    double time_delta = double(curCycle() - rs->getStartCycle());

    for (int i = 0; i < m_networklinks.size(); i++) {
        link_type type = m_networklinks[i]->getType();
        int activity = m_networklinks[i]->getLinkUtilization();

        if (type == EXT_IN_)
            m_total_ext_in_link_utilization += activity;
        else if (type == EXT_OUT_)
            m_total_ext_out_link_utilization += activity;
        else if (type == INT_)
            m_total_int_link_utilization += activity;

        m_average_link_utilization +=
            (double(activity) / time_delta);

        vector<unsigned int> vc_load = m_networklinks[i]->getVcLoad();
        for (int j = 0; j < vc_load.size(); j++) {
            m_average_vc_load[j] += ((double)vc_load[j] / time_delta);
        }
    }

    // Ask the routers to collate their statistics
    for (int i = 0; i < m_routers.size(); i++) {
        m_routers[i]->collateStats();
    }
}

void
GarnetNetwork::print(ostream& out) const
{
    out << "[GarnetNetwork]";
}

    GarnetNetwork *
GarnetNetworkParams::create()
{
    return new GarnetNetwork(this);
}

    uint32_t
GarnetNetwork::functionalWrite(Packet *pkt)
{
    uint32_t num_functional_writes = 0;

    for (unsigned int i = 0; i < m_routers.size(); i++) {
        num_functional_writes += m_routers[i]->functionalWrite(pkt);
    }

    for (unsigned int i = 0; i < m_nis.size(); ++i) {
        num_functional_writes += m_nis[i]->functionalWrite(pkt);
    }

    for (unsigned int i = 0; i < m_networklinks.size(); ++i) {
        num_functional_writes += m_networklinks[i]->functionalWrite(pkt);
    }

    return num_functional_writes;
}

bool
GarnetNetwork::checkCongestion(int src_router, int dest_router, uint64_t pid){
    return !checkFree(src_router, dest_router, pid);
}


bool
GarnetNetwork::checkFree(int src_router, int dest_router, uint64_t pid){
    if (m_routers[dest_router]->optimized_queue.empty()){
        return true;
    }
    else{
        flit * t_flit = m_routers[dest_router]->optimized_queue.front();
        if (t_flit->get_pid() == pid){
            return true;
        }
        return false;
    }
}

bool 
GarnetNetwork::insertFlitInOptimized(int r_id, flit * t_flit){
    m_routers[r_id]->optimized_queue.push_back(t_flit);
    return true;
}


bool
GarnetNetwork::insertFlitInOptimizedNI(int ni_id, flit * t_flit){
    m_nis[ni_id]->optimized_queue.push_back(t_flit);
    return true;
}


NetworkInterface *
GarnetNetwork::get_ni_from_id(int id){
    for (int i =0; i< m_nis.size(); i++){
        if (m_nis[i]->get_id() == id){
            return m_nis[i];
        }
    }
    DPRINTF(Naive, "Can not find Destination NI\n",
            id);
    assert(0);
    return NULL;
}



void
GarnetNetwork::createOutputUnitTable(int num_routers){
    DPRINTF(Vanilla, "creating %dx%d matrix of OutputUnit*\n", num_routers, num_routers);
    output_unit_table.resize(num_routers);
    for (int i =0; i< num_routers; i++){
        output_unit_table[i].resize(num_routers);
        for (int j =0; j < num_routers; j++){
            int src_id =  i;
            int dest_id = j;
            std::vector<OutputUnit*> ou_vector;

            if (i==j){
                ou_vector.clear();
                output_unit_table[i][j] = ou_vector; 

            } else{

                int cur_router_id = src_id;
                Router * cur_router = m_routers[cur_router_id];
                PortDirection cur_dirn = "Local";

                int cur_outport; 
                OutputUnit* cur_output_unit; 

                while(cur_router_id != dest_id){
                    //DPRINTF(Vanilla, "createOutputUnitTable "
                    //        "i %d j %d cur router %d\n", 
                    //        i,j,
                    //        cur_router_id);
                    cur_outport = cur_router->outport_compute_XY(
                            cur_router_id,
                            dest_id,
                            cur_dirn
                            );

                    cur_output_unit = cur_router->getOutputUnit(cur_outport);
                    cur_dirn = cur_router->getInputDirection(
                            cur_router->getOutportDirection(cur_outport));
                   
                    NetworkLink * cur_link = cur_output_unit->get_outlink();
                    ou_vector.push_back(cur_output_unit);    
                    cur_router_id  =  src_r_link_dest_r_map[
                        std::make_pair(
                                cur_router_id,
                                cur_link
                                )
                                ];
                    cur_router = m_routers[cur_router_id];
                   
                }
                output_unit_table[i][j] = ou_vector;
            }

            
        }   
    }

}

