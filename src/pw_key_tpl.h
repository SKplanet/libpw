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
 * \file pw_key_tpl.h
 * \brief Support static key string for perfomance.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#ifndef __PW_KEY_TPL_H__
#define __PW_KEY_TPL_H__

// buffer overflow all tested by Yubin Lim

#ifndef __PW_KEY_H__
#	error "DO NOT USE THIS FILE DIRECTLY"
#endif//!__PW_KEY_H__

namespace pw {

template<typename _Type>
std::ostream&
KeyInterfaceTemplate<_Type>::dump(std::ostream& os) const
{
	os << "type: " << typeid(*this).name() << std::endl;
	os << "length: " << m_size << std::endl;
	os << "buffer address: " << (void*)buf() << std::endl;
	os << "buffer content: "; os.write(buf(), m_size*sizeof(_Type)) << std::endl;
	return os;
};

template<typename _Type>
int
KeyInterfaceTemplate<_Type>::compare(const KeyInterfaceTemplate& v) const
{
	if ( m_size not_eq v.m_size )
	{
		size_t len(std::min(m_size, v.m_size));
		int res( trait_type::compare(buf(), v.buf(), len) );

		if ( res not_eq 0 ) return res;
		if ( m_size > v.m_size ) return 1;
		return -1;
	}

	return trait_type::compare(buf(), v.buf(), m_size);
}

template<typename _Type>
int
KeyInterfaceTemplate<_Type>::compare(const_pointer_type v) const
{
	size_t vlen(trait_type::length(v));

	if ( m_size not_eq vlen )
	{
		size_t len(std::min(m_size, vlen));
		int res( trait_type::compare(buf(), v, len) );

		if ( res not_eq 0 ) return res;
		if ( m_size > vlen ) return 1;
		return -1;
	}

	return trait_type::compare(buf(), v, m_size);
}

template<typename _Type, size_t _Size>
KeyTemplate<_Type, _Size>::KeyTemplate(typename KeyInterfaceTemplate<_Type>::const_pointer_type v)
{
	if ( v == &m_buf[0] ) return;

	const size_t cplen(std::min(size_t(MAX_SIZE), KeyInterfaceTemplate<_Type>::trait_type::length(v)));

	KeyInterfaceTemplate<_Type>::trait_type::copy(m_buf, v, cplen);

	m_buf[cplen] = 0x00;
	KeyInterfaceTemplate<_Type>::m_size = cplen;
}

template<typename _Type, size_t _Size>
KeyTemplate<_Type, _Size>::KeyTemplate(typename KeyInterfaceTemplate<_Type>::const_pointer_type v, size_t len)
{
	const size_t cplen(std::min(size_t(MAX_SIZE), len));

	if ( v not_eq &m_buf[0] ) KeyInterfaceTemplate<_Type>::trait_type::copy(m_buf, v, cplen);

	m_buf[cplen] = 0x00;
	KeyInterfaceTemplate<_Type>::m_size = cplen;
}

template<typename _Type, size_t _Size>
KeyTemplate<_Type, _Size>::KeyTemplate(const KeyTemplate<_Type, _Size>& v)
{
	const size_t cplen(v.size());
	std::char_traits<_Type>::copy(m_buf, v, cplen);
	m_buf[cplen] = 0x00;
	KeyInterfaceTemplate<_Type>::m_size = cplen;
}

template<typename _Type, size_t _Size>
KeyTemplate<_Type, _Size>::KeyTemplate(const KeyInterfaceTemplate<_Type>& v)
{
	const size_t cplen(std::min(size_t(MAX_SIZE), v.size()));
	std::char_traits<_Type>::copy(m_buf, v, cplen);
	m_buf[cplen] = 0x00;
	KeyInterfaceTemplate<_Type>::m_size = cplen;
}

template<typename _Type, size_t _Size>
size_t
KeyTemplate<_Type, _Size>::assign(typename KeyInterfaceTemplate<_Type>::const_pointer_type v)
{
	if ( v == &m_buf[0] ) return KeyInterfaceTemplate<_Type>::m_size;

	const size_t cplen(std::min(size_t(MAX_SIZE), KeyInterfaceTemplate<_Type>::trait_type::length(v)));

	KeyInterfaceTemplate<_Type>::trait_type::copy(m_buf, v, cplen);

	m_buf[cplen] = 0x00;
	KeyInterfaceTemplate<_Type>::m_size = cplen;

	return KeyInterfaceTemplate<_Type>::m_size;
}

template<typename _Type, size_t _Size>
size_t
KeyTemplate<_Type, _Size>::assign(typename KeyInterfaceTemplate<_Type>::const_pointer_type v, size_t len)
{
	const size_t cplen(std::min(size_t(MAX_SIZE), len));

	if ( v not_eq &m_buf[0] ) KeyInterfaceTemplate<_Type>::trait_type::copy(m_buf, v, cplen);

	m_buf[cplen] = 0x00;
	KeyInterfaceTemplate<_Type>::m_size = cplen;

	return KeyInterfaceTemplate<_Type>::m_size;
}

template<typename _Type, size_t _Size>
size_t
KeyTemplate<_Type, _Size>::assign(typename KeyInterfaceTemplate<_Type>::const_std_string_reference_type v)
{
	const size_t cplen(std::min(size_t(MAX_SIZE), v.size()));

	KeyInterfaceTemplate<_Type>::trait_type::copy(m_buf, v.c_str(), cplen);

	m_buf[cplen] = 0x00;
	KeyInterfaceTemplate<_Type>::m_size = cplen;

	return KeyInterfaceTemplate<_Type>::m_size;
}

template<typename _Type, size_t _Size>
size_t
KeyTemplate<_Type, _Size>::assign(const KeyTemplate<_Type, _Size>& v)
{
	if ( this == &v ) return KeyInterfaceTemplate<_Type>::m_size;

	const size_t cplen(std::min(size_t(MAX_SIZE), v.size()));

	KeyInterfaceTemplate<_Type>::trait_type::copy(m_buf, v.m_buf, cplen);

	m_buf[cplen] = 0x00;
	KeyInterfaceTemplate<_Type>::m_size = cplen;

	return KeyInterfaceTemplate<_Type>::m_size;
}

template<typename _Type, size_t _Size>
bool
KeyTemplate<_Type, _Size>::assign(TokenizerTemplate<_Type>& v, typename KeyInterfaceTemplate<_Type>::value_type d)
{
	size_t cplen;
	if ( not v.getNext(m_buf, MAX_SIZE+1, d, &cplen) )
	{
		m_buf[0] = 0x00;
		KeyInterfaceTemplate<_Type>::m_size = 0;
		return false;
	}

	m_buf[cplen] = 0x00;
	KeyInterfaceTemplate<_Type>::m_size = cplen;
	return true;
}

template<typename _Type, size_t _Size>
ssize_t
KeyTemplate<_Type, _Size>::format(const char* fmt, ...)
{
	va_list lst;
	va_start(lst, fmt);
	ssize_t ret( vsnprintf(m_buf, MAX_SIZE+1, fmt, lst) );
	va_end(lst);

	if ( ret <= MAX_SIZE ) KeyInterfaceTemplate<_Type>::m_size = ret;
	else
	{
		m_buf[0] = 0x00;
		KeyInterfaceTemplate<_Type>::m_size = 0;
	}

	return ret;
}

template<typename _Type, size_t _Size>
ssize_t
KeyTemplate<_Type, _Size>::formatV(const char* fmt, va_list lst)
{
	ssize_t ret( vsnprintf(m_buf, MAX_SIZE+1, fmt, lst) );

	if ( ret <= MAX_SIZE ) KeyInterfaceTemplate<_Type>::m_size = ret;
	else
	{
		m_buf[0] = 0x00;
		KeyInterfaceTemplate<_Type>::m_size = 0;
	}

	return ret;
}
}; // namespace pw

#endif//!__PW_KEY_TPL_H__

