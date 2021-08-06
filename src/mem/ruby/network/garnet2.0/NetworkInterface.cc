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
#include "debug/JitterAllStats.hh"
#include "debug/Naive.hh"
#include "debug/RubyNetwork.hh"
#include "debug/SK.hh"
#include "debug/Vanilla.hh"
#include "debug/Vanilla_X86.hh"
#include "mem/ruby/network/MessageBuffer.hh"
#include "mem/ruby/network/garnet2.0/Credit.hh"
#include "mem/ruby/network/garnet2.0/Router.hh"
#include "mem/ruby/network/garnet2.0/flitBuffer.hh"
#include "mem/ruby/slicc_interface/Message.hh"
#include "mem/ruby/slicc_interface/RubyRequest.hh"

using namespace std;

NetworkInterface::NetworkInterface(const Params *p)
    : ClockedObject(p),
      Consumer(this),
      m_id(p->id),
      m_virtual_networks(p->virt_nets),
      m_vc_per_vnet(p->vcs_per_vnet),
      m_router_id(-1),
      m_vc_allocator(m_virtual_networks, 0),
      m_vc_round_robin(0),
      outFlitQueue(),
      outCreditQueue(),
      m_deadlock_threshold(p->garnet_deadlock_threshold),
      vc_busy_counter(m_virtual_networks, 0) {
  const int num_vcs = m_vc_per_vnet * m_virtual_networks;
  niOutVcs.resize(num_vcs);
  m_ni_out_vcs_enqueue_time.resize(num_vcs);

  // instantiating the NI flit buffers
  for (auto &time : m_ni_out_vcs_enqueue_time) {
    time = Cycles(INFINITE_);
  }

  m_stall_count.resize(m_virtual_networks);
  flit_jitter_threshold = Cycles(p->flit_jitter_threshold);
  // DPRINTF(Naive, "flit jitter threshold %d\n", flit_jitter_threshold);

  jq = new flitBufferRTC();
  bq = new flitBufferRTC();
}

void NetworkInterface::init_net_ptr(GarnetNetwork *net_ptr) {
  m_net_ptr = net_ptr;
  //    if (net_ptr->all_out_bypass){
  //        DPRINTF(Vanilla, "All Out Bypass Enabled "
  //                "using bypass queue at %#x\n",
  //                &bq);
  //    }
  //
}

void NetworkInterface::init() {
  const int num_vcs = m_vc_per_vnet * m_virtual_networks;
  outVcState.reserve(num_vcs);
  for (int i = 0; i < num_vcs; i++) {
    outVcState.emplace_back(i, m_net_ptr);
  }
}

void NetworkInterface::addInPort(NetworkLink *in_link,
                                 CreditLink *credit_link) {
  inNetLink = in_link;
  in_link->setLinkConsumer(this);
  outCreditLink = credit_link;
  credit_link->setSourceQueue(&outCreditQueue);
}

void NetworkInterface::addOutPort(NetworkLink *out_link,
                                  CreditLink *credit_link, SwitchID router_id) {
  inCreditLink = credit_link;
  credit_link->setLinkConsumer(this);

  outNetLink = out_link;
  out_link->setSourceQueue(&outFlitQueue);

  m_router_id = router_id;
}

void NetworkInterface::addNode(vector<MessageBuffer *> &in,
                               vector<MessageBuffer *> &out) {
  inNode_ptr = in;
  outNode_ptr = out;

  for (auto &it : in) {
    if (it != nullptr) {
      it->setConsumer(this);
    }
  }
}

void NetworkInterface::dequeueCallback() {
  // An output MessageBuffer has dequeued something this cycle and there
  // is now space to enqueue a stalled message. However, we cannot wake
  // on the same cycle as the dequeue. Schedule a wake at the soonest
  // possible time (next cycle).
  scheduleEventAbsolute(clockEdge(Cycles(1)));
}

void NetworkInterface::incrementStatsForBypassFlit(flit *t_flit) {
  int vnet = t_flit->get_vnet();
  m_net_ptr->increment_received_flits(vnet);
  DPRINTF(Vanilla_X86, "Deq %d Enq %d\n", t_flit->get_dequeue_time(),
          t_flit->get_enqueue_time());
  Cycles network_delay =
      t_flit->get_dequeue_time() - t_flit->get_enqueue_time();
  Cycles src_queueing_delay = t_flit->get_src_delay();
  Cycles dest_queueing_delay = (curCycle() - t_flit->get_dequeue_time());
  DPRINTF(Vanilla_X86, "Src QD %d Dest QD %d\n", src_queueing_delay,
          dest_queueing_delay);
  Cycles queueing_delay = src_queueing_delay + dest_queueing_delay;

  unsigned src_router_id = t_flit->get_route().src_router;
  unsigned dest_router_id = t_flit->get_route().dest_router;

  m_net_ptr->increment_flit_network_latency(network_delay, vnet);
  m_net_ptr->increment_flit_queueing_latency(queueing_delay, vnet);

  if (t_flit->get_type() == TAIL_ || t_flit->get_type() == HEAD_TAIL_) {
    m_net_ptr->increment_received_packets(vnet);
    m_net_ptr->increment_packet_network_latency(network_delay, vnet);
    m_net_ptr->increment_packet_queueing_latency(queueing_delay, vnet);
    if (t_flit->isAttackFlit && vnet == 1) {
      Cycles round_trip_latency = t_flit->get_dequeue_time() -
                                  t_flit->get_msg_ptr()->getReqEnqueueTime() -
                                  Cycles(1);
      bool response_from_closest = src_router_id == m_net_ptr->closest_dest;
      if (response_from_closest) {
        m_net_ptr->closest_rt =
            (Cycles)(((int)m_net_ptr->closest_rt + (int)round_trip_latency) /
                     2);
        m_net_ptr->closest_rt =
            std::min(m_net_ptr->upper_limit, m_net_ptr->closest_rt);
        DPRINTF(Vanilla,
                "[NI:IncrementStats] Closest RTT is now set to be %d\n",
                m_net_ptr->closest_rt);
      }
      m_net_ptr->sample_latency(round_trip_latency, response_from_closest,
                                t_flit->isAttackFlit);
      DPRINTF(Vanilla,
              "SR,%d,DR,%d,"
              "Target Latency,%d,"
              "Enq Time,%d,Deq Time, %d,Round Trip Latency,%d,"
              "vnet,%d,msg_ptr,%#x\n",
              src_router_id, dest_router_id, t_flit->target_latency,
              t_flit->get_msg_ptr()->getReqEnqueueTime(),
              t_flit->get_dequeue_time(), round_trip_latency, vnet,
              t_flit->get_msg_ptr());
    }
  }

  DPRINTF(Vanilla_X86, "Increment Stats 5\n");
  // Hops
  m_net_ptr->increment_total_hops(t_flit->get_route().hops_traversed);
}

void NetworkInterface::incrementStats(flit *t_flit) {
  int vnet = t_flit->get_vnet();

  // Latency
  m_net_ptr->increment_received_flits(vnet);
  DPRINTF(Vanilla_X86, "Increment Stats 0\n");
  Cycles network_delay =
      t_flit->get_dequeue_time() - t_flit->get_enqueue_time() - Cycles(1);
  DPRINTF(Vanilla_X86, "Increment Stats 1\n");
  Cycles src_queueing_delay = t_flit->get_src_delay();
  Cycles dest_queueing_delay = (curCycle() - t_flit->get_dequeue_time());
  DPRINTF(Vanilla_X86, "Increment Stats 2\n");

  // Cycles RTT_latency = t_flit->get_dequeue_time() -
  // t_flit->request_flit->enqueue_time();

  Cycles queueing_delay = src_queueing_delay + dest_queueing_delay;

  unsigned src_router_id = t_flit->get_route().src_router;
  unsigned dest_router_id = t_flit->get_route().dest_router;

  m_net_ptr->increment_flit_network_latency(network_delay, vnet);
  m_net_ptr->increment_flit_queueing_latency(queueing_delay, vnet);

  if (t_flit->get_type() == TAIL_ || t_flit->get_type() == HEAD_TAIL_) {
    m_net_ptr->increment_received_packets(vnet);
    m_net_ptr->increment_packet_network_latency(network_delay, vnet);
    m_net_ptr->increment_packet_queueing_latency(queueing_delay, vnet);
    DPRINTF(Vanilla_X86, "Increment Stats 3\n");
    Cycles actual_latency = network_delay + queueing_delay - Cycles(1);
    DPRINTF(JitterAllStats, "%d,%d,%d,%d,%d,%d,%d\n", src_router_id,
            dest_router_id, t_flit->target_latency, network_delay,
            queueing_delay, actual_latency, t_flit->isAttackFlit);

    if (t_flit->isAttackFlit && vnet == 1) {
      Cycles round_trip_latency = t_flit->get_dequeue_time() -
                                  t_flit->get_msg_ptr()->getReqEnqueueTime() -
                                  Cycles(1);
      bool response_from_closest = src_router_id == m_net_ptr->closest_dest;
      if (response_from_closest) {
        m_net_ptr->closest_rt =
            (Cycles)(((int)m_net_ptr->closest_rt + (int)round_trip_latency) /
                     2);
        m_net_ptr->closest_rt =
            std::min(m_net_ptr->upper_limit, m_net_ptr->closest_rt);
        DPRINTF(Vanilla, "[NI:IncrementStats] Closest RTT %d\n",
                m_net_ptr->closest_rt);
      }
      m_net_ptr->sample_latency(round_trip_latency, response_from_closest,
                                t_flit->isAttackFlit);
      DPRINTF(Vanilla,
              "[NI:IncrementStats] Src Router %d"
              "Closest Dest %d\n",
              src_router_id, m_net_ptr->closest_dest);

      DPRINTF(Vanilla,
              "SR,%d,DR,%d,"
              "Target Latency,%d,"
              "Enq Time,%d,Deq Time, %d,Round Trip Latency,%d,"
              "vnet,%d,msg_ptr,%#x\n",
              src_router_id, dest_router_id, t_flit->target_latency,
              t_flit->get_msg_ptr()->getReqEnqueueTime(),
              t_flit->get_dequeue_time(), round_trip_latency, vnet,
              t_flit->get_msg_ptr());
    }
  }
  /****
   * ADD statistics gathering for attack node
   */

  DPRINTF(Vanilla_X86, "Increment Stats 5\n");
  // Hops
  m_net_ptr->increment_total_hops(t_flit->get_route().hops_traversed);
}

bool NetworkInterface::readJQ() {
  Cycles currentCycle = curCycle();
  Tick curTime = clockEdge();
  if (jq->isReady(currentCycle)) {
    flit *t_flit = jq->getTopFlit();
    flit_type t_flit_type = t_flit->get_type();
    assert(t_flit_type == HEAD_TAIL_ || t_flit_type == TAIL_);
    Cycles latency = curCycle() - t_flit->get_msg_ptr()->getReqEnqueueTime();
    assert(latency >= m_net_ptr->upper_limit);

    int vc = t_flit->get_vc();
    int vnet = t_flit->get_vnet();

    if (outNode_ptr[vnet]->areNSlotsAvailable(1, curTime)) {
      outVcState[vc].setState(IDLE_, currentCycle);
      outNode_ptr[vnet]->enqueue(t_flit->get_msg_ptr(), curTime,
                                 cyclesToTicks(Cycles(1)));
      sendCredit(t_flit, true);
      t_flit->set_dequeue_time(currentCycle);
      incrementStats(t_flit);    
      delete t_flit;
      return true;
    } else {
      jq->insert(t_flit);
      auto cb = std::bind(&NetworkInterface::dequeueCallback, this);
      outNode_ptr[vnet]->registerDequeueCallback(cb);
      return false; 
    }
  }
  return false; 
}

bool NetworkInterface::readBypassQueue() {
  bool read_from_bq = false;
  Cycles currentCycle = curCycle();
  Tick curTime = clockEdge();

  // Tick curTime = clockEdge();
  DPRINTF(Vanilla, "[NI:readBypassQueue] Current BQ addr %#x content %s\n", bq,
          *bq);

  if (bq->isReady(currentCycle)) {
    flit *t_flit = bq->peekTopFlit();
    int vnet = t_flit->get_vnet();
    // if non tail consume flit no need to stall since we are not sending
    // out to the outNode_ptr[vnet]
    DPRINTF(Vanilla_X86, "Reading flit %s at BQ \n", *t_flit);
    flit_type t_flit_type = t_flit->get_type();
    bool tail_flit = (t_flit_type == HEAD_TAIL_) || (t_flit_type == TAIL_);
    if (!tail_flit) {
      t_flit = bq->getTopFlit();
      t_flit->set_dequeue_time(currentCycle);
      incrementStatsForBypassFlit(t_flit);
      delete t_flit;
    } else {
      unsigned src_router_id = t_flit->get_route().src_router;
      unsigned dest_router_id = t_flit->get_route().dest_router;
      // tail flit reached destination, so free OutputUnit
      if (!t_flit->isBypassFlagsReset) {
        std::vector<OutputUnit *> list_of_output_units =
            m_net_ptr->output_unit_table[src_router_id][dest_router_id];
        for (auto ou : list_of_output_units) {
          ou->bypass_flag = false;
        }
        t_flit->isBypassFlagsReset = true;
      }

      // check if we can consume this i.e send via outNode_ptr
      DPRINTF(Vanilla_X86, "Reset the bypass flags for %s\n", *t_flit);
      Cycles rt_latency = curCycle() -
                          t_flit->get_msg_ptr()->getReqEnqueueTime() +
                          Cycles(2);  // queueing delay
      DPRINTF(Vanilla, "SRC %d DEST %d Target_Latency %d RTT %d VNET %d\n",
              src_router_id, dest_router_id, t_flit->target_latency, rt_latency,
              vnet);
      // Do not jitter to request packets
      // compare with RTT instead of one way metrics
      // use target value
      if (rt_latency < t_flit->target_latency && t_flit->get_vnet() == 1) {
        Cycles jitter_amount = t_flit->target_latency - rt_latency;
        t_flit = bq->getTopFlit();
        t_flit->ready_to_commit = currentCycle + jitter_amount;
        DPRINTF(Vanilla,
                "[NI:readBypassQueue]Added %d Jitter to Bypassed Flit %s\n",
                t_flit->target_latency - rt_latency, *t_flit);
        bq->insert(t_flit);
        m_net_ptr->inc_total_jitter_count();
        m_net_ptr->inc_total_jitter_amount(jitter_amount);
        scheduleEvent(jitter_amount);
        return false;
      } else {
        if (outNode_ptr[vnet]->areNSlotsAvailable(1, curTime)) {
          DPRINTF(Vanilla_X86, "Will send out this flit to outnode %s\n",
                  *t_flit);
          // outVcState[vc].setState(IDLE_, currentCycle);
          outNode_ptr[vnet]->enqueue(t_flit->get_msg_ptr(), curTime,
                                     cyclesToTicks(Cycles(1)));
          // sendCredit(t_flit, true);
          t_flit = bq->getTopFlit();
          DPRINTF(Vanilla_X86, "Updating Stats for Flit %s at BQ\n", *t_flit);
          t_flit->set_dequeue_time(currentCycle);
          incrementStatsForBypassFlit(t_flit);
          DPRINTF(Vanilla_X86, "Updated Stats for Flit %s at BQ\n", *t_flit);
          delete t_flit;

          read_from_bq = true;
        }
      }
    }
    // if there are new flits, schedule NI to wake up
    if (!bq->isEmpty()) {
      flit *next_flit = bq->peekTopFlit();
      DPRINTF(Vanilla,
              "[NI:readBypassQueue] next flit is %d"
              " RTC %s \n",
              next_flit->ready_to_commit, *next_flit);
      Cycles ready_to_commit = next_flit->ready_to_commit;
      if (ready_to_commit > curCycle()) {
        DPRINTF(Vanilla, "RTC %d cur %d\n", ready_to_commit, curCycle());
        scheduleEvent(ready_to_commit - curCycle());
      } else {
        DPRINTF(Vanilla,
                "[NI:readBypassQueue] "
                "Scheduling Event on next cycle %d\n",
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

void NetworkInterface::wakeup() {
  DPRINTF(RubyNetwork,
          "Network Interface %d connected to router %d "
          "woke up at time: %lld\n",
          m_id, m_router_id, curCycle());

  MsgPtr msg_ptr;
  Tick curTime = clockEdge();

  // Checking for messages coming from the protocol
  // can pick up a message/cycle for each virtual net
  for (int vnet = 0; vnet < inNode_ptr.size(); ++vnet) {
    MessageBuffer *b = inNode_ptr[vnet];
    DPRINTF(Vanilla, "[NI:wakeup] inNode_ptr %#x\n", b);
    if (b == nullptr) {
      continue;
    }
    if (b->isReady(curTime)) {  // Is there a message waiting
      msg_ptr = b->peekMsgPtr();
      if (flitisizeMessage(msg_ptr, vnet)) {
        b->dequeue(curTime);
      }
    }
  }

  if (m_net_ptr->all_out_bypass) {
    bool bypass_queue_read_flag = readBypassQueue();
    if (bypass_queue_read_flag) {
      DPRINTF(Vanilla, "[NI:Wakeup] Bypass Queue read in this Cycle\n");
    }
  }

  if (m_net_ptr->all_out_bypass || m_net_ptr->jitter_all) {
    bool read_from_jq = readJQ();
    if (read_from_jq) {
      DPRINTF(Naive, "[NI:Wakeup] Read flit from JQ\n");
    }
  }

  scheduleOutputLink();
  checkReschedule();

  // Check if there are flits stalling a virtual channel. Track if a
  // message is enqueued to restrict ejection to one message per cycle.
  bool messageEnqueuedThisCycle = checkStallQueue();

  /*********** Check the incoming flit link **********/
  if (inNetLink->isReady(curCycle())) {
    flit *t_flit = inNetLink->consumeLink();
    int vnet = t_flit->get_vnet();
    t_flit->set_dequeue_time(curCycle());

    // If a tail flit is received, enqueue into the protocol buffers if
    // space is available.
    // Otherwise, exchange non-tail flits for credits.

    if (t_flit->get_type() == TAIL_ || t_flit->get_type() == HEAD_TAIL_) {
      Cycles latency = curCycle() - t_flit->get_msg_ptr()->getReqEnqueueTime();
      if (m_net_ptr->jitter_all && t_flit->getAttackFlit() &&
          latency < m_net_ptr->upper_limit && t_flit->get_vnet() == 1) {
        DPRINTF(Vanilla, "Inserted %s into Jitter Queue\n", *t_flit);
        t_flit->ready_to_commit = m_net_ptr->upper_limit - latency + curCycle(); 
        jq->insert(t_flit);
        m_net_ptr->inc_total_jitter_count();
        m_net_ptr->inc_total_jitter_amount(m_net_ptr->upper_limit - latency);
        scheduleEvent(m_net_ptr->upper_limit - latency);
      } else if (m_net_ptr->all_out_bypass && t_flit->getAttackFlit() &&
                 t_flit->get_vnet() == 1 &&
                 latency < t_flit->target_latency) {  // vnet 2 for response
        DPRINTF(Vanilla, "[NI:Wakeup] flit %s latency %d target %d\n", *t_flit,
                latency, t_flit->target_latency);
        jq->insert(t_flit);
        m_net_ptr->inc_total_jitter_count();
        m_net_ptr->inc_total_jitter_amount(m_net_ptr->upper_limit - latency);

        scheduleEvent(t_flit->target_latency - latency);
        DPRINTF(Vanilla, "%s will wait %d cycles\n", *t_flit,
                t_flit->target_latency - latency);
      } else {
        if (!messageEnqueuedThisCycle &&
            outNode_ptr[vnet]->areNSlotsAvailable(1, curTime)) {
          // Space is available. Enqueue to protocol buffer.
          DPRINTF(Vanilla, "OutNode_ptr %#x enqueue msg: %s\n",
                  outNode_ptr[vnet], *(t_flit->get_msg_ptr()));
          outNode_ptr[vnet]->enqueue(t_flit->get_msg_ptr(), curTime,
                                     cyclesToTicks(Cycles(1)));

          // Simply send a credit back since we are not buffering
          // this flit in the NI
          sendCredit(t_flit, true);
          // Update stats and delete flit pointer
          DPRINTF(Vanilla, "[NI:wakeup] enqueMessage %s\n", *t_flit);

          incrementStats(t_flit);
          delete t_flit;
        } else {
          // No space available-
          // Place tail flit in stall queue and set
          // up a callback for when protocol buffer is dequeued.
          // Stat update and flit pointer deletion
          // will occur upon unstall.
          m_stall_queue.push_back(t_flit);
          m_stall_count[vnet]++;

          auto cb = std::bind(&NetworkInterface::dequeueCallback, this);
          outNode_ptr[vnet]->registerDequeueCallback(cb);
        }
      }
    } else {
      DPRINTF(Naive, "flit %s is consumed at NI\n", *t_flit);
      // Non-tail flit. Send back a credit but not VC free signal.
      sendCredit(t_flit, false);

      // Update stats and delete flit pointer.
      DPRINTF(Vanilla, "[NI:wakeup] Consume NonTail Flit %s\n", *t_flit);

      incrementStats(t_flit);
      delete t_flit;
    }
  }

  /****************** Check the incoming credit link *******/

  if (inCreditLink->isReady(curCycle())) {
    Credit *t_credit = (Credit *)inCreditLink->consumeLink();
    outVcState[t_credit->get_vc()].increment_credit();
    if (t_credit->is_free_signal()) {
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
  if (outCreditQueue.getSize() > 0) {
    outCreditLink->scheduleEventAbsolute(clockEdge(Cycles(1)));
  }
}

void NetworkInterface::sendCredit(flit *t_flit, bool is_free) {
  Credit *credit_flit = new Credit(t_flit->get_vc(), is_free, curCycle());
  outCreditQueue.insert(credit_flit);
}

bool NetworkInterface::checkStallQueue() {
  bool messageEnqueuedThisCycle = false;
  Tick curTime = clockEdge();

  if (!m_stall_queue.empty()) {
    for (auto stallIter = m_stall_queue.begin();
         stallIter != m_stall_queue.end();) {
      flit *stallFlit = *stallIter;
      int vnet = stallFlit->get_vnet();

      // If we can now eject to the protocol buffer, send back credits
      if (outNode_ptr[vnet]->areNSlotsAvailable(1, curTime)) {
        DPRINTF(SK, "[4] enqueue msg: %s\n", *(stallFlit->get_msg_ptr()));
        outNode_ptr[vnet]->enqueue(stallFlit->get_msg_ptr(), curTime,
                                   cyclesToTicks(Cycles(1)));

        // Send back a credit with free signal now that the VC is no
        // longer stalled.
        sendCredit(stallFlit, true);

        // Update Stats
        DPRINTF(Vanilla, "[NI:checkStallQueue] eject to protocol %s\n",
                *stallFlit);
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
      } else {
        ++stallIter;
      }
    }
  }

  return messageEnqueuedThisCycle;
}

// Embed the protocol message into flits
bool NetworkInterface::flitisizeMessage(MsgPtr msg_ptr, int vnet) {
  Message *net_msg_ptr = msg_ptr.get();
  NetDest net_msg_dest = net_msg_ptr->getDestination();

  bool isAttackMessage = net_msg_ptr->isAttackMessage();
  MessageSizeType messageSize = net_msg_ptr->getMessageSize();
  if (messageSize == MessageSizeType_Response_Data)
    DPRINTF(Vanilla_X86, "[NI:flitisize] msg %s at %#x is %s Packet\n",
            *net_msg_ptr, net_msg_ptr, isAttackMessage ? "Attack" : "Regular");

  // gets all the destinations associated with this message.
  vector<NodeID> dest_nodes = net_msg_dest.getAllDest();

  // Number of flits is dependent on the link bandwidth available.
  // This is expressed in terms of bytes/cycle or the flit size
  int num_flits = (int)ceil(
      (double)m_net_ptr->MessageSizeType_to_int(net_msg_ptr->getMessageSize()) /
      m_net_ptr->getNiFlitSize());

  // loop to convert all multicast messages into unicast messages
  for (int ctr = 0; ctr < dest_nodes.size(); ctr++) {
    // this will return a free output virtual channel
    int vc = calculateVC(vnet);

    if (vc == -1) {
      return false;
    }
    MsgPtr new_msg_ptr = msg_ptr->clone();
    NodeID destID = dest_nodes[ctr];

    Message *new_net_msg_ptr = new_msg_ptr.get();
    if (dest_nodes.size() > 1) {
      NetDest personal_dest;
      for (int m = 0; m < (int)MachineType_NUM; m++) {
        if ((destID >= MachineType_base_number((MachineType)m)) &&
            destID < MachineType_base_number((MachineType)(m + 1))) {
          // calculating the NetDest associated with this destID
          personal_dest.clear();
          personal_dest.add(
              (MachineID){(MachineType)m,
                          (destID - MachineType_base_number((MachineType)m))});
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

    m_net_ptr->increment_injected_packets(vnet);
    int use_bypass = 0;
    for (int i = 0; i < num_flits; i++) {
      m_net_ptr->increment_injected_flits(vnet);
      flit *fl =
          new flit(i, vc, vnet, route, num_flits, new_msg_ptr, curCycle());
      fl->set_pid(GarnetNetwork::PACKETID);
      fl->set_src_delay(curCycle() - ticksToCycles(msg_ptr->getTime()));
      fl->isAttackFlit = isAttackMessage;

      if (m_net_ptr->jitter_all) {
        niOutVcs[vc].insert(fl);
        if(fl->isAttackFlit && vnet == 1){
            fl->target_latency = m_net_ptr->upper_limit;            
        }
        DPRINTF(Naive, "Created flit %s at NI\n", *fl);

      } else if (m_net_ptr->all_out_bypass) {
        if (fl->isAttackFlit) {  // vnet 0 for request
          use_bypass = sendAttackFlit(fl);
          DPRINTF(Vanilla, "Decision was %s\n",
                  use_bypass == 1 ? "bypass" : "no_bypass");
        } else {
          niOutVcs[vc].insert(fl);
          DPRINTF(Vanilla,
                  "[NI:flitisize] "
                  "Created flit %s from Router %d to Router %d\n",
                  *fl, route.src_router, route.dest_router);
        }
      } else {
        niOutVcs[vc].insert(fl);
        DPRINTF(Naive, "Created flit %s at NI\n", *fl);
      }
    }
    // potential issue for sendAttackflit
    if (use_bypass == 0) {
      m_net_ptr->inc_total_normal_count();
      m_ni_out_vcs_enqueue_time[vc] = curCycle();
      outVcState[vc].setState(ACTIVE_, curCycle());
    } else {
      m_net_ptr->inc_total_bypass_count();
    }
    if (isAttackMessage) m_net_ptr->inc_attack_packet_count();
    GarnetNetwork::PACKETID++;
  }
  return true;
}

int NetworkInterface::sendAttackFlit(flit *fl) {
  int vc = fl->get_vc();
  RouteInfo route = fl->get_route();
  NodeID destID = route.dest_ni;
  Cycles lower = m_net_ptr->lower_limit;
  Cycles upper = m_net_ptr->upper_limit;

  Cycles target = m_net_ptr->target_latency > m_net_ptr->closest_rt
                      ? m_net_ptr->target_latency
                      : m_net_ptr->closest_rt;

  std::vector<OutputUnit *> ou_units_req =
      m_net_ptr->output_unit_table[m_router_id][route.dest_router];
  std::vector<OutputUnit *> ou_units_resp =
      m_net_ptr->output_unit_table[route.dest_router][m_router_id];

  Cycles latency_b =
      Cycles(ceil((float)ou_units_req.size() / (float)m_net_ptr->max_hpc));
  Cycles latency_n = Cycles(ou_units_req.size());

  Cycles latency_b_rt =
      Cycles(ceil(((float)ou_units_req.size() + ou_units_resp.size()) /
                  ((float)m_net_ptr->max_hpc)));
  Cycles latency_n_rt = Cycles(ou_units_req.size() + ou_units_resp.size());

  DPRINTF(Vanilla, "[NI:sendAttackflit] RTT bypass %d normal %d flit %s\n",
          latency_b_rt, latency_n_rt, *fl);

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
  bool case_1 = latency_n <= lower;
  bool case_2 = latency_n > lower;

  Cycles jitter_amount = Cycles(0);
  DPRINTF(Vanilla, "[SendAttackFlit] ln %d lb %d target %d\n", latency_n,
          latency_b, target);
  DPRINTF(Vanilla, "[SendAttackFlit] Case 1 %d Case 2 %d\n", case_1, case_2);

  fl->lb = latency_b;
  fl->ln = latency_n;
  fl->tc = target;

  // Case 1: latency_n < lower
  // add jitter to make at least lower
  if (case_1) {
    niOutVcs[vc].insert(fl);
    fl->target_latency = lower + Cycles(fl->get_id());
    fl->ready_to_commit = curCycle() + fl->target_latency;

    DPRINTF(Vanilla,
            "[NI:sendAttackflit] "
            "Created flit %s from Router %d to Router %d "
            "with Target %s Jitter %d cycles\n",
            *fl, route.src_router, route.dest_router, fl->target_latency,
            jitter_amount);
    return 0;
  } else if (case_2) {
    for (auto ou : ou_units_req) {
      ou->bypass_flag = true;
    }
    NetworkInterface *dest_ni = m_net_ptr->get_ni_from_id(destID);

    fl->target_latency = m_net_ptr->closest_rt + Cycles(fl->get_id());
    fl->ready_to_commit = curCycle() + latency_b + Cycles(fl->get_id());

    dest_ni->bq->insert(fl);
    DPRINTF(Vanilla,
            "[NI:sendAttackFlit] sending flit %s through bypass"
            "Closest RT %d\n",
            *fl, m_net_ptr->closest_rt);
    dest_ni->scheduleEvent(latency_b + Cycles(fl->get_id()));
    DPRINTF(Vanilla,
            "at Tick %d flit %s inserted into Bypass Queue "
            "that will wake up dest_ni %d at cycle %d\n",
            curTick(), *fl, destID, cyclesToTicks(fl->ready_to_commit));

    return 1;
  } else {
    DPRINTF(Vanilla,
            "latency_b %d latency_n %d"
            " lower %d upper %d target %d\n",
            latency_b, latency_n, lower, upper, target);

    assert(false && "This should not happen\n");
    return 0;
  }
}

void NetworkInterface::setBypassFlags(flit *t_flit) {
  // TODO
}

void NetworkInterface::resetBypassFlags(flit *t_flit) {
  // TODO
}

// Looking for a free output vc
int NetworkInterface::calculateVC(int vnet) {
  for (int i = 0; i < m_vc_per_vnet; i++) {
    int delta = m_vc_allocator[vnet];
    m_vc_allocator[vnet]++;
    if (m_vc_allocator[vnet] == m_vc_per_vnet) m_vc_allocator[vnet] = 0;

    if (outVcState[(vnet * m_vc_per_vnet) + delta].isInState(IDLE_,
                                                             curCycle())) {
      vc_busy_counter[vnet] = 0;
      return ((vnet * m_vc_per_vnet) + delta);
    }
  }

  vc_busy_counter[vnet] += 1;
  warn_if(vc_busy_counter[vnet] > m_deadlock_threshold,
          "%s: Possible network deadlock in vnet: %d at time: %llu \n", name(),
          vnet, curTick());

  return -1;
}

/** This function looks at the NI buffers
 *  if some buffer has flits which are ready to traverse the link in the next
 *  cycle, and the downstream output vc associated with this flit has buffers
 *  left, the link is scheduled for the next cycle
 */

void NetworkInterface::scheduleOutputLink() {
  int vc = m_vc_round_robin;

  for (int i = 0; i < niOutVcs.size(); i++) {
    vc++;
    if (vc == niOutVcs.size()) vc = 0;

    // model buffer backpressure
    if (niOutVcs[vc].isReady(curCycle()) && outVcState[vc].has_credit()) {
      bool is_candidate_vc = true;
      int t_vnet = get_vnet(vc);
      int vc_base = t_vnet * m_vc_per_vnet;

      if (m_net_ptr->isVNetOrdered(t_vnet)) {
        for (int vc_offset = 0; vc_offset < m_vc_per_vnet; vc_offset++) {
          int t_vc = vc_base + vc_offset;
          if (niOutVcs[t_vc].isReady(curCycle())) {
            if (m_ni_out_vcs_enqueue_time[t_vc] <
                m_ni_out_vcs_enqueue_time[vc]) {
              is_candidate_vc = false;
              break;
            }
          }
        }
      }
      if (!is_candidate_vc) continue;

      m_vc_round_robin = vc;

      outVcState[vc].decrement_credit();
      // Just removing the flit
      flit *t_flit = niOutVcs[vc].getTopFlit();
      t_flit->set_time(curCycle() + Cycles(1));
      outFlitQueue.insert(t_flit);
      // schedule the out link
      outNetLink->scheduleEventAbsolute(clockEdge(Cycles(1)));

      if (t_flit->get_type() == TAIL_ || t_flit->get_type() == HEAD_TAIL_) {
        m_ni_out_vcs_enqueue_time[vc] = Cycles(INFINITE_);
      }
      return;
    } else if (niOutVcs[vc].isReady(curCycle())) {
      DPRINTF(Naive, "[NI] outVcState %d does not have credit\n", vc);
    }
  }
}

int NetworkInterface::get_vnet(int vc) {
  for (int i = 0; i < m_virtual_networks; i++) {
    if (vc >= (i * m_vc_per_vnet) && vc < ((i + 1) * m_vc_per_vnet)) {
      return i;
    }
  }
  fatal("Could not determine vc");
}

// Wakeup the NI in the next cycle if there are waiting
// messages in the protocol buffer, or waiting flits in the
// output VC buffer
void NetworkInterface::checkReschedule() {
  for (const auto &it : inNode_ptr) {
    if (it == nullptr) {
      continue;
    }

    while (it->isReady(clockEdge())) {  // Is there a message waiting
      DPRINTF(Vanilla, "[NI:CheckReschedule] call to wakeup next cycle\n");
      scheduleEvent(Cycles(1));
      return;
    }
  }

  for (auto &ni_out_vc : niOutVcs) {
    if (ni_out_vc.isReady(curCycle() + Cycles(1))) {
      // flit * tf = ni_out_vc.peekTopFlit();
      DPRINTF(Vanilla,
              "[NI:CheckReschedule] "
              "current flits waiting in niOutVc %s\n",
              ni_out_vc);
      scheduleEvent(Cycles(1));
      return;
    }
  }
}

void NetworkInterface::print(std::ostream &out) const {
  out << "[Network Interface]";
}

uint32_t NetworkInterface::functionalWrite(Packet *pkt) {
  uint32_t num_functional_writes = 0;
  for (auto &ni_out_vc : niOutVcs) {
    num_functional_writes += ni_out_vc.functionalWrite(pkt);
  }

  num_functional_writes += outFlitQueue.functionalWrite(pkt);
  return num_functional_writes;
}

NetworkInterface *GarnetNetworkInterfaceParams::create() {
  return new NetworkInterface(this);
}
