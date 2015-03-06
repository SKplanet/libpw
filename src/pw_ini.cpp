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
 * \file pw_ini.h
 * \brief Support INI file parser.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_ini.h"
#include "./pw_encode.h"
#include "./pw_string.h"
#include "./pw_log.h"
#include "./pw_tokenizer.h"

namespace pw {

static const std::string g_strTrue("true");
static const std::string g_strFalse("false");

Ini::Ini()
{
	item_cont tmp;
	tmp.insert(item_cont::value_type("", ""));
	m_cont.insert(sec_cont::value_type("", tmp));
}

Ini::Ini(const char* path, Event* e)
{
	this->read(path, e);
}

Ini::Ini(std::istream& is, Event* e)
{
	this->read(is, e);
}

bool
Ini::write(const char* path, Event* e) const
{
	std::ofstream ofs(path);
	if ( ofs.is_open() ) return write(ofs, e);
	PWLOGLIB("Ini::write: failed to open: %s", path);

	return false;
}

bool
Ini::write(std::ostream& os, Event* e) const
{
	sec_citr ibsec(m_cont.begin()), iesec(m_cont.end());
	item_citr ibitem, ieitem;
	std::string value;

	while ( ibsec not_eq iesec )
	{
		const std::string& secname(ibsec->first);
		const item_cont& c(ibsec->second);

		if ( e )
		{
			if ( !e->eventSection(secname) ) return false;
		}

		if ( secname.empty() and c.size() == 1 )
		{
			ibitem = c.begin();
			if ( ibitem->first.empty() and ibitem->second.empty() )
			{
				++ ibsec;
				continue;
			}
		}

		os << '[' << secname << ']' << "\r\n";

		ibitem = c.begin();
		ieitem = c.end();
		while ( ibitem not_eq ieitem )
		{
			if ( e )
			{
				bool add(true);
				if ( !e->eventItem(ibitem->second, ibitem->first, secname, add) ) return false;
				if ( !add ) os << ';';
			}

			os << ibitem->first << '=' << PWEnc::encodeEscape(value, ibitem->second) << "\r\n";
			++ibitem;
		}

		os << "\r\n";

		++ibsec;
	}

	return true;
}

bool
Ini::read(const char* path, Event* e)
{
	std::ifstream ifs(path);
	if ( ifs.is_open() ) return read(ifs, e);
	PWLOGLIB("Ini::read: no file: %s", path);

	return false;
}

bool
Ini::read(std::istream& is, Event* e)
{
	Ini tmp;
	Tokenizer tok;
	std::string line, secname, itemname, value;
	size_t linelen(0);

	sec_itr isec(tmp.m_cont.begin());
	char c;
	//while ( !PWStr::getLine(line, is).eof() )
	while ( not std::getline(is, line).eof() )
	{
		++linelen;

		//PWLOGLIB("line: <%s>", line.c_str());
		if ( PWStr::trimLeft(line).empty() ) continue;
		c = line[0];

		// comment
		if ( c == '#' || c == '\'' || c == '`' || c == '\"' || c == ';' ) continue;

		// section?
		// 섹션이면 섹션을 추가한다.
		// [ ... ]이 제대로된 섹션이면 추가하고,
		// 아니면 아이템으로 처리한다.
		if ( line[0] == '[' )
		{
			PWStr::trimRight(line);
			if ( *(line.end() - 1) == ']' )
			{
				line.substr(1, line.size()-2).swap(secname);
				if ( e )
				{
					if ( !e->eventSection(secname) ) return false;
				}

				if ( (isec = tmp.m_cont.find(secname)) == tmp.m_cont.end() )
				{
					item_cont item;
					item[""] = "";

					isec = tmp.m_cont.insert(sec_cont::value_type(secname, item)).first;
					if ( isec == tmp.m_cont.end() )
					{
						PWLOGLIB("Ini::read: not enough memory: secname: %s", secname.c_str());
						return false;
					}
				}// if ( end == (isec = find(sec)) )

				continue;
			}
		}

		// item?
		// key = value
		tok.setBuffer(line);
		if ( tok.getNext(itemname, '=') )
		{
			value.assign(tok.getPosition(), tok.getLeftSize());
			PWEnc::decodeEscape(value);
			PWStr::trimLeft(value);
			PWStr::trimRight(itemname);
		}
		else
		{
			itemname = line;
			value.clear();
		}

		if ( e )
		{
			bool add(true);
			if ( !e->eventItem(value, itemname, secname, add) ) return false;
			if ( !add ) continue;
		}

		isec->second[itemname] = value;
	}

	this->swap(tmp);
	return true;
}

bool
Ini::getBoolean(const std::string& item, const std::string& sec, bool def) const
{
	const std::string* s(getItem(item, sec));
	return ( nullptr == s ) ? def : PWStr::toBoolean(*s);
}

intmax_t
Ini::getInteger(const std::string& item, const std::string& sec, intmax_t def) const
{
	const std::string* s(getItem(item, sec));
	return ( nullptr == s ) ? (def) : (::strtoimax(s->c_str(), nullptr, 10));
}

long double
Ini::getReal(const std::string& item, const std::string& sec, long double def) const
{
	const std::string* s(getItem(item, sec));
	return ( nullptr == s ) ? def : ::strtold(s->c_str(), nullptr);
}

std::string
Ini::getString(const std::string& item, const std::string& sec, const std::string& def) const
{
	const std::string* s(getItem(item, sec));
	return ( nullptr == s ) ? def : *s;
}

std::string&
Ini::getString2(std::string& out, const std::string& item, const std::string& sec, const std::string& def) const
{
	const std::string* s(getItem(item, sec));
	out = ( nullptr == s ) ? def : *s;
	return out;
}

bool
Ini::getBoolean(const std::string& item, sec_citr sec, bool def) const
{
	item_citr ib(sec->second.find(item));
	return ( sec->second.end() == ib ) ? def : PWStr::toBoolean(ib->second);
}

intmax_t
Ini::getInteger(const std::string& item, sec_citr sec, intmax_t def) const
{
	item_citr ib(sec->second.find(item));
	return ( sec->second.end() == ib ) ? def : ::strtoimax(ib->second.c_str(), nullptr, 10);
}

long double
Ini::getReal(const std::string& item, sec_citr sec, long double def) const
{
	item_citr ib(sec->second.find(item));
	return ( sec->second.end() == ib ) ? def : ::strtold(ib->second.c_str(), nullptr);
}

std::string
Ini::getString(const std::string& item, sec_citr sec, const std::string& def) const
{
	item_citr ib(sec->second.find(item));
	return ( sec->second.end() == ib ) ? def : ib->second;
}

std::string&
Ini::getString2(std::string& out, const std::string& item, sec_citr sec, const std::string& def) const
{
	item_citr ib(sec->second.find(item));
	out = ( sec->second.end() == ib ) ? def : ib->second;
	return out;
}

bool
Ini::getHost(host_type& host, const std::string& item, const std::string& sec) const
{
	std::string line;
	getString2(line, item, sec);
	if ( line.empty() ) return false;
	return host.read(line);
}

host_list_type&
Ini::getHostList(host_list_type& host, const std::string& item, const std::string& sec) const
{
	std::string line;
	getString2(line, item, sec);
	if ( line.empty() ) host.clear();
	else host_type::s_read(host, line);
	return host;
}

bool
Ini::getUrl(url_type& url, const std::string& item, const std::string& sec) const
{
	std::string line;
	getString2(line, item, sec);
	if ( line.empty() ) return false;
	return url.read(line);
}

url_list_type&
Ini::getUrlList(url_list_type& url, const std::string& item, const std::string& sec) const
{
	std::string line;
	getString2(line, item, sec);
	if ( line.empty() ) url.clear();
	else url_type::s_read(url, line);
	return url;
}

bool
Ini::getHost(host_type& host, const std::string& item, sec_citr sec) const
{
	std::string line;
	getString2(line, item, sec);
	if ( line.empty() ) return false;
	return host.read(line);
}

host_list_type&
Ini::getHostList(host_list_type& host, const std::string& item, sec_citr sec) const
{
	std::string line;
	getString2(line, item, sec);
	if ( line.empty() ) host.clear();
	else host_type::s_read(host, line);
	return host;
}

bool
Ini::getUrl(url_type& url, const std::string& item, sec_citr sec) const
{
	std::string line;
	getString2(line, item, sec);
	if ( line.empty() ) return false;
	return url.read(line);
}

url_list_type&
Ini::getUrlList(url_list_type& url, const std::string& item, sec_citr sec) const
{
	std::string line;
	getString2(line, item, sec);
	if ( line.empty() ) url.clear();
	else url_type::s_read(url, line);
	return url;
}

bool
Ini::getUri ( uri_type& uri, const std::string& item, const std::string& sec ) const
{
	std::string line;
	getString2(line, item, sec);
	if ( line.empty() ) return false;
	return uri.parse(line);
}

bool
Ini::getUri ( uri_type& uri, const std::string& item, sec_citr sec ) const
{
	std::string line;
	getString2(line, item, sec);
	if ( line.empty() ) return false;
	return uri.parse(line);
}

void
Ini::setBoolean(bool v, const std::string& item, const std::string& sec)
{
	setString(v?g_strTrue:g_strFalse, item, sec);
}

void
Ini::setBoolean(bool v, const std::string& item, sec_itr sec)
{
	setString(v?g_strTrue:g_strFalse, item, sec);
}

void
Ini::setInteger(intmax_t v, const std::string& item, const std::string& sec)
{
	std::string tmp;
	PWStr::format(tmp, "%jd", v);
	setString(tmp, item, sec);
}

void
Ini::setInteger(intmax_t v, const std::string& item, sec_itr sec)
{
	std::string tmp;
	PWStr::format(tmp, "%jd", v);
	setString(tmp, item, sec);
}

void
Ini::setReal(long double v, const std::string& item, const std::string& sec)
{
	std::string tmp;
	PWStr::format(tmp, "%LF", v);
	setString(tmp, item, sec);
}

void
Ini::setReal(long double v, const std::string& item, sec_itr sec)
{
	std::string tmp;
	PWStr::format(tmp, "%LF", v);
	setString(tmp, item, sec);
}

void
Ini::setString(const std::string& v, const std::string& item, const std::string& sec)
{
	m_cont[sec][item] = v;
}

void
Ini::setString(const std::string& v, const std::string& item, sec_itr sec)
{
	sec->second[item] = v;
}

void
Ini::setHost(const host_type& v, const std::string& item, const std::string& sec)
{
	std::string tmp;
	v.write(tmp);
	setString(tmp, item, sec);
}

void
Ini::setHost(const host_type& v, const std::string& item, sec_itr sec)
{
	std::string tmp;
	v.write(tmp);
	setString(tmp, item, sec);
}

void
Ini::setHostList(const host_list_type& v, const std::string& item, const std::string& sec)
{
	std::string tmp;
	host_type::s_write(tmp, v);
	setString(tmp, item, sec);
}

void
Ini::setHostList(const host_list_type& v, const std::string& item, sec_itr sec)
{
	std::string tmp;
	host_type::s_write(tmp, v);
	setString(tmp, item, sec);
}

void
Ini::setUrl(const url_type& v, const std::string& item, const std::string& sec)
{
	std::string tmp;
	v.write(tmp);
	setString(tmp, item, sec);
}

void
Ini::setUrl(const url_type& v, const std::string& item, sec_itr sec)
{
	std::string tmp;
	v.write(tmp);
	setString(tmp, item, sec);
}

void
Ini::setUrlList(const url_list_type& v, const std::string& item, const std::string& sec)
{
	std::string tmp;
	url_type::s_write(tmp, v);
	setString(tmp, item, sec);
}

void
Ini::setUrlList(const url_list_type& v, const std::string& item, sec_itr sec)
{
	std::string tmp;
	url_type::s_write(tmp, v);
	setString(tmp, item, sec);
}

void
Ini::setUri ( const uri_type& v, const std::string& item, const std::string& sec )
{
	std::string tmp;
	v.str(tmp);
	setString(tmp, item, sec);
}

void
Ini::setUri ( const uri_type& v, const std::string& item, sec_itr sec )
{
	std::string tmp;
	v.str(tmp);
	setString(tmp, item, sec);
}

};//namespace pw

