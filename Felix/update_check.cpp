#include "stdafx.h"
#include "update_check.h"

#include <winhttp.h>
#include <regex>

#pragma comment(lib, "winhttp.lib")

namespace updates
{
	const wchar_t* const LATEST_RELEASE_HOST = L"api.github.com" ;
	const wchar_t* const LATEST_RELEASE_PATH = L"/repos/ginstrom/felix-cat/releases/latest" ;
	const wchar_t* const RELEASES_PAGE = L"https://github.com/ginstrom/felix-cat/releases" ;

	std::vector<int> parse_version(const std::string &text)
	{
		std::vector<int> parts ;
		// Local, not static: these run on the update-check worker thread, which
		// may still be running while static objects are destroyed at exit.
		const std::regex pattern("^\\s*[vV]?(\\d{1,6}(?:\\.\\d{1,6})*)\\s*$") ;
		std::smatch match ;
		if (! std::regex_match(text, match, pattern))
		{
			return parts ;
		}
		const std::string numbers = match[1].str() ;
		size_t start = 0 ;
		while (start <= numbers.size())
		{
			size_t dot = numbers.find('.', start) ;
			if (dot == std::string::npos)
			{
				dot = numbers.size() ;
			}
			parts.push_back(std::stoi(numbers.substr(start, dot - start))) ;
			start = dot + 1 ;
		}
		return parts ;
	}

	bool is_newer(const std::string &latest, const std::string &current)
	{
		std::vector<int> l = parse_version(latest) ;
		std::vector<int> c = parse_version(current) ;
		if (l.empty() || c.empty())
		{
			return false ;
		}
		const size_t count = (std::max)(l.size(), c.size()) ;
		l.resize(count, 0) ;
		c.resize(count, 0) ;
		return l > c ;
	}

	bool parse_release_tag(const std::string &json, std::string &tag)
	{
		// Local, not static (see parse_version). The top-level tag_name comes
		// first in GitHub's release JSON, and nested objects have none.
		const std::regex pattern("\"tag_name\"\\s*:\\s*\"([^\"\\\\]*)\"") ;
		std::smatch match ;
		if (! std::regex_search(json, match, pattern))
		{
			return false ;
		}
		tag = match[1].str() ;
		return ! tag.empty() ;
	}

	std::wstring release_page_url(const std::string &tag)
	{
		std::wstring url(RELEASES_PAGE) ;
		url += L"/tag/" ;
		url += static_cast<LPCWSTR>(CA2W(tag.c_str(), CP_UTF8)) ;
		return url ;
	}

	auto_check_action decide_auto_check(bool asked, bool enabled, long last_check_day, long today)
	{
		if (! asked)
		{
			return AUTO_CHECK_ASK ;
		}
		if (! enabled)
		{
			return AUTO_CHECK_NONE ;
		}
		// Check if never checked, if due, or if the clock went backwards.
		if (last_check_day <= 0 ||
			today < last_check_day ||
			today - last_check_day >= CHECK_INTERVAL_DAYS)
		{
			return AUTO_CHECK_NOW ;
		}
		return AUTO_CHECK_NONE ;
	}

	long day_number(const SYSTEMTIME &time)
	{
		SYSTEMTIME day = time ;
		day.wHour = day.wMinute = day.wSecond = day.wMilliseconds = 0 ;
		FILETIME ft = {} ;
		if (! ::SystemTimeToFileTime(&day, &ft))
		{
			return 0 ;
		}
		ULARGE_INTEGER ticks ;
		ticks.LowPart = ft.dwLowDateTime ;
		ticks.HighPart = ft.dwHighDateTime ;
		const ULONGLONG TICKS_PER_DAY = 864000000000ULL ;
		return static_cast<long>(ticks.QuadPart / TICKS_PER_DAY) ;
	}

	long today_day_number()
	{
		SYSTEMTIME now ;
		::GetLocalTime(&now) ;
		return day_number(now) ;
	}

	namespace
	{
		//! Closes a WinHTTP handle on scope exit.
		class internet_handle
		{
			HINTERNET m_handle ;
			internet_handle(const internet_handle&) ;
			internet_handle& operator=(const internet_handle&) ;
		public:
			explicit internet_handle(HINTERNET handle) : m_handle(handle) {}
			~internet_handle() { if (m_handle) { ::WinHttpCloseHandle(m_handle) ; } }
			operator HINTERNET() const { return m_handle ; }
		} ;

		std::string last_error(const char *step)
		{
			std::ostringstream out ;
			out << step << " failed (error " << ::GetLastError() << ")" ;
			return out.str() ;
		}
	}

	bool fetch_latest_release(std::string &tag, std::string &error)
	{
		std::wstring agent(L"Felix-update-check/") ;
		agent += CA2W(VERSION) ;

		// Automatic proxy uses the system/IE proxy settings (Windows 8.1+;
		// the installer requires Windows 10).
		internet_handle session(::WinHttpOpen(agent.c_str(),
			WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS,
			0)) ;
		if (! session)
		{
			error = last_error("WinHttpOpen") ;
			return false ;
		}
		// resolve, connect, send, receive (milliseconds). Best effort: if this
		// fails, WinHTTP's defaults apply, and this runs off the UI thread anyway.
		::WinHttpSetTimeouts(session, 10000, 10000, 10000, 15000) ;

		internet_handle connection(::WinHttpConnect(session, LATEST_RELEASE_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0)) ;
		if (! connection)
		{
			error = last_error("WinHttpConnect") ;
			return false ;
		}

		internet_handle request(::WinHttpOpenRequest(connection,
			L"GET",
			LATEST_RELEASE_PATH,
			NULL,
			WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			WINHTTP_FLAG_SECURE)) ;
		if (! request)
		{
			error = last_error("WinHttpOpenRequest") ;
			return false ;
		}

		const wchar_t *headers = L"Accept: application/vnd.github+json\r\n" ;
		if (! ::WinHttpSendRequest(request, headers, (DWORD)-1L, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
		{
			error = last_error("WinHttpSendRequest") ;
			return false ;
		}
		if (! ::WinHttpReceiveResponse(request, NULL))
		{
			error = last_error("WinHttpReceiveResponse") ;
			return false ;
		}

		DWORD status = 0 ;
		DWORD status_size = sizeof(status) ;
		if (! ::WinHttpQueryHeaders(request,
			WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
			WINHTTP_HEADER_NAME_BY_INDEX,
			&status,
			&status_size,
			WINHTTP_NO_HEADER_INDEX))
		{
			error = last_error("WinHttpQueryHeaders") ;
			return false ;
		}
		if (status != 200)
		{
			std::ostringstream out ;
			out << "GitHub returned HTTP " << status ;
			error = out.str() ;
			return false ;
		}

		// The release JSON is a few KB; cap it so a bad response can't eat memory.
		const size_t MAX_BODY = 1024 * 1024 ;
		std::string body ;
		for (;;)
		{
			DWORD available = 0 ;
			if (! ::WinHttpQueryDataAvailable(request, &available))
			{
				error = last_error("WinHttpQueryDataAvailable") ;
				return false ;
			}
			if (available == 0)
			{
				break ;
			}
			std::vector<char> buffer(available) ;
			DWORD read = 0 ;
			if (! ::WinHttpReadData(request, &buffer[0], available, &read))
			{
				error = last_error("WinHttpReadData") ;
				return false ;
			}
			body.append(&buffer[0], read) ;
			if (body.size() > MAX_BODY)
			{
				error = "Release response too large" ;
				return false ;
			}
		}

		if (! parse_release_tag(body, tag))
		{
			error = "No tag_name in the GitHub release response" ;
			return false ;
		}
		return true ;
	}
}
