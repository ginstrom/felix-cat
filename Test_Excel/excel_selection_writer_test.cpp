#include "stdafx.h"
#include "../ExcelAssist/ExcelAssist/excel_selection_writer.h"
#include "../ExcelAssist/ExcelAssist/fake_excel.h"

#ifdef UNIT_TEST
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

BOOST_AUTO_TEST_SUITE_END()

#endif
