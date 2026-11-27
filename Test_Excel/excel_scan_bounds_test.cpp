#include "stdafx.h"
#include "../ExcelAssist/ExcelAssist/excel_scan_bounds.h"
#include "../ExcelAssist/ExcelAssist/fake_excel.h"

// The fakes in fake_excel.h exist only in Debug builds.
#if defined(UNIT_TEST) && defined(_DEBUG)
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( ExcelScanBoundsTest )

	// A fake sheet whose used range starts at (first_row, first_col) and
	// spans rows x cols cells.
	excel::sheet_ptr make_sheet( int first_row, int rows, int first_col, int cols )
	{
		excel::FakeWorksheet *sheet = new excel::FakeWorksheet ;
		sheet->m_first_row = first_row ;
		sheet->m_rows = rows ;
		sheet->m_first_col = first_col ;
		sheet->m_cols = cols ;
		return excel::sheet_ptr(sheet) ;
	}

	// QA repro: data in A10:A60 (51 rows). Auto Translate Sheet stopped at row 51.
	BOOST_AUTO_TEST_CASE( rows_reach_the_last_row_of_a_used_range_below_row_1 )
	{
		excel::sheet_ptr sheet = make_sheet( 10, 51, 1, 1 ) ;

		BOOST_CHECK_EQUAL( sheet->LastUsedRow(), 60 ) ;
		BOOST_CHECK_EQUAL( excel::last_row_to_scan( sheet, 1 ), 60 ) ;
		BOOST_CHECK_EQUAL( excel::last_row_to_scan( sheet, 10 ), 60 ) ;
	}

	BOOST_AUTO_TEST_CASE( columns_reach_the_last_column_of_a_used_range_right_of_column_1 )
	{
		// T1:AH1 is 15 columns, ending at column 34.
		excel::sheet_ptr sheet = make_sheet( 1, 1, 20, 15 ) ;

		BOOST_CHECK_EQUAL( sheet->LastUsedColumn(), 34 ) ;
		BOOST_CHECK_EQUAL( excel::last_column_to_scan( sheet ), 34 ) ;
	}

	BOOST_AUTO_TEST_CASE( used_range_from_a1_is_unchanged )
	{
		excel::sheet_ptr sheet = make_sheet( 1, 100, 1, 40 ) ;

		BOOST_CHECK_EQUAL( excel::last_row_to_scan( sheet, 1 ), 100 ) ;
		BOOST_CHECK_EQUAL( excel::last_column_to_scan( sheet ), 40 ) ;
	}

	BOOST_AUTO_TEST_CASE( small_used_range_still_extends_past_the_active_cell )
	{
		excel::sheet_ptr sheet = make_sheet( 2, 4, 2, 3 ) ;

		BOOST_CHECK_EQUAL( excel::last_row_to_scan( sheet, 3 ), 3 + excel::MAX_CELL_EXTEND ) ;
		BOOST_CHECK_EQUAL( excel::last_column_to_scan( sheet ), excel::MAX_CELL_EXTEND ) ;
	}

BOOST_AUTO_TEST_SUITE_END()

#endif
