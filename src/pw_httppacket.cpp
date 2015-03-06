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

#include "./pw_httppacket.h"
#include "./pw_compress.h"
#include "./pw_encode.h"
#include "./pw_log.h"

namespace pw {

namespace http {

const char*
toStringA(Version ce)
{
	switch(ce)
	{
	case Version::VER_1_0: return strVersion_1_0;
	case Version::VER_1_1: return strVersion_1_1;
	case Version::VER_2: return strVersion_2;
	}

	return nullptr;
}

std::string
toString(Version ce)
{
	switch(ce)
	{
	case Version::VER_1_0: return std::string(strVersion_1_0);
	case Version::VER_1_1: return std::string(strVersion_1_1);
	case Version::VER_2: return std::string(strVersion_2);
	}

	return std::string();
}

Version
toVersion(const char* s)
{
	if ( not strcasecmp(s, strVersion_2) ) return Version::VER_2;
	if ( not strcasecmp(s, strVersion_1_1) ) return Version::VER_1_1;
	return Version::VER_1_0;
}

Version
toVersion(const std::string& s)
{
	return toVersion(s.c_str());
}

const char*
toStringA(Method ce)
{
	switch(ce)
	{
	case Method::CONNECT: return strMethod_Connect;
	case Method::OPTIONS: return strMethod_Options;
	case Method::DELETE: return strMethod_Delete;
	case Method::GET: return strMethod_Get;
	case Method::HEAD: return strMethod_Head;
	case Method::POST: return strMethod_Post;
	case Method::PUT: return strMethod_Put;
	case Method::TRACE: return strMethod_Trace;
	case Method::NONE: return strMethod_None;
	}

	return nullptr;
}

std::string
toString(Method ce)
{
	switch(ce)
	{
	case Method::CONNECT: return std::string(strMethod_Connect);
	case Method::OPTIONS: return std::string(strMethod_Options);
	case Method::DELETE: return std::string(strMethod_Delete);
	case Method::GET: return std::string(strMethod_Get);
	case Method::HEAD: return std::string(strMethod_Head);
	case Method::POST: return std::string(strMethod_Post);
	case Method::PUT: return std::string(strMethod_Put);
	case Method::TRACE: return std::string(strMethod_Trace);
	case Method::NONE: return std::string(strMethod_None);
	}

	return std::string();
}

Method
toMethod(const char* s)
{
	if ( not strcasecmp(s, strMethod_Post) ) return Method::POST;
	if ( not strcasecmp(s, strMethod_Get) ) return Method::GET;
	if ( not strcasecmp(s, strMethod_Put) ) return Method::PUT;
	if ( not strcasecmp(s, strMethod_Head) ) return Method::HEAD;
	if ( not strcasecmp(s, strMethod_Delete) ) return Method::DELETE;
	if ( not strcasecmp(s, strMethod_Connect) ) return Method::CONNECT;
	if ( not strcasecmp(s, strMethod_Options) ) return Method::OPTIONS;

	return Method::NONE;
}

Method
toMethod(const std::string& s)
{
	return toMethod(s.c_str());
}

const char*
toStringA(ContentEncoding ce)
{
	switch(ce)
	{
	case ContentEncoding::DEFLATE: return strCE_Deflate;
	case ContentEncoding::GZIP: return strCE_Gzip;
	case ContentEncoding::SDCH: return strCE_Sdch;
	case ContentEncoding::IDENTITY: return strCE_Identity;
	case ContentEncoding::INVALID:
		return nullptr;
	}

	return nullptr;
}

std::string
toString(ContentEncoding ce)
{
	switch(ce)
	{
	case ContentEncoding::DEFLATE: return std::string(strCE_Deflate);
	case ContentEncoding::GZIP: return std::string(strCE_Gzip);
	case ContentEncoding::SDCH: return std::string(strCE_Sdch);
	case ContentEncoding::IDENTITY: return std::string(strCE_Identity);
	case ContentEncoding::INVALID:
		return std::string();
	}

	return std::string();
}

ContentEncoding
toContentEncoding(const char* s)
{
	if ( not strcasecmp(strCE_Deflate, s) ) return ContentEncoding::DEFLATE;
	if ( not strcasecmp(strCE_Gzip, s) ) return ContentEncoding::GZIP;
	if ( not strcasecmp(strCE_Sdch, s) ) return ContentEncoding::SDCH;
	if ( not strcasecmp(strCE_Identity, s) ) return ContentEncoding::IDENTITY;

	return ContentEncoding::NONE;
}

ContentEncoding
toContentEncoding(const std::string& s)
{
	return toContentEncoding(s.c_str());
}

bool
splitUrlencodedForm(keyvalue_cont& out, const char* buf, size_t blen)
{
	Tokenizer tok(buf, blen);
	std::string token;
	keyvalue_cont tmp;

	std::string key, value;
	size_t cplen;

	while (tok.getNext(token, '&'))
	{
		Tokenizer tok2(token);
		tok2.getNext(key, '=');
		if ( key.empty() ) return false;
		if ( (cplen = tok2.getLeftSize()) > 0 )
		{
			value.assign(tok2.getPosition(), cplen);
			PWEnc::decodeURL(value);
		}
		else
		{
			value.clear();
		}

		if ( not tmp.insert(keyvalue_cont::value_type(key, value)).second ) return false;
	}

	out.swap(tmp);
	return true;
}

std::string&
mergeUrlencodedForm(std::string& out, const keyvalue_cont& in)
{
	std::string tmp, enc_value;
	auto ib(in.begin());
	auto ie(in.end());
	while ( ib not_eq ie )
	{
		tmp.append(ib->first);
		tmp.append(1, '=');
		tmp.append( PWEnc::encodeURL(enc_value, ib->second) );
		++ib;
		if ( ib not_eq ie ) tmp.append(1, '&');
	}

	out.swap(tmp);
	return out;
}

blob_type&
mergeUrlencodedForm(blob_type& out, const keyvalue_cont& in)
{
	std::string tmp;
	mergeUrlencodedForm(tmp, in);
	out.assign(tmp.c_str(), tmp.size(), blob_type::CT_MALLOC);
	return out;
}

std::ostream&
mergeUrlencodedForm(std::ostream& out, const keyvalue_cont& in)
{
	std::string enc_value;
	keyvalue_cont::const_iterator ib(in.begin());
	keyvalue_cont::const_iterator ie(in.end());
	while ( ib not_eq ie )
	{
		out.write(ib->first.c_str(), ib->first.size());
		out << '=';
		PWEnc::encodeURL(enc_value, ib->second);
		out.write(enc_value.c_str(), enc_value.size());
		++ib;
		if ( ib not_eq ie ) out << '&';
	}

	return out;
}

bool
isSsl ( const uri_type& uri )
{
	if ( uri.isNullScheme() ) return false;
	return isSsl(uri.getRefOfScheme());
}

bool
isSsl ( const std::string& s )
{
	return ( 0 == strcasecmp(s.c_str(), "https") );
}

bool
isSsl ( const char* s )
{
	return ( 0 == strcasecmp(s, "https") );
}

blob_type&
content_base_type::writeHeadersToBlob ( blob_type& oblob ) const
{
	std::string tmp;
	this->writeHeadersToString(tmp);
	oblob.assign(tmp, blob_type::CT_MALLOC);
	return oblob;
}

std::ostream&
content_base_type::writeHeadersToStream ( std::ostream& os ) const
{
	for ( auto& i : headers )
	{
		os << i.first;
		os.write(": ", 2);
		os << i.second;
		os.write("\r\n", 2);
	}

	return os;
}

std::string&
content_base_type::writeHeadersToString ( std::string& ostr ) const
{
	std::string tmp;
	tmp.reserve(headers.size()*5*512);

	for ( auto& i : headers )
	{
		tmp.append(i.first);
		tmp.append(": ", 2);
		tmp.append(i.second);
		tmp.append("\r\n", 2);
	}

	ostr.swap(tmp);
	return ostr;
}


//namespace http
};

//------------------------------------------------------------------------------
// HttpPacketInterface
bool
HttpPacketInterface::compress(http::ContentEncoding type, void* append_param)
{
	bool res(false);
	if ( type == http::ContentEncoding::GZIP ) res = Compress::s_compress(m_body, 9, Compress::CHUNK_SIZE*8, true);
	else if ( type == http::ContentEncoding::DEFLATE ) res = Compress::s_compress(m_body);

	if ( res ) setHeaderContentEncoding(type);
	return res;
}

bool
HttpPacketInterface::uncompress(http::ContentEncoding type, void* append_param)
{
	bool res(false);
	if ( type == http::ContentEncoding::GZIP ) res = Compress::s_uncompress(m_body, Compress::CHUNK_SIZE*8, true);
	else if ( type == http::ContentEncoding::DEFLATE ) res = Compress::s_uncompress(m_body);

	return res;
}

std::ostream&
HttpPacketInterface::writeHeaders(std::ostream& os) const
{
	auto ib(m_headers.begin());
	auto ie(m_headers.end());
	while ( ib != ie )
	{
		os << ib->first;
		os.write(": ", 2);
		os << ib->second;
		os.write("\r\n", 2);
		++ib;
	}

	os << http::strHeader_CL << ": " << m_body.size;
	os.write("\r\n", 2);

	return os;
}

std::string&
HttpPacketInterface::writeHeaders(std::string& ostr) const
{
	std::string tmp;
	tmp.reserve(m_headers.size()*5*512);

	auto ib(m_headers.begin());
	auto ie(m_headers.end());
	while ( ib != ie )
	{
		tmp.append(ib->first);
		tmp.append(": ", 2);
		tmp.append(ib->second);
		tmp.append("\r\n", 2);
		++ib;
	}

	std::string cl;
	PWStr::format(cl, "%s: %zu\r\n", http::strHeader_CL, m_body.size);
	tmp.append(cl);

	ostr.swap(tmp);
	return ostr;
}

ssize_t
HttpPacketInterface::write(IoBuffer& buf) const
{
	std::string fl;
	writeFirstLine(fl);

	std::string hdrs;
	writeHeaders(hdrs);

	const size_t pklen(fl.size()+hdrs.size()+2+m_body.size);

	IoBuffer::blob_type b;
	if ( not buf.grabWrite(b, pklen+1)) return ssize_t(-1);

	char* bptr(b.buf);
	::memcpy(bptr, fl.c_str(), fl.size());
	bptr += fl.size();
	::memcpy(bptr, hdrs.c_str(), hdrs.size());
	bptr += hdrs.size();
	::memcpy(bptr, "\r\n", 2);
	bptr += 2;

	if ( not m_body.empty() )
	{
		::memcpy(bptr, m_body.buf, m_body.size);
		//bptr += m_body.size();
	}

	buf.moveWrite(pklen);
	return ssize_t(pklen);
}

std::ostream&
HttpPacketInterface::write(std::ostream& os) const
{
	writeFirstLine(os);
	writeHeaders(os);
	os.write("\r\n", 2);
	if ( not m_body.empty() )
	{
		os.write(m_body.buf, m_body.size);
	}

	return os;
}

std::string&
HttpPacketInterface::write(std::string& ostr) const
{
	std::string fl;
	writeFirstLine(fl);

	std::string hdrs;
	writeHeaders(hdrs);

	const size_t pklen(fl.size()+hdrs.size()+2+m_body.size);
	std::string tmp;
	tmp.reserve(pklen);

	tmp.append(fl);
	tmp.append(hdrs);
	tmp.append("\r\n", 2);
	if ( not m_body.empty() )
	{
		tmp.append(m_body.buf, m_body.size);
	}

	ostr.swap(tmp);
	return ostr;
}

bool
HttpPacketInterface::setHeader(const std::string& key, const std::string& value)
{
	m_headers[key]=value;
	return true;
}

bool
HttpPacketInterface::setHeaderF ( const string& key, const char* fmt, ... )
{
	va_list lst;
	va_start(lst, fmt);
	auto res(setHeaderV(key, fmt, lst));
	va_end(lst);

	return res;
}

bool
HttpPacketInterface::setHeaderV ( const string& key, const char* fmt, va_list ap )
{
	StringUtility::formatV(m_headers[key], fmt, ap);
	return true;
}

void
HttpPacketInterface::setHeaderContentTypeMultipartMixed ( const std::string& boundary )
{
	StringUtility::format(m_headers[http::strHeader_CT], "%s; boundary=\"%s\"", http::strCT_MULTIPART_MIXED, cstr(boundary));
}

void
HttpPacketInterface::setHeaderContentTypeMultipartRelated ( const std::string& boundary )
{
	StringUtility::format(m_headers[http::strHeader_CT], "%s; boundary=\"%s\"", http::strCT_MULTIPART_RELATED, cstr(boundary));
}

void
HttpPacketInterface::setHeaderHost(const host_type& host)
{
	PWStr::format(m_headers["Host"], "%s:%s", host.host.c_str(), (host.service.empty() ? "80" : host.service.c_str()) );
}

void
HttpPacketInterface::setHeaderHost(const url_type& host)
{
	PWStr::format(m_headers["Host"], "%s:%s", host.host.c_str(), (host.service.empty() ? "80" : host.service.c_str()) );
}

void
HttpPacketInterface::setHeaderHost(const uri_type& uri)
{
	int port(uri.getNumericPort());
	if ( port and not (port == 80 or port == 443) )
	{
		PWStr::format(m_headers["Host"], "%s:%d", cstr(uri.getHost()) , port);
	}
	else
	{
		m_headers["Host"] = uri.getHost();
	}
}

//------------------------------------------------------------------------------
// HttpRequestPacket

HttpRequestPacket::~HttpRequestPacket()
{
}

bool
HttpRequestPacket::splitUrlencodedFormRequest(keyvalue_cont& out, std::string* page) const
{
	keyvalue_cont tmp1;
	splitUrlencodedForm(tmp1);

	if ( page ) m_uri.getPathString( *page );

	if ( not m_uri.isNullQuery() )
	{
		auto& query(m_uri.getRefOfQuery());
		if ( not query.empty() )
		{
			keyvalue_cont tmp2;

			http::splitUrlencodedForm(tmp2, query);
			tmp1.insert(tmp2.begin(), tmp2.end());
		}
	}

	out.swap(tmp1);
	return true;
}

void
HttpRequestPacket::setDefaultHeaders(const host_type& host)
{
	setHeaderContentType();
	setHeaderAccept();
	setHeaderHost(host);
}

void
HttpRequestPacket::setDefaultHeaders(const url_type& url)
{
	setHeaderContentType();
	setHeaderAccept();
	setHeaderHost(url);
	m_uri = url.page;
}

void
HttpRequestPacket::setDefaultHeaders(const uri_type& uri)
{
	setHeaderContentType();
	setHeaderAccept();
	setHeaderHost(uri);
	m_uri = uri;

	// Path가 없을 경우 최소 경로 "/"을 만들어준다.
	if ( m_uri.isEmptyPath() ) m_uri.appendNullToPath();
}

bool
HttpRequestPacket::setFirstLine(char const* buf, size_t blen)
{
	// METHOD URI VERSION
	Tokenizer tok(buf, blen);
	char tmp[64];
	std::string uri;

	do {
		PWTOKCSTR(tok, tmp, sizeof(tmp), ' ');
		if ( http::Method::NONE == (m_method_type = http::toMethod(tmp)) ) break;

		PWTOKSTR(tok, uri, ' ');
		if ( not m_uri.parse(uri) ) break;

		PWTOKCSTR(tok, tmp, sizeof(tmp), ' ');
		m_version = http::toVersion(tmp);

		return true;
	} while (false);

	return false;
}

void
HttpRequestPacket::clear(void)
{
	m_version = http::Version::VER_1_0;
	m_headers.clear();
	m_body.clear();

	m_method_type = http::Method::NONE;
	m_uri.clear();
}

void
HttpRequestPacket::swap(HttpRequestPacket& pk)
{
	std::swap(m_version, pk.m_version);
	m_headers.swap(pk.m_headers);
	m_body.swap(pk.m_body);

	std::swap(m_method_type, pk.m_method_type);
	m_uri.swap(pk.m_uri);
}

std::string&
HttpRequestPacket::writeFirstLine(std::string& ostr) const
{
	std::string uri;
	m_uri.getPathString(uri);
	if ( not m_uri.isNullQuery() )
	{
		uri.append(1, '?');
		uri.append(m_uri.getRefOfQuery());

		if ( not m_uri.isNullFragment() )
		{
			uri.append(1, '#');
			uri.append(m_uri.getRefOfFragment());
		}
	}

	PWStr::format(ostr, "%s %s %s\r\n", http::toStringA(m_method_type), cstr(uri), http::toStringA(m_version));
	return ostr;
}

std::ostream&
HttpRequestPacket::writeFirstLine(std::ostream& os) const
{
	std::string uri;
	m_uri.getPathString(uri);
	if ( not m_uri.isNullQuery() )
	{
		uri.append(1, '?');
		uri.append(m_uri.getRefOfQuery());

		if ( not m_uri.isNullFragment() )
		{
			uri.append(1, '#');
			uri.append(m_uri.getRefOfFragment());
		}
	}

	os << http::toStringA(m_method_type) << ' ' << uri << ' ' << http::toStringA(m_version);
	os.write("\r\n", 2);
	return os;
}

//------------------------------------------------------------------------------
// HttpResponsePacket
HttpResponsePacket::~HttpResponsePacket()
{
}

void
HttpResponsePacket::clear(void)
{
	m_version = http::Version::VER_1_0;
	m_headers.clear();
	m_body.clear();

	m_res_code = PWRES_CODE_SUCCESS;
	m_res_mesg.clear();
}

void
HttpResponsePacket::swap(HttpResponsePacket& pk)
{
	std::swap(m_version, pk.m_version);
	m_headers.swap(pk.m_headers);
	m_body.swap(pk.m_body);

	std::swap(m_res_code, pk.m_res_code);
	m_res_mesg.swap(pk.m_res_mesg);
}

std::string&
HttpResponsePacket::writeFirstLine(std::string& ostr) const
{
	PWStr::format(ostr, "%s %d %s\r\n", http::toStringA(m_version), m_res_code, m_res_mesg.c_str());
	return ostr;
}

std::ostream&
HttpResponsePacket::writeFirstLine(std::ostream& os) const
{
	os << http::toString(m_version) << ' ' << m_res_code << ' ' << m_res_mesg;
	os.write("\r\n", 2);
	return os;
}

bool
HttpResponsePacket::setFirstLine(char const* buf, size_t blen)
{
	// HTTP/1.0 RCODE MESSAGE
	Tokenizer tok(buf, blen);
	char tmp[32];
	do {
		PWTOKCSTR(tok, tmp, sizeof(tmp), ' ');
		m_version = http::toVersion(tmp);

		PWTOKCSTR(tok, tmp, sizeof(tmp), ' ');
		int icode(::atoi(tmp));
		if ( ( icode < 100 ) or ( icode > 999 ) )
		{
			PWTRACE("invalid response code");
			break;
		}

		m_res_code = static_cast<ResultCode>(icode);

		if ( tok.getLeftSize() > 0 )
		{
			m_res_mesg.assign(tok.getPosition(), tok.getLeftSize());
		}

		return true;
	} while (false);
	return false;
}

}; //namespace pw
