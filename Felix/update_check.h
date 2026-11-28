/*!
	@brief Checks this repository's GitHub releases for a newer Felix.

	Replaces the old Python CheckUpdates.exe and Felix.Utilities COM server.
	The pure functions here (version comparison, release parsing, scheduling)
	are unit-tested; fetch_latest_release does the network call.
 */
#pragma once

#include "RegistryStuff.h"		// CRegMap
#include "resource_string.h"	// resource_string
#include "resource.h"

namespace updates
{
	//! GitHub API endpoint for the newest published (non-draft, non-prerelease) release.
	extern const wchar_t* const LATEST_RELEASE_HOST ;
	extern const wchar_t* const LATEST_RELEASE_PATH ;
	//! Page listing all releases, opened when the user wants to download.
	extern const wchar_t* const RELEASES_PAGE ;
	//! Days between automatic checks.
	const long CHECK_INTERVAL_DAYS = 14 ;

	//! "v3.0.1" or "3.0.1" -> {3, 0, 1}. Empty if the text isn't a dotted number.
	std::vector<int> parse_version(const std::string &text) ;

	//! True if latest is a higher version than current. Missing parts count as 0,
	//! so "3.0" equals "3.0.0". Unparseable versions are never newer.
	bool is_newer(const std::string &latest, const std::string &current) ;

	//! Pulls "tag_name" out of the GitHub latest-release JSON. False if absent.
	bool parse_release_tag(const std::string &json, std::string &tag) ;

	//! The release page for a tag, e.g. .../releases/tag/v3.0
	std::wstring release_page_url(const std::string &tag) ;

	//! What to do at startup.
	enum auto_check_action
	{
		AUTO_CHECK_NONE,	//!< Not due, or the user turned automatic checks off.
		AUTO_CHECK_ASK,		//!< Never asked: ask whether to check automatically.
		AUTO_CHECK_NOW		//!< Due: check in the background.
	} ;

	//! last_check_day is 0 if Felix has never checked successfully.
	auto_check_action decide_auto_check(bool asked, bool enabled, long last_check_day, long today) ;

	//! Days since 1 January 1601 (FILETIME epoch), local time.
	long day_number(const SYSTEMTIME &time) ;
	long today_day_number() ;

	//! Fetches the latest release tag over HTTPS. On failure returns false and
	//! sets error to a short (English) description for the log.
	bool fetch_latest_release(std::string &tag, std::string &error) ;

	/**
		@struct update_settings
		@brief Per-user update-check state, kept in the registry.

		Deliberately separate from properties_general so it isn't exported
		to or imported from .fprefs files.
	 */
	struct update_settings : public props::CRegMap
	{
		BOOL	m_asked ;			//!< The user has answered the "check automatically?" question.
		BOOL	m_enabled ;			//!< Automatic checks are on.
		int		m_last_check_day ;	//!< day_number() of the last successful check; 0 = never.

		update_settings() :
			m_asked(FALSE),
			m_enabled(FALSE),
			m_last_check_day(0)
		{
		}

		BEGIN_REGISTRY_MAP( HKEY_CURRENT_USER, resource_string( IDS_REG_KEY ), _T("UPDATE_CHECK") ) ;
			REG_ENTRY_BOOL( _T("ASKED"),			m_asked )
			REG_ENTRY_BOOL( _T("ENABLED"),			m_enabled )
			REG_ENTRY_INT( _T("LAST_CHECK_DAY"),	m_last_check_day )
		END_REGISTRY_MAP
	} ;

	//! Result handed from the worker thread back to the main window.
	struct check_result
	{
		bool		m_succeeded ;
		std::string	m_latest_tag ;
		std::string	m_error ;

		check_result() :
			m_succeeded(false)
		{
		}
	} ;
}
