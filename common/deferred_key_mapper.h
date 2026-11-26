#pragma once

// Runs shortcut commands outside the keyboard hook (issue #8).
//
// The hook procedures call KeyboardShortcuts::receive_keypress, which calls
// KeyMapper::map_command. Commands that make cross-process COM calls fail with
// RPC_E_CANTCALLOUT_ININPUTSYNCCALL (0x80010005) when run inside the hook, so
// this decorator decides synchronously (accepts), queues the command, and runs
// it later from a message-only window on the same (Office UI) thread.

#include "key_mapper.h"
#include "keyboard_shortcuts.h"
#include "logging.h"
#include "Exceptions.h"
#include <deque>
#include <utility>

namespace shortcuts
{
	const UINT WM_FELIX_RUN_COMMANDS = WM_APP + 0x0F31 ;

	class DeferredKeyMapper : public KeyMapper,
	                          public CWindowImpl<DeferredKeyMapper, CWindow, CNullTraits>
	{
	public:
		// Reads the Shift key when a keystroke is queued (R4). Tests replace it
		// to simulate the physical key.
		boost::function<bool()> m_read_keyboard_shift ;

		explicit DeferredKeyMapper(KeyMapper *inner) :
			m_read_keyboard_shift(&physical_shift_key_is_pressed),
			m_inner(inner),
			m_posted(false),
			m_running(false)
		{
		}
		~DeferredKeyMapper()
		{
			destroy() ;
		}

		BEGIN_MSG_MAP(DeferredKeyMapper)
			MESSAGE_HANDLER(WM_FELIX_RUN_COMMANDS, OnRunCommands)
		END_MSG_MAP()

		// Creates a message-only window (HWND_MESSAGE) on the calling thread.
		// On failure, logs and returns false; commands then run synchronously.
		bool create()
		{
			if (IsWindow())
			{
				return true ;
			}
			if (Create(HWND_MESSAGE) == NULL)
			{
				logging::log_error("Felix hook: could not create the command window (error "
					+ boost::lexical_cast<string>(::GetLastError())
					+ "); shortcut commands will run inside the keyboard hook") ;
				return false ;
			}
			return true ;
		}

		// Destroys the window and drops queued commands.
		void destroy()
		{
			m_queue.clear() ;
			m_posted = false ;
			if (IsWindow())
			{
				DestroyWindow() ;
			}
			m_hWnd = NULL ;
		}

		// Called from the keyboard hook (via receive_keypress). The hook can run
		// while a deferred command and its Shift override are running (Office
		// pumps messages inside commands), so the override is suspended here:
		// accepts, the Shift capture and a synchronous run see the real keyboard.
		bool map_command(string command)
		{
			scoped_physical_shift_state physical_shift ;
			if (! m_inner->accepts(command))
			{
				logging::log_debug("Felix hook: declined " + command) ;
				return false ;
			}
			if (! IsWindow())
			{
				return m_inner->map_command(command) ;
			}
			m_queue.push_back(std::make_pair(command, m_read_keyboard_shift())) ;
			if (! m_posted)
			{
				if (PostMessage(WM_FELIX_RUN_COMMANDS))
				{
					m_posted = true ;
				}
				else
				{
					logging::log_warn("Felix hook: could not post the command message (error "
						+ boost::lexical_cast<string>(::GetLastError()) + ")") ;
				}
			}
			logging::log_debug("Felix hook: queued " + command) ;
			return true ;
		}

		bool accepts(const string &command)
		{
			return m_inner->accepts(command) ;
		}

		// Runs the queued commands in order, one at a time, each with the Shift
		// state of its keystroke. A call made while a command is running (Office
		// pumping messages inside it) returns at once; the outer call runs the rest.
		void run_pending()
		{
			m_posted = false ;
			if (m_running)
			{
				return ;
			}
			m_running = true ;
			while (! m_queue.empty())
			{
				const std::pair<string, bool> item = m_queue.front() ;
				m_queue.pop_front() ;
				scoped_shift_state shift(item.second) ;
				run_one(item.first) ;
			}
			m_running = false ;
		}

		size_t pending_count() const
		{
			return m_queue.size() ;
		}

	private:
		LRESULT OnRunCommands(UINT, WPARAM, LPARAM, BOOL &)
		{
			run_pending() ;
			return 0 ;
		}

		void run_one(const string &command)
		{
			try
			{
				m_inner->map_command(command) ;
			}
			catch (_com_error &e)
			{
				logging::log_error("Felix hook: command failed: " + command) ;
				logging::log_exception(e) ;
			}
			catch (except::CException &e)
			{
				logging::log_error("Felix hook: command failed: " + command) ;
				logging::log_exception(e) ;
			}
			catch (std::exception &e)
			{
				logging::log_error("Felix hook: command failed: " + command) ;
				logging::log_exception(e) ;
			}
			catch (...)
			{
				logging::log_error("Felix hook: command failed with an unknown exception: " + command) ;
			}
		}

		KeyMapper *m_inner ;
		std::deque<std::pair<string, bool> > m_queue ;
		bool m_posted ;
		bool m_running ;
	} ;
}
