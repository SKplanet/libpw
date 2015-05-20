/*
 * Generated by PWGenPrj
 * Generated at __PWTEMPLATE_DATE__
 */

/*!
 * \file myjob.h
 * \brief Job logic
 */

#ifndef MYJOB_H
#define MYJOB_H

class HttpDemoJob : public pw::Job
{
public:
	static HttpDemoJob* s_run(pw::ChannelInterface& caller_ch, void* param);

public:
	explicit HttpDemoJob(pw::ChannelInterface& caller_ch);
	inline virtual ~HttpDemoJob() {}

private:
	void eventReadPacket(pw::ChannelInterface* response_ch, const pw::PacketInterface& response_pk, void* param, bool& del_this) override;
	void eventTimeout(int64_t diff, bool& del_this) override;
	void eventError(pw::ChannelInterface* response_ch, pw::ChannelInterface::Error error_type, int error, bool& del_this) override;

private:
	pw::ch_name_type m_req_ch = 0;
	pw::ch_name_type m_res_ch = 0;
};

#endif // MYJOB_H