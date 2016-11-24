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
 * \file pw_encode.cpp
 * \brief Support string encoding/decoding.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_encode.h"
#include "./pw_string.h"
#include "./pw_log.h"

namespace pw
{

#ifdef PWT
#	undef PWT
#endif

#ifndef PWF
#	undef PWF
#endif

#define PWT	true
#define PWF	false

struct parse_stream_type
{
	std::ostream& out;
	std::istream& in;

	inline parse_stream_type ( std::ostream& os, std::istream& is ) : out ( os ), in ( is ) {}

	inline bool isEnd ( void ) const
	{
		return in.eof();
	}
	inline bool isNotEnd ( void ) const
	{
		return not in.eof();
	}
	inline bool isFail ( void ) const
	{
		return in.fail();
	}
	inline uint8_t get ( void )
	{
		return in.peek();
	}
	inline uint8_t getAndNext ( void )
	{
		return in.get();
	}
	inline void next ( void )
	{
		in.get();
	}

	inline void append ( uint8_t c )
	{
		out.write ( reinterpret_cast<const char*> ( &c ), 1 );
	}
	inline void append ( const uint8_t* s, size_t len )
	{
		out.write ( reinterpret_cast<const char*> ( s ), len );
	}
};

template<typename _OutputType>
struct  parse_blob_type
{
	_OutputType* out;
	size_t outlen = 0;
	const uint8_t* ib;
	const uint8_t* ie;

	template<typename _InputType>
	inline parse_blob_type ( _OutputType* in_out, const _InputType& in_ib, const _InputType& in_ie ) : out ( in_out ), ib ( reinterpret_cast<const uint8_t*> ( in_ib ) ), ie ( reinterpret_cast<uint8_t*> ( in_ie ) ) {}

	template<typename _InputType>
	inline parse_blob_type ( _OutputType* in_out, const _InputType& in_str ) : out ( in_out ), ib ( reinterpret_cast<const uint8_t*> ( in_str ) ), ie ( ib + strlen ( in_str ) ) {}

	template<typename _InputType>
	inline parse_blob_type ( _OutputType* in_out, const _InputType& in_str, size_t slen ) : out ( in_out ), ib ( reinterpret_cast<const uint8_t*> ( in_str ) ), ie ( ib + slen ) {}

	inline bool isEnd ( void ) const
	{
		return ib == ie;
	}
	inline bool isNotEnd ( void ) const
	{
		return ib not_eq ie;
	}
	inline constexpr bool isFail ( void ) const
	{
		return false;
	}
	inline uint8_t get ( void )
	{
		return *ib;
	}
	inline uint8_t getAndNext ( void )
	{
		return *ib++;
	}
	inline void next ( void )
	{
		++ib;
	}

	inline void append ( uint8_t c );
	inline void append ( const uint8_t* s, size_t len );
};

template<>
void parse_blob_type<uint8_t>::append ( uint8_t c )
{
	*out = c;
	++out;
	++outlen;
}

template<>
void parse_blob_type<uint8_t>::append ( const uint8_t* s, size_t len )
{
	memcpy ( out, s, len );
	out += len;
	outlen += len;
}

template<>
void parse_blob_type<std::string>::append ( uint8_t c )
{
	out->append ( 1, static_cast<char> ( c ) );
	++outlen;
}

template<>
void parse_blob_type<std::string>::append ( const uint8_t* s, size_t len )
{
	out->append ( reinterpret_cast<const char*> ( s ), len );
	outlen += len;
}

template<>
void parse_blob_type<std::ostream>::append ( uint8_t c )
{
	out->write ( reinterpret_cast<char*> ( &c ), 1 );
	++outlen;
}

template<>
void parse_blob_type<std::ostream>::append ( const uint8_t* s, size_t len )
{
	out->write ( reinterpret_cast<const char*> ( s ), len );
	outlen += len;
}

static constexpr bool g_url_table[] =
{
	PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT,
	PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT,
	PWT, PWF, PWT, PWT, PWF, PWT, PWT, PWF, PWF, PWF, PWF, PWT, PWF, PWF, PWF, PWT,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWT, PWT, PWT, PWT, PWT, PWT,
	PWT, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWT, PWT, PWT, PWT, PWF,
	PWT, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWT, PWT, PWT, PWT, PWT,
	PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT,
	PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT,
	PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT,
	PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT,
	PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT,
	PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT,
	PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT,
	PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT
};

static constexpr bool g_url2_table[] =
{
	PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT,
	PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT,
	PWT, PWF, PWT, PWT, PWF, PWT, PWT, PWF, PWF, PWF, PWF, PWT, PWF, PWF, PWF, PWT,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWT, PWT, PWT, PWT, PWT, PWT,
	PWT, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWT, PWT, PWT, PWT, PWF,
	PWT, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWT, PWT, PWT, PWT, PWT,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
	PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF
};

static constexpr uint8_t g_base64_enc_table[] =
{
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
};

static constexpr uint8_t g_base64_enc2_table[] =
{
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '-', '_'
};

static constexpr int g_base64_dec_table[] =
{
	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,	-1,  -1,  -1,  62,  -1,  62,  -1,  63,
	52,  53,  54,  55,  56,  57,  58,  59,	60,  61,  -1,  -1,  -1,  -1,  -1,  -1,
	-1,   0,   1,   2,   3,   4,   5,   6,	 7,   8,   9,  10,  11,  12,  13,  14,
	15,  16,  17,  18,  19,  20,  21,  22,	23,  24,  25,  -1,  -1,  -1,  -1,  63,
	-1,  26,  27,  28,  29,  30,  31,  32,	33,  34,  35,  36,  37,  38,  39,  40,
	41,  42,  43,  44,  45,  46,  47,  48,	49,  50,  51,  -1,  -1,  -1,  -1,  -1,
	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,	-1,  -1,  -1,  -1,  -1,  -1,  -1,  -1
};

static constexpr uint8_t g_escape_enc_table[] =
{
	0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x61, 0x62, 0x74, 0x6e, 0x76, 0x66, 0x72, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static constexpr uint8_t g_escape_dec_table2[] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x01, 0x04, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#if 0
inline
static
uint8_t
_Oct2Bin ( const uint8_t* v, int len )
{
	unsigned int out ( 0 );

	for ( int i ( 0 ); i < len; i++ )
	{
		out <<= 3;
		out or_eq ( v[i] - uint8_t ( '0' ) );
	}

	return out;
}
#endif

inline
int
_Bin2Oct ( uint8_t out[3], uint8_t in )
{
	const int far ( in < 8 ? 1 : in < 64 ? 2 : 3 );

	int i ( 0 );

	do
	{
		out[far - i - 1] = ( in bitand 7 ) + uint8_t ( '0' );

		if ( ( in >>= 3 ) == 0 ) break;
	}
	while ( ++i < far );

	return far;
}

inline
static
uint8_t
_Bin2HexHalf ( uint8_t c )
{
	if ( c < 10 ) return ( uint8_t ( '0' ) + c );

	return uint8_t ( 'A' ) + c - 10;
}

inline
static
uint8_t
_Bin2HexHalfLower ( uint8_t c )
{
	if ( c < 10 ) return ( uint8_t ( '0' ) + c );

	return uint8_t ( 'a' ) + c - 10;
}

inline
static
uint8_t
_Hex2BinHalf ( uint8_t i )
{
	if ( i >= uint8_t ( '0' ) && i <= uint8_t ( '9' ) ) return ( i - uint8_t ( '0' ) );

	if ( i >= uint8_t ( 'A' ) && i <= uint8_t ( 'Z' ) ) return ( i - uint8_t ( 'A' ) + 10 );

	return ( i - uint8_t ( 'a' ) + 10 );
}

inline
static
bool
_isHex ( uint8_t c )
{
	static constexpr bool hextbl[] =
	{
		PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
		PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
		PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
		PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWT, PWF, PWF, PWF, PWF, PWF, PWF,
		PWF, PWT, PWT, PWT, PWT, PWT, PWT, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
		PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
		PWF, PWT, PWT, PWT, PWT, PWT, PWT, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
		PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
		PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
		PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
		PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
		PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
		PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
		PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
		PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF,
		PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF, PWF
	};

	return hextbl[c];
}

inline
static
void
_Bin2HexFull ( uint8_t out[2], uint8_t in )
{
	out[0] = _Bin2HexHalf ( in >> 4 );
	out[1] = _Bin2HexHalf ( in & uint8_t ( 0x0f ) );
}

inline
static
void
_Bin2HexFullLower ( uint8_t out[2], uint8_t in )
{
	out[0] = _Bin2HexHalfLower ( in >> 4 );
	out[1] = _Bin2HexHalfLower ( in & uint8_t ( 0x0f ) );
}

#if 0
inline
static
uint8_t
_Hex2BinFull ( const uint8_t in[2] )
{
	//fprintf(stderr, "in: %c%c\n", (in[0]), (in[1]));
	return ( _Hex2BinHalf ( in[0] ) << 4 | _Hex2BinHalf ( in[1] ) );
}
#endif

template<uint8_t _PaddingChar>
inline
static
size_t
_Bin2Base64 ( uint8_t out[4], const uint8_t* ibuf, size_t ilen, const uint8_t* tab )
{
	if ( ilen > 2 )
	{
		out[0] = tab[ ibuf[0] >> 2 ];
		out[1] = tab[ ( ( ibuf[0] & 0x03 ) << 4 ) + ( ibuf[1] >> 4 ) ];
		out[2] = tab[ ( ( ibuf[1] & 0x0f ) << 2 ) + ( ibuf[2] >> 6 ) ];
		out[3] = tab[ ibuf[2] & 0x3f ];
		return 4;
	}

	if ( ilen == 2 )
	{
		out[0] = tab[ ibuf[0] >> 2 ];
		out[1] = tab[ ( ( ibuf[0] & 0x03 ) << 4 ) + ( ibuf[1] >> 4 ) ];
		out[2] = tab[ ( ibuf[1] & 0x0f ) << 2 ];
		out[3] = _PaddingChar;
		return 3;
	}

	out[0] = tab[ ibuf[0] >> 2 ];
	out[1] = tab[ ( ibuf[0] & 0x03 ) << 4 ];
	out[2] = out[3] = _PaddingChar;
	return 2;
}

inline
static
size_t
_Base642Bin ( uint8_t out[3], const uint8_t ibuf[4] )
{
	int c;

	size_t idx ( 0 );

	while ( idx < 4 )
	{
		c = ibuf[idx];

		if ( c == '=' )
		{
			break;
		}

		if ( c == ' ' )
		{
			c = '+';
		}

		if ( ( c = g_base64_dec_table[c] ) == -1 )
		{
			// ERROR
			break;
		}

		switch ( idx )
		{
		case 0:
		{
			out[0] = c << 2;
			break;
		}

		case 1:
		{
			out[0] |= c >> 4;
			out[1] = ( c & 0x0f ) << 4;
			break;
		}

		case 2:
		{
			out[1] |= c >> 2;
			out[2] = ( c & 0x03 ) << 6;
			break;
		}

		case 3:
		{
			out[2] |= c;
		}
		}// switch(idx)

		++idx;
	}// while ( idx<4 )

	// idx
	// 4 -> 3
	// 3 -> 2
	// 2 -> 1
	// 1 -> 0
	// 0 -> 0

	if ( idx < 2 ) return 0;

	return idx - 1;
}

inline
static
size_t
_Bin2Escape ( uint8_t out[2], uint8_t c )
{
	uint8_t rpl;

	if ( 0x00 not_eq ( rpl = g_escape_enc_table[c] ) )
	{
		out[0] = '\\';
		out[1] = rpl;
		return 2;
	}

	out[0] = c;
	return 1;
}

// URL encode/decode
template<typename _OutputType, const bool* _Table>
inline
static
void
encodeURL_loop ( _OutputType& b )
{
	uint8_t c;
	uint8_t tmp[4] = { '%', 0x00, 0x00, 0x00 };

	while ( b.isNotEnd() )
	{
		c = b.getAndNext();

		if ( b.isFail() ) break;

		if ( c == uint8_t ( ' ' ) ) b.append ( uint8_t ( '+' ) );
		else if ( _Table[c] )
		{
			_Bin2HexFull ( & ( tmp[1] ), c );
			b.append ( tmp, 3 );
		}
		else b.append ( c );
	}
}

size_t
Encode::encodeURLA ( void* out, const void* in, size_t ilen )
{
	parse_blob_type<uint8_t> b ( static_cast<uint8_t*> ( out ), in, ilen );

	encodeURL_loop<parse_blob_type<uint8_t>, g_url_table> ( b );

	return b.outlen;
}

std::string&
Encode::encodeURL ( std::string& out, const void* in, size_t ilen )
{
	std::string buf;
	parse_blob_type<std::string> b ( &buf, in, ilen );
	buf.reserve ( ilen * 1.5 );

	encodeURL_loop<parse_blob_type<std::string>, g_url_table> ( b );

	buf.swap ( out );
	return out;
}

std::ostream&
Encode::encodeURL ( std::ostream& out, std::istream& in )
{
	in.clear();
	parse_stream_type b ( out, in );

	encodeURL_loop<parse_stream_type, g_url_table> ( b );

	return out;
}

ostream&
Encode::encodeURL ( ostream& out, const void* in, size_t ilen )
{
	parse_blob_type<std::ostream> b ( &out, in, ilen );

	encodeURL_loop<parse_blob_type<std::ostream>, g_url_table> ( b );

	return out;
}

template<typename _OutputType>
inline
static
void
decodeURL_loop ( _OutputType& b )
{
	uint8_t c;

	while ( b.isNotEnd() )
	{
		c = b.getAndNext();

		if ( b.isFail() ) return;

		if ( c == uint8_t ( '%' ) )
		{
			unsigned int num ( 0 );

			if ( b.isEnd() ) break;

			c = b.get();

			if ( not _isHex ( c ) ) continue;

			b.next();

			num = _Hex2BinHalf ( c );

			if ( b.isEnd() )
			{
				b.append ( num );
				break;
			}

			c = b.get();

			if ( not _isHex ( c ) )
			{
				b.append ( num );
				continue;
			}

			b.next();

			num = ( num << 4 ) bitor _Hex2BinHalf ( c );
			b.append ( num );

			continue;
		}
		else if ( c == uint8_t ( '+' ) ) b.append ( uint8_t ( ' ' ) );
		else b.append ( c );
	}
}

size_t
Encode::decodeURLA ( void* out, const void* in, size_t ilen )
{
	parse_blob_type<uint8_t> b ( static_cast<uint8_t*> ( out ), in, ilen );

	decodeURL_loop ( b );

	return b.outlen;
}

std::string&
Encode::decodeURL ( std::string& out, const void* in, size_t ilen )
{
	std::string buf;
	parse_blob_type<std::string> b ( &buf, in, ilen );
	buf.reserve ( ilen );

	decodeURL_loop ( b );

	buf.swap ( out );
	return out;

}

std::ostream&
Encode::decodeURL ( std::ostream& out, std::istream& in )
{
	in.clear();
	parse_stream_type b ( out, in );

	decodeURL_loop ( b );

	return out;
}

ostream&
Encode::decodeURL ( ostream& out, const void* in, size_t ilen )
{
	parse_blob_type< std::ostream > b ( &out, in, ilen );

	encodeURL_loop<parse_blob_type<std::ostream>, g_url_table> ( b );

	return out;
}

// URL2 encode/decode
size_t
Encode::encodeURL2A ( void* out, const void* in, size_t ilen )
{
	parse_blob_type<uint8_t> b ( static_cast<uint8_t*> ( out ), in, ilen );

	encodeURL_loop<parse_blob_type<uint8_t>, g_url2_table> ( b );

	return b.outlen;
}

std::string&
Encode::encodeURL2 ( std::string& out, const void* in, size_t ilen )
{
	std::string buf;
	parse_blob_type<std::string> b ( &buf, in, ilen );
	buf.reserve ( ilen * 1.5 );

	encodeURL_loop<parse_blob_type<std::string>, g_url2_table> ( b );

	buf.swap ( out );

	return out;
}

std::ostream&
Encode::encodeURL2 ( std::ostream& out, std::istream& in )
{
	in.clear();
	parse_stream_type b ( out, in );

	encodeURL_loop<parse_stream_type, g_url2_table> ( b );

	return out;
}

std::ostream&
Encode::encodeURL2 ( std::ostream& out, const void* in, size_t ilen )
{
	parse_blob_type< std::ostream > b ( &out, in, ilen );

	encodeURL_loop<parse_blob_type<std::ostream>, g_url2_table> ( b );

	return out;
}


// Hex encode/decode

template<typename _OutputType>
inline
static
void
encodeHex_loop ( _OutputType& b, bool upper )
{
	uint8_t c;
	uint8_t buf[2];

	if ( upper )
	{
		while ( b.isNotEnd() )
		{
			c = b.getAndNext();

			if ( b.isFail() ) return;

			_Bin2HexFull ( buf, c );
			b.append ( buf, 2 );
		}
	}
	else
	{
		while ( b.isNotEnd() )
		{
			c = b.getAndNext();

			if ( b.isFail() ) return;

			_Bin2HexFullLower ( buf, c );
			b.append ( buf, 2 );
		}
	}
}

size_t
Encode::encodeHexA ( void* out, const void* in, size_t ilen, bool upper )
{
	parse_blob_type<uint8_t> b ( static_cast<uint8_t*> ( out ), in, ilen );

	encodeHex_loop ( b, upper );

	return b.outlen;
}

std::string&
Encode::encodeHex ( std::string& out, const void* in, size_t ilen, bool upper )
{
	std::string buf;
	parse_blob_type<std::string> b ( &buf, in, ilen );
	buf.reserve ( ilen * 2 );

	encodeHex_loop ( b, upper );

	buf.swap ( out );

	return out;
}

std::ostream&
Encode::encodeHex ( std::ostream& out, std::istream& in, bool upper )
{
	in.clear();
	parse_stream_type b ( out, in );
	encodeHex_loop ( b, upper );
	return out;
}

std::ostream&
Encode::encodeHex ( std::ostream& out, const void* in, size_t ilen, bool upper )
{
	parse_blob_type<std::ostream> b ( &out, in, ilen );

	encodeHex_loop ( b, upper );

	return out;
}

template<typename _OutputType>
inline
static
void
decodeHex_loop ( _OutputType& b )
{
	unsigned int num ( 0 );
	uint8_t c;

	while ( b.isNotEnd() )
	{
		c = b.getAndNext();

		if ( b.isFail() ) return;

		num = _Hex2BinHalf ( c );

		if ( b.isEnd() )
		{
			b.append ( num );
			return;
		}

		c = b.getAndNext();

		if ( b.isFail() )
		{
			b.append ( num );
			return;
		}

		num = ( num << 4 ) bitor _Hex2BinHalf ( c );

		b.append ( num );
	}
}

size_t
Encode::decodeHexA ( void* out, const void* in, size_t ilen )
{
	parse_blob_type<uint8_t> b ( static_cast<uint8_t*> ( out ), in, ilen );

	decodeHex_loop ( b );

	return b.outlen;
}

std::string&
Encode::decodeHex ( std::string& out, const void* in, size_t ilen )
{
	std::string buf;
	parse_blob_type<std::string> b ( &buf, in, ilen );
	buf.reserve ( ilen / 2 + 1 );

	decodeHex_loop ( b );

	buf.swap ( out );
	return out;
}

std::ostream&
Encode::decodeHex ( std::ostream& out, std::istream& in )
{
	in.clear();

	parse_stream_type b ( out, in );

	decodeHex_loop ( b );

	return out;
}

ostream&
Encode::decodeHex ( ostream& out, const void* in, size_t ilen )
{
	parse_blob_type< std::ostream > b ( &out, in, ilen );

	decodeHex_loop ( b );

	return out;
}

// Base64 encode/decode
template<typename _OutputType>
inline
static
void
encodeBase64_loop ( _OutputType& b, bool isURI, bool isPadding )
{
	uint8_t tmp_in[3];
	uint8_t tmp_out[4];
	size_t in_len;

	auto tab ( isURI ? g_base64_enc2_table : g_base64_enc_table );
	auto enc ( isPadding ? _Bin2Base64 < uint8_t ( '=' ) > : _Bin2Base64<uint8_t ( 0x00 ) > );

	if ( isPadding )
	{
		while ( b.isNotEnd() )
		{
			in_len = 0;

			// Input 3bytes
			while ( in_len < 3 and b.isNotEnd() )
			{
				tmp_in[in_len] = b.getAndNext();

				if ( b.isFail() )
				{
					if ( in_len == 0 ) return;

					break;
				}

				++in_len;
			}

			// 3 bytes...
			//PWTRACE("in_len: %zu", in_len);
			enc ( tmp_out, tmp_in, in_len, tab );
			b.append ( tmp_out, sizeof ( tmp_out ) );
		}// loop!
	}
	else
	{
		size_t out_len;

		while ( b.isNotEnd() )
		{
			in_len = 0;

			// Input 3bytes
			while ( in_len < 3 and b.isNotEnd() )
			{
				tmp_in[in_len] = b.getAndNext();

				if ( b.isFail() )
				{
					if ( in_len == 0 ) return;

					break;
				}

				++in_len;
			}

			// 3 bytes...
			out_len = enc ( tmp_out, tmp_in, in_len, tab );
			b.append ( tmp_out, out_len );
		}// loop!
	}
}

size_t
Encode::encodeBase64A ( void* out, const void* in, size_t ilen, bool isURI, bool isPadding )
{
	parse_blob_type<uint8_t> b ( static_cast<uint8_t*> ( out ), in, ilen );

	encodeBase64_loop ( b, isURI, isPadding );

	return b.outlen;
}

std::string&
Encode::encodeBase64 ( std::string& out, const void* in, size_t ilen, bool isURI, bool isPadding )
{
	std::string buf;
	parse_blob_type<std::string> b ( &buf, in, ilen );
	buf.reserve ( ( ilen / 3 + 1 ) * 4 );

	encodeBase64_loop ( b, isURI, isPadding );

	buf.swap ( out );
	return out;
}

std::ostream&
Encode::encodeBase64 ( std::ostream& out, std::istream& in, bool isURI, bool isPadding )
{
	in.clear();
	parse_stream_type b ( out, in );

	encodeBase64_loop ( b, isURI, isPadding );

	return out;
}

ostream&
Encode::encodeBase64 ( ostream& out, const void* in, size_t ilen, bool isURI, bool isPadding )
{
	parse_blob_type<std::ostream> b ( &out, in, ilen );

	encodeBase64_loop ( b, isURI, isPadding );

	return out;
}

template<typename _OutputType>
inline
static
void
decodeBase64_loop ( _OutputType& b )
{
	uint8_t in[4];
	uint8_t out[3];
	size_t ilen, olen;

	ilen = 0;

	while ( b.isNotEnd() )
	{
		in[ilen] = b.getAndNext();

		if ( b.isFail() ) break;

		if ( ++ilen >= 4 )
		{
			olen = _Base642Bin ( out, in );

			if ( olen ) b.append ( out, olen );

			ilen = 0;
		}
	}

	if ( ilen )
	{
		if ( ilen < 4 ) in[ilen] = '=';

		olen = _Base642Bin ( out, in );

		if ( olen ) b.append ( out, olen );
	}
}

size_t
Encode::decodeBase64A ( void* out, const void* in, size_t ilen )
{
	parse_blob_type<uint8_t> b ( static_cast<uint8_t*> ( out ), in, ilen );

	decodeBase64_loop ( b );

	return b.outlen;
}

std::string&
Encode::decodeBase64 ( std::string& out, const void* in, size_t ilen )
{
	std::string buf;
	parse_blob_type<std::string> b ( &buf, in, ilen );
	buf.reserve ( ( ilen * 3 ) / 4 + 1 );

	decodeBase64_loop ( b );

	buf.swap ( out );
	return out;
}

std::ostream&
Encode::decodeBase64 ( std::ostream& out, std::istream& in )
{
	in.clear();
	parse_stream_type b ( out, in );

	decodeBase64_loop ( b );

	return out;
}

ostream&
Encode::decodeBase64 ( ostream& out, const void* in, size_t ilen )
{
	parse_blob_type< std::ostream > b ( &out, in, ilen );

	decodeBase64_loop ( b );

	return out;
}


template<typename _OutputType>
inline
void
encodeEscape_loop ( _OutputType& b )
{
	uint8_t c;
	uint8_t tmp[10];
	size_t size;

	while ( b.isNotEnd() )
	{
		c = b.getAndNext();

		if ( b.isFail() ) return;

		size = _Bin2Escape ( tmp, c );

		if ( size ) b.append ( tmp, size );
	}
}

size_t
Encode::encodeEscapeA ( void* out, const void* in, size_t ilen )
{
	parse_blob_type<uint8_t> b ( static_cast<uint8_t*> ( out ), in, ilen );

	encodeEscape_loop ( b );

	return b.outlen;
}

std::ostream&
Encode::encodeEscape ( std::ostream& out, const void* in, size_t ilen )
{
	parse_blob_type<std::ostream> b ( &out, in, ilen );

	encodeEscape_loop ( b );

	return out;
}

std::string&
Encode::encodeEscape ( std::string& out, const void* in, size_t ilen )
{
	std::string buf;
	parse_blob_type<std::string> b ( &buf, in, ilen );
	buf.reserve ( ilen * 1.5 );

	encodeEscape_loop ( b );

	buf.swap ( out );
	return out;
}

std::ostream&
Encode::encodeEscape ( std::ostream& out, std::istream& in )
{
	in.clear();
	parse_stream_type b ( out, in );

	encodeEscape_loop ( b );

	return out;
}

template<typename _OutputType>
inline
void
decodeEscape_control ( _OutputType& b, uint8_t c )
{
	switch ( c )
	{
	case 'a':
		c = 0x07;
		break;

	case 'b':
		c = 0x08;
		break;

	case 't':
		c = 0x09;
		break;

	case 'n':
		c = 0x0a;
		break;

	case 'v':
		c = 0x0b;
		break;

	case 'f':
		c = 0x0c;
		break;

	case 'r':
		c = 0x0d;
		break;
	}

	b.append ( c );
}

template<typename _OutputType>
inline
void
decodeEscape_otet ( _OutputType& b, uint8_t c )
{
	unsigned int num ( 0 );
	int size ( 1 );

	do
	{
		num <<= 3;
		num or_eq ( c - uint8_t ( '0' ) );

		if ( b.isEnd() or ( ++size > 3 ) )
		{
			break;
		}

		c = b.get();

		if ( ( c < '0' ) or ( c > '7' ) )
		{
			break;
		}

		b.next();
	}
	while ( true );

	b.append ( num );
}

template<typename _OutputType>
inline
void
decodeEscape_hex ( _OutputType& b )
{
	if ( b.isEnd() )
	{
		b.append ( 0x00 );
		return;
	}

	unsigned int num ( 0 );
	const auto c ( b.get() );

	if ( not _isHex ( c ) )
	{
		b.append ( 0x00 );
		return;
	}

	b.next();

	if ( b.isNotEnd() )
	{
		const auto d ( b.get() );

		if ( _isHex ( d ) )
		{
			num = _Hex2BinHalf ( c ) << 4 bitor _Hex2BinHalf ( d );
			b.append ( num );
			b.next();
			return;
		}
	}

	num = _Hex2BinHalf ( c );
	b.append ( num );
}

template<typename _OutputType>
inline
void
decodeEscape_unicode ( _OutputType& b, size_t maxlen )
{
	if ( b.isEnd() )
	{
		b.append ( 0x00 );
		return;
	}

	decltype ( maxlen ) i ( 0 );
	unsigned int num ( 0 );
	auto c ( b.get() );

	if ( not _isHex ( c ) )
	{
		b.append ( 0x00 );
		return;
	}

	b.next();

	do
	{
		num <<= 4;
		num or_eq ( c - uint8_t ( '0' ) );

		if ( ( i % 2 ) == 1 )
		{
			b.append ( num );
			num = 0;
		}

		if ( b.isEnd() or ( ++i >= maxlen ) )
		{
			if ( ( i % 2 ) == 0 ) return;

			break;
		}

		if ( not _isHex ( ( c = b.get() ) ) )
		{
			break;
		}

		b.next();
	}
	while ( true );

	b.append ( num );
}

template<typename _OutputType>
void
decodeEscape_loop ( _OutputType& b )
{
	uint8_t c;

	while ( b.isNotEnd() )
	{
		c = b.getAndNext();

		if ( b.isFail() ) return;

		if ( c == '\\' )
		{
			if ( b.isEnd() ) break;

			switch ( g_escape_dec_table2[ ( c = b.getAndNext() )] )
			{
			case 0x01:
				decodeEscape_control ( b, c );
				break; // control

			case 0x02:
				decodeEscape_otet ( b, c );
				break; // Octet

			case 0x03:
				decodeEscape_hex ( b );
				break; // Hex

			case 0x04:
				decodeEscape_unicode ( b, 4 );
				break; // small unicode

			case 0x05:
				decodeEscape_unicode ( b, 8 );
				break; // big unicode

			default:
				b.append ( c );
			}//switch

			continue;
		}

		b.append ( c );
	}
}

size_t
Encode::decodeEscapeA ( void* out, const void* in, size_t ilen )
{
	parse_blob_type<uint8_t> b ( static_cast<uint8_t*> ( out ), in, ilen );

	decodeEscape_loop ( b );

	return b.outlen;
}

std::string&
Encode::decodeEscape ( std::string& out, const void* in, size_t ilen )
{
	std::string buf;
	parse_blob_type<std::string> b ( &buf, in, ilen );
	buf.reserve ( ilen );

	decodeEscape_loop ( b );

	buf.swap ( out );
	return out;
}

std::ostream&
Encode::decodeEscape ( std::ostream& out, std::istream& in )
{
	in.clear();
	parse_stream_type b ( out, in );

	uint8_t c;

	while ( b.isNotEnd() )
	{
		c = b.getAndNext();

		if ( not in.good() ) continue;

		if ( c == '\\' )
		{
			if ( b.isEnd() ) break;

			switch ( g_escape_dec_table2[ ( c = b.getAndNext() )] )
			{
			case 0x01:
				decodeEscape_control ( b, c );
				break; // control

			case 0x02:
				decodeEscape_otet ( b, c );
				break; // Octet

			case 0x03:
				decodeEscape_hex ( b );
				break; // Hex

			case 0x04:
				decodeEscape_unicode ( b, 4 );
				break; // small unicode

			case 0x05:
				decodeEscape_unicode ( b, 8 );
				break; // big unicode

			default:
				b.append ( c );
			}//switch

			continue;
		}

		b.append ( c );
	}

	return out;
}


std::ostream&
Encode::decodeEscape ( std::ostream& out, const char* in, size_t ilen )
{
	parse_blob_type<std::ostream> b ( &out, in, ilen );
	decodeEscape_loop ( b );
	return out;
}

}; //namespace pw
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4;
