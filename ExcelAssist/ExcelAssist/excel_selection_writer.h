#pragma once

#include "excel_application.h"
#include "FelixString.h"
#include "logging.h"
#include "stringconversions.h"

/*!
 * Writes text into a cell, falling back to the cell's formula
 * if writing its characters fails.
 */
inline void set_cell_text(excel::range_ptr cell, _bstr_t text)
{
	CFelixString felix_text(text) ;
	try
	{
		cell->SetText(felix_text.as_variant())  ;
	}
	catch (_com_error& e)
	{
		logging::log_error("COM exception setting cell text") ;
		logging::log_exception(e) ;
		cell->SetFormula(felix_text.as_variant()) ;
	}
}

/*!
 * Asks Excel to refresh the formula bar for the active cell.
 * A failure is logged and ignored; this never throws.
 */
inline void refresh_formula_bar(excel::app_ptr app)
{
	try
	{
		app->RefreshActiveCell() ;
	}
	catch ( _com_error &e )
	{
		// Logged at warn level on purpose: log_exception() would log at error
		// level, and the Debug post-build step treats "COM error 0x...:" output
		// from the refresh_throws test as a build error.
		CString msg ;
		msg.Format(_T("Failed to refresh the formula bar after writing the cell: COM exception 0x%08X: %s"),
			e.Error(), static_cast<LPCTSTR>(e.ErrorMessage())) ;
		logging::log_warn(string2string(static_cast<LPCWSTR>(msg))) ;
	}
	catch ( ... )
	{
		logging::log_warn("Failed to refresh the formula bar after writing the cell") ;
	}
}

/*!
 * If `written` is the application's active cell (same row and column),
 * refreshes the formula bar now. Never throws; failures are logged.
 */
inline void refresh_formula_bar_if_active(excel::app_ptr app, excel::range_ptr written)
{
	try
	{
		excel::range_ptr active = app->GetActiveCell() ;
		if ( written->Row() == active->Row() && written->Column() == active->Column() )
		{
			refresh_formula_bar(app) ;
		}
	}
	catch ( _com_error &e )
	{
		// Warn level on purpose; see refresh_formula_bar.
		CString msg ;
		msg.Format(_T("Failed to check the active cell before refreshing the formula bar: COM exception 0x%08X: %s"),
			e.Error(), static_cast<LPCTSTR>(e.ErrorMessage())) ;
		logging::log_warn(string2string(static_cast<LPCWSTR>(msg))) ;
	}
	catch ( ... )
	{
		logging::log_warn("Failed to check the active cell before refreshing the formula bar") ;
	}
}

/*!
 * Writes text into the current selection (a cell or a shape such as a
 * text box), falling back to the active cell, then asks Excel to refresh
 * the formula bar.
 *
 * Returns false and writes nothing if the text is empty. A failure to
 * refresh the formula bar is logged and ignored.
 */
inline bool write_selection_text(excel::app_ptr app, _bstr_t text)
{
	if (text.length() == 0)
	{
		return false ;
	}

	CFelixString felix_text(text) ;

	// First we try writing to the Selection.
	try
	{
		app->GetSelectedChars()->SetText(felix_text.as_bstr()) ;
	}
	// If that fails, we try writing to the active cell
	catch ( ... )
	{
		set_cell_text(app->GetActiveCell(), text) ;
	}

	// Writing through Characters does not update the formula bar (#3).
	refresh_formula_bar(app) ;
	return true ;
}

/*!
 * Refreshes the formula bar once when it goes out of scope, for commands
 * such as Auto Translate that write cells with set_cell_text (through
 * Characters, which leaves the formula bar stale) and may return early.
 */
class formula_bar_refresher
{
	excel::app_ptr m_app ;
	formula_bar_refresher(const formula_bar_refresher&) ;
	formula_bar_refresher& operator=(const formula_bar_refresher&) ;
public:
	explicit formula_bar_refresher(excel::app_ptr app) : m_app(app) {}
	~formula_bar_refresher()
	{
		if ( m_app )
		{
			refresh_formula_bar(m_app) ;
		}
	}
};
