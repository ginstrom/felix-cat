#pragma once

#include "config_file.h"
#include "pugxml/src/pugixml.hpp"
#include "key_mapper.h"
#include "xml_writer.h"

void add_child_text(pugi::xml_node &node, string name, string text) ;
void create_shortcut_node(pugi::xml_node &node, 
						  string stype, 
						  string first, 
						  string second, 
						  string command) ;


namespace shortcuts
{
	namespace detail
	{
		struct shift_override
		{
			bool active ;
			bool down ;
		};

		// The Shift state forced by the innermost live scoped_shift_state.
		// One per module; each add-in runs its shortcuts on one UI thread.
		inline shift_override &current_shift_override()
		{
			static shift_override value = { false, false } ;
			return value ;
		}
	}

	// While alive, shift_key_is_pressed() returns `down` instead of reading the
	// keyboard. Used to run a deferred command with the Shift state of its keystroke.
	// The innermost scope wins; the destructor restores the previous override.
	struct scoped_shift_state
	{
		explicit scoped_shift_state(bool down) :
			m_previous(detail::current_shift_override())
		{
			detail::shift_override &current = detail::current_shift_override() ;
			current.active = true ;
			current.down = down ;
		}
		~scoped_shift_state()
		{
			detail::current_shift_override() = m_previous ;
		}
	private:
		const detail::shift_override m_previous ;

		scoped_shift_state(const scoped_shift_state &) ;
		scoped_shift_state &operator=(const scoped_shift_state &) ;
	};

	// While alive, no override is in force: shift_key_is_pressed() reads the
	// keyboard. The keyboard-hook path uses it, because the hook can run while a
	// deferred command (and its override) is running. The destructor restores
	// the previous override.
	struct scoped_physical_shift_state
	{
		scoped_physical_shift_state() :
			m_previous(detail::current_shift_override())
		{
			detail::current_shift_override().active = false ;
		}
		~scoped_physical_shift_state()
		{
			detail::current_shift_override() = m_previous ;
		}
	private:
		const detail::shift_override m_previous ;

		scoped_physical_shift_state(const scoped_physical_shift_state &) ;
		scoped_physical_shift_state &operator=(const scoped_physical_shift_state &) ;
	};

	// The Shift key as the keyboard reports it, ignoring any override.
	// Use this in hook procedures.
	inline bool physical_shift_key_is_pressed()
	{
		return !! ( ::GetKeyState(VK_SHIFT) & 0x8000 ) ;
	}
}

inline bool shift_key_is_pressed()
{
	const shortcuts::detail::shift_override &override_state = shortcuts::detail::current_shift_override() ;
	if (override_state.active)
	{
		return override_state.down ;
	}
	return shortcuts::physical_shift_key_is_pressed() ;
}
inline bool control_key_is_pressed()
{
	return !! ( ::GetKeyState(VK_CONTROL) & 0x8000 ) ;
}

inline bool alt_key_is_pressed( WORD key_message )
{
	return !! ( key_message & KF_ALTDOWN ) ;
}
namespace shortcuts
{
	typedef enum 
	{
		SHORTCUT_CTRL,
		SHORTCUT_ALT,
		SHORTCUT_CTRL_ALT,
	} SHORTCUT_TYPE ;

	struct KeyboardShortcut
	{
		const SHORTCUT_TYPE m_type ;
		const wchar_t m_firstkey ;
		const wchar_t m_secondkey ;
		const string m_command ;
		KeyboardShortcut(SHORTCUT_TYPE stype,
						wchar_t firstkey,
						wchar_t secondkey,
						const string &command);
	};

	// True if keystrokes for `focus` should be treated as Felix shortcuts: `focus` is
	// non-NULL, its top-level ancestor (GetAncestor(focus, GA_ROOT)) has window class
	// `main_window_class` (case-insensitive), and that top-level window is enabled.
	bool keystroke_target_is_main_window(HWND focus, LPCWSTR main_window_class);

	typedef std::shared_ptr<KeyboardShortcut> shortcut_ptr ;

	class KeyboardShortcuts
	{
	public:
		typedef std::map<wchar_t, shortcut_ptr> shortcut_map ;
		typedef std::shared_ptr<shortcut_map> shortcut_map_ptr ;
		typedef std::map<wchar_t, shortcut_map_ptr> key_map ;

		key_map m_ctrl_shortcuts ;
		key_map m_alt_shortcuts ;
		key_map m_ctrl_alt_shortcuts ;

		std::map<string, wchar_t> virtkeys ;
		KeyMapper				*m_mapper ;

		wchar_t					m_second_key ;
		shortcut_map_ptr		m_second_keys ;
		string					m_second_command ;
		bool					m_shortcuts_enabled ;

		boost::function<void(BOOL)> m_on_toggle_shortcuts ;

		KeyboardShortcuts(KeyMapper *mapper);
		void load(const string text);

		void add_shortcut( shortcut_ptr shortcut );
		bool receive_keypress(bool ctrl_pressed, bool alt_pressed, wchar_t key);

		key_map * get_key_map( bool ctrl_pressed, bool alt_pressed );
		shortcut_ptr load_shortcut( pugi::xml_node &shortcut_node );

		SHORTCUT_TYPE get_shortcut_type(const string shortcut_type) ;
		wchar_t get_key(const string shortcut_key) ;
	};
}
