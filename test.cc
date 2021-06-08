#include <stdio.h>

class Message
{
    public:
        bool b;

        Message()
        {
            printf("Message()\n");
            b = 1;
        }

        void print()
        {
            printf("Message %d\n", b);
        }
};


class RequestMessage : public Message
{
    public:
        RequestMessage()
        {
            printf("RequestMessage()\n");
        }
        void print()
        {
            printf("RequestMessage %d\n", b);
        }
};


int main()
{
    Message* msg = new RequestMessage();
    msg->print();
}
