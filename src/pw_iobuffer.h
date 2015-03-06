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
 * \file pw_iobuffer.h
 * \brief I/O buffer for channel.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"
#include "./pw_tokenizer.h"

#ifndef __PW_IOBUFFER_H__
#define __PW_IOBUFFER_H__

namespace pw {

class Ssl;

//! \brief IO 버퍼.
//	|---DUMMY---|---READABLE---|---WRITABLE---|
//	m_buf       m_posRead      m_posWrite
class IoBuffer
{
public:
	enum
	{
		DEFAULT_SIZE = 1024*10,
		DEFAULT_DELTA = DEFAULT_SIZE / 2
	};

	struct blob_type final
	{
		char*	buf = nullptr;
		size_t	size = 0;

		inline blob_type() = default;
		inline blob_type(const char* _buf, size_t _size) : buf(const_cast<char*>(_buf)), size(_size) {}
		inline blob_type(const blob_type&) = default;
		inline blob_type(blob_type&&) = default;
		inline blob_type(const std::string& v) : buf(const_cast<char*>(v.c_str())), size(v.size()) {}

		blob_type& operator = (const blob_type&) = default;
		blob_type& operator = (blob_type&&) = default;
	};

public:
	explicit IoBuffer(size_t init_size = DEFAULT_SIZE, size_t delta_size = DEFAULT_DELTA);
	virtual ~IoBuffer();

public:
	std::ostream& dump(std::ostream& os, bool show_buf = false) const;

	virtual ssize_t readFromFile(int fd);
	virtual ssize_t writeToFile(int fd);

	ssize_t writeToBuffer(const char* buf, size_t blen);
	ssize_t writeToBuffer(const std::string& buf) { return this->writeToBuffer(buf.c_str(), buf.size()); }
	ssize_t readFromBuffer(char* buf, size_t blen);
	ssize_t readFromBuffer(std::string& buf, size_t blen);
	ssize_t readFromBufferAll(std::string& buf);

	bool peekLine(Tokenizer& tok) const;
	bool getLine(std::string& out);
	size_t getLine(char* buf, size_t blen);

	inline bool isEmpty(void) const { return ( m_posRead == m_posWrite ); }
	inline bool isFull(void) const { return (m_posWrite == (m_buf +  m_size)); }

	bool increase(size_t delta);
	void decrease(size_t target_size = size_t(-1));
	void flush(void);
	void clear(void);

	//! \brief Dummy 공간 비우기 판단
	inline bool isFlush(void) const
	{
		const size_t th(m_size/2);

		return ( getDummySize() >= th );
	}

	//! \brief Flush 한 뒤, 얻을 수 있는 사용한 공간.
	inline size_t getDummySize(void) const
	{
		return size_t (m_posRead - m_buf);
	}

	//! \brief 버퍼에 쓸 수 있는 공간.
	inline size_t getWriteableSize(void) const
	{
		return (m_size - size_t(m_posWrite - m_buf));
	}

	//! \brief 버퍼에 쓴 공간.
	inline size_t getReadableSize(void) const
	{
		return size_t(m_posWrite - m_posRead);
	}

public:
	//--------------------------------------------------------------------------
	// Dangerous methods.
	// DO NOT USE THESE DIRECTLY!

	inline bool grabRead(blob_type& buf) const
	{
		buf.buf = m_posRead;
		buf.size = getReadableSize();

		return true;
	}

	inline bool grabWrite(blob_type& buf) const
	{
		buf.buf = m_posWrite;
		buf.size = getWriteableSize();

		return true;
	}

	bool grabWrite(blob_type& buf, size_t aquire_size);

	bool moveRead(const size_t blen)
	{
		m_posRead += blen;
		return true;
	}

	bool moveWrite(const size_t blen)
	{
		m_posWrite += blen;
		return true;
	}

protected:
	const size_t	m_init;
	char*			m_buf;
	char*			m_posRead;
	char*			m_posWrite;
	size_t			m_size;
	size_t			m_delta;
};

class IoBufferSsl : public IoBuffer
{
public:
	inline explicit IoBufferSsl(size_t init_size = DEFAULT_SIZE, size_t delta_size = DEFAULT_DELTA, Ssl* ssl = nullptr) : IoBuffer(init_size, delta_size), m_ssl(ssl) {}
	inline virtual ~IoBufferSsl() = default;

public:
	ssize_t readFromFile(int fd) override;
	ssize_t writeToFile(int fd) override;

protected:
	Ssl*			m_ssl;
};

}; //namespace pw;

#endif//!__PW_IOBUFFER_H__

