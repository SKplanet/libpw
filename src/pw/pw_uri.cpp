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
 * \file pw_uri.h
 * \brief Support URI(RFC 3986) with URIParser(http://uriparser.sourceforge.net/).
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_uri.h"
#include "./pw_log.h"
#include "./pw_httppacket.h"
#include "./pw_exception.h"
#ifdef HAVE_URIPARSER
#	include <uriparser/Uri.h>
#endif
#include <netdb.h>

#define _PWRELEASE(x) if (x) { delete(x); x = nullptr; }

namespace pw {

using path_list = uri_type::path_list;
using __str_ptr = uri_type::string_pointer;

#ifdef HAVE_URIPARSER
using __pathseg_ptr = UriPathSegmentA*;
using __text_ptr = UriTextRangeA*;
using __uri_ptr = UriUriA*;
#endif

template<typename _Type>
static
inline
_Type&
_memClear(_Type& out)
{
	memset(&out, 0x00, sizeof(_Type));
	return out;
}

static
inline
int
_getPort(const char* service)
{
	int port(::atoi(service));
	if ( 0 == port )
	{
		auto e(::getservbyname(service, nullptr));
		if ( e )
		{
			port = ntohs(e->s_port);
			return port;
		}
	}
	else if ( port > UINT16_MAX )
	{
		return 0;
	}

	return port;
}

static
const char*
_getErrorMessage(int err)
{
#ifdef HAVE_URIPARSER
	switch(err)
	{
		case URI_SUCCESS: return "Success";
		case URI_ERROR_SYNTAX: return "Parsed text violates expected format";
		case URI_ERROR_NULL: return "One of the params passed was NULL although it mustn't be";
		case URI_ERROR_MALLOC: return "Requested memory could not be allocated";
		case URI_ERROR_OUTPUT_TOO_LARGE: return "Some output is too large for the receiving buffer";
		case URI_ERROR_NOT_IMPLEMENTED: return "The called function is not implemented yet";
		case URI_ERROR_RANGE_INVALID: return "The parameters passed contained invalid ranges";
		case URI_ERROR_ADDBASE_REL_BASE:
		case URI_ERROR_REMOVEBASE_REL_BASE:
		case URI_ERROR_REMOVEBASE_REL_SOURCE:
			return "Given base is not absolute";
	}
#endif
	return "Unknown";
}

#ifdef HAVE_URIPARSER
inline
static
void
_assign(__str_ptr& out, const UriTextRangeA& in)
{
	if ( in.first and in.afterLast )
	{
		if ( not out )
		{
			if ((out = new std::string(in.first, size_t(in.afterLast - in.first))) == nullptr)
			{
				throw std::bad_alloc();
			}

			return;
		}

		out->assign(in.first, size_t(in.afterLast - in.first));
		return;
	}

	_PWRELEASE(out);
}
#endif

#ifdef HAVE_URIPARSER
inline
static
void
_assign(path_list& out, const UriPathSegmentA* head)
{
	path_list tmp;

	if ( head )
	{
		auto ib(head);
		do {
			auto& in(ib->text);
			if ( in.first and in.afterLast )
			{
				auto p(new std::string(in.first, size_t(in.afterLast - in.first)));
				if ( not p ) throw std::bad_alloc();
				tmp.push_back(p);
			}
			else
			{
				tmp.push_back(nullptr);
			}
		} while ( nullptr not_eq (ib = ib->next) );
	}// if ( head )

	out.swap(tmp);
	for ( auto& p : tmp )
	{
		if ( p ) delete p;
	}
}
#endif

#ifdef HAVE_URIPARSER
inline
static
void
_assign(UriTextRangeA& out, const std::string* in)
{
	if ( in )
	{
		out.first = const_cast<char*>(in->c_str());
		out.afterLast = out.first + in->size();
		return;
	}

	out.first = out.afterLast = nullptr;
}
#endif

#ifdef HAVE_URIPARSER
inline
static
__pathseg_ptr
_newPathSeg(void)
{
	auto p( static_cast<__pathseg_ptr>(::malloc(sizeof(UriPathSegmentA))) );
	if ( p ) _memClear(*p);
	return p;
}
#endif

#ifdef HAVE_URIPARSER
inline
static
void
_assign(__pathseg_ptr& head, __pathseg_ptr& tail, const path_list& in)
{
	if ( in.empty() )
	{
		head = tail = nullptr;
		return;
	}

	auto lst_ib(in.cbegin());
	auto lst_ie(in.cend());
	__pathseg_ptr tmp(nullptr), tmp_prev(nullptr);

	if ( nullptr == (tmp = _newPathSeg()) )
	{
		PWLOGLIB("not enough memory");
		return;
	}

	head = tmp;
	_assign(tmp->text, *lst_ib);

	++lst_ib;

	while ( lst_ib not_eq lst_ie )
	{
		tmp_prev = tmp;
		if ( nullptr == (tmp = _newPathSeg()) )
		{
			PWLOGLIB("not enough memory");
			return;
		}

		tmp_prev->next = tmp;
		_assign(tmp->text, *lst_ib);

		++lst_ib;
	}

	tail = tmp;
}
#endif

static
inline
__str_ptr
_copy(const __str_ptr p)
{
	if ( p )
	{
		auto p2(new std::string(*p));
		if ( nullptr == p2 ) throw std::bad_alloc();
		return p2;
	}

	return nullptr;
}

uri_type::uri_type(const uri_type& obj)
{
	data.scheme = _copy(obj.data.scheme);
	data.userInfo = _copy(obj.data.userInfo);
	data.host = _copy(obj.data.host);
	data.port = _copy(obj.data.port);
	data.query = _copy(obj.data.query);
	data.fragment = _copy(obj.data.fragment);

	for ( auto& p : obj.data.path ) data.path.push_back(_copy(p));

	data.isAbs = obj.data.isAbs;
}

uri_type::uri_type ( uri_type && obj )
{
	data.scheme = obj.data.scheme; obj.data.scheme = nullptr;
	data.userInfo = obj.data.userInfo; obj.data.userInfo = nullptr;
	data.host = obj.data.host; obj.data.host = nullptr;
	data.port = obj.data.port; obj.data.port = nullptr;
	data.query = obj.data.query; obj.data.query = nullptr;
	data.fragment = obj.data.fragment; obj.data.fragment = nullptr;

	data.path = std::move(obj.data.path);
	data.isAbs = obj.data.isAbs;
}

uri_type&
uri_type::operator= ( const uri_type& obj )
{
	uri_type tmp(obj);
	swap(tmp);
	return *this;
}

uri_type&
uri_type::operator= ( uri_type && obj )
{
	uri_type tmp(obj);
	swap(tmp);
	return *this;
}

uri_type::inner_type::~inner_type()
{
	clear();
}

void
uri_type::inner_type::clear ( void )
{
	_PWRELEASE(scheme);
	_PWRELEASE(userInfo);
	_PWRELEASE(host);
	_PWRELEASE(port);
	for ( auto& p : path ) if ( p ) delete p;
	path.clear();
	_PWRELEASE(query);
	_PWRELEASE(fragment);
	isAbs = false;
}

void
uri_type::_setFromData ( const void* _in )
{
#ifdef HAVE_URIPARSER
	auto p(static_cast<const UriUriA*>(_in));
	inner_type tmp;
	_assign(tmp.scheme, p->scheme);
	_assign(tmp.userInfo, p->userInfo);
	_assign(tmp.host, p->hostText);
	_assign(tmp.port, p->portText);
	_assign(tmp.path, p->pathHead);
	_assign(tmp.query, p->query);
	_assign(tmp.fragment, p->fragment);
	tmp.isAbs = ( p->hostText.first ? bool(p->absolutePath) : false );

	tmp.swap(data);
#else
	throw new Exception_not_implemented;
#endif
}

void
uri_type::_setToData ( void* _out ) const
{
#ifdef HAVE_URIPARSER
	auto p(static_cast<UriUriA*>(_out));
	_assign(p->scheme, data.scheme);
	_assign(p->userInfo, data.userInfo);
	_assign(p->hostText, data.host);
	_assign(p->portText, data.port);
	_assign(p->pathHead, p->pathTail, data.path);
	_assign(p->query, data.query);
	_assign(p->fragment, data.fragment);
	p->absolutePath = data.isAbs;
	p->owner = false;
	p->reserved = nullptr;
#else
	throw new Exception_not_implemented;
#endif
}

bool
uri_type::parse ( const char* s )
{
#ifdef HAVE_URIPARSER
	UriUriA uri;
	UriParserStateA state{&uri};

	auto res(uriParseUriA(&state, s));
	if ( res == URI_SUCCESS )
	{
		_setFromData(&uri);
		uriFreeUriMembersA(&uri);
		return true;
	}

	PWTRACE("%s", _getErrorMessage(res));

	uriFreeUriMembersA(&uri);
#endif
	return false;
}

bool
uri_type::parse ( const char* s, size_t slen )
{
#ifdef HAVE_URIPARSER
	UriUriA uri;
	_memClear(uri);
	UriParserStateA state{&uri};

	auto res(uriParseUriExA(&state, s, s+slen));
	if ( res == URI_SUCCESS )
	{
		_setFromData(&uri);
		uriFreeUriMembersA(&uri);
 		return true;
	}

	PWTRACE("%s", _getErrorMessage(res));

	uriFreeUriMembersA(&uri);
#endif
	return false;
}

int
uri_type::getNumericPort(void) const
{
	if ( data.port )
	{
		int port( _getPort(data.port->c_str()));
		if ( port ) return port;
	}

	return (data.scheme ? _getPort(data.scheme->c_str()) : 0);
}

std::string&
uri_type::getPathString ( std::string& out ) const
{
	if ( data.path.empty() )
	{
		out.assign(1, '/');
		return out;
	}

	// count
	size_t cplen(0);
	for ( auto& txt : data.path )
	{
		cplen += (txt ? txt->size() : 0) + 1;
	}

	// assign
	std::string tmp;
	tmp.reserve(cplen);

	for ( auto& txt : data.path )
	{
		tmp.append(1, '/');
		if ( txt ) tmp.append(*txt);
	}

	tmp.swap(out);

	return out;
}

std::string
uri_type::getPathString ( void ) const
{
	if ( data.path.empty() )
	{
		return std::string(1, '/');
	}

	// count
	size_t cplen(0);
	for ( auto& txt : data.path )
	{
		cplen += (txt ? txt->size() : 0) + 1;
	}

	// assign
	std::string tmp;
	tmp.reserve(cplen);

	for ( auto& txt : data.path )
	{
		tmp.append(1, '/');
		if ( txt ) tmp.append(*txt);
	}

	return tmp;
}

std::string&
uri_type::str ( std::string& out ) const
{
#ifdef HAVE_URIPARSER
	int cplen(0), res(0);
	UriUriA uri;
	_memClear(uri);
	_setToData(&uri);

	if ( ( res = uriToStringCharsRequiredA(&uri, &cplen) ) == URI_SUCCESS )
	{
		if ( cplen > 1 )
		{
			++cplen;

			std::string tmp(cplen, 0x00);
			auto out_ptr(const_cast<char*>(tmp.c_str()));
			int written(0);

			if ( ( res = uriToStringA(out_ptr, &uri, cplen, &written)) == URI_SUCCESS )
			{
				tmp.resize(written - 1);
				out.swap(tmp);
				uriFreeUriMembersA(&uri);
				return out;
			}

			PWTRACE("uriToStringA: %s, cplen:%d written: %d", _getErrorMessage(res), cplen, written);
		}
	}
	else
	{
		PWTRACE("uriToStringA: %s, cplen: %d", _getErrorMessage(res), cplen);
	}

	out.clear();
	uriFreeUriMembersA(&uri);

	return out;
#else
	throw new Exception_not_implemented;
#endif
}

bool
uri_type::normalize ( void )
{
#ifdef HAVE_URIPARSER
	UriUriA uri;
	_memClear(uri);
	_setToData(&uri);

	auto flags(uriNormalizeSyntaxMaskRequiredA(&uri));
	auto res(uriNormalizeSyntaxExA(&uri, flags));

	if ( res == URI_SUCCESS )
	{
		_setFromData(&uri);
		uriFreeUriMembersA(&uri);
		return true;
	}

	PWTRACE("uriNormalizeSyntaxExA: %s", _getErrorMessage(res));

	uriFreeUriMembersA(&uri);
#endif
	return false;
}

bool
uri_type::normalize( uri_type& out ) const
{
#ifdef HAVE_URIPARSER
	UriUriA uri;
	_memClear(uri);
	_setToData(&uri);

	auto flags(uriNormalizeSyntaxMaskRequiredA(&uri));
	auto res(uriNormalizeSyntaxExA(&uri, flags));

	if ( res == URI_SUCCESS )
	{
		out._setFromData(&uri);
		uriFreeUriMembersA(&uri);
		return true;
	}

	PWTRACE("uriNormalizeSyntaxExA: %s", _getErrorMessage(res));

	uriFreeUriMembersA(&uri);
	out.clear();
#endif
	return false;
}

void
uri_type::setPath ( const std::string& in )
{
	Tokenizer tok(in);
	path_list lst;
	std::string pathSeg;
	while ( tok.getNext(pathSeg, '/') )
	{
		auto p(new std::string(pathSeg));
		if ( not p ) throw std::bad_alloc();
		lst.push_back( p );
	}

	data.path.swap(lst);
	for ( auto& p : lst )
	{
		if ( p ) delete p;
	}
}

bool
uri_type::setPortByService(const std::string& in)
{
	auto e(::getservbyname(in.c_str(), nullptr));
	if ( e )
	{
		auto port(ntohs(e->s_port));
		auto port_str(std::to_string(port));
		if ( data.port ) *data.port = port_str;
		else if ( nullptr == (data.port = new std::string(port_str)) )
		{
			throw std::bad_alloc();
		}

		return true;
	}

	return false;
}

bool
uri_type::setPortByScheme ( void )
{
	if ( isNullScheme() ) return false;
	return setPortByService(getRefOfScheme());
}

keyvalue_cont&
uri_type::getQuery ( keyvalue_cont& out ) const
{
	if ( data.query )
	{
		http::splitUrlencodedForm(out, *data.query);
	}
	else
	{
		out.clear();
	}

	return out;
}

bool
uri_type::addBase ( uri_type& out, const uri_type& base ) const
{
#ifdef HAVE_URIPARSER
	UriUriA out_uri, base_uri, this_uri;
	_memClear(out_uri);
	_memClear(base_uri);
	_memClear(this_uri);
	base._setToData(&base_uri);
	_setToData(&this_uri);

	int res;
	if ( (res = uriAddBaseUriA(&out_uri, &this_uri, &base_uri)) == URI_SUCCESS )
	{
		out._setFromData(&out_uri);
		uriFreeUriMembersA(&out_uri);
		uriFreeUriMembersA(&base_uri);
		uriFreeUriMembersA(&this_uri);

		return true;
	}

	uriFreeUriMembersA(&out_uri);
	uriFreeUriMembersA(&base_uri);
	uriFreeUriMembersA(&this_uri);
	out.clear();

	PWTRACE("failed to addBase: %s", _getErrorMessage(res));
#endif
	return false;
}

bool
uri_type::addBase ( const uri_type& base )
{
#ifdef HAVE_URIPARSER
	UriUriA out_uri, base_uri, this_uri;
	_memClear(out_uri);
	_memClear(base_uri);
	_memClear(this_uri);
	base._setToData(&base_uri);
	_setToData(&this_uri);

	int res;
	if ( (res = uriAddBaseUriA(&out_uri, &this_uri, &base_uri)) == URI_SUCCESS )
	{
		_setFromData(&out_uri);
		uriFreeUriMembersA(&out_uri);
		uriFreeUriMembersA(&base_uri);
		uriFreeUriMembersA(&this_uri);

		return true;
	}

	uriFreeUriMembersA(&out_uri);
	uriFreeUriMembersA(&base_uri);
	uriFreeUriMembersA(&this_uri);

	PWTRACE("failed to addBase: %s", _getErrorMessage(res));
#endif
	return false;
}

bool
uri_type::removeBase ( uri_type& out, const uri_type& base, bool useDomainRoot ) const
{
#ifdef HAVE_URIPARSER
	UriUriA out_uri, base_uri, this_uri;
	_memClear(out_uri);
	_memClear(base_uri);
	_memClear(this_uri);
	base._setToData(&base_uri);
	_setToData(&this_uri);

	int res;
	if ( (res = uriRemoveBaseUriA(&out_uri, &this_uri, &base_uri, useDomainRoot)) == URI_SUCCESS )
	{
		out._setFromData(&out_uri);
		uriFreeUriMembersA(&out_uri);
		uriFreeUriMembersA(&base_uri);
		uriFreeUriMembersA(&this_uri);

		return true;
	}

	uriFreeUriMembersA(&out_uri);
	uriFreeUriMembersA(&base_uri);
	uriFreeUriMembersA(&this_uri);
	out.clear();

	PWTRACE("failed to removeBase: %s", _getErrorMessage(res));
#endif
	return false;
}

bool
uri_type::removeBase ( const uri_type& base, bool useDomainRoot )
{
#ifdef HAVE_URIPARSER
	UriUriA out_uri, base_uri, this_uri;
	_memClear(out_uri);
	_memClear(base_uri);
	_memClear(this_uri);
	base._setToData(&base_uri);
	_setToData(&this_uri);

	int res;
	if ( (res = uriRemoveBaseUriA(&out_uri, &this_uri, &base_uri, useDomainRoot)) == URI_SUCCESS )
	{
		_setFromData(&out_uri);
		uriFreeUriMembersA(&out_uri);
		uriFreeUriMembersA(&base_uri);
		uriFreeUriMembersA(&this_uri);

		return true;
	}

	uriFreeUriMembersA(&out_uri);
	uriFreeUriMembersA(&base_uri);
	uriFreeUriMembersA(&this_uri);

	PWTRACE("failed to removeBase: %s", _getErrorMessage(res));
#endif
	return false;
}

//namespace pw
};

std::ostream&
operator<< ( std::ostream& out, const pw::uri_type& uri )
{
	out << uri.str();
	return out;
}
