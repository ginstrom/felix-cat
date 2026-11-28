#pragma once

#include "excel_application.h"

#ifdef _DEBUG

namespace excel
{
	class FakeRange : public range
	{
	public:
		int row ;
		int col ;
		_variant_t text ;

		int Row()
		{
			return row ;
		}
		int Column()
		{
			return col ;
		}
		_variant_t GetText()
		{
			return this->text ;
		}
		void SetText( const _variant_t text_to_set )
		{
			this->text = text_to_set ;
		}

	};
	class FakeWorksheet : public worksheet
	{
	public:
		_bstr_t m_name ;
		int m_rows ;
		int m_cols ;
		int m_first_row ;
		int m_first_col ;

		std::vector< string > m_calls ;

		FakeWorksheet():
			m_rows(0),
			m_cols(0),
			m_first_row(1),
			m_first_col(1)
		{

		}
		_bstr_t GetName()
		{
			return m_name ;
		}

		void SetName(_bstr_t name)
		{
			m_name  = name ;
		}

		void Activate()
		{
			m_calls.push_back("Activate") ;
		}

		range_ptr Cell( int /*row*/, int /*col*/ )
		{
			range_ptr cell = range_ptr(new range) ;
			return cell ;
		}

		virtual int UsedRows()
		{
			return m_rows ;
		}
		virtual int UsedColumns()
		{
			return m_cols ;
		}
		virtual int UsedFirstRow()
		{
			return m_first_row ;
		}
		virtual int UsedFirstColumn()
		{
			return m_first_col ;
		}
	};

	//! Fake Excel characters class
	class FakeCharacters : public characters
	{
	public:
		_bstr_t text ;

		_bstr_t GetText()
		{
			return this->text ;
		}
		void SetText(_bstr_t text_to_set ) 
		{
			this->text = text_to_set ;
		}
	};

	//! Fake Excel application class.
	class FakeApplication : public application
	{
	public:
		sheet_ptr active_sheet ;
		chars_ptr chars ;
		range_ptr active_cell ;
		int refresh_count ;
		bool refresh_throws ;
		bool formula_bar_shown ;
		bool formula_bar_query_throws ;
		int show_failures ; // times ShowFormulaBar(true) throws before it works
		std::vector<bool> formula_bar_sets ;

		FakeApplication() :
			active_sheet(new FakeWorksheet),
			refresh_count(0),
			refresh_throws(false),
			formula_bar_shown(true),
			formula_bar_query_throws(false),
			show_failures(0)
			{

			}

		bool FormulaBarShown()
		{
			if ( formula_bar_query_throws )
			{
				throw _com_error(E_FAIL) ;
			}
			return formula_bar_shown ;
		}

		void ShowFormulaBar(bool show)
		{
			formula_bar_sets.push_back(show) ;
			if ( show && show_failures > 0 )
			{
				--show_failures ;
				throw _com_error(E_FAIL) ;
			}
			formula_bar_shown = show ;
		}

		void RefreshActiveCell()
		{
			++refresh_count ;
			if ( refresh_throws )
			{
				throw _com_error(E_FAIL) ;
			}
		}

		sheet_ptr GetActiveSheet()
		{
			return active_sheet ;
		}
		chars_ptr GetSelectedChars()
		{
			if (! this->chars )
			{
				throw _com_error(E_FAIL) ;
			}
			return this->chars ;
		}

		range_ptr GetSelectedCell()
		{
			if ( ! this->active_cell )
			{
				throw _com_error(E_FAIL) ;
			}
			return this->active_cell ;
		}
		range_ptr GetActiveCell()
		{
			if ( ! this->active_cell )
			{
				throw _com_error(E_FAIL) ;
			}
			return this->active_cell ;
		}
	};
}
#endif