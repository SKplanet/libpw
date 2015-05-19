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
 * \file pw_date.cpp
 * \brief Support time/date string and ASN.1 type.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_date.h"
#include "./pw_string.h"
#include <sys/time.h>

namespace pw
{

#define PW_USEC		(1000000LL)

static
inline
time_t
_makeTime(const struct tm& tm)
{
	struct tm tm2(tm);
	return mktime(&tm2);
}

static
int
_str2int(const char* _in, size_t _inlen)
{
	char buf[4+1];
	memcpy(buf, _in, _inlen);
	buf[_inlen] = 0x00;
	return atoi(buf);
}

static
inline
bool
_makeASN1ToTime(time_t& out, int& out_msec, const std::string& in)
{
	//20140328180858.998		(18)
	//20140328090858.998Z		(19)
	//20140328180858.998+0900	(23)
	//140328090858Z				(13)
	//1403280908Z				(11)

	struct tm tm;
	memset(&tm, 0x00, sizeof(tm));
	const char* p(in.c_str());

	switch(in.size())
	{
	case 18: //20140328180858.998		(18)
	{
		tm.tm_year = _str2int(p, 4) - 1900; p += 4;
		tm.tm_mon = _str2int(p, 2) - 1; p += 2;
		tm.tm_mday = _str2int(p, 2); p += 2;
		tm.tm_hour = _str2int(p, 2); p += 2;
		tm.tm_min = _str2int(p, 2); p += 2;
		tm.tm_sec = _str2int(p, 2); p += 2;
		if ( *p not_eq '.' ) break;
		++p;
		out_msec = _str2int(p, 3);
		out = mktime(&tm);
		return true;
	}
	case 19: //20140328090858.998Z		(19)
	{
		tm.tm_year = _str2int(p, 4) - 1900; p += 4;
		tm.tm_mon = _str2int(p, 2) - 1; p += 2;
		tm.tm_mday = _str2int(p, 2); p += 2;
		tm.tm_hour = _str2int(p, 2); p += 2;
		tm.tm_min = _str2int(p, 2); p += 2;
		tm.tm_sec = _str2int(p, 2); p += 2;
		if ( *p not_eq '.' ) break;
		++p;
		out_msec = _str2int(p, 3);
		p+=3;
		if ( *p not_eq 'Z' ) break;
		out = mktime(&tm) - timezone;
		return true;
	}
	case 23: //20140328180858.998+0900	(23)
	{
		tm.tm_year = _str2int(p, 4) - 1900; p += 4;
		tm.tm_mon = _str2int(p, 2) - 1; p += 2;
		tm.tm_mday = _str2int(p, 2); p += 2;
		tm.tm_hour = _str2int(p, 2); p += 2;
		tm.tm_min = _str2int(p, 2); p += 2;
		tm.tm_sec = _str2int(p, 2); p += 2;
		if ( *p not_eq '.' ) break;
		++p;
		out_msec = _str2int(p, 3);
		p+=3;

		int sign(0);
		if ( *p == '+' ) sign = 1;
		else if ( *p == '-' ) sign = -1;
		else break;
		++p;

		int hour(_str2int(p, 2)); p += 2;
		int min(_str2int(p, 2)); p += 2;
		int myzone(((hour * 3600) + (min * 60)) * sign);

		out = mktime(&tm) + myzone;
		return true;
	}
	case 13: //140328090858Z				(13)
	{
		tm.tm_year = _str2int(p, 2); p += 2;
		if ( tm.tm_year < 50 ) tm.tm_year += 100;
		tm.tm_mon = _str2int(p, 2) - 1; p += 2;
		tm.tm_mday = _str2int(p, 2); p += 2;
		tm.tm_hour = _str2int(p, 2); p += 2;
		tm.tm_min = _str2int(p, 2); p += 2;
		tm.tm_sec = _str2int(p, 2); p += 2;
		if ( *p not_eq 'Z' ) return false;

		out = mktime(&tm) - timezone;
		return true;
	}
	case 11: //1403280908Z				(11)
	{
		tm.tm_year = _str2int(p, 2); p += 2;
		if ( tm.tm_year < 50 ) tm.tm_year += 100;
		tm.tm_mon = _str2int(p, 2) - 1; p += 2;
		tm.tm_mday = _str2int(p, 2); p += 2;
		tm.tm_hour = _str2int(p, 2); p += 2;
		tm.tm_min = _str2int(p, 2); p += 2;
		if ( *p not_eq 'Z' ) return false;

		out = mktime(&tm) - timezone;
		return true;
	}
	}

	return false;
}

static
inline
std::string&
_makeASN1UTC(std::string& out, bool use_sec, time_t sec)
{
	struct tm tm;
	gmtime_r(&sec, &tm);
	if ( use_sec )
	{
		PWStr::format(out, "%02d%02d%02d%02d%02d%02dZ", int(tm.tm_year%100), int(tm.tm_mon+1), tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	}
	else
	{
		PWStr::format(out, "%02d%02d%02d%02d%02dZ", int(tm.tm_year%100), int(tm.tm_mon+1), tm.tm_mday, tm.tm_hour, tm.tm_min);
	}

	return out;
}

static
inline
std::string&
_makeASN1Generalized(std::string& out, int type, time_t sec, int msec)
{
	// Type 0: Localtime. YYYYMMDDHHMMSS.fff
	// Type 1: UTC only. YYMMDDHHMMSS.fffZ
	// Type 2: Localtime with diff. YYYYMMDDHHMMSS.fff+-HHMM

	struct tm tm;

	switch(type)
	{
	case 1:
	{
		gmtime_r(&sec, &tm);
		if ( tm.tm_year >= 100 )
		{
			PWStr::format(out, "%04d%02d%02d%02d%02d%02d.%03dZ", int(tm.tm_year+1900), int(tm.tm_mon+1), tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, int(msec));
		}
		else
		{
			PWStr::format(out, "%02d%02d%02d%02d%02d%02d.%03dZ", tm.tm_year, int(tm.tm_mon+1), tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, int(msec));
		}

		break;
	}// Type 1
	case 2:
	{
		const char sign(timezone <= 0 ? '+' : '-');
		const int abs_timezone(timezone >= 0 ? timezone : -timezone);
		const int hour(abs_timezone/3600);
		const int min((abs_timezone%3600)/60);
		localtime_r(&sec, &tm);
		PWStr::format(out, "%04d%02d%02d%02d%02d%02d.%03d%c%02d%02d", int(tm.tm_year+1900), int(tm.tm_mon+1), tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, int(msec), sign, hour, min);
		break;
	}// Type 2
	case 0:
	default:
	{
		localtime_r(&sec, &tm);
		PWStr::format(out, "%04d%02d%02d%02d%02d%02d.%03d", int(tm.tm_year+1900), int(tm.tm_mon+1), tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, int(msec));
		break;
	}// Type 0
	}//switch(type)

	return out;
}

template<>
void
DateTemplate<intmax_t, 1>::getNow(void)
{
	m_time = ::time(nullptr);
}

template<>
void
DateTemplate<intmax_t, PW_USEC>::getNow(void)
{
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	m_time = (tv.tv_sec * PW_USEC) + tv.tv_usec;
}

template<>
DateTemplate<intmax_t, 1>&
DateTemplate<intmax_t, 1>::assign(const struct tm& now, bool utc)
{
	m_time = _makeTime(now) - ( utc ? timezone : 0 );
	return *this;
}

template<>
DateTemplate<intmax_t, PW_USEC>&
DateTemplate<intmax_t, PW_USEC>::assign(const struct tm& now, bool utc)
{
	m_time = (_makeTime(now) - ( utc ? timezone : 0 ) ) * PW_USEC;
	return *this;
}

template<>
struct tm&
DateTemplate<intmax_t, 1>::getTime(struct tm& now, bool utc) const
{
	const time_t ltm(m_time);
	if ( utc ) gmtime_r(&ltm, &now);
	else localtime_r(&ltm, &now);
	return now;
}

template<>
struct tm&
DateTemplate<intmax_t, PW_USEC>::getTime(struct tm& now, bool utc) const
{
	const time_t ltm(m_time / PW_USEC);
	if ( utc ) gmtime_r(&ltm, &now);
	else localtime_r(&ltm, &now);
	return now;
}

template<>
std::string&
DateTemplate<intmax_t, 1>::getASN1GeneralizedTime(std::string& out, int type) const
{
	return _makeASN1Generalized(out, type, m_time, 0);
}

template<>
std::string&
DateTemplate<intmax_t, PW_USEC>::getASN1GeneralizedTime(std::string& out, int type) const
{
	return _makeASN1Generalized(out, type, m_time/PW_USEC, (m_time%PW_USEC)/1000LL);
}

template<>
std::string&
DateTemplate<intmax_t, 1>::getASN1UTCTime(std::string& out, bool use_sec) const
{
	return _makeASN1UTC(out, use_sec, m_time);
}

template<>
std::string&
DateTemplate<intmax_t, PW_USEC>::getASN1UTCTime(std::string& out, bool use_sec) const
{
	return _makeASN1UTC(out, use_sec, m_time/PW_USEC);
}

template<>
DateTemplate<intmax_t, 1>&
DateTemplate<intmax_t, 1>::assignASN1(const std::string& in)
{
	time_t sec(0);
	int msec(0);

	m_time = _makeASN1ToTime(sec, msec, in) ? sec : -1;
	return *this;
}

template<>
DateTemplate<intmax_t, PW_USEC>&
DateTemplate<intmax_t, PW_USEC>::assignASN1(const std::string& in)
{
	time_t sec(0);
	int msec(0);

	m_time = _makeASN1ToTime(sec, msec, in) ? ((sec*PW_USEC)+(msec*1000LL)) : -1;
	return *this;
}

};//namespace pw

