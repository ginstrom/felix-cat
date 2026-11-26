#pragma once

#include "logging.h"
#include "keyboard_shortcuts.h"
#include <cstdio>

namespace shortcuts
{
	// Debug log line saying why a keyboard hook passed a key on to Office.
	// Only keystrokes with Alt or Ctrl down are logged (key-up messages never).
	// Makes no COM calls and never throws.
	// Debug builds only: the add-ins log at debug level in Release too, so the
	// line would cost a write per Ctrl/Alt key (Ctrl+C etc.) on the UI thread.
	inline void log_hook_pass_on( const char *reason, WPARAM vk, WORD key_message )
	{
#ifdef _DEBUG
		try
		{
			if ( key_message & KF_UP )
			{
				return ;
			}
			if ( ! control_key_is_pressed() && ! alt_key_is_pressed( key_message ) )
			{
				return ;
			}
			char buf[96] ;
			_snprintf_s( buf, _TRUNCATE, "Felix hook: passing on key (%s) vk=0x%02X", reason, static_cast<unsigned>(vk) ) ;
			logging::log_debug( buf ) ;
		}
		catch ( ... )
		{
		}
#else
		reason ; vk ; key_message ;
#endif
	}
}
