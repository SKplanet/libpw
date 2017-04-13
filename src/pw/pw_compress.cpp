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
 * \file pw_compress.cpp
 * \brief Support compression with zlib(http://www.zlib.net/),
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_compress.h"
#include "./pw_log.h"
#include <cassert>

#include <zlib.h>

namespace pw {

#ifndef MAX_WBITS
#	define MAX_WBITS	15
#endif//MAX_WBITS

static Compress* g_comp(nullptr);
static Compress* g_comp_gzip(nullptr);
static Compress* g_uncomp(nullptr);
static Compress* g_uncomp_gzip(nullptr);

class _pw_compress : public Compress
{
public:
	bool		m_init;
	::z_stream	m_stream;
	int			m_level;
	const int	m_window_bits;

public:
	inline compress_type getCompressType(void) const { return CT_COMPRESS; }
	inline void* getStream(void) { return &m_stream; }

public:
	inline _pw_compress(int level, size_t chunk_size, bool gzip) : Compress(chunk_size), m_init(false), m_level(level), m_window_bits(gzip ? MAX_WBITS + 16 : MAX_WBITS)
	{
		if ( nullptr == m_chunk ) return;

		m_stream.zalloc = Z_NULL;
		m_stream.zfree = Z_NULL;
		m_stream.opaque = Z_NULL;

		int res;
		if ( Z_OK == (res = ::deflateInit2(&m_stream, level, Z_DEFLATED, m_window_bits, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY))) m_init = true;
		//PWTRACE("deflateInit2: %d", res);
	}

	inline ~_pw_compress(void)
	{
		if ( m_init )
		{
			::deflateEnd(&m_stream);
			m_init = false;
		}
	}

	bool reinitialize(void)
	{
		if ( m_init )
		{
			::deflateEnd(&m_stream);
			m_init = false;
		}

		m_stream.zalloc = Z_NULL;
		m_stream.zfree = Z_NULL;
		m_stream.opaque = Z_NULL;

		if ( Z_OK == ::deflateInit2(&m_stream, m_level, Z_DEFLATED, m_window_bits, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) ) m_init = true;

		return m_init;
	}

	bool reinitialize(int level, size_t chunk_size)
	{
		if ( m_init )
		{
			::deflateEnd(&m_stream);
			m_init = false;
		}

		if ( m_chunk_size not_eq chunk_size )
		{
			if ( m_chunk ) ::free(m_chunk);
			if ( nullptr == (m_chunk = (unsigned char*)::malloc(chunk_size)) )
			{
				PWLOGLIB("not enough memory");
				return false;
			}

			*const_cast<size_t*>(&m_chunk_size) = chunk_size;
		}

		if ( m_level not_eq level ) m_level = level;

		m_stream.zalloc = Z_NULL;
		m_stream.zfree = Z_NULL;
		m_stream.opaque = Z_NULL;

		if ( Z_OK == ::deflateInit2(&m_stream, m_level, Z_DEFLATED, m_window_bits, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) ) m_init = true;

		return m_init;
	}

	bool reinitialize(size_t chunk_size)
	{
		if ( m_init )
		{
			::deflateEnd(&m_stream);
			m_init = false;
		}

		if ( m_chunk_size not_eq chunk_size )
		{
			if ( m_chunk ) ::free(m_chunk);
			if ( nullptr == (m_chunk = (unsigned char*)::malloc(chunk_size)) )
			{
				PWLOGLIB("not enough memory");
				return false;
			}

			*const_cast<size_t*>(&m_chunk_size) = chunk_size;
		}

		m_stream.zalloc = Z_NULL;
		m_stream.zfree = Z_NULL;
		m_stream.opaque = Z_NULL;

		if ( Z_OK == ::deflateInit2(&m_stream, m_level, Z_DEFLATED, m_window_bits, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) ) m_init = true;

		return m_init;
	}

	bool update(std::string& out, const char* _in, size_t ilen)
	{
		if ( not m_init ) return false;

		unsigned char* in((unsigned char*)_in);
		size_t left(ilen), cplen;
		int ret;

		while ( left > 0 )
		{
			m_stream.avail_in = std::min(m_chunk_size, left);
			m_stream.next_in = in;
			in += m_stream.avail_in;
			left -= m_stream.avail_in;

			do {
				m_stream.avail_out = m_chunk_size;
				m_stream.next_out = m_chunk;
				ret = ::deflate(&m_stream, Z_NO_FLUSH);
				if ( not ( Z_OK == ret || Z_STREAM_END == ret ) )
				{
					::deflateEnd(&m_stream);
					m_init = false;
					return false;
				}

				cplen = m_chunk_size - m_stream.avail_out;
				if ( cplen > 0 ) out.append((char*)m_chunk, cplen);
			} while (m_stream.avail_out == 0);
		}

		return true;
	}

	bool update(std::ostream& out, const char* _in, size_t ilen)
	{
		if ( not m_init ) return false;

		unsigned char* in((unsigned char*)_in);
		size_t left(ilen), cplen;
		int ret;

		while ( left > 0 )
		{
			m_stream.avail_in = std::min(m_chunk_size, left);
			m_stream.next_in = in;
			in += m_stream.avail_in;
			left -= m_stream.avail_in;

			do {
				m_stream.avail_out = m_chunk_size;
				m_stream.next_out = m_chunk;
				ret = ::deflate(&m_stream, Z_NO_FLUSH);
				if ( not ( Z_OK == ret || Z_STREAM_END == ret ) )
				{
					::deflateEnd(&m_stream);
					m_init = false;
					return false;
				}

				cplen = m_chunk_size - m_stream.avail_out;
				if ( cplen > 0 ) out.write((char*)m_chunk, cplen);
			} while (m_stream.avail_out == 0);
		}

		return true;
	}

	bool update(blob_type& out, const char* _in, size_t ilen)
	{
		if ( not m_init ) return false;

		unsigned char* in((unsigned char*)_in);
		size_t left(ilen), cplen;
		int ret;

		while ( left > 0 )
		{
			m_stream.avail_in = std::min(m_chunk_size, left);
			m_stream.next_in = in;
			in += m_stream.avail_in;
			left -= m_stream.avail_in;

			do {
				m_stream.avail_out = m_chunk_size;
				m_stream.next_out = m_chunk;
				ret = ::deflate(&m_stream, Z_NO_FLUSH);
				if ( not ( Z_OK == ret || Z_STREAM_END == ret ) )
				{
					::deflateEnd(&m_stream);
					m_init = false;
					return false;
				}

				cplen = m_chunk_size - m_stream.avail_out;
				if ( cplen > 0 ) out.append(m_chunk, cplen);
			} while (m_stream.avail_out == 0);
		}

		return true;
	}

	bool finalize(std::string& out)
	{
		if ( not m_init ) return false;

		int ret(0);
		size_t cplen(0);
		m_stream.avail_in = 0;
		m_stream.next_in = nullptr;

		do {
			m_stream.avail_out = m_chunk_size;
			m_stream.next_out = m_chunk;
			ret = ::deflate(&m_stream, Z_FINISH);
			if ( not (Z_OK == ret || Z_STREAM_END == ret) )
			{
				::deflateEnd(&m_stream);
				m_init = false;
				return false;
			}

			cplen = m_chunk_size - m_stream.avail_out;
			if ( cplen > 0 ) out.append((char*)m_chunk, cplen);
		} while ( m_stream.avail_out == 0);

		::deflateEnd(&m_stream);
		m_init = false;

		return true;
	}

	bool finalize(std::ostream& out)
	{
		if ( not m_init ) return false;

		int ret(0);
		size_t cplen(0);
		m_stream.avail_in = 0;
		m_stream.next_in = nullptr;

		do {
			m_stream.avail_out = m_chunk_size;
			m_stream.next_out = m_chunk;
			ret = ::deflate(&m_stream, Z_FINISH);
			if ( not (Z_OK == ret || Z_STREAM_END == ret) )
			{
				::deflateEnd(&m_stream);
				m_init = false;
				return false;
			}

			cplen = m_chunk_size - m_stream.avail_out;
			if ( cplen > 0 ) out.write((char*)m_chunk, cplen);
		} while ( m_stream.avail_out == 0);

		::deflateEnd(&m_stream);
		m_init = false;

		return true;
	}

	bool finalize(blob_type& out)
	{
		if ( not m_init ) return false;

		int ret(0);
		size_t cplen(0);
		m_stream.avail_in = 0;
		m_stream.next_in = nullptr;

		do {
			m_stream.avail_out = m_chunk_size;
			m_stream.next_out = m_chunk;
			ret = ::deflate(&m_stream, Z_FINISH);
			if ( not (Z_OK == ret || Z_STREAM_END == ret) )
			{
				::deflateEnd(&m_stream);
				m_init = false;
				return false;
			}

			cplen = m_chunk_size - m_stream.avail_out;
			if ( cplen > 0 ) out.append(m_chunk, cplen);
		} while ( m_stream.avail_out == 0);

		::deflateEnd(&m_stream);
		m_init = false;

		return true;
	}

};

class _pw_uncompress : public Compress
{
public:
	bool		m_init;
	::z_stream	m_stream;
	const int	m_window_bits;

public:
	inline compress_type getCompressType(void) const { return CT_UNCOMPRESS; }
	inline void* getStream(void) { return &m_stream; }

public:
	inline _pw_uncompress(size_t chunk_size, bool gzip) : Compress(chunk_size), m_init(false), m_window_bits(gzip ? MAX_WBITS + 16 : MAX_WBITS)
	{
		if ( nullptr == m_chunk ) return;

		m_stream.zalloc = Z_NULL;
		m_stream.zfree = Z_NULL;
		m_stream.opaque = Z_NULL;

		int res;
		if ( Z_OK == (res = ::inflateInit2(&m_stream, m_window_bits)) ) m_init = true;
	}

	inline ~_pw_uncompress(void)
	{
		if ( m_init )
		{
			::inflateEnd(&m_stream);
			m_init = false;
		}
	}

	bool reinitialize(void)
	{
		if ( m_init )
		{
			::inflateEnd(&m_stream);
			m_init = false;
		}

		m_stream.zalloc = Z_NULL;
		m_stream.zfree = Z_NULL;
		m_stream.opaque = Z_NULL;

		if ( Z_OK == ::inflateInit2(&m_stream, m_window_bits) ) m_init = true;

		return m_init;
	}

	inline bool reinitialize(int /* level */, size_t chunk_size)
	{
		return reinitialize(chunk_size);
	}

	bool reinitialize(size_t chunk_size)
	{
		if ( m_init )
		{
			::inflateEnd(&m_stream);
			m_init = false;
		}

		if ( m_chunk_size not_eq chunk_size )
		{
			if ( m_chunk ) ::free(m_chunk);
			if ( nullptr == (m_chunk = (unsigned char*)::malloc(chunk_size)) )
			{
				PWLOGLIB("not enough memory");
				return false;
			}

			*const_cast<size_t*>(&m_chunk_size) = chunk_size;
		}

		m_stream.zalloc = Z_NULL;
		m_stream.zfree = Z_NULL;
		m_stream.opaque = Z_NULL;

		if ( Z_OK == ::inflateInit2(&m_stream, m_window_bits) ) m_init = true;

		return m_init;
	}

	// UNCOMPRESS
	bool update(std::string& out, const char* _in, size_t ilen)
	{
		if ( not m_init ) return false;

		unsigned char* in((unsigned char*)_in);
		size_t left(ilen), cplen(0);
		int ret(0);

		while ( left > 0 )
		{
			m_stream.avail_in = std::min(m_chunk_size, left);
			m_stream.next_in = in;
			in += m_stream.avail_in;
			left -= m_stream.avail_in;

			do {
				m_stream.avail_out = m_chunk_size;
				m_stream.next_out = m_chunk;
				ret = ::inflate(&m_stream, Z_NO_FLUSH);
				if ( not ( Z_OK == ret || Z_STREAM_END == ret ) )
				{
					//PWTRACE("inflate: %d", ret);
					::inflateEnd(&m_stream);
					m_init = false;
					return false;
				}
				assert(ret != Z_STREAM_ERROR);

				cplen = m_chunk_size - m_stream.avail_out;
				if ( cplen > 0 ) out.append((char*)m_chunk, cplen);
			} while (m_stream.avail_out == 0);
		}

		return true;
	}

	// UNCOMPRESS
	bool update(std::ostream& out, const char* _in, size_t ilen)
	{
		if ( not m_init ) return false;

		unsigned char* in((unsigned char*)_in);
		size_t left(ilen), cplen(0);
		int ret(0);

		while ( left > 0 )
		{
			m_stream.avail_in = std::min(m_chunk_size, left);
			m_stream.next_in = in;
			in += m_stream.avail_in;
			left -= m_stream.avail_in;

			do {
				m_stream.avail_out = m_chunk_size;
				m_stream.next_out = m_chunk;
				ret = ::inflate(&m_stream, Z_NO_FLUSH);
				if ( not ( Z_OK == ret || Z_STREAM_END == ret ) )
				{
					//PWTRACE("inflate: %d", ret);
					::inflateEnd(&m_stream);
					m_init = false;
					return false;
				}
				assert(ret != Z_STREAM_ERROR);

				cplen = m_chunk_size - m_stream.avail_out;
				if ( cplen > 0 ) out.write((char*)m_chunk, cplen);
			} while (m_stream.avail_out == 0);
		}

		return true;
	}

	// UNCOMPRESS
	bool update(blob_type& out, const char* _in, size_t ilen)
	{
		if ( not m_init ) return false;

		unsigned char* in((unsigned char*)_in);
		size_t left(ilen), cplen(0);
		int ret(0);

		while ( left > 0 )
		{
			m_stream.avail_in = std::min(m_chunk_size, left);
			m_stream.next_in = in;
			in += m_stream.avail_in;
			left -= m_stream.avail_in;

			do {
				m_stream.avail_out = m_chunk_size;
				m_stream.next_out = m_chunk;
				ret = ::inflate(&m_stream, Z_NO_FLUSH);
				if ( not ( Z_OK == ret || Z_STREAM_END == ret ) )
				{
					//PWTRACE("inflate: %d", ret);
					::inflateEnd(&m_stream);
					m_init = false;
					return false;
				}
				assert(ret != Z_STREAM_ERROR);

				cplen = m_chunk_size - m_stream.avail_out;
				if ( cplen > 0 ) out.append(m_chunk, cplen);
			} while (m_stream.avail_out == 0);
		}

		return true;
	}

	bool finalize(std::string& out)
	{
		if ( not m_init ) return false;

		int ret(0);
		size_t cplen(0);
		m_stream.avail_in = 0;
		m_stream.next_in = nullptr;

		do {
			m_stream.avail_out = m_chunk_size;
			m_stream.next_out = m_chunk;
			ret = ::inflate(&m_stream, Z_FINISH);
			if ( not (Z_OK == ret || Z_STREAM_END == ret) )
			{
				::inflateEnd(&m_stream);
				m_init = false;
				return false;
			}

			cplen = m_chunk_size - m_stream.avail_out;
			if ( cplen > 0 ) out.append((char*)m_chunk, cplen);
		} while ( m_stream.avail_out == 0);

		::inflateEnd(&m_stream);
		m_init = false;

		return true;
	}

	bool finalize(std::ostream& out)
	{
		if ( not m_init ) return false;

		int ret(0);
		size_t cplen(0);
		m_stream.avail_in = 0;
		m_stream.next_in = nullptr;

		do {
			m_stream.avail_out = m_chunk_size;
			m_stream.next_out = m_chunk;
			ret = ::inflate(&m_stream, Z_FINISH);
			if ( not (Z_OK == ret || Z_STREAM_END == ret) )
			{
				::inflateEnd(&m_stream);
				m_init = false;
				return false;
			}

			cplen = m_chunk_size - m_stream.avail_out;
			if ( cplen > 0 ) out.write((char*)m_chunk, cplen);
		} while ( m_stream.avail_out == 0);

		::inflateEnd(&m_stream);
		m_init = false;

		return true;
	}

	bool finalize(blob_type& out)
	{
		if ( not m_init ) return false;

		int ret(0);
		size_t cplen(0);
		m_stream.avail_in = 0;
		m_stream.next_in = nullptr;

		do {
			m_stream.avail_out = m_chunk_size;
			m_stream.next_out = m_chunk;
			ret = ::inflate(&m_stream, Z_FINISH);
			if ( not (Z_OK == ret || Z_STREAM_END == ret) )
			{
				::inflateEnd(&m_stream);
				m_init = false;
				return false;
			}

			cplen = m_chunk_size - m_stream.avail_out;
			if ( cplen > 0 ) out.append(m_chunk, cplen);
		} while ( m_stream.avail_out == 0);

		::inflateEnd(&m_stream);
		m_init = false;

		return true;
	}

};

bool
Compress::s_initialize(void)
{
	if ( nullptr == g_comp ) if ( nullptr == (g_comp = Compress::s_createCompress(9, CHUNK_SIZE*8, false)) ) return false;
	if ( nullptr == g_uncomp ) if ( nullptr == (g_uncomp = Compress::s_createUncompress(CHUNK_SIZE*8, false)) ) return false;
	if ( nullptr == g_comp_gzip ) if ( nullptr == (g_comp_gzip = Compress::s_createCompress(9, CHUNK_SIZE*8, true)) ) return false;
	if ( nullptr == g_uncomp_gzip ) if ( nullptr == (g_uncomp_gzip = Compress::s_createUncompress(CHUNK_SIZE*8, true)) ) return false;
	return true;
}

const char*
Compress::s_getGZHeader(void)
{
	static constexpr char hdr[GZ_HEADER_SIZE] = { 0x1f, char(0x8b), Z_DEFLATED, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03 };
	return hdr;
}

Compress*
Compress::s_create(int ct, ...)
{
	va_list lst;
	va_start(lst, ct);

	Compress* ret(nullptr);

	if ( CT_COMPRESS == ct )
	{
		int level(va_arg(lst, int));
		size_t chunk_size(va_arg(lst, size_t));
		bool gzip(va_arg(lst, int));
		ret = s_createCompress(level, chunk_size, gzip);
	}
	else if ( CT_UNCOMPRESS == ct )
	{
		size_t chunk_size(va_arg(lst, size_t));
		bool gzip(va_arg(lst, int));
		ret = s_createUncompress(chunk_size, gzip);
	}

	va_end(lst);
	return ret;
}

Compress*
Compress::s_createCompress(int level, size_t chunk_size, bool gzip)
{
	if ( 0 == chunk_size ) chunk_size = CHUNK_SIZE;

	_pw_compress* obj(new _pw_compress(level, chunk_size, gzip));
	if ( nullptr == obj ) return nullptr;

	if ( obj->m_init ) return obj;
	delete obj;
	return nullptr;
}

Compress*
Compress::s_createUncompress(size_t chunk_size, bool gzip)
{
	if ( 0 == chunk_size ) chunk_size = CHUNK_SIZE;

	_pw_uncompress* obj(new _pw_uncompress(chunk_size, gzip));
	if ( nullptr == obj ) return nullptr;

	if ( obj->m_init ) return obj;
	delete obj;
	return nullptr;
}

bool
Compress::s_compress(std::string& out, const char* buf, size_t blen, int level, size_t chunk_size, bool gzip)
{
	Compress* comp(gzip ? g_comp_gzip : g_comp);
	if ( not comp->reinitialize(level, chunk_size) ) return false;
	if ( not comp->update(out, buf, blen) ) return false;
	if ( not comp->finalize(out) ) return false;
	return true;
}

bool
Compress::s_compress(blob_type& out, const char* buf, size_t blen, int level, size_t chunk_size, bool gzip)
{
	Compress* comp(gzip ? g_comp_gzip : g_comp);
	if ( not comp->reinitialize(level, chunk_size) ) return false;
	if ( not comp->update(out, buf, blen) ) return false;
	if ( not comp->finalize(out) ) return false;
	return true;
}

bool
Compress::s_compress(blob_type& inout, int level, size_t chunk_size, bool gzip)
{
	Compress* comp(gzip ? g_comp_gzip : g_comp);
	blob_type out;
	if ( not comp->reinitialize(level, chunk_size) ) return false;
	if ( not comp->update(out, inout.buf, inout.size) ) return false;
	if ( not comp->finalize(out) ) return false;
	inout.swap(out);
	return true;
}

bool
Compress::s_uncompress(std::string& out, const char* buf, size_t blen, size_t chunk_size, bool gzip)
{
	Compress* comp(gzip ? g_uncomp_gzip : g_uncomp);
	if ( not comp->reinitialize(chunk_size) ) return false;
	if ( not comp->update(out, buf, blen) ) return false;
	if ( not comp->finalize(out) ) return false;
	return true;
}

bool
Compress::s_uncompress(blob_type& inout, size_t chunk_size, bool gzip)
{
	Compress* comp(gzip ? g_uncomp_gzip : g_uncomp);
	blob_type out;
	if ( not comp->reinitialize(chunk_size) ) return false;
	if ( not comp->update(out, inout.buf, inout.size) ) return false;
	if ( not comp->finalize(out) ) return false;
	inout.swap(out);
	return true;
}

bool
Compress::s_uncompress(blob_type& out, const char* buf, size_t blen, size_t chunk_size, bool gzip)
{
	Compress* comp(gzip ? g_uncomp_gzip : g_uncomp);
	if ( not comp->reinitialize(chunk_size) ) return false;
	if ( not comp->update(out, buf, blen) ) return false;
	if ( not comp->finalize(out) ) return false;
	return true;
}

};//namespace pw
