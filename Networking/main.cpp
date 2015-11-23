#include <stdlib.h>
#include <time.h>
#include <iostream>
#include "mac.h"

using namespace std;

// Global Settings //

const int sifs_time = 3; // Represents time to wait before sending an ack + time to wait for ack
const int difs_time = 5; // Represents time to wait before sending a packet
const int packets_per_node = 0; // If 0, random per node
const int packet_num_min = 5;
const int packet_num_max = 15;

int runSimulation(int numNodes, int* collisions); // Begins Simulation with given number of nodes (wifi devices)
void setupNode(int numPackets, Node* node); // Generates packets to send in the node
bool activateNode(Node* node, bool* idle, int timer); // Starts packet transfer or decrements backoff counter
int checkForCompletion(int numNodes, Node* nodes); // Is sim done?
bool checkForCollision(); // Are multiple nodes ready to transmit?

int main()
{
	srand(time(NULL));
	int nodeCount = 5;
	int collisions = 0;
	int elapsed_time = runSimulation(nodeCount, &collisions);
	cout << "Elapsed Time: " << elapsed_time - 1 << endl;
	return 0;
}

int runSimulation(int numNodes, int* collisions)
{
	cout << "-----------------------------------" << endl;
	cout << "Begin Simulation: " << numNodes << " nodes" << endl;
	Node* nodes = new Node[numNodes];
	for (int count = 0; count < numNodes; count++)
	{
		cout << "Node " << count + 1 << " initialized with ";
		setupNode(packets_per_node, &nodes[count]);
	}
	cout << "-----------------------------------" << endl;
	int timer = 1;
	collisions = 0;
	bool finished = false;
	bool channel_idle = true;
	bool node_finished;
	while (!finished)
	{
		if (checkForCollision()) { collisions++; }
		node_finished = false;
		for (int count = 0; count < numNodes; count++)
		{
			nodes[count].id = count + 1;
			if (activateNode(&nodes[count], &channel_idle, timer))
			{
				node_finished = true;
			}
		}
		if (node_finished) { channel_idle = true; }
		if (checkForCompletion(numNodes, nodes) == numNodes) { finished = true; }
		timer++;
		cout << "-----------------------------------" << endl;
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
		int range = packet_num_min - packet_num_max;
		int num = rand() % range + packet_num_min + 1;
		node->packets_to_send = num;
	}
	else
	{
		node->packets_to_send = numPackets;
	}
	cout << node->packets_to_send << " packets:" << endl;
	// Create packets
	node->packets = new int[node->packets_to_send];
	for (int count = 0; count < node->packets_to_send; count++)
	{
		int range = packet_size_max - packet_size_min + 1;
		int num = rand() % range + packet_size_min;
		node->packets[count] = num;
		cout << "Packet " << count + 1 << ": " << num << " units" << endl;
	}
}

bool activateNode(Node* node, bool* idle, int timer)
{
	if (node->isFinished())	{ return false;	}
	if (node->waiting_for_ack)
	{
		if (node->waitForAck())
		{
			int ack_recieved = rand() % 100 + 1;
			if (ack_recieved < 6)
			{
				//Collision: Ack was not recieved
				node->contentionWindow *= 2;
				cout << "Time " << timer << " - Node: " << node->id << " - Ack Not Recieved" << endl;	
				return false;
			}
			else
			{
				cout << "Time " << timer << " - Node: " << node->id << " - Ack Recieved" << endl;
				node->packets_sent++;
				return false;
			}	
		}
		cout << "Time " << timer << " - Node: " << node->id << " - Waiting for Ack" << endl;
		return false;
	}
	// DIFS countdown
	if (node->waiting_to_transmit && node->current_difs > 0)
	{
		node->current_difs--;
		cout << "Time " << timer << " - Node: " << node->id << " - waiting to transmit - difs" << endl;
		return false;
	}
	// Currently transmitting a packet
	if (node->is_transmitting)
	{
		cout << "Time " << timer << " - Node: " << node->id << " - transmitting... " << node->current_packet_time << endl;
		// If finished this time
		if (node->transmit())
		{
			cout << "Time " << timer << " - Node: " << node->id << " - transmitting finished " << endl;
			node->current_ack_wait = sifs_time;
			node->waiting_for_ack = true;
			return true;
		}
		return false;
	}
	if (*idle)
	{
		// Count down when idle
		if (node->current_backoff > 0)
		{
			node->current_backoff--;
			cout << "Time " << timer << " - Node: " << node->id << " - decrement backoff -> " << node->current_backoff << endl;
			return false;		
		}
		// Counter is 0, transmit and wait for ack
		if (node->current_backoff == 0)
		{
			if (node->backoff_set == false && node->waiting_to_transmit == false)
			{
				// Chance to idle
				int want_to_transmit = rand() % 10 + 1;
				if (want_to_transmit < 6)
				{
					cout << "Time " << timer << " - Node: " << node->id << " - idle" << endl;
					return false;
				}
				// Initialize difs wait
				else
				{
					node->waiting_to_transmit = true;
					node->current_difs = difs_time - 1;
					cout << "Time " << timer << " - Node: " << node->id << " - waiting to transmit - difs" << endl;
					return false;
				}
			}
			node->beginTransmit();
			node->contentionWindow = 2;
			node->waiting_to_transmit = false;
			node->backoff_set = false;
			*idle = false;
			cout << "Time " << timer << " - Node: " << node->id << " - begin transmit - packet " << node->packets_sent + 1 << endl;
			cout << "Time " << timer << " - Node: " << node->id << " - transmitting... " << node->current_packet_time << endl;
			// If finished this time
			if (node->transmit())
			{
				cout << "Time " << timer << " - Node: " << node->id << " - transmitting finished " << endl;
				node->current_ack_wait = sifs_time;
				node->waiting_for_ack = true;
				return true;
			}
			return false;
		}
	}
	// Choose backoff or do nothing if not idle
	else if (node->backoff_set == false)
	{
		int backoff = rand() % node->contentionWindow;
		node->setBackoff(backoff);
		cout << "Time " << timer << " - Node: " << node->id << " - idle - backoff set to " << node->current_backoff << endl;
	}
	else
	{
		cout << "Time " << timer << " - Node: " << node->id << " - idle - backoff " << node->current_backoff << endl;			
	}
	// Ack recieved? Packet has been recieved -> step 2, else choose higher backoff
	return false;
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

bool checkForCollision()
{
	
}