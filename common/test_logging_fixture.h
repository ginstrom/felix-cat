/*!
	@brief Boost.Test global fixture that installs a logger for test runs.

	The add-in test programs are console apps, so nothing sets up Felix's
	logger the way Felix's WinMain or the add-ins' OnConnection do. Without one,
	any code under test that logs trips the "Logger not initialized"
	assertion. Include this once, in the file that defines BOOST_TEST_MODULE.
 */
#pragma once

#include "logging.h"

//! A logger that keeps nothing, so tests don't write to the real log file.
class discarding_logger : public logger
{
public:
	void write_entry(const string, const string) {}
	void send_report(const CString, const CString) {}
} ;

struct TestLoggingFixture
{
	TestLoggingFixture()  { logging::set_logger(logger_ptr(new discarding_logger)) ; }
	~TestLoggingFixture() { logging::set_logger(logger_ptr()) ; }
} ;

BOOST_TEST_GLOBAL_FIXTURE( TestLoggingFixture ) ;
