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
 * \file pw_string.h
 * \brief String utility.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_string.h"
#include "./pw_tokenizer.h"

namespace pw {

#ifdef PWT
#	undef PWT
#endif

#ifndef PWF
#	undef PWF
#endif

#define PWT	true
#define PWF	false

#define PW_BASE36_BUFF_SIZE	40

static const bool g_white_spaces_table[] = {
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWT,  PWT, PWT, PWT, PWT, PWT, PWT, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,  PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWT, PWF, PWF, PWF, PWF, PWF, PWF, PWF,  PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,  PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,  PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,  PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,  PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,  PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,  PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,  PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,  PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,  PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,  PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,  PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,  PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,  PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF
};

static const char g_white_spaces_cstr[8] = { 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x20 };
static const std::string g_white_spaces( g_white_spaces_cstr, 8 );

static const char g_base36_table[36] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

#define _begin(b,l)		((uint8_t*)(b))
#define _end(b,l)		((uint8_t*)(b)+(l))
#define _rbegin(b,l)	((uint8_t*)(b)+(l)-1)
#define _rend(b,l)		((uint8_t*)(b)-1)

static
inline
size_t
_getUtf8Size(const uint8_t* ib, const uint8_t* ie)
{
	auto s(ib);

	while ( ++ib not_eq ie )
	{
		if ( ((*ib) bitand 0xc0) not_eq 0x80 ) break;
	}

	return size_t(ib-s);
}

static
inline
size_t
_getUtf8Size(const uint8_t* ib)
{
	auto s(ib);

	while ( *(++ib) )
	{
		if ( ((*ib) bitand 0xc0) not_eq 0x80 ) break;
	}

	return size_t(ib-s);
}

#if 0
inline
static
const char*
_find_first_of(const char* ib, const char* ie, const char* del)
{
	while ( ib not_eq ie )
	{
		if ( strchr(del, *ib) not_eq nullptr ) break;
		++ib;
	}
	return ib;
}

inline
static
const char*
_find_first_not_of(const char* ib, const char* ie, const char* del)
{
	while ( ib not_eq ie )
	{
		if ( strchr(del, *ib) == nullptr ) break;
		++ib;
	}
	return ib;
}

inline
static
const uint8_t*
_findWhiteSpace(const uint8_t* ib, const uint8_t* ie)
{
	while ( ib not_eq ie )
	{
		if ( g_white_spaces_table[*ib] ) return ib;
		++ib;
	}

	return ie;
}
#endif

inline
static
const uint8_t*
_findWhiteSpaceNot(const uint8_t* ib, const uint8_t* ie)
{
	while ( ib not_eq ie )
	{
		if ( !g_white_spaces_table[*ib] ) return ib;
		++ib;
	}

	return ie;
}

#if 0
inline
static
const uint8_t*
_rfindWhiteSpace(const uint8_t* ib, const uint8_t* ie)
{
	while ( ib not_eq ie )
	{
		if ( g_white_spaces_table[*ib] ) return ib;
		--ib;
	}

	return ie;
}
#endif

inline
static
const uint8_t*
_rfindWhiteSpaceNot(const uint8_t* ib, const uint8_t* ie)
{
	while ( ib not_eq ie )
	{
		if ( !g_white_spaces_table[*ib] ) return ib;
		--ib;
	}

	return ie;
}

#if 0
static
std::ostream&
dump(std::ostream& os, const ios_base::iostate& s)
{
	if ( s&ios_base::goodbit ) os << 'G';
	if ( s&ios_base::eofbit ) os << 'E';
	if ( s&ios_base::failbit ) os << 'F';
	if ( s&ios_base::badbit ) os << 'B';
	return os;
}
#endif

std::string&
StringUtility::format(std::string& out, const char* fmt, ...)
{
	int errno_backup(errno);

	va_list lst;

	va_start(lst, fmt);
	const int size(vsnprintf(nullptr, 0, fmt, lst));
	va_end(lst);

	std::string tmp;

	if ( size > 0 )
	{
		va_start(lst, fmt);
		tmp.resize(size);
		vsnprintf(const_cast<char*>(tmp.c_str()), size+1, fmt, lst);
		va_end(lst);
	}

	out.swap(tmp);
	if ( errno not_eq errno_backup ) errno = errno_backup;

	return out;
}

std::string&
StringUtility::formatV(std::string& out, const char* fmt, va_list lst)
{
	int errno_backup(errno);

	va_list lst_counter;
	va_copy(lst_counter, lst);
	const int size(vsnprintf(nullptr, 0, fmt, lst_counter));
	va_end(lst_counter);

	std::string tmp;
	if ( size > 0 )
	{
		tmp.resize(size);
		vsnprintf(const_cast<char*>(tmp.c_str()), size+1, fmt, lst);
	}

	out.swap(tmp);
	if ( errno not_eq errno_backup ) errno = errno_backup;

	return out;
}

std::string
StringUtility::formatS(const char* fmt, ...)
{
	int errno_backup(errno);

	va_list lst;

	va_start(lst, fmt);
	const int size(vsnprintf(nullptr, 0, fmt, lst));
	va_end(lst);

	std::string tmp;

	if ( size > 0 )
	{
		va_start(lst, fmt);
		tmp.resize(size);
		vsnprintf(const_cast<char*>(tmp.c_str()), size+1, fmt, lst);
		va_end(lst);
	}

	if ( errno not_eq errno_backup ) errno = errno_backup;

	return tmp;
}

std::string
StringUtility::formatSV(const char* fmt, va_list lst)
{
	int errno_backup(errno);

	va_list lst_counter;
	va_copy(lst_counter, lst);
	const int size(vsnprintf(nullptr, 0, fmt, lst_counter));
	va_end(lst_counter);

	std::string tmp;
	if ( size > 0 )
	{
		tmp.resize(size);
		vsnprintf(const_cast<char*>(tmp.c_str()), size+1, fmt, lst);
	}

	if ( errno not_eq errno_backup ) errno = errno_backup;

	return tmp;
}

char*
StringUtility::findLine(const char* _p, size_t blen)
{
#if 0
	std::cerr << "findLine: ";
	std::cerr.write(_p, blen);
	std::cerr << "\r\n";
#endif

	char* ib(const_cast<char*>(_p));
	char* ie(ib+blen);

	while ( ib not_eq ie )
	{
		if ( '\r' == (*ib) )
		{
			char* ibnext(ib+1);
			if ( ibnext == ie ) return nullptr;
			if ( '\n' == (*ibnext) ) return ib;
		}

		++ib;
	}

	return nullptr;
}

std::istream&
StringUtility::getLine(std::string& out, std::istream& is)
{
	char buf[DEFAULT_BUFFER_SIZE];
	ios_base::iostate istat(is.rdstate());

	std::string tmp;

	while(!(istat&ios_base::eofbit))
	{
		//dump(std::cerr, istat);
		buf[0] = 0x00;

		is.getline(buf, sizeof(buf));
		tmp.append(buf);
		istat = is.rdstate();

		if (istat&ios_base::failbit)
		{
			is.clear(istat&~ios_base::failbit);
			continue;
		}

		if (ios_base::goodbit == istat)
		{
			break;
		}
	}

	out.swap(tmp);

	return is;
}

std::string&
StringUtility::trim(std::string& out, const char* in, size_t ilen)
{
	if ( ilen == 0 ) return out;

	const uint8_t* ileft(_begin(in, ilen));

	// find left bound
	do {
		const uint8_t* ib(_begin(in, ilen));
		const uint8_t* ie(_end(in, ilen));
		if ( (ib = _findWhiteSpaceNot(ib, ie)) == ie )
		{
			out.clear();
			return out;
		}

		ileft = ib;
	} while (false);

	// find right bound
	const uint8_t* iright( _rfindWhiteSpaceNot(_rbegin(in, ilen), _rend(in, ilen)) );

	const size_t cplen(size_t(iright - ileft) + 1);
	std::string tmp((char*)ileft, cplen);
	out.swap(tmp);
	return out;
}

std::string&
StringUtility::trimLeft(std::string& out, const char* in, size_t ilen)
{
	if ( ilen == 0 ) return out;

	const uint8_t* ib(_begin(in, ilen));
	const uint8_t* ie(_end(in, ilen));

	if ( (ib = _findWhiteSpaceNot(ib, ie)) == ie )
	{
		out.clear();
		return out;
	}

	const size_t cplen(size_t(ie - ib));
	std::string tmp((char*)ib, cplen);
	out.swap(tmp);
	return out;
}

std::string&
StringUtility::trimRight(std::string& out, const char* in, size_t ilen)
{
	if ( ilen == 0 ) return out;

	const uint8_t* ib(_rbegin(in, ilen));
	const uint8_t* ie(_rend(in, ilen));

	if ( (ib = _rfindWhiteSpaceNot(ib, ie)) == ie )
	{
		out.clear();
		return out;
	}

	const size_t cplen(size_t(ib - ((uint8_t*)in)) + 1);
	std::string tmp(in, cplen);
	out.swap(tmp);
	return out;
}

std::ostream&
StringUtility::merge(std::ostream& os, const string_list& in, char del)
{
	if ( in.empty() ) return os;

	string_list::const_iterator ib(in.begin());
	string_list::const_iterator ie(in.end());

	--ie;

	while ( ib not_eq ie )
	{
		os << *ib;
		os << del;
		++ib;
	}

	os << *ib;
	return os;
}

std::ostream&
StringUtility::merge(std::ostream& os, const string_list& in, const std::string& del)
{
	if ( in.empty() ) return os;

	string_list::const_iterator ib(in.begin());
	string_list::const_iterator ie(in.end());

	--ie;

	while ( ib not_eq ie )
	{
		os << *ib;
		os << del;
		++ib;
	}

	os << *ib;
	return os;
}

string_list&
StringUtility::split(string_list& out, const char* buf, size_t blen, char d)
{
	Tokenizer tok(buf, blen);
	string_list tmp;
	std::string tstr;
	while (tok.getNext2(tstr, d))
	{
		tmp.push_back(tstr);
	}

	out.swap(tmp);
	return out;
}

string_list&
StringUtility::split2(string_list& out, const std::string& buf, const char* del)
{
	string_list _out;

	if ( nullptr == del ) del = g_white_spaces.c_str();

	const size_t n(buf.size());
	size_t start(buf.find_first_not_of(del));
	size_t stop;

	while ( start < n )
	{
		stop = buf.find_first_of(del, start);
		if ( stop > n ) stop = n;

		_out.push_back(buf.substr(start, stop - start));
		start = buf.find_first_not_of(del, stop+1);
	}

	_out.swap(out);
	return out;
}

char*
StringUtility::toLower(char* out, const char* in, size_t len)
{
	if ( len == size_t(-1) ) len = ::strlen(in);
	std::transform(in, in+len, out, ::tolower);
	return out;
}

std::string&
StringUtility::toLower(std::string& inout)
{
	std::transform(inout.begin(), inout.end(), inout.begin(), ::tolower);
	return inout;
}


std::string&
StringUtility::toLower(std::string& out, const std::string& in)
{
	std::string tmp(in);
	std::transform(in.begin(), in.end(), tmp.begin(), ::tolower);
	out.swap(tmp);
	return out;
}

char*
StringUtility::toUpper(char* out, const char* in, size_t len)
{
	if ( len == size_t(-1) ) len = ::strlen(in);
	std::transform(in, in+len, out, ::tolower);
	return out;
}

std::string&
StringUtility::toUpper(std::string& inout)
{
	std::transform(inout.begin(), inout.end(), inout.begin(), ::toupper);
	return inout;
}


std::string&
StringUtility::toUpper(std::string& out, const std::string& in)
{
	std::string tmp(in);
	std::transform(in.begin(), in.end(), tmp.begin(), ::toupper);
	out.swap(tmp);
	return out;
}

std::string&
StringUtility::toBase36(std::string& out, uintmax_t u)
{
	char buf[PW_BASE36_BUFF_SIZE];
	char* p(&buf[sizeof(buf)-1]);
	*p = 0x00;

	do {
		*(--p) = g_base36_table[u % 36];
	} while ( (u /= 36) > 0 );

	const size_t cplen((&buf[sizeof(buf)-1]) - p);
	out.assign(p, cplen);
	return out;
}

std::string&
StringUtility::toBase36(std::string& out, intmax_t i)
{
	char buf[PW_BASE36_BUFF_SIZE];
	char* p(&buf[sizeof(buf)-1]);
	*p = 0x00;

	const bool minus_sign(i < 0);
	uintmax_t u( minus_sign ? -i : i );

	do {
		*(--p) = g_base36_table[u % 36];
	} while ( (u /= 36) > 0 );

	if ( minus_sign ) *(--p) = '-';

	const size_t cplen((&buf[sizeof(buf)-1]) - p);
	out.assign(p, cplen);
	return out;
}

char*
StringUtility::toBase36(char* out, uintmax_t u, size_t* olen)
{
	char buf[PW_BASE36_BUFF_SIZE];
	char* p(&buf[sizeof(buf)-1]);
	*p = 0x00;

	do {
		*(--p) = g_base36_table[u % 36];
	} while ( (u /= 36) > 0 );

	const size_t cplen((&buf[sizeof(buf)-1]) - p);
	::memcpy(out, p, cplen+1);
	if ( olen ) *olen = cplen;
	return out;
}

char*
StringUtility::toBase36(char* out, intmax_t i, size_t* olen)
{
	char buf[PW_BASE36_BUFF_SIZE];
	char* p(&buf[sizeof(buf)-1]);
	*p = 0x00;

	const bool minus_sign(i < 0);
	uintmax_t u( minus_sign ? -i : i );

	do {
		*(--p) = g_base36_table[u % 36];
	} while ( (u /= 36) > 0 );

	if ( minus_sign ) *(--p) = '-';

	const size_t cplen((&buf[sizeof(buf)-1]) - p);
	::memcpy(out, p, cplen+1);
	if ( olen ) *olen = cplen;
	return out;
}

inline
static
uint8_t
_Bin2HexHalf(uint8_t c)
{
	if ( c < 10 ) return (uint8_t('0')+c);
	return uint8_t('A')+c-10;
}

inline
static
void
_Bin2HexFull(uint8_t out[2], uint8_t in)
{
	out[0] = _Bin2HexHalf(in >> 4);
	out[1] = _Bin2HexHalf(in & uint8_t(0x0f));
}

inline
static
void
_dumpHex(std::ostream& out, const uint8_t* in, size_t ilen)
{
	uint8_t ohex[2];

	size_t i(0);
	uint8_t u;

	while ( i < ilen )
	{
		u = *(in+i);
		_Bin2HexFull(ohex, u);
		out.write(reinterpret_cast<char*>(ohex), 2);
		out << ' ';
		if ( (( i + 1) % 8) == 0 ) out << ' ';
		++i;
	}

	while ( i < 16 )
	{
		out.write("   ", 3);
		if ( (( i + 1) % 8) == 0 ) out << ' ';
		++i;
	}

	out.write("    ", 5);

	i = 0;

	while ( i < ilen )
	{
		u = *(in+i);
		if ( not isprint(u) ) u = '.';
		out.write( reinterpret_cast<char*>(&u), 1 );
		if ( (( i + 1) % 8) == 0 ) out << ' ';
		++i;
	}

	out << endl;
}

ostream&
StringUtility::dumpHex ( std::ostream& out, const void* _in, size_t ilen)
{
	size_t cplen(0);
	const char* in(reinterpret_cast<const char*>(_in));

	while ( ilen )
	{
		cplen = std::min(ilen, size_t(16));
		_dumpHex( out, reinterpret_cast<const uint8_t*>(in), cplen );
		ilen -= cplen;
		in += cplen;
	}

	return out;
}

size_t
StringUtility::sliceUTF8 ( const char* in, size_t less_bytes )
{
	auto s(reinterpret_cast<const uint8_t*>(in));
	auto ib(s);
	size_t size(0);
	size_t sum(0);

	while ( *ib )
	{
		if ( (sum + (size = _getUtf8Size(ib))) > less_bytes )
		{
			return sum;
		}

		sum += size;
		ib += size;
	}

	return sum;
}

size_t
StringUtility::sliceUTF8 ( const char* in, size_t ilen, size_t less_bytes )
{
	auto s(reinterpret_cast<const uint8_t*>(in));
	auto ib(s);
	auto ie(ib + ilen);
	size_t size(0);
	size_t sum(0);

	while ( ib not_eq ie )
	{
		if ( (sum + (size = _getUtf8Size(ib))) > less_bytes )
		{
			return sum;
		}

		sum += size;
		ib += size;
	}

	return sum;
}

}; // namespace pw
