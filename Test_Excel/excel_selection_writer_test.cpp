#include "stdafx.h"
#include "../ExcelAssist/ExcelAssist/excel_selection_writer.h"
#include "../ExcelAssist/ExcelAssist/fake_excel.h"

// The fakes in fake_excel.h exist only in Debug builds.
#if defined(UNIT_TEST) && defined(_DEBUG)
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( ExcelSelectionWriterTest )

	struct writer_fixture
	{
		std::shared_ptr< excel::FakeApplication > fake_app ;
		excel::app_ptr app ;

		writer_fixture() :
			fake_app(new excel::FakeApplication),
			app(fake_app)
		{
			formula_bar_left_hidden() = false ;
		}
		~writer_fixture()
		{
			formula_bar_left_hidden() = false ;
		}

		excel::FakeCharacters* add_selected_chars()
		{
			excel::FakeCharacters *chars = new excel::FakeCharacters ;
			fake_app->chars = excel::chars_ptr(chars) ;
			return chars ;
		}

		excel::FakeRange* add_active_cell()
		{
			excel::FakeRange *cell = new excel::FakeRange ;
			fake_app->active_cell = excel::range_ptr(cell) ;
			return cell ;
		}
	};

	BOOST_AUTO_TEST_CASE( writes_selected_chars_and_refreshes_once )
	{
		writer_fixture f ;
		excel::FakeCharacters *chars = f.add_selected_chars() ;
		excel::FakeRange *cell = f.add_active_cell() ;

		BOOST_CHECK( write_selection_text(f.app, _bstr_t(L"spam")) ) ;

		BOOST_CHECK( chars->text == _bstr_t(L"spam") ) ;
		BOOST_CHECK( cell->text.vt == VT_EMPTY ) ;
		BOOST_CHECK_EQUAL( f.fake_app->refresh_count, 1 ) ;
	}

	BOOST_AUTO_TEST_CASE( falls_back_to_active_cell_when_chars_write_fails )
	{
		writer_fixture f ;
		excel::FakeRange *cell = f.add_active_cell() ;

		BOOST_CHECK( write_selection_text(f.app, _bstr_t(L"eggs")) ) ;

		BOOST_CHECK( cell->text.vt == VT_BSTR && wstring(cell->text.bstrVal) == L"eggs" ) ;
		BOOST_CHECK_EQUAL( f.fake_app->refresh_count, 1 ) ;
	}

	BOOST_AUTO_TEST_CASE( empty_text_writes_nothing_and_does_not_refresh )
	{
		writer_fixture f ;
		excel::FakeCharacters *chars = f.add_selected_chars() ;
		chars->text = L"original" ;

		BOOST_CHECK( ! write_selection_text(f.app, _bstr_t(L"")) ) ;

		BOOST_CHECK( chars->text == _bstr_t(L"original") ) ;
		BOOST_CHECK_EQUAL( f.fake_app->refresh_count, 0 ) ;
	}

	BOOST_AUTO_TEST_CASE( refresh_failure_is_swallowed_and_text_is_written )
	{
		writer_fixture f ;
		excel::FakeCharacters *chars = f.add_selected_chars() ;
		f.fake_app->refresh_throws = true ;

		bool result = false ;
		BOOST_CHECK_NO_THROW( result = write_selection_text(f.app, _bstr_t(L"ham")) ) ;

		BOOST_CHECK( result ) ;
		BOOST_CHECK( chars->text == _bstr_t(L"ham") ) ;
		BOOST_CHECK_EQUAL( f.fake_app->refresh_count, 1 ) ;
	}

	// Auto Translate writes cells through set_cell_text, which does not
	// update the formula bar; the refresher does it once on scope exit.
	BOOST_AUTO_TEST_CASE( formula_bar_refresher_refreshes_once_on_scope_exit )
	{
		writer_fixture f ;
		{
			formula_bar_refresher refresher(f.app) ;
			BOOST_CHECK_EQUAL( f.fake_app->refresh_count, 0 ) ;
		}
		BOOST_CHECK_EQUAL( f.fake_app->refresh_count, 1 ) ;
	}

	BOOST_AUTO_TEST_CASE( formula_bar_refresher_swallows_refresh_failure )
	{
		writer_fixture f ;
		f.fake_app->refresh_throws = true ;
		BOOST_CHECK_NO_THROW( { formula_bar_refresher refresher(f.app) ; } ) ;
		BOOST_CHECK_EQUAL( f.fake_app->refresh_count, 1 ) ;
	}

	BOOST_AUTO_TEST_CASE( formula_bar_refresher_ignores_null_app )
	{
		BOOST_CHECK_NO_THROW( { formula_bar_refresher refresher((excel::app_ptr())) ; } ) ;
	}

	// Auto Translate refreshes the formula bar as soon as the active cell is written (#10).
	BOOST_AUTO_TEST_CASE( refresh_if_active_refreshes_when_same_row_and_column )
	{
		writer_fixture f ;
		excel::FakeRange *active = f.add_active_cell() ;
		active->row = 10 ;
		active->col = 1 ;
		excel::FakeRange *written = new excel::FakeRange ;
		written->row = 10 ;
		written->col = 1 ;

		refresh_formula_bar_if_active(f.app, excel::range_ptr(written)) ;

		BOOST_CHECK_EQUAL( f.fake_app->refresh_count, 1 ) ;
	}

	BOOST_AUTO_TEST_CASE( refresh_if_active_skips_different_row_or_column )
	{
		writer_fixture f ;
		excel::FakeRange *active = f.add_active_cell() ;
		active->row = 10 ;
		active->col = 1 ;

		excel::FakeRange *other_row = new excel::FakeRange ;
		other_row->row = 11 ;
		other_row->col = 1 ;
		refresh_formula_bar_if_active(f.app, excel::range_ptr(other_row)) ;

		excel::FakeRange *other_col = new excel::FakeRange ;
		other_col->row = 10 ;
		other_col->col = 2 ;
		refresh_formula_bar_if_active(f.app, excel::range_ptr(other_col)) ;

		BOOST_CHECK_EQUAL( f.fake_app->refresh_count, 0 ) ;
	}

	BOOST_AUTO_TEST_CASE( refresh_if_active_swallows_missing_active_cell )
	{
		writer_fixture f ;
		excel::FakeRange *written = new excel::FakeRange ;
		written->row = 10 ;
		written->col = 1 ;

		BOOST_CHECK_NO_THROW( refresh_formula_bar_if_active(f.app, excel::range_ptr(written)) ) ;
		BOOST_CHECK_EQUAL( f.fake_app->refresh_count, 0 ) ;
	}

	BOOST_AUTO_TEST_CASE( refresh_if_active_swallows_refresh_failure )
	{
		writer_fixture f ;
		excel::FakeRange *active = f.add_active_cell() ;
		active->row = 3 ;
		active->col = 4 ;
		excel::FakeRange *written = new excel::FakeRange ;
		written->row = 3 ;
		written->col = 4 ;
		f.fake_app->refresh_throws = true ;

		BOOST_CHECK_NO_THROW( refresh_formula_bar_if_active(f.app, excel::range_ptr(written)) ) ;
		BOOST_CHECK_EQUAL( f.fake_app->refresh_count, 1 ) ;
	}

	// #10: Activate alone leaves the formula bar stale until the command
	// returns; hiding and showing the formula bar makes Excel reload it now.
	BOOST_AUTO_TEST_CASE( refresh_if_active_reloads_shown_formula_bar )
	{
		writer_fixture f ;
		excel::FakeRange *active = f.add_active_cell() ;
		active->row = 10 ;
		active->col = 1 ;
		excel::FakeRange *written = new excel::FakeRange ;
		written->row = 10 ;
		written->col = 1 ;

		refresh_formula_bar_if_active(f.app, excel::range_ptr(written)) ;

		BOOST_CHECK_EQUAL( f.fake_app->formula_bar_sets.size(), 2u ) ;
		if ( f.fake_app->formula_bar_sets.size() == 2u )
		{
			BOOST_CHECK( ! f.fake_app->formula_bar_sets[0] ) ;
			BOOST_CHECK( f.fake_app->formula_bar_sets[1] ) ;
		}
		BOOST_CHECK( f.fake_app->formula_bar_shown ) ;
	}

	BOOST_AUTO_TEST_CASE( refresh_if_active_leaves_hidden_formula_bar_hidden )
	{
		writer_fixture f ;
		excel::FakeRange *active = f.add_active_cell() ;
		active->row = 10 ;
		active->col = 1 ;
		excel::FakeRange *written = new excel::FakeRange ;
		written->row = 10 ;
		written->col = 1 ;
		f.fake_app->formula_bar_shown = false ;

		refresh_formula_bar_if_active(f.app, excel::range_ptr(written)) ;

		BOOST_CHECK( f.fake_app->formula_bar_sets.empty() ) ;
		BOOST_CHECK( ! f.fake_app->formula_bar_shown ) ;
	}

	BOOST_AUTO_TEST_CASE( refresh_if_active_does_not_reload_for_other_cell )
	{
		writer_fixture f ;
		excel::FakeRange *active = f.add_active_cell() ;
		active->row = 10 ;
		active->col = 1 ;
		excel::FakeRange *written = new excel::FakeRange ;
		written->row = 11 ;
		written->col = 1 ;

		refresh_formula_bar_if_active(f.app, excel::range_ptr(written)) ;

		BOOST_CHECK( f.fake_app->formula_bar_sets.empty() ) ;
	}

	BOOST_AUTO_TEST_CASE( refresh_if_active_shows_formula_bar_again_after_failed_show )
	{
		writer_fixture f ;
		excel::FakeRange *active = f.add_active_cell() ;
		active->row = 10 ;
		active->col = 1 ;
		excel::FakeRange *written = new excel::FakeRange ;
		written->row = 10 ;
		written->col = 1 ;
		f.fake_app->show_failures = 1 ;

		BOOST_CHECK_NO_THROW( refresh_formula_bar_if_active(f.app, excel::range_ptr(written)) ) ;

		BOOST_CHECK( f.fake_app->formula_bar_shown ) ;
	}

	BOOST_AUTO_TEST_CASE( refresh_if_active_swallows_formula_bar_query_failure )
	{
		writer_fixture f ;
		excel::FakeRange *active = f.add_active_cell() ;
		active->row = 10 ;
		active->col = 1 ;
		excel::FakeRange *written = new excel::FakeRange ;
		written->row = 10 ;
		written->col = 1 ;
		f.fake_app->formula_bar_query_throws = true ;

		BOOST_CHECK_NO_THROW( refresh_formula_bar_if_active(f.app, excel::range_ptr(written)) ) ;
		BOOST_CHECK( f.fake_app->formula_bar_sets.empty() ) ;
	}

	// If both shows fail after the hide, the bar is shown again when the
	// command returns; Excel saves the setting, so it must not stay hidden.
	BOOST_AUTO_TEST_CASE( refresher_shows_formula_bar_left_hidden_by_reload )
	{
		writer_fixture f ;
		excel::FakeRange *active = f.add_active_cell() ;
		active->row = 10 ;
		active->col = 1 ;
		excel::FakeRange *written = new excel::FakeRange ;
		written->row = 10 ;
		written->col = 1 ;
		f.fake_app->show_failures = 2 ;

		{
			formula_bar_refresher refresher(f.app) ;
			BOOST_CHECK_NO_THROW( refresh_formula_bar_if_active(f.app, excel::range_ptr(written)) ) ;
			BOOST_CHECK( ! f.fake_app->formula_bar_shown ) ;
		}

		BOOST_CHECK( f.fake_app->formula_bar_shown ) ;
		BOOST_CHECK( ! formula_bar_left_hidden() ) ;
	}

	BOOST_AUTO_TEST_CASE( refresher_keeps_flag_when_show_fails_again )
	{
		writer_fixture f ;
		excel::FakeRange *active = f.add_active_cell() ;
		active->row = 10 ;
		active->col = 1 ;
		excel::FakeRange *written = new excel::FakeRange ;
		written->row = 10 ;
		written->col = 1 ;
		f.fake_app->show_failures = 3 ;

		{
			formula_bar_refresher refresher(f.app) ;
			refresh_formula_bar_if_active(f.app, excel::range_ptr(written)) ;
		}

		// Never throws; the next command's refresher tries again.
		BOOST_CHECK( ! f.fake_app->formula_bar_shown ) ;
		BOOST_CHECK( formula_bar_left_hidden() ) ;
		{
			formula_bar_refresher refresher(f.app) ;
		}
		BOOST_CHECK( f.fake_app->formula_bar_shown ) ;
		BOOST_CHECK( ! formula_bar_left_hidden() ) ;
	}

	BOOST_AUTO_TEST_CASE( refresher_does_not_show_bar_it_did_not_hide )
	{
		writer_fixture f ;
		f.fake_app->formula_bar_shown = false ;
		{
			formula_bar_refresher refresher(f.app) ;
		}
		BOOST_CHECK( ! f.fake_app->formula_bar_shown ) ;
		BOOST_CHECK( f.fake_app->formula_bar_sets.empty() ) ;
	}

	BOOST_AUTO_TEST_CASE( end_of_command_refreshes_do_not_reload_formula_bar )
	{
		writer_fixture f ;
		f.add_selected_chars() ;
		write_selection_text(f.app, _bstr_t(L"text")) ;
		{
			formula_bar_refresher refresher(f.app) ;
		}

		BOOST_CHECK( f.fake_app->formula_bar_sets.empty() ) ;
	}

BOOST_AUTO_TEST_SUITE_END()

#endif
