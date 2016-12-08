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
 * \file pw_redispacket.h
 * \brief Packet for Redis(http://redis.io/).
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_packet_if.h"

#ifndef __PW_REDISPACKET_H__
#define __PW_REDISPACKET_H__

namespace pw {

namespace redis {

enum class ValueType : char
{
	SIMPLE_STRING = '+',
	ERROR = '-',
	INTEGER = ':',
	BULK_STRING = '$',
	ARRAY = '*'
};

class Value final
{
public:
	using array_type = std::vector<Value>;

public:
	~Value() { _destroy(); }

	Value() = delete;
	Value(ValueType v);
	Value(const Value&);
	Value(Value&&);
	Value(int64_t v) : m_type(ValueType::INTEGER), m_data(new int64_t(v)) {}
	Value(ValueType t, size_t reserved_size);
	explicit Value(const std::string& s, ValueType t);
	explicit Value(const char* s, ValueType t);
	Value(const char* s, size_t l, ValueType t);
	Value(std::initializer_list<Value> l);

public:
	inline ValueType getType(void) const { return m_type; }
	bool isSimpleString(void) const { return ValueType::SIMPLE_STRING == getType(); }
	bool isError(void) const { return ValueType::ERROR == getType(); }
	bool isInteger(void) const { return ValueType::INTEGER == getType(); }
	bool isBulkString(void) const { return ValueType::BULK_STRING == getType(); }
	bool isArray(void) const { return ValueType::ARRAY == getType(); }
	bool isNull(void) const { return nullptr == m_data; }

	void resetAsArray(size_t reserved_size);

	size_t size(void) const;
	void clear(void);
	void swap(Value& v) { std::swap(m_type, v.m_type); std::swap(m_data, v.m_data); }

	void append(const Value&);
	void append(Value&&);
	void append(int64_t v);
	void append(const std::string& v, ValueType t);
	void append(const char* s, ValueType t);
	void append(const char* s, size_t l, ValueType t);
	void appendNullBulk(void);
	void appendNullArray(void);

	void assign(const Value& v);
	void assign(Value&& v);

	const int64_t& getInteger(void) const
	{
		if ( isInteger() ) return _asInteger();
		_throwInvalidType();
		return _asInteger();
	}

	int64_t getInteger(void)
	{
		if ( isInteger() ) return _asInteger();
		_throwInvalidType();
		return _asInteger();
	}

	const std::string& getString(void) const
	{
		if ( m_data )
		{
			if ( m_type == ValueType::SIMPLE_STRING
				or m_type == ValueType::BULK_STRING
				or m_type == ValueType::ERROR ) return _asString();
		}
		_throwInvalidType();
		return _asString();
	}

	std::string& getString(void)
	{
		if ( m_data )
		{
			if ( m_type == ValueType::SIMPLE_STRING
				or m_type == ValueType::BULK_STRING
				or m_type == ValueType::ERROR ) return _asString();
		}
		_throwInvalidType();
		return _asString();
	}

	const array_type& getArray(void) const
	{
		if ( isArray() ) return _asArray();
		_throwInvalidType();
		return _asArray();
	}

	array_type& getArray(void)
	{
		if ( isArray() ) return _asArray();
		_throwInvalidType();
		return _asArray();
	}

	Value& operator [] (size_t idx);
	const Value& operator [] (size_t idx) const;

	Value& operator = (const Value&);
	Value& operator = (Value&&);

	std::ostream& write(std::ostream& os) const;
	inline std::string& write(std::string& ostr) const
	{
		std::stringstream ss;
		this->write(ss);
		ss.str().swap(ostr);
		return ostr;
	}

	inline std::string writeAsString(void) const
	{
		std::stringstream ss;
		this->write(ss);
		return ss.str();
	}

private:
	explicit inline Value(ValueType type, void* data) : m_type(type), m_data(data) {}

	void _destroy(void);
	int64_t& _asInteger(void) { return *reinterpret_cast<int64_t*>(m_data); }
	const int64_t& _asInteger(void) const { return *reinterpret_cast<const int64_t*>(m_data); }
	std::string& _asString(void) { return *reinterpret_cast<std::string*>(m_data); }
	const std::string& _asString(void) const { return *reinterpret_cast<const std::string*>(m_data); }
	array_type& _asArray(void) { return *reinterpret_cast<array_type*>(m_data); }
	const array_type& _asArray(void) const { return *reinterpret_cast<const array_type*>(m_data); }

private:
	ValueType m_type;
	void* m_data = nullptr;

public:
	static void _throwNotArray(void) throw(std::domain_error) { throw(std::domain_error("not array type")); }
	static void _throwInvalidType(void) throw(std::invalid_argument) { throw(std::invalid_argument("invalid type")); }
	static void _throwNoMemory(void) throw(std::system_error) { throw(std::system_error(ENOMEM, std::system_category())); }
	static void _throwNotAllowLine(void) throw(std::invalid_argument) { throw(std::invalid_argument("not allow line feed or carrage return")); }

	friend class Reader;
};

class Reader final
{
public:
	inline Reader() = default;
//	inline Reader(size_t limit) : m_limit_depth(limit) {}

public:
	ssize_t parse(const char* buf, size_t blen);
	ssize_t parse(pw::IoBuffer& buf);
	void clear(void);

	inline Value pop(void) { Value tmp(m_cont.front()); m_cont.pop(); return tmp;}
	inline bool pop(Value& out) { if ( not m_cont.empty() ) { out = m_cont.front(); m_cont.pop(); return true; } return false; }
	inline size_t size(void) const { return m_cont.size(); }

private:
	struct status_type
	{
		size_t index;
		size_t count;
		Value node;
	};

	pw::IoBuffer m_buf;
	std::stack<status_type> m_status;
	std::queue<Value> m_cont;
//	const size_t m_limit_depth = 7;

private:
	ssize_t _parse(const std::string& line);
	ssize_t _parseInteger(const std::string& line);
	ssize_t _parseSimpleString(const std::string& line);
	ssize_t _parseError(const std::string& line);
	ssize_t _parseBulkString(const std::string& line);
	ssize_t _parseBulkString2(const std::string& line);
	ssize_t _parseArray(const std::string& line);
	ssize_t _rollback(void);
	ssize_t _appendToTop(Value&& v);
};

//namespace redis
};

class RedisPacket : public PacketInterface
{
public:
	using PacketInterface::PacketInterface;
	virtual ~RedisPacket() = default;
};

class RedisResponsePacket : public RedisPacket
{
public:
	using RedisPacket::RedisPacket;
	virtual ~RedisResponsePacket() = default;

public:
	inline ssize_t write(IoBuffer& buf) const { return buf.writeToBuffer(m_body.writeAsString()); }
	inline std::ostream& write(std::ostream& os) const { return m_body.write(os); }
	inline std::string& write(std::string& ostr) const { return m_body.write(ostr); }

	inline void clear(void) { m_body.clear(); }

public:
	redis::Value m_body{pw::redis::ValueType::INTEGER};
};

//namespace pw
}

inline std::ostream& operator << (std::ostream& os, const pw::redis::Value& v) { return v.write(os); }

#endif//__PW_REDISPACKET_H__
