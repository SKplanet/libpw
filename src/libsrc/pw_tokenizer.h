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
 * \file pw_tokenizer.h
 * \brief Tokenizer.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"

#ifndef __PW_TOKENIZER_H__
#define __PW_TOKENIZER_H__

namespace pw {

//! \brief 토크나이저 템플릿
template<typename _Type>
class TokenizerTemplate final
{
public:
	//! \brief 변수 타입. 보통 char
	using value_type = _Type;
	//! \brief 상수 타입.
	using const_value_type = const _Type;
	//! \brief 레퍼런스 타입.
	using reference_type = _Type&;
	//! \brief 레퍼런스 상수 타입.
	using const_reference_type = const _Type&;
	//! \brief 포인터 타입. 보통 char*
	using pointer_type = _Type*;
	//! \brief 포인터 상수 타입.
	using const_pointer_type = const _Type*;
	//! \brief 문자를 다루는 유틸리티.
	using trait_type = std::char_traits<_Type>;
	//! \brief 스트링 타입
	using string_type = std::basic_string<_Type>;
	//! \brief 스트링 레퍼런스 타입
	using string_reference_type = std::basic_string<_Type>&;
	//! \brief 스트링 레퍼런스 상수 타입.
	using const_string_reference_type = const std::basic_string<_Type>&;

public:
	//! \brief "\r\n"을 구분자로 한 줄을 읽는다.
	//! \param[out] v 출력 버퍼.
	//! \param[in] len 출력 버퍼 크기.
	//! \param[out] outlen 최종 길이.
	//! \return 성공하면 true를 반환한다.
	bool getLineA(pointer_type v, size_t len, size_t* outlen = nullptr);

	//! \brief "\r\n"을 구분자로 한 줄을 읽으며, 출력문자열은 반드시 널로
	//끝난다.
	//! \param[out] v 출력 버퍼.
	//! \param[in] len 출력 버퍼 크기.
	//! \param[out] outlen 최종 길이.
	//! \return 성공하면 true를 반환한다.
	bool getLine(pointer_type v, size_t len, size_t* outlen = nullptr);

	//! \brief "\r\n"을 구분자로 한 줄을 읽는다.
	//! \param[out] v 출력 버퍼.
	//! \param[in] len 출력 버퍼 크기.
	//! \return 성공하면 true를 반환한다.
	bool getLine(string_reference_type v, size_t len = size_t(-1));

	//! \brief "\r\n"을 구분자로 한 줄을 파악한다.
	//! \param[out] outptr 줄 시작 포인터 레퍼런스.
	//! \param[out] outlen 최종 길이.
	//! \return 성공하면 true를 반환한다.
	bool getLineZ(pointer_type& outptr, size_t& outlen);

	//! \brief 토큰을 읽는다.
	//! \warning 출력은 널문자로 끝나지 않는다.
	//! \param[out] v 출력 버퍼.
	//! \param[in] len 출력 버퍼 크기.
	//! \param[in] d 구분자.
	//! \param[out] outlen 최종 길이.
	//! \return 성공하면 true를 반환한다. strict 모드가 off 이면 언제나 true를 반환한다.
	bool getNextA(pointer_type v, size_t len, value_type d = value_type(' '), size_t* outlen = nullptr);

	//! \brief 토큰을 읽는다.
	//!	동일 구분자가 중복되면 건너뛴다.
	//! \warning 출력은 널문자로 끝나지 않는다.
	//! \param[out] v 출력 버퍼.
	//! \param[in] len 출력 버퍼 크기.
	//! \param[in] d 구분자.
	//! \param[out] outlen 최종 길이.
	//! \return 성공하면 true를 반환한다. strict 모드가 off 이면 언제나 true를 반환한다.
	bool getNextA2(pointer_type v, size_t len, value_type d = value_type(' '), size_t* outlen = nullptr);

	//! \brief 나머지를 읽는다.
	//! \warning 출력은 널문자로 끝나지 않는다.
	//! \param[out] v 출력 버퍼.
	//! \param[in] len 출력 버퍼 크기.
	//! \param[out] outlen 최종 길이.
	//! \return 성공하면 true를 반환한다. strict 모드가 off 이면 언제나 true를 반환한다.
	bool getLeftA(pointer_type v, size_t len, size_t* outlen = nullptr);

	//! \brief 토큰을 읽으며, 출력은 널문자로 끝나는 C-style 문자열이다.
	//! \param[out] v 출력 버퍼.
	//! \param[in] len 출력 버퍼 크기.
	//! \param[in] d 구분자.
	//! \param[out] outlen 최종 길이.
	//! \return 성공하면 true를 반환한다. strict 모드가 off 이면 언제나 true를 반환한다.
	bool getNext(pointer_type v, size_t len, value_type d = value_type(' '), size_t* outlen = nullptr);

	//! \brief 토큰을 읽으며, 출력은 널문자로 끝나는 C-style 문자열이다.
	//!	동일 구분자가 중복되면 건너뛴다.
	//! \param[out] v 출력 버퍼.
	//! \param[in] len 출력 버퍼 크기.
	//! \param[in] d 구분자.
	//! \param[out] outlen 최종 길이.
	//! \return 성공하면 true를 반환한다. strict 모드가 off 이면 언제나 true를 반환한다.
	bool getNext2(pointer_type v, size_t len, value_type d = value_type(' '), size_t* outlen = nullptr);

	//! \brief 나머지를 읽으며, 출력은 널문자로 끝나는 C-style 문자열이다.
	//! \warning 출력은 널문자로 끝나지 않는다.
	//! \param[out] v 출력 버퍼.
	//! \param[in] len 출력 버퍼 크기.
	//! \param[out] outlen 최종 길이.
	//! \return 성공하면 true를 반환한다. strict 모드가 off 이면 언제나 true를 반환한다.
	bool getLeft(pointer_type v, size_t len, size_t* outlen = nullptr);

	//! \brief 토큰을 읽는다.
	//! \param[out] v 출력 버퍼.
	//! \param[in] d 구분자.
	//! \return 성공하면 true를 반환한다. strict 모드가 off 이면 언제나 true를 반환한다.
	bool getNext(string_reference_type v, value_type d = value_type(' '));

	//! \brief 토큰을 읽는다.
	//!	동일 구분자가 중복되면 건너뛴다.
	//! \param[out] v 출력 버퍼.
	//! \param[in] d 구분자.
	//! \return 성공하면 true를 반환한다. strict 모드가 off 이면 언제나 true를 반환한다.
	bool getNext2(string_reference_type v, value_type d = value_type(' '));
	//
	//! \brief 나머지를 읽는다.
	//! \param[out] v 출력 버퍼.
	//! \return 성공하면 true를 반환한다. strict 모드가 off 이면 언제나 true를 반환한다.
	bool getLeft(string_reference_type v);

	//! \brief 한 글자를 읽는다.
	//! \param[out] v 출력.
	//! \param[in] d 구분자.
	//! \return 성공하면 true를 반환한다. strict 모드가 off 이면 언제나 true를 반환한다.
	bool getNextUnit(reference_type v, value_type d = value_type(' '));

	//! \brief 한 글자를 읽는다.
	//!	동일 구분자가 중복되면 건너뛴다.
	//! \param[out] v 출력.
	//! \param[in] d 구분자.
	//! \return 성공하면 true를 반환한다. strict 모드가 off 이면 언제나 true를 반환한다.
	bool getNextUnit2(reference_type v, value_type d = value_type(' '));

	//! \brief 나머지를 읽는다.
	//! \param[out] outptr 시작 포인터 레퍼런스.
	//! \param[out] outlen 최종 길이.
	//! \return 성공하면 true를 반환한다. strict 모드가 off 이면 언제나 true를 반환한다.
	bool getLeftZ(pointer_type& outptr, size_t& outlen);

	//! \brief 토큰을 읽는다.
	//! \param[out] outptr 시작 포인터 레퍼런스.
	//! \param[out] outlen 최종 길이.
	//! \param[in] d 구분자.
	//! \return 성공하면 true를 반환한다. strict 모드가 off 이면 언제나 true를 반환한다.
	bool getNextZ(pointer_type& outptr, size_t& outlen, value_type d = value_type(' '));

	//! \brief 토큰을 읽는다.
	//!	동일 구분자가 중복되면 건너뛴다.
	//! \param[out] outptr 시작 포인터 레퍼런스.
	//! \param[out] outlen 최종 길이.
	//! \param[in] d 구분자.
	//! \return 성공하면 true를 반환한다. strict 모드가 off 이면 언제나 true를 반환한다.
	bool getNextZ2(pointer_type& outptr, size_t& outlen, value_type d = value_type(' '));

	//!	\brief 다음 구분자까지 토큰을 건너뛴다.
	//! \param[in] d 구분자.
	//! \return 성공하면 true를 반환한다. strict 모드가 off 이면 언제나 true를 반환한다.
	bool skip(value_type d);
	//bool skip2(value_type d);

public:
	//! \brief 기본 생성자
	explicit inline TokenizerTemplate();

	//! \brief C-style 문자열로 초기화한다.
	//! \warning 널문자로 끝나지 않으면, 문제가 반드시 발생한다.
	//! \param[in] v C-style 문자열
	explicit inline TokenizerTemplate(const_pointer_type v);

	//! \brief 메모리 버퍼로 초기화한다.
	//! \param[in] v 메모리 버퍼.
	//! \param[in] len 메모리 버퍼 크기.
	explicit inline TokenizerTemplate(const_pointer_type v, size_t len);

	//! \brief C++ 스트링 객체로 초기화한다.
	//! \param[in] v 스트링 객체.
	explicit inline TokenizerTemplate(const_string_reference_type v);

	//! \brief 소멸자.
	inline ~TokenizerTemplate() = default;

public:
	//! \brief 버퍼를 설정한다.
	//!	토크나이저를 재사용할 때 사용한다.
	//! \param[in] v C-style 문자열
	inline void setBuffer(const_pointer_type v);

	//! \brief 버퍼를 설정한다.
	//!	토크나이저를 재사용할 때 사용한다.
	//! \param[in] v 메모리 버퍼.
	//! \param[in] len 메모리 버퍼 크기.
	inline void setBuffer(const_pointer_type v, size_t len);

	//! \brief 버퍼를 설정한다.
	//!	토크나이저를 재사용할 때 사용한다.
	//! \param[in] v 스트링 객체.
	inline void setBuffer(const_string_reference_type v);

	//! \brief 현재위치를 설정한다.
	//! \warning 범위를 검사하지 않으므로 사용을 자제한다.
	inline void setPosition(const_pointer_type v) { m_pos = v; }

	//! \brief 현재위치를 얻어온다.
	//! \return 현재위치 포인터.
	inline const_pointer_type getPosition(void) const { return m_pos; }

	//! \brief 현재위치를 재설정한다.
	inline void resetPosition(void) { m_pos = m_begin; }

	//! \brief 현재위치를 인덱스로 얻어온다.
	//! \return '0'부터 시작하는 현재위치 인덱스.
	inline size_t getIndex(void) const;

	//! \brief 현재위치가 시작점인지 확인한다.
	inline bool isBegin(void) const { return m_pos == m_begin; }

	//! \brief 현재위치가 끝점인지 확인한다.
	inline bool isEnd(void) const { return m_pos == m_end; }

	//! \brief 나머지 버퍼 크기를 계산한다.
	inline size_t getLeftSize(void) const { return size_t(m_end - m_pos); }

	//! \brief strict 모드를 얻어온다.
	inline bool getStrict(void) const { return m_strict; }

	//! \brief strict 모드를 설정한다.
	inline bool setStrict(bool s) { bool ret(m_strict); m_strict = s; return ret; }

public:
	//! \brief 객체 컨텐츠를 교환한다.
	void swap(TokenizerTemplate<_Type>& v);

	//! \brief 객체 버퍼크기를 구한다.
	inline size_t size(void) const { return m_size; }

	//! \brief 시작점을 반환한다.
	inline pointer_type begin(void) { return const_cast<pointer_type>(m_begin); }
	//! \brief 시작점을 반환한다.
	inline const_pointer_type begin(void) const { return m_begin; }

	//! \brief 시작점을 반환한다.
	inline const_pointer_type cbegin(void) const { return m_begin; }

	//! \brief 끝점을 반환한다.
	inline const_pointer_type end(void) const { return m_end; }

	//! \brief 끝점을 반환한다.
	inline const_pointer_type cend(void) const { return m_end; }

private:
	size_t				m_size;		//!< 버퍼 크기
	const_pointer_type	m_begin;	//!< 시작점
	const_pointer_type	m_end;		//!< 끝점
	const_pointer_type	m_pos;		//!< 현재 위치
	bool				m_strict;	//!< strict 모드

private:
	inline const_pointer_type _find(value_type d) const;
	inline const_pointer_type _find(value_type d, size_t len) const;
	inline const_pointer_type _find_not(value_type d) const;
	inline const_pointer_type _find_not(value_type d, size_t len) const;
	inline const_pointer_type _findLine(void) const;
	inline const_pointer_type _findLine(size_t len) const;
};

//! \brief 기본 토크나이저.
using Tokenizer = TokenizerTemplate<char>;

}; //namespace pw

//! \brief C-style string으로 토큰을 얻어오며, 실패하면 로그를 출력하고 break한다.
#define PWTOKCSTR(tok,buf,blen,d) \
	if ( not (tok).getNext((buf), (blen), (d)) ) { PWLOGLIB("PWTOKCSTR failed: " #buf "=%.128s", ((tok).getPosition())); break; }

//! \brief STL string으로 토큰을 얻어오며, 실패하면 로그를 출력하고 break한다.
#define PWTOKSTR(tok,buf,d) \
	if ( not (tok).getNext((buf), (d)) ) { PWLOGLIB("PWTOKSTR failed: " #buf "=%.128s", ((tok).getPosition())); break; }

//! \brief Key로 토큰을 얻어오며, 실패하면 로그를 출력하고 break한다.
#define PWTOKKEY(tok,key,d) \
	if ( not (key).assign((tok), (d)) ) { PWLOGLIB("PWTOKKEY failed: " #key "=%.128s", ((tok).getPosition())); break; }

#endif//__PW_TOKENIZER_H__

#include "./pw_tokenizer_tpl.h"

