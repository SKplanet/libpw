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
 * \file pw_ssl.cpp
 * \brief Support SSL/TLS protocols and certificates with openssl(http://openssl.org).
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_ssl.h"
#include "./pw_iopoller.h"
#include "./pw_string.h"
#include "./pw_log.h"
#include "./pw_crypto.h"

#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/engine.h>
#include <openssl/md5.h>
#include <openssl/pkcs12.h>

namespace pw {

using cert_list_type = STACK_OF(X509);
using cert_name_list_type = STACK_OF(X509_NAME);

#define _toSSL(x)			static_cast<SSL*>(x)
#define _toSSL_CTX(x)		static_cast<SSL_CTX*>(x)
#define _toX509(x)			static_cast<X509*>(x)
#define _toEVP_PKEY(x)		static_cast<EVP_PKEY*>(x)
#define _toRSA(x)			static_cast<RSA*>(x)

#define THIS_SSL()			static_cast<SSL*>(this->m_ssl)
#define THIS_SSL_CTX()		static_cast<SSL_CTX*>(this->m_ctx)
#define THIS_X509()			static_cast<X509*>(this->m_cert)
#define THIS_X509_STORE()	static_cast<X509_STORE*>(this->m_cert_store)
#define THIS_X509_STORE_CTX()			static_cast<X509_STORE_CTX*>(this->m_cert_store_ctx)
#define THIS_EVP_PKEY()		static_cast<EVP_PKEY*>(this->m_pkey)
#define THIS_RSA()			static_cast<RSA*>(this->m_rsa)

static const striint_cont s_str2asymkeyalg =
{
	{"RSA", int ( ssl::AsymKeyAlg::RSA ) },
	{"DSA", int ( ssl::AsymKeyAlg::DSA ) },
	{"DH", int ( ssl::AsymKeyAlg::DH ) },
	{"EC", int ( ssl::AsymKeyAlg::EC ) },
	{"HMAC", int ( ssl::AsymKeyAlg::HMAC ) },
	{"CMAC", int ( ssl::AsymKeyAlg::CMAC ) },
	{"INVALID", int ( ssl::AsymKeyAlg::INVALID ) },
};

static const intstr_cont s_asymkeyalg2str =
{
	{int ( ssl::AsymKeyAlg::RSA ), "RSA"},
	{int ( ssl::AsymKeyAlg::DSA ), "DSA"},
	{int ( ssl::AsymKeyAlg::DH ), "DH"},
	{int ( ssl::AsymKeyAlg::EC ), "EC"},
	{int ( ssl::AsymKeyAlg::HMAC ), "HMAC"},
	{int ( ssl::AsymKeyAlg::CMAC ), "CMAC"},
	{int ( ssl::AsymKeyAlg::INVALID ), "INVALID"},
};

// Sign/Verify with RSA parameter
struct sv_param_rsa_type
{
	int pt;
	int pss_saltlen;
};

// Union of Sign/Verify parameter
union sv_param_type
{
	sv_param_rsa_type rsa;
};

struct sv_context_type
{
	EVP_MD_CTX* ctx = nullptr;
	const EVP_MD* md = nullptr;
	EVP_PKEY* pkey = nullptr;
	int pkey_type = 0;

	sv_param_type param;
};

inline
static
std::string&
_make_ini_tag(std::string& out, const char* prefix, const char* post)
{
	if ( prefix and (prefix[0] not_eq 0x00) )
	{
		PWStr::format(out, "%s.%s", prefix, post);
	}
	else
	{
		PWStr::format(out, "%s", post);
	}

	return out;
}

inline
static
int
_pem_passwd_cb(char* buf, int size, int rwflag, void* u)
{
	if ( nullptr == u ) return 0;

	const char* passwd((const char*)u);
	int passwd_len(strlen(passwd));

	if ( passwd_len > size ) return 0;
	memcpy(buf, passwd, passwd_len);

	return passwd_len;
}

inline
static
int
_pem_passwd_cb_blob(char* buf, int size, int rwflag, void* u)
{
	if ( nullptr == u ) return 0;
	auto in(reinterpret_cast<const blob_type*>(u));

	if ( in->size > size_t(size) ) return 0;

	if ( in->buf and in->size )
	{
		memcpy(buf, in->buf, in->size);
		return in->size;
	}

	return 0;
}

inline
static
int
_print_str(const char* str, size_t len, void* u)
{
	if ( nullptr == u ) return 0;
	std::string* out((std::string*)u);
	out->assign(str, len);
	return 1;
}

inline
static
int
_print_str2(const char* str, size_t len, void* u)
{
	if ( nullptr == u ) return 0;
	const char** out((const char**)u);
	*out = str;
	return 1;
}

#if 0
inline
static
int
_toNIDType(ssl::Sign stype)
{
	switch(stype)
	{
	case ssl::Sign::SHA1: return NID_sha1;
	case ssl::Sign::RIPEMD160: return NID_ripemd160;
	case ssl::Sign::MD5: return NID_md5;
	case ssl::Sign::MD5_SHA1: return NID_md5_sha1;
	}

	return -1;
}
#endif

inline
static
int
_toRSAPadding(ssl::RSAPadding mypad)
{
	switch (mypad)
	{
	case ssl::RSAPadding::PKCS1: return RSA_PKCS1_PADDING;
	case ssl::RSAPadding::PKCS1_OAEP: return RSA_PKCS1_OAEP_PADDING;
#ifdef RSA_PKCS1_PSS_PADDING
	case ssl::RSAPadding::PKCS1_PSS: return RSA_PKCS1_PSS_PADDING;
#else
	case ssl::RSAPadding::PKCS1_PSS: return RSA_PKCS1_PADDING;
#endif//RSA_PKCS1_PSS_PADDING
	case ssl::RSAPadding::SSL_V2_V3: return RSA_SSLV23_PADDING;
	case ssl::RSAPadding::X931: return RSA_X931_PADDING;
	case ssl::RSAPadding::NONE: return RSA_NO_PADDING;
	case ssl::RSAPadding::INVALID: return RSA_PKCS1_PADDING;
	}

	return RSA_PKCS1_PADDING;
}

#if 0
inline
static
int
_toEVP_PKEY_id(ssl::AsymKeyAlg ka)
{
	switch(ka)
	{
	case ssl::AsymKeyAlg::RSA: return EVP_PKEY_RSA;
	case ssl::AsymKeyAlg::DSA: return EVP_PKEY_DSA;
	case ssl::AsymKeyAlg::DH: return EVP_PKEY_DH;
	case ssl::AsymKeyAlg::EC: return EVP_PKEY_EC;
#ifdef EVP_PKEY_HMAC
	case ssl::AsymKeyAlg::HMAC: return EVP_PKEY_HMAC;
#else
	case ssl::AsymKeyAlg::HMAC: return -1;
#endif
#ifdef EVP_PKEY_CMAC
	case ssl::AsymKeyAlg::CMAC: return EVP_PKEY_CMAC;
#else
	case ssl::AsymKeyAlg::CMAC: return -1;
#endif
	case ssl::AsymKeyAlg::INVALID: return -1;
	}

	return -1;
}
#endif

inline
static
EVP_PKEY*
_genAsymKeyRSA(size_t size)
{
#ifdef HAVE_EVP_PKEY_CTX_NEW_ID
	auto keyctx(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr));
	if ( not keyctx )
	{
		PWTRACE("EVP_PKEY_CTX_new_id failed.");
		return nullptr;
	}

	do {
		if ( EVP_PKEY_keygen_init(keyctx) <= 0 )
		{
			PWTRACE("EVP_PKEY_keygen_init failed.");
			break;
		}

		if ( EVP_PKEY_CTX_set_rsa_keygen_bits(keyctx, 2048) <= 0 )
		{
			PWTRACE("EVP_PKEY_CTX_set_rsa_keygen_bits failed.");
			break;
		}

		EVP_PKEY* pkey(nullptr);
		if ( EVP_PKEY_keygen(keyctx, &pkey) <= 0 )
		{
			PWTRACE("EVP_PKEY_keygen failed.");
			break;
		}

		EVP_PKEY_CTX_free(keyctx);
		return pkey;
	} while (false);

	if (keyctx) EVP_PKEY_CTX_free(keyctx);
	return nullptr;
#else
	return nullptr;
#endif
}

inline
static
EVP_PKEY*
_getAsymKeyMAC(ssl::AsymKeyAlg ka, const char* key, size_t klen)
{
#ifdef HAVE_EVP_PKEY_NEW_MAC_KEY
	const unsigned char* in_buf(reinterpret_cast<const unsigned char*>(key));
	if ( key and (klen == 0) ) klen = strlen(key);
	else if ( not key ) klen = 0;

	if ( ka == ssl::AsymKeyAlg::HMAC )
	{
		return EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, nullptr, in_buf, klen);
	}
	else if ( ka == ssl::AsymKeyAlg::CMAC )
	{
		auto keyctx(EVP_PKEY_CTX_new_id(EVP_PKEY_CMAC, nullptr));
		do {
			if ( not keyctx )
			{
				PWTRACE("EVP_PKEY_CTX_new_id failed");
				break;
			}

			if ( EVP_PKEY_keygen_init(keyctx) <= 0 )
			{
				PWTRACE("EVP_PKEY_CTX_init failed");
				break;
			}

			if ( EVP_PKEY_CTX_ctrl(keyctx,
				-1, EVP_PKEY_OP_KEYGEN,
				EVP_PKEY_CTRL_CIPHER,
				0, (void*)EVP_aes_256_ecb()) <= 0 )
			{
				PWTRACE("EVP_PKEY_CTX_ctrl+EVP_PKEY_OP_KEYGEN+EVP_PKEY_CTRL_SET_MAC_CIPHER failed");
				break;
			}

			if ( EVP_PKEY_CTX_ctrl(keyctx,
				-1, EVP_PKEY_OP_KEYGEN,
				EVP_PKEY_CTRL_SET_MAC_KEY,
				klen, (void*)key) <= 0 )
			{
				PWTRACE("EVP_PKEY_CTX_ctrl+EVP_PKEY_OP_KEYGEN+EVP_PKEY_CTRL_SET_MAC_KEY failed");
				break;
			}

			EVP_PKEY* ret(nullptr);
			if ( EVP_PKEY_keygen(keyctx, &ret) <= 0 )
			{
				PWTRACE("EVP_PKEY_keygen failed");
				break;
			}

			EVP_PKEY_CTX_free(keyctx);
			return ret;
		} while (false);

		if ( keyctx ) EVP_PKEY_CTX_free(keyctx);
		return nullptr;
	}

	return nullptr;
#else
	return nullptr;
#endif
}

inline
static
const
SSL_METHOD*
_toSSL_METHOD(ssl::Method type)
{
	switch(type)
	{
#if 1
	case ssl::Method::CLIENT_V3: return SSLv3_client_method();
	case ssl::Method::SERVER_V3: return SSLv3_server_method();
	case ssl::Method::BOTH_V3: return SSLv3_method();
#endif

#if 0
	case ssl::Method::CLIENT_V2_V3: return SSLv23_client_method();
	case ssl::Method::SERVER_V2_V3: return SSLv23_server_method();
	case ssl::Method::BOTH_V2_V3: return SSLv23_method();
#endif

#ifdef HAVE_TLSV1_3_METHOD
	case ssl::Method::CLIENT_T1_3: return TLSv1_3_client_method();
	case ssl::Method::SERVER_T1_3: return TLSv1_3_server_method();
	case ssl::Method::BOTH_T1_3: return TLSv1_3_method();
#endif

#ifdef HAVE_TLSV1_2_METHOD
	case ssl::Method::CLIENT_T1_2: return TLSv1_2_client_method();
	case ssl::Method::SERVER_T1_2: return TLSv1_2_server_method();
	case ssl::Method::BOTH_T1_2: return TLSv1_2_method();
#endif

#ifdef HAVE_TLSV1_1_METHOD
	case ssl::Method::CLIENT_T1_1: return TLSv1_1_client_method();
	case ssl::Method::SERVER_T1_1: return TLSv1_1_server_method();
	case ssl::Method::BOTH_T1_1: return TLSv1_1_method();
#endif

#ifdef HAVE_TLSV1_METHOD
	case ssl::Method::CLIENT_T1: return TLSv1_client_method();
	case ssl::Method::SERVER_T1: return TLSv1_server_method();
	case ssl::Method::BOTH_T1: return TLSv1_method();
#endif

	case ssl::Method::INVALID:
	default:
		return nullptr;
	}

	return nullptr;
}

inline
static
bool
_toKeyValue(SslCertificate::kv_cont& out, const ::X509_NAME* _name, bool sn)
{
	auto name(const_cast<X509_NAME*>(_name));

	int ib(0), ie(::X509_NAME_entry_count(name));
	::X509_NAME_ENTRY* ne(nullptr);
	::ASN1_OBJECT* obj(nullptr);
	::ASN1_STRING* str(nullptr);
	int obj_cnt(0), nid(0), last(-1), oid(0), slen(0);
	char* sname(nullptr);
	unsigned char* sutf(nullptr);

	SslCertificate::kv_cont		tmp;
	SslCertificate::line_cont*	lines;

	while ( ib not_eq ie )
	{
		ne = ::X509_NAME_get_entry(name, ib);
		obj = ::X509_NAME_ENTRY_get_object(ne);
		nid = ::OBJ_obj2nid(obj);
		obj_cnt = 0;

		sname = (char*)(sn ? ::OBJ_nid2sn(nid) : ::OBJ_nid2ln(nid));
		lines = &(tmp[sname]);
		last = -1;

		do
		{
			oid = ::X509_NAME_get_index_by_OBJ(name, obj, last);

			if ( oid >= 0 )
			{
				++ obj_cnt;
				ne = ::X509_NAME_get_entry(name, oid);
				str = ::X509_NAME_ENTRY_get_data(ne);

				if ( ::ASN1_STRING_type(str) == V_ASN1_UTF8STRING )
				{
					// UTF-8
					sutf = ::ASN1_STRING_data(str);
					slen = ::ASN1_STRING_length(str);
				}
				else
				{
					// to UTF-8
					slen = ::ASN1_STRING_to_UTF8(&sutf, str);
				}

				if ( -1 != slen ) lines->push_back(std::string((char*)sutf, slen));

				last = oid;
			}// if ( oid > 0 )
			else
			{
				if ( last != -1 ) break;
			}
		} while (true);

		ib = last + 1;

	}// while ( ib < ie )

	tmp.swap(out);

	return true;
}

inline
static
bool
_toString(std::string& out, const X509_NAME* _name)
{
	std::string tmp;

	BIO* fp(BIO_new(BIO_s_mem()));

	if ( nullptr == fp ) return false;

	X509_NAME* name(const_cast<X509_NAME*>(_name));
	::X509_NAME_print_ex(fp, name, 0, XN_FLAG_RFC2253);

	char* buf;
	long blen(BIO_get_mem_data(fp, &buf));
	if ( blen > 0 ) tmp.assign(buf, blen);

	BIO_free(fp);

	out.swap(tmp);
	return true;
}

inline
static
int
_mem2int(const unsigned char* buf, size_t blen)
{
	int ret(0);
	int pow(1);

	const unsigned char* ib(buf+blen-1);
	const unsigned char* ie(buf-1);
	while ( ib not_eq ie )
	{
		ret += ( (*ib) - int('0') ) * pow;
		--ib;
		pow *= 10;
	}

	return ret;
}

inline
static
int
_mem2offset(const unsigned char* buf)
{
	return ((_mem2int(buf, 2) * 3600) + (_mem2int(buf+2, 2) * 60));
}

inline
static
time_t
_toTime(const char* asn_str, size_t asn_len, bool utc)
{
	if ( size_t(-1) == asn_len ) asn_len = ::strlen(asn_str);

	//UTC
	//YYMMDDHHMMSS{Z,{+,-}hhmm}

	//Generalize
	//YYYYMMDDHHMMSS{Z,{+,-}hhmm}
	struct tm tm;
	memset(&tm, 0x00, sizeof(tm));

	const unsigned char* p((unsigned char*)asn_str);

	if ( utc )
	{
		if ( asn_len < 13 ) return time_t(-1);

		tm.tm_year = _mem2int(p, 2); p += 2;
		if ( tm.tm_year < 50 ) tm.tm_year += 100;
		tm.tm_mon = _mem2int(p, 2) - 1; p += 2;
		tm.tm_mday = _mem2int(p, 2); p += 2;
		tm.tm_hour = _mem2int(p, 2); p += 2;
		tm.tm_min = _mem2int(p, 2); p += 2;
		tm.tm_sec = _mem2int(p, 2); p += 2;
	}
	else
	{
		if ( asn_len < 15 ) return time_t(-1);

		tm.tm_year = _mem2int(p, 4) - 1900; p += 4;
		tm.tm_mon = _mem2int(p, 2) - 1; p += 2;
		tm.tm_mday = _mem2int(p, 2); p += 2;
		tm.tm_hour = _mem2int(p, 2); p += 2;
		tm.tm_min = _mem2int(p, 2); p += 2;
		tm.tm_sec = _mem2int(p, 2); p += 2;
	}

	const unsigned char e(*p); ++p;
	switch(e)
	{
	case 'Z': break;
	case '+': tm.tm_gmtoff = _mem2offset(p); break;
	case '-': tm.tm_gmtoff = 0 - _mem2offset(p); break;
	default: return time_t(-1);
	}

	time_t t(time(nullptr));
	time_t off(tm.tm_gmtoff + localtime(&t)->tm_gmtoff);
	tm.tm_gmtoff = 0;

	return ::mktime(&tm) + off;
}

inline
static
ssize_t
_ssl_err_proc(SSL* ssl, int res)
{
	const int err(SSL_get_error(ssl, res));
	switch(err)
	{
	case SSL_ERROR_WANT_READ: errno = EAGAIN; break;
	case SSL_ERROR_WANT_WRITE: errno = EAGAIN; break;
	case SSL_ERROR_NONE: PWTRACE("none"); break;
	case SSL_ERROR_ZERO_RETURN: PWTRACE("zero return"); break;
	case SSL_ERROR_SYSCALL: PWTRACE("system call:%d %s %lu", errno, strerror(errno), ERR_get_error()); break;
	case SSL_ERROR_SSL: PWTRACE("ssl"); break;
	}

	return ssize_t(res);
}

//------------------------------------------------------------------------------
// namespace ssl
//
namespace ssl {

const char*
toStringA(AsymKeyAlg ka)
{
	auto ib(s_asymkeyalg2str.find(int(ka)));
	if ( ib == s_asymkeyalg2str.end() ) return nullptr;
	return ib->second.c_str();
}

std::string
toString(AsymKeyAlg ka)
{
	auto ib(s_asymkeyalg2str.find(int(ka)));
	if ( ib == s_asymkeyalg2str.end() ) return nullptr;
	return ib->second.c_str();
}

std::string&
toString(std::string& out, AsymKeyAlg ka)
{
	auto ib(s_asymkeyalg2str.find(int(ka)));
	if ( ib == s_asymkeyalg2str.end() )
	{
		out = std::string("INVALID");
		return out;
	}

	return ( out = ib->second );
}

AsymKeyAlg
toAsymKeyAlg(const char* s)
{
	auto ib(s_str2asymkeyalg.find(s));
	if ( ib == s_str2asymkeyalg.end() )
	{
		return AsymKeyAlg::INVALID;
	}
	return AsymKeyAlg(ib->second);
}

AsymKeyAlg
toAsymKeyAlg(const std::string& s)
{
	auto ib(s_str2asymkeyalg.find(s));
	if ( ib == s_str2asymkeyalg.end() )
	{
		return AsymKeyAlg::INVALID;
	}
	return AsymKeyAlg(ib->second);
}

bool
loadPKCS12File ( pkcs12_load_param_type& inout, const char* path )
{
	PKCS12* pkcs(nullptr);

	do {
		auto in(BIO_new_file(path, "rb"));
		if ( not in ) return false;

		pkcs = d2i_PKCS12_bio(in, nullptr);


		BIO_free(in);
	} while (false);

	if ( not pkcs )
	{
		PWTRACE("d2i_PKCS12_bio failed");
		return false;
	}

	SslAsymmetricKey* ssl_pkey(nullptr);
	SslCertificate* ssl_cert(nullptr);
	SslCertificateList* ssl_cert_list(nullptr);

	EVP_PKEY* pkey(nullptr);
	X509* cert(nullptr);
	cert_list_type* cert_list(nullptr);

	do {
		const char* passwd(inout.in.passwd ? inout.in.passwd : "");

		auto p_pkey(inout.in.need.key ? &pkey : nullptr);
		auto p_cert(inout.in.need.cert ? &cert : nullptr);
		auto p_cert_list(inout.in.need.cert_list ? &cert_list : nullptr);
		if ( PKCS12_parse(pkcs, passwd, p_pkey, p_cert, p_cert_list) <= 0 )
		{
			PWTRACE("PKCS12_parse failed");
			break;
		}

		if ( pkey and not (ssl_pkey = SslAsymmetricKey::s_createRawData(pkey)) )
		{
			PWTRACE("failed to create asymkey");
			break;
		}

		if ( cert and not (ssl_cert = SslCertificate::s_createRawData(cert)) )
		{
			PWTRACE("failed to create cert");
			break;
		}

		if ( cert_list and not (ssl_cert_list = SslCertificateList::s_createRawData(cert_list)) )
		{
			PWTRACE("failed to create cert list");
			break;
		}

		auto& out(inout.out);
		out.key = ssl_pkey;
		out.cert = ssl_cert;
		out.cert_list = ssl_cert_list;

		PKCS12_free(pkcs);

		return true;
	} while (false);

	if ( pkcs ) PKCS12_free(pkcs);

	return false;
}


VerifyMode&
VerifyMode::addFlagPeer(void)
{
	m_mode or_eq SSL_VERIFY_PEER;
	return *this;
}

VerifyMode&
VerifyMode::addFlagFailIfNoPeerCert(void)
{
	m_mode or_eq SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
	return *this;
}

VerifyMode&
VerifyMode::addFlagClientOnce(void)
{
	m_mode or_eq SSL_VERIFY_CLIENT_ONCE;
	return *this;
}

bool
initialize(void)
{
	SSL_library_init();
	SSL_load_error_strings();

	return crypto::initializeLocks();
}

std::string&
getLastErrorString(std::string& out)
{
	ERR_print_errors_cb(_print_str, &out);
	return out;
}

const char*
getLastErrorString(void)
{
	const char* out;
	ERR_print_errors_cb(_print_str2, &out);
	return out;
}

const char*
getMethodString(ssl::Method type)
{
	switch(type)
	{
	case ssl::Method::CLIENT_T1: return "CLIENT_T1";
	case ssl::Method::SERVER_T1: return "SERVER_T1";
	case ssl::Method::BOTH_T1: return "BOTH_T1";

	case ssl::Method::CLIENT_T1_3: return "CLIENT_T1_3";
	case ssl::Method::SERVER_T1_3: return "SERVER_T1_3";
	case ssl::Method::BOTH_T1_3: return "BOTH_T1_3";

	case ssl::Method::CLIENT_T1_2: return "CLIENT_T1_2";
	case ssl::Method::SERVER_T1_2: return "SERVER_T1_2";
	case ssl::Method::BOTH_T1_2: return "BOTH_T1_2";

	case ssl::Method::CLIENT_T1_1: return "CLIENT_T1_1";
	case ssl::Method::SERVER_T1_1: return "SERVER_T1_1";
	case ssl::Method::BOTH_T1_1: return "BOTH_T1_1";

	case ssl::Method::CLIENT_V3: return "CLIENT_V3";
	case ssl::Method::SERVER_V3: return "SERVER_V3";
	case ssl::Method::BOTH_V3: return "BOTH_V3";

	case ssl::Method::CLIENT_V2_V3: return "CLIENT_V2_V3";
	case ssl::Method::SERVER_V2_V3: return "SERVER_V2_V3";
	case ssl::Method::BOTH_V2_V3: return "BOTH_V2_V3";

	case ssl::Method::CLIENT_V2: return "CLIENT_V2";
	case ssl::Method::SERVER_V2: return "SERVER_V2";
	case ssl::Method::BOTH_V2: return "BOTH_V2";
	case ssl::Method::INVALID:
	default:
		return nullptr;
	}

	return nullptr;
}

ssl::Method
getMethod(const char* p)
{
	if (not strcasecmp("CLIENT_LATEST", p)) return getLastClientMethod();
	if (not strcasecmp("SERVER_LATEST", p)) return getLastServerMethod();
	if (not strcasecmp("BOTH_LATEST", p)) return getLastBothMethod();

	if (not strcasecmp("CLIENT_T1", p)) return ssl::Method::CLIENT_T1;
	if (not strcasecmp("SERVER_T1", p)) return ssl::Method::SERVER_T1;
	if (not strcasecmp("BOTH_T1", p)) return ssl::Method::BOTH_T1;

	if (not strcasecmp("CLIENT_T1_1", p)) return ssl::Method::CLIENT_T1_1;
	if (not strcasecmp("SERVER_T1_1", p)) return ssl::Method::SERVER_T1_1;
	if (not strcasecmp("BOTH_T1_1", p)) return ssl::Method::BOTH_T1_1;

	if (not strcasecmp("CLIENT_T1_2", p)) return ssl::Method::CLIENT_T1_2;
	if (not strcasecmp("SERVER_T1_2", p)) return ssl::Method::SERVER_T1_2;
	if (not strcasecmp("BOTH_T1_2", p)) return ssl::Method::BOTH_T1_2;

	if (not strcasecmp("CLIENT_T1_3", p)) return ssl::Method::CLIENT_T1_3;
	if (not strcasecmp("SERVER_T1_3", p)) return ssl::Method::SERVER_T1_3;
	if (not strcasecmp("BOTH_T1_3", p)) return ssl::Method::BOTH_T1_3;

	if (not strcasecmp("CLIENT_V3", p)) return ssl::Method::CLIENT_V3;
	if (not strcasecmp("SERVER_V3", p)) return ssl::Method::SERVER_V3;
	if (not strcasecmp("BOTH_V3", p)) return ssl::Method::BOTH_V3;

	if (not strcasecmp("CLIENT_V2_V3", p)) return ssl::Method::CLIENT_V2_V3;
	if (not strcasecmp("SERVER_V2_V3", p)) return ssl::Method::SERVER_V2_V3;
	if (not strcasecmp("BOTH_V2_V3", p)) return ssl::Method::BOTH_V2_V3;

	if (not strcasecmp("CLIENT_V2", p)) return ssl::Method::CLIENT_V2;
	if (not strcasecmp("SERVER_V2", p)) return ssl::Method::SERVER_V2;
	if (not strcasecmp("BOTH_V2", p)) return ssl::Method::BOTH_V2;
	return ssl::Method::INVALID;
}

//------------------------------------------------------------------------------
// context_type
// Instance에서 Ssl context를 용이하게 쓰기 위한 구조체
void
context_type::swap(context_type& v)
{
	method.swap(v.method);
	ca_file.swap(v.ca_file);
	ca_path.swap(v.ca_path);
	cipher_list.swap(v.cipher_list);
	cert_file.swap(v.cert_file);
	cert_passwd.swap(v.cert_passwd);
	pkey_file.swap(v.pkey_file);
	pkey_passwd.swap(v.pkey_passwd);
	std::swap(ctx, v.ctx);
}

void context_type::clear ( void )
{
	if ( ctx ) { SslContext::s_release(ctx); ctx = nullptr; }
	method.clear();
	ca_file.clear(); ca_path.clear();
	cipher_list.clear();
	cert_file.clear(); cert_passwd.clear();
	pkey_file.clear(); pkey_passwd.clear();
}

bool
context_type::read(const Ini& ini, const char* prefix, const std::string& sec)
{
	auto isec(ini.find(sec));
	if ( isec == ini.end() ) return false;
	return this->read(ini, prefix, isec);
}

bool
context_type::read(const Ini& conf, const char* prefix, Ini::sec_citr sec)
{
	// Ssl settings...
	do {
		context_type tmp;
		std::string tag;

		conf.getString2(tmp.method, _make_ini_tag(tag, prefix, "method"), sec, this->method);
		if ( tmp.method.empty() )
		{
			PWTRACE("method is empty: tag:%s", tag.c_str());
			break;
		}

		PWTRACE("method: %s", cstr(tmp.method));
		PWTRACE("method final: %s", ssl::getMethodString(ssl::getMethod(cstr(tmp.method))));

		conf.getString2(tmp.ca_file, _make_ini_tag(tag, prefix, "ca.file"), sec, this->ca_file);
		conf.getString2(tmp.ca_path, _make_ini_tag(tag, prefix, "ca.path"), sec, this->ca_path);
		conf.getString2(tmp.cipher_list, _make_ini_tag(tag, prefix, "cipher.list"), sec, this->cipher_list);
		conf.getString2(tmp.cert_file, _make_ini_tag(tag, prefix, "cert.file"), sec, this->cert_file);
		conf.getString2(tmp.cert_passwd, _make_ini_tag(tag, prefix, "cert.passwd"), sec, this->cert_passwd);
		conf.getString2(tmp.pkey_file, _make_ini_tag(tag, prefix, "pkey.file"), sec, this->pkey_file);
		conf.getString2(tmp.pkey_passwd, _make_ini_tag(tag, prefix, "pkey.passwd"), sec, this->pkey_passwd);

		const bool isUpdated(
			(tmp.method not_eq this->method)
			or (tmp.ca_file not_eq this->ca_file)
			or (tmp.ca_path not_eq this->ca_path)
			or (tmp.cipher_list not_eq this->cipher_list)
			or (tmp.cert_file not_eq this->cert_file)
			or (tmp.cert_passwd not_eq this->cert_passwd)
			or (tmp.pkey_file not_eq this->pkey_file)
			or (tmp.pkey_passwd not_eq this->pkey_passwd)
		);

		if ( not isUpdated )
		{
			PWTRACE("parameters are the same!");
			return (this->ctx not_eq nullptr);
		}

		// new context
		if ( nullptr == (tmp.ctx = SslContext::s_create(tmp.method.c_str())) )
		{
			PWLOGLIB("failed to create context: method:%s", tmp.method.c_str());
			break;
		}

		// set verify location
		if ( not tmp.ca_file.empty() or not tmp.ca_path.empty() )
		{
			const char* file(tmp.ca_file.empty() ? nullptr : tmp.ca_file.c_str());
			const char* path(tmp.ca_path.empty() ? nullptr : tmp.ca_path.c_str());
			if ( not tmp.ctx->setVerifyLocation(file, path) )
			{
				PWLOGLIB("failed to set verify location: file:%s path:%s", file, path);
				break;
			}
		}

		// set cipher list
		if ( not tmp.cipher_list.empty() )
		{
			if ( not tmp.ctx->setCipherList(tmp.cipher_list.c_str()) )
			{
				PWLOGLIB("failed to set cipher list: %s", tmp.cipher_list.c_str());
				break;
			}
		}

		if ( not tmp.cert_file.empty() )
		{
			const char* file(tmp.cert_file.c_str());
			const char* passwd(tmp.cert_passwd.empty() ? nullptr : tmp.cert_passwd.c_str());

			if ( not tmp.ctx->setCertificate(file, passwd) )
			{
				PWLOGLIB("failed to set certificate: file:%s", file);
				break;
			}
		}

		if ( not tmp.pkey_file.empty() )
		{
			const char* file(tmp.pkey_file.c_str());
			const char* passwd(tmp.pkey_passwd.empty() ? nullptr : tmp.pkey_passwd.c_str());

			if ( not tmp.ctx->setPrivateKey(file, passwd) )
			{
				PWLOGLIB("failed to set privatekey: file:%s", file);
				break;
			}

			if ( not tmp.ctx->verifyPrivateKey() )
			{
				PWLOGLIB("failed to verify private key: file:%s", file);
				break;
			}
		}

		this->swap(tmp);

		// Keep wrapper class pointer.
		if ( (tmp.ctx != nullptr) and (this->ctx != nullptr) )
		{
			std::swap(tmp.ctx, this->ctx);
			this->ctx->swap(*tmp.ctx);
		}

		return true;
	} while (false);
	return false;
}

context_type::~context_type()
{
	if ( ctx ) { SslContext::s_release(ctx); ctx = nullptr; }
}

//namespace ssl
};

static
bool
_reinit_SV_RSA(void* klass_ctx, bool is_sign)
{
#if ( HAVE_EVP_DIGESTSIGNINIT && HAVE_EVP_DIGESTVERIFYINIT )
	auto myctx(reinterpret_cast<sv_context_type*>(klass_ctx));
	auto initfunc( is_sign ? EVP_DigestSignInit : EVP_DigestVerifyInit );

	auto ctx(myctx->ctx);
	EVP_MD_CTX_cleanup(ctx);

	auto md(myctx->md);
	auto pkey(myctx->pkey);

	EVP_PKEY_CTX* pkey_ctx(nullptr);

	do {
		if ( initfunc(ctx, &pkey_ctx, md, nullptr, pkey) <= 0 )
		{
			PWTRACE("EVP_Digest(Sign|Verify)Init failed");
			break;
		}

		auto pt(myctx->param.rsa.pt);
		if ( EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, pt) <= 0 )
		{
			PWTRACE("EVP_PKEY_CTX_set_rsa_padding failed");
			break;
		}

		if ( (RSA_PKCS1_PSS_PADDING == pt) and ( EVP_PKEY_CTX_set_rsa_pss_saltlen(pkey_ctx, myctx->param.rsa.pss_saltlen) <= 0 ) )
		{
			PWTRACE("EVP_PKEY_CTX_set_rsa_pss_saltlen failed");
			break;
		}

		return true;
	} while (false);

	EVP_MD_CTX_cleanup(ctx);
	return false;
#else
	return false;
#endif
}

static
bool
_create_SV_RSA( void** klass_ctx, bool is_sign, SslAsymmetricKey& in_pkey, digest::DigestType in_ht, ssl::RSAPadding in_pt, size_t in_saltlen )
{
#if ( HAVE_EVP_DIGESTSIGNINIT && HAVE_EVP_DIGESTVERIFYINIT )
	if ( not in_pkey.isRSA() ) return false;

	auto md(static_cast<const EVP_MD*>(digest::getAlg(in_ht)));
	if ( not md )
	{
		PWTRACE("invalid hash alg");
		return false;
	}

	auto pkey(static_cast<EVP_PKEY*>(in_pkey.getRawData()));
	if ( not pkey )
	{
		PWTRACE("private key is empty");
		return false;
	}

	auto pt(_toRSAPadding(in_pt));
	EVP_PKEY_CTX* pkey_ctx(nullptr);

	auto initfunc( is_sign ? EVP_DigestSignInit : EVP_DigestVerifyInit );

	auto ctx(EVP_MD_CTX_create());
	do {
		if ( not ctx )
		{
			PWTRACE("EVP_MD_CTX_create failed");
			break;
		}

		if ( pt == RSA_PKCS1_PADDING )
		{
			if ( initfunc(ctx, nullptr, md, nullptr, pkey) <= 0 )
			{
				PWTRACE("EVP_Digest(Sign|Verify)Init failed");
				break;
			}
		}
		else
		{
			if ( initfunc(ctx, &pkey_ctx, md, nullptr, pkey) <= 0 )
			{
				PWTRACE("EVP_Digest(Sign|Verify)Init failed");
				break;
			}

			if ( EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, pt) <= 0 )
			{
				PWTRACE("EVP_PKEY_CTX_set_rsa_padding failed");
				break;
			}

			if ( (pt == RSA_PKCS1_PSS_PADDING) and ( EVP_PKEY_CTX_set_rsa_pss_saltlen(pkey_ctx, in_saltlen) <= 0 ) )
			{
				PWTRACE("EVP_PKEY_CTX_set_rsa_pss_saltlen failed");
				break;
			}
		}

		auto myctx(new sv_context_type);
		if ( not myctx ) break;

		myctx->ctx = ctx;
		myctx->md = md;
		myctx->pkey = pkey;
		myctx->pkey_type = EVP_PKEY_RSA;
		myctx->param.rsa.pt = pt;
		myctx->param.rsa.pss_saltlen = in_saltlen;

		*klass_ctx = myctx;

		return true;
	} while (false);

	if ( ctx ) EVP_MD_CTX_destroy(ctx);
	return false;
#else
	return false;
#endif
}

//------------------------------------------------------------------------------
// Ssl
// Ssl 통신 객체.
Ssl::~Ssl()
{
	//PWTRACE("destroy ssl: %p", this);
	if ( m_ssl )
	{
		//SSL_free(_toSSL(m_ssl));
		SSL_free(THIS_SSL());
		m_ssl = nullptr;
	}
}

Ssl*
Ssl::s_create(const SslContext& ctx)
{
	Ssl* ssl(new Ssl());

	if ( nullptr == ( ssl = new Ssl() ) )
	{
		PWLOGLIB("not enough memory");
		return nullptr;
	}

	if ( nullptr == (ssl->m_ssl = SSL_new(_toSSL_CTX(ctx.m_ctx))) )
	{
		PWLOGLIB("failed to create ssl object: %s", ssl::getLastErrorString());
		delete ssl;
		return nullptr;
	}

	return ssl;
}

bool
Ssl::setFD(int fd)
{
	return 1 == SSL_set_fd(THIS_SSL(), fd);
}

bool
Ssl::setReadFD(int fd)
{
	return 1 == SSL_set_rfd(THIS_SSL(), fd);
}

bool
Ssl::setWriteFD(int fd)
{
	return 1 == SSL_set_wfd(THIS_SSL(), fd);
}

bool
Ssl::reset(void)
{
	return (1 == SSL_clear(THIS_SSL()));
}

bool
Ssl::handshake(int* need)
{
	SSL* ssl(THIS_SSL());

	int res(SSL_do_handshake(ssl));
	if ( 1 == res ) return true;

	if ( need )
	{
		int err( SSL_get_error(ssl, res) );
		//PWTRACE("SSL_get_error: err:%d %s", err, ssl::getLastErrorString());

		switch(err)
		{
		case SSL_ERROR_WANT_READ:
			*need = POLLIN;
			errno = EINPROGRESS;
			break;
		case SSL_ERROR_WANT_WRITE:
			*need = POLLOUT;
			errno = EINPROGRESS;
			break;
		default:
			*need = 0;
		}
	}

	return false;
}

bool
Ssl::connect(int* need)
{
	SSL* ssl(THIS_SSL());

	int res(SSL_connect(ssl));
	if ( 1 == res ) return true;

	if ( need )
	{
		int err( SSL_get_error(ssl, res) );

		switch(err)
		{
		case SSL_ERROR_WANT_READ:
			*need = POLLIN;
			errno = EINPROGRESS;
			break;
		case SSL_ERROR_WANT_WRITE:
			*need = POLLOUT;
			errno = EINPROGRESS;
			break;
		default:
			*need = 0;
		}
	}

	return false;
}

bool
Ssl::accept(int* need)
{
	SSL* ssl(THIS_SSL());

	int res(SSL_accept(ssl));
	if ( 1 == res ) return true;

	if ( need )
	{
		int err( SSL_get_error(ssl, res) );

		switch(err)
		{
		case SSL_ERROR_WANT_READ:
			PWTRACE("WANT_READ");
			*need = POLLIN;
			errno = EINPROGRESS;
			break;
		case SSL_ERROR_WANT_WRITE:
			PWTRACE("WANT_WRITE");
			*need = POLLOUT;
			errno = EINPROGRESS;
			break;
		default:
			PWTRACE("ERROR?");
			*need = 0;
		}
	}

	return false;
}

ssize_t
Ssl::read(char* buf, size_t blen)
{
	errno = 0;
	auto ssl(THIS_SSL());
	ssize_t res(SSL_read(ssl, buf, blen));
	if ( res > 0 ) return res;

	return _ssl_err_proc(ssl, res);
}

ssize_t
Ssl::write(const char* buf, size_t blen)
{
	errno = 0;
	auto ssl(THIS_SSL());
	ssize_t res(SSL_write(ssl, buf, blen));
	if ( res > 0 ) return res;

	return _ssl_err_proc(ssl, res);
}

SslCertificate*
Ssl::getPeerCertificate(void) const
{
	SslCertificate* cert(new SslCertificate());
	if ( nullptr == cert ) return nullptr;

	X509* x(SSL_get_peer_certificate(THIS_SSL()));
	if ( nullptr == x )
	{
		delete cert;
		return nullptr;
	}

	cert->m_cert = x;
	return cert;
}

bool
Ssl::verifyPeerCertificate(void) const
{
	return X509_V_OK == SSL_get_verify_result(THIS_SSL());
}

void
Ssl::setVerify(const ssl::VerifyMode& mode, ssl::verify_func_type func)
{
	SSL_set_verify(THIS_SSL(), mode.getFlags(), nullptr);
}

void
Ssl::setVerifyDepth(size_t depth)
{
	SSL_set_verify_depth(THIS_SSL(), int(depth));
}

const void*
Ssl::getCipher(void) const
{
	return SSL_get_current_cipher(THIS_SSL());
}

const char*
Ssl::getCipherName(void) const
{
	return SSL_get_cipher_name(THIS_SSL());
}

size_t
Ssl::getCipherBits(void) const
{
	return SSL_get_cipher_bits(THIS_SSL(), nullptr);
}

const char*
Ssl::getCipherVersion(void) const
{
	return SSL_get_cipher_version(THIS_SSL());
}

//------------------------------------------------------------------------------
// SslContext
// 컨텍스트.
SslContext::~SslContext()
{
	if ( m_ctx )
	{
		SSL_CTX_free(THIS_SSL_CTX());
		m_ctx = nullptr;
	}
}

SslContext*
SslContext::s_create(ssl::Method type)
{
	SslContext* ctx(new SslContext());
	if ( nullptr == ctx )
	{
		PWLOGLIB("not enough memory");
		return nullptr;
	}

	auto method(_toSSL_METHOD(type));
	do {
		if ( nullptr == method )
		{
			PWLOGLIB("invalid method");
			break;
		}

		if ( nullptr == (ctx->m_ctx = SSL_CTX_new(method)) )
		{
			PWLOGLIB("failed to create ssl context: %s", ssl::getLastErrorString());
			break;
		}

		return ctx;
	} while (false);

	if ( ctx ) delete ctx;
	return nullptr;
}

bool
SslContext::setVerifyLocation(const char* file, const char* path)
{
	return 1 == SSL_CTX_load_verify_locations(THIS_SSL_CTX(), file, path);
}

bool
SslContext::setClientCAList(const char* file)
{
	cert_name_list_type* lst(SSL_load_client_CA_file(file));
	if ( nullptr == lst ) return false;

	SSL_CTX_set_client_CA_list(THIS_SSL_CTX(), lst);
	return true;
}

bool
SslContext::addExtraChainCertificate ( const SslCertificate& in_cert )
{
	auto cert(static_cast<const X509*>(in_cert.m_cert));
	return 1 == SSL_CTX_add_extra_chain_cert(THIS_SSL_CTX(), cert);
}

bool
SslContext::setCertificate(const SslCertificate& cert)
{
	return 1 == SSL_CTX_use_certificate(THIS_SSL_CTX(), _toX509(cert.m_cert));
}

bool
SslContext::setCertificate(const char* path, const char* passwd)
{
	SslCertificate* cert(SslCertificate::s_create(SslCertificate::create_type::CT_FILE, path, passwd));
	if ( nullptr == cert ) return false;

	bool ret(this->setCertificate(*cert));
	SslCertificate::s_release(cert);

	return ret;
}

bool
SslContext::setPrivateKey(const SslAsymmetricKey& pkey)
{
	return 1 == SSL_CTX_use_PrivateKey(THIS_SSL_CTX(), _toEVP_PKEY(pkey.m_pkey));
}

bool
SslContext::setPrivateKey(const char* path, const char* passwd)
{
	SslAsymmetricKey* pkey(SslAsymmetricKey::s_create(SslAsymmetricKey::create_type::CT_FILE, path, passwd));
	if ( nullptr == pkey ) return false;

	bool ret(this->setPrivateKey(*pkey));
	SslAsymmetricKey::s_release(pkey);

	return ret;
}

bool
SslContext::verifyPrivateKey(void) const
{
	return 1 == SSL_CTX_check_private_key(THIS_SSL_CTX());
}

bool
SslContext::setCipherList(const char* lst)
{
	return 1 == SSL_CTX_set_cipher_list(THIS_SSL_CTX(), lst);
}


void
SslContext::setVerify(const ssl::VerifyMode& mode, ssl::verify_func_type func)
{
	SSL_CTX_set_verify(THIS_SSL_CTX(), mode.getFlags(), nullptr);
}

void
SslContext::setVerifyDepth(size_t depth)
{
	SSL_CTX_set_verify_depth(THIS_SSL_CTX(), int(depth));
}
//------------------------------------------------------------------------------
// SslCertificate
// 인증서.
SslCertificate::~SslCertificate()
{
	if ( m_cert )
	{
		::X509_free(_toX509(m_cert));
		m_cert = nullptr;
	}
}

SslCertificate*
SslCertificate::s_createRawData ( void* rawdata )
{
	auto p(new SslCertificate);
	if ( p ) p->m_cert = rawdata;
	return p;
}

SslCertificate*
SslCertificate::s_createFile ( const char* path, const char* passwd )
{
	auto in(BIO_new_file(path, "rb"));
	if ( not in ) return nullptr;

	auto cert(PEM_read_bio_X509(in, nullptr, _pem_passwd_cb, const_cast<char*>(passwd)));
	BIO_free(in);

	if ( not cert ) return nullptr;

	auto ret( s_createRawData(cert) );

	if ( not ret )
	{
		X509_free(cert);
		return nullptr;
	}

	return ret;
}


SslCertificate*
SslCertificate::s_create(create_type ct, ...)
{
	va_list lst;
	va_start(lst, ct);

	SslCertificate* ret(nullptr);

	switch(ct)
	{
	case create_type::CT_RAW_DATA:
	{
		void* rawdata(va_arg(lst, void*));
		ret = s_createRawData(rawdata);
		break;
	}// case CT_RAW_DATA
	case create_type::CT_FILE:
	{
		char* path(va_arg(lst, char*));
		char* passwd(va_arg(lst, char*));
		ret = s_createFile(path, passwd);
		break;
	}// case CT_FILE
	}

	va_end(lst);

	return ret;
}

bool
SslCertificate::getSubject(std::string& out) const
{
	return _toString(out, ::X509_get_subject_name(THIS_X509()));
}

bool
SslCertificate::getSubject(kv_cont& out, bool sn) const
{
	return _toKeyValue(out, ::X509_get_subject_name(THIS_X509()), sn);
}

bool
SslCertificate::getIssuer(std::string& out) const
{
	return _toString(out, ::X509_get_issuer_name(THIS_X509()));
}

bool
SslCertificate::getIssuer(kv_cont& out, bool sn) const
{
	return _toKeyValue(out, ::X509_get_issuer_name(THIS_X509()), sn);
}

bool
SslCertificate::getSubjectHash(std::string& out) const
{
	long hash(getSubjectHash());
	PWStr::format(out, "%08lx", hash);
	return true;
}

uint32_t
SslCertificate::getSubjectHash(void) const
{
	return X509_subject_name_hash(THIS_X509());
}

bool
SslCertificate::getSerial(std::string& out) const
{
	char* buf(i2s_ASN1_INTEGER(nullptr, ::X509_get_serialNumber(THIS_X509())));
	if ( nullptr == buf ) return false;

	PWStr::format(out, "%s", buf);
	::OPENSSL_free(buf);
	return true;
}

int32_t
SslCertificate::getVersion(void) const
{
	return int32_t(::X509_get_version(THIS_X509()));
}

time_t
SslCertificate::getExpirationBefore(void) const
{
	::ASN1_TIME* atm(X509_get_notBefore(THIS_X509()));
	if ( nullptr == atm ) return time_t(-1);
	return _toTime((char*)atm->data, atm->length, atm->type == V_ASN1_UTCTIME);
}

time_t
SslCertificate::getExpirationAfter(void) const
{
	::ASN1_TIME* atm(X509_get_notAfter(THIS_X509()));
	if ( nullptr == atm ) return time_t(-1);
	return _toTime((char*)atm->data, atm->length, atm->type == V_ASN1_UTCTIME);
}

bool
SslCertificate::verifyExpiration(void) const
{
	time_t now(::time(nullptr));
	if ( this->getExpirationBefore() > now ) return false;
	if ( this->getExpirationAfter() < now ) return false;
	return true;
}


//------------------------------------------------------------------------------
// SslCertificateList
// certificate Authority List
SslCertificateList::~SslCertificateList()
{
	if ( m_cl )
	{
		auto ca(static_cast<cert_list_type*>(m_cl));
		sk_X509_free(ca);
		m_cl = nullptr;
	}
}

SslCertificateList*
SslCertificateList::s_createRawData ( void* rawdata )
{
	auto p(new SslCertificateList);
	if ( p ) p->m_cl = rawdata;
	return p;
}
//------------------------------------------------------------------------------
// SslCertificateStore
// 인증서 보관소
SslCertificateStore::~SslCertificateStore()
{
	if ( m_cs )
	{
		auto cs(static_cast<X509_STORE*>(m_cs));
		X509_STORE_free(cs);
		m_cs = nullptr;
	}
}

SslCertificateStore::self_ptr_type
SslCertificateStore::s_createRawData ( void* rawdata )
{
	auto p(new self_type);
	if ( p ) p->m_cs = rawdata;
	return p;
}

//------------------------------------------------------------------------------
// SslCertificateStoreContext
// 인증서 보관소 컨텍스트
SslCertificateStoreContext::~SslCertificateStoreContext()
{
	if ( m_ctx )
	{
		auto ctx(static_cast<X509_STORE_CTX*>(m_ctx));
		X509_STORE_CTX_free(ctx);
		m_ctx = nullptr;
	}
}

SslCertificateStoreContext::self_ptr_type
SslCertificateStoreContext::s_createRawData ( void* rawdata )
{
	auto p(new self_type);
	if ( p ) p->m_ctx = rawdata;
	return p;
}


//------------------------------------------------------------------------------
// SslAsymmetricKey
// 비밀키.
SslAsymmetricKey::~SslAsymmetricKey()
{
	if ( m_pkey )
	{
		EVP_PKEY_free(THIS_EVP_PKEY());
		m_pkey = nullptr;
	}
}

SslAsymmetricKey*
SslAsymmetricKey::s_createRawData ( void* rawdata )
{
	auto p(new SslAsymmetricKey);
	if ( p ) p->m_pkey = rawdata;
	return p;
}

SslAsymmetricKey*
SslAsymmetricKey::s_createNull ( void )
{
	SslAsymmetricKey* pkey(new SslAsymmetricKey());
	if ( not pkey ) return nullptr;

	auto inner(EVP_PKEY_new());
	do {
		if ( not inner ) break;

		pkey->m_pkey = inner;
		return pkey;
	} while (false);

	delete pkey;
	return nullptr;
}

SslAsymmetricKey*
SslAsymmetricKey::s_createGenerateRSA ( size_t keysize )
{
	SslAsymmetricKey* pkey(new SslAsymmetricKey());
	if ( not pkey ) return nullptr;

	auto inner( _genAsymKeyRSA(keysize) );
	do {
		if ( not inner ) break;

		pkey->m_pkey = inner;
		return pkey;
	} while (false);

	delete pkey;
	return nullptr;

}

SslAsymmetricKey*
SslAsymmetricKey::s_createGenerateCMAC ( const void* passwd, size_t passwd_len )
{
	SslAsymmetricKey* pkey(new SslAsymmetricKey());
	if ( not pkey ) return nullptr;

	auto inner( _getAsymKeyMAC(ssl::AsymKeyAlg::CMAC, (const char*)passwd, passwd_len) );
	do {
		if ( not inner ) break;

		pkey->m_pkey = inner;
		return pkey;
	} while (false);

	delete pkey;
	return nullptr;

}

SslAsymmetricKey*
SslAsymmetricKey::s_createGenerateHMAC ( const void* passwd, size_t passwd_len )
{
	SslAsymmetricKey* pkey(new SslAsymmetricKey());
	if ( not pkey ) return nullptr;

	auto inner( _getAsymKeyMAC(ssl::AsymKeyAlg::HMAC, (const char*)passwd, passwd_len) );
	do {
		if ( not inner ) break;

		pkey->m_pkey = inner;
		return pkey;
	} while (false);

	delete pkey;
	return nullptr;

}

SslAsymmetricKey*
SslAsymmetricKey::s_createPrivateKeyFile ( const char* fn, const char* pw )
{
	SslAsymmetricKey* pkey(new SslAsymmetricKey());
	if ( not pkey ) return nullptr;

	auto bio(BIO_new_file(fn, "rb"));
	do {
		auto inner(PEM_read_bio_PrivateKey(bio, nullptr, _pem_passwd_cb, const_cast<char*>(pw)));
		if ( not inner )
		{
			PWTRACE("failed to read pem file: %s", fn);
			break;
		}

		pkey->m_pkey = inner;
		BIO_free(bio);
		return pkey;
	} while (false);

	delete pkey;
	if ( bio ) BIO_free(bio);

	return nullptr;

}

SslAsymmetricKey*
SslAsymmetricKey::s_createPrivateKeyMemory ( const void* in, size_t ilen, const char* pw )
{
	SslAsymmetricKey* pkey(new SslAsymmetricKey());
	if ( not pkey ) return nullptr;

	auto bio(BIO_new_mem_buf(const_cast<void*>(in), ilen));
	do {
		if ( not bio )
		{
			PWTRACE("failed to open memory");
			break;
		}

		auto inner(PEM_read_bio_PrivateKey(bio, nullptr, _pem_passwd_cb, const_cast<char*>(pw)));
		if ( not inner )
		{
			PWTRACE("failed to read pem memory");
			break;
		}

		pkey->m_pkey = inner;
		BIO_free(bio);
		return pkey;
	} while (false);

	delete pkey;
	if ( bio ) BIO_free(bio);

	return nullptr;

}

SslAsymmetricKey*
SslAsymmetricKey::s_createPublicKeyFile ( const char* fn, const char* pw )
{
	SslAsymmetricKey* pkey(new SslAsymmetricKey());
	if ( not pkey ) return nullptr;

	auto bio(BIO_new_file(fn, "rb"));
	do {
		if ( not bio )
		{
			PWTRACE("failed to open file: %s", fn);
			break;
		}

		auto inner(PEM_read_bio_PUBKEY(bio, nullptr, _pem_passwd_cb, const_cast<char*>(pw)));
		if ( not inner )
		{
			PWTRACE("failed to read pem file: %s", fn);
			break;
		}

		pkey->m_pkey = inner;
		BIO_free(bio);
		return pkey;
	} while (false);

	delete pkey;
	if ( bio ) BIO_free(bio);

	return nullptr;

}

SslAsymmetricKey*
SslAsymmetricKey::s_createPublicKeyMemory ( const void* in, size_t ilen, const char* pw )
{
	SslAsymmetricKey* pkey(new SslAsymmetricKey());
	if ( not pkey ) return nullptr;

	auto bio(BIO_new_mem_buf(const_cast<void*>(in), ilen));
	do {
		if ( not bio )
		{
			PWTRACE("failed to open memory");
			break;
		}

		auto inner(PEM_read_bio_PUBKEY(bio, nullptr, _pem_passwd_cb, const_cast<char*>(pw)));
		if ( not inner )
		{
			PWTRACE("failed to read pem memory");
			break;
		}

		pkey->m_pkey = inner;
		BIO_free(bio);
		return pkey;
	} while (false);

	delete pkey;
	if ( bio ) BIO_free(bio);

	return nullptr;

}

SslAsymmetricKey*
SslAsymmetricKey::s_create(create_type ct, ...)
{
	va_list lst;
	va_start(lst, ct);
	SslAsymmetricKey* ret(nullptr);
	switch(ct)
	{
	case create_type::CT_RAW_DATA:
	{
		void* rawdata(va_arg(lst, void*));
		ret = s_createRawData(rawdata);
		break;
	}
	case create_type::CT_NULL:
	{
		ret = s_createNull();
		break;
	}
	case create_type::CT_GENERATE_RSA:
	{
		size_t keysize(va_arg(lst, size_t));
		ret = s_createGenerateRSA(keysize);
		break;
	}//case CT_RSA
	case create_type::CT_GENERATE_HMAC:
	{
		const char* passwd(va_arg(lst, char*));
		size_t passwd_len(va_arg(lst, size_t));
		ret = s_createGenerateHMAC(passwd, passwd_len);
		break;
	}//case CT_HMAC
	case create_type::CT_GENERATE_CMAC:
	{
		const char* passwd(va_arg(lst, char*));
		size_t passwd_len(va_arg(lst, size_t));
		ret = s_createGenerateCMAC(passwd, passwd_len);
		break;
	}//case CT_CMAC
	case create_type::CT_PRIVATE_KEY_FILE:
	{
		char* path(va_arg(lst, char*));
		char* passwd(va_arg(lst, char*));
		ret = s_createPrivateKeyFile(path, passwd);
		break;
	}// case CT_PRIVATE_KEY_FILE
	case create_type::CT_PUBLIC_KEY_FILE:
	{
		char* path(va_arg(lst, char*));
		char* passwd(va_arg(lst, char*));
		ret = s_createPublicKeyFile(path, passwd);
		break;
	}// case CT_FILE_PUBLIC
	case create_type::CT_PRIVATE_KEY_MEMORY:
	{
		void* in(va_arg(lst, void*));
		size_t ilen(va_arg(lst, size_t));
		char* passwd(va_arg(lst, char*));
		ret = s_createPrivateKeyMemory(in, ilen, passwd);
		break;
	}//CT_PRIVATE_KEY_MEMORY
	case create_type::CT_PUBLIC_KEY_MEMORY:
	{
		void* in(va_arg(lst, void*));
		size_t ilen(va_arg(lst, size_t));
		char* passwd(va_arg(lst, char*));
		ret = s_createPublicKeyMemory(in, ilen, passwd);
		break;
	}//CT_PUBLIC_KEY_MEMORY
	}//switch

	va_end(lst);

	return ret;
}

size_t
SslAsymmetricKey::getSize(void) const
{
	return size_t(EVP_PKEY_size(_toEVP_PKEY(m_pkey)));
}

ssl::AsymKeyAlg
SslAsymmetricKey::getType(void) const
{
	//auto type(EVP_PKEY_type(EVP_PKEY_id(_toEVP_PKEY(m_pkey))));
	auto type(EVP_PKEY_type(_toEVP_PKEY(m_pkey)->type));

	//PWTRACE("type=%d", type);

	switch(type)
	{
	case EVP_PKEY_RSA: return ssl::AsymKeyAlg::RSA;
	case EVP_PKEY_DSA: return ssl::AsymKeyAlg::DSA;
	case EVP_PKEY_DH: return ssl::AsymKeyAlg::DH;
	case EVP_PKEY_EC: return ssl::AsymKeyAlg::EC;
#ifdef EVP_PKEY_HMAC
	case EVP_PKEY_HMAC: return ssl::AsymKeyAlg::HMAC;
#endif
#ifdef EVP_PKEY_CMAC
	case EVP_PKEY_CMAC: return ssl::AsymKeyAlg::CMAC;
#endif
	}

	return ssl::AsymKeyAlg::INVALID;
}

void*
SslAsymmetricKey::_write_priv_pem ( SslAsymmetricKey::out_type& ostr, size_t& olen, crypto::CipherType ct, const blob_type* key, const blob_type* passwd ) const
{
	const EVP_CIPHER* e(nullptr);

	if ( ct not_eq crypto::CipherType::EMPTY )
	{
		crypto::cipher_spec cs;
		if ( not crypto::getCipherSpec(cs, ct) )
		{
			PWTRACE("no cipher");
			return nullptr;
		}

		e = reinterpret_cast<const EVP_CIPHER*>(cs.engine);
	} while (false);

	auto bio(BIO_new(BIO_s_mem()));
	if ( not bio )
	{
		PWTRACE("failed to create bio");
		return nullptr;
	}

	do {
		unsigned char* in_key(key ? reinterpret_cast<unsigned char*>(const_cast<char*>(key->buf)) : nullptr );
		size_t in_key_size(key ? key->size : 0);
		auto pkey(static_cast<EVP_PKEY*>(const_cast<void*>(m_pkey)));

		if ( PEM_write_bio_PrivateKey ( bio, pkey, e, in_key, in_key_size, _pem_passwd_cb_blob, const_cast<blob_type*>(passwd)) <= 0 )
		{
			PWTRACE("PEM_write_bio_PrivateKey failed");
			break;
		}

		char* out(nullptr);
		auto len(BIO_get_mem_data(bio, &out));
		if ( len > 0 )
		{
			ostr = out;
		}

		olen = size_t(len);
		return bio;
	} while (false);

	BIO_free(bio);

	return nullptr;
}

void*
SslAsymmetricKey::_write_pub_pem ( SslAsymmetricKey::out_type& ostr, size_t& olen ) const
{
	auto bio(BIO_new(BIO_s_mem()));

	do {
		if ( not bio )
		{
			PWTRACE("failed to create bio");
			break;
		}

		if ( PEM_write_bio_PUBKEY(bio, _toEVP_PKEY(m_pkey)) <= 0 )
		{
			PWTRACE("PEM_write_bio_PUBKEY failed");
			break;
		}

		char* out(nullptr);
		auto len(BIO_get_mem_data(bio, &out));
		if ( len > 0 )
		{
			ostr = out;
		}

		olen = size_t(len);
		return bio;
	} while (false);

	if ( bio ) BIO_free(bio);

	return nullptr;
}

ssize_t
SslAsymmetricKey::writePublicKeyAsDER ( std::ostream& os ) const
{
	unsigned char* r(nullptr);
	int res(0);
	if ( (res = i2d_PUBKEY(_toEVP_PKEY(m_pkey), &r)) <= 0 )
	{
		PWTRACE("failed to write to stream");
		return -1;
	}

	if ( r and (res > 0) )
	{
		os.write((char*)r, res);
		::OPENSSL_free(r);
	}

	return ssize_t(res);
}

ssize_t
SslAsymmetricKey::writePublicKeyAsDER ( std::string& os ) const
{
	os.clear();

	unsigned char* r(nullptr);
	int res(0);
	if ( (res = i2d_PUBKEY(_toEVP_PKEY(m_pkey), &r)) <= 0 )
	{
		PWTRACE("failed to write to stream");
		return -1;
	}

	if ( r and (res > 0) )
	{
		os.assign((char*)r, res);
		::OPENSSL_free(r);
	}

	return ssize_t(res);
}

ssize_t
SslAsymmetricKey::writePrivateKeyAsDER ( std::ostream& os ) const
{
	unsigned char* r(nullptr);
	int res(0);
	if ( (res = i2d_PrivateKey(_toEVP_PKEY(m_pkey), &r)) <= 0 )
	{
		PWTRACE("failed to write to stream");
		return -1;
	}

	if ( r and (res > 0) )
	{
		os.write((char*)r, res);
		::OPENSSL_free(r);
	}

	return ssize_t(res);
}

ssize_t
SslAsymmetricKey::writePrivateKeyAsDER ( std::string& os ) const
{
	os.clear();

	unsigned char* r(nullptr);
	int res(0);
	if ( (res = i2d_PrivateKey(_toEVP_PKEY(m_pkey), &r)) <= 0 )
	{
		PWTRACE("failed to write to stream");
		return -1;
	}

	if ( r and (res > 0) )
	{
		os.assign((char*)r, res);
		::OPENSSL_free(r);
	}

	return ssize_t(res);
}

ssize_t
SslAsymmetricKey::writePublicKeyAsPEM ( std::ostream& os ) const
{
	char* out(nullptr);
	size_t olen(0);
	auto bio(_write_pub_pem(out, olen));
	if ( bio )
	{
		if ( olen ) os.write(out, olen);
		BIO_free(static_cast<BIO*>(bio));
		return ssize_t(olen);
	}

	return -1;
}

ssize_t
SslAsymmetricKey::writePublicKeyAsPEM ( std::string& os ) const
{
	char* out(nullptr);
	size_t olen(0);
	auto bio(_write_pub_pem(out, olen));
	os.clear();
	if ( bio )
	{
		if ( olen ) os.assign(out, olen);

		BIO_free(static_cast<BIO*>(bio));
		return ssize_t(olen);
	}

	return -1;
}

ssize_t
SslAsymmetricKey::writePrivateKeyAsPEM ( std::ostream& os, crypto::CipherType ct, const blob_type* key, const blob_type* passwd ) const
{
	char* out(nullptr);
	size_t olen(0);
	auto bio(_write_priv_pem(out, olen, ct, key, passwd));
	if ( bio )
	{
		if ( olen ) os.write(out, olen);

		BIO_free(static_cast<BIO*>(bio));
		return ssize_t(olen);
	}

	return -1;
}

ssize_t
SslAsymmetricKey::writePrivateKeyAsPEM ( std::string& os, crypto::CipherType ct, const blob_type* key, const blob_type* passwd ) const
{
	char* out(nullptr);
	size_t olen(0);
	auto bio(_write_priv_pem(out, olen, ct, key, passwd));
	os.clear();
	if ( bio )
	{
		if ( olen ) os.assign(out, olen);

		BIO_free(static_cast<BIO*>(bio));
		return ssize_t(olen);
	}

	return -1;
}

bool
SslAsymmetricKey::_enc(out_type& ostr, size_t& olen, const void* _in, size_t ilen, ssl::RSAPadding pt, bool direction) const
{
#ifdef HAVE_EVP_PKEY_CTX_NEW_ID
	auto pkey(static_cast<EVP_PKEY*>(const_cast<void*>(m_pkey)));
	auto ctx(EVP_PKEY_CTX_new(pkey, nullptr));

	if ( not ctx )
	{
		PWTRACE("EVP_PKEY_CTX_new failed");
		return false;
	}

	auto ctx_init_func( direction ? EVP_PKEY_encrypt_init : EVP_PKEY_decrypt_init );
	auto ctx_enc_func( direction ? EVP_PKEY_encrypt : EVP_PKEY_decrypt );

	do {
		if ( ctx_init_func(ctx) <= 0 )
		{
			PWTRACE("EVP_PKEY_encrypt_init failed");
			break;
		}

		if ( EVP_PKEY_CTX_set_rsa_padding(ctx, _toRSAPadding(pt) ) <= 0 )
		{
			PWTRACE("EVP_PKEY_CTX_set_rsa_padding failed");
			break;
		}

		size_t outlen(0);
		auto in(reinterpret_cast<const unsigned char*>(_in));
		if ( ctx_enc_func(ctx, nullptr, &outlen, in, ilen) <= 0 )
		{
			PWTRACE("EVP_PKEY_encrypt? with null failed");
			PWTRACE("%s", ssl::getLastErrorString());
			break;
		}

		if ( outlen )
		{
			unsigned char* out((unsigned char*)::malloc(outlen));
			if ( not out )
			{
				PWTRACE("not enough memory");
				break;
			}

			if ( ctx_enc_func(ctx, out, &outlen, in, ilen) <= 0 )
			{
				PWTRACE("EVP_PKEY_encrypt? failed");
				::free(out);
				break;
			}

			ostr = reinterpret_cast<char*>(out);
		}

		olen = outlen;

		EVP_PKEY_CTX_free(ctx);
		return true;
	} while (false);

	EVP_PKEY_CTX_free(ctx);

	return false;
#else
	return false;
#endif
}

ssize_t
SslAsymmetricKey::encrypt ( ostream& os, const void* in, size_t ilen, ssl::RSAPadding pt ) const
{
	char* out(nullptr);
	size_t outlen(0);
	if ( _enc(out, outlen, in, ilen, pt, true) )
	{
		if ( outlen )
		{
			os.write(out, outlen);
			::free(out);
		}

		return ssize_t(outlen);
	}

	return -1;
}

ssize_t
SslAsymmetricKey::encrypt ( string& os, const void* in, size_t ilen, ssl::RSAPadding pt ) const
{
	char* out(nullptr);
	size_t outlen(0);
	if ( _enc(out, outlen, in, ilen, pt, true) )
	{
		if ( outlen )
		{
			os.assign(out, outlen);
			::free(out);
		}

		return ssize_t(outlen);
	}

	return -1;
}

ssize_t
SslAsymmetricKey::encrypt ( char* obuf, size_t* olen, const void* in, size_t ilen, ssl::RSAPadding pt ) const
{
	char* out(nullptr);
	size_t outlen(0);
	if ( _enc(out, outlen, in, ilen, pt, true) )
	{
		if ( outlen )
		{
			::memcpy(obuf, out, outlen);
			::free(out);
		}

		if ( olen ) *olen = outlen;

		return ssize_t(outlen);
	}

	return -1;
}

ssize_t
SslAsymmetricKey::decrypt ( ostream& os, const void* in, size_t ilen, ssl::RSAPadding pt ) const
{
	char* out(nullptr);
	size_t outlen(0);
	if ( _enc(out, outlen, in, ilen, pt, false) )
	{
		if ( outlen )
		{
			os.write(out, outlen);
			::free(out);
		}

		return ssize_t(outlen);
	}

	return -1;
}

ssize_t
SslAsymmetricKey::decrypt ( string& os, const void* in, size_t ilen, ssl::RSAPadding pt ) const
{
	char* out(nullptr);
	size_t outlen(0);
	if ( _enc(out, outlen, in, ilen, pt, false) )
	{
		if ( outlen )
		{
			os.assign(out, outlen);
			::free(out);
		}

		return ssize_t(outlen);
	}

	return -1;
}

ssize_t
SslAsymmetricKey::decrypt ( char* obuf, size_t* olen, const void* in, size_t ilen, ssl::RSAPadding pt ) const
{
	char* out(nullptr);
	size_t outlen(0);
	if ( _enc(out, outlen, in, ilen, pt, false) )
	{
		if ( outlen )
		{
			::memcpy(obuf, out, outlen);
			::free(out);
		}

		if ( olen ) *olen = outlen;

		return ssize_t(outlen);
	}

	return -1;
}

//------------------------------------------------------------------------------
// SslSign
// Signing

SslSign::~SslSign()
{
	if ( m_ctx )
	{
		auto ctx(reinterpret_cast<sv_context_type*>(m_ctx));

		if ( ctx->ctx ) EVP_MD_CTX_destroy(ctx->ctx);

		delete ctx;

		ctx = nullptr;
	}
}

SslSign*
SslSign::s_createRSA_PKCS1(SslAsymmetricKey& in_pkey, digest::DigestType in_ht)
{
	auto s(new SslSign);
	if ( not s ) return nullptr;

	if ( _create_SV_RSA(&(s->m_ctx), true, in_pkey, in_ht, ssl::RSAPadding::PKCS1, 0) )
	{
		return s;
	}

	delete s;
	return nullptr;
}

SslSign*
SslSign::s_createRSA_PKCS1_PSS ( SslAsymmetricKey& in_pkey, digest::DigestType in_ht, size_t saltlen )
{
	auto s(new SslSign);
	if ( not s ) return nullptr;

	if ( _create_SV_RSA(&(s->m_ctx), true, in_pkey, in_ht, ssl::RSAPadding::PKCS1_PSS, saltlen) )
	{
		return s;
	}

	delete s;
	return nullptr;
}

SslSign*
SslSign::s_createRSA_X931 ( SslAsymmetricKey& in_pkey, digest::DigestType in_ht )
{
	auto s(new SslSign);
	if ( not s ) return nullptr;

	if ( _create_SV_RSA(&(s->m_ctx), true, in_pkey, in_ht, ssl::RSAPadding::X931, 0) )
	{
		return s;
	}

	delete s;
	return nullptr;
}

const void*
SslSign::getRawDataMD ( void ) const
{
	return static_cast<const sv_context_type*>(m_ctx)->md;
}

void*
SslSign::getRawDataMDContext ( void )
{
	return static_cast<sv_context_type*>(m_ctx)->ctx;
}

const void*
SslSign::getRawDataMDContext ( void ) const
{
	return static_cast<const sv_context_type*>(m_ctx)->ctx;
}

void*
SslSign::getRawDataAsymKey ( void )
{
	return static_cast<sv_context_type*>(m_ctx)->pkey;
}

const void*
SslSign::getRawDataAsymKey ( void ) const
{
	return static_cast<const sv_context_type*>(m_ctx)->pkey;
}

bool
SslSign::reinitialize(void)
{
#ifdef HAVE_EVP_DIGESTSIGNINIT
	auto pkey_type(static_cast<sv_context_type*>(m_ctx)->pkey_type);
	if ( pkey_type == EVP_PKEY_RSA ) return _reinit_SV_RSA(m_ctx, true);
	return false;
#else
	return false;
#endif
}

bool
SslSign::update(const void* in, size_t ilen)
{
#ifdef HAVE_EVP_DIGESTSIGNINIT
	auto ctx(static_cast<sv_context_type*>(m_ctx)->ctx);
	return (1 == EVP_DigestSignUpdate(ctx, in, ilen));
#else
	return false;
#endif
}

bool
SslSign::finalize(void* out, size_t* olen)
{
#ifdef HAVE_EVP_DIGESTSIGNINIT
	auto ctx(static_cast<sv_context_type*>(m_ctx)->ctx);
	size_t s(olen?*olen:0);
	const auto res( EVP_DigestSignFinal(ctx, static_cast<unsigned char*>(out), &s) );

	if ( res == 1 )
	{
		if ( olen ) *olen = s;
		return true;
	}

	return false;
#else
	return false;
#endif
}

bool
SslSign::finalize(std::string& out)
{
#ifdef HAVE_EVP_DIGESTSIGNINIT
	auto ctx(static_cast<sv_context_type*>(m_ctx)->ctx);
	size_t olen(0);
	if ( EVP_DigestSignFinal(ctx, nullptr, &olen) <= 0 )
	{
		PWTRACE("EVP_DigestSignFinal(nullptr) failed");
		return false;
	}

	if ( olen )
	{
		std::string tmp(olen, 0x00);
		auto obuf(reinterpret_cast<unsigned char*>(const_cast<char*>(tmp.c_str())));

		if ( EVP_DigestSignFinal(ctx, obuf, &olen) <= 0 )
		{
			PWTRACE("EVP_DigestSignFinal failed");
			return false;
		}

		out.swap(tmp);
	}
	else
	{
		out.clear();
	}

	return true;
#else
	return false;
#endif
}

//------------------------------------------------------------------------------
// SslVerify
// Verifying

SslVerify::~SslVerify()
{
	if ( m_ctx )
	{
		auto ctx(reinterpret_cast<sv_context_type*>(m_ctx));

		if ( ctx->ctx ) EVP_MD_CTX_destroy(ctx->ctx);

		delete ctx;

		ctx = nullptr;
	}
}

SslVerify*
SslVerify::s_createRSA_PKCS1(SslAsymmetricKey& in_pkey, digest::DigestType in_ht)
{
	auto s(new SslVerify);
	if ( not s ) return nullptr;

	if ( _create_SV_RSA(&(s->m_ctx), false, in_pkey, in_ht, ssl::RSAPadding::PKCS1, 0) )
	{
		return s;
	}

	delete s;
	return nullptr;
}

SslVerify*
SslVerify::s_createRSA_PKCS1_PSS ( SslAsymmetricKey& in_pkey, digest::DigestType in_ht, size_t saltlen )
{
	auto s(new SslVerify);
	if ( not s ) return nullptr;

	if ( _create_SV_RSA(&(s->m_ctx), false, in_pkey, in_ht, ssl::RSAPadding::PKCS1_PSS, saltlen) )
	{
		return s;
	}

	delete s;
	return nullptr;
}

const void*
SslVerify::getRawDataMD ( void ) const
{
	return static_cast<const sv_context_type*>(m_ctx)->md;
}

void*
SslVerify::getRawDataMDContext ( void )
{
	return static_cast<sv_context_type*>(m_ctx)->ctx;
}

const void*
SslVerify::getRawDataMDContext ( void ) const
{
	return static_cast<const sv_context_type*>(m_ctx)->ctx;
}

void*
SslVerify::getRawDataAsymKey ( void )
{
	return static_cast<sv_context_type*>(m_ctx)->pkey;
}

const void*
SslVerify::getRawDataAsymKey ( void ) const
{
	return static_cast<const sv_context_type*>(m_ctx)->pkey;
}

bool
SslVerify::reinitialize(void)
{
#ifdef HAVE_EVP_DIGESTVERIFYINIT
	auto pkey_type(static_cast<sv_context_type*>(m_ctx)->pkey_type);
	if ( pkey_type == EVP_PKEY_RSA ) return _reinit_SV_RSA(m_ctx, false);
	return false;
#else
	return false;
#endif
}

bool
SslVerify::update(const void* in, size_t ilen)
{
#ifdef HAVE_EVP_DIGESTVERIFYINIT
	auto ctx(static_cast<sv_context_type*>(m_ctx)->ctx);
	return (1 == EVP_DigestVerifyUpdate(ctx, in, ilen));
#else
	return false;
#endif
}

bool
SslVerify::finalize ( const void* sign, size_t sign_len )
{
#ifdef HAVE_EVP_DIGESTVERIFYINIT
	auto ctx(static_cast<sv_context_type*>(m_ctx)->ctx);
	unsigned char sign_cpy[sign_len];
	::memcpy(sign_cpy, sign, sign_len);
	return ( 1 == EVP_DigestVerifyFinal(ctx, sign_cpy, sign_len) );
#else
	return false;
#endif
}

};//namespace pw

