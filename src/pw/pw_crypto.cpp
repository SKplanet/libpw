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
 * \file pw_crypto.cpp
 * \brief Support crypto library with openssl(http://openssl.org/).
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_log.h"
#include "./pw_crypto.h"
#include "./pw_string.h"

#define OPENSSL_THREAD_DEFINES
#include <openssl/opensslconf.h>
#include <openssl/evp.h>

namespace pw
{

#define THIS()		static_cast<EVP_CIPHER_CTX*>(this->m_ctx)
#define THIS_TYPE()	static_cast<const EVP_CIPHER*>(m_cipher)
#define SCTX(ptr)	static_cast<EVP_CIPHER_CTX*>((ptr)->m_ctx)

static striptr_cont	s_str2cipher;
static intint_cont s_nid2pw;
static intint_cont s_pw2nid;

static std::mutex*	s_ssl_locks ( nullptr );

struct cipher_create_param_type
{
	struct
	{
		const EVP_CIPHER* cipher = nullptr;
		const void* key = nullptr;
		const void* iv = nullptr;
		crypto::Direction dt;
	} in;

	struct
	{
		EVP_CIPHER_CTX* ctx = nullptr;
		void* key = nullptr;
		void* iv = nullptr;
	} out;
};

inline
static
bool
_createCipher ( cipher_create_param_type& inout )
{
	auto& in ( inout.in );

	if ( not in.cipher ) return false;

	auto key_size ( EVP_CIPHER_key_length ( in.cipher ) );
	auto iv_size ( EVP_CIPHER_iv_length ( in.cipher ) );

	if ( (key_size and not in.key) or ( iv_size and not in.iv ) )
	{
		return false;
	}

	EVP_CIPHER_CTX* ctx ( nullptr );
	unsigned char* key ( nullptr );
	unsigned char* iv ( nullptr );

	do
	{
		if ( nullptr == ( ctx = EVP_CIPHER_CTX_new() ) ) break;

		if ( key_size )
		{
			if ( nullptr == ( key = ( unsigned char* ) malloc ( key_size ) ) ) break;

			if ( nullptr == ( iv = ( unsigned char* ) malloc ( key_size ) ) ) break;

			memcpy ( key, in.key, key_size );
			memcpy ( iv, in.iv, key_size );
		}

		if ( not EVP_CipherInit_ex ( ctx, in.cipher, nullptr, key, iv, static_cast<int> ( in.dt ) ) )
		{
			PWTRACE ( "EVP_CipherInit_ex failed" );
			break;
		}

		auto& out ( inout.out );
		out.ctx = ctx;

		if ( iv ) out.iv = iv;

		if ( key ) out.key = key;

		return true;
	}
	while ( false );

	if ( ctx ) EVP_CIPHER_CTX_free ( ctx );

	if ( key )
	{
		memset(key, 0x00, key_size);
		free ( key );
	}

	if ( iv )
	{
		memset(iv, 0x00, iv_size);
		free ( iv );
	}

	return false;
}

// OpenSSL용 락제어함수
inline
static
void
_locking_function ( int mode, int type, const char* file, int line )
{
	if ( mode bitand CRYPTO_LOCK ) s_ssl_locks[type].lock();
	else s_ssl_locks[type].unlock();
}

inline
unsigned long
_threadid_function ( void )
{
	return static_cast<unsigned long> ( std::hash<std::thread::id>() ( std::this_thread::get_id() ) );
}

inline
static
const ::EVP_CIPHER*
_getCipherByNID ( int nid )
{
	return EVP_get_cipherbynid ( nid );
}

inline
static
const ::EVP_CIPHER*
_getCipherByName ( const std::string& name )
{
	auto ib ( s_str2cipher.find ( name ) );

	if ( s_str2cipher.end() == ib ) return ::EVP_get_cipherbyname ( name.c_str() );

	//std::cout << "name: " << name << " " << ib->second << std::endl;
	return reinterpret_cast<const ::EVP_CIPHER*> ( ib->second );
}

inline
static
const ::EVP_CIPHER*
_getCipherByPW ( crypto::CipherType ct )
{
	auto ib ( s_pw2nid.find ( static_cast<int> ( ct ) ) );

	if ( ib == s_pw2nid.end() ) return nullptr;

	return _getCipherByNID ( ib->second );
}

namespace crypto
{

bool
initializeLocks ( void )
{
	if ( nullptr == s_ssl_locks )
	{
		if ( nullptr == ( s_ssl_locks = new std::mutex[CRYPTO_num_locks()] ) )
		{
			PWLOGLIB ( "failed to initialize openssl lock" );
			return false;
		}

		CRYPTO_set_id_callback ( _threadid_function );
		CRYPTO_set_locking_callback ( _locking_function );
	}

	return true;
}

static
inline
void
_setCipher ( const EVP_CIPHER* e, CipherType ct )
{
	const auto nid ( EVP_CIPHER_nid ( e ) );
	auto sn ( OBJ_nid2sn ( nid ) );

	if ( 0 == strcmp ( sn, "UNDEF" ) )
	{
		if ( nid == 0 ) sn = "NULL";
		else throw ( std::runtime_error ( "undefined cipher" ) );
	}

	//PWTRACE("engine:%p nid:%d ct:%d sn:%s ln:%s", e, nid, ct, sn, OBJ_nid2ln(nid));
	auto ptr ( ( void* ) const_cast<EVP_CIPHER*> ( e ) );
	s_str2cipher[sn] = ptr;
	s_nid2pw[nid] = static_cast<int> ( ct );
	s_pw2nid[static_cast<int> ( ct )] = nid;
}

bool
initialize ( void )
{
	static std::atomic<bool> s_init ( false );

	if ( s_init ) return true;

	OpenSSL_add_all_algorithms();

	initializeLocks();

	_setCipher ( EVP_enc_null(), CipherType::EMPTY );
	_setCipher ( EVP_des_ede3_ecb(), CipherType::DES_EDE3_ECB );
	_setCipher ( EVP_des_ede3_cfb64(), CipherType::DES_EDE3_CFB64 );
	_setCipher ( EVP_des_ede3_cfb1(), CipherType::DES_EDE3_CFB1 );
	_setCipher ( EVP_des_ede3_cfb8(), CipherType::DES_EDE3_CFB8 );
	_setCipher ( EVP_des_ede3_cbc(), CipherType::DES_EDE3_CBC );
	_setCipher ( EVP_aes_128_ecb(), CipherType::AES_128_ECB );
	_setCipher ( EVP_aes_128_cbc(), CipherType::AES_128_CBC );
	_setCipher ( EVP_aes_128_cfb1(), CipherType::AES_128_CFB1 );
	_setCipher ( EVP_aes_128_cfb8(), CipherType::AES_128_CFB8 );
	_setCipher ( EVP_aes_128_cfb128(), CipherType::AES_128_CFB128 );
	_setCipher ( EVP_aes_192_ecb(), CipherType::AES_192_ECB );
	_setCipher ( EVP_aes_192_cbc(), CipherType::AES_192_CBC );
	_setCipher ( EVP_aes_192_cfb1(), CipherType::AES_192_CFB1 );
	_setCipher ( EVP_aes_192_cfb8(), CipherType::AES_192_CFB8 );
	_setCipher ( EVP_aes_192_cfb128(), CipherType::AES_192_CFB128 );
	_setCipher ( EVP_aes_256_ecb(), CipherType::AES_256_ECB );
	_setCipher ( EVP_aes_256_cbc(), CipherType::AES_256_CBC );
	_setCipher ( EVP_aes_256_cfb1(), CipherType::AES_256_CFB1 );
	_setCipher ( EVP_aes_256_cfb8(), CipherType::AES_256_CFB8 );
	_setCipher ( EVP_aes_256_cfb128(), CipherType::AES_256_CFB128 );
	_setCipher ( EVP_des_ede3(), CipherType::DES_EDE3 );
	_setCipher ( EVP_des_ede3_ofb(), CipherType::DES_EDE3_OFB );
	_setCipher ( EVP_aes_128_ofb(), CipherType::AES_128_OFB );
	_setCipher ( EVP_aes_192_ofb(), CipherType::AES_192_OFB );
	_setCipher ( EVP_aes_256_ofb(), CipherType::AES_256_OFB );

#ifdef HAVE_EVP_AES_256_CTR
	_setCipher ( EVP_aes_128_ctr(), CipherType::AES_128_CTR );
	_setCipher ( EVP_aes_192_ctr(), CipherType::AES_192_CTR );
	_setCipher ( EVP_aes_256_ctr(), CipherType::AES_256_CTR );
#endif

#ifdef HAVE_EVP_AES_256_CCM
	_setCipher ( EVP_aes_128_ccm(), CipherType::AES_128_CCM );
	_setCipher ( EVP_aes_192_ccm(), CipherType::AES_192_CCM );
	_setCipher ( EVP_aes_256_ccm(), CipherType::AES_256_CCM );
#endif

#ifdef HAVE_EVP_AES_256_GCM
	_setCipher ( EVP_aes_128_gcm(), CipherType::AES_128_GCM );
	_setCipher ( EVP_aes_192_gcm(), CipherType::AES_192_GCM );
	_setCipher ( EVP_aes_256_gcm(), CipherType::AES_256_GCM );
#endif

#ifdef HAVE_EVP_AES_256_WRAP
	_setCipher ( EVP_aes_128_wrap(), CipherType::AES_128_WRAP );
	_setCipher ( EVP_aes_192_wrap(), CipherType::AES_192_WRAP );
	_setCipher ( EVP_aes_256_wrap(), CipherType::AES_256_WRAP );
#endif

#ifdef HAVE_EVP_AES_256_XTS
	_setCipher ( EVP_aes_128_xts(), CipherType::AES_128_XTS );
#ifdef HAVE_EVP_AES_192_XTS
	_setCipher ( EVP_aes_192_xts(), CipherType::AES_192_XTS );
#endif
	_setCipher ( EVP_aes_256_xts(), CipherType::AES_256_XTS );
#endif

#ifdef HAVE_EVP_AES_256_CBC_HMAC_SHA256
	_setCipher ( EVP_aes_128_cbc_hmac_sha1(), CipherType::AES_128_CBC_HMAC_SHA1 );
	_setCipher ( EVP_aes_256_cbc_hmac_sha1(), CipherType::AES_256_CBC_HMAC_SHA1 );
	_setCipher ( EVP_aes_128_cbc_hmac_sha256(), CipherType::AES_128_CBC_HMAC_SHA256 );
	_setCipher ( EVP_aes_256_cbc_hmac_sha256(), CipherType::AES_256_CBC_HMAC_SHA256 );
#endif

	s_init = true;
	return true;
}

int
toNID ( CipherType in )
{
	auto e ( _getCipherByPW ( in ) );

	if ( e == nullptr ) return -1;

	return EVP_CIPHER_nid ( e );
}

CipherType
toCipherType ( int nid )
{
	auto ib ( s_nid2pw.find ( nid ) );

	if ( ib == s_nid2pw.end() )
	{
		return CipherType::INVALID;
	}

	return static_cast<CipherType> ( ib->second );
}

bool
getCipherSpec ( cipher_spec& out, CipherType in )
{
	auto e ( _getCipherByPW ( in ) );

	if ( e == nullptr ) return false;

	out.engine = e;
	auto nid = out.nid = EVP_CIPHER_nid ( e );

	if ( nid )
	{
		out.name_long = OBJ_nid2ln ( nid );
		out.name_short = OBJ_nid2sn ( nid );
	}
	else
	{
		out.name_long = "null";
		out.name_short = "NULL";
	}

	out.cipher = in;
	out.size.block = EVP_CIPHER_block_size ( e );
	out.size.key = EVP_CIPHER_key_length ( e );
	out.size.iv = EVP_CIPHER_iv_length ( e );

	return true;
}

bool
getCipherSpecByNID ( cipher_spec& out, int nid )
{
	auto e ( _getCipherByNID ( nid ) );

	if ( e == nullptr ) return false;

	out.engine = e;

	if ( ( out.nid = nid ) not_eq 0 )
	{
		out.name_long = OBJ_nid2ln ( nid );
		out.name_short = OBJ_nid2sn ( nid );
	}
	else
	{
		out.name_long = "null";
		out.name_short = "NULL";
	}

	out.cipher = toCipherType ( nid );
	out.size.block = EVP_CIPHER_block_size ( e );
	out.size.key = EVP_CIPHER_key_length ( e );
	out.size.iv = EVP_CIPHER_iv_length ( e );

	return true;
}

std::ostream&
cipher_spec::dump ( std::ostream& os ) const
{
	return os << "nid: " << nid
		   << " long name: " << name_long
		   << " short name: " << name_short
		   << " block size: " << size.block
		   << " key length: " << size.key
		   << " iv length: " << size.iv;
}

//namespace crypto
}

Crypto::~Crypto()
{
	size_t key_size(0), iv_size(0);
	if ( m_ctx )
	{
		key_size = getKeySize();
		iv_size = getIVSize();

		auto p ( THIS() );
		EVP_CIPHER_CTX_free ( p );
		m_ctx = nullptr;
	}

	if ( m_key )
	{
		auto m(const_cast<void*>(m_key));
		m_key = nullptr;

		if ( key_size ) memset(m, 0x00, key_size);
		free ( m );
	}

	if ( m_iv )
	{
		auto m(const_cast<void*>(m_iv));
		m_iv = nullptr;

		if ( iv_size ) memset(m, 0x00, iv_size);
		free (m);
	}
}

Crypto*
Crypto::s_create ( crypto::CipherType ct, const char* key, const char* iv, Direction dt )
{
	auto cipher ( _getCipherByPW ( ct ) );

	if ( nullptr == cipher ) return nullptr;

	Crypto* ret ( new Crypto() );

	if ( nullptr == ret ) return nullptr;

	cipher_create_param_type param;
	auto& in ( param.in );
	in.cipher = cipher;

	if ( key ) in.key = key;

	if ( iv ) in.iv = iv;

	in.dt = dt;

	if ( not _createCipher ( param ) )
	{
		delete ret;
		return nullptr;
	}

	auto& out ( param.out );
	ret->m_cipher = cipher;
	ret->m_ctx = out.ctx;

	if ( out.iv ) ret->m_iv = out.iv;

	if ( out.key ) ret->m_key = out.key;

	return ret;
}

Crypto*
Crypto::s_create ( const char* name, const char* key, const char* iv, Direction dt )
{
	auto cipher ( _getCipherByName ( name ) );

	if ( nullptr == cipher ) return nullptr;

	Crypto* ret ( new Crypto() );

	if ( nullptr == ret ) return nullptr;

	cipher_create_param_type param;
	auto& in ( param.in );
	in.cipher = cipher;

	if ( key ) in.key = key;

	if ( iv ) in.iv = iv;

	in.dt = dt;

	if ( not _createCipher ( param ) )
	{
		delete ret;
		return nullptr;
	}

	auto& out ( param.out );
	ret->m_cipher = cipher;
	ret->m_ctx = out.ctx;

	if ( out.iv ) ret->m_iv = out.iv;

	if ( out.key ) ret->m_key = out.key;

	return ret;
}

Crypto*
Crypto::s_createByNID ( int nid, const char* key, const char* iv, Direction dt )
{
	auto cipher ( _getCipherByNID ( nid ) );

	if ( nullptr == cipher ) return nullptr;

	Crypto* ret ( new Crypto() );

	if ( nullptr == ret ) return nullptr;

	cipher_create_param_type param;
	auto& in ( param.in );
	in.cipher = cipher;

	if ( key ) in.key = key;

	if ( iv ) in.iv = iv;

	in.dt = dt;

	if ( not _createCipher ( param ) )
	{
		delete ret;
		return nullptr;
	}

	auto& out ( param.out );
	ret->m_cipher = cipher;
	ret->m_ctx = out.ctx;

	if ( out.iv ) ret->m_iv = out.iv;

	if ( out.key ) ret->m_key = out.key;

	return ret;
}

int
Crypto::getNID ( void ) const
{
	return EVP_CIPHER_nid ( THIS_TYPE() );
}

size_t
Crypto::getBlockSize ( void ) const
{
	return EVP_CIPHER_block_size ( THIS_TYPE() );
}

size_t
Crypto::getIVSize ( void ) const
{
	return EVP_CIPHER_iv_length ( THIS_TYPE() );
}

size_t
Crypto::getKeySize ( void ) const
{
	return EVP_CIPHER_key_length ( THIS_TYPE() );
}

size_t
Crypto::getEncryptedSize ( size_t insize ) const
{
	const size_t bsize ( EVP_CIPHER_block_size ( THIS_TYPE() ) );
	return ( bsize > 1 ) ? ( insize + bsize - ( insize % bsize ) ) : insize;
}

bool
Crypto::update ( std::string& out, const char* in, size_t ilen )
{
	int buflen ( ilen + getBlockSize() );
	std::string tmp ( buflen, 0x00 );
	auto buf ( const_cast<char*> ( tmp.c_str() ) );

	if ( EVP_CipherUpdate ( THIS(), ( unsigned char* ) buf, &buflen, ( unsigned char* ) in, ilen ) )
	{
		tmp.resize ( buflen );
		out.swap ( tmp );
		return true;
	}

	return false;
}

bool
Crypto::update ( char* out, size_t* olen, const char* in, size_t ilen )
{
	int _olen ( olen ? *olen : ( ilen + getBlockSize() ) );

	if ( 1 == ::EVP_CipherUpdate ( THIS(), ( unsigned char* ) out, &_olen, ( unsigned char* ) in, ilen ) )
	{
		if ( olen ) *olen = _olen;

		return true;
	}

	return false;
}

size_t
Crypto::update ( std::ostream& os, const char* in, size_t blen )
{
	char obuf[PWStr::DEFAULT_BUFFER_SIZE];
	int _olen ( 0 );
	size_t sum ( 0 );

	size_t left_len ( blen ), cplen;
	size_t min_size ( PWStr::DEFAULT_BUFFER_SIZE - EVP_CIPHER_block_size ( THIS_TYPE() ) );
	const unsigned char* in_ptr ( reinterpret_cast<const unsigned char*> ( in ) );
	unsigned char* out_ptr ( reinterpret_cast<unsigned char*> ( obuf ) );

	while ( left_len )
	{
		cplen = std::min ( min_size, left_len );

		if ( 1 not_eq ::EVP_CipherUpdate ( THIS(), out_ptr, &_olen, in_ptr, cplen ) )
		{
			return sum;
		}

		if ( _olen > 0 )
		{
			if ( os.write ( obuf, _olen ).fail() )
			{
				return sum;
			}
		}

		in_ptr += cplen;
		left_len -= cplen;
		sum += cplen;
	}

	return sum;
}

bool
Crypto::finalize ( std::string& out )
{
	int buflen ( getBlockSize() + 32 );
	std::string tmp ( buflen, 0x00 );
	auto buf ( const_cast<char*> ( tmp.c_str() ) );

	if ( EVP_CipherFinal_ex ( THIS(), ( unsigned char* ) buf, &buflen ) )
	{
		tmp.resize ( buflen );
		out.swap ( tmp );
		return true;
	}

	return false;
}

bool
Crypto::finalize ( char* out, size_t* olen )
{
	int _olen ( olen ? *olen : 0 );

	if ( EVP_CipherFinal_ex ( THIS(), ( unsigned char* ) out, &_olen ) )
	{
		if ( olen ) *olen = _olen;

		return true;
	}

	return false;
}

size_t
Crypto::finalize ( std::ostream& os )
{
	int buflen ( getBlockSize() + 32 );
	unsigned char buf[buflen];

	if ( EVP_CipherFinal_ex ( THIS(), buf, &buflen ) )
	{
		if ( buflen )
		{
			os.write ( ( char* ) buf, buflen );
		}

		return buflen;
	}

	return 0;
}

bool
Crypto::reinitialize ( void )
{
	return ( 1 == ::EVP_CipherInit_ex ( THIS(), THIS_TYPE(), nullptr, ( unsigned char* ) m_key, ( unsigned char* ) m_iv, -1 ) );
}

bool
Crypto::execute ( char* out, size_t* olen, const char* in, size_t ilen )
{
	if ( not reinitialize() ) return false;

	size_t total( olen ? *olen : (ilen + getBlockSize()) );
	size_t sum(0);
	size_t cplen(total);
	do {
		if ( not update(out, &cplen, in, ilen ) ) break;
		out += cplen;
		sum += cplen;
		cplen = total - sum;
		if ( not finalize(out, &cplen) ) break;
		sum += cplen;

		if ( olen ) *olen = sum;

		reinitialize();
		return true;
	} while (false);

	reinitialize();
	return false;
}

bool
Crypto::execute ( std::string& out, const char* in, size_t ilen )
{
	if ( not reinitialize() ) return false;

	do {
		std::string tmp1;
		if ( not update(tmp1, in, ilen) ) break;

		std::string tmp2;
		if ( not finalize(tmp2) ) break;

		tmp1.append(tmp2);
		out.swap(tmp1);

		reinitialize();
		return true;
	} while (false);

	reinitialize();
	return false;
}

size_t
Crypto::execute ( std::ostream& os, const char* in, size_t ilen )
{
	if ( not reinitialize() ) return false;

	size_t sum(update(os, in, ilen));
	sum += finalize(os);

	return sum;
}

};

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4;
