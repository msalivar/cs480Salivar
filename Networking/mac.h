
const int packet_size_min = 1;
const int packet_size_max = 4;

class Node
{
	public:
		bool waiting_for_ack;
		bool is_transmitting;
		bool backoff_set;
		int id;
		int contentionWindow;
		int packets_to_send;
		int packets_sent;
		int* packets;
		int current_packet_time;
		int current_backoff;
		int current_ack_wait;

		Node();
		~Node();
		bool isFinished();
		void beginTransmit();
		bool transmit();
		void setBackoff(int value);
		bool waitForAck();
};