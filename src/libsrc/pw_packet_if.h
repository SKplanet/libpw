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
 * \file pw_packet_if.h
 * \brief Packet interface.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"
#include "./pw_iobuffer.h"

#ifndef __PW_PACKET_IF_H__
#define __PW_PACKET_IF_H__

namespace pw
{

//! \brief 매드 패킷 인터페이스. 모든 패킷은 해당 인터페이스를 상속하여 만든다.
class PacketInterface
{
protected:
	PacketInterface() = default;
	PacketInterface(const PacketInterface&) = delete;
	PacketInterface(PacketInterface&&) = delete;
	PacketInterface& operator = (const PacketInterface&) = delete;
	PacketInterface& operator = (PacketInterface&&) = delete;
	virtual ~PacketInterface() = default;

public:
	virtual ssize_t write(IoBuffer& buf) const = 0;
	virtual std::ostream& write(std::ostream& os) const = 0;
	virtual std::string& write(std::string& ostr) const = 0;

	virtual void clear(void) = 0;
};

class EmptyPacket final : public PacketInterface
{
private:
	using PacketInterface::PacketInterface;

public:
	ssize_t write(IoBuffer&) const override { return 0; }
	std::ostream& write(std::ostream& os) const override { return os; }
	std::string& write(std::string& ostr) const override { ostr.clear(); return ostr; }
	void clear(void) override {}

	static const PacketInterface& s_getInstance(void) { static EmptyPacket p; return p; }
	static const PacketInterface* s_getPointer(void) { return &(s_getInstance()); }
};

inline
const PacketInterface&
getSafePacketInstance(const PacketInterface* p)
{
	return (p ? *p : EmptyPacket::s_getInstance());
}

inline
const PacketInterface*
getSafePacketPointer(const PacketInterface* p)
{
	return (p ? p : EmptyPacket::s_getPointer());
}

// \brief 간단하게 쓸 수 있는 Blob형 패킷
class BlobPacket final : public PacketInterface
{
public:
	using PacketInterface::PacketInterface;
	using PacketInterface::operator =;

public:
	ssize_t write (IoBuffer& buf) const override;
	inline std::ostream& write (std::ostream& os) const override { return os << m_body; }
	inline std::string& write (std::string& ostr) const override { return ostr << m_body; }

	inline void clear (void) override { m_body.clear(); }

	inline blob_type& getBody(void) { return m_body; }
	inline const blob_type& getBody(void) const { return m_body; }

public:
	blob_type	m_body;
};//BlobPacket class

//! \brief 간단하게 쓸 수 있는 std::string형 패킷
class StlStringPacket final : public PacketInterface
{
public:
	using PacketInterface::PacketInterface;
	using PacketInterface::operator =;

public:
	ssize_t write (IoBuffer& buf) const override;
	inline std::ostream& write (std::ostream& os) const override { return os << m_body; }
	inline std::string& write (std::string& ostr) const override { return ostr.append(m_body); }

	inline void clear (void) override { m_body.clear(); }

	inline std::string& getBody(void) { return m_body; }
	inline const std::string& getBody(void) const { return m_body; }

public:
	std::string	m_body;
};//StlStringPacket

};// namespace pw

#endif//!__PW_PACKET_IF_H__

