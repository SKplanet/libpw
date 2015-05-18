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
 * \file pw_date.h
 * \brief Support time/date string and ASN.1 type.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"

#ifndef __PW_DATE_H__
#define __PW_DATE_H__

struct tm;

namespace pw
{

//! \brief 시간을 처리한다.
template<typename _Type, intmax_t _Res>
class DateTemplate
{
public:
	_Type	m_time;

public:
	inline static std::string& s_getASN1GeneralizedTime(std::string& out, _Type now, int type = 0) { DateTemplate<_Type, _Res> inst(now); return inst.getASN1GeneralizedTime(out, type); }
	inline static std::string s_getASN1GeneralizedTime(_Type now, int type = 0) { std::string tmp; return s_getASN1GeneralizedTime(tmp, now, type); }
	inline static std::string& s_getASN1UTCTime(std::string& out, _Type now, bool use_sec = true) { DateTemplate<_Type, _Res> inst(now); return inst.getASN1UTCTime(out, use_sec); }
	inline static std::string s_getASN1UTCTime(_Type now, bool use_sec = true) { std::string tmp; return s_getASN1UTCTime(tmp, now, use_sec); }

public:
	explicit inline DateTemplate() : m_time(0) {}
	explicit inline DateTemplate(time_t now) : m_time(now * _Res) {}
	//explicit inline DateTemplate(_Type now) : m_time(now) {}
	inline DateTemplate(const DateTemplate& now) : m_time(now.m_time) {}
	inline DateTemplate(const struct tm& now, bool utc = false) { this->assign(now, utc); }

public:
	//! \brief 시간값을 초기화한다.
	inline void clear(void) { m_time = 0; }

	//! \brief 두 객체를 교체한다.
	inline void swap(DateTemplate& v) { std::swap(m_time, v.m_time); }

	//! \brief 시간값을 출력한다.
	inline std::ostream& write(std::ostream& os) const { return os << m_time; }

	//! \brief 현재 시간을 얻어온다.
	//! \warning 각 인스턴스별로 구현해야한다.
	void getNow(void);

	//! \brief 브로큰다운 구조체로부터 시간값을 설정한다.
	//! \param[in] now 브로큰다운 구조체
	//! \param[in] utc UTC 여부
	//! \warning 각 인스턴스별로 구현해야한다.
	DateTemplate& assign(const struct tm& now, bool utc = false);

	//! \brief 시간값을 설정한다.
	inline DateTemplate& assign(time_t v) { m_time = v * _Res; return *this; }

	//! \brief 시간값을 설정한다.
	inline DateTemplate& assign(const DateTemplate& v) { m_time = v.m_time; return *this; }

	//! \brief ASN.1 문자열로 시간값을 설정한다.
	//! \warning 각 인스턴스별로 구현해야한다.
	DateTemplate& assignASN1(const std::string& asn1_str);

	//! \brief UTC를 반환한다. 단위: 초
	inline time_t getUTC(void) const { return time_t(m_time / _Res); }

	//! \brief 브로큰다운 구조체로 반환한다.
	//! \param[out] now 출력할 브로큰다운 구조체
	//! \param[in] utc UTC 여부
	//! \warning 각 인스턴스별로 구현해야한다.
	struct tm& getTime(struct tm& now, bool utc = false) const;

	//! \brief 현재 시간값을 반환한다.
	inline _Type& getTime(void) { return m_time; }

	//! \brief 현재 시간값을 반환한다.
	inline const _Type& getTime(void) const { return m_time; }

	//! \brief ASN.1 타입 문자열로 출력한다.
	//! \warning 각 인스턴스별로 구현해야한다.
	std::string& getASN1GeneralizedTime(std::string& out, int type = 0) const;
	inline std::string getASN1GeneralizedTime(int type = 0) const { std::string tmp; return this->getASN1GeneralizedTime(tmp, type); }
	std::string& getASN1UTCTime(std::string& out, bool use_sec = true) const;
	inline std::string getASN1UTCTime(bool use_sec = true) const { std::string tmp; return this->getASN1UTCTime(tmp, use_sec); }

public:
	inline DateTemplate operator + (time_t v) { return DateTemplate(m_time + ( v * _Res )); }
	inline DateTemplate operator + (const DateTemplate& v) { return DateTemplate(m_time + v.m_time); }
	inline DateTemplate operator - (time_t v) { return DateTemplate(m_time - ( v * _Res )); }
	inline DateTemplate operator - (const DateTemplate& v) { return DateTemplate(m_time - v.m_time); }

	inline DateTemplate& operator += (time_t v) { m_time += ( v * _Res ); return *this; }
	inline DateTemplate& operator += (const DateTemplate& v) { m_time += v.m_time; return *this; }
	inline DateTemplate& operator -= (time_t v) { m_time -= ( v * _Res ); return *this; }
	inline DateTemplate& operator -= (const DateTemplate& v) { m_time -= v.m_time; return *this; }

	inline DateTemplate& operator = (const DateTemplate& v) { m_time = v.m_time; return *this; }
	inline DateTemplate& operator = (time_t v) { m_time = ( v * _Res ); return *this; }
	inline DateTemplate& operator = (const struct tm& now) { this->assign(now); return *this; }

	inline bool operator == (const DateTemplate& v) const { return m_time == v.m_time; }
	inline bool operator != (const DateTemplate& v) const { return m_time != v.m_time; }
	inline bool operator < (const DateTemplate& v) const { return m_time < v.m_time; }
	inline bool operator <= (const DateTemplate& v) const { return m_time <= v.m_time; }
	inline bool operator > (const DateTemplate& v) const { return m_time > v.m_time; }
	inline bool operator >= (const DateTemplate& v) const { return m_time >= v.m_time; }
};

template<typename _Type, intmax_t _Res>
inline std::ostream& operator << (std::ostream& os, const DateTemplate<_Type, _Res>& v) { return v.write(os); }

//!< 초단위 시간관리
using DateSecond = DateTemplate<intmax_t, intmax_t(1)>;

//!< 마이크로초단위 시간관리
using DateMicro = DateTemplate<intmax_t, intmax_t(1000000LL)>;

}; //namespace pw

#endif//__PW_DATE_H__

