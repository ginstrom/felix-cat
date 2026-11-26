#include "stdafx.h"
#include "PowerPointInterface.h"
#include "logging.h"

#ifdef UNIT_TEST

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( PowerPointInterfaceConstructionTest )

	class counting_logger : public logger
	{
	public:
		int entries ;
		counting_logger() : entries(0) {}
		void write_entry(const string, const string) { ++entries ; }
		void send_report(const CString, const CString) {}
	} ;

	// CConnect holds a CPowerPointInterface, so it is constructed when COM
	// creates the add-in, before OnConnection installs the logger. Logging
	// from the constructor (e.g. "File does not exist." for a missing
	// abbreviations.txt) then asserts "Logger not initialized!" in Debug.
	BOOST_AUTO_TEST_CASE( constructor_does_not_log )
	{
		logger_ptr previous = logging::get_logger() ;
		std::shared_ptr< counting_logger > counter(new counting_logger) ;
		logging::set_logger(counter) ;

		{
			CPowerPointInterface ppt_interface ;
		}

		logging::set_logger(previous) ;
		BOOST_CHECK_EQUAL( counter->entries, 0 ) ;
	}

BOOST_AUTO_TEST_SUITE_END()

#endif
