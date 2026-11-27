#pragma once

#include <algorithm>
#include "excel_worksheet.h"

namespace excel
{
	// Felix scans at least this many rows past the active cell, and at least
	// this many columns, even when the used range is smaller.
	static const int MAX_CELL_EXTEND = 30 ;

	// Last row to scan (Look Up Next, Auto Translate Sheet) from active_row:
	// the used range's last row, not its row count, since the used range
	// need not start at row 1.
	inline int last_row_to_scan( sheet_ptr sheet, int active_row )
	{
		return (std::max)( sheet->LastUsedRow(), active_row + MAX_CELL_EXTEND ) ;
	}

	// Last column to scan (Look Up Next, Auto Translate Sheet): the used
	// range's last column, not its column count.
	inline int last_column_to_scan( sheet_ptr sheet )
	{
		return (std::max)( sheet->LastUsedColumn(), MAX_CELL_EXTEND ) ;
	}
}
