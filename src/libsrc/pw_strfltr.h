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
 * \file pw_strfltr.h
 * \brief Support simple string filter.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"
#include "./pw_key.h"

#ifndef __PW_STRFLTR_H__
#define __PW_STRFLTR_H__

namespace pw
{

namespace strfltr
{
enum class Pattern : int
{
	NONE,	//!< 오류
	SUBSTRING,	//!< 대소문자 가리는 서브스트링
	SUBSTRING_I,	//!< 대소문자 가리지 않는 서브스트링
	REGEX,	//!< 정규식 (ECMA | nosubs)
	REGEX_I,	//!< 대소문자 가리지 않는 정규식 (ECMA | nosubs | icase)
	HASH,	//!< SHA256
};

extern const char* toStringA(Pattern ptrn);
extern std::string toString(Pattern ptrn);
extern Pattern toPattern(const std::string& str);
extern Pattern toPattern(const char* s);

};

//! \brief 문자열 필터
class StringFilter final
{
public:
	using Pattern = strfltr::Pattern;
	using check_res_type = std::pair<Pattern, const std::string>;

public:
	inline explicit StringFilter() = default;
	inline explicit StringFilter(const char* filepath) { this->readFromFile(filepath); }
	inline explicit StringFilter(std::istream& is) { this->readFromStream(is); }
	explicit StringFilter(const StringFilter&) = delete;
	explicit StringFilter(StringFilter&&) = delete;
	~StringFilter();

	StringFilter& operator = (const StringFilter&) = delete;
	StringFilter& operator = (StringFilter&&) = delete;

public:
	//! \brief 스왑.
	inline void swap(StringFilter& obj) { m_cont.swap(obj.m_cont); m_hash.swap(obj.m_hash); }

	inline bool empty(void) const { return m_cont.empty() and m_hash.empty(); }
	inline size_t size(void) const { return m_cont.size() + m_hash.size(); }

	//! \brief 파일로부터 읽기
	bool readFromFile(const char* file);

	//! \brief 스트림으로부터 읽기
	bool readFromStream(std::istream& is);

	//! \brief 파일에 쓰기
	bool writeToFile(const char* file) const;

	//! \brief 스트림에 쓰기
	bool writeToStream(std::ostream& os) const;

public:
	//! \brief 문자열 검사
	//! \return 필터를 모두 통과할 경우, Pattern::NONE을 반환한다.
	Pattern check(const std::string& str) const;

	check_res_type check2(const std::string& str) const;

	//! \brief 패턴을 추가한다.
	bool add(Pattern ptrn, const std::string& needle);

public:
	struct context_type
	{
		const std::string before_compiled;
		virtual bool isMatched(const std::string& key) = 0;
		virtual bool compile(const std::string& comp) = 0;
		virtual Pattern getPatternType(void) const = 0;
		inline virtual ~context_type() = default;

		inline bool operator == (const context_type& ctx) const { return (getPatternType() == ctx.getPatternType()) and (before_compiled == ctx.before_compiled); }
		inline bool operator not_eq (const context_type& ctx) const { return (getPatternType() not_eq ctx.getPatternType()) or (before_compiled not_eq ctx.before_compiled); }
		inline bool operator < (const context_type& ctx) const
		{
			auto pt1(getPatternType()), pt2(ctx.getPatternType());
			if ( pt1 == pt2 ) return before_compiled < ctx.before_compiled;
			else if ( pt1 < pt2 ) return true;
			return false;
		}

		inline bool operator <= (const context_type& ctx) const
		{
			auto pt1(getPatternType()), pt2(ctx.getPatternType());
			if ( pt1 == pt2 ) return before_compiled <= ctx.before_compiled;
			else if ( pt1 < pt2 ) return true;
			return false;
		}
	};

private:
	std::list<context_type*> m_cont;
	std::set<std::string> m_hash;
};

}

#endif//__PW_STRFLTR_H__
