#include <iostream>
#include "mac.h"

Node::Node()
{
	waiting_for_ack = false;
	is_transmitting = false;
	backoff_set = false;
	contentionWindow = 2;
	packets_to_send = 0;
	packets_sent = 0;
	packets = 0;
	current_packet_time = -1;
	current_backoff = 0;
	current_ack_wait = 0;
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

void Node::beginTransmit()
{
	current_packet_time = packets[packets_sent];
	is_transmitting = true;
}

bool Node::transmit()
{
	current_packet_time--;
	if (current_packet_time == 0)
	{
		is_transmitting = false;
		return true;
	}
	return false;
}

void Node::setBackoff(int value)
{
	current_backoff = value;
	backoff_set = true;
}

bool Node::waitForAck()
{
	if (current_ack_wait == 0)
	{
		waiting_for_ack = false;
		return true;
	}
	else
	{
		current_ack_wait--;
		return false;
	}
}