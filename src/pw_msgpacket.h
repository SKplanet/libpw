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
 * \file pw_msgpacket.h
 * \brief Channel for pw default message.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"
#include "./pw_packet_if.h"
#include "./pw_key.h"

#ifndef __PW_MSGPACKET_H__
#define __PW_MSGPACKET_H__

namespace pw {

//! \brief 메시지 패킷
//! [CODE:4] [TRID:5] [FLAGS:3] [BODY_LENGTH:7]( [CHUNKED_TOTAL] [CHUNKED_INDEX] [APPENDIX])\\r\\n([BODY])
class MsgPacket : public PacketInterface
{
public:
	//! \brief 한계치
	enum class limit_type : intmax_t
	{
		MIN_HEADER_SIZE = (4+1+1+1+1+1+1+2),
		MAX_HEADER_SIZE = 1024LL*4,		//!< 최대 헤더 길이
		MAX_BODY_SIZE = 1024LL*1024,	//!< 최대 바디 길이
	};

	//! \brief 플래그 타입
	enum class flag_type : uint8_t
	{
		COMPRESSED = 0,
		ENCRYPTED,
		CHUNKED,
	};

	//! \brief 나눠 받을 때 청크 정보
	typedef union chunked_info_type
	{
		struct {
			uint16_t total;		//!< 전체 개수
			uint16_t index;		//!< 1부터 시작
		} p;

		uint32_t i;

		inline chunked_info_type() : i(0) {}
		inline chunked_info_type(const chunked_info_type& o) : i(o.i) {}
		inline chunked_info_type(uint16_t total, uint16_t index) { p.total = total; p.index = index; }

		inline uint16_t& total(void) { return p.total; }
		inline const uint16_t& total(void) const { return p.total; }
		inline uint16_t& index(void) { return p.index; }
		inline const uint16_t& index(void) const { return p.index; }

		inline void clear(void) { i = 0; }
		inline void swap(chunked_info_type& v) { std::swap(v.i, i); }
	} chunked_info_type;

public:
	explicit MsgPacket();
	explicit MsgPacket(const MsgPacket& pk);
	explicit MsgPacket(const char* buf, size_t blen);
	inline virtual ~MsgPacket() {}

public:
	//! \brief 패킷을 비운다.
	void clear(void);

	//! \brief 패킷 내용을 교환한다.
	void swap(MsgPacket& pk);

public:
	//! \brief 바디 사이즈를 얻어온다.
	inline size_t getBodySize(void) const { return m_body.size; }

	//! \brief 플래그 비트를 얻어온다.
	inline bool getFlag(size_t idx) const { return (m_flags >> idx) bitand 1; }

	//! \brief 플래그 비트를 얻어온다.
	inline bool getFlag(flag_type idx) const { return (m_flags >> int8_t(idx)) bitand 1; }

	//! \brief 플래그 비트를 설정한다.
	inline void setFlag(size_t idx, bool v) { if ( v ) m_flags |= ( 1 << idx ); else m_flags &= ~( 1 << idx ); }

	//! \brief 플래그 비트를 설정한다.
	inline void setFlag(flag_type idx, bool v) { if ( v ) m_flags |= ( 1 << int8_t(idx) ); else m_flags &= ~( 1 << int8_t(idx) ); }

	//! \brief 패킷을 여러개로 나누었는가?
	inline bool isFlagChunked(void) const { return this->getFlag(flag_type::CHUNKED); }

	//! \brief 패킷을 암호화 하였는가?
	inline bool isFlagEncrypted(void) const { return this->getFlag(flag_type::ENCRYPTED); }

	//! \brief 패킷을 압축하였는가?
	inline bool isFlagCompressed(void) const { return this->getFlag(flag_type::COMPRESSED); }

public:
	//! \brief 헤더를 파싱해서 설정한다.
	//! \param[in] buf 헤더
	//! \param[in] blen 헤더 길이
	//! \return 성공하면 true를 반환한다.
	bool setHeader(const char* buf, size_t blen);

	//! \brief 코드와 트랜젝션 아이디만 복제한다. 응답을 만들 때 용이하다.
	inline void setCodeTrid(const MsgPacket& pk) { m_code = pk.m_code; m_trid = pk.m_trid; }

	//! \brief 응답코드 형태로 설정한다.
	inline void setResultCode(ResultCode code) { m_code.format("%d", static_cast<int>(code)); }

	//! \brief 응답코드 형태로 설정한다.
	inline void setResultCode(ResultCode code, uint16_t trid) { m_code.format("%d", static_cast<int>(code)); m_trid = trid; }

	//! \brief 응답코드 형태로 설정한다.
	inline void setIntCode(int code) { m_code.format("%d", code); }

	//! \brief 응답코드 형태로 설정한다.
	inline void setIntCode(int code, uint16_t trid) { m_code.format("%d", code); m_trid = trid; }

	//! \brief 실제 패킷 사이즈를 구한다.
	size_t getPacketSize(void) const;

	std::ostream& write(std::ostream&) const;
	std::string& write(std::string&) const;
	ssize_t write(IoBuffer&) const;

	//! \brief 패킷을 덤프한다.
	std::ostream& dump(std::ostream& os) const;

public:
	KeyCode				m_code;
	uint16_t			m_trid;
	uint8_t				m_flags;
	chunked_info_type	m_chunked;
	std::string			m_appendix;
	blob_type			m_body;

friend class MsgChannel;
};

};// namespace pw

#endif//__PW_MSGPACKET_H__
