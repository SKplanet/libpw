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
 * \file pw_module.cpp
 * \brief Support module(plugin-like).
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include <dlfcn.h>
#include "pw_module.h"
#include "pw_log.h"

namespace pw
{

static std::mutex g_modules_lock;
static std::set<void*>	g_modules;

inline
static
bool
__insert_handle ( void* hdl )
{
	std::unique_lock<std::mutex> l(g_modules_lock);
	return g_modules.insert ( hdl ).second;
}

inline
static
void
__remove_handle ( void* hdl )
{
	std::unique_lock<std::mutex> l(g_modules_lock);
	g_modules.erase ( hdl );
}

bool
Module::load ( const char* path, void* in_param )
{
	close();

	void* hdl ( nullptr );
	std::string comment;

	do
	{
		if ( nullptr == ( hdl = dlopen ( path, RTLD_NOW | RTLD_GLOBAL ) ) )
		{
			comment = dlerror();
			break;
		}

		if ( not __insert_handle ( hdl ) )
		{
			hdl = nullptr;
			comment = "already loaded";
			break;
		}

		func_load_ptr	f_load ( reinterpret_cast<func_load_ptr> ( dlsym ( hdl, "pw_module_load" ) ) );

		if ( nullptr == f_load )
		{
			comment = "no pw_module_load symbol";
			break;
		}

		load_param_type load_param;
		load_param.in.param = in_param;
		load_param.in.self = this;

		if ( not f_load ( load_param ) )
		{
			comment = load_param.out.errstr.empty() ? std::string ( "pw_module_load return false" ) : load_param.out.errstr;
			break;
		}

		m_handle = hdl;
		m_name = load_param.out.name;
		m_version = load_param.out.version;

		m_func_load = f_load;
		m_func_unload = reinterpret_cast<func_unload_ptr> ( dlsym ( hdl, "pw_module_unload" ) );
		m_func_new = reinterpret_cast<func_new_ptr> ( dlsym ( hdl, "pw_module_new" ) );
		m_func_delete = reinterpret_cast<func_delete_ptr> ( dlsym ( hdl, "pw_module_delete" ) );

		return true;
	}
	while ( false );

	PWLOGLIB ( "failed to load module: path:%s in_param:%p error:%s", path, in_param, comment.c_str() );

	if ( hdl )
	{
		dlclose ( hdl );
		__remove_handle ( hdl );
	}

	return false;
}

void
Module::close ( void )
{
	if ( m_func_unload ) m_func_unload ( this );

	if ( m_handle ) dlclose ( m_handle );

	__remove_handle ( m_handle );

	m_handle = nullptr;
	m_name.clear();
	m_version.clear();
	m_func_load = nullptr;
	m_func_unload = nullptr;
	m_func_new = nullptr;
	m_func_delete = nullptr;
}

void*
Module::getAddress ( const char* name )
{
	if ( m_handle ) return dlsym ( m_handle, name );

	return nullptr;
}

void*
Module::newObject ( int* rcode, void* in_param )
{
	if ( m_func_new )
	{
		create_param_type param ( this, in_param );
		m_func_new ( param );

		if ( rcode ) *rcode = param.out.code;

		return param.out.obj;
	}

	if ( rcode ) *rcode = ENOSYS;

	return nullptr;
}

void
Module::swap ( Module& v )
{
	std::swap ( m_handle, v.m_handle );
	m_name.swap ( v.m_name );
	m_version.swap ( v.m_version );

	std::swap ( m_func_load, v.m_func_load );
	std::swap ( m_func_unload, v.m_func_unload );
	std::swap ( m_func_new, v.m_func_new );
	std::swap ( m_func_delete, v.m_func_delete );
}


};// namespace pw
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4;
