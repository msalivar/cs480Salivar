
const int packet_size_min = 1;
const int packet_size_max = 20;

class Node
{
	public:
		int contentionWindow;
		int packets_to_send;
		int packets_sent;
		int* packets;

		Node();
		~Node();
		bool isFinished();
};