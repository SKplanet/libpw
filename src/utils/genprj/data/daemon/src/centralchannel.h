#ifndef CENTERALCHANNEL_H
#define CENTERALCHANNEL_H

//! \brief DEMO central channel
class CentralChannel : public pw::MultiChannelInterface
{
public:
	static bool s_initialize(void);
	static void s_destroy(void);

public:
	using pw::MultiChannelInterface::MultiChannelInterface;
	inline virtual ~CentralChannel() {}

private:
	void eventConnected(void);
	void eventDisconnected(void);
	//void eventConnect(void);
	void eventReadPacket(const pw::PacketInterface& pk, const char* body, size_t bodylen);

	bool getHelloPacket(pw::MsgPacket& pk, bool& flag_send, bool& flag_wait);
	bool checkHelloPacket(std::string& peer_name, const pw::MsgPacket& pk);

private:
	static pw::MultiChannelPool* s_pool;
};

#endif // CENTERALCHANNEL_H
