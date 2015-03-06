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
 * \file pw_simplechpool_tpl.h
 * \brief Simple channel pool.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#ifndef __PW_SIMPLECHPOOL_TPL_H__
#define __PW_SIMPLECHPOOL_TPL_H__

#ifndef __PW_SIMPLECHPOOL_H__
#	error "DO NOT USE THIS FILE DIRECTLY"
#endif//__PW_SIMPLECHPOOL_H__

namespace pw
{

template<typename _ChType>
SimpleChPoolTemplate<_ChType>::SimpleChPoolTemplate() {}

template<typename _ChType>
SimpleChPoolTemplate<_ChType>::SimpleChPoolTemplate(SimpleChPoolTemplate&& pool) : m_pool(std::move(pool.m_pool)), m_index(pool.m_index)
{
	pool.m_pool.clear();
	pool.m_index = 0;
}

template<typename _ChType>
SimpleChPoolTemplate<_ChType>::~SimpleChPoolTemplate()
{
	/*
	for ( auto ich : m_pool )
	{
		auto pch(reinterpret_cast<ch_ptr_type>(ich));
		if ( pch ) delete pch;
	}
	*/
}

template<typename _ChType>
SimpleChPoolTemplate<_ChType>&
SimpleChPoolTemplate<_ChType>::operator = (SimpleChPoolTemplate&& pool)
{
	m_pool = pool.m_pool;
	m_index = pool.m_index;
	pool.m_pool.clear();
}

template<typename _ChType>
template<typename _ParamType>
bool
SimpleChPoolTemplate<_ChType>::initialize(size_t count, _ParamType& param)
{
	if ( m_pool.empty() )
	{
		for ( size_t i(0); i < count; i++ )
		{
			auto pch(new ch_type(param));
			if ( nullptr == pch ) return false;
			m_pool.insert(reinterpret_cast<intptr_t>(pch));
		}

		return true;
	}

	return false;
}

template<typename _ChType>
template<typename _FactoryType>
bool
SimpleChPoolTemplate<_ChType>::initializeWithFactory(size_t count, _FactoryType& fac)
{
	if ( m_pool.empty() )
	{
		for ( size_t i(0); i < count; i++ )
		{
			auto pch(fac());
			if ( nullptr == pch ) return false;
			m_pool.insert(reinterpret_cast<intptr_t>(pch));
		}

		return true;
	}

	return false;
}

template<typename _ChType>
template<typename _OutputType>
_OutputType
SimpleChPoolTemplate<_ChType>::getChannel ( void )
{
	if ( m_pool.empty() ) return nullptr;

	auto ib(m_pool.lower_bound(m_index));
	if ( ib == m_pool.end() )
	{
		ib = m_pool.begin();
	}

	auto ret(reinterpret_cast<_OutputType>(*ib));
	m_index = (*ib)+1;
	return ret;
}

template<typename _ChType>
bool
SimpleChPoolTemplate<_ChType>::add(ch_ptr_type pch)
{
	PWTRACE("add: %p", pch);
	return m_pool.insert(reinterpret_cast<intptr_t>(pch)).second;
}

template<typename _ChType>
bool
SimpleChPoolTemplate<_ChType>::remove(ch_ptr_type pch)
{
	auto ib(m_pool.find(reinterpret_cast<intptr_t>(pch)));
	if ( ib == m_pool.end() ) return false;
	m_pool.erase(ib);
	return true;
}

//namespace pw
}

#endif//__PW_SIMPLECHPOOL_TPL_H__
