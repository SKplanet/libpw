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
 * \file pw_crypto.h
 * \brief Support crypto library with openssl(http://openssl.org/).
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"

#ifndef __PW_CRYPTO_H__
#define __PW_CRYPTO_H__

namespace pw
{

namespace crypto
{
//! \brief 암호화 알고리즘 타입
enum class CipherType : int
{
	INVALID = -1,
	EMPTY = 0,

	DES_EDE3,
	DES_EDE3_ECB,
	DES_EDE3_CFB64,
	DES_EDE3_CFB1,
	DES_EDE3_CFB8,
	DES_EDE3_OFB,
	DES_EDE3_CBC,

	AES_128_ECB,
	AES_128_CBC,
	AES_128_CFB1,
	AES_128_CFB8,
	AES_128_CFB128,
	AES_128_OFB,
	AES_128_CTR,
	AES_128_CCM,
	AES_128_GCM,
	AES_128_XTS,
	AES_128_WRAP,

	AES_192_ECB,
	AES_192_CBC,
	AES_192_CFB1,
	AES_192_CFB8,
	AES_192_CFB128,
	AES_192_OFB,
	AES_192_CTR,
	AES_192_CCM,
	AES_192_GCM,
	AES_192_XTS,
	AES_192_WRAP,

	AES_256_ECB,
	AES_256_CBC,
	AES_256_CFB1,
	AES_256_CFB8,
	AES_256_CFB128,
	AES_256_OFB,
	AES_256_CTR,
	AES_256_CCM,
	AES_256_GCM,
	AES_256_XTS,
	AES_256_WRAP,

	AES_128_CBC_HMAC_SHA1,
	AES_256_CBC_HMAC_SHA1,
	AES_128_CBC_HMAC_SHA256,
	AES_256_CBC_HMAC_SHA256,
};

enum class Direction : int
{
	ENCRYPT = 1,
	DECRYPT = 0,
};

struct cipher_spec final
{
	using engine_type = const void* ;

	engine_type engine = nullptr;
	int nid = -1;
	CipherType cipher = CipherType::INVALID;

	std::string name_long;
	std::string name_short;

	struct size_type final
	{
		size_t block = 0;
		size_t key = 0;
		size_t iv = 0;

		inline size_type() = default;
		inline size_type ( size_t in_block, size_t in_key, size_t in_iv ) : block ( in_block ), key ( in_key ), iv ( in_iv ) {}
	} size;

	inline size_t getEncryptedSize ( size_t insize )
	{
		if ( size.block > 1 ) return ( size.block > 1 ) ? ( insize + size.block - ( insize % size.block ) ) : insize;
	}

	std::ostream& dump ( std::ostream& os ) const;
};

extern bool initialize ( void );

//! \brief OpenSSL 관련하여 멀티스레드 락 설정
extern bool initializeLocks ( void );

extern int toNID ( CipherType in );
extern CipherType toCipherType ( int nid );
extern bool getCipherSpec ( cipher_spec& out, CipherType in );
extern bool getCipherSpecByNID ( cipher_spec& out, int nid );

//namespace crypto
}

class Crypto
{
public:
	//! \brief 암호화/복호화 타입.
	using Direction = crypto::Direction;

public:
	static void s_release ( Crypto* v )
	{
		delete v;
	}
	static Crypto* s_create ( crypto::CipherType ct, const char* key, const char* iv, Direction dt );
	static Crypto* s_create ( const char* name, const char* key, const char* iv, Direction dt );
	static Crypto* s_createByNID ( int nid, const char* key, const char* iv, Direction dt );

	virtual ~Crypto();

public:
	bool reinitialize ( void );

	int getNID ( void ) const;
	size_t getBlockSize ( void ) const;
	size_t getKeySize ( void ) const;
	size_t getIVSize ( void ) const;

	//! \brief 객체 반환하기.
	inline void release ( void )
	{
		delete this;
	}

	//! \brief 암호화할 시 필요한 사이즈를 구한다.
	size_t getEncryptedSize ( size_t insize ) const;

public:
	//! \brief Cipher 업데이트.
	bool update ( std::string& out, const char* in, size_t ilen );

	//! \brief Cipher 업데이트.
	bool update ( char* out, size_t* olen, const char* in, size_t ilen );

	//! \brief Cipher 업데이트.
	inline bool update ( std::string& out, const std::string& in )
	{
		return this->update ( out, in.c_str(), in.size() );
	}

	//! \brief Cipher 업데이트.
	inline bool update ( char* out, size_t* olen, const std::string& in )
	{
		return this->update ( out, olen, in.c_str(), in.size() );
	}

	//! \brief Cipher 업데이트.
	size_t update ( std::ostream& os, const char* in, size_t blen );

	//! \brief Cipher 업데이트.
	inline size_t update ( std::ostream& os, const std::string& in )
	{
		return update ( os, in.c_str(), in.size() );
	}

	//! \brief Cipher 정리.
	//! \warning 호출 이후엔 반드시 reinitialize 해야한다.
	bool finalize ( std::string& out );

	//! \brief Cipher 정리.
	//! \warning 호출 이후엔 반드시 reinitialize 해야한다.
	bool finalize ( char* out, size_t* olen );

	//! \brief Cipher 정리.
	//! \warning 호출 이후엔 반드시 reinitialize 해야한다.
	size_t finalize ( std::ostream& os );

	bool execute ( std::string& out, const char* in, size_t ilen );
	inline bool execute ( std::string& out, const std::string& in )
	{
		return execute ( out, in.c_str(), in.size() );
	}

	bool execute ( char* out, size_t* olen, const char* in, size_t ilen );
	inline bool execute ( char* out, size_t* olen, const std::string& in )
	{
		return execute ( out, olen, in.c_str(), in.size() );
	}

	size_t execute ( std::ostream& os, const char* in, size_t ilen );
	inline size_t execute ( std::ostream& os, const std::string& in )
	{
		return execute ( os, in.c_str(), in.size() );
	}

private:
	inline explicit Crypto() = default;

private:
	void* m_ctx = nullptr;
	const void*	m_cipher = nullptr;
	const void* m_key = nullptr;
	const void* m_iv = nullptr;
};

}; //namespace pw

namespace std
{

inline
ostream&
operator << ( ostream& os, const pw::crypto::cipher_spec& in )
{
	return in.dump ( os );
}

};

#endif//__PW_CRYPTO_H__

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4;
