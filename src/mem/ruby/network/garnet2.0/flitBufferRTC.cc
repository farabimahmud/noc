#include "mem/ruby/network/garnet2.0/flitBufferRTC.hh"

flitBufferRTC::flitBufferRTC()
{
    max_size = INFINITE_;
}

flitBufferRTC::flitBufferRTC(int maximum_size)
{
    max_size = maximum_size;
}

bool
flitBufferRTC::isEmpty()
{
    return (m_buffer.size() == 0);
}

bool
flitBufferRTC::isReady(Cycles curTime)
{
    if (m_buffer.size() != 0 ) {
        flit *t_flit = peekTopFlit();
        if (t_flit->ready_to_commit <= curTime)
            return true;
    }
    return false;
}

void
flitBufferRTC::print(std::ostream& out) const
{
    out << "[flitBuffer: " << m_buffer.size() << "] " << std::endl;
    for(auto flit:m_buffer){
        out << *flit << std::endl;
    }
}

bool
flitBufferRTC::isFull()
{
    return (m_buffer.size() >= max_size);
}

void
flitBufferRTC::setMaxSize(int maximum)
{
    max_size = maximum;
}

uint32_t
flitBufferRTC::functionalWrite(Packet *pkt)
{
    uint32_t num_functional_writes = 0;

    for (unsigned int i = 0; i < m_buffer.size(); ++i) {
        if (m_buffer[i]->functionalWrite(pkt)) {
            num_functional_writes++;
        }
    }

    return num_functional_writes;
}
