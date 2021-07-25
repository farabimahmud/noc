/*
 * Copyright (c) 2020 Inria
 * Copyright (c) 2016 Georgia Institute of Technology
 * Copyright (c) 2008 Princeton University
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

#include "mem/ruby/network/garnet2.0/NetworkInterface.hh"

#include <cassert>
#include <cmath>

#include "base/cast.hh"
#include "debug/RubyNetwork.hh"
#include "debug/Naive.hh"
#include "debug/SK.hh"
#include "debug/Vanilla.hh"
#include "mem/ruby/network/MessageBuffer.hh"
#include "mem/ruby/network/garnet2.0/Credit.hh"
#include "mem/ruby/network/garnet2.0/flitBuffer.hh"
#include "mem/ruby/slicc_interface/Message.hh"
#include "mem/ruby/slicc_interface/RubyRequest.hh"
#include "mem/ruby/slicc_interface/RequestMsg.hh"
#include "mem/ruby/network/garnet2.0/Router.hh"

using namespace std;

NetworkInterface::NetworkInterface(const Params *p)
    : ClockedObject(p), Consumer(this), m_id(p->id),
      m_virtual_networks(p->virt_nets), m_vc_per_vnet(p->vcs_per_vnet),
      m_router_id(-1), m_vc_allocator(m_virtual_networks, 0),
      m_vc_round_robin(0), outFlitQueue(), outCreditQueue(),
      m_deadlock_threshold(p->garnet_deadlock_threshold),
      vc_busy_counter(m_virtual_networks, 0)
{
    const int num_vcs = m_vc_per_vnet * m_virtual_networks;
    niOutVcs.resize(num_vcs);
    m_ni_out_vcs_enqueue_time.resize(num_vcs);

    // instantiating the NI flit buffers
    for (auto &time : m_ni_out_vcs_enqueue_time)
    {
        time = Cycles(INFINITE_);
    }

    m_stall_count.resize(m_virtual_networks);
    flit_jitter_threshold = Cycles(p->flit_jitter_threshold);
    // DPRINTF(Naive, "flit jitter threshold %d\n", flit_jitter_threshold);

    jq = new flitBufferRTC();
    bq = new flitBufferRTC();
}

void NetworkInterface::init_net_ptr(GarnetNetwork *net_ptr)
{
    m_net_ptr = net_ptr;
    //    if (net_ptr->all_out_bypass){
    //        DPRINTF(Vanilla, "All Out Bypass Enabled "
    //                "using bypass queue at %#x\n",
    //                &bq);
    //    }
}

void NetworkInterface::init()
{
    const int num_vcs = m_vc_per_vnet * m_virtual_networks;
    outVcState.reserve(num_vcs);
    for (int i = 0; i < num_vcs; i++)
    {
        outVcState.emplace_back(i, m_net_ptr);
    }
}

void NetworkInterface::addInPort(NetworkLink *in_link,
                                 CreditLink *credit_link)
{
    inNetLink = in_link;
    in_link->setLinkConsumer(this);
    outCreditLink = credit_link;
    credit_link->setSourceQueue(&outCreditQueue);
}

void NetworkInterface::addOutPort(NetworkLink *out_link,
                                  CreditLink *credit_link,
                                  SwitchID router_id)
{
    inCreditLink = credit_link;
    credit_link->setLinkConsumer(this);

    outNetLink = out_link;
    out_link->setSourceQueue(&outFlitQueue);

    m_router_id = router_id;
}

void NetworkInterface::addNode(vector<MessageBuffer *> &in,
                               vector<MessageBuffer *> &out)
{
    inNode_ptr = in;
    outNode_ptr = out;

    for (auto &it : in)
    {
        if (it != nullptr)
        {
            it->setConsumer(this);
        }
    }
}

void NetworkInterface::dequeueCallback()
{
    // An output MessageBuffer has dequeued something this cycle and there
    // is now space to enqueue a stalled message. However, we cannot wake
    // on the same cycle as the dequeue. Schedule a wake at the soonest
    // possible time (next cycle).
    scheduleEventAbsolute(clockEdge(Cycles(1)));
}

void NetworkInterface::incrementStats(flit *t_flit)
{
    int vnet = t_flit->get_vnet();

    // Latency
    m_net_ptr->increment_received_flits(vnet);
    Cycles network_delay =
        t_flit->get_dequeue_time() - t_flit->get_enqueue_time() - Cycles(1);
    Cycles src_queueing_delay = t_flit->get_src_delay();
    Cycles dest_queueing_delay = (curCycle() - t_flit->get_dequeue_time());
    Cycles queueing_delay = src_queueing_delay + dest_queueing_delay;

    unsigned src_router_id = t_flit->get_route().src_router;
    unsigned dest_router_id = t_flit->get_route().dest_router;
    Router *src_router = m_net_ptr->m_routers[src_router_id];
    Router *dest_router = m_net_ptr->m_routers[dest_router_id];

    m_net_ptr->increment_flit_network_latency(network_delay, vnet);
    m_net_ptr->increment_flit_queueing_latency(queueing_delay, vnet);

    if (t_flit->get_type() == TAIL_ || t_flit->get_type() == HEAD_TAIL_)
    {
        m_net_ptr->increment_received_packets(vnet);
        m_net_ptr->increment_packet_network_latency(network_delay, vnet);
        m_net_ptr->increment_packet_queueing_latency(queueing_delay, vnet);
        m_net_ptr->sample_latency(network_delay);
        DPRINTF(Vanilla, "SR %d, DR %d, ND %d, QD %d, AT %d\n",
                src_router_id,
                dest_router_id,
                network_delay,
                queueing_delay,
                t_flit->isAttackFlit);

        // update expected delay table
        if (m_net_ptr->dynamic_delay)
        {
            src_router->setExpectedDelay(dest_router_id,
                                         queueing_delay + network_delay);
        }

        /****
         * ADD statistics gathering for attack node
         */
        Cycles lat = queueing_delay + network_delay;

        if (src_router_id == m_net_ptr->attack_node)
        {

            dest_router->sample_latency(lat);

            if (t_flit->isAttackFlit)
            {
                dest_router->sample_attack_latency(lat);
            }
            else
            {
                dest_router->sample_regular_latency(lat);
            }
        }
    }
    //   if(t_flit->getAttackFlit()){
    //
    //       DPRINTF(Vanilla, "Attack,%d,%d,%d,%d\n",
    //               curCycle(),
    //               src_router_id,
    //               dest_router_id,
    //               queueing_delay + network_delay
    //              );
    //   }

    // Hops
    m_net_ptr->increment_total_hops(t_flit->get_route().hops_traversed);
}

bool NetworkInterface::readJQ()
{
    bool read_from_JQ = false;
    Cycles currentCycle = curCycle();
    Tick curTime = clockEdge();
    DPRINTF(Vanilla, "contents of jq is %s\n", *jq);
    if (jq->isReady(currentCycle))
    {
        DPRINTF(Vanilla, "[NI:readJQ] entered JQ\n");
        flit *t_flit = jq->getTopFlit();
        flit_type t_flit_type = t_flit->get_type();
        assert(t_flit_type == HEAD_TAIL_ || t_flit_type == TAIL_);
        int vc = t_flit->get_vc();
        int vnet = t_flit->get_vnet();
        outVcState[vc].setState(IDLE_, currentCycle);
        outNode_ptr[vnet]->enqueue(t_flit->get_msg_ptr(), curTime,
                                   cyclesToTicks(Cycles(1)));
        sendCredit(t_flit, true);
        incrementStats(t_flit);

        DPRINTF(Vanilla, "[NI:readJQ] Reading from new JQ %s\n", *t_flit);

        delete t_flit;

        read_from_JQ = true;
        // if there are new flits, schedule NI to wake up
        if (!jq->isEmpty())
        {
            flit *next_flit = jq->peekTopFlit();

            Cycles ready_to_commit = next_flit->ready_to_commit;
            if (ready_to_commit - curCycle() > 0)
            {
                scheduleEvent(ready_to_commit - curCycle());
            }
            else
            {
                scheduleEvent(Cycles(1));
            }
        }
    }
    return read_from_JQ;
}

bool NetworkInterface::readBypassQueue()
{
    bool read_from_bq = false;
    Cycles currentCycle = curCycle();
    // Tick curTime = clockEdge();
    // DPRINTF(Vanilla, "[NI:readBypassQueue] Current BQ addr %#x content %s\n",
    //        bq, *bq);
    if (bq->isReady(currentCycle))
    {
        flit *t_flit = bq->getTopFlit();
        unsigned src_router_id = t_flit->get_route().src_router;
        unsigned dest_router_id = t_flit->get_route().dest_router;
        Router *dest_router = m_net_ptr->m_routers[dest_router_id];
        t_flit->set_dequeue_time(curCycle());
        DPRINTF(Vanilla, "[NI:readBypassQueue] Reading from BQ %s\n", *t_flit);
        Cycles network_delay =
            t_flit->get_dequeue_time() - t_flit->get_enqueue_time() - Cycles(1);
        Cycles src_queueing_delay = t_flit->get_src_delay();
        Cycles dest_queueing_delay = (curCycle() - t_flit->get_dequeue_time());
        Cycles queueing_delay = src_queueing_delay + dest_queueing_delay;

        Cycles lat = queueing_delay + network_delay;

        if (src_router_id == m_net_ptr->attack_node)
        {
            dest_router->sample_latency(lat);
            if (t_flit->isAttackFlit)
            {
                dest_router->sample_attack_latency(lat);
            }
            else
            {
                dest_router->sample_regular_latency(lat);
            }
        }

        if (t_flit->get_type() == HEAD_TAIL_ ||
            t_flit->get_type() == TAIL_)
        {
            std::vector<OutputUnit *> list_of_output_units =
                m_net_ptr->output_unit_table[src_router_id][dest_router_id];
            for (auto ou : list_of_output_units)
            {
                ou->bypass_flag = false;
            }
        }

        delete t_flit;

        read_from_bq = true;
        // if there are new flits, schedule NI to wake up
        if (!bq->isEmpty())
        {
            flit *next_flit = bq->peekTopFlit();
            DPRINTF(Vanilla, "[NI:readBypassQueue] next flit is %d"
                             " RTC %s \n",
                    next_flit->ready_to_commit,
                    *next_flit);
            Cycles ready_to_commit = next_flit->ready_to_commit;
            if (ready_to_commit > curCycle())
            {
                DPRINTF(Vanilla, "RTC %d cur %d\n",
                        ready_to_commit, curCycle());
                scheduleEvent(ready_to_commit - curCycle());
            }
            else
            {
                DPRINTF(Vanilla, "[NI:readBypassQueue] "
                                 "Scheduling Event on cycle %d\n",
                        curCycle() + 1);

                scheduleEvent(Cycles(1));
            }
        }
    }
    return read_from_bq;
}

/*
 * The NI wakeup checks whether there are any ready messages in the protocol
 * buffer. If yes, it picks that up, flitisizes it into a number of flits and
 * puts it into an output buffer and schedules the output link. On a wakeup
 * it also checks whether there are flits in the input link. If yes, it picks
 * them up and if the flit is a tail, the NI inserts the corresponding message
 * into the protocol buffer. It also checks for credits being sent by the
 * downstream router.
 */

void NetworkInterface::wakeup()
{
    DPRINTF(RubyNetwork, "Network Interface %d connected to router %d "
                         "woke up at time: %lld\n",
            m_id, m_router_id, curCycle());

    MsgPtr msg_ptr;
    Tick curTime = clockEdge();

    // Checking for messages coming from the protocol
    // can pick up a message/cycle for each virtual net
    for (int vnet = 0; vnet < inNode_ptr.size(); ++vnet)
    {
        MessageBuffer *b = inNode_ptr[vnet];
        if (b == nullptr)
        {
            continue;
        }
        if (b->isReady(curTime))
        { // Is there a message waiting
            msg_ptr = b->peekMsgPtr();
            if (flitisizeMessage(msg_ptr, vnet))
            {
                b->dequeue(curTime);
            }
        }
    }

    if (m_net_ptr->all_out_bypass)
    {
        bool bypass_queue_read_flag = readBypassQueue();
        if (bypass_queue_read_flag)
        {
            DPRINTF(Vanilla, "[NI:Wakeup] Bypass Queue read in this Cycle\n");
        }
    }

    if (m_net_ptr->all_out_bypass)
    {
        bool read_from_jq = readJQ();
        if (read_from_jq)
        {
            DPRINTF(Naive, "[NI:Wakeup] Read flit from JQ\n");
        }
    }

    scheduleOutputLink();
    checkReschedule();

    // Check if there are flits stalling a virtual channel. Track if a
    // message is enqueued to restrict ejection to one message per cycle.
    bool messageEnqueuedThisCycle = checkStallQueue();

    /*********** Check the incoming flit link **********/
    if (inNetLink->isReady(curCycle()))
    {
        flit *t_flit = inNetLink->consumeLink();
        int vnet = t_flit->get_vnet();
        t_flit->set_dequeue_time(curCycle());

        // If a tail flit is received, enqueue into the protocol buffers if
        // space is available.
        // Otherwise, exchange non-tail flits for credits.
        Cycles latency = curCycle() - t_flit->get_enqueue_time();
        if (t_flit->get_type() == TAIL_ ||
            t_flit->get_type() == HEAD_TAIL_)
        {
            // Cycles elapsed = curCycle() - t_flit->get_enqueue_time();
            // Cycles ready_to_commit = t_flit->ready_to_commit;
            if (m_net_ptr->all_out_bypass &&
                t_flit->getAttackFlit() &&
                t_flit->get_route().dest_ni == m_id &&
                t_flit->get_vnet() == 2 &&
                latency < t_flit->target_latency)
            { // vnet 2 for response
                Cycles remaining = t_flit->target_latency - latency;
                jq->insert(t_flit);
                scheduleEvent(remaining);
            } else
            {
                if (!messageEnqueuedThisCycle &&
                    outNode_ptr[vnet]->areNSlotsAvailable(1, curTime))
                {
                    // Space is available. Enqueue to protocol buffer.
                    DPRINTF(SK, "[3] enqueue msg: %s\n", *(t_flit->get_msg_ptr()));
                    outNode_ptr[vnet]->enqueue(t_flit->get_msg_ptr(), curTime,
                                            cyclesToTicks(Cycles(1)));
                    // Simply send a credit back since we are not buffering
                    // this flit in the NI
                    sendCredit(t_flit, true);
                    // Update stats and delete flit pointer
                    incrementStats(t_flit);
                    delete t_flit;
                }
                else
                {
                    // No space available-
                    // Place tail flit in stall queue and set
                    // up a callback for when protocol buffer is dequeued.
                    // Stat update and flit pointer deletion
                    // will occur upon unstall.
                    m_stall_queue.push_back(t_flit);
                    m_stall_count[vnet]++;

                    auto cb = std::bind(&NetworkInterface::dequeueCallback,
                                        this);
                    outNode_ptr[vnet]->registerDequeueCallback(cb);
                }
            }
        }
        else
        {
            DPRINTF(Naive, "flit %s is consumed at NI\n", *t_flit);
            // Non-tail flit. Send back a credit but not VC free signal.
            sendCredit(t_flit, false);

            // Update stats and delete flit pointer.
            incrementStats(t_flit);
            delete t_flit;
        }
    }

    /****************** Check the incoming credit link *******/

    if (inCreditLink->isReady(curCycle()))
    {
        Credit *t_credit = (Credit *)inCreditLink->consumeLink();
        outVcState[t_credit->get_vc()].increment_credit();
        if (t_credit->is_free_signal())
        {
            outVcState[t_credit->get_vc()].setState(IDLE_, curCycle());
        }
        delete t_credit;
    }

    // It is possible to enqueue multiple outgoing credit flits
    // if a message was unstalled in the same cycle
    // as a new message arrives.
    // In this case, we should schedule another wakeup
    // to ensure the credit is sent
    // back.
    if (outCreditQueue.getSize() > 0)
    {
        outCreditLink->scheduleEventAbsolute(clockEdge(Cycles(1)));
    }
}

void NetworkInterface::sendCredit(flit *t_flit, bool is_free)
{
    Credit *credit_flit = new Credit(t_flit->get_vc(), is_free, curCycle());
    outCreditQueue.insert(credit_flit);
}

bool NetworkInterface::checkStallQueue()
{
    bool messageEnqueuedThisCycle = false;
    Tick curTime = clockEdge();

    if (!m_stall_queue.empty())
    {
        for (auto stallIter = m_stall_queue.begin();
             stallIter != m_stall_queue.end();)
        {
            flit *stallFlit = *stallIter;
            int vnet = stallFlit->get_vnet();

            // If we can now eject to the protocol buffer, send back credits
            if (outNode_ptr[vnet]->areNSlotsAvailable(1, curTime))
            {
                DPRINTF(SK, "[4] enqueue msg: %s\n", *(stallFlit->get_msg_ptr()));
                outNode_ptr[vnet]->enqueue(stallFlit->get_msg_ptr(), curTime,
                                           cyclesToTicks(Cycles(1)));

                // Send back a credit with free signal now that the VC is no
                // longer stalled.
                sendCredit(stallFlit, true);

                // Update Stats
                incrementStats(stallFlit);

                // Flit can now safely be deleted and removed from stall queue
                delete stallFlit;
                m_stall_queue.erase(stallIter);
                m_stall_count[vnet]--;

                // If there are no more stalled messages for this vnet, the
                // callback on it's MessageBuffer is not needed.
                if (m_stall_count[vnet] == 0)
                    outNode_ptr[vnet]->unregisterDequeueCallback();

                messageEnqueuedThisCycle = true;
                break;
            }
            else
            {
                ++stallIter;
            }
        }
    }

    return messageEnqueuedThisCycle;
}

// Embed the protocol message into flits
bool NetworkInterface::flitisizeMessage(MsgPtr msg_ptr, int vnet)
{
    Message *net_msg_ptr = msg_ptr.get();
    NetDest net_msg_dest = net_msg_ptr->getDestination();

    bool isAttackMessage = net_msg_ptr->m_attackMessage;

    DPRINTF(Naive, "[NI:flitisize] msg %s at %#x is %s Packet\n",
            *net_msg_ptr, net_msg_ptr,
            isAttackMessage ? "Attack" : "Regular");
    DPRINTF(Naive, "isAttackMessage: %d, %#x\n", net_msg_ptr->m_attackMessage, &(net_msg_ptr->m_attackMessage));

    // gets all the destinations associated with this message.
    vector<NodeID> dest_nodes = net_msg_dest.getAllDest();

    // Number of flits is dependent on the link bandwidth available.
    // This is expressed in terms of bytes/cycle or the flit size
    int num_flits = (int)ceil((double)m_net_ptr->MessageSizeType_to_int(
                                  net_msg_ptr->getMessageSize()) /
                              m_net_ptr->getNiFlitSize());

    // loop to convert all multicast messages into unicast messages
    for (int ctr = 0; ctr < dest_nodes.size(); ctr++)
    {

        // this will return a free output virtual channel
        int vc = calculateVC(vnet);

        if (vc == -1)
        {
            return false;
        }
        MsgPtr new_msg_ptr = msg_ptr->clone();
        NodeID destID = dest_nodes[ctr];

        Message *new_net_msg_ptr = new_msg_ptr.get();
        if (dest_nodes.size() > 1)
        {
            NetDest personal_dest;
            for (int m = 0; m < (int)MachineType_NUM; m++)
            {
                if ((destID >= MachineType_base_number((MachineType)m)) &&
                    destID < MachineType_base_number((MachineType)(m + 1)))
                {
                    // calculating the NetDest associated with this destID
                    personal_dest.clear();
                    personal_dest.add((MachineID){(MachineType)m, (destID -
                                                                   MachineType_base_number((MachineType)m))});
                    new_net_msg_ptr->getDestination() = personal_dest;
                    break;
                }
            }
            net_msg_dest.removeNetDest(personal_dest);
            // removing the destination from the original message to reflect
            // that a message with this particular destination has been
            // flitisized and an output vc is acquired
            net_msg_ptr->getDestination().removeNetDest(personal_dest);
        }

        // Embed Route into the flits
        // NetDest format is used by the routing table
        // Custom routing algorithms just need destID
        RouteInfo route;
        route.vnet = vnet;
        route.net_dest = new_net_msg_ptr->getDestination();
        route.src_ni = m_id;
        route.src_router = m_router_id;
        route.dest_ni = destID;
        route.dest_router = m_net_ptr->get_router_id(destID);

        // initialize hops_traversed to -1
        // so that the first router increments it to 0
        route.hops_traversed = -1;

        // check if this packet is attack packet
        bool isAttackFarFlit = (route.dest_router ==
                                m_net_ptr->fixedTargetFarNode);
        bool isAttackNearFlit = (route.dest_router ==
                                 m_net_ptr->fixedTargetNearNode);
        DPRINTF(Naive, "[AttackFlit] Far %d Near %d\n", isAttackFarFlit,
                isAttackNearFlit);
        m_net_ptr->increment_injected_packets(vnet);
        int algorithm_decision = 0;
        for (int i = 0; i < num_flits; i++)
        {
            m_net_ptr->increment_injected_flits(vnet);
            flit *fl = new flit(i, vc, vnet, route, num_flits, new_msg_ptr,
                                curCycle());
            fl->set_pid(GarnetNetwork::PACKETID);
            fl->set_src_delay(curCycle() - ticksToCycles(msg_ptr->getTime()));
            fl->isAttackFlit = isAttackMessage;

            if (m_net_ptr->jitter_all)
            {
                fl->set_jittered(true);
            }

            else if (m_net_ptr->all_out_bypass)
            {
                if (fl->isAttackFlit)
                { // vnet 0 for request
                    algorithm_decision = sendAttackFlit(fl);
                }
                else
                {
                    niOutVcs[vc].insert(fl);
                    DPRINTF(Vanilla, "[NI:flitisize] "
                                     "Created flit %s from Router %d to Router %d\n",
                            *fl,
                            route.src_router,
                            route.dest_router);
                }
            }
            else
            {
                niOutVcs[vc].insert(fl);
                DPRINTF(Naive, "Created flit %s at NI\n",
                        *fl);
            }
        }
        // potential issue for sendAttackflit
        if (algorithm_decision == 0)
        {
            m_ni_out_vcs_enqueue_time[vc] = curCycle();
            outVcState[vc].setState(ACTIVE_, curCycle());
        }

        GarnetNetwork::PACKETID++;
        if (m_net_ptr->all_out_bypass && isAttackMessage)
        {
            m_net_ptr->target = std::max(
                m_net_ptr->lower_limit,
                m_net_ptr->target - m_net_ptr->delta_s);
        }
    }
    return true;
}

int NetworkInterface::sendAttackFlit(flit *fl)
{
    int vc = fl->get_vc();
    RouteInfo route = fl->get_route();
    NodeID destID = route.dest_ni;
    Cycles lower = m_net_ptr->lower_limit;
    Cycles upper = m_net_ptr->upper_limit;
    Cycles target = m_net_ptr->target;

    std::vector<OutputUnit *> ou_units_req =
        m_net_ptr->output_unit_table[m_router_id][route.dest_router];
    std::vector<OutputUnit *> ou_units_resp =
        m_net_ptr->output_unit_table[route.dest_router][m_router_id];

    Cycles latency_b = Cycles(ceil(
        (float)ou_units_req.size() / (float)m_net_ptr->max_hpc));
    Cycles latency_n = Cycles(ou_units_req.size());

    //Cycles latency_b_rt = Cycles(ceil(
    //            ((float) ou_units_req.size() + ou_units_resp.size())
    //            / ((float) m_net_ptr->max_hpc)));
    // Cycles latency_n_rt = Cycles(ou_units_req.size()+ ou_units_resp.size());

    // DPRINTF(Vanilla, "[NI:sendAttackflit] RTT bypass %d normal %d\n",
    //         latency_b_rt, latency_n_rt);

    /**
    // (1) latency_b <= latency_n 
    // (2) latency_b <= lower_limit
    // (3) lower_limit <= target <= upper_limit 
    //
    //
    // Case 1: latency_b < lower < upper, latency_n < lower < upper
    // Case 2: latency_b < lower < upper, lower < latency_n < upper
    // Case 3: lower < latency_b < upper, lower < latency_n < upper
    // Case 4: lower < latency_b < upper, lower < upper < latency_n
    // Case 5: lower < upper < latency_b, lower < upper < latency_n 

     **/
    bool case_1 = latency_n < lower;
    bool case_2 = latency_n >= lower && latency_n < target;
    bool case_3 = latency_n >= target;

    Cycles jitter_amount = Cycles(0);

    // Case 1: latency_n < lower
    // Case 2: latency_n > lower  && latency_n < upper
    // add jitter to make at least lower
    if (case_1 || case_2)
    {
        niOutVcs[vc].insert(fl);
        if (target > m_net_ptr->delta_s + latency_n)
        {
            jitter_amount = (target + Cycles(fl->get_id()) - m_net_ptr->delta_s - latency_n);
        }
        fl->target_latency = target - m_net_ptr->delta_s + Cycles(fl->get_id());
        fl->ready_to_commit = curCycle() + fl->target_latency;

        DPRINTF(Vanilla, "[NI:sendAttackflit] "
                         "Created flit %s from Router %d to Router %d "
                         "with Target %s Jitter %d cycles\n",
                *fl,
                route.src_router,
                route.dest_router,
                fl->target_latency,
                jitter_amount);
        return 0;
    }
    else if (case_3)
    {
        for (auto ou : ou_units_req)
        {
            ou->bypass_flag = true;
        }
        if (target > m_net_ptr->delta_s + latency_b)
        {
            jitter_amount = target - m_net_ptr->delta_s - latency_b;
        }
        fl->set_jitter_amount(jitter_amount);

        NetworkInterface *dest_ni = m_net_ptr->get_ni_from_id(destID);
        fl->ready_to_commit = curCycle() + latency_b + Cycles(fl->get_id());
        fl->target_latency = target - m_net_ptr->delta_s;
        dest_ni->bq->insert(fl);
        dest_ni->scheduleEvent(latency_b + Cycles(fl->get_id()));
        DPRINTF(Vanilla, "flit %s inserted into Bypass Queue "
                         "that will wake up dest_ni %d at cycle %d\n",
                *fl, destID, latency_b + Cycles(fl->get_id()));
        return 1;
    }
    else
    {
        DPRINTF(Vanilla, "latency_b %d latency_n %d"
                         " lower %d upper %d target %d\n",
                latency_b, latency_n, lower, upper, target);

        assert(false && "This should not happen\n");
        return 0;
    }
}

void NetworkInterface::setBypassFlags(flit *t_flit)
{
    // TODO
}

void NetworkInterface::resetBypassFlags(flit *t_flit)
{
    // TODO
}

// Looking for a free output vc
int NetworkInterface::calculateVC(int vnet)
{
    for (int i = 0; i < m_vc_per_vnet; i++)
    {
        int delta = m_vc_allocator[vnet];
        m_vc_allocator[vnet]++;
        if (m_vc_allocator[vnet] == m_vc_per_vnet)
            m_vc_allocator[vnet] = 0;

        if (outVcState[(vnet * m_vc_per_vnet) + delta].isInState(
                IDLE_, curCycle()))
        {
            vc_busy_counter[vnet] = 0;
            return ((vnet * m_vc_per_vnet) + delta);
        }
    }

    vc_busy_counter[vnet] += 1;
    panic_if(vc_busy_counter[vnet] > m_deadlock_threshold,
             "%s: Possible network deadlock in vnet: %d at time: %llu \n",
             name(), vnet, curTick());

    return -1;
}

/** This function looks at the NI buffers
 *  if some buffer has flits which are ready to traverse the link in the next
 *  cycle, and the downstream output vc associated with this flit has buffers
 *  left, the link is scheduled for the next cycle
 */

void NetworkInterface::scheduleOutputLink()
{
    int vc = m_vc_round_robin;

    for (int i = 0; i < niOutVcs.size(); i++)
    {
        vc++;
        if (vc == niOutVcs.size())
            vc = 0;

        // model buffer backpressure
        if (niOutVcs[vc].isReady(curCycle()) &&
            outVcState[vc].has_credit())
        {

            bool is_candidate_vc = true;
            int t_vnet = get_vnet(vc);
            int vc_base = t_vnet * m_vc_per_vnet;

            if (m_net_ptr->isVNetOrdered(t_vnet))
            {
                for (int vc_offset = 0; vc_offset < m_vc_per_vnet;
                     vc_offset++)
                {
                    int t_vc = vc_base + vc_offset;
                    if (niOutVcs[t_vc].isReady(curCycle()))
                    {
                        if (m_ni_out_vcs_enqueue_time[t_vc] <
                            m_ni_out_vcs_enqueue_time[vc])
                        {
                            is_candidate_vc = false;
                            break;
                        }
                    }
                }
            }
            if (!is_candidate_vc)
                continue;

            m_vc_round_robin = vc;

            outVcState[vc].decrement_credit();
            // Just removing the flit
            flit *t_flit = niOutVcs[vc].getTopFlit();
            t_flit->set_time(curCycle() + Cycles(1));
            outFlitQueue.insert(t_flit);
            // schedule the out link
            outNetLink->scheduleEventAbsolute(clockEdge(Cycles(1)));

            if (t_flit->get_type() == TAIL_ ||
                t_flit->get_type() == HEAD_TAIL_)
            {
                m_ni_out_vcs_enqueue_time[vc] = Cycles(INFINITE_);
            }
            return;
        }
        else if (niOutVcs[vc].isReady(curCycle()))
        {
            DPRINTF(Naive, "[NI] outVcState %d does not have credit\n", vc);
        }
    }
}

int NetworkInterface::get_vnet(int vc)
{
    for (int i = 0; i < m_virtual_networks; i++)
    {
        if (vc >= (i * m_vc_per_vnet) && vc < ((i + 1) * m_vc_per_vnet))
        {
            return i;
        }
    }
    fatal("Could not determine vc");
}

// Wakeup the NI in the next cycle if there are waiting
// messages in the protocol buffer, or waiting flits in the
// output VC buffer
void NetworkInterface::checkReschedule()
{
    for (const auto &it : inNode_ptr)
    {
        if (it == nullptr)
        {
            continue;
        }

        while (it->isReady(clockEdge()))
        { // Is there a message waiting
            DPRINTF(Vanilla, "[NI:CheckReschedule] call to wakeup next cycle\n");
            scheduleEvent(Cycles(1));
            return;
        }
    }

    for (auto &ni_out_vc : niOutVcs)
    {
        if (ni_out_vc.isReady(curCycle() + Cycles(1)))
        {
            // flit * tf = ni_out_vc.peekTopFlit();
            DPRINTF(Vanilla, "Current flits waiting in niOutVc %s\n", ni_out_vc);
            scheduleEvent(Cycles(1));
            return;
        }
    }
}

void NetworkInterface::print(std::ostream &out) const
{
    out << "[Network Interface]";
}

uint32_t
NetworkInterface::functionalWrite(Packet *pkt)
{
    uint32_t num_functional_writes = 0;
    for (auto &ni_out_vc : niOutVcs)
    {
        num_functional_writes += ni_out_vc.functionalWrite(pkt);
    }

    num_functional_writes += outFlitQueue.functionalWrite(pkt);
    return num_functional_writes;
}

NetworkInterface *
GarnetNetworkInterfaceParams::create()
{
    return new NetworkInterface(this);
}
