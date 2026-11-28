#pragma once

#ifdef FELIX_NO_EXCEL

// Built without Microsoft Office: no Excel type library is available, so
// Excel export reports an error instead. Remove FELIX_NO_EXCEL from the
// project's preprocessor definitions once Office is installed.
#include "ExcelInterface.h"
#include "Exceptions.h"

class ExcelInterfaceReal : public ExcelInterface
{
public:
	ExcelInterfaceReal()
	{
		throw except::CProgramException(
			ATL::CString(L"Excel export is not available in this build of Felix "
				L"(it was built without Microsoft Office). "
				L"Export to tab-separated text instead."),
			_T(__FILE__), __LINE__) ;
	}
	void SaveAs(const CString) {}
	void SetCellText(int, int, const wstring) {}
	void MakeCellBold(int, int) {}
};

#else // FELIX_NO_EXCEL

// Office core (MSOffice), VBA extensibility (VBE6) and Excel, from headers
// generated from Office's type libraries, so building needs no Office.
// See common\office_typelibs\README.md.
#include "office_typelibs/msoffice_excel.h"

#include "ExcelInterface.h"
#include "logging.h"

//! Concrete Excel writer
class ExcelInterfaceReal : public ExcelInterface
{
	Excel::_ApplicationPtr	m_app ;
	Excel::_WorkbookPtr		m_book ;
	Excel::_WorksheetPtr	m_sheet ;

	bool m_needs_quitting;

public:
	ExcelInterfaceReal() :
	  m_needs_quitting(false)
	  {
		  m_app.CreateInstance( L"Excel.Application" ) ;

		  Excel::WorkbooksPtr wBooks = m_app->Workbooks ;

		  if ( wBooks->GetCount() == 0 ) 
		  {
			  m_needs_quitting = true ;
		  }

		  m_book = wBooks->Add( ) ;
		  m_sheet = m_book->GetActiveSheet() ;
	  }
	  ~ExcelInterfaceReal()
	  {
		  if (m_needs_quitting)
		  {
			  m_app->Quit() ;
		  }
	  }

	  void SaveAs(const CString location)
	  {
		  const _bstr_t save_loc(location) ;
		  _variant_t file_format(Excel::xlWorkbookNormal) ;
		  m_book->SaveAs( save_loc, 
			  file_format,
			  vtMissing,
			  vtMissing,
			  vtMissing,
			  vtMissing,
			  Excel::xlNoChange) ;
	  }

	  void SetCellText(int row, int col, const wstring text)
	  {
		  Excel::RangePtr cell = m_sheet->Cells->Item[row][col] ;

		  try
		  {
			  cell->GetCharacters()->Text = string2BSTR(text) ;
		  }
		  catch (_com_error& e)
		  {
			  logging::log_error("_com_error setting text in cell - using formula property") ;
			  logging::log_exception(e) ;
			  cell->FormulaR1C1 = _variant_t(string2BSTR(text)) ;
		  }

	  }
	  void MakeCellBold(int row, int col)
	  {
		  Excel::RangePtr cell = m_sheet->Cells->Item[row][col] ;
		  cell->Font->Bold = VARIANT_TRUE ;
	  }

};

#endif // FELIX_NO_EXCEL