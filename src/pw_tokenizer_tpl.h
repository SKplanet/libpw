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
 * \file pw_tokenizer_tpl.h
 * \brief Tokenizer.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#ifndef __PW_TOKENIZER_TPL_H__
#define __PW_TOKENIZER_TPL_H__

#ifndef __PW_TOKENIZER_H__
#	error "DO NOT USE THIS FILE DIRECTLY"
#endif//!__PW_TOKENIZER_H__

namespace pw {

template<typename _Type>
TokenizerTemplate<_Type>::TokenizerTemplate() : m_size(0), m_begin(nullptr), m_end(nullptr), m_pos(nullptr), m_strict(false)
{
}

template<typename _Type>
TokenizerTemplate<_Type>::TokenizerTemplate(const_pointer_type v) : m_size(trait_type::length(v)), m_begin(v), m_end(v+m_size), m_pos(v), m_strict(false)
{
	//PWTRACE("begin:%p end:%p pos:%p", m_begin, m_end, m_pos);
}

template<typename _Type>
TokenizerTemplate<_Type>::TokenizerTemplate(const_pointer_type v, size_t len) : m_size(len), m_begin(v), m_end(v+len), m_pos(v), m_strict(false)
{
}

template<typename _Type>
TokenizerTemplate<_Type>::TokenizerTemplate(const_string_reference_type v) : m_size(v.size()), m_begin(v.c_str()), m_end(m_begin+m_size), m_pos(m_begin), m_strict(false)
{
}

template<typename _Type>
typename TokenizerTemplate<_Type>::const_pointer_type
TokenizerTemplate<_Type>::_find(value_type d) const
{
	const_pointer_type ib(m_pos), ie(m_end);
	while ( ib != ie )
	{
		if ( d == *ib ) return ib;
		++ib;
	}

	return ib;
}

template<typename _Type>
typename TokenizerTemplate<_Type>::const_pointer_type
TokenizerTemplate<_Type>::_find(value_type d, size_t len) const
{
	const_pointer_type ib(m_pos), ie(m_end);
	while ( ib != ie )
	{
		if ( d == *ib )
		{
			if ( ib == m_pos ) return ib;
			if ( size_t(ib - m_pos - 1) > len )
			{
				//PWTRACE("OVERFLOW: %zu", size_t(ib-m_pos-1));
				return ie;
			}

			return ib;
		}

		++ib;
	}

	return ib;
}

template<typename _Type>
typename TokenizerTemplate<_Type>::const_pointer_type
TokenizerTemplate<_Type>::_find_not(value_type d) const
{
	const_pointer_type ib(m_pos), ie(m_end);
	while ( ib != ie )
	{
		if ( d != *ib ) return ib;
		++ib;
	}

	return ib;
}

template<typename _Type>
typename TokenizerTemplate<_Type>::const_pointer_type
TokenizerTemplate<_Type>::_find_not(value_type d, size_t len) const
{
	const_pointer_type ib(m_pos), ie(m_end);
	while ( ib != ie )
	{
		if ( d != *ib )
		{
			if ( ib == m_pos ) return ib;
			if ( size_t(ib - m_pos - 1) > len )
			{
				return ie;
			}

			return ib;
		}
		++ib;
	}

	return ib;
}

template<typename _Type>
typename TokenizerTemplate<_Type>::const_pointer_type
TokenizerTemplate<_Type>::_findLine(void) const
{
	const_pointer_type ib(m_pos), ie(m_end);
	//PWTRACE("ib: %p ie: %p", ib, ie);
	while ( ib != ie )
	{
		//PWTRACE("NOW: %c", char(*ib));
		if ( value_type('\r') == *ib )
		{
			const_pointer_type ibnext(ib+1);
			if ( ibnext == ie ) return ie;
			if ( value_type('\n') == *ibnext ) return ib;
		}

		++ib;
	}

	return ie;
}

template<typename _Type>
typename TokenizerTemplate<_Type>::const_pointer_type
TokenizerTemplate<_Type>::_findLine(size_t len) const
{
	const_pointer_type ib(m_pos), ie(m_end);
	//PWTRACE("ib: %p ie: %p", ib, ie);
	while ( ib != ie )
	{
		//PWTRACE("NOW: 0x%02x ib: %p ie: %p ", int(*ib), ib, ie);
		//if ( ib > ie ) PWABORT("FUCK!");
		if ( value_type('\r') == *ib )
		{
			const_pointer_type ibnext(ib+1);
			if ( ibnext == ie )
			{
				//PWTRACE("NO LINEFEED");
				return ie;
			}

			if ( value_type('\n') == *ibnext )
			{
				return ib;
			}
		}

		++ib;
		//PWTRACE("cplen: %zu/%zu", size_t(ib-m_pos-1), len);
		if ( size_t(ib-m_pos-1) > len )
		{
			//PWTRACE("BUFFER OVERFLOW %zu:%zu", size_t(ib-m_pos-1), len);
			return ie;
		}
	}

	return ie;
}

template<typename _Type>
void
TokenizerTemplate<_Type>::setBuffer(const_pointer_type v)
{
	m_size = trait_type::length(v);
	m_begin = v;
	m_end = m_begin + m_size;
	m_pos = m_begin;
}

template<typename _Type>
void
TokenizerTemplate<_Type>::setBuffer(const_pointer_type v, size_t len)
{
	m_size = len;
	m_begin = v;
	m_end = m_begin + m_size;
	m_pos = m_begin;
}

template<typename _Type>
void
TokenizerTemplate<_Type>::setBuffer(const_string_reference_type v)
{
	m_size = v.size();
	m_begin = v.c_str();
	m_end = m_begin + m_size;
	m_pos = m_begin;
}

template<typename _Type>
size_t
TokenizerTemplate<_Type>::getIndex(void) const
{
	return size_t(m_pos - m_begin);
}

template<typename _Type>
bool
TokenizerTemplate<_Type>::getLineA(pointer_type v, size_t len, size_t* outlen)
{
	if ( m_pos == m_end ) return false;
	//PWSHOWMETHOD();
	const_pointer_type ib(_findLine(len));
	if ( ib == m_end )
	{
		if ( m_strict ) return false;

		size_t cplen(ib - m_pos);
		if ( cplen > len ) return false;

		memcpy(v, m_pos, cplen*sizeof(value_type));
		if ( cplen < len ) v[cplen] = 0x00;
		if ( outlen ) *outlen = cplen;

		m_pos = m_end;

		return false;
	}

	size_t cplen(ib - m_pos);
	if ( cplen > 0 )
	{
		memcpy(v, m_pos, cplen*sizeof(value_type));
		if ( len != cplen ) v[cplen] = 0x00;
	}
	else
	{
		v[0] = 0x00;
	}

	if ( outlen ) *outlen = cplen;
	m_pos = ib + 2;

	return true;
}

template<typename _Type>
bool
TokenizerTemplate<_Type>::getLineZ(pointer_type& outptr, size_t& outlen)
{
	if ( m_pos == m_end ) return false;
	//PWSHOWMETHOD();
	const_pointer_type ib(_findLine());
	if ( ib == m_end )
	{
		if ( m_strict ) return false;

		outptr = m_pos;
		outlen = size_t(ib-m_pos);

		m_pos = m_end;

		return false;
	}

	outptr = m_pos;
	outlen = size_t(ib-m_pos);

	m_pos = ib + 2;

	return true;
}
template<typename _Type>
bool
TokenizerTemplate<_Type>::getLine(pointer_type v, size_t len, size_t* outlen)
{
	if ( m_pos == m_end ) return false;
	//PWSHOWMETHOD();
	--len;
	const_pointer_type ib(_findLine(len));
	if ( ib == m_end )
	{
		if ( m_strict ) return false;

		size_t cplen(ib - m_pos);
		if ( cplen > len ) return false;

		//PWTRACE("cplen:%zu", cplen);
		memcpy(v, m_pos, cplen*sizeof(value_type));
		v[cplen] = 0x00;
		if ( outlen ) *outlen = cplen;

		m_pos = m_end;

		return false;
	}

	size_t cplen(ib - m_pos);
	if ( cplen > 0 )
	{
		//PWTRACE("cplen:%zu", cplen);
		memcpy(v, m_pos, cplen*sizeof(value_type));
		v[cplen] = 0x00;
	}
	else
	{
		v[0] = 0x00;
	}

	if ( outlen ) *outlen = cplen;
	m_pos = ( ib != m_end ) ? ( ib + 2 ) : m_end;

	return true;
}

template<typename _Type>
bool
TokenizerTemplate<_Type>::getLine(string_reference_type v, size_t len)
{
	if ( m_pos == m_end ) return false;
	//PWSHOWMETHOD();
	const_pointer_type ib(size_t(-1) == len ? _findLine() : _findLine(len));
	if ( ib == m_end )
	{
		if ( m_strict ) return false;

		size_t cplen(ib - m_pos);
		if ( cplen > len ) return false;

		string_type tmp(m_pos, cplen);
		v.swap(tmp);

		m_pos = m_end;

		return false;
	}

	size_t cplen(ib - m_pos);
	if ( cplen > 0 )
	{
		string_type tmp(m_pos, cplen);
		v.swap(tmp);
	}
	else
	{
		v.clear();
	}

	m_pos = ib + 2;

	return true;
}

template<typename _Type>
bool
TokenizerTemplate<_Type>::getNextA(pointer_type v, size_t len, value_type d, size_t* outlen)
{
	if ( m_pos == m_end ) return false;
	const_pointer_type ib(_find(d, len));
	if ( m_end == ib )
	{
		if ( m_strict ) return false;

		size_t cplen(ib-m_pos);
		if ( cplen > len ) return false;

		memcpy(v, m_pos, cplen*sizeof(value_type));
		if ( cplen < len ) v[cplen] = 0x00;
		if ( outlen ) *outlen = cplen;

		m_pos = m_end;

		return true;
	}

	size_t cplen(ib-m_pos);
	if ( cplen > 0 )
	{
		memcpy(v, m_pos, cplen*sizeof(value_type));
		if ( cplen < len ) v[cplen] = 0x00;
	}
	else
	{
		v[0] = 0x00;
	}

	if ( outlen ) *outlen = cplen;
	m_pos = ib+1;

	return true;
}

template<typename _Type>
bool
TokenizerTemplate<_Type>::getNextZ(pointer_type& outptr, size_t& outlen, value_type d)
{
	if ( m_pos == m_end ) return false;
	const_pointer_type ib(_find(d));
	if ( m_end == ib )
	{
		if ( m_strict ) return false;

		const size_t cplen(ib-m_pos);
		outptr = m_pos;
		outlen = cplen;

		m_pos = m_end;

		return true;
	}

	const size_t cplen(ib-m_pos);
	outptr = m_pos;
	outlen = cplen;

	m_pos = ib+1;

	return true;
}

template<typename _Type>
bool
TokenizerTemplate<_Type>::getNext(pointer_type v, size_t len, value_type d, size_t* outlen)
{
	if ( m_pos == m_end ) return false;
	--len;

	const_pointer_type ib(_find(d, len));
	if ( m_end == ib )
	{
		if ( m_strict ) return false;

		size_t cplen(ib-m_pos);
		if ( cplen > len ) return false;

		memcpy(v, m_pos, cplen*sizeof(value_type));
		v[cplen] = 0x00;
		if ( outlen ) *outlen = cplen;

		m_pos = m_end;

		return true;
	}

	size_t cplen(ib-m_pos);
	if ( cplen > 0 )
	{
		memcpy(v, m_pos, cplen*sizeof(value_type));
		v[cplen] = 0x00;
	}
	else
	{
		v[0] = 0x00;
	}

	if ( outlen ) *outlen = cplen;
	m_pos = ib+1;

	return true;
}

template<typename _Type>
bool
TokenizerTemplate<_Type>::getNextA2(pointer_type v, size_t len, value_type d, size_t* outlen)
{
	if ( m_pos == m_end ) return false;
	const_pointer_type tmp_pos(_find_not(d));
	if ( m_end == tmp_pos )
	{
		if ( m_strict ) return false;

		v[0] = 0x00;
		if ( outlen ) *outlen = 0;
		m_pos = m_end;

		return true;
	}

	size_t cplen(0);
	if ( m_pos == tmp_pos )
	{
		const_pointer_type ib(_find(d, len));
		if ( m_end == ib )
		{
			if ( m_strict ) return false;
			if ( (cplen = size_t(ib-tmp_pos)) > len ) return false;
			memcpy(v, tmp_pos, cplen*sizeof(value_type));
			m_pos = m_end;
			if ( outlen ) *outlen = cplen;
			return true;
		}

		cplen = size_t(ib-tmp_pos);
		memcpy(v, tmp_pos, cplen*sizeof(value_type));
		m_pos = ib+1;
	}
	else
	{
		const_pointer_type old_pos(m_pos);
		m_pos = tmp_pos;
		const_pointer_type ib(_find(d, len));
		if ( m_end == ib )
		{
			if ( m_strict )
			{
				m_pos = old_pos;
				return false;
			}

			if ( (cplen = size_t(ib-tmp_pos)) > len )
			{
				m_pos = old_pos;
				return false;
			}

			memcpy(v, tmp_pos, cplen*sizeof(value_type));
			if ( cplen < len ) v[cplen] = 0x00;
			m_pos = m_end;
			if ( outlen ) *outlen = cplen;
			return true;
		}

		cplen = size_t(ib-tmp_pos);
		memcpy(v, tmp_pos, cplen*sizeof(value_type));
		if ( cplen < len ) v[cplen] = 0x00;
		m_pos = ib+1;
		//PWTRACE("pos != end");
	}

	if ( outlen ) *outlen = cplen;
	return true;
}

template<typename _Type>
bool
TokenizerTemplate<_Type>::getNextZ2(pointer_type& outptr, size_t& outlen, value_type d)
{
	if ( m_pos == m_end ) return false;
	const_pointer_type tmp_pos(_find_not(d));
	if ( m_end == tmp_pos )
	{
		if ( m_strict ) return false;

		outptr = m_end;
		outlen = 0;
		m_pos = m_end;

		return true;
	}

	if ( m_pos == tmp_pos )
	{
		const_pointer_type ib(_find(d));
		if ( m_end == ib )
		{
			if ( m_strict ) return false;

			outptr = tmp_pos;
			outlen = size_t(m_end - tmp_pos);
			m_pos = m_end;

			return true;
		}

		outlen = size_t(ib-tmp_pos);
		outptr = tmp_pos;
		m_pos = ib+1;
	}
	else
	{
		const_pointer_type old_pos(m_pos);
		m_pos = tmp_pos;
		const_pointer_type ib(_find(d));
		if ( m_end == ib )
		{
			if ( m_strict )
			{
				m_pos = old_pos;
				return false;
			}

			outlen = size_t(ib-tmp_pos);
			outptr = tmp_pos;
			m_pos = m_end;
			return true;
		}

		outlen = size_t(ib-tmp_pos);
		m_pos = ib+1;
	}

	return true;
}

template<typename _Type>
bool
TokenizerTemplate<_Type>::getNext2(pointer_type v, size_t len, value_type d, size_t* outlen)
{
	if ( m_pos == m_end ) return false;
	--len;

	const_pointer_type tmp_pos(_find_not(d));
	if ( m_end == tmp_pos )
	{
		if ( m_strict ) return false;

		m_pos = m_end;
		v[0] = 0x00;
		if ( outlen ) *outlen = 0;

		return true;
	}

	size_t cplen(0);
	if ( m_pos == tmp_pos )
	{
		const_pointer_type ib(_find(d, len));
		if ( m_end == ib )
		{
			if ( m_strict ) return false;

			if ( (cplen = size_t(ib-tmp_pos)) > len ) return false;

			memcpy(v, tmp_pos, cplen*sizeof(value_type));
			if ( outlen ) *outlen = cplen;
			m_pos = m_end;
			return true;
		}

		cplen = size_t(ib-tmp_pos);
		memcpy(v, tmp_pos, cplen*sizeof(value_type));
		m_pos = ib+1;
	}
	else
	{
		const_pointer_type old_pos(m_pos);
		m_pos = tmp_pos;
		const_pointer_type ib(_find(d, len));
		if ( m_end == ib )
		{
			if ( m_strict )
			{
				m_pos = old_pos;
				return false;
			}

			if ( (cplen = size_t(ib-tmp_pos)) > len )
			{
				m_pos = old_pos;
				return false;
			}

			memcpy(v, tmp_pos, cplen*sizeof(value_type));
			if ( cplen < len ) v[cplen] = 0x00;
			m_pos = m_end;
			if ( outlen ) *outlen = cplen;
			return true;
		}

		cplen = size_t(ib-tmp_pos);
		memcpy(v, tmp_pos, cplen*sizeof(value_type));
		v[cplen] = 0x00;
		m_pos = ib+1;
	}

	if ( outlen ) *outlen = cplen;
	return true;
}

template<typename _Type>
bool
TokenizerTemplate<_Type>::getNext(string_reference_type v, value_type d)
{
	if ( m_pos == m_end )
	{
		//PWTRACE("end of position");
		return false;
	}

	const_pointer_type ib(_find(d));
	if ( m_end == ib )
	{
		if ( m_strict )
		{
			//PWTRACE("strict!");
			return false;
		}

		size_t cplen(ib-m_pos);
		string_type tmp(m_pos, cplen);
		v.swap(tmp);

		m_pos = m_end;

		return true;
	}

	size_t cplen(ib-m_pos);
	if ( cplen > 0 )
	{
		string_type tmp(m_pos, cplen);
		v.swap(tmp);
	}
	else
	{
		string_type tmp;
		v.swap(tmp);
	}

	m_pos = ib+1;

	return true;
}

template<typename _Type>
bool
TokenizerTemplate<_Type>::getNext2(string_reference_type v, value_type d)
{
	if ( m_pos == m_end ) return false;
	const_pointer_type tmp_pos(_find_not(d));
	if ( m_end == tmp_pos )
	{
		if ( m_strict ) return false;

		string_type tmp;
		v.swap(tmp);
		m_pos = m_end;

		return true;
	}

	size_t cplen(0);
	const_pointer_type old_pos(m_pos);

	m_pos = tmp_pos;
	const_pointer_type ib(_find(d));
	if ( m_end == ib )
	{
		if ( m_strict )
		{
			m_pos = old_pos;
			return false;
		}

		cplen = size_t(ib - m_pos);
		string_type tmp(m_pos, cplen);
		v.swap(tmp);
		m_pos = m_end;

		return true;
	}

	cplen = size_t(ib - m_pos);
	if ( cplen > 0 )
	{
		string_type tmp(m_pos, cplen);
		v.swap(tmp);
	}
	else
	{
		v.clear();
	}

	m_pos = ib+1;

	return true;
}

template<typename _Type>
bool
TokenizerTemplate<_Type>::getNextUnit(reference_type v, value_type d)
{
	return this->getNext(&v, 1, d);
}

template<typename _Type>
bool
TokenizerTemplate<_Type>::getNextUnit2(reference_type v, value_type d)
{
	return this->getNext2(&v, 1, d);
}

template<typename _Type>
bool
TokenizerTemplate<_Type>::skip(value_type d)
{
	if ( isEnd() ) return false;

	const_pointer_type ib(_find(d));
	if ( ib == m_end ) return false;

	m_pos = ib+1;
	return true;
}

template<typename _Type>
bool
TokenizerTemplate<_Type>::getLeftA(pointer_type v, size_t len, size_t* outlen)
{
	const size_t left(getLeftSize());
	if ( (0 == left) and m_strict ) return false;
	if ( left > len ) return false;
	::memcpy(v, m_pos, sizeof(_Type)*left);
	if ( outlen ) *outlen = left;
	m_pos = m_end;
	return true;
}

template<typename _Type>
bool
TokenizerTemplate<_Type>::getLeftZ(pointer_type& outptr, size_t& outlen)
{
	const size_t left(getLeftSize());
	if ( (0 == left) and m_strict ) return false;
	outptr = m_pos;
	outlen = left;
	m_pos = m_end;
	return true;
}

template<typename _Type>
bool
TokenizerTemplate<_Type>::getLeft(pointer_type v, size_t len, size_t* outlen)
{
	const size_t left(getLeftSize());
	if ( (0 == left) and m_strict ) return false;
	if ( left+1 > len ) return false;
	::memcpy(v, m_pos, sizeof(_Type)*left);
	v[left] = 0x00;
	if ( outlen ) *outlen = left;
	m_pos = m_end;
	return true;
}

template<typename _Type>
bool
TokenizerTemplate<_Type>::getLeft(string_reference_type v)
{
	const size_t left(getLeftSize());
	if ( (0 == left) and m_strict ) return false;
	v.assign(m_pos, left);
	m_pos = m_end;
	return true;
}

}; //namespace pw

#endif//!__PW_TOKENIZER_TPL_H__
