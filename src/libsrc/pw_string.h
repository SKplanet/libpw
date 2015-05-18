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
 * \file pw_string.h
 * \brief String utility.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"

#ifndef __PW_STRING_H__
#define __PW_STRING_H__

namespace pw {

//! \brief 문자열 유틸리티
class StringUtility final
{
public:
	enum
	{
		DEFAULT_BUFFER_SIZE = 1024*4,	//!< 내부 기본 버퍼 크기
	};

public:
	static std::istream& getLine(std::string& out, std::istream& is);
	static char* findLine(const char* p, size_t blen);

	static inline std::istream& getAll(std::string& out, std::istream& is)
	{
		std::string tmp;
		tmp.assign(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());
		out.swap(tmp);
		return is;
	}

	static inline bool getAll(std::string& out, const char* path, bool bin = true);

	static std::string& format(std::string& out, const char* fmt, ...) __attribute__((format(printf,2,3)));
	static std::string& formatV(std::string& out, const char* fmt, va_list lst);
	static std::string formatS(const char* fmt, ...) __attribute__((format(printf,1,2)));
	static std::string formatSV(const char* fmt, va_list lst);

	static std::string& trim(std::string& out, const char* in, size_t ilen);
	static inline std::string& trim(std::string& out, const std::string& in) { return trim(out, in.c_str(), in.size()); }
	static inline std::string& trim(std::string& inout) { return trim(inout, inout.c_str(), inout.size()); }

	static std::string& trimLeft(std::string& out, const char* in, size_t ilen);
	static inline std::string& trimLeft(std::string& out, const std::string& in) { return trimLeft(out, in.c_str(), in.size()); }
	static inline std::string& trimLeft(std::string& inout) { return trimLeft(inout, inout.c_str(), inout.size()); }

	static std::string& trimRight(std::string& out, const char* in, size_t ilen);
	static inline std::string& trimRight(std::string& out, const std::string& in) { return trimRight(out, in.c_str(), in.size()); }
	static inline std::string& trimRight(std::string& inout) { return trimRight(inout, inout.c_str(), inout.size()); }

	static inline const char* toTrueFalse(bool v) { return v ? "true" : "false"; }

	static inline char toYN(bool v) { return v ? 'Y' : 'N'; }

	static inline bool toBoolean(const char* v)
	{
		if ( v[0] == 0x00 ) return false;

		if ( v[0] == 'Y' or v[0] == 'y' or v[0] == 'T' or v[0] == 't') return true;
		if ( v[0] == 'N' or v[0] == 'n' or v[0] == 'F' or v[0] == 'f') return false;

		if ( 0 == strcasecmp(v, "true") ) return true;
		if ( 0 == strcasecmp(v, "yes") ) return true;

		if ( atoi(v) == 0 ) return false;

		return true;
	}

	static inline bool toBoolean(const std::string& v) { return toBoolean(v.c_str()); }

	static string_list& split(string_list& out, const char* buf, size_t blen, char d = char(' '));
	static inline string_list& split(string_list& out, const std::string& buf, char d = char(' ')) { return split(out, buf.c_str(), buf.size(), d); }
	static inline string_list& split2(string_list& out, const char* buf, size_t blen, const char* del = nullptr) { return split2(out, std::string(buf, blen), del); }
	static string_list& split2(string_list& out, const std::string& buf, const char* del = nullptr);

	static std::ostream& merge(std::ostream& out, const string_list& in, const std::string& del);
	static inline std::string& merge(std::string& out, const string_list& in, const std::string& del) { std::ostringstream os; merge(os, in, del); os.str().swap(out); return out; }
	static std::ostream& merge(std::ostream& out, const string_list& in, char del);
	static inline std::string& merge(std::string& out, const string_list& in, char del) { std::ostringstream os; merge(os, in, del); os.str().swap(out); return out; }

	static std::string& toLower(std::string& out, const std::string& in);
	static std::string& toLower(std::string& inout);
	static char* toLower(char* out, const char* in, size_t len = size_t(-1));
	static inline char* toLower(char* inout, size_t len = size_t(-1)) { return toLower(inout, inout, len); }

	static std::string& toUpper(std::string& out, const std::string& in);
	static std::string& toUpper(std::string& inout);
	static char* toUpper(char* out, const char* in, size_t len = size_t(-1));
	static inline char* toUpper(char* inout, size_t len = size_t(-1)) { return toUpper(inout, inout, len); }

	static std::string& toBase36(std::string& out, uintmax_t u);
	static std::string& toBase36(std::string& out, intmax_t i);

	static char* toBase36(char* out, uintmax_t u, size_t* olen = nullptr);
	static char* toBase36(char* out, intmax_t i, size_t* olen = nullptr);

	static size_t sliceUTF8(const char* in, size_t less_bytes);
	static size_t sliceUTF8(const char* in, size_t ilen, size_t less_bytes);
	inline static size_t sliceUTF8(const std::string& in, size_t less_bytes) { return sliceUTF8(in.c_str(), in.size(), less_bytes); }

	static std::ostream& dumpHex(std::ostream& out, const void* in, size_t ilen);

private:
	explicit StringUtility() {}
	virtual ~StringUtility() {}
};

bool
StringUtility::getAll(std::string& out, const char* path, bool bin)
{
	std::ifstream ifs;
	if ( bin ) ifs.open(path, ios::in|ios::binary);
	else ifs.open(path);

	if ( ifs.is_open() )
	{
		getAll(out, ifs);
		return true;
	}

	return false;
}

}; //namespace pw

using PWStr = pw::StringUtility;

#endif//!__PW_STRING_H__
