#include "stdafx.h"
#include "keyboard_shortcuts.h"

#ifdef UNIT_TEST

#include <boost/test/unit_test.hpp>

namespace
{
	const wchar_t *MAIN_CLASS = L"FelixTestMain" ;
	const wchar_t *OTHER_CLASS = L"FelixTestOther" ;

	LRESULT CALLBACK test_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		return ::DefWindowProcW(hwnd, msg, wparam, lparam) ;
	}

	// Registers two private window classes and destroys every window it made.
	struct WindowFixture
	{
		HINSTANCE m_instance ;
		std::vector<HWND> m_windows ;

		static void register_class(HINSTANCE instance, const wchar_t *name)
		{
			WNDCLASSW wc = { 0 } ;
			wc.lpfnWndProc = test_wnd_proc ;
			wc.hInstance = instance ;
			wc.lpszClassName = name ;
			::RegisterClassW(&wc) ;
		}

		WindowFixture() : m_instance(::GetModuleHandleW(NULL))
		{
			register_class(m_instance, MAIN_CLASS) ;
			register_class(m_instance, OTHER_CLASS) ;
		}
		~WindowFixture()
		{
			for (auto it = m_windows.rbegin() ; it != m_windows.rend() ; ++it)
			{
				if (::IsWindow(*it))
				{
					::DestroyWindow(*it) ;
				}
			}
			::UnregisterClassW(MAIN_CLASS, m_instance) ;
			::UnregisterClassW(OTHER_CLASS, m_instance) ;
		}

		HWND make(const wchar_t *cls, DWORD style, HWND parent)
		{
			HWND hwnd = ::CreateWindowExW(0, cls, L"", style,
				0, 0, 100, 100, parent, NULL, m_instance, NULL) ;
			BOOST_REQUIRE(hwnd != NULL) ;
			m_windows.push_back(hwnd) ;
			return hwnd ;
		}
		HWND make_main()
		{
			return make(MAIN_CLASS, WS_OVERLAPPEDWINDOW, NULL) ;
		}
		HWND make_child(HWND parent)
		{
			return make(OTHER_CLASS, WS_CHILD, parent) ;
		}
	};
}

BOOST_AUTO_TEST_SUITE( KeystrokeTargetTest )

	BOOST_FIXTURE_TEST_CASE(null_focus_is_not_main_window, WindowFixture)
	{
		BOOST_CHECK(! shortcuts::keystroke_target_is_main_window(NULL, MAIN_CLASS)) ;
	}

	BOOST_FIXTURE_TEST_CASE(main_window_itself_is_main_window, WindowFixture)
	{
		HWND main_wnd = make_main() ;
		BOOST_CHECK(shortcuts::keystroke_target_is_main_window(main_wnd, MAIN_CLASS)) ;
	}

	BOOST_FIXTURE_TEST_CASE(child_of_main_window_is_main_window, WindowFixture)
	{
		HWND main_wnd = make_main() ;
		HWND child = make_child(main_wnd) ;
		BOOST_CHECK(shortcuts::keystroke_target_is_main_window(child, MAIN_CLASS)) ;
	}

	BOOST_FIXTURE_TEST_CASE(grandchild_of_main_window_is_main_window, WindowFixture)
	{
		HWND main_wnd = make_main() ;
		HWND child = make_child(main_wnd) ;
		HWND grandchild = make_child(child) ;
		BOOST_CHECK(shortcuts::keystroke_target_is_main_window(grandchild, MAIN_CLASS)) ;
	}

	BOOST_FIXTURE_TEST_CASE(child_of_disabled_main_window_is_not_main_window, WindowFixture)
	{
		HWND main_wnd = make_main() ;
		HWND child = make_child(main_wnd) ;
		::EnableWindow(main_wnd, FALSE) ;
		BOOST_CHECK(! shortcuts::keystroke_target_is_main_window(child, MAIN_CLASS)) ;
	}

	BOOST_FIXTURE_TEST_CASE(owned_popup_of_other_class_is_not_main_window, WindowFixture)
	{
		HWND main_wnd = make_main() ;
		HWND popup = make(OTHER_CLASS, WS_POPUP, main_wnd) ;
		BOOST_CHECK(! shortcuts::keystroke_target_is_main_window(popup, MAIN_CLASS)) ;
	}

	BOOST_FIXTURE_TEST_CASE(class_name_match_ignores_case, WindowFixture)
	{
		HWND main_wnd = make_main() ;
		BOOST_CHECK(shortcuts::keystroke_target_is_main_window(main_wnd, L"felixtestmain")) ;
	}

	BOOST_FIXTURE_TEST_CASE(wrong_class_name_is_not_main_window, WindowFixture)
	{
		HWND main_wnd = make_main() ;
		BOOST_CHECK(! shortcuts::keystroke_target_is_main_window(main_wnd, L"SomeOtherClass")) ;
	}

BOOST_AUTO_TEST_SUITE_END()

#endif
