/*
 * The MIT License (MIT)
 * Copyright (c) 2015 SK PLANET. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*!
 * \file pw_apnschannel.h
 * \brief Channel for Apple Push Notification Service
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "pw_common.h"
#include "pw_packet_if.h"

#ifndef __PW_APNSPACKET_H__
#define __PW_APNSPACKET_H__

namespace pw {

class ApnsPacket : public PacketInterface
{
public:
	enum class Command : uint8_t
	{
		REQUEST = 0x02,
		RESPONSE = 0x08
	};

	enum class ItemId : uint8_t
	{
		DEVICE_TOKEN = 0x01,
		PAYLOAD = 0x02,
		NOTI_ID = 0x03,
		EXP_DATE = 0x04,
		PRIORITY = 0x05,
	};

	using item_type = struct item_type final {
		ItemId id;
		blob_type body;

		item_type() = default;
		item_type(const item_type&) = default;
		item_type(item_type&&) = default;
		item_type(ItemId in_id, const blob_type& in_body) : id(in_id), body(in_body) {}
		item_type(ItemId in_id, blob_type&& in_body) : id(in_id), body(std::move(in_body)) {}
	};

	using noti_id_type = union noti_id_type final {
		uint8_t u8[4];
		uint32_t u32;
	};

	using item_list = std::list<item_type>;

public:
#pragma pack(push, 1)
	using binary_item_header_type = struct binary_item_header_type {
		uint8_t id{0x00};
		uint16_t size{0};
	};

	using binary_packet_header_type = struct binary_packet_header_type {
		uint8_t cmd{0x00};
		uint32_t size{0};

		binary_packet_header_type() = default;
		binary_packet_header_type(Command in_cmd) : cmd(static_cast<uint8_t>(in_cmd)) {}
	};
#pragma pack(pop)
public:
	inline size_t getPacketSize(void) const
	{
		size_t sum(sizeof(binary_packet_header_type));
		for ( auto& i : m_items )
		{
			sum += sizeof(binary_item_header_type);
			sum += i.body.size;
		}

		return sum;
	}

	ssize_t write(IoBuffer& buf) const override;
	std::ostream& write(std::ostream& os) const override;
	std::string& write(std::string& ostr) const override;

	void clear(void) override;

protected:
	void _write(char* out, size_t pklen) const;

public:
	Command m_cmd{Command::REQUEST};
	item_list m_items;

};

class ApnsResponsePacket : public PacketInterface
{
public:
	enum class Status : uint8_t {
		SUCCESS = 0,
		PROCESSING_ERROR = 1,
		MISSING_DEVICE_TOKEN = 2,
		MISSING_TOPIC = 3,
		MISSING_PAYLOAD = 4,
		INVALID_TOKEN_SIZE = 5,
		INVALID_PAYLOAD_SIZE = 6,
		INVALID_TOKEN = 8,
		SHUTDOWN = 10,
		UNKNOWN = 255
	};

public:
	using PacketInterface::PacketInterface;
	virtual ~ApnsResponsePacket() = default;

public:
	inline size_t getPacketSize(void) const { return (sizeof(m_cmd) + sizeof(m_status) + sizeof(m_noti_id)); }

public:
	ApnsPacket::Command m_cmd{ApnsPacket::Command::RESPONSE};
	Status m_status{Status::SUCCESS};
	ApnsPacket::noti_id_type m_noti_id;

public:
	ssize_t write(IoBuffer& buf) const override;
	std::ostream& write(std::ostream& os) const override;
	std::string& write(std::string& ostr) const override;

	void clear(void) override;

protected:
	void _write(char* out) const;
};

}

#endif//__APNSPACKET_H__
