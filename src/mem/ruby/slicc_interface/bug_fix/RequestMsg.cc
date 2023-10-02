#include <iostream>
#include <memory>

#include "mem/ruby/slicc_interface/bug_fix/RequestMsg.hh"
#include "mem/ruby/system/RubySystem.hh"

using namespace std;
/** \brief Print the state of this object */
void
RequestMsg::print(ostream& out) const
{
    out << "[RequestMsg: ";
    out << "addr = " << printAddress(m_addr) << " ";
    out << "Type = " << m_Type << " ";
    out << "AccessMode = " << m_AccessMode << " ";
    out << "Requestor = " << m_Requestor << " ";
    out << "Destination = " << m_Destination << " ";
    out << "MessageSize = " << m_MessageSize << " ";
    out << "DataBlk = " << m_DataBlk << " ";
    out << "Len = " << m_Len << " ";
    out << "Dirty = " << m_Dirty << " ";
    out << "Prefetch = " << m_Prefetch << " ";
    out << "attackMsg = " << m_attackMessage << " "; 
    out << "Req Enqueue Time = " << m_req_enqueue_time << " ";
    out << "]";
}
bool
RequestMsg::functionalRead(Packet* param_pkt)
{
    if ((m_Type == CoherenceRequestType_PUTX)) {
        return (testAndRead(m_addr, m_DataBlk, param_pkt));
    }
    return (false);

}
bool
RequestMsg::functionalWrite(Packet* param_pkt)
{
return (testAndWrite(m_addr, m_DataBlk, param_pkt));

}
