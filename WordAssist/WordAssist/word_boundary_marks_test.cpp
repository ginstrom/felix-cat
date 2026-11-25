#include "stdafx.h"
#include "word_boundary_marks.h"

#include <boost/test/unit_test.hpp>
#ifdef UNIT_TEST

BOOST_AUTO_TEST_SUITE( test_word_boundary_marks )

	using std::wstring ;

	// paragraph mark, LF, end-of-cell, line break, page break, column break
	static const wstring CR( 1, L'\x0D' ) ;
	static const wstring LF( 1, L'\x0A' ) ;
	static const wstring BEL( 1, L'\x07' ) ;
	static const wstring VT( 1, L'\x0B' ) ;
	static const wstring FF( 1, L'\x0C' ) ;
	static const wstring SO( 1, L'\x0E' ) ;

	BOOST_AUTO_TEST_CASE( trailing_paragraph_mark )
	{
		BOOST_CHECK( trim_word_boundary_marks( L"blue lantern" + CR ) == L"blue lantern" ) ;
	}
	BOOST_AUTO_TEST_CASE( trailing_end_of_cell )
	{
		BOOST_CHECK( trim_word_boundary_marks( L"cell" + CR + BEL ) == L"cell" ) ;
	}
	BOOST_AUTO_TEST_CASE( leading_paragraph_mark )
	{
		BOOST_CHECK( trim_word_boundary_marks( CR + L"blue lantern" ) == L"blue lantern" ) ;
	}
	BOOST_AUTO_TEST_CASE( other_marks_at_the_ends )
	{
		BOOST_CHECK( trim_word_boundary_marks( L"x" + VT ) == L"x" ) ;
		BOOST_CHECK( trim_word_boundary_marks( L"x" + FF ) == L"x" ) ;
		BOOST_CHECK( trim_word_boundary_marks( L"x" + LF ) == L"x" ) ;
		BOOST_CHECK( trim_word_boundary_marks( L"x" + SO ) == L"x" ) ;
		BOOST_CHECK( trim_word_boundary_marks( VT + L"x" ) == L"x" ) ;
		BOOST_CHECK( trim_word_boundary_marks( FF + L"x" ) == L"x" ) ;
		BOOST_CHECK( trim_word_boundary_marks( LF + L"x" ) == L"x" ) ;
		BOOST_CHECK( trim_word_boundary_marks( SO + L"x" ) == L"x" ) ;
	}
	BOOST_AUTO_TEST_CASE( mark_in_the_middle_is_kept )
	{
		const wstring text = L"one" + CR + L"two" ;
		BOOST_CHECK( trim_word_boundary_marks( text ) == text ) ;
	}
	BOOST_AUTO_TEST_CASE( trailing_space_is_kept )
	{
		BOOST_CHECK( trim_word_boundary_marks( L"lantern " ) == L"lantern " ) ;
	}
	BOOST_AUTO_TEST_CASE( space_before_mark_is_kept )
	{
		BOOST_CHECK( trim_word_boundary_marks( L"lantern " + CR ) == L"lantern " ) ;
	}
	BOOST_AUTO_TEST_CASE( tab_and_nbsp_are_kept )
	{
		const wstring text = L"\tlantern\x00A0" ;
		BOOST_CHECK( trim_word_boundary_marks( text ) == text ) ;
	}
	BOOST_AUTO_TEST_CASE( mark_inside_closing_tag )
	{
		BOOST_CHECK( trim_word_boundary_marks( L"<font face=\"Calibri\">blue lantern" + CR + L"</font>" )
			== L"<font face=\"Calibri\">blue lantern</font>" ) ;
	}
	BOOST_AUTO_TEST_CASE( mark_after_closing_tag )
	{
		BOOST_CHECK( trim_word_boundary_marks( L"<b>x</b>" + CR ) == L"<b>x</b>" ) ;
	}
	BOOST_AUTO_TEST_CASE( mark_only_in_tag_leaves_empty_tag_pair )
	{
		BOOST_CHECK( trim_word_boundary_marks( L"x<b>" + CR + L"</b>" ) == L"x<b></b>" ) ;
	}
	BOOST_AUTO_TEST_CASE( leading_mark_before_tag )
	{
		BOOST_CHECK( trim_word_boundary_marks( CR + L"<b>x</b>" ) == L"<b>x</b>" ) ;
	}
	BOOST_AUTO_TEST_CASE( leading_mark_inside_tag )
	{
		BOOST_CHECK( trim_word_boundary_marks( L"<b>" + CR + L"x</b>" ) == L"<b>x</b>" ) ;
	}
	BOOST_AUTO_TEST_CASE( lone_mark_gives_empty )
	{
		BOOST_CHECK( trim_word_boundary_marks( CR ).empty() ) ;
	}
	BOOST_AUTO_TEST_CASE( empty_gives_empty )
	{
		BOOST_CHECK( trim_word_boundary_marks( L"" ).empty() ) ;
	}
	BOOST_AUTO_TEST_CASE( only_marks_gives_empty )
	{
		BOOST_CHECK( trim_word_boundary_marks( CR + BEL + VT ).empty() ) ;
	}
	BOOST_AUTO_TEST_CASE( only_tags_and_marks_keeps_tags )
	{
		BOOST_CHECK( trim_word_boundary_marks( L"<b>" + CR + L"</b>" ) == L"<b></b>" ) ;
		BOOST_CHECK( trim_word_boundary_marks( CR + L"<b>" + CR + L"</b>" + CR ) == L"<b></b>" ) ;
	}
	BOOST_AUTO_TEST_CASE( text_without_marks_unchanged )
	{
		const wstring text = L"a &lt;b&gt; &amp; <i>c</i> d" ;
		BOOST_CHECK( trim_word_boundary_marks( text ) == text ) ;
	}
	BOOST_AUTO_TEST_CASE( stray_greater_than_without_open_does_not_throw )
	{
		BOOST_CHECK_NO_THROW( trim_word_boundary_marks( L"x>" + CR ) ) ;
		BOOST_CHECK_NO_THROW( trim_word_boundary_marks( L">" ) ) ;
		BOOST_CHECK_NO_THROW( trim_word_boundary_marks( L"<" + CR ) ) ;
	}

	BOOST_AUTO_TEST_CASE( has_text_outside_tags_plain_text )
	{
		BOOST_CHECK( has_text_outside_tags( L"x" ) ) ;
		BOOST_CHECK( has_text_outside_tags( L" " ) ) ;
		BOOST_CHECK( ! has_text_outside_tags( L"" ) ) ;
	}
	BOOST_AUTO_TEST_CASE( has_text_outside_tags_only_tags )
	{
		BOOST_CHECK( ! has_text_outside_tags( L"<FONT FACE=\"x\"></FONT>" ) ) ;
		BOOST_CHECK( ! has_text_outside_tags( L"<b><i></i></b>" ) ) ;
	}
	BOOST_AUTO_TEST_CASE( has_text_outside_tags_text_between_tags )
	{
		BOOST_CHECK( has_text_outside_tags( L"<b><i>x</i></b>" ) ) ;
		BOOST_CHECK( has_text_outside_tags( L"<b>&lt;</b>" ) ) ;
		BOOST_CHECK( has_text_outside_tags( L"<b> </b>" ) ) ;
	}
	BOOST_AUTO_TEST_CASE( concordance_query_empty_paragraph_in_font_tag )
	{
		BOOST_CHECK( trim_concordance_query( L"<FONT FACE=\"x\">" + CR + L"</FONT>" ).empty() ) ;
		BOOST_CHECK( trim_concordance_query( L"<b><i>" + CR + BEL + L"</i></b>" ).empty() ) ;
		BOOST_CHECK( trim_concordance_query( CR ).empty() ) ;
	}
	BOOST_AUTO_TEST_CASE( concordance_query_keeps_text_and_tags )
	{
		BOOST_CHECK( trim_concordance_query( L"<FONT FACE=\"x\">blue lantern" + CR + L"</FONT>" )
			== L"<FONT FACE=\"x\">blue lantern</FONT>" ) ;
		BOOST_CHECK( trim_concordance_query( L"<b>&lt;" + CR + L"</b>" ) == L"<b>&lt;</b>" ) ;
		BOOST_CHECK( trim_concordance_query( L"lantern " + CR ) == L"lantern " ) ;
	}

BOOST_AUTO_TEST_SUITE_END()

#endif // #ifdef UNIT_TEST
