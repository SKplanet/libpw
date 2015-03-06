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
 * \file pw_redispacket.cpp
 * \brief Packet for Redis(http://redis.io/).
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_redispacket.h"
#include "./pw_log.h"
#include "./pw_encode.h"
#include <sys/types.h>

#define RETURN_READER_ERROR(msg) do { PWLOGLIB("ERROR: %s", (msg)); this->clear(); return -1; } while (false)

namespace pw {
namespace redis {

inline
static
void*
_assignSimple(const char* s)
{
	if ( s )
	{
		auto size(strlen(s));
		if ( size )
		{
			std::for_each(s, s+size,
						[](char c) { if ( c == '\r' or c == '\n' ) Value::_throwNotAllowLine(); }
						);

			return new std::string(s, size);
		}
	}

	return new std::string();
}

inline
static
void*
_assignSimple(const char* s, size_t l)
{
	if ( s and l )
	{
		std::for_each(s, s+l,
					[](char c) { if ( c == '\r' or c == '\n' ) Value::_throwNotAllowLine(); }
					);

		return new std::string(s, l);
	}

	return new std::string();
}

inline
static
void*
_assignBulk(const char* s)
{
	if ( s )
	{
		return new std::string(s);
	}

	return nullptr;
}

inline
static
void*
_assignBulk(const char* s, size_t l)
{
	if ( s and l )
	{
		return new std::string(s, l);
	}

	return new std::string();
}

Value::Value ( ValueType t, size_t reserved_size ) : m_type(t)
{
	switch(m_type)
	{
	case ValueType::SIMPLE_STRING:
	case ValueType::ERROR:
	case ValueType::BULK_STRING:
		m_data = new std::string(reserved_size, 0x00);
		break;

	case ValueType::INTEGER:
		m_data = new int64_t(reserved_size);
		break;

	case ValueType::ARRAY:
		m_data = new array_type(reserved_size, Value(0));
		break;

	default:
		Value::_throwInvalidType();
	}//switch

	if ( m_data == nullptr ) Value::_throwNoMemory();
}

Value::Value(Value&& v) :m_type(v.m_type), m_data(v.m_data)
{
	v.m_data = nullptr;
}

Value::Value(std::initializer_list<Value> l) : m_type(ValueType::ARRAY), m_data(new array_type(l))
{
	//PWSHOWMETHOD();
}

Value::Value(ValueType v) : m_type(v)
{
	//PWSHOWMETHOD();
	switch(m_type)
	{
	case ValueType::SIMPLE_STRING:
	case ValueType::ERROR:
	case ValueType::BULK_STRING:
		m_data = new std::string();
		break;
	case ValueType::INTEGER:
		m_data = new int64_t(0);
		break;
	case ValueType::ARRAY:
		m_data = new array_type;
		break;
	default:
		Value::_throwInvalidType();
	}

	if ( m_data == nullptr ) Value::_throwNoMemory();
}

Value::Value(const Value& v) : m_type(v.m_type), m_data(nullptr)
{
	//PWSHOWMETHOD();
	if ( v.m_data == nullptr ) return;

	switch(m_type)
	{
	case ValueType::SIMPLE_STRING:
	case ValueType::ERROR:
	case ValueType::BULK_STRING:
		m_data = new std::string(v._asString());
		break;
	case ValueType::INTEGER:
		m_data = new int64_t(v._asInteger());
		break;
	case ValueType::ARRAY:
		m_data = new array_type(v._asArray());
		break;
	default:
		Value::_throwInvalidType();
	}

	if ( m_data == nullptr ) Value::_throwNoMemory();
}

Value::Value(const std::string& s, ValueType t)
{
	//PWSHOWMETHOD();
	switch(t)
	{
	case ValueType::SIMPLE_STRING:
	case ValueType::ERROR:
        for ( auto c : s ) if ( c == '\r' or c == '\n' ) Value::_throwNotAllowLine();
        m_data = new std::string(s);
        break;

	case ValueType::BULK_STRING:
		m_data = new std::string(s);
		break;

	case ValueType::ARRAY:
		m_data = new array_type;
		break;

	case ValueType::INTEGER:
	default:
		Value::_throwInvalidType();
	}

	if ( m_data == nullptr ) Value::_throwNoMemory();
	m_type = t;
}

Value::Value(const char* s, ValueType t)
{
	//PWSHOWMETHOD();
	switch(t)
	{
	case ValueType::SIMPLE_STRING:
	case ValueType::ERROR:
		m_data = _assignSimple(s);
		break;

	case ValueType::BULK_STRING:
		m_data = _assignBulk(s);
		break;

	case ValueType::ARRAY:
		if ( nullptr == s ) break;

	case ValueType::INTEGER:
	default:
		Value::_throwInvalidType();
	}

	m_type = t;
}

Value::Value(const char* s, size_t l, ValueType t)
{
	//PWSHOWMETHOD();
	switch(t)
	{
	case ValueType::SIMPLE_STRING:
	case ValueType::ERROR:
		m_data = _assignSimple(s, l);
		break;

	case ValueType::BULK_STRING:
		m_data = _assignBulk(s);
		break;

	case ValueType::ARRAY:
		if ( nullptr == s or 0 == l ) break;

	case ValueType::INTEGER:
	default:
		Value::_throwInvalidType();
	}

	m_type = t;
}

std::ostream&
Value::write(std::ostream& os) const
{
	os.write(reinterpret_cast<const char*>(&m_type), 1);
	switch(m_type)
	{
	case ValueType::BULK_STRING:
	{
		if ( not m_data ) { os.write("-1", 2); break; }
		auto& s(_asString());
		os << s.size();
		os.write("\r\n", 2);
		//PWEnc::encodeEscape(os, s.c_str(), s.size());
		os.write(s.c_str(), s.size());
		break;
	}

	case ValueType::SIMPLE_STRING:
	case ValueType::ERROR:
	{
		auto& s(_asString());
		os.write(s.c_str(), s.size());
		break;
	}

	case ValueType::INTEGER:
		os << _asInteger();
		break;

	case ValueType::ARRAY:
	{
		if ( not m_data ) { os.write("-1", 2); break; }
		auto& av(_asArray());
		os << av.size();
		os.write("\r\n", 2);
		for ( auto& v : _asArray() ) v.write(os);
		return os;
	}

	default:
		Value::_throwInvalidType();
	}//switch

	os.write("\r\n", 2);
	return os;
}

void
Value::_destroy(void)
{
	if ( m_data == nullptr ) return;

	switch(m_type)
	{
	case ValueType::SIMPLE_STRING:
	case ValueType::ERROR:
	case ValueType::BULK_STRING:
		//PWTRACE("String");
		delete reinterpret_cast<std::string*>(m_data);
		break;

	case ValueType::INTEGER:
		//PWTRACE("Integer");
		delete reinterpret_cast<int64_t*>(m_data);
		break;

	case ValueType::ARRAY:
		//PWTRACE("Array");
		delete reinterpret_cast<array_type*>(m_data);
		break;

	default:
		Value::_throwInvalidType();
	}
}

size_t
Value::size(void) const
{
	if ( m_data == nullptr ) return 0;

	switch(m_type)
	{
	case ValueType::SIMPLE_STRING:
	case ValueType::ERROR:
	case ValueType::BULK_STRING:
		return _asString().size();
	case ValueType::INTEGER:
		return 1;
	case ValueType::ARRAY:
		return _asArray().size();
	default:
		Value::_throwInvalidType();
	}

	return 0;
}

void
Value::clear(void)
{
	if ( m_data == nullptr ) return;

	switch(m_type)
	{
	case ValueType::SIMPLE_STRING:
	case ValueType::ERROR:
		_asString().clear();
		break;

	case ValueType::BULK_STRING:
		delete reinterpret_cast<std::string*>(m_data);
		m_data = nullptr;
		break;

	case ValueType::INTEGER:
		_asInteger() = 0;
		break;

	case ValueType::ARRAY:
		_asArray().clear();
		break;
	}
}

void
Value::append(const Value& v)
{
	if ( not isArray() ) Value::_throwNotArray();
	_asArray().push_back(v);
}

void
Value::append(Value&& v)
{
	if ( not isArray() ) Value::_throwNotArray();
	if ( this not_eq &v ) _asArray().push_back(v);
	else _asArray().push_back(std::move(v));
}

void
Value::append(int64_t v)
{
	if ( not isArray() ) Value::_throwNotArray();
	_asArray().push_back(Value(v));
}

void
Value::append(const std::string& v, ValueType t)
{
	if ( not isArray() ) Value::_throwNotArray();
	_asArray().push_back(Value(v, t));
}

void
Value::append(const char* s, ValueType t)
{
	if ( not isArray() ) Value::_throwNotArray();
	_asArray().push_back(Value(s, t));
}

void
Value::append(const char* s, size_t l, ValueType t)
{
	if ( not isArray() ) Value::_throwNotArray();
	_asArray().push_back(Value(s, l, t));
}

void
Value::appendNullBulk(void)
{
	if ( not isArray() ) Value::_throwNotArray();
	_asArray().push_back(Value(nullptr, ValueType::BULK_STRING));
}

void
Value::appendNullArray(void)
{
	if ( not isArray() ) Value::_throwNotArray();
	_asArray().push_back(Value(nullptr, ValueType::ARRAY));
}

void
Value::assign ( Value && v )
{
	if ( this == &v ) return;

	Value newobj(v);
	swap(newobj);
}

void
Value::assign ( const Value& v )
{
	if ( this == &v ) return;

	Value newobj(v);
	swap(newobj);
}

void
Value::resetAsArray(size_t reserved_size)
{
	_destroy();
	m_type = ValueType::ARRAY;
	m_data = new array_type(reserved_size, Value(0));
}

Value&
Value::operator [] (size_t idx)
{
	if ( not isArray() ) Value::_throwNotArray();
	return _asArray().at(idx);
}

const Value&
Value::operator [] (size_t idx) const
{
	if ( not isArray() ) Value::_throwNotArray();
	return _asArray().at(idx);
}

Value&
Value::operator = (const Value& v)
{
	if ( this == &v ) return *this;

	Value newobj(v);
	swap(newobj);
	return *this;
}

Value&
Value::operator = (Value&& v)
{
	if ( this == &v ) return *this;

	Value newobj(v);
	swap(newobj);
	return *this;
}

ssize_t
Reader::_rollback ( void )
{
	//PWSHOWMETHOD();
	auto& top(m_status.top());
	if ( top.index not_eq top.count ) return m_cont.size();

	if ( m_status.size() == 1 )
	{
		m_cont.push(std::move(top.node));
		m_status.pop();
		return m_cont.size();
	}

	Value tmp(top.node);
	m_status.pop();

	auto& top2(m_status.top());
	top2.node[top2.index].assign(std::move(tmp));
	++top2.index;

	if ( top2.index == top2.count )
	{
		return _rollback();
	}

	return m_cont.size();
}

ssize_t
Reader::_appendToTop ( Value && v )
{
	//PWSHOWMETHOD();
	if ( m_status.empty() )
	{
		m_cont.push(v);
		return m_cont.size();
	}

	auto& top(m_status.top());
	if ( not top.node.isArray() ) RETURN_READER_ERROR("last stack is not array");

	top.node[top.index].assign(v);
	++top.index;

	if ( top.index == top.count ) return _rollback();
	return m_cont.size();
}

ssize_t
Reader::_parseInteger(const std::string& line)
{
	//PWSHOWMETHOD();
	int64_t i(strtoll(line.c_str()+1, nullptr, 10));
	return _appendToTop(Value(i));
}

ssize_t
Reader::_parseSimpleString(const std::string& line)
{
	//PWSHOWMETHOD();
	auto buf(line.c_str()+1);
	auto blen(line.size()-1);

	return _appendToTop(Value(buf, blen, ValueType::SIMPLE_STRING));
}

ssize_t
Reader::_parseError(const std::string& line)
{
	//PWSHOWMETHOD();
	auto buf(line.c_str()+1);
	auto blen(line.size()-1);

	return _appendToTop(Value(buf, blen, ValueType::ERROR));
}

ssize_t
Reader::_parseBulkString(const std::string& line)
{
	//PWSHOWMETHOD();
	ssize_t bulklen(strtossize(line.c_str()+1, nullptr, 10));
	if ( bulklen > -1 )
	{
		m_status.push(status_type{0, size_t(bulklen), Value(nullptr, ValueType::BULK_STRING)});
		return m_cont.size();
	}

	return _appendToTop(Value(nullptr, ValueType::BULK_STRING));
}

ssize_t
Reader::_parseBulkString2 ( const string& line )
{
	//PWSHOWMETHOD();
	auto& top(m_status.top());
	if ( top.count not_eq line.size() ) RETURN_READER_ERROR("characters count is not same");

	m_status.pop();
	return _appendToTop(Value(line, ValueType::BULK_STRING));
}

ssize_t
Reader::_parseArray(const std::string& line)
{
	//PWSHOWMETHOD();
	ssize_t arlen(strtossize(line.c_str()+1, nullptr, 10));
	if ( arlen > -1 )
	{
		m_status.push(status_type{0, size_t(arlen), Value(ValueType::ARRAY, size_t(arlen))});
		return m_cont.size();
	}

	return _appendToTop(Value(nullptr, ValueType::ARRAY));
}

void
Reader::clear ( void )
{
	do { decltype(m_status) v; m_status.swap(v); } while (false);
	do { decltype(m_cont) v; m_cont.swap(v); } while (false);
	m_buf.clear();
}

ssize_t
Reader::parse ( IoBuffer& buf )
{
	IoBuffer::blob_type b;
	buf.grabRead(b);
	auto res(this->parse(b.buf, b.size));
	buf.moveRead(b.size);

	return res;
}

ssize_t
Reader::parse(const char* buf, size_t blen)
{
	if ( buf and blen ) m_buf.writeToBuffer(buf, blen);

	std::string line;
	do {
		if ( not m_status.empty() )
		{
			//PWTRACE("status is not empty");
			auto& top(m_status.top());
			if ( top.node.isBulkString() )
			{
				if ( m_buf.getReadableSize() < top.count+2 )
				{
					//PWTRACE("m_buf.getReadableSize() < top.count+2: %zu %zu", m_buf.getReadableSize(), top.count+2);
					return m_cont.size();
				}

				m_buf.readFromBuffer(line, top.count);
				m_buf.moveRead(2);
				if ( _parseBulkString2(line) < 0 ) return -1;

				continue;
			}
		}

		if ( m_buf.getLine(line) )
		{
			if ( _parse(line) < 0 )
			{
				//PWTRACE("error!");
				return -1;
			}
		}
		else
		{
			//PWTRACE("no line");
			break;
		}
	} while (true);

	return m_cont.size();
}

ssize_t
Reader::_parse ( const string& line )
{
	//PWTRACE("input line(%zu): <%s>", line.size(), line.c_str());
	if ( not m_status.empty() )
	{
		auto& top(m_status.top());
		if ( top.node.isBulkString() )
		{
			return _parseBulkString2(line);
		}
	}

	if ( line.size() < 2 ) RETURN_READER_ERROR("invalid line");

	ValueType c(static_cast<ValueType>(line[0]));
	switch(c)
	{
		case ValueType::INTEGER: return _parseInteger(line);
		case ValueType::SIMPLE_STRING: return _parseSimpleString(line);
		case ValueType::ERROR: return _parseError(line);
		case ValueType::BULK_STRING: return _parseBulkString(line);
		case ValueType::ARRAY: return _parseArray(line);
	}// switch

	RETURN_READER_ERROR("invalid identifier");
}


//namespace redis
}

//namespace pw
}
