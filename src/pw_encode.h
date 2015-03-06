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
 * \file pw_encode.h
 * \brief Support string encoding/decoding.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"

#ifndef __PW_ENCODE_H__
#define __PW_ENCODE_H__

namespace pw {

//! \brief 문자열 인코딩 관련 유틸리티 클래스
class Encode
{
public:
	// URL encode/decode
	static size_t encodeURLA(void* out, const void* in, size_t ilen);
	static inline size_t encodeURLA(void* out, const std::string& in) { return encodeURLA(out, in.c_str(), in.size()); }
	static std::string& encodeURL(std::string& out, const void* in, size_t ilen);
	static inline std::string& encodeURL(std::string& out, const std::string& in) { return encodeURL(out, in.c_str(), in.size()); }
	static std::string& encodeURL(std::string& inout) { return encodeURL(inout, inout.c_str(), inout.size()); }
	static std::ostream& encodeURL(std::ostream& out, std::istream& in);
	static std::ostream& encodeURL(std::ostream& out, const void* in, size_t ilen);

	static size_t decodeURLA(void* out, const void* in, size_t ilen);
	static inline size_t decodeURL(void* out, const std::string& in) { return decodeURLA(out, in.c_str(), in.size()); }
	static std::string& decodeURL(std::string& out, const void* in, size_t ilen);
	static inline std::string& decodeURL(std::string& out, const std::string& in) { return decodeURL(out, in.c_str(), in.size()); }
	static inline std::string& decodeURL(std::string& inout) { return decodeURL(inout, inout.c_str(), inout.size()); }
	static std::ostream& decodeURL(std::ostream& out, std::istream& in);
	static std::ostream& decodeURL(std::ostream& out, const void* in, size_t ilen);

	// URL2 encode/decode
	static size_t encodeURL2A(void* out, const void* in, size_t ilen);
	static inline size_t encodeURL2A(void* out, const std::string& in) { return encodeURL2A(out, in.c_str(), in.size()); }
	static std::string& encodeURL2(std::string& out, const void* in, size_t ilen);
	static inline std::string& encodeURL2(std::string& out, const std::string& in) { return encodeURL2(out, in.c_str(), in.size()); }
	static inline std::string& encodeURL2(std::string& inout) { return encodeURL2(inout, inout.c_str(), inout.size()); }
	static std::ostream& encodeURL2(std::ostream& out, std::istream& in);
	static std::ostream& encodeURL2(std::ostream& out, const void* in, size_t ilen);

	static inline size_t decodeURL2A(void* out, const void* in, size_t ilen) { return decodeURLA(out, in, ilen); }
	static inline size_t decodeURL2A(void* out, const std::string& in) { return decodeURLA(out, in.c_str(), in.size()); }
	static inline std::string& decodeURL2(std::string& out, const void* in, size_t ilen) { return decodeURL(out, in, ilen); }
	static inline std::string& decodeURL2(std::string& out, const std::string& in) { return decodeURL(out, in); }
	static inline std::string& decodeURL2(std::string& inout) { return decodeURL(inout); }
	static inline std::ostream& decodeURL2(std::ostream& out, std::istream& in) { return decodeURL(out, in); }
	static inline std::ostream& decodeURL2(std::ostream& out, const void* in, size_t ilen) { return decodeURL(out, in, ilen); }

	// Hex encode/decode
	static size_t encodeHexA(void* out, const void* in, size_t ilen, bool upper = true);
	static inline size_t encodeHexA(void* out, const std::string& in, bool upper = true) { return encodeHexA(out, in.c_str(), in.size(), upper); }
	static std::string& encodeHex(std::string& out, const void* in, size_t ilen, bool upper = true);
	static inline std::string& encodeHex(std::string& out, const std::string& in, bool upper = true) { return encodeHex(out, in.c_str(), in.size(), upper); }
	static inline std::string& encodeHex(std::string& inout, bool upper = true) { return encodeHex(inout, inout.c_str(), inout.size(), upper); }
	static std::ostream& encodeHex(std::ostream& out, std::istream& in, bool upper = true);
	static std::ostream& encodeHex(std::ostream& out, const void* in, size_t ilen, bool upper = true);

	static size_t decodeHexA(void* out, const void* in, size_t ilen);
	static inline size_t decodeHexA(void* out, const std::string& in) { return decodeHexA(out, in.c_str(), in.size()); }
	static std::string& decodeHex(std::string& out, const void* in, size_t ilen);
	static inline std::string& decodeHex(std::string& out, const std::string& in) { return decodeHex(out, in.c_str(), in.size()); }
	static inline std::string& decodeHex(std::string& inout) { return decodeHex(inout, inout.c_str(), inout.size()); }
	static std::ostream& decodeHex(std::ostream& out, std::istream& in);
	static std::ostream& decodeHex(std::ostream& out, const void* in, size_t ilen);

	// Base64 encode/decode
	static size_t encodeBase64A(void* out, const void* in, size_t ilen, bool isURI = true, bool isPadding = true);
	static inline size_t encodeBase64A(void* out, const std::string& in, bool isURI = true, bool isPadding = true) { return encodeBase64A(out, in.c_str(), in.size(), isURI, isPadding); }
	static std::string& encodeBase64(std::string& out, const void* in, size_t ilen, bool isURI = true, bool isPadding = true);
	static inline std::string& encodeBase64(std::string& out, const std::string& in, bool isURI = true, bool isPadding = true) { return encodeBase64(out, in.c_str(), in.size(), isURI, isPadding); }
	static inline std::string& encodeBase64(std::string& inout, bool isURI = true, bool isPadding = true) { return encodeBase64(inout, inout.c_str(), inout.size(), isURI, isPadding); }
	static std::ostream& encodeBase64(std::ostream& out, std::istream& in, bool isURI = true, bool isPadding = true);
	static std::ostream& encodeBase64(std::ostream& out, const void* in, size_t ilen, bool isURI = true, bool isPadding = true);

	static size_t decodeBase64A(void* out, const void* in, size_t ilen);
	static inline size_t decodeBase64A(void* out, const std::string& in) { return decodeBase64A(out, in.c_str(), in.size()); }
	static std::string& decodeBase64(std::string& out, const void* in, size_t ilen);
	static inline std::string& decodeBase64(std::string& out, const std::string& in) { return decodeBase64(out, in.c_str(), in.size()); }
	static inline std::string& decodeBase64(std::string& inout) { return decodeBase64(inout, inout.c_str(), inout.size()); }
	static std::ostream& decodeBase64(std::ostream& out, std::istream& in);
	static std::ostream& decodeBase64(std::ostream& out, const void* in, size_t ilen);

	// Escape encode/decode
	static size_t encodeEscapeA(void* out, const void* in, size_t ilen);
	static inline size_t encodeEscapeA(void* out, const std::string& in) { return encodeEscapeA(out, in.c_str(), in.size()); }
	static std::string& encodeEscape(std::string& out, const void* in, size_t ilen);
	static inline std::string& encodeEscape(std::string& out, const std::string& in) { return encodeEscape(out, in.c_str(), in.size()); }
	static inline std::string& encodeEscape(std::string& inout) { return encodeEscape(inout, inout.c_str(), inout.size()); }
	static std::ostream& encodeEscape(std::ostream& out, std::istream& in);
	static std::ostream& encodeEscape(std::ostream& out, const void* in, size_t ilen);

	static size_t decodeEscapeA(void* out, const void* in, size_t ilen);
	static inline size_t decodeEscapeA(void* out, const std::string& in) { return decodeEscapeA(out, in.c_str(), in.size()); }
	static std::string& decodeEscape(std::string& out, const void* in, size_t ilen);
	static inline std::string& decodeEscape(std::string& out, const std::string& in) { return decodeEscape(out, in.c_str(), in.size()); }
	static inline std::string& decodeEscape(std::string& inout) { return decodeEscape(inout, inout.c_str(), inout.size()); }
	static std::ostream& decodeEscape(std::ostream& out, std::istream& in);
	static std::ostream& decodeEscape(std::ostream& out, const char* in, size_t ilen);

private:
	explicit Encode() {}
	virtual ~Encode() {}
};

}; //namespace pw

using PWEnc = pw::Encode;

#endif//!__PW_ENCODE_H__

