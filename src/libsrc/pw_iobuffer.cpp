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
 * \file pw_iobuffer.cpp
 * \brief I/O buffer for channel.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_iobuffer.h"
#include "./pw_ssl.h"
#include "./pw_log.h"

namespace pw {

IoBuffer::IoBuffer(size_t init_size, size_t delta_size) : m_init(init_size), m_buf(nullptr), m_posRead(nullptr), m_posWrite(nullptr), m_size(init_size), m_delta(delta_size)
{
	if ( m_init == 0 || m_init == size_t(-1) )
	{
		PWABORT("invalid iobuffer initialize size: %zu", m_init);
	}

	if ( nullptr == ( m_buf = static_cast<char*>(::malloc(m_init+1)) ) )
	{
		PWABORT("not enough memory: init_size: %zu %s", init_size, strerror(errno));
	}
	else
	{
		m_posRead = m_posWrite = m_buf;
	}

	if ( m_delta == 0 )
	{
		//m_delta = 4;
		m_delta = 512;
	}

	//PWTRACE("iobuffer is successful: this:%p init:%zu size:%zu buf:%p", this, m_init, m_size, m_buf);
}

std::ostream&
IoBuffer::dump(std::ostream& os, bool show_buf) const
{
	os << "IoBuffer: this: " << this
		<< " m_init: " << m_init
		<< " m_size: " << m_size
		<< " m_delta: " << m_delta
		<< " m_posRead: " << static_cast<void*>(m_posRead)
		<< " m_posWrite: " << static_cast<void*>(m_posWrite)
		<< " dummySize: " << getDummySize()
		<< " end_of_pos: " << static_cast<void*>(m_buf+m_size);
	if ( show_buf ) os << " m_buf: " << static_cast<void*>(m_buf);
	return os;
}

IoBuffer::~IoBuffer()
{
	if ( m_buf )
	{
		free(m_buf);
		m_buf = nullptr;
	}
}

bool
IoBuffer::grabWrite(blob_type& buf, size_t blen)
{
	if ( isFlush() ) flush();

	grabWrite(buf);
	size_t cplen(std::min(blen, buf.size));
	if ( cplen < blen )
	{
		//PWTRACE("auto INCREASE!");
		const size_t delta_times( ((blen - cplen) / m_delta) + 1 );

		increase(delta_times * m_delta);
		grabWrite(buf);
		//PWTRACE("blen:%zu buf.size:%zu", blen, buf.size);
		cplen = std::min(blen, buf.size);
	}

	return ( cplen >= blen );
}

void
IoBuffer::decrease(size_t target_size)
{
	if ( target_size == size_t(-1) ) target_size = m_init;
	else if ( (not target_size) or (target_size == m_size) ) return;

	const size_t cplen(getReadableSize());
	if ( cplen ) flush();

	char* nbuf(static_cast<char*>(::realloc(m_buf, target_size+1)));
	if ( nullptr == nbuf )
	{
		PWLOGLIB("not enough memory: target_size: %zu %s", target_size, strerror(errno));
		return;
	}

	if ( nbuf not_eq m_buf )
	{
		m_buf = nbuf;
		m_posRead = nbuf;
		m_posWrite = nbuf + cplen;
	}

	m_size = target_size;
}

bool
IoBuffer::increase(size_t delta)
{
	//PWSHOWMETHOD();
	if ( not delta ) return false;

	const size_t cplen(getReadableSize());
	if ( cplen ) flush();

	const size_t target_size(m_size+delta);
	PWTRACE("%s %p now:%zu target:%zu", __func__, this, m_size, target_size);

	char* nbuf ( static_cast<char*>(::realloc(m_buf, target_size+1)) );
	if ( nullptr == nbuf )
	{
		PWLOGLIB("not enough memory: target_size: %zu %s", target_size, strerror(errno));
		return false;
	}

	if ( nbuf not_eq m_buf )
	{
		m_buf = nbuf;
		m_posRead = nbuf;
		m_posWrite = nbuf + cplen;
	}

	m_size = target_size;

	return true;
}

void
IoBuffer::flush(void)
{
	//PWTRACE("flush!");
	if ( m_posRead == m_buf )
	{
		PWTRACE("skip flush!");
		return;
	}

	const size_t cplen(getReadableSize());
	if ( cplen > 0 )
	{
		::memmove(m_buf, m_posRead, cplen);
		m_posRead = m_buf;
		m_posWrite = m_buf + cplen;
	}
	else
	{
		m_posRead = m_buf;
		m_posWrite = m_buf;
	}
}

ssize_t
IoBuffer::writeToBuffer(const char* buf, size_t blen)
{
	if ( isFlush() ) flush();
	if ( blen == 0 or buf == nullptr ) return 0;

	blob_type b;
	grabWrite(b, blen);
	size_t cplen(std::min(blen, b.size));

	//PWTRACE("class: %p buf:%p b.buf:%p", this, m_buf, b.buf);
	::memcpy(b.buf, buf, cplen);
	m_posWrite += cplen;

	return cplen;
}

ssize_t
IoBuffer::writeToFile(int fd)
{
	blob_type b;
	grabRead(b);
	if ( b.size > 0 )
	{
		const ssize_t cplen(::write(fd, b.buf, b.size));
		if ( cplen > 0 )
		{
			m_posRead += cplen;
			return cplen;
		}
		else if ( cplen == 0 ) return 0;

		return -1;
	}

	return 0;
}

ssize_t
IoBuffer::readFromBuffer(char* buf, size_t blen)
{
	blob_type b;
	grabRead(b);
	const size_t cplen(std::min(blen, b.size));
	::memcpy(buf, b.buf, cplen);
	m_posRead += cplen;

	return cplen;
}

ssize_t
IoBuffer::readFromBuffer ( string& buf, size_t blen )
{
	blob_type b;
	grabRead(b);
	auto cplen(std::min(blen, b.size));
	if ( cplen ) buf.assign(b.buf, cplen);
	else buf.clear();
	m_posRead += cplen;

	return cplen;
}

ssize_t
IoBuffer::readFromBufferAll ( string& buf )
{
	blob_type b;
	grabRead(b);
	if ( b.size ) buf.assign(b.buf, b.size);
	else buf.clear();
	m_posRead += b.size;

	return b.size;
}

ssize_t
IoBuffer::readFromFile(int fd)
{
	if ( isFlush() ) flush();

	if ( getWriteableSize() == 0 )
	{
		PWTRACE("INCREASE!!! %zu", getDummySize());
		increase(m_delta);
	}

	blob_type b;
	grabWrite(b);

	const ssize_t cplen(::read(fd, b.buf, b.size));
	if ( cplen > 0 )
	{
		m_posWrite += cplen;
		return cplen;
	}
	else if ( cplen == 0 ) return ssize_t(0);

	return ssize_t(-1);
}

void
IoBuffer::clear(void)
{
	if ( m_buf )
	{
		m_buf[0] = 0x00;
		m_posRead = m_buf;
		m_posWrite = m_buf;
	}
}

bool
IoBuffer::peekLine(Tokenizer& tok) const
{
	const char* p(m_posRead);
	size_t rlen(getReadableSize()), gap;

	while ( nullptr != ( p = (char*)memchr(p, '\n', rlen) ) )
	{
		if ( (gap = size_t(p - m_posRead)) < 1 )
		{
			++p;
			if ( p == m_posWrite ) break;
			rlen -= 1;
		}

		rlen -= (gap+1);

		if ( *(p-1) == '\r' )
		{
			tok.setBuffer(m_posRead, gap-1);
			return true;
		}

		++p;
	}

	return false;
}

bool
IoBuffer::getLine(std::string& out)
{
	const char* p(m_posRead);
	size_t rlen(getReadableSize()), gap;

	while ( nullptr != ( p = (char*)memchr(p, '\n', rlen) ) )
	{
		if ( (gap = size_t(p - m_posRead)) < 1 )
		{
			++p;
			if ( p == m_posWrite ) break;
			rlen -= 1;
		}

		rlen -= (gap+1);

		if ( *(p-1) == '\r' )
		{
			out.assign(m_posRead, gap-1);
			moveRead(gap+1);
			return true;
		}

		++p;
	}

	return false;
}

size_t
IoBuffer::getLine(char* buf, size_t blen)
{
	const char* p(m_posRead);
	size_t rlen(getReadableSize()), gap(0);

	while ( nullptr != ( p = (char*)memchr(p, '\n', rlen) ) )
	{
		if ( (gap = size_t(p - m_posRead)) < 1 )
		{
			++p;
			if ( p == m_posWrite ) break;
			rlen -= 1;
		}

		--gap;

		if ( gap > blen )
		{
			PWTRACE("overflow: blen:%zu gap:%zu", blen, gap);
			break;
		}

		rlen -= (gap+2);

		if ( *(p-1) == '\r' )
		{
			::memcpy(buf, m_posRead, gap);
			if ( gap < blen ) buf[gap] = 0x00;
			moveRead(gap+2);
			return gap;
		}

		++p;
	}

	return size_t(-1);
}

ssize_t
IoBufferSsl::readFromFile(int)
{
	if ( m_ssl )
	{
		if ( isFlush() ) flush();

		if ( getWriteableSize() == 0 )
		{
			PWTRACE("INCREASE!!!");
			increase(m_delta);
		}

		blob_type b;
		grabWrite(b);

		const ssize_t cplen(m_ssl->read(b.buf, b.size));
		if ( cplen > 0 )
		{
			m_posWrite += cplen;
			return cplen;
		}
		else if ( cplen == 0 ) return ssize_t(0);
	}

	return ssize_t(-1);
}

ssize_t
IoBufferSsl::writeToFile(int)
{
	if ( m_ssl )
	{
		blob_type b;
		grabRead(b);
		if ( b.size > 0 )
		{
			const ssize_t cplen(m_ssl->write(b.buf, b.size));
			if ( cplen > 0 )
			{
				m_posRead += cplen;
				return cplen;
			}
			else if ( cplen == 0 ) return ssize_t(0);
			else return ssize_t(-1);
		}

		return 0;
	}

	return ssize_t(-1);
}

};//namespace pw
