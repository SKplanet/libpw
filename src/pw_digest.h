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
 * \file pw_digest.h
 * \brief Support message digest with openssl(http://openssl.org/).
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"

#ifndef __PW_HASH_H__
#define __PW_HASH_H__

namespace pw
{

namespace digest
{
//! \brief 해쉬 타입
enum class DigestType : int
{
 	INVALID = -1,
	MD_NULL,
	MD2,
	MD5,
	SHA,
	SHA1,
	SHA224,
	SHA256,
	SHA384,
	SHA512,
	DSS,
	DSS1,
	MDC2,
	RIPEMD160,
};

//! \brief 라이브러리 초기화.
bool initialize(void);

//! \brief 해쉬 알고리즘 객체
const void* getAlg(DigestType hashType);

//namespace digest
}

#if (HAVE_EVP_MD_CTX > 0 )
//! \brief 해시 클래스
class Digest final
{
public:
	enum
	{
		MAX_HASH_SIZE = (16+20),	//!< 지원하는 최대 해시 크기
	};

public:
	//! \brief 객체 생성하기.
	static Digest* s_create(digest::DigestType type);

	//! \brief 객체 생성하기.
	static Digest* s_create(const char* type_name);

	//! \brief 객체 반환하기.
	inline static void s_release(Digest* v) { delete v; }

	//! \brief 해시 결과 크기.
	static size_t s_getHashSize(digest::DigestType type);

	//! \brief 간편한 해시.
	static bool s_execute(void* out, const void* in, size_t inlen, digest::DigestType type, size_t* outlen = nullptr);
	inline static bool s_execute(void* out, const std::string& in, digest::DigestType type, size_t* outlen = nullptr) { return s_execute(out, in.c_str(), in.size(), type, outlen); }
	static bool s_execute(std::string& out, const void* in, size_t inlen, digest::DigestType type);
	inline static bool s_execute(std::string& out, const std::string& in, digest::DigestType type) { return s_execute(out, in.c_str(), in.size(), type); }
	inline static bool s_execute(std::string& inout, digest::DigestType type) { return s_execute(inout, inout.c_str(), inout.size(), type); }

public:
	//! \brief 객체 반환하기.
	inline void release(void) { delete this; }

#if 0
	//! \brief 객체 교환하기.
	void swap(Hash& v);
#endif

	//! \brief 해시 결과 크기.
	size_t getHashSize(void) const;

	//! \brief 객체 다시 쓰기.
	bool reinitialize(void);

	//! \brief 객체 다시 쓰기.
	bool reinitialize(digest::DigestType type);

	//! \brief 객체 다시 쓰기.
	bool reinitialize(const char* type_name);

	//! \brief 객체 다시 쓰기
	bool reinitializeByNID(int nid);

	//! \warning 호출 이후엔 반드시 reinitialize 해야한다.
	inline bool execute(void* out, const void* in, size_t ilen)
	{
		if ( not reinitialize() ) return false;
		if ( not update(in, ilen) ) return false;
		return finalize(out, nullptr);
	}

	//! \warning 호출 이후엔 반드시 reinitialize 해야한다.
	inline bool execute(void* out, const std::string& in)
	{
		return execute(out, in.c_str(), in.size());
	}

	//! \warning 호출 이후엔 반드시 reinitialize 해야한다.
	inline bool execute(std::string& out, const void* in, size_t ilen)
	{
		if ( not reinitialize() ) return false;
		if ( not update(in, ilen) ) return false;
		return finalize(out);
	}

	//! \warning 호출 이후엔 반드시 reinitialize 해야한다.
	inline bool execute(std::string& out, const std::string& in)
	{
		return execute(out, in.c_str(), in.size());
	}

	//! \brief 해시 업데이트.
	bool update(const void* in, size_t ilen);

	//! \brief 해시 업데이트.
	inline bool update(const std::string& in) { return this->update(in.c_str(), in.size()); }

	//! \brief 해시 결과.
	//! \warning 호출 이후엔 반드시 reinitialize 해야한다.
	bool finalize(void* out, size_t* olen);

	//! \brief 해시 결과.
	//! \warning 호출 이후엔 반드시 reinitialize 해야한다.
	bool finalize(std::string& out)
	{
		char tmp[MAX_HASH_SIZE];
		size_t olen(0);
		if ( this->finalize(tmp, &olen) )
		{
			out.assign(tmp, olen);
			return true;
		}
		return false;
	}

	//! \brief 해시 결과.
	//! \warning 호출 이후엔 반드시 reinitialize 해야한다.
	bool finalize(std::ostream& os, size_t* olen)
	{
		char tmp[MAX_HASH_SIZE];
		size_t tmp_len(0);
		if ( this->finalize(tmp, &tmp_len) )
		{
			os.write(tmp, tmp_len);
			if ( olen ) *olen = tmp_len;
			return true;
		}
		return false;
	}

private:
	Digest();
	~Digest();

private:
	bool _init(void);
	inline bool _init(const void* type) { m_type = type; return this->_init(); }

private:
	char		m_hash[HAVE_EVP_MD_CTX];
	const void*	m_type;
};

#endif//HAVE_EVP_MD_CTX > 0

}; //namespace pw

#endif//__PW_HASH_H__

