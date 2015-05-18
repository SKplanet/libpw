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
 * \file pw_ssl.h
 * \brief Support SSL/TLS protocols and certificates with openssl(http://openssl.org).
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"
#include "./pw_ini.h"
#include "./pw_digest.h"
#include "./pw_crypto.h"

#ifndef __PW_SSL_H__
#define __PW_SSL_H__

namespace pw {

class SslContext;
class SslCertificate;
class SslCertificateStoreContext;
class SslAsymmetricKey;
class SslSign;
class SslVerify;
class SslCertificateList;

namespace ssl {

//! \brief SSL 알고리즘 및 엔진 초기화
extern bool initialize(void);

//! \brief 검증 함수 타입
using verify_func_type = int (*) (int, const SslCertificateStoreContext*);

//! \brief 검증 타입.
class VerifyMode final
{
public:
	inline VerifyMode() = default;
	inline VerifyMode(const VerifyMode&) = default;
	inline VerifyMode(VerifyMode&&) = default;

	VerifyMode& operator = (const VerifyMode&) = default;
	VerifyMode& operator = (VerifyMode&&) = default;

	inline void clear(void) { m_mode = 0; }
	VerifyMode& addFlagPeer(void);
	VerifyMode& addFlagFailIfNoPeerCert(void);
	VerifyMode& addFlagClientOnce(void);

	inline int getFlags(void) const { return m_mode; }

private:
	int m_mode = 0;
};

//! \brief 메소드 타입.
enum class Method : int
{
	INVALID = 0, //!< 잘못된 메소드
	CLIENT_V2,
	SERVER_V2,
	BOTH_V2,
	CLIENT_V3,
	SERVER_V3,
	BOTH_V3,
	CLIENT_V2_V3,
	SERVER_V2_V3,
	BOTH_V2_V3,
	CLIENT_T1,
	SERVER_T1,
	BOTH_T1,
	CLIENT_T1_1,
	SERVER_T1_1,
	BOTH_T1_1,
	CLIENT_T1_2,
	SERVER_T1_2,
	BOTH_T1_2,
	CLIENT_T1_3,
	SERVER_T1_3,
	BOTH_T1_3
};

//! \brief 사이드 타입.
enum class Side : int
{
	CLIENT,
	SERVER,
};

//! \brief 패딩 타입.
enum class RSAPadding : int
{
	INVALID = 0,
	NONE,	//!< No padding
	PKCS1,	//!< PKCS#1
	PKCS1_OAEP,	//!< PKCS#1 with OEAP
	PKCS1_PSS,	//!< PKCS#1 with PSS
	X931,	//!< X9.31
	SSL_V2_V3,	//!< SSLv2 v3
};

//! \brief 사이닝 타입.
enum class Sign : int
{
	SHA1,
	RIPEMD160,
	MD5,
	MD5_SHA1,
};

//! \brief 키 알고리즘
enum class AsymKeyAlg : int
{
	INVALID = 0,
	RSA,
	DSA,
 	DH,
 	EC,
	HMAC,
	CMAC
};

extern const char* toStringA(AsymKeyAlg ka);
extern std::string toString(AsymKeyAlg ka);
extern std::string& toString(std::string& out, AsymKeyAlg ka);
extern AsymKeyAlg toAsymKeyAlg(const char* s);
extern AsymKeyAlg toAsymKeyAlg(const std::string& s);

struct pkcs12_load_param_type
{
	struct
	{
		SslAsymmetricKey* key = nullptr;
		SslCertificate* cert = nullptr;
		SslCertificateList* cert_list = nullptr;
	} out;

	struct
	{
		const char* passwd = nullptr;

		struct {
			bool key = true;
			bool cert = true;
			bool cert_list = true;
		} need;
	} in;
};

extern bool loadPKCS12File(pkcs12_load_param_type& inout, const char* path);

//! \brief 클라이언트측 최신 메소드
inline
constexpr
Method
getLastClientMethod(void)
{
#ifdef HAVE_TLSV1_3_METHOD
	return Method::CLIENT_T1_3;
#elif HAVE_TLSV1_2_METHOD
	return Method::CLIENT_T1_2;
#elif HAVE_TLSV1_1_METHOD
	return Method::CLIENT_T1_1;
#elif HAVE_TLSV1_METHOD
	return Method::CLIENT_T1;
#else
	return Method::CLIENT_V3;
#endif
}

//! \brief 서버측 최신 메소드
inline
constexpr
Method
getLastServerMethod(void)
{
#ifdef HAVE_TLSV1_3_METHOD
	return Method::SERVER_T1_3;
#elif HAVE_TLSV1_2_METHOD
	return Method::SERVER_T1_2;
#elif HAVE_TLSV1_1_METHOD
	return Method::SERVER_T1_1;
#elif HAVE_TLSV1_METHOD
	return Method::SERVER_T1;
#else
	return Method::SERVER_V3;
#endif
}

//! \brief 양단 최신 메소드
inline
constexpr
Method
getLastBothMethod(void)
{
#ifdef HAVE_TLSV1_3_METHOD
	return Method::BOTH_T1_3;
#elif HAVE_TLSV1_2_METHOD
	return Method::BOTH_T1_2;
#elif HAVE_TLSV1_1_METHOD
	return Method::BOTH_T1_1;
#elif HAVE_TLSV1_METHOD
	return Method::BOTH_T1;
#else
	return Method::BOTH_V3;
#endif
}

inline
constexpr
digest::DigestType
getDefaultSignHash(void)
{
	return digest::DigestType::SHA256;
}

inline
constexpr
size_t
getDefaultRSA_PSS_SaltSize(void)
{
	return size_t(20);
}

//! \brief Ssl 환경설정 세트
struct context_type final
{
	SslContext*	ctx = nullptr;//!< Context
	std::string	method;		//!< Method
	std::string	ca_file;	//!< CAfile
	std::string	ca_path;	//!< CApath
	std::string	cipher_list;//!< Cipherlist
	std::string	cert_file;	//!< Certificate file
	std::string	cert_passwd;//!< Certificate password
	std::string	pkey_file;	//!< Privatekey file
	std::string	pkey_passwd;//!< Privatekey password

	inline context_type() = default;
	~context_type();

	context_type(const context_type&) = delete;
	context_type(context_type&&) = delete;
	context_type& operator = (const context_type&) = delete;
	context_type& operator = (context_type&&) = delete;

	//! \brief 설정 초기화
	void clear(void);

	//! \brief 준비 되었는지 확인한다.
	inline bool isReady(void) const { return ctx not_eq nullptr; }

	//! \brief SSL 설정을 INI 파일에서 읽고 초기화한다.
	//! \param[in] ini INI 객체
	//! \param[in] prefix 아이템 이름 prefix.
	//! \param[in] sec 섹션 반복자.
	//! \return 성공하면 true를 반환한다.
	bool read(const Ini& ini, const char* prefix, Ini::sec_citr sec);
	bool read(const Ini& ini, const char* prefix, const std::string& sec);

	//! \brief Swap
	void swap(context_type& v);
};

//! \brief 마지막 SSL 오류 문자열 반환.
std::string& getLastErrorString(std::string& out);

//! \brief 마지막 SSL 오류 문자열 반환.
const char* getLastErrorString(void);

//! \brief 메소드를 문자열로 반환.
const char* getMethodString(Method type);

//! \brief 문자열을 메소드로 반환.
Method getMethod(const char* type);

//namespace ssl
};

//! \brief SSL 상태머신.
class Ssl final
{
public:
	//! \brief Ssl 생성하기.
	static Ssl* s_create(const SslContext& ctx);
	inline static Ssl* s_create(const SslContext* ctx) { return s_create(*ctx); }

	//! \brief Ssl 해제하기.
	inline static void s_release(Ssl* v) { delete v; }

	~Ssl();

public:
	inline void release(void) { delete this; }
	inline void swap(Ssl& v) { std::swap(m_ssl, v.m_ssl); }
	bool reset(void);
	bool setFD(int fd);
	bool setReadFD(int fd);
	bool setWriteFD(int fd);
	bool connect(int* need);
	bool accept(int* need);
	bool handshake(int* need = nullptr);
	ssize_t read(char* buf, size_t blen);
	ssize_t write(const char* buf, size_t blen);

public:
	SslCertificate* getPeerCertificate(void) const;
	bool verifyPeerCertificate(void) const;
	const void* getCipher(void) const;
	const char* getCipherName(void) const;
	size_t getCipherBits(void) const;
	const char* getCipherVersion(void) const;
	void setVerify(const ssl::VerifyMode& v, ssl::verify_func_type func = nullptr);
	void setVerifyDepth(size_t depth = 3);

private:
	inline Ssl() = default;

	Ssl(const Ssl&) = delete;
	Ssl(Ssl&&) = delete;
	Ssl& operator = (const Ssl&) = delete;
	Ssl& operator = (Ssl&&) = delete;

private:
	void*	m_ssl = nullptr;
};

//! \brief SSL 컨텍스트
class SslContext final
{
public:
	static SslContext* s_create(ssl::Method type);
	inline static SslContext* s_create(const char* type) { return s_create( ssl::getMethod(type) ); }
	inline static void s_release(SslContext* v) { delete v; }

	~SslContext();

public:
	inline Ssl* create(void) { return Ssl::s_create(this); }
	inline void release(void) { delete this; }
	inline void swap(SslContext& v) { std::swap(m_ctx, v.m_ctx); }
	bool setVerifyLocation(const char* file, const char* path = nullptr);
	bool setClientCAList(const char* file);
	bool addExtraChainCertificate(const SslCertificate& cert);
	bool setCertificate(const SslCertificate& cert);
	bool setCertificate(const char* path, const char* passwd = nullptr);
	bool setPrivateKey(const SslAsymmetricKey& pkey);
	bool setPrivateKey(const char* path, const char* passwd = nullptr);
	bool verifyPrivateKey(void) const;
	bool setCipherList(const char* lst);
	void setVerify(const ssl::VerifyMode& v, ssl::verify_func_type func = nullptr);
	void setVerifyDepth(size_t depth = 3);

private:
	inline SslContext()  = default;

	SslContext(const SslContext&) = delete;
	SslContext(SslContext&&) = delete;
	SslContext& operator = (const SslContext&) = delete;
	SslContext& operator = (SslContext&&) = delete;

private:
	void*	m_ctx = nullptr;

friend class Ssl;
};

//! \brief X509 인증서
class SslCertificate final
{
public:
	enum create_type
	{
		CT_RAW_DATA,
		CT_FILE,
	};

	using line_cont = std::list<std::string>;
	using kv_cont = std::map<std::string, line_cont>;

public:
	static SslCertificate* s_create(create_type ct, ...);
	static SslCertificate* s_createRawData(void* rawdata);
	static SslCertificate* s_createFile(const char* path, const char* passwd = nullptr);
	inline static void s_release(SslCertificate* v) { delete v; }

	~SslCertificate();

public:
	inline void release(void) { delete this; }
	inline void swap(SslCertificate& v) { std::swap(m_cert, v.m_cert); }
	bool getSubject(std::string& out) const;
	bool getSubject(kv_cont& cont, bool sn = false) const;
	bool getIssuer(std::string& out) const;
	bool getIssuer(kv_cont& cont, bool sn = false) const;
	bool getSubjectHash(std::string& out) const;
	uint32_t getSubjectHash(void) const;
	bool getSerial(std::string& out) const;
	int32_t getVersion(void) const;
	time_t getExpirationBefore(void) const;
	time_t getExpirationAfter(void) const;
	bool verifyExpiration(void) const;

private:
	inline SslCertificate() = default;

	SslCertificate(const SslCertificate&) = delete;
	SslCertificate(SslCertificate&&) = delete;
	SslCertificate& operator = (const SslCertificate&) = delete;
	SslCertificate& operator = (SslCertificate&&) = delete;

private:
	void* m_cert = nullptr;

friend class Ssl;
friend class SslContext;
};

//! \brief 인증서 목록(STACK_OF(X509))
class SslCertificateList final
{
public:
	using self_type = SslCertificateList;
	using self_ptr_type = SslCertificateList*;

public:
	static self_ptr_type s_createRawData(void* rawdata);
	~SslCertificateList();

public:
	inline void* getRawData(void) { return m_cl; }
	inline const void* getRawData(void) const { return m_cl; }

private:
	void* m_cl = nullptr;
};

//! \brief Certificate store
class SslCertificateStore final
{
public:
	using self_type = SslCertificateStore;
	using self_ptr_type = SslCertificateStore*;

public:
	static self_ptr_type s_createRawData(void* rawdata);
	~SslCertificateStore();

public:
	bool setCertificate(SslCertificate& in);

private:
	void* m_cs = nullptr;
};

//! \brief Certificate store context
class SslCertificateStoreContext final
{
public:
	using self_type = SslCertificateStoreContext;
	using self_ptr_type = SslCertificateStoreContext*;

public:
	static self_ptr_type s_createRawData(void* rawdata);
	~SslCertificateStoreContext();

private:
	void* m_ctx = nullptr;
};

//! \brief 비대칭키
class SslAsymmetricKey final
{
public:
	enum create_type
	{
		CT_NULL,
		CT_RAW_DATA,
		CT_GENERATE_RSA,
		CT_GENERATE_HMAC,
		CT_GENERATE_CMAC,
		CT_FILE,
		CT_PRIVATE_KEY_FILE = CT_FILE,
		CT_PUBLIC_KEY_FILE,
		CT_MEMORY,
		CT_PRIVATE_KEY_MEMORY = CT_MEMORY,
		CT_PUBLIC_KEY_MEMORY
	};

public:
	static SslAsymmetricKey* s_create(create_type ct, ...);
	static SslAsymmetricKey* s_createNull(void);
	static SslAsymmetricKey* s_createRawData(void* rawdata);
	static SslAsymmetricKey* s_createGenerateRSA(size_t keysize);
	static SslAsymmetricKey* s_createGenerateHMAC(const void* passwd, size_t passwd_len);
	static SslAsymmetricKey* s_createGenerateCMAC(const void* passwd, size_t passwd_len);
	static SslAsymmetricKey* s_createPrivateKeyFile(const char* fn, const char* pw = nullptr);
	static SslAsymmetricKey* s_createPublicKeyFile(const char* fn, const char* pw = nullptr);
	static SslAsymmetricKey* s_createPrivateKeyMemory(const void* in, size_t ilen, const char* pw = nullptr);
	static SslAsymmetricKey* s_createPublicKeyMemory(const void* in, size_t ilen, const char* pw = nullptr);
	inline static void s_release(SslAsymmetricKey* v) { delete v; }

	virtual ~SslAsymmetricKey();

public:
	inline void release(void) { delete this; }
	inline void swap(SslAsymmetricKey& v) { std::swap(m_pkey, v.m_pkey); }

	ssl::AsymKeyAlg getType(void) const;
	size_t getSize(void) const;

public:
	ssize_t writePublicKeyAsDER(std::ostream& os) const;
	ssize_t writePublicKeyAsDER(std::string& ostr) const;
	ssize_t writePrivateKeyAsDER(std::ostream& os) const;
	ssize_t writePrivateKeyAsDER(std::string& ostr) const;

	ssize_t writePublicKeyAsPEM(std::ostream& os) const;
	ssize_t writePublicKeyAsPEM(std::string& ostr) const;
	ssize_t writePrivateKeyAsPEM(std::ostream& os, crypto::CipherType ct = crypto::CipherType::EMPTY, const blob_type* key = nullptr, const blob_type* passwd = nullptr) const;
	ssize_t writePrivateKeyAsPEM(std::string& ostr, crypto::CipherType ct = crypto::CipherType::EMPTY, const blob_type* key = nullptr, const blob_type* passwd = nullptr) const;

	ssize_t encrypt(std::ostream& os, const void* in, size_t ilen, ssl::RSAPadding pt = ssl::RSAPadding::PKCS1_OAEP) const;
	inline ssize_t encrypt(std::ostream& os, const std::string& in, ssl::RSAPadding pt = ssl::RSAPadding::PKCS1_OAEP) const { return this->encrypt(os, in.c_str(), in.size(), pt); }
	ssize_t encrypt(std::string& os, const void* in, size_t ilen, ssl::RSAPadding pt = ssl::RSAPadding::PKCS1_OAEP) const;
	inline ssize_t encrypt(std::string& os, const std::string& in, ssl::RSAPadding pt = ssl::RSAPadding::PKCS1_OAEP) const { return this->encrypt(os, in.c_str(), in.size(), pt); }
	ssize_t encrypt(char* obuf, size_t* olen, const void* in, size_t ilen, ssl::RSAPadding pt = ssl::RSAPadding::PKCS1_OAEP) const;
	inline ssize_t encrypt(char* obuf, size_t* olen, const std::string& in, ssl::RSAPadding pt = ssl::RSAPadding::PKCS1_OAEP) const { return this->encrypt(obuf, olen, in.c_str(), in.size(), pt); }

	ssize_t decrypt(std::ostream& os, const void* in, size_t ilen, ssl::RSAPadding pt = ssl::RSAPadding::PKCS1_OAEP) const;
	inline ssize_t decrypt(std::ostream& os, const std::string& in, ssl::RSAPadding pt = ssl::RSAPadding::PKCS1_OAEP) const { return this->decrypt(os, in.c_str(), in.size(), pt); }
	ssize_t decrypt(std::string& os, const void* in, size_t ilen, ssl::RSAPadding pt = ssl::RSAPadding::PKCS1_OAEP) const;
	inline ssize_t decrypt(std::string& os, const std::string& in, ssl::RSAPadding pt = ssl::RSAPadding::PKCS1_OAEP) const { return this->decrypt(os, in.c_str(), in.size(), pt); }
	ssize_t decrypt(char* obuf, size_t* olen, const void* in, size_t ilen, ssl::RSAPadding pt = ssl::RSAPadding::PKCS1_OAEP) const;
	inline ssize_t decrypt(char* obuf, size_t* olen, const std::string& in, ssl::RSAPadding pt = ssl::RSAPadding::PKCS1_OAEP) const { return this->decrypt(obuf, olen, in.c_str(), in.size(), pt); }

public:
	inline bool isRSA(void) const { return (getType() == ssl::AsymKeyAlg::RSA); }
	inline bool isDH(void) const { return (getType() == ssl::AsymKeyAlg::DH); }
	inline bool isDSA(void) const { return (getType() == ssl::AsymKeyAlg::DSA); }
	inline bool isEC(void) const { return (getType() == ssl::AsymKeyAlg::EC); }

	inline void* getRawData(void) { return m_pkey; }
	inline const void* getRawData(void) const { return m_pkey; }

private:
	inline explicit SslAsymmetricKey() = default;

	SslAsymmetricKey(const SslAsymmetricKey&) = delete;
	SslAsymmetricKey(SslAsymmetricKey&&) = delete;
	SslAsymmetricKey& operator = (const SslAsymmetricKey&) = delete;
	SslAsymmetricKey& operator = (SslAsymmetricKey&&) = delete;

private:
	using out_type = char*;

	bool _enc(out_type& ostr, size_t& olen, const void* in, size_t ilen, ssl::RSAPadding pt, bool direction) const;
	void* _write_priv_pem(out_type& ostr, size_t& olen, crypto::CipherType ct, const blob_type* key, const blob_type* passwd) const;
	void* _write_pub_pem(out_type& ostr, size_t& olen) const;

private:
	void* m_pkey = nullptr;

friend class SslContext;
friend class SslSign;
friend class SslVerify;
};

//! \brief SslAsymmetricKey를 이용한 디지털 서명
class SslSign final
{
public:
	static SslSign* s_createRSA_PKCS1(SslAsymmetricKey& pkey, digest::DigestType ht = ssl::getDefaultSignHash());
	static SslSign* s_createRSA_X931(SslAsymmetricKey& pkey, digest::DigestType ht = ssl::getDefaultSignHash());
	static SslSign* s_createRSA_PKCS1_PSS(SslAsymmetricKey& pkey, digest::DigestType ht = ssl::getDefaultSignHash(), size_t saltlen = ssl::getDefaultRSA_PSS_SaltSize());
	static inline void s_release(SslSign* p) { delete p; }

	~SslSign();

public:
	inline void release(void) { s_release(this); }

	bool update(const void* in, size_t ilen);
	inline bool update(const std::string& in) { return this->update(in.c_str(), in.size()); }

	bool finalize(void* out, size_t* olen);
	bool finalize(std::string& out);

	bool reinitialize(void);

	inline void* getRawData(void) { return m_ctx; }
	inline const void* getRawData(void) const { return m_ctx; }

	void* getRawDataMDContext(void);
	const void* getRawDataMDContext(void) const;
	const void* getRawDataMD(void) const;
	void* getRawDataAsymKey(void);
	const void* getRawDataAsymKey(void) const;

private:
	void* m_ctx = nullptr;

private:
	inline explicit SslSign() = default;

	SslSign(const SslSign&) = delete;
	SslSign(SslSign&&) = delete;
	SslSign& operator = (const SslSign&) = delete;
	SslSign& operator = (SslSign&&) = delete;
};

//! \brief SslAsymmetricKey를 이용한 디지털 서명 검증
class SslVerify final
{
public:
	static SslVerify* s_createRSA_PKCS1(SslAsymmetricKey& pkey, digest::DigestType ht = ssl::getDefaultSignHash());
// 	static SslVerify* s_createRSA_X931(SslAsymmetricKey& pkey, hash::DigestType ht = ssl::getDefaultSignHash());
	static SslVerify* s_createRSA_PKCS1_PSS(SslAsymmetricKey& pkey, digest::DigestType ht = ssl::getDefaultSignHash(), size_t saltlen = ssl::getDefaultRSA_PSS_SaltSize());
	static inline void s_release(SslVerify* p) { delete p; }

	~SslVerify();

public:
	inline void release(void) { s_release(this); }

	bool update(const void* in, size_t ilen);
	inline bool update(const std::string& in) { return this->update(in.c_str(), in.size()); }

	bool finalize(const void* sign, size_t sign_len);
	inline bool finalize(const std::string& sign) { return this->finalize(sign.c_str(), sign.size()); }

	bool reinitialize(void);

	inline void* getRawData(void) { return m_ctx; }
	inline const void* getRawData(void) const { return m_ctx; }

	void* getRawDataMDContext(void);
	const void* getRawDataMDContext(void) const;
	const void* getRawDataMD(void) const;
	void* getRawDataAsymKey(void);
	const void* getRawDataAsymKey(void) const;

private:
	void* m_ctx = nullptr;

private:
	inline explicit SslVerify() = default;

	SslVerify(const SslVerify&) = delete;
	SslVerify(SslVerify&&) = delete;
	SslVerify& operator = (const SslVerify&) = delete;
	SslVerify& operator = (SslVerify&&) = delete;
};

};//namespace pw

#endif//__PW_SSL_H__
