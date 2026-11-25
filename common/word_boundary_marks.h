#pragma once

#include <string>
#include <vector>
#include <utility>

/*!
 * Remove Word's structural marks from both ends of a selection string.
 *
 * Word includes the paragraph mark (0x0D) in a selection that reaches the end
 * of a paragraph, and the end-of-cell mark (0x07) in a table cell. Those
 * characters never occur in the text a user wants to search for, so concordance
 * queries drop them. Removed: 0x07, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E.
 *
 * The text may be HTML (formatted selection), so tags at either end are looked
 * past: marks next to or inside them are removed, but no tag is ever removed
 * or changed. An empty tag pair such as "<b></b>" may be left behind.
 * Spaces, tabs, NBSP and all other characters are kept.
 *
 * Does not throw (apart from allocation failure).
 */
inline bool is_word_boundary_mark( wchar_t c )
{
	switch ( c )
	{
	case L'\x07': // end of cell / row
	case L'\x0A': // line feed
	case L'\x0B': // manual line break
	case L'\x0C': // page / section break
	case L'\x0D': // paragraph mark
	case L'\x0E': // column break
		return true ;
	}
	return false ;
}

inline std::wstring trim_word_boundary_marks( const std::wstring &text )
{
	typedef std::pair< std::wstring::size_type, std::wstring::size_type > span ; // [first, last)

	// trailing end; tags are collected last-to-first
	std::wstring::size_type end = text.size() ;
	std::vector< span > trailing_tags ;
	while ( end > 0 )
	{
		const wchar_t c = text[end-1] ;
		if ( c == L'>' )
		{
			const std::wstring::size_type open = text.rfind( L'<', end-1 ) ;
			if ( open == std::wstring::npos )
			{
				break ;
			}
			trailing_tags.push_back( span( open, end ) ) ;
			end = open ;
		}
		else if ( is_word_boundary_mark( c ) )
		{
			--end ;
		}
		else
		{
			break ;
		}
	}

	// leading end, never reading past the trailing end
	std::wstring::size_type begin = 0 ;
	std::vector< span > leading_tags ;
	while ( begin < end )
	{
		const wchar_t c = text[begin] ;
		if ( c == L'<' )
		{
			const std::wstring::size_type close = text.find( L'>', begin ) ;
			if ( close == std::wstring::npos || close >= end )
			{
				break ;
			}
			leading_tags.push_back( span( begin, close + 1 ) ) ;
			begin = close + 1 ;
		}
		else if ( is_word_boundary_mark( c ) )
		{
			++begin ;
		}
		else
		{
			break ;
		}
	}

	std::wstring result ;
	for ( size_t i = 0 ; i < leading_tags.size() ; ++i )
	{
		result.append( text, leading_tags[i].first, leading_tags[i].second - leading_tags[i].first ) ;
	}
	result.append( text, begin, end - begin ) ;
	for ( size_t i = trailing_tags.size() ; i > 0 ; --i )
	{
		const span &tag = trailing_tags[i-1] ;
		result.append( text, tag.first, tag.second - tag.first ) ;
	}
	return result ;
}

/*!
 * True if the text has any character outside "<...>" tags. Entities such as
 * "&lt;" and whitespace count as text. A "<" with no closing ">" is text.
 */
inline bool has_text_outside_tags( const std::wstring &text )
{
	std::wstring::size_type pos = 0 ;
	while ( pos < text.size() )
	{
		if ( text[pos] == L'<' )
		{
			const std::wstring::size_type close = text.find( L'>', pos ) ;
			if ( close != std::wstring::npos )
			{
				pos = close + 1 ;
				continue ;
			}
		}
		return true ;
	}
	return false ;
}

/*!
 * Trim a concordance query (see trim_word_boundary_marks). A query left with
 * no text outside tags, such as an empty paragraph's mark wrapped in a font
 * tag, becomes the empty string.
 */
inline std::wstring trim_concordance_query( const std::wstring &text )
{
	const std::wstring trimmed = trim_word_boundary_marks( text ) ;
	return has_text_outside_tags( trimmed ) ? trimmed : std::wstring() ;
}