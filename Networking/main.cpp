#include <stdlib.h>
#include <time.h>
#include <iostream>
#include "mac.h"

using namespace std;

// Global Settings //

const int rts_time = 1;
const int cts_time = 1;
const int slot_time = 20;
const int sifs_time = 5;
const int difs_time = sifs_time + (2 * slot_time); // 45
const int ack_time = 5;
const int packets_per_node = 0; // If 0, random per node

int runSimulation(int numNodes); // Begins Simulation with given number of nodes (wifi devices)
void setupNode(int numPackets, Node* node); // Generates packets to send in the node
void activateNode(Node* node, bool* idle); // Starts packet transfer or decrements backoff counter
int checkForCompletion(int numNodes, Node* nodes); // Is sim done?

int main()
{
	srand(time(NULL));
	int nodeCount = 2;
	int elapsed_time = runSimulation(nodeCount);
	return 0;
}

int runSimulation(int numNodes)
{
	Node* nodes = new Node[numNodes];
	for (int count = 0; count < numNodes; count++)
	{
		setupNode(packets_per_node, &nodes[count]);
	}

	int timer = 0;
	bool finished = false;
	bool channel_idle = true;
	while (!finished)
	{
		for (int count = 0; count < numNodes; count++)
		{
			activateNode(&nodes[count], &channel_idle);
		}
		if (checkForCompletion(numNodes, nodes) == numNodes) { finished = true; }
		timer++;
	}

	delete [] nodes;
	nodes = 0;

	return timer;
}

void setupNode(int numPackets, Node* node)
{
	// Set number of packets?
	if (numPackets == 0)
	{
		int num = rand() % 21;
		node->packets_to_send = num;
	}
	else
	{
		node->packets_to_send = numPackets;
	}
	// Create packets
	node->packets = new int[node->packets_to_send];
	for (int count = 0; count < node->packets_to_send; count++)
	{
		int range = packet_size_max - packet_size_min;
		int num = rand() % range + packet_size_min + 1;
		node->packets[count] = num;
		//cout << num << endl;
	}
}

void activateNode(Node* node, bool* idle)
{
	if (node->is_transmitting)
	{
		// If finished this time
		if (node->transmit()) { *idle = true; }
		return;
	}
	// If chanel is idle and packet can be sent
	if (idle && node->isFinished() == false)
	{
		node->beginTransmit(ack_time);
		node->contentionWindow = 2;
		*idle = false;
		return;
	}
	if (*idle)
	{
		int backoff = rand() % node->contentionWindow;
		node->setBackoff(backoff);
	}
	else
	{
		node->current_backoff--;
		if (node->current_backoff == 0)
		{
			// Collision
			if ()
		}
	}
	// Else choose backoff or count down when idle
	// Counter is 0, transmit and wait for ack
	// Ack recieved? Packet has been recieved -> step 2, else choose higher backoff
}

int checkForCompletion(int numNodes, Node* nodes)
{
	int nodes_finished = 0;
	for (int count = 0; count < numNodes; count++)
	{
		if (nodes[count].isFinished()) { nodes_finished++; }
	}
	return nodes_finished;
}