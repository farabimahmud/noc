#ifndef __MEM_RUBY_SLICC_INTERFACE_REQUESTMSG_HH__
#define __MEM_RUBY_SLICC_INTERFACE_REQUESTMSG_HH__

#include <ostream>
#include <vector>

#include "mem/ruby/common/Address.hh"
#include "mem/ruby/common/DataBlock.hh"
#include "mem/ruby/common/WriteMask.hh"
#include "mem/ruby/protocol/HSAScope.hh"
#include "mem/ruby/protocol/HSASegment.hh"
#include "mem/ruby/protocol/Message.hh"
#include "mem/ruby/protocol/PrefetchBit.hh"
#include "mem/ruby/protocol/RubyAccessMode.hh"
#include "mem/ruby/protocol/RubyRequestType.hh"
#include "mem/ruby/protocol/CoherenceRequestType.hh"
#include "debug/Naive.hh"

class RequestMsg:public Message
{
    public:

        Addr m_addr;
        CoherenceRequestType m_Type;
        DataBlock m_DataBlk;
        NetDest m_Destination;
        MachineID m_Requestor;
        MessageSizeType m_MessageSize;
 //        bool m_attackMessage;


        NetDest & getDestination ()
        {
            return m_Destination;
        }

        RequestMsg (Tick curTime):Message (curTime)
    {
        DPRINTF (Naive, "RequestMsg is being called to create %#x\n", this);

        m_Type = CoherenceRequestType_MSG;
        m_MessageSize = MessageSizeType_Data;
        m_attackMessage = false;
    }


        RequestMsg (const RequestMsg & other):Message(other)
        {

            DPRINTF(Naive, "RequestMsg is called with other %s\n",
                    other);
            m_addr = other.m_addr;
            m_Type = other.m_Type;
            m_Destination = other.m_Destination;
     m_DataBlk = other.m_DataBlk;
            m_MessageSize = other.m_MessageSize;
            m_attackMessage = other.m_attackMessage;

            DPRINTF(Naive, "%#x RequestMsg got data from other RequestMsg %#x," 
                "value for attackMessgae is %d and in other %d\n",
                this, &other, m_attackMessage, other.m_attackMessage);
        }

        const Addr & getaddr () const
        {
            return m_addr;
        }


        MsgPtr clone () const
        {
            return std::shared_ptr < Message > (new RequestMsg (*this));
        }


        MachineID & getRequestor(){
            return m_Requestor;
        }

        DataBlock & getDataBlk(){
            return m_DataBlk;
        }

        MessageSizeType& getMessageSize(){
            return m_MessageSize;            
        }


        void print (std::ostream & out) const;
        bool functionalRead (Packet * pkt);
        bool functionalWrite (Packet * pkt);
};

inline
    std::ostream &
operator<< (std::ostream & out, const RequestMsg & obj)
{
    obj.print (out);
    out << std::flush;
    return out;
}

#endif //__MEM_RUBY_SLICC_INTERFACE_REQUESTMSG_HH__
