/*!
	@brief Small Felix data files that unit tests write to a TestTempDir.

	These replace files that used to live on the original development
	machine (C:\test\test_memories\one_record.xml and so on).
 */
#pragma once

#ifdef UNIT_TEST

#include "test_temp_dir.h"

namespace test_data
{
	//! A Felix memory (UTF-8 XML) with a single record: "foo" -> "bar".
	inline const char *one_record_memory_xml()
	{
		return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<!DOCTYPE memory >\n"
			"<!-- Created by Felix v 1.0 -->\n"
			"<memory>\n"
			"<head>\n"
			"<creator>RyanVista</creator>\n"
			"<created_on>2008/08/15 12:04:14</created_on>\n"
			"<creation_tool>Felix</creation_tool>\n"
			"<creation_tool_version>1.0</creation_tool_version>\n"
			"<num_records>1</num_records>\n"
			"<locked>false</locked>\n"
			"<is_memory>true</is_memory>\n"
			"</head>\n"
			"<records><record>  <source><![CDATA[foo]]></source>\n"
			"<trans><![CDATA[bar]]></trans>\n"
			"<date_created>2008/08/15 12:04:14</date_created>\n"
			"<last_modified>2008/08/15 12:04:15</last_modified>\n"
			"<reliability>0</reliability>\n"
			"<validated>false</validated>\n"
			"<ref_count>0</ref_count>\n"
			"</record>\n"
			"</records></memory>\n" ;
	}

	//! Writes one_record_memory_xml() to the folder and returns its path.
	inline CString write_one_record_memory(const TestTempDir &temp)
	{
		const char *xml = one_record_memory_xml() ;
		return temp.write_file(_T("one_record.xml"), xml, strlen(xml)) ;
	}

	//! A few lines of Japanese, long enough for MLang to detect the encoding.
	inline std::wstring japanese_sample_text()
	{
		std::wstring line = L"\x65E5\x672C\x8A9E\x306E\x6587\x7AE0\x3067\x3059\x3002"   // 日本語の文章です。
			L"\x7FFB\x8A33\x30E1\x30E2\x30EA\x306E\x30C6\x30B9\x30C8\x3002\r\n" ;            // 翻訳メモリのテスト。
		std::wstring text ;
		for (int i = 0 ; i < 20 ; ++i)
		{
			text += line ;
		}
		return text ;
	}
}

#endif // UNIT_TEST
