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
 * \file pw_apnspacket.cpp
 * \brief Packet for Apple Push Notification Service
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "pw_apnspacket.h"
#include <netinet/in.h>

namespace pw {

ssize_t
pw::ApnsPacket::write (IoBuffer& obuf) const
{
	IoBuffer::blob_type b;
	const size_t pklen(getPacketSize());
	if ( not obuf.grabWrite(b, pklen+1) ) return ssize_t(-1);

	this->_write(b.buf, pklen);
	obuf.moveWrite(pklen);

	return pklen;
}

std::ostream&
pw::ApnsPacket::write (std::ostream& os) const
{
	blob_type b;
	b.allocate(getPacketSize());
	this->_write(const_cast<char*>(b.buf), b.size);
	return os.write(b.buf, b.size);
}

std::string&
pw::ApnsPacket::write (std::string& ostr) const
{
	ostr.resize(getPacketSize());
	this->_write(const_cast<char*>(ostr.c_str()), ostr.size());
	return ostr;
}

void
pw::ApnsPacket::clear (void)
{
	this->m_cmd = Command::REQUEST;
	this->m_items.clear();
}

void
pw::ApnsPacket::_write(char* buf, size_t pklen) const
{
	binary_packet_header_type bp (m_cmd);
	bp.size = htonl(pklen - sizeof(bp));

	memcpy(buf, &bp, sizeof(bp));
	buf += sizeof(bp);

	binary_item_header_type bi;
	//size_t bi_size(0);
	for ( auto& i : m_items )
	{
		bi.id = static_cast<uint8_t>(i.id);
		auto bi_size(static_cast<uint16_t>(i.body.size));
		bi.size = htons(bi_size);
		memcpy(buf, &bi, sizeof(bi));
		buf += sizeof(bi);
		if ( bi_size )
		{
			memcpy(buf, i.body.buf, bi_size);
			buf += bi_size;
		}
	}
}

ssize_t
pw::ApnsResponsePacket::write (IoBuffer& obuf) const
{
	IoBuffer::blob_type b;
	if ( not obuf.grabWrite(b, getPacketSize()) ) return ssize_t(-1);

	this->_write(b.buf);
	obuf.moveWrite(getPacketSize());

	return getPacketSize();
}

std::ostream&
pw::ApnsResponsePacket::write (std::ostream& os) const
{
	char b[getPacketSize()];
	this->_write(b);
	return os.write(b, getPacketSize());
}

std::string&
pw::ApnsResponsePacket::write (std::string& ostr) const
{
	ostr.resize(getPacketSize());
	this->_write(const_cast<char*>(ostr.c_str()));
	return ostr;
}

void
pw::ApnsResponsePacket::clear (void)
{
	this->m_cmd = ApnsPacket::Command::RESPONSE;
	this->m_noti_id.u32 = 0;
	this->m_status = Status::SUCCESS;
}

void
pw::ApnsResponsePacket::_write (char* out) const
{
	out[0] = static_cast<char>(m_cmd);
	out[1] = static_cast<char>(m_status);
	memcpy(&(out[2]), &m_noti_id, sizeof(m_noti_id));
}

/* namespace pw */
}
