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
 * \file pw_msgpacket.cpp
 * \brief Channel for pw default message.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_msgpacket.h"
#include "./pw_encode.h"
#include "./pw_log.h"

namespace pw {

inline
static
unsigned long
_str2ulong(const char* buf)
{
	uint8_t ret(0);
	int i(0);
	while ( (i < 8) and (*(buf+i) not_eq 0x00) )
	{
		if ( *(buf+i) not_eq '0' ) ret |= (1 << i);
		++i;
	}

	return ret;
}

inline
static
char*
_uint82str(char* buf, uint8_t flags)
{
	if ( 0 == flags )
	{
		buf[0] = '0';
		buf[1] = 0x00;
	}

	int i(0);
	while ( i < 3 )
	{
		buf[i] = ((flags >> i) bitand 1) ? '1' : '0';
		++i;
	}
	buf[3] = 0x00;

	return buf;
}

MsgPacket::MsgPacket() : m_trid(0), m_flags(0)
{
}

MsgPacket::MsgPacket(const MsgPacket& pk) : m_code(pk.m_code), m_trid(pk.m_trid), m_flags(pk.m_flags), m_appendix(pk.m_appendix), m_body(pk.m_body, blob_type::CT_MALLOC)
{
}

MsgPacket::MsgPacket(const char* buf, size_t blen) : m_trid(0), m_flags(0)
{
	setHeader(buf, blen);
}

void
MsgPacket::clear(void)
{
	m_code.clear();
	m_trid = 0;
	m_flags = 0;
	m_chunked.clear();
	m_appendix.clear();
	m_body.clear();
}

void
MsgPacket::swap(MsgPacket& pk)
{
	std::swap(m_code, pk.m_code);
	std::swap(m_trid, pk.m_trid);
	std::swap(m_flags, pk.m_flags);
	m_chunked.swap(pk.m_chunked);
	m_appendix.swap(pk.m_appendix);
	m_body.swap(pk.m_body);
}

bool
MsgPacket::setHeader(const char* buf, size_t blen)
{
	//PWTRACE("%s %s", __func__, buf);
	MsgPacket tmp;
	Tokenizer tok(buf, blen);
	char tmpstr[16];

	do
	{
		if ( not tmp.m_code.assign(tok, ' ') ) break;
		if ( not tok.getNext(tmpstr, sizeof(tmpstr), ' ') ) break;
		tmp.m_trid = uint16_t(::atoi(tmpstr));
		if ( not tok.getNext(tmpstr, sizeof(tmpstr), ' ') ) break;
		tmp.m_flags = _str2ulong(tmpstr);

		// Body size
		if ( not tok.getNext(tmpstr, sizeof(tmpstr), ' ') ) break;
		intmax_t blen(strtoimax(tmpstr, nullptr, 10));
		if ( blen > intmax_t(limit_type::MAX_BODY_SIZE) )
		{
			PWLOGLIB("too large body size: input:%jd limit:%jd", blen, intmax_t(limit_type::MAX_BODY_SIZE));
			break;
		}

		if ( blen > 0 )
		{
			if ( not tmp.m_body.allocate(blen) )
			{
				PWLOGLIB("not enough memory: body size:%jd", blen);
				break;
			}
		}

		// Chunked info
		if ( tmp.isFlagChunked() )
		{
			if ( not tok.getNext(tmpstr, sizeof(tmpstr), ' ') ) break;
			tmp.m_chunked.total() = atoi(tmpstr);
			if ( not tok.getNext(tmpstr, sizeof(tmpstr), ' ') ) break;
			tmp.m_chunked.index() = atoi(tmpstr);

			if ( (tmp.m_chunked.total() > 0) and (tmp.m_chunked.index() == 0) )
			{
				// 잘못된 인덱스
				PWLOGLIB("invalid index: total:%d index:%d", int(tmp.m_chunked.total()), int(tmp.m_chunked.index()));
				break;
			}
		}

		// Appendix
		tmp.m_appendix.assign(tok.getPosition(), tok.getLeftSize());
		swap(tmp);

		return true;
	} while (false);

	return false;
}

size_t
MsgPacket::getPacketSize(void) const
{
	char strflags[3+1];

	size_t sum = snprintf(nullptr, 0, "%s %d %s %zu",
			m_code.c_str(),
			static_cast<int>(m_trid),
			_uint82str(strflags, m_flags),
			m_body.size) + 2;

	if ( isFlagChunked() )
	{
		sum += snprintf(nullptr, 0, " %d %d", static_cast<int>(m_chunked.total()), static_cast<int>(m_chunked.index()));
	}

	if ( not m_appendix.empty() ) sum += m_appendix.size() + 1;

	if ( m_body.size ) sum += m_body.size;

	return sum;
}

ssize_t
MsgPacket::write(IoBuffer& obuf) const
{
	PWSHOWMETHOD();
	const ssize_t pklen(getPacketSize());
	char strflags[3+1];

	IoBuffer::blob_type b;
	if ( not obuf.grabWrite(b, pklen+1) ) return ssize_t(-1);

	ssize_t sum = snprintf(b.buf, pklen+1, "%s %d %s %zu",
			m_code.c_str(),
			static_cast<int>(m_trid),
			_uint82str(strflags, m_flags),
			m_body.size);

	if ( isFlagChunked() )
	{
		sum += snprintf(b.buf+sum, pklen+1 - sum, " %d %d", static_cast<int>(m_chunked.total()), static_cast<int>(m_chunked.index()));
	}

	if ( not m_appendix.empty() )
	{
		char* p(b.buf+sum);
		*p = ' ';
		::memcpy(p+1, m_appendix.c_str(), m_appendix.size());
		sum += m_appendix.size() + 1;
	}

	::memcpy(b.buf+sum, "\r\n", 2);
	sum += 2;

	if ( m_body.size )
	{
		::memcpy(b.buf+sum, m_body.buf, m_body.size);
		sum += m_body.size;
	}

	obuf.moveWrite(sum);
	if ( sum not_eq pklen )
	{
		PWABORT("packet length is incorrect: pklen:%zd sum:%zd", pklen, sum);
	}

	return sum;
}

std::string&
MsgPacket::write(std::string& os) const
{
	PWSHOWMETHOD();
	std::stringstream ss;
	this->write(ss);
	ss.str().swap(os);
	return os;
}

std::ostream&
MsgPacket::write(std::ostream& os) const
{
	PWSHOWMETHOD();
	char strflags[3+1];

	os << m_code << ' '
		<< static_cast<int>(m_trid) << ' '
		<< _uint82str(strflags, m_flags) << ' '
		<< m_body.size;

	if ( isFlagChunked() )
	{
		os << ' ' << m_chunked.total() << ' ' << m_chunked.index();
	}

	if ( not m_appendix.empty() )
	{
		os << ' ' << m_appendix;
	}

	os.write("\r\n", 2);
	if ( m_body.size > 0 ) os.write(m_body.buf, m_body.size);

	return os;
}

std::ostream&
MsgPacket::dump(std::ostream& os) const
{
	os << "Type: " << typeid(*this).name() << std::endl;
	os << "Addr: " << static_cast<const void*>(this) << std::endl;
	os << "Code: " << m_code << std::endl;
	os << "Trid: " << m_trid << std::endl;
	os << "Flag: " << static_cast<unsigned long>(m_flags) << std::endl;
	os << "isCompressed: " << static_cast<int>(isFlagCompressed()) << std::endl;
	os << "isEncrypted: " << static_cast<int>(isFlagEncrypted()) << std::endl;
	os << "isChunked: " << static_cast<int>(isFlagChunked()) << std::endl;
	os << "Chunked total: " << m_chunked.total() << std::endl;
	os << "Chunked index: " << m_chunked.index() << std::endl;
	os << "Appendix(" << m_appendix.size() << "): " << m_appendix << std::endl;
	os << "Body length: " << m_body.size << std::endl;
	os << "Body addr:" << static_cast<const void*>(m_body.buf) << std::endl;

	return os;
}

};//namespace pw

