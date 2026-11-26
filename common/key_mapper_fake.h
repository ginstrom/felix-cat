#pragma once

#include "key_mapper.h"
#include "keyboard_shortcuts.h"
#include <stdexcept>

class KeyMapperFake : public KeyMapper
{
public:
	DECLARE_SENSING_VAR ;
	bool m_accepts_result = true ;               // returned by accepts
	std::vector<bool> m_shift_values ;           // shift_key_is_pressed() on each map_command
	boost::function<void(const string &)> m_on_map_command ; // called inside map_command if set
	bool m_throw = false ;                       // map_command throws after sensing

	bool accepts(const string &command)
	{
		command ;
		return m_accepts_result ;
	}
	bool map_command(string command)
	{
		command ;
		SENSE(command) ;
		m_shift_values.push_back(shift_key_is_pressed()) ;
		if (m_on_map_command)
		{
			m_on_map_command(command) ;
		}
		if (m_throw)
		{
			throw std::runtime_error("KeyMapperFake: m_throw is set") ;
		}
		return true ;
	}
};