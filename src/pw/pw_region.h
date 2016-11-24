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
 * \file pw_region.h
 * \brief Support country code of phone.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"
#include "./pw_key.h"
#include "./pw_string.h"

#ifndef __PW_REGION_H__
#define __PW_REGION_H__

namespace pw {

class Ini;

//! \brief Region 정보 관리.
//!	싱글톤으로 사용할 수 있다.
class Region
{
public:
	Region() = default;

//private:
//	static const Region s_inst;

public:
	inline static const Region& s_getInstance(void) { static Region inst; return inst; }
	static bool s_initialize(void);

public:
	typedef KeyTemplate<char, 2>		code_type;	//!< 국가 코드 타입
	typedef KeyTemplate<char, 5>		phone_type;	//!< 해외전화 코드 타입

	//! \brief Region 정보
	typedef struct region_type {
		const std::string full_name;
		const char* code;
		const char* phone;

		inline region_type(const std::string& _full_name, const char* _code, const char* _phone) : full_name(_full_name), code(_code), phone(_phone) {}

		inline std::ostream& dump(std::ostream& os) const
		{
			os << "full_name: " << full_name << ", code: " << code << ", phone-code: " << phone << std::endl;
			return os;
		}
	} region_type;

	typedef std::list<region_type>		region_cont;
	typedef region_cont::iterator		region_itr;
	typedef region_cont::const_iterator	region_citr;

	typedef std::list<const region_type*>	region_res_list;

public:
	const region_type* findByCode(const code_type& code) const;
	region_res_list& findByPhone(region_res_list& out, const phone_type& phone) const;

	void swap(Region& v);
	void clear(void);

	bool read(const Ini& ini, const std::string& sec);
	bool write(Ini& ini, const std::string& sec) const;

private:
	bool _insert(const std::string& full_name, const char* code, const char* phone);

private:
	typedef std::map<code_type, const region_type*, str_ci_less<code_type> >
		code_index_cont;
	typedef std::multimap<phone_type, const region_type*>
		phone_index_cont;

	typedef std::set<code_type>		code_cont;
	typedef std::set<phone_type>	phone_cont;

private:
	region_cont			m_region;
	code_cont			m_codes;
	phone_cont			m_phones;
	code_index_cont		m_code_index;
	phone_index_cont	m_phone_index;
};

extern std::ostream& operator << (std::ostream& os, const Region::region_res_list& ls);

}; //namespace pw

extern const pw::Region& PWRegion;

#endif//__PW_REGION_H__

