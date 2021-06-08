#include <iostream>

#include "mem/ruby/slicc_interface/RubySlicc_Util.hh"
#include "mem/ruby/slicc_interface/RequestMsg.hh"

using namespace std;


void
RequestMsg::print(ostream& out) const
{
    out << "[RequestMsg: ";
    out << "at = " << std::hex << this << std::dec << " ";
    out << "addr = " << printAddress(m_addr) << " ";
    out << "Type = " << m_Type << " ";
    out << "Requestor = " << m_Requestor << " ";
    // out << "Destination = " << m_Destination << " ";
    // out << "DataBlk = " << m_DataBlk << " ";
    out << "MessageSize = " << m_MessageSize << " ";
    out << "attackMessage = " << m_attackMessage << " ";
    out << "&attackMessage = " << &m_attackMessage << " ";
    out << "]";
}


bool 
RequestMsg::functionalRead(Packet* p){
    panic("Not Implemented in Gem5");
    return false;
}

bool 
RequestMsg::functionalWrite(Packet *p){

    panic("Not Implemented in Gem5");
    return false;
}
