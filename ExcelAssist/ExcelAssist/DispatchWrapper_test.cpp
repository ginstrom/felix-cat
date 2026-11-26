#include "stdafx.h"

#include "DispatchWrapper.h"

#ifdef UNIT_TEST

#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_SUITE( CDispatchWrapperTest )

	// Felix checks for updates through Felix.Utilities, which may not be
	// registered: that must be a catchable _com_error, not a Debug assert.
	BOOST_AUTO_TEST_CASE( unregistered_progid_throws_com_error )
	{
		BOOST_CHECK_THROW( CDispatchWrapper wrapper(L"Felix.NoSuchProgIdForTests") , _com_error ) ;
	}


BOOST_AUTO_TEST_SUITE_END()

#endif

