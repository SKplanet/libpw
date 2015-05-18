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
 * \file pw_simplechpool.h
 * \brief Simple channel pool.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"
#include "./pw_channel_if.h"

#ifndef __PW_SIMPLECHPOOL_H__
#define __PW_SIMPLECHPOOL_H__

namespace pw
{

//! \brief 간단한 채널 풀 템플릿
template<typename _ChType>
class SimpleChPoolTemplate final
{
public:
	using ch_type = _ChType;	//!< 채널 타입
	using ch_ptr_type = ch_type*;	//!< 채널 포인터 타입
	using ch_ref_type = ch_type&;	//!< 채널 레퍼런스 타입
	using pool_type = std::set<intptr_t>;	//!< 내부 풀 타입

public:
	inline explicit SimpleChPoolTemplate();
	inline explicit SimpleChPoolTemplate(SimpleChPoolTemplate&& pool);
	inline explicit SimpleChPoolTemplate(const SimpleChPoolTemplate&) = delete;
	inline ~SimpleChPoolTemplate();

	inline SimpleChPoolTemplate& operator = (const SimpleChPoolTemplate&) = delete;
	inline SimpleChPoolTemplate& operator = (SimpleChPoolTemplate&& pool);

public:
	//! \brief 풀을 초기화한다. (필수요건 아님)
	template<typename _ParamType>
	inline bool initialize(size_t count, _ParamType& param);

	template<typename _FactoryType>
	inline bool initializeWithFactory(size_t count, _FactoryType& fac);

	//! \brief 채널을 하나 꺼내온다.
	template<typename _OutputType = ch_ptr_type>
	inline _OutputType getChannel(void);

	//! \brief 채널을 추가한다.
	inline bool add(ch_ptr_type pch);

	//! \brief 채널을 제거한다.
	inline bool remove(ch_ptr_type pch);

	//! \brief 풀 개수를 구한다.
	inline size_t size(void) const { return this->m_pool.size(); }

	//! \brief 풀이 비워져 있는지 확인한다.
	inline bool empty(void) const { return this->m_pool.empty(); }

	//! \brief 시작 반복자
	inline pool_type::iterator begin(void) { return m_pool.begin(); }
	inline pool_type::const_iterator begin(void) const { return m_pool.begin(); }
	inline pool_type::const_iterator cbegin(void) const { return m_pool.cbegin(); }

	//! \brief 끝 반복자
	inline pool_type::iterator end(void) { return m_pool.end(); }
	inline pool_type::const_iterator end(void) const { return m_pool.end(); }
	inline pool_type::const_iterator cend(void) const { return m_pool.cend(); }

private:
	pool_type m_pool;
	intptr_t m_index = 0;
};

//! \brief 기본 채널 풀
using SimpleChPool = SimpleChPoolTemplate<ChannelInterface>;

//namespace pw
}

#include "./pw_simplechpool_tpl.h"

#endif//__PW_SIMPLECHPOOL_H__
