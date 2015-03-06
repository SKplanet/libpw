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
 * \file pw_exception.h
 * \brief Exception classes.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "pw_common.h"

#ifndef __PW_EXCEPT_H__
#define __PW_EXCEPT_H__

namespace pw {

//! \brief 기본 예외 클래스
class Exception
{
public:
	inline Exception(ResultCode _rcode) : m_rcode{_rcode} {}
	inline Exception(ResultCode _rcode, const std::string& _rmesg) : m_rcode{_rcode}, m_rmesg{_rmesg} {}
	inline Exception(const Exception&) = default;
	inline Exception(Exception&&) = default;
	inline ~Exception() = default;

	Exception() = delete;
	inline Exception& operator = (const Exception&) = default;
	inline Exception& operator = (Exception&&) = default;

public:
	inline ResultCode code(void) const { return m_rcode; }
	inline const char* what(void) const { return m_rmesg.c_str(); }
	inline const std::string& whatS(void) const { return m_rmesg; }

private:
	ResultCode		m_rcode;
	std::string		m_rmesg;
};

//! \brief 기타 예외 클래스
template<ResultCode _Code>
class ExceptionTemplate final : public Exception
{
public:
	using Exception::Exception;
	inline ExceptionTemplate() : Exception(_Code, pw::getErrorMessage(_Code)) {}
	inline ExceptionTemplate(const std::string& _rmesg) : Exception(_Code, _rmesg) {}
	ExceptionTemplate(ResultCode) = delete;
	ExceptionTemplate(ResultCode, const std::string&) = delete;
};

using Exception_continue = ExceptionTemplate<ResultCode::CONTINUE>;
using Exception_switching_protocol = ExceptionTemplate<ResultCode::SWITCHING_PROTOCOL>;

using Exception_success = ExceptionTemplate<ResultCode::SUCCESS>;
using Exception_created = ExceptionTemplate<ResultCode::CREATED>;
using Exception_accepted = ExceptionTemplate<ResultCode::ACCEPTED>;
using Exception_noauth_information = ExceptionTemplate<ResultCode::NOAUTH_INFORMATION>;
using Exception_no_content = ExceptionTemplate<ResultCode::NO_CONTENT>;
using Exception_reset_content = ExceptionTemplate<ResultCode::RESET_CONTENT>;
using Exception_partial_content = ExceptionTemplate<ResultCode::PARTIAL_CONTENT>;

using Exception_multiple_choices = ExceptionTemplate<ResultCode::MULTIPLE_CHOICES>;
using Exception_moved_permanently = ExceptionTemplate<ResultCode::MOVED_PERMANENTLY>;
using Exception_found = ExceptionTemplate<ResultCode::FOUND>;
using Exception_see_other = ExceptionTemplate<ResultCode::SEE_OTHER>;
using Exception_not_modified = ExceptionTemplate<ResultCode::NOT_MODIFIED>;
using Exception_use_proxy = ExceptionTemplate<ResultCode::USE_PROXY>;
using Exception_temporary_redirect = ExceptionTemplate<ResultCode::TEMPORARY_REDIRECT>;

using Exception_bad_request = ExceptionTemplate<ResultCode::BAD_REQUEST>;
using Exception_unauthorized = ExceptionTemplate<ResultCode::UNAUTHORIZED>;
using Exception_payment_required = ExceptionTemplate<ResultCode::PAYMENT_REQUIRED>;
using Exception_forbidden = ExceptionTemplate<ResultCode::FORBIDDEN>;
using Exception_not_found = ExceptionTemplate<ResultCode::NOT_FOUND>;
using Exception_method_not_allowed = ExceptionTemplate<ResultCode::METHOD_NOT_ALLOWED>;
using Exception_not_acceptable = ExceptionTemplate<ResultCode::NOT_ACCEPTABLE>;
using Exception_proxy_auth_required = ExceptionTemplate<ResultCode::PROXY_AUTH_REQUIRED>;
using Exception_request_timeout = ExceptionTemplate<ResultCode::REQUEST_TIMEOUT>;
using Exception_conflict = ExceptionTemplate<ResultCode::CONFLICT>;
using Exception_gone = ExceptionTemplate<ResultCode::GONE>;
using Exception_length_required = ExceptionTemplate<ResultCode::LENGTH_REQUIRED>;
using Exception_precondition_failed = ExceptionTemplate<ResultCode::PRECONDITION_FAILED>;
using Exception_request_entity_too_large = ExceptionTemplate<ResultCode::REQUEST_ENTITY_TOO_LARGE>;
using Exception_request_uri_too_long = ExceptionTemplate<ResultCode::REQUEST_URI_TOO_LONG>;
using Exception_unsupported_media_type = ExceptionTemplate<ResultCode::UNSUPPORTED_MEDIA_TYPE>;
using Exception_request_range_failed = ExceptionTemplate<ResultCode::REQUEST_RANGE_FAILED>;
using Exception_expect_failed = ExceptionTemplate<ResultCode::EXPECT_FAILED>;

using Exception_internal_server_error = ExceptionTemplate<ResultCode::INTERNAL_SERVER_ERROR>;
using Exception_not_implemented = ExceptionTemplate<ResultCode::NOT_IMPLEMENTED>;
using Exception_bad_gateway = ExceptionTemplate<ResultCode::BAD_GATEWAY>;
using Exception_service_unavailable = ExceptionTemplate<ResultCode::SERVICE_UNAVAILABLE>;
using Exception_gateway_timeout = ExceptionTemplate<ResultCode::GATEWAY_TIMEOUT>;
using Exception_version_not_supported = ExceptionTemplate<ResultCode::VERSION_NOT_SUPPORTED>;

};// namespace pw

#endif//__PW_EXCEPT_H__
