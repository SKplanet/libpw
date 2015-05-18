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
 * \file pw_packet_if.cpp
 * \brief Packet interface.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_packet_if.h"

namespace pw {

ssize_t
BlobPacket::write(IoBuffer& obuf) const
{
	if ( m_body.empty() ) return 0;

	IoBuffer::blob_type b;
	if ( not obuf.grabWrite(b, m_body.size) ) return ssize_t(-1);
	memcpy(b.buf, m_body.buf, m_body.size);
	obuf.moveWrite(m_body.size);
	return m_body.size;
}

ssize_t
StlStringPacket::write(IoBuffer& obuf) const
{
	if ( m_body.empty() ) return 0;

	IoBuffer::blob_type b;
	const auto size(m_body.size());
	if ( not obuf.grabWrite(b, size) ) return ssize_t(-1);
	memcpy(b.buf, m_body.c_str(), size);
	obuf.moveWrite(size);
	return size;
}

};//namespace pw
