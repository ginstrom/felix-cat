/*!
	@brief Matching COM server switches (/RegServer etc.) on Felix's command line.
 */
#pragma once

#include <tchar.h>

/*!
	True if the command-line token starting at `token` is `word`
	(case-insensitive). The token ends at the end of the string, whitespace or
	a quote, so "/RegServer " (Start-Process adds a trailing space) and
	"/RegServer /foo" still match. Felix used to compare against the whole rest
	of the command line, so a trailing space made it ignore /RegServer and
	start normally instead of registering.
*/
inline bool command_line_token_is(LPCTSTR token, LPCTSTR word)
{
	const size_t len = _tcslen(word) ;
	if (_tcsnicmp(token, word, len) != 0)
	{
		return false ;
	}
	const TCHAR next = token[len] ;
	return next == _T('\0') || next == _T(' ') || next == _T('\t') || next == _T('"') ;
}
