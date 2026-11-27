#include "stdafx.h"
#include "WordAutomationObject.h"

#include <boost/test/unit_test.hpp>

#ifdef UNIT_TEST


BOOST_AUTO_TEST_SUITE( test_symbol2unicode )

	BOOST_AUTO_TEST_CASE( test_0x22 )
	{
		_variant_t symbol(0x22 - 0x1000) ;

		wchar_t expected = static_cast<wchar_t>(0x2200) ;
		wchar_t actual = symbol2unicode(symbol) ;
		TRACE(actual) ;
		BOOST_TEST_MESSAGE("Testing symbol2unicode") ;
		BOOST_CHECK_EQUAL(expected, actual) ;
	}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE( WordFontTest )

	BOOST_AUTO_TEST_CASE( Instantiate )
	{
		BOOST_CHECK_NO_THROW(WordFont()) ;
	}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( WordRangeTest )

	BOOST_AUTO_TEST_CASE( Instantiate )
	{
		BOOST_CHECK_NO_THROW(WordRange()) ;
	}


BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( WordWordsTest )

	BOOST_AUTO_TEST_CASE( Instantiate )
	{
		BOOST_CHECK_NO_THROW(WordWords()) ;
	}


BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( WordCharactersTest )

	BOOST_AUTO_TEST_CASE( Instantiate )
	{
		BOOST_CHECK_NO_THROW(WordCharacters()) ;
	}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( WordSelectionTest )

	BOOST_AUTO_TEST_CASE( Instantiate )
	{
		BOOST_CHECK_NO_THROW(WordSelection()) ;
	}


BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( WordBookmarkTest )

	BOOST_AUTO_TEST_CASE( Instantiate )
	{
		BOOST_CHECK_NO_THROW(WordBookmark()) ;
	}


BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( WordBookmarksTest )

	BOOST_AUTO_TEST_CASE( Instantiate )
	{
		BOOST_CHECK_NO_THROW(WordBookmarks()) ;
	}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( WordDocumentTest )

	BOOST_AUTO_TEST_CASE( Instantiate )
	{
		BOOST_CHECK_NO_THROW(WordDocument()) ;
	}


BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( WordApplicationTest )

	// Hook-time probe must not assert or throw when there is no Word object
	BOOST_AUTO_TEST_CASE( selection_is_empty_noexcept_not_instantiated )
	{
		WordApplication app ;
		BOOST_CHECK(! app.is_instantiated()) ;
		BOOST_CHECK(app.selection_is_empty_noexcept()) ;
		BOOST_CHECK(! app.is_instantiated()) ;
	}
	// is_active keeps its "Word object exists" meaning; the active path needs Word (manual AC6)
	BOOST_AUTO_TEST_CASE( is_active_false_when_not_instantiated )
	{
		WordApplication app ;
		BOOST_CHECK(! app.is_instantiated()) ;
		BOOST_CHECK(! app.is_active()) ;
	}
	BOOST_AUTO_TEST_CASE( Instantiate )
	{
		BOOST_CHECK_NO_THROW(WordApplication()) ;
	}
BOOST_AUTO_TEST_SUITE_END()

#endif
