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
 * \file pw_compress.h
 * \brief Support compression with zlib(http://www.zlib.net/).
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"

#ifndef __PW_COMPRESS_H__
#define __PW_COMPRESS_H__

namespace pw {

class Compress
{
public:
	enum
	{
		GZ_HEADER_SIZE = 10,	//!< GZ 헤더 크기
		CHUNK_SIZE = 1024,		//!< 기본 청크 크기
	};

	typedef enum compress_type
	{
		CT_COMPRESS,	//!< 압축
		CT_UNCOMPRESS,	//!< 압축 해제
	} compress_type;

public:
	static bool s_initialize(void);

	//! \brief 기본 GZ 헤더 문자 배열.
	static const char* s_getGZHeader(void);

	//! \brief 생성하기.
	static Compress* s_create(compress_type ct, ...);

	//! \brief 압축객체 생성하기.
	static Compress* s_createCompress(int level = 9, size_t chunk_size = CHUNK_SIZE, bool gzip = false);

	//! \brief 해제객체 생성하기.
	static Compress* s_createUncompress(size_t chunk_size = CHUNK_SIZE, bool gzip = false);

	//! \brief 객체 반환.
	inline static void s_release(Compress* v) { delete v; }

	//! \brief 압축하기.
	static bool s_compress(std::string& out, const char* buf, size_t blen, int level = 9, size_t chunk_size = CHUNK_SIZE, bool gzip = false);
	inline static bool s_compress(std::string& out, const std::string& in, int level = 9, size_t chunk_size = CHUNK_SIZE, bool gzip = false) { return s_compress(out, in.c_str(), in.size(), level, chunk_size, gzip); }
	static bool s_compress(blob_type& out, const char* buf, size_t blen, int level = 9, size_t chunk_size = CHUNK_SIZE, bool gzip = false);
	inline static bool s_compress(blob_type& out, const std::string& in, int level = 9, size_t chunk_size = CHUNK_SIZE, bool gzip = false) { return s_compress(out, in.c_str(), in.size(), level, chunk_size, gzip); }
	inline static bool s_compress(blob_type& out, const blob_type& in, int level = 9, size_t chunk_size = CHUNK_SIZE, bool gzip = false) { return s_compress(out, in.buf, in.size, level, chunk_size, gzip); }
	static bool s_compress(blob_type& inout, int level = 9, size_t chunk_size = CHUNK_SIZE, bool gzip = false);

	//! \brief 해제하기.
	static bool s_uncompress(std::string& out, const char* buf, size_t blen, size_t chunk_size = CHUNK_SIZE, bool gzip = false);
	inline static bool s_uncompress(std::string& out, const std::string& in, size_t chunk_size = CHUNK_SIZE, bool gzip = false) { return s_uncompress(out, in.c_str(), in.size(), chunk_size, gzip); }
	static bool s_uncompress(blob_type& out, const char* buf, size_t blen, size_t chunk_size = CHUNK_SIZE, bool gzip = false);
	inline static bool s_uncompress(blob_type& out, const std::string& in, size_t chunk_size = CHUNK_SIZE, bool gzip = false) { return s_uncompress(out, in.c_str(), in.size(), chunk_size, gzip); }
	inline static bool s_uncompress(blob_type& out, const blob_type& in, size_t chunk_size = CHUNK_SIZE, bool gzip = false) { return s_uncompress(out, in.buf, in.size, chunk_size, gzip); }
	static bool s_uncompress(blob_type& inout, size_t chunk_size = CHUNK_SIZE, bool gzip = false);

public:
	//! \brief 객체 타입 반환.
	virtual compress_type getCompressType(void) const = 0;

public:
	//! \brief 객체 반환하기.
	inline void release(void) { delete this; }

	//! \brief 객체 다시 사용하기.
	virtual bool reinitialize(void) = 0;

	//! \brief 객체 다시 사용하기.
	virtual bool reinitialize(int level, size_t chunk_size) = 0;

	//! \brief 객체 다시 사용하기.
	virtual bool reinitialize(size_t chunk_size) = 0;

	//! \brief 스트림 추가.
	//! \warning 실패할 경우, 객체를 다시 사용할 수 없다.
	//!	다시 사용해야할 경우, reinitialize를 통해 다시 사용할 수 있다.
	virtual bool update(std::string& out, const char* in, size_t ilen) = 0;
	inline bool update(std::string& out, const std::string& in) { return update(out, in.c_str(), in.size()); }
	virtual bool update(std::ostream& out, const char* in, size_t ilen) = 0;
	inline bool update(std::ostream& out, const std::string& in) { return update(out, in.c_str(), in.size()); }
	virtual bool update(blob_type& out, const char* in, size_t ilen) = 0;
	inline bool update(blob_type& out, const std::string& in) { return update(out, in.c_str(), in.size()); }
	inline bool update(blob_type& out, const blob_type& in) { return update(out, in.buf, in.size); }

	//! \brief 스트림 정리.
	//! \warning 성공 여부와 상관 없이, 객체를 다시 사용할 수 없다.
	//!	다시 사용해야할 경우, reinitialize를 통해 다시 사용할 수 있다.
	virtual bool finalize(std::string& out) = 0;
	virtual bool finalize(std::ostream& out) = 0;
	virtual bool finalize(blob_type& out) = 0;

protected:
	inline explicit Compress(size_t chunk_size) : m_chunk_size(chunk_size), m_chunk((unsigned char*)::malloc(chunk_size)) {}
	inline virtual ~Compress() { if ( m_chunk ) { ::free(m_chunk); m_chunk = nullptr; } }

protected:
	const size_t	m_chunk_size;
	unsigned char*	m_chunk;
};

};//namespace pw

#endif//__PW_COMPRESS_H__
