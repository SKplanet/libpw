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
 * \file pw_httppacket.h
 * \brief Packet for HTTP/1.x.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"
#include "./pw_packet_if.h"
#include "./pw_string.h"
#include "./pw_uri.h"

#ifdef HAVE_JSONCPP
#	include <json/json.h>
#endif//HAVE_JSONCPP

#ifndef __PW_HTTPPACKET_H__
#define __PW_HTTPPACKET_H__

namespace pw {

class HttpRequestPacket;
class HttpResponsePacket;

namespace http {
//! \brief 요청/응답
enum class PacketType
{
	REQUEST,
	RESPONSE,
};

//! \brief 메소드
enum class Method
{
	NONE = 0,
	GET,
	HEAD,
	POST,
	PUT,
	DELETE,
	TRACE,
	CONNECT,
	OPTIONS,
};

constexpr auto strMethod_None("NONE");
constexpr auto strMethod_Get("GET");
constexpr auto strMethod_Head("HEAD");
constexpr auto strMethod_Post("POST");
constexpr auto strMethod_Put("PUT");
constexpr auto strMethod_Delete("DELETE");
constexpr auto strMethod_Trace("TRACE");
constexpr auto strMethod_Connect("CONNECT");
constexpr auto strMethod_Options("OPTIONS");

//! \brief HTTP 버전
enum class Version
{
	VER_1_0,	//!< 1.0
	VER_1_1,	//!< 1.1
	VER_2,	//!< 2
};

constexpr auto strVersion_1_0("HTTP/1.0");
constexpr auto strVersion_1_1("HTTP/1.1");
constexpr auto strVersion_2("HTTP/2");

//! \brief Content-Encoding
enum class ContentEncoding
{
	INVALID = -1,
	NONE = 0,	//!< identity
	IDENTITY = NONE,
	GZIP,	//!< gzip
	DEFLATE,	//!< deflate
	SDCH,	//!< Google Shared Dictionary Compresssion for HTTP/RFC 3284
};

constexpr auto strCE_Identity("identity");
constexpr auto strCE_Gzip("gzip");
constexpr auto strCE_Deflate("deflate");
constexpr auto strCE_Sdch("sdch");
constexpr auto strCE_GzipDeflate("gzip, deflate");

enum class Connection : int
{
	INVALID = -1,
	NONE = 0,
	CLOSE = 1,
	KEEP_ALIVE = 2,
	UPGRADE = 4,
	HTTP2_SETTINGS = 8,
};

constexpr auto strCONN_Close("Close");
constexpr auto strCONN_Keep_Alive("Keep-Alive");
constexpr auto strCONN_Upgrade("Upgrade");
constexpr auto strCONN_HTTP2_Settings("HTTP2-Settings");
constexpr auto strCONN_Upgrade_HTTP2_Settings("Upgrade, HTTP2-Settings");

constexpr auto strUPG_H2("h2");
constexpr auto strUPG_H2C("h2c");

//! \brief UrlencodeForm을 해석한다.
extern bool splitUrlencodedForm(keyvalue_cont& out, const char* buf, size_t blen);

//! \brief UrlencodeForm을 해석한다.
inline bool splitUrlencodedForm(keyvalue_cont& out, const std::string& buf) { return splitUrlencodedForm(out, buf.c_str(), buf.size()); }

//! \brief UrlEncodeForm 문자열로 합친다.
extern std::string& mergeUrlencodedForm(std::string& out, const keyvalue_cont& in);

//! \brief UrlEncodeForm 문자열로 합친다.
extern blob_type& mergeUrlencodedForm(blob_type& out, const keyvalue_cont& in);

//! \brief UrlEncodeForm 문자열로 합친다.
extern std::ostream& mergeUrlencodedForm(std::ostream& out, const keyvalue_cont& in);

constexpr auto strHeader_CONN("Connection");
constexpr auto strHeader_CE("Content-Encoding");
constexpr auto strHeader_CT("Content-Type");
constexpr auto strHeader_CL("Content-Length");
constexpr auto strHeader_CTE("Content-Transfer-Encoding");
constexpr auto strHeader_UA("User-Agent");
constexpr auto strHeader_Accept("Accept");
constexpr auto strHeader_AE("Accept-Encoding");
constexpr auto strHeader_Upgrade("Upgrade");
constexpr auto strHeader_H2SET("HTTP2-Settings");

constexpr auto strCT_APP_URLE("application/x-www-form-urlencoded");
constexpr auto strCT_APP_JSON("application/json");
constexpr auto strCT_APP_OCTSTREAM("application/octet-stream");
constexpr auto strCT_TEXT_XML("text/xml");
constexpr auto strCT_TEXT_PLAIN("text/plain");
constexpr auto strCT_MULTIPART_MIXED("multipart/mixed");
constexpr auto strCT_MULTIPART_RELATED("multipart/related");

extern const char* toStringA(Version ce);
extern std::string toString(Version ce);
extern Version toVersion(const char* s);
extern Version toVersion(const std::string& s);

extern const char* toStringA(Method ce);
extern std::string toString(Method ce);
extern Method toMethod(const char* s);
extern Method toMethod(const std::string& s);

extern const char* toStringA(ContentEncoding ce);
extern std::string toString(ContentEncoding ce);
extern ContentEncoding toContentEncoding(const char* s);
extern ContentEncoding toContentEncoding(const std::string& s);

extern bool isSsl(const char* s);
extern bool isSsl(const std::string& s);
extern bool isSsl(const uri_type& uri);

struct content_base_type
{
	inline virtual ~content_base_type() = default;

	keyivalue_cont headers;

	std::ostream& writeHeadersToStream(std::ostream& os) const;
	std::string& writeHeadersToString(std::string& ostr) const;
	blob_type& writeHeadersToBlob(blob_type& oblob) const;

	virtual void* getRawBody(void) = 0;
	virtual const void* getRawBody(void) const = 0;
};

template<typename _BodyType>
struct content_type_template final : public content_base_type
{
	using body_type = _BodyType;

	virtual ~ content_type_template () = default;

	body_type body;

	void* getRawBody(void) override { return &body; }
	const void* getRawBody(void) const override { return &body; }
};

using content_blob_type = content_type_template<blob_type>;
using content_string_type = content_type_template<std::string>;
using content_stringstream_type = content_type_template<std::stringstream>;

#ifdef HAVE_JSONCPP
using content_json_type = content_type_template<Json::Value>;
#endif//HAVE_JSONCPP

// namespace http
};

//! \brief HTTP 패킷 인터페이스
class HttpPacketInterface : public PacketInterface
{
public:
	enum
	{
		MAX_FIRST_LINE_SIZE = 1024*10,
		MAX_HEADER_LINE_SIZE = MAX_FIRST_LINE_SIZE,
		MAX_BODY_SIZE = 1024*1024,
		DEFAULT_BODY_SIZE = 1024*10,
	};

public:
	inline explicit HttpPacketInterface() : m_version(http::Version::VER_1_0) {}
	inline virtual ~HttpPacketInterface() {}

public:
	//! \brief 패킷 타입을 반환한다.
	virtual http::PacketType getPacketType(void) const = 0;

	//! \brief 패킷을 요청 패킷으로 변환한다.
	//! \warning 요청 패킷이 아닌데 변환하면 abort()를 반환한다.
	inline HttpRequestPacket& toRequest(void);
	inline const HttpRequestPacket& toRequest(void) const;

	//! \brief 패킷을 응답 패킷으로 변환한다.
	//! \warning 응답 패킷이 아닌데 변환하면 abort()를 반환한다.
	inline HttpResponsePacket& toResponse(void);
	inline const HttpResponsePacket& toResponse(void) const;

	//! \brief 버전 정보를 반환한다.
	inline http::Version getVersion(void) const { return m_version; }

	//! \brief 헤더를 출력한다.
	std::string& writeHeaders(std::string& ostr) const;

	//! \brief 헤더를 출력한다.
	std::ostream& writeHeaders(std::ostream& os) const;

	//! \brief 첫줄을 출력한다.
	virtual std::string& writeFirstLine(std::string& ostr) const = 0;

	//! \brief 첫줄을 출력한다.
	virtual std::ostream& writeFirstLine(std::ostream& os) const = 0;

	//! \brief 첫줄을 설정한다.
	virtual bool setFirstLine(const char* buf, size_t blen) = 0;

	//! \brief 헤더를 추가한다.
	bool setHeader(const std::string& key, const std::string& value);
	bool setHeaderF(const std::string& key, const char* fmt, ...) __attribute__((format(printf,3,4)));
	bool setHeaderV(const std::string& key, const char* fmt, va_list ap);

	inline void removeHeader(const std::string& key) { m_headers.erase(key); }

	//! \brief 바디가 널인지 확인한다.
	bool isBodyNull(void) const { return m_body.isNull(); }

	//! \brief 헤더를 얻어온다.
	inline std::string* getHeader(const std::string& hdr) { auto ib(m_headers.find(hdr)); return m_headers.end() == ib ? nullptr : &(ib->second); }
	inline const std::string* getHeader(const std::string& hdr) const { auto ib(m_headers.find(hdr)); return m_headers.end() == ib ? nullptr : &(ib->second); }

	inline std::string* getHeaderContentType(void) { return this->getHeader(http::strHeader_CT); }
	inline const std::string* getHeaderContentType(void) const { return this->getHeader(http::strHeader_CT); }

	ssize_t write(IoBuffer& buf) const;
	std::ostream& write(std::ostream& os) const;
	std::string& write(std::string& ostr) const;

	//! \brief 바디를 파싱한다.
	//! \warning 요청 패킷일 경우, splitUrlencodedFormRequest() 메소드를 사용할 것.
	inline bool splitUrlencodedForm(keyvalue_cont& out) const
	{
		return http::splitUrlencodedForm(out, m_body.buf, m_body.size);
	}

	//! \brief 바디로 합친다.
	inline bool mergeUrlencodedForm(const keyvalue_cont& in)
	{
		std::string buf;
		http::mergeUrlencodedForm(buf, in);
		return m_body.assign(buf.c_str(), buf.size(), blob_type::CT_MALLOC);
	}

	//! \brief 헤더에 기본 Content-Type(application/x-www-form-urlencoded)으로 설정한다.
	inline void setHeaderContentType(void) { m_headers[http::strHeader_CT] = http::strCT_APP_URLE; }

	//! \brief 헤더에 Content-Type을 application/json으로 설정한다.
	inline void setHeaderContentTypeJson(void) { m_headers[http::strHeader_CT] = http::strCT_APP_JSON; }

	//! \brief 헤더에 Content-Type을 application/octet-stream으로 설정한다.
	inline void setHeaderContentTypeOctStream(void) { m_headers[http::strHeader_CT] = http::strCT_APP_OCTSTREAM; }

	//! \brief 헤더에 Content-Type을 text/plain으로 설정한다.
	inline void setHeaderContentTypePlain(void) { m_headers[http::strHeader_CT] = http::strCT_TEXT_PLAIN; }

	//! \brief 헤더에 Content-Type을 text/xml으로 설정한다.
	inline void setHeaderContentTypeXml(void) { m_headers[http::strHeader_CT] = http::strCT_TEXT_XML; }

	//! \brief 헤더에 Content-Type을 multipart/mixed로 설정한다.
	void setHeaderContentTypeMultipartMixed(const std::string& boundary);

	//! \brief 헤더에 Content-Type을 multipart/related로 설정한다.
	void setHeaderContentTypeMultipartRelated(const std::string& boundary);

	//! \brief 헤더에 Content-Type을 설정한다.
	inline void setHeaderContentType(const std::string& v) { m_headers[http::strHeader_CT] = v; }

	//! \brief 헤더에 Content-Transfer-Encoding을 설정한다.
	inline void setHeaderContentTransferEncoding(const std::string& v) { m_headers[http::strHeader_CTE] = v; }

	//! \brief 헤더에 User-Agent를 설정한다.
	inline void setHeaderUserAgent(const std::string& v) { m_headers[http::strHeader_UA] = v; }

	//! \brief 헤더에 기본 Accept(*/*)를 설정한다.
	inline void setHeaderAccept(void) { m_headers[http::strHeader_Accept] = "*/*"; }

	//! \brief 헤더에 Accept를 설정한다.
	inline void setHeaderAccept(const std::string& v) { m_headers[http::strHeader_Accept] = v; }

	//! \brief 헤더에 기본 Accept-Encoding(gzip, deflate)을 설정한다.
	inline void setHeaderAcceptEncoding(void) { m_headers[http::strHeader_AE] = http::strCE_GzipDeflate; }

	//! \brief 헤더에 Accept-Encoding을 설정한다.
	inline void setHeaderAcceptEncoding(const std::string& v) { m_headers[http::strHeader_AE] = v; }

	//! \brief 헤더에 Content-Length를 설정한다.
	inline void setHeaderContentLength(size_t length) { PWStr::format(m_headers[http::strHeader_CL], "%zu", length); }

	//! \brief 헤더에 Content-Encoding을 설정한다.
	inline void setHeaderContentEncoding(http::ContentEncoding ce);

	//! \brief 헤더에 Host를 설정한다.
	void setHeaderHost(const host_type& host);

	//! \brief 헤더에 Host를 설정한다.
	void setHeaderHost(const url_type& url);

	void setHeaderHost(const uri_type& uri);

	//! \brief 바디를 압축한다.
	bool compress(http::ContentEncoding type = http::ContentEncoding::GZIP, void* append_param = nullptr);

	//! \brief 바디 압축을 해제한다.
	bool uncompress(http::ContentEncoding type = http::ContentEncoding::GZIP, void* append_param = nullptr);

public:
	http::Version	m_version;	//!< HTTP 버전
	keyivalue_cont	m_headers;	//!< 헤더
	blob_type		m_body;		//!< 바디
};

inline
void
HttpPacketInterface::setHeaderContentEncoding(http::ContentEncoding ce)
{
	m_headers[http::strHeader_CE] = http::toString(ce);
}

//! \brief HTTP 요청 패킷
class HttpRequestPacket final: public HttpPacketInterface
{
public:
	inline explicit HttpRequestPacket() : m_method_type(http::Method::POST), m_uri("/") {}
	virtual ~HttpRequestPacket();

public:
	//! \brief GET 방식 파라매터도 파싱한다.
	//! \param[out] out 키와 값 쌍.
	//! \param[out] page 파라매터를 제외한 실제 페이지.
	bool splitUrlencodedFormRequest(keyvalue_cont& out, std::string* page = nullptr) const;

	//! \brief 페이지를 설정한다.
	inline void setPage(const std::string& page) { m_uri = page; }
	inline void setUri(const std::string& uri) { m_uri = uri; }

	//! \brief 페이지를 설정한다.
	inline void setPage(const char* page) { m_uri = page; }
	inline void setUri(const char* uri) { m_uri = uri; }

	//! \brief 기본 헤더를 설정한다.
	void setDefaultHeaders(const host_type& host);

	//! \brief 기본 헤더를 설정한다.
	void setDefaultHeaders(const url_type& url);

	void setDefaultHeaders(const uri_type& uri);

	//! \brief User-Agent헤더를 파이어폭스로 설정한다.
	inline void setHeaderUserAgent_Firefox(const std::string& version = "26.0", const std::string& engine_version = "20100101", const std::string& product_version = "5.0");

	//! \brief User-Agent헤더를 CURL로 설정한다.
	inline void setHeaderUserAgent_cURL(const std::string& version = "7.37.1");

	//! \brief User-Agent헤더를 IE로 설정한다.
	inline void setHeaderUserAgent_IE(const std::string& version = "11.0", const std::string& engine_version = "7.0", const std::string& product_version = "5.0");

	inline http::PacketType getPacketType(void) const { return http::PacketType::REQUEST; }

	//! \brief 메소드 타입을 설정한다.
	inline void setMethodType(http::Method mt) { m_method_type = mt; }

	//! \brief 메소드 타입을 가져온다.
	inline http::Method getMethodType(void) const { return m_method_type; }

	//! \brief 첫 요청 라인을 설정한다.
	bool setFirstLine(const char* buf, size_t blen);

	std::string& writeFirstLine(std::string& ostr) const;
	std::ostream& writeFirstLine(std::ostream& os) const;

	void clear(void);
	void swap(HttpRequestPacket& pk);

	HttpRequestPacket& toRequest(void) = delete;
	const HttpRequestPacket& toRequest(void) const = delete;
	HttpResponsePacket& toResponse(void) = delete;
	const HttpResponsePacket& toResponse(void) const = delete;

public:
	http::Method m_method_type;
	uri_type m_uri;
};

inline void HttpRequestPacket::setHeaderUserAgent_Firefox(const std::string& version, const std::string& engine_version, const std::string& product_version)
{
	PWStr::format(m_headers[http::strHeader_UA],
				  "Mozilla/%s (Windows NT 6.1; WOW64; rv:%s) Gecko/%s Firefox/%s",
				  product_version.c_str(),
				  version.c_str(),
				  engine_version.c_str(),
				  version.c_str());
}

inline void HttpRequestPacket::setHeaderUserAgent_cURL ( const string& version )
{
	PWStr::format(m_headers[http::strHeader_UA],
				  "curl/%s", version.c_str());
}

inline void HttpRequestPacket::setHeaderUserAgent_IE(const std::string& version, const std::string& engine_version, const std::string& product_version)
{
	//Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; TCO_20150102155011; rv:11.0) like Gecko
	PWStr::format(m_headers[http::strHeader_UA],
				  "Mozilla/%s (Windows NT 6.1; WOW64; Trident/%s; TCO_20150102155011; rv:%s) like Gecko",
				  product_version.c_str(),
				  engine_version.c_str(),
				  version.c_str() );
}

//! \brief HTTP 응답 패킷
class HttpResponsePacket final: public HttpPacketInterface
{
public:
	inline explicit HttpResponsePacket() : m_res_code(PWRES_CODE_SUCCESS) {}
	virtual ~HttpResponsePacket();

public:
	//! \brief 응답 코드 및 메시지를 설정한다.
	inline void setResCode(int code)
	{
		m_res_code = static_cast<ResultCode>(code);
		pw::getErrorMessage(m_res_mesg, m_res_code);
	}

	inline void setResCode(int code, const std::string& rmesg)
	{
		m_res_code = static_cast<ResultCode>(code);
		m_res_mesg = rmesg;
	}

	inline void setResCode(ResultCode code)
	{
		m_res_code = code;
		pw::getErrorMessage(m_res_mesg, code);
	}

	inline void setResCode(ResultCode code, const std::string& rmesg)
	{
		m_res_code = code;
		m_res_mesg = rmesg;
	}

	inline http::PacketType getPacketType(void) const { return http::PacketType::RESPONSE; }

	//! \brief 응답코드를 반환한다.
	inline ResultCode getResCode(void) const { return m_res_code; }

	//! \brief 응답코드 메시지를 반환한다.
	inline const char* getResMessage(void) const { return m_res_mesg.c_str(); }

	//! \brief 첫 요청 라인을 설정한다.
	bool setFirstLine(const char* buf, size_t blen);

	std::string& writeFirstLine(std::string& ostr) const;
	std::ostream& writeFirstLine(std::ostream& os) const;

	void clear(void);
	void swap(HttpResponsePacket& pk);

	HttpRequestPacket& toRequest(void) = delete;
	const HttpRequestPacket& toRequest(void) const = delete;
	HttpResponsePacket& toResponse(void) = delete;
	const HttpResponsePacket& toResponse(void) const = delete;

public:
	pw::ResultCode	m_res_code;
	std::string		m_res_mesg;
};

HttpRequestPacket&
HttpPacketInterface::toRequest(void)
{
	return *dynamic_cast<HttpRequestPacket*>(this);
}

const HttpRequestPacket&
HttpPacketInterface::toRequest(void) const
{
	return *dynamic_cast<const HttpRequestPacket*>(this);
}

HttpResponsePacket&
HttpPacketInterface::toResponse(void)
{
	return *dynamic_cast<HttpResponsePacket*>(this);
}

const HttpResponsePacket&
HttpPacketInterface::toResponse(void) const
{
	return *dynamic_cast<const HttpResponsePacket*>(this);
}

}; //namespace pw

#endif//__PW_HTTPPACKET_H__

