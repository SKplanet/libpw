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
 * \file pw_region.h
 * \brief Support country code of phone.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_region.h"
#include "./pw_ini.h"
#include "./pw_encode.h"
#include "./pw_log.h"

const pw::Region& PWRegion(pw::Region::s_getInstance());

namespace pw {

bool
Region::_insert(const std::string& full_name, const char* code, const char* phone)
{
	do {
		std::pair<code_cont::iterator,bool> res(m_codes.insert(code));
		code = res.first->c_str();
	} while (false);

	do {
		std::pair<phone_cont::iterator,bool> res(m_phones.insert(phone));
		phone = res.first->c_str();
	} while (false);

	m_region.push_back(region_type(full_name, code, phone));
	const region_type& region(m_region.back());

	do {
		if ( not m_code_index.insert(code_index_cont::value_type(code, &region)).second ) break;
		m_phone_index.insert(phone_index_cont::value_type(phone, &region));

		return true;
	} while (false);

	m_code_index.erase(code);
	m_region.pop_back();

	return false;
}

void
Region::clear(void)
{
	m_region.clear();
	m_codes.clear();
	m_phones.clear();
	m_code_index.clear();
	m_phone_index.clear();
}

void
Region::swap(Region& v)
{
	m_region.swap(v.m_region);
	m_codes.swap(v.m_codes);
	m_phones.swap(v.m_phones);
	m_code_index.swap(v.m_code_index);
	m_phone_index.swap(v.m_phone_index);
}

const Region::region_type*
Region::findByCode(const code_type& code) const
{
	code_index_cont::const_iterator ib(m_code_index.find(code));
	if ( m_code_index.end() == ib ) return nullptr;
	return ib->second;
}

Region::region_res_list&
Region::findByPhone(region_res_list& out, const phone_type& _phone) const
{
	const char* phone(_phone.c_str() +  ((_phone[0] == '+') ? 1:0));
	size_t plen(strlen(phone));
	phone_type sub(phone);
	std::pair<phone_index_cont::const_iterator, phone_index_cont::const_iterator> ib;
	do {
		ib = m_phone_index.equal_range(sub);
		if ( ib.first not_eq ib.second )
		{
			phone_index_cont::const_iterator iib(ib.first);
			phone_index_cont::const_iterator iie(ib.second);
			region_res_list tmp;

			while ( iib not_eq iie )
			{
				tmp.push_back(iib->second);
				++iib;
			}

			out.swap(tmp);
			return out;
		}

		if ( (--plen) == 0 ) break;
		sub.truncate(plen);
	} while (true);

	out.clear();
	return out;
}

bool
Region::read(const Ini& ini, const std::string& sec)
{
	Ini::sec_citr isec(ini.cfind(sec));
	if ( isec == ini.cend() )
	{
		PWTRACE("no section: %s", sec.c_str());
		return false;
	}

	const intmax_t count(ini.getInteger("region.count", isec, 0));
	if ( 0 == count )
	{
		PWTRACE("no region.count tag or zero");
		return false;
	}

	Region tmp;
	std::string iname, ivalue;

	std::string full_name, code, phone;

	for ( intmax_t i(0); i<count; i++ )
	{
		PWStr::format(iname, "region.%ju", i);
		ini.getString2(ivalue, iname, isec);
		if ( ivalue.empty() )
		{
			PWTRACE("tag %s is empty line", iname.c_str());
			return false;
		}

		Tokenizer tok(ivalue);
		if ( not tok.getNext(full_name, ' ') )
		{
			PWTRACE("failed to get full_name");
			return false;
		}
		PWTRACE("full_name: %s", full_name.c_str());

		if ( not tok.getNext(code, ' ') )
		{
			PWTRACE("failed to get code");
			return false;
		}
		PWTRACE("code: %s", code.c_str());

		if ( not tok.getNext(phone, ' ') )
		{
			PWTRACE("failed to get phone");
			return false;
		}
		PWTRACE("phone: %s", phone.c_str());

		PWEnc::decodeURL(full_name);
		PWEnc::decodeURL(code);
		PWEnc::decodeURL(phone);

		if ( not tmp._insert(full_name, code.c_str(), phone.c_str()) ) return false;
	}

	swap(tmp);
	return true;
}

bool
Region::write(Ini& ini, const std::string& sec) const
{
	region_citr ib(m_region.begin());
	region_citr ie(m_region.end());

	std::string iname, ivalue;
	std::string efull_name, ecode, ephone;
	size_t i(0);

	ini.setInteger(m_region.size(), "region.count", sec);

	while ( ib not_eq ie )
	{
		PWStr::format(iname, "region.%zu", i);
		PWEnc::encodeURL(efull_name, ib->full_name);
		PWEnc::encodeURL(ecode, ib->code);
		PWEnc::encodeURL(ephone, ib->phone);
		PWStr::format(ivalue, "%s %s %s", efull_name.c_str(), ecode.c_str(), ephone.c_str());

		ini.setString(ivalue, iname, sec);
		++ib;
		++i;
	}

	return true;
}

bool
Region::s_initialize(void)
{
	Region& inst(const_cast<Region&>(Region::s_getInstance()));
	if ( not inst.m_region.empty() ) return true;

	//anybody chat
	inst._insert("Anybody Nederland", "NQ", "9999");
	inst._insert("Anybody Korea", "KQ", "9998");

	inst._insert("Afghanistan", "AF", "93");
	inst._insert("Albania", "AL", "355");
	inst._insert("Algeria", "DZ", "213");
	inst._insert("American Samoa", "AS", "1684");
	inst._insert("Andorra", "AD", "376");
	inst._insert("Angola", "AO", "244");
	inst._insert("Anguilla", "AI", "1264");
	inst._insert("Antarctica", "AQ", "672");
	inst._insert("Antigua and Barbuda", "AG", "1268");
	inst._insert("Argentina", "AR", "54");
	inst._insert("Armenia", "AM", "374");
	inst._insert("Aruba", "AW", "297");
	inst._insert("Australia", "AU", "61");
	inst._insert("Austria", "AT", "43");
	inst._insert("Azerbaijan", "AZ", "994");
	inst._insert("Bahamas", "BS", "1242");
	inst._insert("Bahrain", "BH", "973");
	inst._insert("Bangladesh", "BD", "880");
	inst._insert("Barbados", "BB", "1246");
	inst._insert("Belarus", "BY", "375");
	inst._insert("Belgium", "BE", "32");
	inst._insert("Belize", "BZ", "501");
	inst._insert("Benin", "BJ", "229");
 	inst._insert("Bermuda", "BM", "1441");
	inst._insert("Bhutan", "BT", "975");
	inst._insert("Bolivia", "BO", "591");
	inst._insert("Bosnia and Herzegovina", "BA", "387");
	inst._insert("Botswana", "BW", "267");
	inst._insert("Brazil", "BR", "55");
	inst._insert("British Indian Ocean Territory", "IO", "");
	inst._insert("British Virgin Islands", "VG", "1284");
	inst._insert("Brunei", "BN", "673");
	inst._insert("Bulgaria", "BG", "359");
	inst._insert("Burkina Faso", "BF", "226");
	inst._insert("Burma", "MM", "95");
	inst._insert("Burundi", "BI", "257");
	inst._insert("Cambodia", "KH", "855");
	inst._insert("Cameroon", "CM", "237");
	inst._insert("Canada", "CA", "1");
	inst._insert("Cape Verde", "CV", "238");
	inst._insert("Cayman Islands", "KY", "1345");
	inst._insert("Central African Republic", "CF", "236");
	inst._insert("Chad", "TD", "235");
	inst._insert("Chile", "CL", "56");
	inst._insert("China", "CN", "86");
	inst._insert("Christmas Island", "CX", "61");
	inst._insert("Cocos (Keeling) Islands", "CC", "61");
	inst._insert("Colombia", "CO", "57");
	inst._insert("Comoros", "KM", "269");
	inst._insert("Cook Islands", "CK", "682");
	inst._insert("Costa Rica", "CR", "506");
	inst._insert("Croatia", "HR", "385");
	inst._insert("Cuba", "CU", "53");
	inst._insert("Cyprus", "CY", "357");
	inst._insert("Czech Republic", "CZ", "420");
	inst._insert("Democratic Republic of the Congo", "CD", "243");
	inst._insert("Denmark", "DK", "45");
	inst._insert("Djibouti", "DJ", "253");
	inst._insert("Dominica", "DM", "1767");
	inst._insert("Dominican Republic", "DO", "1809");
	inst._insert("Ecuador", "EC", "593");
	inst._insert("Egypt", "EG", "20");
	inst._insert("El Salvador", "SV", "503");
	inst._insert("Equatorial Guinea", "GQ", "240");
	inst._insert("Eritrea", "ER", "291");
	inst._insert("Estonia", "EE", "372");
	inst._insert("Ethiopia", "ET", "251");
	inst._insert("Falkland Islands", "FK", "500");
	inst._insert("Faroe Islands", "FO", "298");
	inst._insert("Fiji", "FJ", "679");
	inst._insert("Finland", "FI", "358");
	inst._insert("France", "FR", "33");
	inst._insert("French Polynesia", "PF", "689");
	inst._insert("Gabon", "GA", "241");
	inst._insert("Gambia", "GM", "220");
	inst._insert("Gaza Strip", "/  ", "970");
	inst._insert("Georgia", "GE", "995");
	inst._insert("Germany", "DE", "49");
	inst._insert("Ghana", "GH", "233");
	inst._insert("Gibraltar", "GI", "350");
	inst._insert("Greece", "GR", "30");
	inst._insert("Greenland", "GL", "299");
	inst._insert("Grenada", "GD", "1473");
	inst._insert("Guam", "GU", "1671");
	inst._insert("Guatemala", "GT", "502");
	inst._insert("Guinea", "GN", "224");
	inst._insert("Guinea-Bissau", "GW", "245");
	inst._insert("Guyana", "GY", "592");
	inst._insert("Haiti", "HT", "509");
	inst._insert("Holy See (Vatican City)", "VA", "39");
	inst._insert("Honduras", "HN", "504");
	inst._insert("Hong Kong", "HK", "852");
	inst._insert("Hungary", "HU", "36");
	inst._insert("Iceland", "IS", "354");
	inst._insert("India", "IN", "91");
	inst._insert("Indonesia", "ID", "62");
	inst._insert("Iran", "IR", "98");
	inst._insert("Iraq", "IQ", "964");
	inst._insert("Ireland", "IE", "353");
	inst._insert("Isle of Man", "IM", "44");
	inst._insert("Israel", "IL", "972");
	inst._insert("Italy", "IT", "39");
	inst._insert("Ivory Coast", "CI", "225");
	inst._insert("Jamaica", "JM", "1876");
	inst._insert("Japan", "JP", "81");
	inst._insert("Jersey", "JE", "");
	inst._insert("Jordan", "JO", "962");
	inst._insert("Kazakhstan", "KZ", "7");
	inst._insert("Kenya", "KE", "254");
	inst._insert("Kiribati", "KI", "686");
	inst._insert("Kosovo", "/  ", "381");
	inst._insert("Kuwait", "KW", "965");
	inst._insert("Kyrgyzstan", "KG", "996");
	inst._insert("Laos", "LA", "856");
	inst._insert("Latvia", "LV", "371");
	inst._insert("Lebanon", "LB", "961");
	inst._insert("Lesotho", "LS", "266");
	inst._insert("Liberia", "LR", "231");
	inst._insert("Libya", "LY", "218");
	inst._insert("Liechtenstein", "LI", "423");
	inst._insert("Lithuania", "LT", "370");
	inst._insert("Luxembourg", "LU", "352");
	inst._insert("Macau", "MO", "853");
	inst._insert("Macedonia", "MK", "389");
	inst._insert("Madagascar", "MG", "261");
	inst._insert("Malawi", "MW", "265");
	inst._insert("Malaysia", "MY", "60");
	inst._insert("Maldives", "MV", "960");
	inst._insert("Mali", "ML", "223");
	inst._insert("Malta", "MT", "356");
	inst._insert("Marshall Islands", "MH", "692");
	inst._insert("Mauritania", "MR", "222");
	inst._insert("Mauritius", "MU", "230");
	inst._insert("Mayotte", "YT", "262");
	inst._insert("Mexico", "MX", "52");
	inst._insert("Micronesia", "FM", "691");
	inst._insert("Moldova", "MD", "373");
	inst._insert("Monaco", "MC", "377");
	inst._insert("Mongolia", "MN", "976");
	inst._insert("Montenegro", "ME", "382");
	inst._insert("Montserrat", "MS", "1664");
	inst._insert("Morocco", "MA", "212");
	inst._insert("Mozambique", "MZ", "258");
	inst._insert("Namibia", "NA", "264");
	inst._insert("Nauru", "NR", "674");
	inst._insert("Nepal", "NP", "977");
	inst._insert("Netherlands", "NL", "31");
	inst._insert("Netherlands Antilles", "AN", "599");
	inst._insert("New Caledonia", "NC", "687");
	inst._insert("New Zealand", "NZ", "64");
	inst._insert("Nicaragua", "NI", "505");
	inst._insert("Niger", "NE", "227");
	inst._insert("Nigeria", "NG", "234");
	inst._insert("Niue", "NU", "683");
	inst._insert("Norfolk Island", "", "672");
	inst._insert("North Korea", "KP", "850");
	inst._insert("Northern Mariana Islands", "MP", "1670");
	inst._insert("Norway", "NO", "47");
	inst._insert("Oman", "OM", "968");
	inst._insert("Pakistan", "PK", "92");
	inst._insert("Palau", "PW", "680");
	inst._insert("Panama", "PA", "507");
	inst._insert("Papua New Guinea", "PG", "675");
	inst._insert("Paraguay", "PY", "595");
	inst._insert("Peru", "PE", "51");
	inst._insert("Philippines", "PH", "63");
	inst._insert("Pitcairn Islands", "PN", "870");
	inst._insert("Poland", "PL", "48");
	inst._insert("Portugal", "PT", "351");
	inst._insert("Puerto Rico", "PR", "1");
	inst._insert("Qatar", "QA", "974");
	inst._insert("Republic of the Congo", "CG", "242");
	inst._insert("Romania", "RO", "40");
	inst._insert("Russia", "RU", "7");
	inst._insert("Rwanda", "RW", "250");
	inst._insert("Saint Barthelemy", "BL", "590");
	inst._insert("Saint Helena", "SH", "290");
	inst._insert("Saint Kitts and Nevis", "KN", "1869");
	inst._insert("Saint Lucia", "LC", "1758");
	inst._insert("Saint Martin", "MF", "1599");
	inst._insert("Saint Pierre and Miquelon", "PM", "508");
	inst._insert("Saint Vincent and the Grenadines", "VC", "1784");
	inst._insert("Samoa", "WS", "685");
	inst._insert("San Marino", "SM", "378");
	inst._insert("Sao Tome and Principe", "ST", "239");
	inst._insert("Saudi Arabia", "SA", "966");
	inst._insert("Senegal", "SN", "221");
	inst._insert("Serbia", "RS", "381");
	inst._insert("Seychelles", "SC", "248");
	inst._insert("Sierra Leone", "SL", "232");
	inst._insert("Singapore", "SG", "65");
	inst._insert("Slovakia", "SK", "421");
	inst._insert("Slovenia", "SI", "386");
	inst._insert("Solomon Islands", "SB", "677");
	inst._insert("Somalia", "SO", "252");
	inst._insert("South Africa", "ZA", "27");
	inst._insert("South Korea", "KR", "82");
	inst._insert("Spain", "ES", "34");
	inst._insert("Sri Lanka", "LK", "94");
	inst._insert("Sudan", "SD", "249");
	inst._insert("Suriname", "SR", "597");
	inst._insert("Svalbard", "SJ", "");
	inst._insert("Swaziland", "SZ", "268");
	inst._insert("Sweden", "SE", "46");
	inst._insert("Switzerland", "CH", "41");
	inst._insert("Syria", "SY", "963");
	inst._insert("Taiwan", "TW", "886");
	inst._insert("Tajikistan", "TJ", "992");
	inst._insert("Tanzania", "TZ", "255");
	inst._insert("Thailand", "TH", "66");
	inst._insert("Timor-Leste", "TL", "670");
	inst._insert("Togo", "TG", "228");
	inst._insert("Tokelau", "TK", "690");
	inst._insert("Tonga", "TO", "676");
	inst._insert("Trinidad and Tobago", "TT", "1868");
	inst._insert("Tunisia", "TN", "216");
	inst._insert("Turkey", "TR", "90");
	inst._insert("Turkmenistan", "TM", "993");
	inst._insert("Turks and Caicos Islands", "TC", "1649");
	inst._insert("Tuvalu", "TV", "688");
	inst._insert("Uganda", "UG", "256");
	inst._insert("Ukraine", "UA", "380");
	inst._insert("United Arab Emirates", "AE", "971");
	inst._insert("United Kingdom", "GB", "44");
	inst._insert("United States", "US", "1");
	inst._insert("Uruguay", "UY", "598");
	inst._insert("US Virgin Islands", "VI", "1340");
	inst._insert("Uzbekistan", "UZ", "998");
	inst._insert("Vanuatu", "VU", "678");
	inst._insert("Venezuela", "VE", "58");
	inst._insert("Vietnam", "VN", "84");
	inst._insert("Wallis and Futuna", "WF", "681");
	inst._insert("West Bank", "", "970");
	inst._insert("Western Sahara", "EH", "");
	inst._insert("Yemen", "YE", "967");
	inst._insert("Zambia", "ZM", "260");
	inst._insert("Zimbabwe", "ZW", "263");

	//PWTRACE("Region is initialized");

	return true;
}

std::ostream&
operator << (std::ostream& os, const Region::region_res_list& ls)
{
	os << "type: Region::region_res_list, addr: " << (void*)&ls
		<< ", count: " << ls.size()
		<< std::endl;
	Region::region_res_list::const_iterator ib(ls.begin());
	Region::region_res_list::const_iterator ie(ls.end());

	while ( ib not_eq ie )
	{
		os << '\t';
		(*ib)->dump(os);
		++ib;
	}

	return os;
}

}; //namespace pw
