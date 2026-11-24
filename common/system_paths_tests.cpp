/*!
	@brief Unit tests for system_paths file
 */

#include "StdAfx.h"
#include "system_paths.h"

#ifdef UNIT_TEST
#include <boost/test/unit_test.hpp>

// The local app data folder, read from the environment rather than with the
// shell API that the code under test uses.
static string local_appdata_from_env()
{
	TCHAR buffer[MAX_PATH] = {0} ;
	::GetEnvironmentVariable(_T("LOCALAPPDATA"), buffer, MAX_PATH) ;
	return string(CT2A(buffer)) ;
}

BOOST_AUTO_TEST_SUITE( test_system_paths )

	BOOST_AUTO_TEST_CASE( test_get_help_file_path )
	{
		CString docs_path = "C:\\" ;
		CString help_file_path = get_help_file_path(docs_path) ;
		string actual = CT2A(help_file_path); 
		string expected = "http://felix-cat.com/media/manuals/felix/" ;

		BOOST_CHECK_EQUAL(expected, actual) ;
	}
	BOOST_AUTO_TEST_CASE( test_get_docs_path )
	{
		CString docs_path = get_docs_path() ;
		string actual = CT2A(docs_path); 
		string expected = local_appdata_from_env() + "\\Felix\\DOCS\\" ;

		BOOST_CHECK_EQUAL(expected, actual) ;
	}
	BOOST_AUTO_TEST_CASE( test_get_local_helpfile_name )
	{
		CString docs_path = get_local_helpfile_name(get_docs_path()) ;
		string actual = CT2A(docs_path); 
		string expected = local_appdata_from_env() + "\\Felix\\DOCS\\Felix Help.chm" ;

		BOOST_CHECK_EQUAL(expected, actual) ;
	}
BOOST_AUTO_TEST_SUITE_END()

#endif 
