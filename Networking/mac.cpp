#include "mac.h"

Node::Node()
{
	is_transmitting = false;
	contentionWindow = 2;
	packets_to_send = 0;
	packets_sent = 0;
	packets = 0;
	current_packet_time = -1;
	current_backoff = 0;
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

void Node::beginTransmit(int ack_time)
{
	current_packet_time = packets[packets_sent] + ack_time;
}

bool Node::transmit()
{
	current_packet_time--;
	if (current_packet_time == 0)
	{
		is_transmitting = false;
		packets_sent++;
		return true;
	}
	return false;
}

void Node::setBackoff(int value)
{
	current_backoff = value;
}