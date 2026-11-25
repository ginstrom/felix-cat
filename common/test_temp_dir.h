/*!
	@brief Scratch folder for unit tests that need real files on disk.

	Creates a uniquely named folder under %TEMP% and deletes it (and the
	files in it) when the object goes out of scope, so tests don't depend
	on hardcoded paths such as c:\test existing on the build machine.
 */
#pragma once

#ifdef UNIT_TEST

#include <string>

class TestTempDir
{
	CString m_path ;

	TestTempDir(const TestTempDir&) ;
	TestTempDir& operator=(const TestTempDir&) ;

public:
	TestTempDir()
	{
		TCHAR temp_dir[MAX_PATH] = {0} ;
		::GetTempPath(MAX_PATH, temp_dir) ;

		// GetTempFileName gives us a unique name (and creates an empty file
		// with it); swap that file for a folder of the same name.
		TCHAR unique[MAX_PATH] = {0} ;
		::GetTempFileName(temp_dir, _T("flx"), 0, unique) ;
		::DeleteFile(unique) ;
		::CreateDirectory(unique, NULL) ;

		m_path = unique ;
	}

	~TestTempDir()
	{
		WIN32_FIND_DATA data = {0} ;
		HANDLE find = ::FindFirstFile(m_path + _T("\\*"), &data) ;
		if (find != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (! (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					::DeleteFile(path(data.cFileName)) ;
				}
			} while (::FindNextFile(find, &data)) ;
			::FindClose(find) ;
		}
		::RemoveDirectory(m_path) ;
	}

	//! Full path of the folder, without a trailing backslash.
	CString dir() const { return m_path ; }

	//! Full path of a file in the folder (the file needn't exist).
	CString path(LPCTSTR filename) const { return m_path + _T("\\") + filename ; }

	//! Writes raw bytes to a file in the folder and returns its full path.
	CString write_file(LPCTSTR filename, const void *bytes, size_t len) const
	{
		const CString full_path = path(filename) ;
		HANDLE file = ::CreateFile(full_path, GENERIC_WRITE, 0, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL) ;
		if (file != INVALID_HANDLE_VALUE)
		{
			DWORD written = 0 ;
			::WriteFile(file, bytes, static_cast<DWORD>(len), &written, NULL) ;
			::CloseHandle(file) ;
		}
		return full_path ;
	}

	//! Writes text in a multibyte code page (e.g. 932 for Shift-JIS, CP_UTF8).
	CString write_codepage_file(LPCTSTR filename, const std::wstring &text, UINT code_page) const
	{
		const int len = ::WideCharToMultiByte(code_page, 0, text.c_str(),
			static_cast<int>(text.size()), NULL, 0, NULL, NULL) ;
		std::string bytes(static_cast<size_t>(len), '\0') ;
		::WideCharToMultiByte(code_page, 0, text.c_str(), static_cast<int>(text.size()),
			&bytes[0], len, NULL, NULL) ;
		return write_file(filename, bytes.data(), bytes.size()) ;
	}

	//! Writes text as UTF-16LE with a byte order mark.
	CString write_utf16_file(LPCTSTR filename, const std::wstring &text) const
	{
		std::wstring with_bom = L"\xFEFF" + text ;
		return write_file(filename, with_bom.c_str(), with_bom.size() * sizeof(wchar_t)) ;
	}
} ;

#endif // UNIT_TEST
