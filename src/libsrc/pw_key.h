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
 * \file pw_key.h
 * \brief Support static key string for perfomance.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"

#ifndef __PW_KEY_H__
#define __PW_KEY_H__

#include "./pw_tokenizer.h"

namespace pw {

//! \brief 키 인터페이스 템플릿.
// 실제 구현은 KeyTemplate 템플릿 클래스를 사용한다.
template<typename _Type>
class KeyInterfaceTemplate
{
public:
	typedef _Type		value_type;
	typedef _Type&		reference_type;
	typedef const _Type&	const_reference_type;
	typedef _Type*		pointer_type;
	typedef const _Type*	const_pointer_type;
	typedef std::char_traits<_Type>		trait_type;
	typedef std::basic_string<_Type>	std_string_type;
	typedef std_string_type&		std_string_reference_type;
	typedef const std_string_type&		const_std_string_reference_type;

public:
	inline KeyInterfaceTemplate() : m_size(0) {}
	inline KeyInterfaceTemplate(const_pointer_type v) : m_size(0) {}
	inline KeyInterfaceTemplate(const KeyInterfaceTemplate& v) : m_size(0) {}
	inline virtual ~KeyInterfaceTemplate() {}

public:
	virtual inline size_t size(void) const { return m_size; }
	virtual inline bool empty(void) const { return m_size == 0; }
	virtual inline const_pointer_type c_str(void) const { return buf(); }
	virtual inline std::ostream& dump(std::ostream& os) const;
	virtual inline pointer_type begin(void) { return buf(); }
	virtual inline const_pointer_type begin(void) const { return buf(); }
	virtual inline pointer_type end(void) { return buf()+m_size; }
	virtual inline const_pointer_type end(void) const { return buf()+m_size; }
	virtual inline int compare(const KeyInterfaceTemplate& v) const;
	virtual inline int compare(const_pointer_type v) const;
	virtual inline reference_type at(size_t idx) { return *(buf()+idx); }
	virtual inline const_reference_type at(size_t idx) const { return *(buf()+idx); }
	virtual size_t assign(const_pointer_type v) = 0;
	virtual size_t assign(const_pointer_type v, size_t len) = 0;
	virtual size_t assign(const_std_string_reference_type v) = 0;
	virtual ssize_t format(const char* fmt, ...) __attribute__((format(printf,2,3))) = 0;
	virtual ssize_t formatV(const char* fmt, va_list ap) = 0;
	virtual size_t truncate(size_t idx) { *(buf()+idx) = 0x00; return (m_size = idx); }

	virtual size_t capacity(void) const = 0;
	virtual void clear(void) = 0;

public:
	virtual inline KeyInterfaceTemplate& operator = (const_pointer_type v) { this->assign(v); return *this; }
	virtual inline KeyInterfaceTemplate& operator = (const_std_string_reference_type v) { this->assign(v); return *this; }

	virtual inline bool operator < (const KeyInterfaceTemplate& v) const { return compare(v) < 0; }
	virtual inline bool operator <= (const KeyInterfaceTemplate& v) const { return compare(v) <= 0; }
	virtual inline bool operator > (const KeyInterfaceTemplate& v) const { return compare(v) > 0; }
	virtual inline bool operator >= (const KeyInterfaceTemplate& v) const { return compare(v) >= 0; }
	virtual inline bool operator == (const KeyInterfaceTemplate& v) const { return compare(v) == 0; }
	virtual inline bool operator != (const KeyInterfaceTemplate& v) const { return compare(v) != 0; }

	virtual inline bool operator < (const_pointer_type v) const { return compare(v) < 0; }
	virtual inline bool operator <= (const_pointer_type v) const { return compare(v) <= 0; }
	virtual inline bool operator > (const_pointer_type v) const { return compare(v) > 0; }
	virtual inline bool operator >= (const_pointer_type v) const { return compare(v) >= 0; }
	virtual inline bool operator == (const_pointer_type v) const { return compare(v) == 0; }
	virtual inline bool operator != (const_pointer_type v) const { return compare(v) != 0; }

	virtual inline reference_type operator[] (size_t idx) { return at(idx); }
	virtual inline const_reference_type operator[] (size_t idx) const { return at(idx); }

	virtual inline operator pointer_type(void) { return buf(); }
	virtual inline operator const_pointer_type(void) const { return buf(); }

	virtual inline size_t hash(void) const { size_t sum(0); for ( size_t i(0), e(size()); i < e; i++ ) sum += size_t(at(i)); return sum; }

	virtual std::ostream& operator << (std::ostream& os) const { os.write(buf(), m_size); return os; }

protected:
	virtual pointer_type buf(void) = 0;
	virtual const_pointer_type buf(void) const = 0;
	virtual inline void setEnd(void) = 0;

protected:
	size_t	m_size;
};

template<typename _Type>
inline
std::ostream&
operator << (std::ostream& os, const KeyInterfaceTemplate<_Type>& o)
{
	return os.write(o.c_str(), (o.size() * sizeof(_Type)));
}

//! \brief 고정길이 키 템플릿
template<typename _Type, size_t _Size>
class KeyTemplate : public KeyInterfaceTemplate<_Type>
{
public:
	enum
	{
		MAX_SIZE = _Size
	};

public:
	inline KeyTemplate() { m_buf[0] = 0x00; }
	inline KeyTemplate(typename KeyInterfaceTemplate<_Type>::const_pointer_type v);
	inline KeyTemplate(typename KeyInterfaceTemplate<_Type>::const_pointer_type v, size_t len);
	inline KeyTemplate(const KeyTemplate<_Type, _Size>& v);
	inline KeyTemplate(const KeyInterfaceTemplate<_Type>& v);
	inline virtual ~KeyTemplate() {}

public:
	inline void clear(void) override { m_buf[0] = 0x00; KeyInterfaceTemplate<_Type>::m_size = 0; }
	inline size_t capacity(void) const override { return MAX_SIZE; }
	inline size_t assign(typename KeyInterfaceTemplate<_Type>::const_pointer_type v) override;
	inline size_t assign(typename KeyInterfaceTemplate<_Type>::const_pointer_type v, size_t len) override;
	inline size_t assign(typename KeyInterfaceTemplate<_Type>::const_std_string_reference_type v) override;
	inline size_t assign(const KeyTemplate<_Type, _Size>& v);
	inline bool assign(TokenizerTemplate<_Type>& v, typename KeyInterfaceTemplate<_Type>::value_type d);
	inline ssize_t format(const char* fmt, ...) override __attribute__((format(printf,2,3)));
	inline ssize_t formatV(const char* fmt, va_list ap) override;

protected:
	inline typename KeyInterfaceTemplate<_Type>::pointer_type buf(void) override { return m_buf; }
	inline typename KeyInterfaceTemplate<_Type>::const_pointer_type buf(void) const override { return (typename KeyInterfaceTemplate<_Type>::const_pointer_type) m_buf; }
	inline void setEnd(void) override { m_buf[MAX_SIZE] = 0x00; }

protected:
	typename KeyInterfaceTemplate<_Type>::value_type m_buf[MAX_SIZE+1];
};

//! \brief 4바이트 char 타입 키.
//!	int32_t형으로 변환 가능하여 switch 문 등으로 분기 가능하다.
class KeyCode : public KeyTemplate<char, PW_CODE_SIZE>
{
public:
	typedef KeyTemplate<char, PW_CODE_SIZE>		parent_type;

public:
	inline virtual ~KeyCode() {}
	inline KeyCode() : parent_type() {}
	inline KeyCode(const char* v) : parent_type(v) {}
	inline KeyCode(const char* v, size_t len) : parent_type(v, len) {}
	inline KeyCode(const parent_type& p) : parent_type(p) {}

public:
	int32_t toInt32 (void) const;
	operator int32_t (void) const;
	KeyCode& operator = (int32_t i);
};

#define PWKeyCodeToInt32(x) ((pw::KeyCode::toint32_type*)(x))->i

}; // namespace pw

#include "./pw_key_tpl.h"

#endif//__PW_KEY_H__
