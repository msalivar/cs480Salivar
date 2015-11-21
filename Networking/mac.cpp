#include "mac.h"

Node::Node()
{
	contentionWindow = 1;
	packets_to_send = 0;
	packets_sent = 0;
	packets = 0;
}

Node::~Node()
{
	delete [] packets;
	packets = 0;
}

bool Node::isFinished()
{
	return packets_sent == packets_to_send;
}