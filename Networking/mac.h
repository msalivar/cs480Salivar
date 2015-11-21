
const int packet_size_min = 1;
const int packet_size_max = 20;

class Node
{
	public:
		bool is_transmitting;
		int contentionWindow;
		int packets_to_send;
		int packets_sent;
		int* packets;

		int current_packet_time;
		int current_backoff;

		Node();
		~Node();
		bool isFinished();
		void beginTransmit(int ack_time);
		bool transmit();
		void setBackoff(int value);
};