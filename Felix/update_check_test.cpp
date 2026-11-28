#include "StdAfx.h"
#include "update_check.h"

#include <boost/test/unit_test.hpp>
#ifdef UNIT_TEST

using namespace updates ;

BOOST_AUTO_TEST_SUITE( TestUpdateCheck )

	// parse_version
	BOOST_AUTO_TEST_CASE( parse_version_with_v_prefix )
	{
		std::vector<int> v = parse_version("v3.0.1") ;
		BOOST_REQUIRE_EQUAL( v.size(), 3u ) ;
		BOOST_CHECK_EQUAL( v[0], 3 ) ;
		BOOST_CHECK_EQUAL( v[1], 0 ) ;
		BOOST_CHECK_EQUAL( v[2], 1 ) ;
	}
	BOOST_AUTO_TEST_CASE( parse_version_without_prefix )
	{
		std::vector<int> v = parse_version("3.10") ;
		BOOST_REQUIRE_EQUAL( v.size(), 2u ) ;
		BOOST_CHECK_EQUAL( v[1], 10 ) ;
	}
	BOOST_AUTO_TEST_CASE( parse_version_rejects_text )
	{
		BOOST_CHECK( parse_version("").empty() ) ;
		BOOST_CHECK( parse_version("latest").empty() ) ;
		BOOST_CHECK( parse_version("v3.0-beta").empty() ) ;
		BOOST_CHECK( parse_version("3..0").empty() ) ;
		BOOST_CHECK( parse_version("3.").empty() ) ;
	}

	// is_newer
	BOOST_AUTO_TEST_CASE( is_newer_higher_patch )
	{
		BOOST_CHECK( is_newer("v3.0.2", "3.0.1") ) ;
	}
	BOOST_AUTO_TEST_CASE( is_newer_same_version )
	{
		BOOST_CHECK( ! is_newer("v3.0.1", "3.0.1") ) ;
	}
	BOOST_AUTO_TEST_CASE( is_newer_missing_parts_are_zero )
	{
		BOOST_CHECK( ! is_newer("v3.0", "3.0.0") ) ;
		BOOST_CHECK( ! is_newer("v3.0", "3.0.1") ) ;
		BOOST_CHECK( is_newer("v3.0.1", "3.0") ) ;
	}
	BOOST_AUTO_TEST_CASE( is_newer_compares_numerically )
	{
		BOOST_CHECK( is_newer("v3.10", "3.9") ) ;
		BOOST_CHECK( ! is_newer("v2.9.9", "3.0") ) ;
		BOOST_CHECK( is_newer("v4", "3.9.9") ) ;
	}
	BOOST_AUTO_TEST_CASE( is_newer_unparseable_is_never_newer )
	{
		BOOST_CHECK( ! is_newer("nightly", "3.0.1") ) ;
		BOOST_CHECK( ! is_newer("v9.0", "dev") ) ;
	}
	BOOST_AUTO_TEST_CASE( is_newer_against_build_version )
	{
		// The running build's own version must parse.
		BOOST_CHECK( ! parse_version(VERSION).empty() ) ;
		BOOST_CHECK( ! is_newer(std::string("v") + VERSION, VERSION) ) ;
	}

	// parse_release_tag
	BOOST_AUTO_TEST_CASE( parse_release_tag_from_github_json )
	{
		std::string json =
			"{\"url\":\"https://api.github.com/repos/ginstrom/felix-cat/releases/1\","
			"\"html_url\":\"https://github.com/ginstrom/felix-cat/releases/tag/v3.0\","
			"\"id\":1,\"tag_name\": \"v3.0\",\"name\":\"Felix 3.0\","
			"\"author\":{\"html_url\":\"https://github.com/ginstrom\"}}" ;
		std::string tag ;
		BOOST_REQUIRE( parse_release_tag(json, tag) ) ;
		BOOST_CHECK_EQUAL( tag, "v3.0" ) ;
	}
	BOOST_AUTO_TEST_CASE( parse_release_tag_missing )
	{
		std::string tag ;
		BOOST_CHECK( ! parse_release_tag("{\"message\":\"Not Found\"}", tag) ) ;
		BOOST_CHECK( ! parse_release_tag("", tag) ) ;
		BOOST_CHECK( ! parse_release_tag("{\"tag_name\":\"\"}", tag) ) ;
	}

	BOOST_AUTO_TEST_CASE( release_page_url_for_tag )
	{
		std::wstring url = release_page_url("v3.0.2") ;
		BOOST_CHECK( url == L"https://github.com/ginstrom/felix-cat/releases/tag/v3.0.2" ) ;
	}

	// decide_auto_check
	BOOST_AUTO_TEST_CASE( decide_asks_when_never_asked )
	{
		BOOST_CHECK_EQUAL( decide_auto_check(false, false, 0, 1000), AUTO_CHECK_ASK ) ;
		BOOST_CHECK_EQUAL( decide_auto_check(false, true, 999, 1000), AUTO_CHECK_ASK ) ;
	}
	BOOST_AUTO_TEST_CASE( decide_nothing_when_disabled )
	{
		BOOST_CHECK_EQUAL( decide_auto_check(true, false, 0, 1000), AUTO_CHECK_NONE ) ;
		BOOST_CHECK_EQUAL( decide_auto_check(true, false, 1, 1000), AUTO_CHECK_NONE ) ;
	}
	BOOST_AUTO_TEST_CASE( decide_checks_when_never_checked )
	{
		BOOST_CHECK_EQUAL( decide_auto_check(true, true, 0, 1000), AUTO_CHECK_NOW ) ;
	}
	BOOST_AUTO_TEST_CASE( decide_interval )
	{
		BOOST_CHECK_EQUAL( decide_auto_check(true, true, 1000, 1000), AUTO_CHECK_NONE ) ;
		BOOST_CHECK_EQUAL( decide_auto_check(true, true, 1000, 1013), AUTO_CHECK_NONE ) ;
		BOOST_CHECK_EQUAL( decide_auto_check(true, true, 1000, 1014), AUTO_CHECK_NOW ) ;
		BOOST_CHECK_EQUAL( decide_auto_check(true, true, 1000, 2000), AUTO_CHECK_NOW ) ;
	}
	BOOST_AUTO_TEST_CASE( decide_checks_when_clock_went_back )
	{
		BOOST_CHECK_EQUAL( decide_auto_check(true, true, 1000, 900), AUTO_CHECK_NOW ) ;
	}

	// day_number
	BOOST_AUTO_TEST_CASE( day_number_ignores_time_of_day )
	{
		SYSTEMTIME morning = {} ;
		morning.wYear = 2026 ; morning.wMonth = 11 ; morning.wDay = 28 ;
		morning.wHour = 1 ;
		SYSTEMTIME evening = morning ;
		evening.wHour = 23 ; evening.wMinute = 59 ;
		BOOST_CHECK_EQUAL( day_number(morning), day_number(evening) ) ;
	}
	BOOST_AUTO_TEST_CASE( day_number_counts_days_across_months )
	{
		SYSTEMTIME a = {} ;
		a.wYear = 2026 ; a.wMonth = 11 ; a.wDay = 28 ;
		SYSTEMTIME b = {} ;
		b.wYear = 2026 ; b.wMonth = 12 ; b.wDay = 12 ;
		BOOST_CHECK_EQUAL( day_number(b) - day_number(a), 14 ) ;
		BOOST_CHECK( day_number(a) > 0 ) ;
	}

BOOST_AUTO_TEST_SUITE_END()

#endif
