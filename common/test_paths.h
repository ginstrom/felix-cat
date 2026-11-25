/*!
	@brief Machine-independent paths and names for unit tests.

	Tests used to hardcode the original development machine
	(D:\Users\Ryan\..., D:\dev\cpp\Assistant Suite\..., user "Ryan").
	These helpers work them out on whatever machine runs the tests.

	The expected values come from the environment rather than the Windows
	APIs the code under test uses, so a test still checks something.
 */
#pragma once

// Not wrapped in #ifdef UNIT_TEST: some test files declare their suites
// outside that guard, so this must compile in every configuration.
#include <string>

namespace test_paths
{
	inline CString env_var(LPCTSTR name)
	{
		TCHAR buffer[MAX_PATH * 2] = {0} ;
		::GetEnvironmentVariable(name, buffer, MAX_PATH * 2) ;
		return CString(buffer) ;
	}

	//! %LOCALAPPDATA%, e.g. C:\Users\name\AppData\Local (no trailing backslash).
	inline CString local_appdata()
	{
		return env_var(_T("LOCALAPPDATA")) ;
	}

	//! The Felix folder under %LOCALAPPDATA%, plus an optional relative path.
	inline CString felix_appdata(LPCTSTR relative = _T(""))
	{
		CString path = local_appdata() + _T("\\Felix") ;
		if (relative && *relative)
		{
			path += _T("\\") ;
			path += relative ;
		}
		return path ;
	}

	//! The logged-in user's name (%USERNAME%).
	inline std::wstring user_name()
	{
		return std::wstring(CT2W(env_var(_T("USERNAME")))) ;
	}

	//! Folder of the running test executable (e.g. <repo>\Felix\Testing).
	inline CString exe_dir()
	{
		TCHAR path[MAX_PATH] = {0} ;
		::GetModuleFileName(NULL, path, MAX_PATH) ;
		::PathRemoveFileSpec(path) ;
		return CString(path) ;
	}

	//! A path in the source tree. The test exe is <repo>\Felix\Testing\Felix.exe,
	//! so the repo root is two folders up.
	inline CString repo_path(LPCTSTR relative)
	{
		CString combined = exe_dir() + _T("\\..\\..\\") + relative ;
		TCHAR canonical[MAX_PATH] = {0} ;
		::PathCanonicalize(canonical, combined) ;
		return CString(canonical) ;
	}

	//! Where Felix will find an English template: %LOCALAPPDATA%\Felix\html\en
	//! if the file is there, otherwise the html folder next to the exe.
	inline CString template_path(LPCTSTR relative)
	{
		const CString local = felix_appdata(CString(_T("html\\en\\")) + relative) ;
		if (::PathFileExists(local))
		{
			return local ;
		}
		return exe_dir() + _T("\\html\\en\\") + relative ;
	}

	inline std::string narrow(const CString &text)
	{
		return std::string(CT2A(text)) ;
	}

	//! True if a COM class with this ProgID is registered on this machine.
	inline bool prog_id_registered(LPCOLESTR prog_id)
	{
		CLSID clsid ;
		return SUCCEEDED(::CLSIDFromProgID(prog_id, &clsid)) ;
	}
}

#ifdef BOOST_TEST_DECORATOR
//! Boost.Test precondition: skip a suite (reported as skipped, with this
//! message) unless the given COM ProgID is registered. Usage:
//!   BOOST_AUTO_TEST_SUITE( name, * boost::unit_test::precondition(
//!       test_paths::requires_prog_id(L"Felix.RemoteMemoryFake")) )
namespace test_paths
{
	struct requires_prog_id
	{
		LPCOLESTR m_prog_id ;
		explicit requires_prog_id(LPCOLESTR prog_id) : m_prog_id(prog_id) {}
		boost::test_tools::assertion_result operator()(boost::unit_test::test_unit_id) const
		{
			boost::test_tools::assertion_result result(prog_id_registered(m_prog_id)) ;
			if (! result)
			{
				result.message() << "COM class " << std::string(CW2A(m_prog_id))
					<< " is not registered (see python_tools\\MemoryEngineFake.py)" ;
			}
			return result ;
		}
	} ;
}
#endif // BOOST_TEST_DECORATOR
