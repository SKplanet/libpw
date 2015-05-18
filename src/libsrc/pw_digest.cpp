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
 * \file pw_digest.cpp
 * \brief Support message digest with openssl(http://openssl.org/).
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_digest.h"
#include "./pw_string.h"
#include "./pw_log.h"
#include "./pw_crypto.h"

#include <openssl/evp.h>

namespace pw {

#if (HAVE_EVP_MD_CTX > 0 )

#define THIS()		static_cast<EVP_MD_CTX*>((void*)&(this->m_hash[0]))
#define THIS_TYPE()	static_cast<const EVP_MD*>(m_type)

static intptr_cont		g_nid2md;
static intptr_cont		g_pw2md;
static striptr_cont	g_str2md;

inline
static
const ::EVP_MD*
_getMDByNID(int nid)
{
	auto ib(g_nid2md.find(nid));
	if ( g_nid2md.end() == ib ) return nullptr;
	return static_cast<const ::EVP_MD*>(ib->second);
}

inline
static
const ::EVP_MD*
_getMDByName(const std::string& name)
{
	auto ib(g_str2md.find(name));
	if ( g_str2md.end() == ib ) return ::EVP_get_digestbyname(name.c_str());
	return static_cast<const ::EVP_MD*>(ib->second);
}

inline
static
const ::EVP_MD*
_getMDByPW(digest::DigestType ht)
{
	auto ib(g_pw2md.find(static_cast<int>(ht)));
	return static_cast<const ::EVP_MD*>( ib == g_pw2md.end() ? nullptr : ib->second );
}

namespace digest
{
bool
initialize(void)
{
	OpenSSL_add_all_algorithms();

#ifdef HAVE_EVP_MD2
	g_str2md["md2"] = g_nid2md[NID_md2] = g_pw2md[static_cast<int>(DigestType::MD2)] = (void*)EVP_md2();
#endif

	g_str2md["md5"] = g_nid2md[NID_md5] = g_pw2md[static_cast<int>(DigestType::MD5)] = (void*)EVP_md5();
	g_str2md["ripemd160"] = g_nid2md[NID_ripemd160] = g_pw2md[static_cast<int>(DigestType::RIPEMD160)] = (void*)EVP_ripemd160();
	g_str2md["sha1"] = g_nid2md[NID_sha1] = g_pw2md[static_cast<int>(DigestType::SHA1)] = (void*)EVP_sha1();
	g_str2md["sha224"] = g_nid2md[NID_sha224] = g_pw2md[static_cast<int>(DigestType::SHA224)] = (void*)EVP_sha224();
	g_str2md["sha256"] = g_nid2md[NID_sha256] = g_pw2md[static_cast<int>(DigestType::SHA256)] = (void*)EVP_sha256();
	g_str2md["sha384"] = g_nid2md[NID_sha384] = g_pw2md[static_cast<int>(DigestType::SHA384)] = (void*)EVP_sha384();
	g_str2md["sha512"] = g_nid2md[NID_sha512] = g_pw2md[static_cast<int>(DigestType::SHA512)] = (void*)EVP_sha512();
	g_str2md["sha"] = g_nid2md[NID_sha] = g_pw2md[static_cast<int>(DigestType::SHA)] = (void*)EVP_sha();

#ifdef NID_dss1
	//g_nid2md[NID_dss1] =
#endif
	g_str2md["dss1"] = g_pw2md[static_cast<int>(DigestType::DSS1)] = (void*)EVP_dss1();

#ifdef NID_dss
	//g_nid2md[NID_dss] =
#endif
	g_str2md["dss"] = g_pw2md[static_cast<int>(DigestType::DSS)] = (void*)EVP_dss();

#ifdef NID_md_null
	//g_nid2md[NID_md_null] =
#endif
	g_str2md["null"] = g_pw2md[static_cast<int>(DigestType::MD_NULL)] = (void*)EVP_md_null();

	//g_str2md["mdc2"] = g_nid2md[NID_mdc2] = g_pw2md[static_cast<int>(MDC2)] = (void*)EVP_mdc2();

	return crypto::initializeLocks();
}

const void*
getAlg ( DigestType hashType )
{
	return _getMDByPW(hashType);
}

//namespace hash
}

bool
Digest::_init(void)
{
	::EVP_MD_CTX_cleanup(THIS());
	if ( nullptr == m_type ) return false;
	return 1 == ::EVP_DigestInit_ex(THIS(), THIS_TYPE(), nullptr);
}

size_t
Digest::s_getHashSize(digest::DigestType ht)
{
	const ::EVP_MD* md(_getMDByPW(ht));
	if ( nullptr == md ) return 0;
	return size_t(EVP_MD_size(md));
}

Digest*
Digest::s_create(const char* name)
{
	const ::EVP_MD* type(_getMDByName(name));
	if ( nullptr == type )
	{
		PWLOGLIB("not supported hash type");
		return nullptr;
	}

	Digest* ret(new Digest());
	if ( nullptr == ret )
	{
		PWLOGLIB("not enough memory");
		return nullptr;
	}

	ret->m_type = type;
	if ( not ret->_init() )
	{
		delete ret;
		return nullptr;
	}

	return ret;
}

Digest*
Digest::s_create(digest::DigestType ht)
{
	const ::EVP_MD* type(_getMDByPW(ht));
	if ( nullptr == type )
	{
		PWLOGLIB("not supported hash type");
		return nullptr;
	}

	Digest* ret(new Digest());
	if ( nullptr == ret )
	{
		PWLOGLIB("not enough memory");
		return nullptr;
	}

	ret->m_type = type;
	if ( not ret->_init() )
	{
		delete ret;
		return nullptr;
	}

	return ret;
}

bool
Digest::s_execute(void* out, const void* in, size_t len, digest::DigestType type, size_t* outlen)
{
	Digest* hash(s_create(type));
	if ( nullptr == hash ) return false;

	do {
		if ( not hash->update(in, len) ) break;
		if ( not hash->finalize(out, outlen) ) break;

		hash->release();
		return true;
	} while (false);

	hash->release();
	return false;
}

bool
Digest::s_execute(std::string& out, const void* in, size_t len, digest::DigestType type)
{
	Digest* hash(s_create(type));
	if ( nullptr == hash ) return false;

	do {
		if ( not hash->update(in, len) ) break;

		std::string tmp;
		if ( not hash->finalize(tmp) ) break;

		hash->release();
		out.swap(tmp);

		return true;
	} while (false);

	hash->release();
	return false;
}

Digest::Digest() : m_type(nullptr)
{
	::EVP_MD_CTX_init(THIS());
}

Digest::~Digest()
{
	::EVP_MD_CTX_cleanup(THIS());
}

bool
Digest::update(const void* in, size_t ilen)
{
	return (1 == ::EVP_DigestUpdate(THIS(), in, ilen));
}

bool
Digest::finalize(void* out, size_t* olen)
{
	unsigned char *md((unsigned char*)out);
    unsigned int s(0);
	if ( 1 == ::EVP_DigestFinal_ex(THIS(), md, &s))
	{
		if ( olen ) *olen = s;
		return true;
	}
	return false;
}

size_t
Digest::getHashSize(void) const
{
	return EVP_MD_CTX_size(THIS());
}

bool
Digest::reinitialize(void)
{
	EVP_MD_CTX* ctx(THIS());

	::EVP_MD_CTX_cleanup(ctx);

	if ( nullptr == m_type ) return false;

	return (1 == ::EVP_DigestInit_ex(ctx, THIS_TYPE(), nullptr));
}

bool
Digest::reinitialize(digest::DigestType type)
{
	m_type = _getMDByPW(type);
	return reinitialize();
}

bool
Digest::reinitialize(const char* type_name)
{
	m_type = _getMDByName(type_name);
	return reinitialize();
}

bool
Digest::reinitializeByNID(int nid)
{
	m_type = _getMDByNID(nid);
	return reinitialize();
}

#endif//HAVE_EVP_MD_CTX > 0

};//namespace pw
