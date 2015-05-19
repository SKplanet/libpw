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
 * \file pw_common.cpp
 * \brief Library common settings.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 * \warning Do not use this file directly.
 */

#include "./pw_common.h"
#include "./pw_string.h"
#include "./pw_region.h"
#include "./pw_compress.h"
#include "./pw_uri.h"

namespace pw {

static std::list<init_func_type> s_init_funcs;

#define LIST_DEL	char(';')

static const std::map<ResultCode, const std::string>	s_rescode_mesg = {
	{ResultCode::EMPTY, "Invalid?"},
	{ResultCode::CONTINUE, "Continue"},
	{ResultCode::SWITCHING_PROTOCOL, "Switching Protocols"},
	{ResultCode::SUCCESS, "OK"},
	{ResultCode::CREATED, "Created"},
	{ResultCode::ACCEPTED, "Accepted"},
	{ResultCode::NOAUTH_INFORMATION, "Non-Authoritative Information"},
	{ResultCode::NO_CONTENT, "No Content"},
	{ResultCode::RESET_CONTENT, "Reset Content"},
	{ResultCode::PARTIAL_CONTENT, "Partial Content"},
	{ResultCode::MULTIPLE_CHOICES, "Multiple Choices"},
	{ResultCode::MOVED_PERMANENTLY, "Moved Permanently"},
	{ResultCode::FOUND, "Found"},
	{ResultCode::SEE_OTHER, "See Other"},
	{ResultCode::NOT_MODIFIED, "Not Modified"},
	{ResultCode::USE_PROXY, "Use Proxy"},
	{ResultCode::TEMPORARY_REDIRECT, "Temporary Redirect"},
	{ResultCode::BAD_REQUEST, "Bad Request"},
	{ResultCode::UNAUTHORIZED, "Unauthorized"},
	{ResultCode::PAYMENT_REQUIRED, "Payment Required"},
	{ResultCode::FORBIDDEN, "Forbidden"},
	{ResultCode::NOT_FOUND, "Not Found"},
	{ResultCode::METHOD_NOT_ALLOWED, "Method Not Allowed"},
	{ResultCode::NOT_ACCEPTABLE, "Not Acceptable"},
	{ResultCode::PROXY_AUTH_REQUIRED, "Proxy Authentication Required"},
	{ResultCode::REQUEST_TIMEOUT, "Request Time-out"},
	{ResultCode::CONFLICT, "Conflict"},
	{ResultCode::GONE, "Gone"},
	{ResultCode::LENGTH_REQUIRED, "Length Required"},
	{ResultCode::PRECONDITION_FAILED, "Precondition Failed"},
	{ResultCode::REQUEST_ENTITY_TOO_LARGE, "Request Entity Too Large"},
	{ResultCode::REQUEST_URI_TOO_LONG, "Request-URI Too Large"},
	{ResultCode::UNSUPPORTED_MEDIA_TYPE, "Unsupported Media Type"},
	{ResultCode::REQUEST_RANGE_FAILED, "Requested range not satisfiable"},
	{ResultCode::EXPECT_FAILED, "Expectation Failed"},
	{ResultCode::INTERNAL_SERVER_ERROR, "Internal Server Error"},
	{ResultCode::NOT_IMPLEMENTED, "Not Implemented"},
	{ResultCode::BAD_GATEWAY, "Bad Gateway"},
	{ResultCode::SERVICE_UNAVAILABLE, "Service Unavailable"},
	{ResultCode::GATEWAY_TIMEOUT, "Gateway Time-out"},
	{ResultCode::VERSION_NOT_SUPPORTED, "HTTP Version not supported"},
};

bool
_PWINIT(void)
{
	Region::s_initialize();
	Compress::s_initialize();

	bool result(true);
	for( auto func : s_init_funcs )
	{
		result and_eq func();
	}

	return result;
}

static
inline
const std::string&
__getErrorMessageEmpty(void)
{
	static auto& s(s_rescode_mesg.find(ResultCode::EMPTY)->second);
	return s;
}

ResultCode
checkResultCode(ResultCode code)
{
	auto ib(s_rescode_mesg.find(code));
	if ( ib == s_rescode_mesg.end() ) return ResultCode::EMPTY;
	return code;
}

const char*
getErrorMessageA(ResultCode code)
{
	auto ib(s_rescode_mesg.find(code));
	if ( ib not_eq s_rescode_mesg.end() ) return ib->second.c_str();
	return __getErrorMessageEmpty().c_str();
}

const std::string&
getErrorMessage(ResultCode code)
{
	auto ib(s_rescode_mesg.find(code));
	if ( ib not_eq s_rescode_mesg.end() ) return ib->second;
	return __getErrorMessageEmpty();
}

std::string&
getErrorMessage(std::string& out, ResultCode code)
{
	auto ib(s_rescode_mesg.find(code));
	out = (ib not_eq s_rescode_mesg.end()) ? ib->second : __getErrorMessageEmpty();
	return out;
}

host_type::host_type ( const uri_type& uri ) : host(uri.getHost()), service(uri.getPort())
{
	if ( service.empty() ) service = std::to_string(uri.getNumericPort());
}

host_type&
host_type::operator= ( const uri_type& uri )
{
	host = uri.getHost();
	if ( (service = uri.getPort()).empty() ) service = std::to_string(uri.getNumericPort());

	return *this;
}

bool
host_type::read(const std::string& line)
{
	std::string::size_type ib(line.rfind(char(':')));
	if ( std::string::npos == ib ) return false;

	host.assign(line, 0, ib);
	service.assign(line, ib+1, std::string::npos);

	return true;
}

std::string&
host_type::write(std::string& line) const
{
	PWStr::format(line, "%s:%s", host.c_str(), service.c_str());
	return line;
}

host_list_type&
host_type::s_read(host_list_type& out, const std::string& line)
{
	Tokenizer tok(line);
	std::string tmp;
	host_list_type outtmp;
	while ( tok.getNext2(tmp, LIST_DEL) )
	{
		if ( tmp.empty() ) break;

		host_type t;
		if ( t.read(tmp) ) outtmp.push_back(t);
	}

	outtmp.swap(out);
	return out;
}

std::string&
host_type::s_write(std::string& line, const host_list_type& in)
{
	host_list_type::const_iterator ib(in.begin());
	host_list_type::const_iterator ie(in.end());

	std::string tmp, tmpl;

	while ( ib != ie )
	{
		tmpl += (*ib).write(tmp);
		++ib;
		if ( ib != ie ) tmpl += LIST_DEL;
	}

	line.swap(tmpl);
	return line;
}

url_type::url_type ( const uri_type& uri ) : host(uri.getHost()), service(uri.getPort()), page(uri.getPathString())
{
	if ( service.empty() ) service = std::to_string(uri.getNumericPort());

	if ( page.empty() ) page.assign(1, '/');

	if ( uri.isNullQuery() ) return;

	const auto& query(uri.getRefOfQuery());
	if ( not query.empty() )
	{
		page.append(1, '?');
		page.append(query);

		if ( uri.isNullFragment() ) return;

		const auto& fragment(uri.getRefOfFragment());
		if ( not fragment.empty() )
		{
			page.append(1, '#');
			page.append(fragment);
		}
	}
}

url_type&
url_type::operator= ( const uri_type& uri )
{
	host = uri.getHost();
	if ( (service = uri.getPort()).empty() ) service = std::to_string(uri.getNumericPort());

	if ( uri.getPathString(page).empty() ) page.assign(1, '/');

	if ( uri.isNullQuery() ) return *this;

	const auto& query(uri.getRefOfQuery());
	if ( not query.empty() )
	{
		page.append(1, '?');
		page.append(query);

		if ( uri.isNullFragment() ) return *this;

		const auto& fragment(uri.getRefOfFragment());
		if ( not fragment.empty() )
		{
			page.append(1, '#');
			page.append(fragment);
		}
	}

	return *this;
}

bool
url_type::read(const std::string& line)
{
	// XXXXX:YYYYYYY/ZZZZZZZ
	std::string::size_type pos(line.find(char(':')));
	if ( pos == std::string::npos ) return false;

	host.assign(line, 0, pos);
	++pos;

	std::string::size_type pos2(line.find(char('/'), pos));
	if ( pos2 == std::string::npos )
	{
		service.assign(line, pos, std::string::npos);
		page.clear();
	}
	else
	{
		service.assign(line, pos, pos2-pos);
		page.assign(line, pos2, std::string::npos);
	}

	return true;
}

std::string&
url_type::write(std::string& line) const
{
	PWStr::format(line, "%s:%s%s", host.c_str(), service.c_str(), page.c_str());
	return line;
}

url_list_type&
url_type::s_read(url_list_type& out, const std::string& line)
{
	Tokenizer tok(line);
	std::string tmp;
	url_list_type outtmp;
	while ( tok.getNext2(tmp, LIST_DEL) )
	{
		if ( tmp.empty() ) break;

		url_type t;
		if ( t.read(tmp) ) outtmp.push_back(t);
	}

	outtmp.swap(out);
	return out;
}

std::string&
url_type::s_write(std::string& line, const url_list_type& in)
{
	url_list_type::const_iterator ib(in.begin());
	url_list_type::const_iterator ie(in.end());

	std::string tmp, tmpl;

	while ( ib != ie )
	{
		tmpl += (*ib).write(tmp);
		++ib;
		if ( ib != ie ) tmpl += LIST_DEL;
	}

	line.swap(tmpl);
	return line;
}

bool
blob_type::format(const char* fmt, ...)
{
	int errno_backup(errno);

	va_list lst;

	va_start(lst, fmt);
	const int in_size(vsnprintf(nullptr, 0, fmt, lst));
	va_end(lst);

	bool ret(false);

	do {
		if ( not allocate(in_size+1) ) break;
		if ( in_size > 0 )
		{
			va_start(lst, fmt);
			vsnprintf(const_cast<char*>(buf), size, fmt, lst);
			va_end(lst);
		}
		else
		{
			*(const_cast<char*>(buf) + in_size) = 0x00;
		}

		--size;
		ret = true;

	} while (false);

	if ( errno not_eq errno_backup ) errno = errno_backup;
	return ret;
}

bool
blob_type::append(size_t pos, const void* ibuf, size_t iblen)
{
	if ( (nullptr == ibuf) or (0 == iblen) ) return true;
	if ( empty() ) return assign(ibuf, iblen, CT_MALLOC);
	if ( size < pos ) pos = size;

	const size_t leftsize(size-pos);
	const size_t delta( (leftsize<iblen) ? (iblen-leftsize) : 0 );

	if ( CT_POINTER == type )
	{
		char* nbuf(static_cast<char*>(::malloc(size+delta)));
		if ( nullptr == nbuf ) return false;

		if ( pos > 0 ) ::memcpy(nbuf, buf, pos);

		buf = nbuf;
		size += delta;
		type = CT_MALLOC;
	}
	else if ( delta )
	{
		char* nbuf(static_cast<char*>(::realloc(const_cast<char*>(buf), size+delta)));
		if ( nullptr == nbuf ) return false;

		if ( nbuf not_eq buf ) buf = nbuf;
		size += delta;
	}

	::memcpy(const_cast<char*>(buf)+pos, ibuf, iblen);

	return true;
}

};//namespace pw
