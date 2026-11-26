#include "stdafx.h"
#include "word_boundary_marks.h"

#ifdef UNIT_TEST

#include <boost/test/unit_test.hpp>

// PowerPoint concordance queries are plain text: paragraph marks and line
// breaks are dropped from the ends, and nothing is treated as a tag.
BOOST_AUTO_TEST_SUITE( TrimPlainConcordanceQueryTest )

	BOOST_AUTO_TEST_CASE( drops_trailing_paragraph_mark )
	{
		BOOST_CHECK( trim_plain_concordance_query( L"spam\r" ) == L"spam" ) ;
	}

	BOOST_AUTO_TEST_CASE( drops_marks_at_both_ends )
	{
		BOOST_CHECK( trim_plain_concordance_query( L"\r\x0Bspam eggs\x0B\r" ) == L"spam eggs" ) ;
	}

	BOOST_AUTO_TEST_CASE( keeps_inner_marks_and_spaces )
	{
		BOOST_CHECK( trim_plain_concordance_query( L" spam\reggs " ) == L" spam\reggs " ) ;
	}

	BOOST_AUTO_TEST_CASE( only_marks_becomes_empty )
	{
		BOOST_CHECK( trim_plain_concordance_query( L"\r" ).empty() ) ;
		BOOST_CHECK( trim_plain_concordance_query( L"" ).empty() ) ;
	}

	BOOST_AUTO_TEST_CASE( tag_like_text_is_kept )
	{
		BOOST_CHECK( trim_plain_concordance_query( L"<b>\r" ) == L"<b>" ) ;
	}

BOOST_AUTO_TEST_SUITE_END()

#endif
