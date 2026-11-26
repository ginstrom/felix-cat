#pragma once

#include "logging.h"
#include "keyboard_shortcuts.h"
#include <cstdio>

namespace shortcuts
{
	// Debug log line saying why a keyboard hook passed a key on to Office.
	// Only keystrokes with Alt or Ctrl down are logged (key-up messages never).
	// Makes no COM calls and never throws.
	inline void log_hook_pass_on( const char *reason, WPARAM vk, WORD key_message )
	{
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
	}
}
