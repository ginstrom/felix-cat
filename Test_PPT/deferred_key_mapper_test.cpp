#include "stdafx.h"
#include "deferred_key_mapper.h"
#include "keyboard_shortcuts.h"
#include "key_mapper_fake.h"

#ifdef UNIT_TEST

#include <boost/test/unit_test.hpp>

using namespace shortcuts ;

namespace
{
	// Dispatches every message waiting on this thread's queue (including those
	// for HWND_MESSAGE windows); returns how many were WM_FELIX_RUN_COMMANDS.
	size_t pump_messages()
	{
		size_t run_messages = 0 ;
		MSG msg ;
		while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_FELIX_RUN_COMMANDS)
			{
				++run_messages ;
			}
			::TranslateMessage(&msg) ;
			::DispatchMessage(&msg) ;
		}
		return run_messages ;
	}

	bool real_shift_state()
	{
		return !! (::GetKeyState(VK_SHIFT) & 0x8000) ;
	}

	// Inside "First", a keystroke queues "Second" and the message loop is
	// pumped, as Office does when a command makes a COM call.
	struct NestedQueuer
	{
		DeferredKeyMapper *m_deferred ;
		KeyMapperFake *m_inner ;
		size_t m_runs_seen_inside ;
		size_t m_pending_seen_inside ;

		NestedQueuer(DeferredKeyMapper *deferred, KeyMapperFake *inner) :
			m_deferred(deferred), m_inner(inner), m_runs_seen_inside(0), m_pending_seen_inside(0)
		{
		}
		void operator()(const string &command)
		{
			if (command != "First")
			{
				return ;
			}
			m_deferred->map_command("Second") ;
			pump_messages() ;
			m_runs_seen_inside = m_inner->m_sensing_variable.size() ;
			m_pending_seen_inside = m_deferred->pending_count() ;
		}
	};

	// Stands in for the physical Shift key (DeferredKeyMapper::m_read_keyboard_shift).
	struct KeyboardShift
	{
		bool m_down ;
		explicit KeyboardShift(bool down) : m_down(down) {}
		bool operator()() const { return m_down ; }
	};

	// Records the Shift state that accepts() sees.
	class KeyMapperRecordsAccepts : public KeyMapperFake
	{
	public:
		std::vector<bool> m_accepts_shift_values ;
		bool accepts(const string &command)
		{
			m_accepts_shift_values.push_back(shift_key_is_pressed()) ;
			return KeyMapperFake::accepts(command) ;
		}
	};

	// A keystroke arrives while "First" runs with a Shift override active
	// (Office pumps messages inside the command); the hook queues "Second".
	struct KeystrokeDuringOverride
	{
		DeferredKeyMapper *m_deferred ;
		bool m_override ;
		bool m_override_seen ;

		KeystrokeDuringOverride(DeferredKeyMapper *deferred, bool override_value) :
			m_deferred(deferred), m_override(override_value), m_override_seen(false)
		{
		}
		void operator()(const string &command)
		{
			if (command != "First")
			{
				return ;
			}
			scoped_shift_state running_command_shift(m_override) ;
			m_override_seen = (shift_key_is_pressed() == m_override) ;
			m_deferred->map_command("Second") ;
		}
	};

	struct ToggleRecorder
	{
		std::vector<BOOL> *m_values ;
		explicit ToggleRecorder(std::vector<BOOL> *values) : m_values(values) {}
		void operator()(BOOL enabled) { m_values->push_back(enabled) ; }
	};
}

BOOST_AUTO_TEST_SUITE( ScopedShiftStateTest )

	BOOST_AUTO_TEST_CASE(test_no_override_reads_keyboard)
	{
		BOOST_CHECK_EQUAL(real_shift_state(), shift_key_is_pressed()) ;
	}

	BOOST_AUTO_TEST_CASE(test_override_down_and_up)
	{
		{
			scoped_shift_state shift(true) ;
			BOOST_CHECK(shift_key_is_pressed()) ;
		}
		{
			scoped_shift_state shift(false) ;
			BOOST_CHECK(! shift_key_is_pressed()) ;
		}
		BOOST_CHECK_EQUAL(real_shift_state(), shift_key_is_pressed()) ;
	}

	BOOST_AUTO_TEST_CASE(test_nesting_restores_outer_value)
	{
		{
			scoped_shift_state outer(true) ;
			{
				scoped_shift_state inner(false) ;
				BOOST_CHECK(! shift_key_is_pressed()) ;
			}
			BOOST_CHECK(shift_key_is_pressed()) ;
		}
		BOOST_CHECK_EQUAL(real_shift_state(), shift_key_is_pressed()) ;
	}

	BOOST_AUTO_TEST_CASE(test_physical_scope_suspends_and_restores_override)
	{
		const bool real = real_shift_state() ;
		{
			scoped_shift_state outer(! real) ;
			{
				scoped_physical_shift_state physical ;
				BOOST_CHECK_EQUAL(real, shift_key_is_pressed()) ;
				BOOST_CHECK_EQUAL(real, physical_shift_key_is_pressed()) ;
			}
			BOOST_CHECK_EQUAL(! real, shift_key_is_pressed()) ;
			BOOST_CHECK_EQUAL(real, physical_shift_key_is_pressed()) ;
		}
		BOOST_CHECK_EQUAL(real, shift_key_is_pressed()) ;
	}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( DeferredKeyMapperTest )

	BOOST_AUTO_TEST_CASE(test_without_window_runs_inner_immediately)
	{
		pump_messages() ;
		KeyMapperFake inner ;
		DeferredKeyMapper deferred(&inner) ;

		BOOST_CHECK(deferred.map_command("LookupNext")) ;

		BOOST_CHECK_EQUAL(1u, inner.m_sensing_variable.size()) ;
		BOOST_CHECK_EQUAL(0u, deferred.pending_count()) ;
		BOOST_CHECK_EQUAL(0u, pump_messages()) ;
	}

	// KeyMapper whose commands decline (return false), to check the result is passed through.
	class KeyMapperDeclinesCommand : public KeyMapperFake
	{
	public:
		bool map_command(string command)
		{
			KeyMapperFake::map_command(command) ;
			return false ;
		}
	};

	BOOST_AUTO_TEST_CASE(test_without_window_returns_inner_result)
	{
		KeyMapperDeclinesCommand inner ;
		DeferredKeyMapper deferred(&inner) ;

		BOOST_CHECK(! deferred.map_command("Concordance")) ;
		BOOST_CHECK_EQUAL(1u, inner.m_sensing_variable.size()) ;
	}

	BOOST_AUTO_TEST_CASE(test_with_window_queues_until_pumped)
	{
		pump_messages() ;
		KeyMapperFake inner ;
		DeferredKeyMapper deferred(&inner) ;
		BOOST_REQUIRE(deferred.create()) ;

		BOOST_CHECK(deferred.map_command("LookupNext")) ;

		BOOST_CHECK_EQUAL(0u, inner.m_sensing_variable.size()) ;
		BOOST_CHECK_EQUAL(1u, deferred.pending_count()) ;

		BOOST_CHECK_EQUAL(1u, pump_messages()) ;

		BOOST_REQUIRE_EQUAL(1u, inner.m_sensing_variable.size()) ;
		BOOST_CHECK_EQUAL("LookupNext", inner.m_sensing_variable[0]) ;
		BOOST_CHECK_EQUAL(0u, deferred.pending_count()) ;
		deferred.destroy() ;
	}

	BOOST_AUTO_TEST_CASE(test_three_commands_one_message_in_order)
	{
		pump_messages() ;
		KeyMapperFake inner ;
		DeferredKeyMapper deferred(&inner) ;
		BOOST_REQUIRE(deferred.create()) ;

		deferred.map_command("One") ;
		deferred.map_command("Two") ;
		deferred.map_command("Three") ;
		BOOST_CHECK_EQUAL(3u, deferred.pending_count()) ;

		BOOST_CHECK_EQUAL(1u, pump_messages()) ;

		BOOST_REQUIRE_EQUAL(3u, inner.m_sensing_variable.size()) ;
		BOOST_CHECK_EQUAL("One", inner.m_sensing_variable[0]) ;
		BOOST_CHECK_EQUAL("Two", inner.m_sensing_variable[1]) ;
		BOOST_CHECK_EQUAL("Three", inner.m_sensing_variable[2]) ;
		BOOST_CHECK_EQUAL(0u, deferred.pending_count()) ;

		// A second pump has nothing to do.
		BOOST_CHECK_EQUAL(0u, pump_messages()) ;
		BOOST_CHECK_EQUAL(3u, inner.m_sensing_variable.size()) ;
		deferred.destroy() ;
	}

	BOOST_AUTO_TEST_CASE(test_declined_command_is_not_queued)
	{
		pump_messages() ;
		KeyMapperFake inner ;
		inner.m_accepts_result = false ;
		DeferredKeyMapper deferred(&inner) ;
		BOOST_REQUIRE(deferred.create()) ;

		BOOST_CHECK(! deferred.map_command("LookupNext")) ;

		BOOST_CHECK_EQUAL(0u, deferred.pending_count()) ;
		BOOST_CHECK_EQUAL(0u, pump_messages()) ;
		BOOST_CHECK_EQUAL(0u, inner.m_sensing_variable.size()) ;
		deferred.destroy() ;
	}

	BOOST_AUTO_TEST_CASE(test_declined_command_without_window_does_not_run)
	{
		KeyMapperFake inner ;
		inner.m_accepts_result = false ;
		DeferredKeyMapper deferred(&inner) ;

		BOOST_CHECK(! deferred.map_command("LookupNext")) ;
		BOOST_CHECK_EQUAL(0u, inner.m_sensing_variable.size()) ;
	}

	BOOST_AUTO_TEST_CASE(test_accepts_forwards_to_inner)
	{
		KeyMapperFake inner ;
		DeferredKeyMapper deferred(&inner) ;

		BOOST_CHECK(deferred.accepts("LookupNext")) ;
		inner.m_accepts_result = false ;
		BOOST_CHECK(! deferred.accepts("LookupNext")) ;
	}

	BOOST_AUTO_TEST_CASE(test_shift_down_at_keypress_is_used_when_run)
	{
		pump_messages() ;
		KeyMapperFake inner ;
		DeferredKeyMapper deferred(&inner) ;
		BOOST_REQUIRE(deferred.create()) ;

		deferred.m_read_keyboard_shift = KeyboardShift(true) ;
		deferred.map_command("Correct") ;
		deferred.m_read_keyboard_shift = KeyboardShift(false) ;
		pump_messages() ;

		BOOST_REQUIRE_EQUAL(1u, inner.m_shift_values.size()) ;
		BOOST_CHECK(inner.m_shift_values[0]) ;
		deferred.destroy() ;
	}

	BOOST_AUTO_TEST_CASE(test_shift_up_at_keypress_is_used_when_run)
	{
		pump_messages() ;
		KeyMapperFake inner ;
		DeferredKeyMapper deferred(&inner) ;
		BOOST_REQUIRE(deferred.create()) ;

		deferred.m_read_keyboard_shift = KeyboardShift(false) ;
		deferred.map_command("Correct") ;
		deferred.m_read_keyboard_shift = KeyboardShift(true) ;
		pump_messages() ;

		BOOST_REQUIRE_EQUAL(1u, inner.m_shift_values.size()) ;
		BOOST_CHECK(! inner.m_shift_values[0]) ;
		deferred.destroy() ;
	}

	// The hook path reads the physical Shift key, not the override of a
	// command that is running (Office pumps messages inside commands).
	BOOST_AUTO_TEST_CASE(test_keystroke_during_running_command_ignores_its_shift_override)
	{
		pump_messages() ;
		const bool real = real_shift_state() ;
		KeyMapperRecordsAccepts inner ;
		DeferredKeyMapper deferred(&inner) ;
		BOOST_REQUIRE(deferred.create()) ;
		KeystrokeDuringOverride keystroke(&deferred, ! real) ;
		inner.m_on_map_command = boost::ref(keystroke) ;

		deferred.map_command("First") ;
		pump_messages() ;

		BOOST_CHECK(keystroke.m_override_seen) ;
		BOOST_REQUIRE_EQUAL(2u, inner.m_sensing_variable.size()) ;
		BOOST_CHECK_EQUAL("Second", inner.m_sensing_variable[1]) ;
		// Shift captured for Second (and in force while it ran) is the real state.
		BOOST_REQUIRE_EQUAL(2u, inner.m_shift_values.size()) ;
		BOOST_CHECK_EQUAL(real, inner.m_shift_values[1]) ;
		// accepts for Second saw the real state.
		BOOST_REQUIRE_EQUAL(2u, inner.m_accepts_shift_values.size()) ;
		BOOST_CHECK_EQUAL(real, inner.m_accepts_shift_values[1]) ;
		deferred.destroy() ;
	}

	// With no window, the command runs at once, also with the physical Shift state.
	BOOST_AUTO_TEST_CASE(test_without_window_runs_with_physical_shift)
	{
		const bool real = real_shift_state() ;
		KeyMapperFake inner ;
		DeferredKeyMapper deferred(&inner) ;

		{
			scoped_shift_state running_command_shift(! real) ;
			deferred.map_command("LookupNext") ;
		}

		BOOST_REQUIRE_EQUAL(1u, inner.m_shift_values.size()) ;
		BOOST_CHECK_EQUAL(real, inner.m_shift_values[0]) ;
	}

	BOOST_AUTO_TEST_CASE(test_command_queued_while_running_waits)
	{
		pump_messages() ;
		KeyMapperFake inner ;
		DeferredKeyMapper deferred(&inner) ;
		BOOST_REQUIRE(deferred.create()) ;
		NestedQueuer queuer(&deferred, &inner) ;
		inner.m_on_map_command = boost::ref(queuer) ;

		deferred.map_command("First") ;
		pump_messages() ;

		// Inside "First", the nested pump did not run "Second"; it ran afterwards.
		BOOST_CHECK_EQUAL(1u, queuer.m_runs_seen_inside) ;
		BOOST_CHECK_EQUAL(1u, queuer.m_pending_seen_inside) ;
		BOOST_REQUIRE_EQUAL(2u, inner.m_sensing_variable.size()) ;
		BOOST_CHECK_EQUAL("First", inner.m_sensing_variable[0]) ;
		BOOST_CHECK_EQUAL("Second", inner.m_sensing_variable[1]) ;
		BOOST_CHECK_EQUAL(0u, deferred.pending_count()) ;
		deferred.destroy() ;
	}

	BOOST_AUTO_TEST_CASE(test_throwing_command_does_not_stop_queue)
	{
		pump_messages() ;
		KeyMapperFake inner ;
		inner.m_throw = true ;
		DeferredKeyMapper deferred(&inner) ;
		BOOST_REQUIRE(deferred.create()) ;

		deferred.map_command("First") ;
		deferred.map_command("Second") ;
		BOOST_CHECK_NO_THROW(pump_messages()) ;
		BOOST_CHECK_NO_THROW(deferred.run_pending()) ;

		BOOST_REQUIRE_EQUAL(2u, inner.m_sensing_variable.size()) ;
		BOOST_CHECK_EQUAL("First", inner.m_sensing_variable[0]) ;
		BOOST_CHECK_EQUAL("Second", inner.m_sensing_variable[1]) ;
		BOOST_CHECK_EQUAL(0u, deferred.pending_count()) ;

		// The queue still works after the throws.
		deferred.map_command("Third") ;
		BOOST_CHECK_NO_THROW(pump_messages()) ;
		BOOST_REQUIRE_EQUAL(3u, inner.m_sensing_variable.size()) ;
		BOOST_CHECK_EQUAL("Third", inner.m_sensing_variable[2]) ;
		BOOST_CHECK_EQUAL(0u, deferred.pending_count()) ;
		deferred.destroy() ;
	}

	BOOST_AUTO_TEST_CASE(test_destroy_drops_pending_commands)
	{
		pump_messages() ;
		KeyMapperFake inner ;
		DeferredKeyMapper deferred(&inner) ;
		BOOST_REQUIRE(deferred.create()) ;

		deferred.map_command("LookupNext") ;
		deferred.destroy() ;

		BOOST_CHECK_EQUAL(0u, deferred.pending_count()) ;
		pump_messages() ;
		deferred.run_pending() ;
		BOOST_CHECK_EQUAL(0u, inner.m_sensing_variable.size()) ;

		// With the window gone, commands run synchronously again (R5).
		BOOST_CHECK(deferred.map_command("LookupNext")) ;
		BOOST_CHECK_EQUAL(1u, inner.m_sensing_variable.size()) ;
	}

	BOOST_AUTO_TEST_CASE(test_chord_second_key_consumed_and_queued)
	{
		pump_messages() ;
		KeyMapperFake inner ;
		DeferredKeyMapper deferred(&inner) ;
		BOOST_REQUIRE(deferred.create()) ;
		KeyboardShortcuts keyboard_shortcuts(&deferred) ;
		keyboard_shortcuts.add_shortcut(shortcut_ptr(new KeyboardShortcut(SHORTCUT_ALT, L'M', L'C', "Concordance"))) ;

		BOOST_CHECK(keyboard_shortcuts.receive_keypress(false, true, L'M')) ;
		BOOST_CHECK(keyboard_shortcuts.receive_keypress(false, false, L'C')) ;

		BOOST_CHECK_EQUAL(0u, inner.m_sensing_variable.size()) ;
		BOOST_CHECK_EQUAL(1u, deferred.pending_count()) ;

		pump_messages() ;

		BOOST_REQUIRE_EQUAL(1u, inner.m_sensing_variable.size()) ;
		BOOST_CHECK_EQUAL("Concordance", inner.m_sensing_variable[0]) ;
		deferred.destroy() ;
	}

	BOOST_AUTO_TEST_CASE(test_single_key_shortcut_consumed_and_queued)
	{
		pump_messages() ;
		KeyMapperFake inner ;
		DeferredKeyMapper deferred(&inner) ;
		BOOST_REQUIRE(deferred.create()) ;
		KeyboardShortcuts keyboard_shortcuts(&deferred) ;
		keyboard_shortcuts.add_shortcut(shortcut_ptr(new KeyboardShortcut(SHORTCUT_CTRL, L'M', 0, "LookupNext"))) ;

		BOOST_CHECK(keyboard_shortcuts.receive_keypress(true, false, L'M')) ;
		BOOST_CHECK_EQUAL(0u, inner.m_sensing_variable.size()) ;

		pump_messages() ;

		BOOST_REQUIRE_EQUAL(1u, inner.m_sensing_variable.size()) ;
		BOOST_CHECK_EQUAL("LookupNext", inner.m_sensing_variable[0]) ;
		deferred.destroy() ;
	}

	BOOST_AUTO_TEST_CASE(test_toggle_shortcuts_stays_synchronous)
	{
		pump_messages() ;
		KeyMapperFake inner ;
		DeferredKeyMapper deferred(&inner) ;
		BOOST_REQUIRE(deferred.create()) ;
		KeyboardShortcuts keyboard_shortcuts(&deferred) ;
		std::vector<BOOL> toggles ;
		keyboard_shortcuts.m_on_toggle_shortcuts = ToggleRecorder(&toggles) ;
		keyboard_shortcuts.add_shortcut(shortcut_ptr(new KeyboardShortcut(SHORTCUT_CTRL, VK_F1, 0, "ToggleShortcuts"))) ;

		BOOST_CHECK(keyboard_shortcuts.receive_keypress(true, false, VK_F1)) ;

		BOOST_CHECK_EQUAL(1u, toggles.size()) ;
		BOOST_CHECK(! keyboard_shortcuts.m_shortcuts_enabled) ;
		BOOST_CHECK_EQUAL(0u, deferred.pending_count()) ;
		BOOST_CHECK_EQUAL(0u, pump_messages()) ;
		BOOST_CHECK_EQUAL(0u, inner.m_sensing_variable.size()) ;
		deferred.destroy() ;
	}

BOOST_AUTO_TEST_SUITE_END()

#endif
