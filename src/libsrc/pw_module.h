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
 * \file pw_module.h
 * \brief Support module(plugin-like).
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"

#ifndef __PW_PLUGIN_H__
#define __PW_PLUGIN_H__

namespace pw
{

class Module final
{
public:
	//! \brief 플러그인 로드 파라매터
	struct load_param_type
	{
		struct _out
		{
			std::string name;		//!< 모듈 이름
			std::string version;	//!< 모듈 버전
			std::string errstr;		//!< 실패시 실패 사유. 모듈에서 작성해야함.
		} out;

		struct _in
		{
			Module*	self = nullptr;
			void* 		param = nullptr;	//!< 사용자 파라매터
		} in;
	};

	//! \brief 객체 생성 파라매터
	struct create_param_type
	{
		struct _out
		{
			void*	obj = nullptr;	//!< 생성한 객체
			int		code = 0;		//!< 반환코드
		} out;

		struct _in
		{
			Module*	self = nullptr;	//!< 모듈 객체
			void* 		param = nullptr;//!< 사용자 파라매터

			inline _in ( Module* in_self, void* in_param ) : self ( in_self ), param ( in_param ) {}
		} in;

		inline create_param_type ( Module* in_self, void* in_param ) : in ( in_self, in_param ) {}
	};

	using func_load_ptr = bool (* ) ( load_param_type& );	//!< 플러그인을 로드할 때 호출할 함수.
	using func_unload_ptr = void (* ) ( Module* );			//!< 플러그인을 프리할 때 호출할 함수.
	using func_new_ptr = void (* ) ( create_param_type& );	//!< 오브젝트를 생성 함수.
	using func_delete_ptr = void (* ) ( Module*, void* );	//!< 오브젝트 삭제 함수.

public:
	explicit inline Module() {}
	inline ~Module()
	{
		close();
	}

public:
	//! \brief 모듈 로드하기
	bool load ( const char* path, void* param = nullptr );

	//! \brief 모듈 닫기
	void close ( void );

	//! \brief 모듈로부터 새로운 객체 생성하기
	void* newObject ( int* rcode = nullptr, void* in_param = nullptr );

	//! \brief 모듈로부터 생성한 객체 삭제하기
	inline void deleteObject ( void* obj )
	{
		if ( m_func_delete ) m_func_delete ( this, obj );
	}

	//! \brief 모듈로부터 심볼 주소 얻어오기
	void* getAddress ( const char* name );

private:
	void swap ( Module& v );
	Module ( const Module& ) = delete;
	Module ( Module && ) = delete;
	Module& operator = ( const Module& ) = delete;
	Module& operator = ( const Module && ) = delete;

private:
	void* m_handle = nullptr;

	std::string m_name;
	std::string m_version;

	func_load_ptr m_func_load = nullptr;
	func_unload_ptr m_func_unload = nullptr;
	func_new_ptr m_func_new = nullptr;
	func_delete_ptr m_func_delete = nullptr;
};// class Module

}; // namespace pw

#endif//__PW_PLUGIN_H__
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4;
