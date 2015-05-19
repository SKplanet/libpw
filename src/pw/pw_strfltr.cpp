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
 * \file pw_strfltr.cpp
 * \brief Support simple string filter.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_strfltr.h"
#include "./pw_log.h"
#include "./pw_string.h"
#include "./pw_tokenizer.h"
#include "./pw_digest.h"
#include "./pw_encode.h"

#ifdef PW_USE_STD_REGEX
#	include <regex>
#	define REGNS	std
#elif PW_USE_BOOST_REGEX
#	include <boost/regex.hpp>
#	define REGNS	boost
#else
#	include <regex.h>
#endif

namespace pw {

static
inline
StringFilter::Pattern
_Str2Pattern(const std::string& line)
{
	using Pattern = StringFilter::Pattern;
	static const std::map<std::string, StringFilter::Pattern, pw::str_ci_less<std::string>> s2p
	{
		{ "substr", Pattern::SUBSTRING },
		{ "substring", Pattern::SUBSTRING },
		{ "substr_icase", Pattern::SUBSTRING_I },
		{ "substring_icase", Pattern::SUBSTRING_I },
		{ "regex", Pattern::REGEX },
		{ "regex_icase", Pattern::REGEX_I },
		{ "hash", Pattern::HASH },
	};

	auto ib(s2p.find(line));
	if ( ib == s2p.end() ) return Pattern::NONE;
	return ib->second;
}

static
inline
const char*
_Pattern2Str(StringFilter::Pattern ptn)
{
	using Pattern = StringFilter::Pattern;
	static const std::map<Pattern, const char*> p2s
	{
		{Pattern::SUBSTRING, "substr"},
		{Pattern::SUBSTRING_I, "substr_icase"},
		{Pattern::REGEX, "regex"},
		{Pattern::REGEX_I, "regex_icase"},
		{Pattern::HASH, "hash"}
	};

	auto ib(p2s.find(ptn));
	if ( ib == p2s.end() ) return "invalid";
	return ib->second;
}

namespace strfltr {

Pattern
toPattern ( const char* s )
{
	return _Str2Pattern(s);
}

Pattern
toPattern ( const std::string& str )
{
	return _Str2Pattern(str.c_str());
}

std::string
toString ( Pattern ptrn )
{
	return std::string(_Pattern2Str(ptrn));
}

const char*
toStringA ( Pattern ptrn )
{
	return _Pattern2Str(ptrn);
}

};

struct ctx_substr : public StringFilter::context_type
{
	inline virtual ~ctx_substr() = default;
	inline bool isMatched ( const std::string& key ) override { return (std::string::npos not_eq key.find(before_compiled)); }
	inline bool compile ( const std::string& comp ) override { if ( comp.empty() ) return false; const_cast<std::string&>(before_compiled) = comp; return true; }
	inline StringFilter::Pattern getPatternType ( void ) const override { return StringFilter::Pattern::SUBSTRING; }
};

struct ctx_substr_icase : public StringFilter::context_type
{
	inline virtual ~ctx_substr_icase() = default;
	inline bool isMatched ( const std::string& key ) override
	{
		auto ib(std::search(key.begin(), key.end(), before_compiled.begin(), before_compiled.end(),
							[] (char keych, char upperch) { return std::toupper(keych) == upperch; }
					 ));
		return ( ib not_eq key.end() );
	}

	inline bool compile ( const std::string& comp ) override
	{
		if ( comp.empty() ) return false;
		auto& data(const_cast<std::string&>(before_compiled));
		data.resize(comp.size());
		std::transform( comp.begin(), comp.end(), data.begin(), [] (char c) { return std::toupper(c); });
		return true;
	}

	inline StringFilter::Pattern getPatternType ( void ) const override { return StringFilter::Pattern::SUBSTRING_I; }
};

#if (PW_USE_STD_REGEX || PW_USE_BOOST_REGEX)
template<REGNS::regex_constants::syntax_option_type _RegexFlags, StringFilter::Pattern _PatternType>
struct ctx_regex_tpl : public StringFilter::context_type
{
	REGNS::regex reg;
	inline virtual ~ctx_regex_tpl() = default;
	inline bool isMatched ( const std::string& key ) override
	{
		return REGNS::regex_search(key, this->reg);
	}

	inline bool compile ( const std::string& comp ) override
	{
		try
		{
			reg = REGNS::regex( comp, _RegexFlags );
		}
		catch(std::exception& e)
		{
			return false;
		}

		const_cast<std::string&>(before_compiled) = comp;
		return true;
	}

	inline StringFilter::Pattern getPatternType ( void ) const override { return _PatternType; }
};

constexpr auto regex_flags(REGNS::regex_constants::optimize /*bitor REGNS::regex_constants::extended*/ bitor REGNS::regex_constants::nosubs bitor REGNS::regex_constants::ECMAScript);
constexpr auto regex_flags_icase(regex_flags bitor REGNS::regex_constants::icase);

using ctx_regex = ctx_regex_tpl<regex_flags, StringFilter::Pattern::REGEX>;
using ctx_regex_icase = ctx_regex_tpl<regex_flags_icase, StringFilter::Pattern::REGEX_I>;

#else
template<int _RegexFlags, StringFilter::Pattern _PatternType>
struct ctx_regex_tpl : public StringFilter::context_type
{
	regex_t* preg = nullptr;
	inline virtual ~ctx_regex_tpl()
	{
		if ( preg )
		{
			regfree(preg);
			delete preg;
		}
	}

	inline bool isMatched ( const std::string& key ) override
	{
		return (0 == regexec(preg, key.c_str(), 0, nullptr, 0));
	}

	inline bool compile ( const std::string& comp ) override
	{
		auto mypreg(new regex_t);
		if ( nullptr == mypreg ) return false;

		if ( 0 == regcomp(mypreg, comp.c_str(), _RegexFlags) )
		{
			//PWTRACE("compiled!");
			if ( preg )
			{
				regfree(preg);
				delete preg;
			}

			const_cast<std::string&>(before_compiled) = comp;
			preg = mypreg;
			return true;
		}

		delete mypreg;
		return false;
	}

	inline StringFilter::Pattern getPatternType ( void ) const override { return _PatternType; }
};

using ctx_regex = ctx_regex_tpl<REG_EXTENDED bitor REG_NOSUB, StringFilter::Pattern::REGEX>;
using ctx_regex_icase = ctx_regex_tpl<REG_EXTENDED bitor REG_NOSUB bitor REG_ICASE, StringFilter::Pattern::REGEX_I>;

#endif

inline
StringFilter::context_type*
_createContext(StringFilter::Pattern ptrn, const std::string& needle)
{
	using context_type = StringFilter::context_type;
	using Pattern = StringFilter::Pattern;

	context_type* pctx(nullptr);
	switch(ptrn)
	{
		case Pattern::SUBSTRING: pctx = new ctx_substr; break;
		case Pattern::SUBSTRING_I: pctx = new ctx_substr_icase; break;
		case Pattern::REGEX: pctx = new ctx_regex; break;
		case Pattern::REGEX_I: pctx = new ctx_regex_icase; break;
		case Pattern::HASH:
		case Pattern::NONE: return nullptr;
	}

	if (pctx->compile(needle)) return pctx;

	delete pctx;
	return nullptr;
}

StringFilter::~StringFilter()
{
	for ( auto& p : m_cont )
	{
		if ( p ) delete p;
	}
}

bool
StringFilter::writeToFile ( const char* file ) const
{
	std::ofstream ofs(file);
	if ( ofs.is_open() ) return writeToStream(ofs);
	return false;
}

bool
StringFilter::readFromFile ( const char* file )
{
	std::ifstream ifs(file);
	if ( ifs.is_open() ) return readFromStream(ifs);
	PWTRACE("no file to open: %s", file);
	return false;
}

bool
StringFilter::writeToStream ( ostream& os ) const
{
	for ( auto& p : m_cont )
	{
		if ( p ) os << _Pattern2Str(p->getPatternType()) << '>' << p->before_compiled << endl;
	}

	return true;
}

bool
StringFilter::readFromStream ( istream& is )
{
	decltype(m_cont) tmp;
	decltype(m_hash) tmp_hash;
	is.clear();

	std::string type_str;

	while ( not is.eof() )
	{
		std::string line;
		if ( PWStr::getLine(line, is).fail() )
		//if ( std::getline(is, line).fail() )
		{
			//PWTRACE("failed to get line");
			continue;
		}

		//PWTRACE("input line: %s", cstr(line));

		PWStr::trim(line);
		if ( line.empty() )
		{
			//PWTRACE("empty line");
			continue;
		}

		Tokenizer tok(line);
		const auto first_ch(line[0]);
		if ( (first_ch == '#') or (first_ch == '\'') or (first_ch == ';') or (first_ch == '`') ) continue;

		if ( not tok.getNext(type_str, '>') ) continue;

		if ( not tok.getLeftSize() ) continue;

		auto ptrn(_Str2Pattern(type_str));
		if ( ptrn == Pattern::HASH )
		{
			std::string hash(tok.getPosition(), tok.getLeftSize());
			PWEnc::decodeHex(hash);
			if ( hash.size() not_eq 32 ) continue;
			if ( not tmp_hash.insert(hash).second ) continue;
		}
		else
		{
			context_type* pctx(_createContext(ptrn, std::string(tok.getPosition(), tok.getLeftSize()) ));
			if ( nullptr == pctx )
			{
				PWTRACE("failed to create context: line: %s", line.c_str());
				continue;
			}

			tmp.push_back(pctx);
			//PWTRACE("pushed: %p", pctx);
		}
	}

	//tmp.sort([](const context_type* p1, const context_type* p2) { return *p1 < *p2; });

	m_cont.swap(tmp);
	m_hash.swap(tmp_hash);
	//PWTRACE("m_cont size: %zu", m_cont.size());

	return true;
}

bool
StringFilter::add ( StringFilter::Pattern ptrn, const string& needle )
{
	if ( ptrn == Pattern::HASH )
	{
		std::string hash;
		PWEnc::decodeHex(hash, needle);
		if ( hash.size() not_eq 32 ) return false;
		return m_hash.insert(hash).second;
	}

	auto pctx( _createContext(ptrn, needle) );
	if ( pctx )
	{
		m_cont.push_back(pctx);
		//m_cont.sort([](const context_type* p1, const context_type* p2) { return *p1 < *p2; });
		return true;
	}
	return false;
}

StringFilter::Pattern
StringFilter::check ( const string& str ) const
{
	for( auto& p : m_cont )
	{
		if ( p->isMatched(str) )
		{
			//PWTRACE("MATCHED! addr:%p type:%s orig:%s", this, typeid(*p).name(), p->before_compiled.c_str());
			return p->getPatternType();
		}
	}

	if ( not m_hash.empty() )
	{
		std::string c;
		Digest::s_execute(c, str, digest::DigestType::SHA256);

		auto ib(m_hash.find(c));
		if ( ib not_eq m_hash.end() ) return Pattern::HASH;
	}

	return Pattern::NONE;
}

std::pair< StringFilter::Pattern, const string >
StringFilter::check2 ( const string& str ) const
{
	for( auto& p : m_cont )
	{
		if ( p->isMatched(str) )
		{
			//PWTRACE("MATCHED! addr:%p type:%s orig:%s", this, typeid(*p).name(), p->before_compiled.c_str());
			return check_res_type(p->getPatternType(), p->before_compiled);
		}
	}

	if ( not m_hash.empty() )
	{
		std::string c;
		Digest::s_execute(c, str, digest::DigestType::SHA256);

		auto ib(m_hash.find(c));
		if ( ib not_eq m_hash.end() ) return check_res_type(Pattern::HASH, *ib);
	}

	return check_res_type(Pattern::NONE, std::string());
}


}
