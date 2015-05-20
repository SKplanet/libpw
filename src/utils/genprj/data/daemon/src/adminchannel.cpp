/*
 * Generated by PWGenPrj
 * Generated at __PWTEMPLATE_DATE__
 */

/*!
 * \file adminchannel.cpp
 * \brief Admin channel logic
 */

#include "./mycommon.h"
#include "./adminchannel.h"

static const std::map<pw::KeyCode, AdminChannel::proc_ptr_t> s_handler = {
	{"EXIT", &AdminChannel::procEXIT},
	// Add new command here!
};

void AdminChannel::eventReadPacket(const pw::PacketInterface& pk, const char*, size_t)
{
	auto& msgpk(dynamic_cast<const pw::MsgPacket&>(pk));
	auto ib(s_handler.find(msgpk.m_code));
	if ( ib not_eq s_handler.end() )
	{
		auto method(ib->second);
		(*this.*method)(msgpk);
	}
	else eventError(Error::INVALID_PACKET, 0);
}

void AdminChannel::procEXIT(const pw::MsgPacket& pk)
{
	CMDLOG.log("%s", pk.m_code.c_str());
	write(pk);
	INST.setFlagRun(false);
}
