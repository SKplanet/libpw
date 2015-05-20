#include "./mycommon.h"
#include "./centralchannel.h"

static class CentralChannelFactory : public pw::MultiChannelFactory
{
public:
	virtual ~CentralChannelFactory() {}

public:
	pw::MultiChannelInterface* create(pw::MultiChannelPool::create_param_type& param)
	{
		return new CentralChannel(param);
	}

} s_central_factory;

pw::MultiChannelPool*	CentralChannel::s_pool(nullptr);

bool
CentralChannel::s_initialize(void)
{
	static pw::MultiChannelPool::create_param_type param;
	param.tag = "central";
	param.factory = &s_central_factory;

	return nullptr not_eq (s_pool = pw::MultiChannelPool::s_create(param));
}

void
CentralChannel::s_destroy(void)
{
	pw::MultiChannelPool::s_release(s_pool);
}

#if 0
void
CentralChannel::eventConnect(void)
{
	setConnected();
}
#endif

void
CentralChannel::eventConnected(void)
{
}

void
CentralChannel::eventDisconnected(void)
{
}

void
CentralChannel::eventReadPacket(const pw::PacketInterface& pk, const char* body, size_t bodylen)
{
}

bool
CentralChannel::getHelloPacket(pw::MsgPacket& pk, bool& flag_send, bool& flag_wait)
{
	flag_send = false;	// Send hello packet to server
	flag_wait = false;	// Wait for response packet from server
	return true;
}

bool
CentralChannel::checkHelloPacket(std::string& peer_name, const pw::MsgPacket& pk)
{
	//! \todo Check hello packet is validate.
	return true;
}

