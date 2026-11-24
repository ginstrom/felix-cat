#pragma once


// Type libraries are located through the registry (requires Office installed
// on the build machine). Was: Office 2000 OLBs at hardcoded D:\dev paths.
// Office core (MSO.DLL)
#import "libid:2DF8D04C-5BFA-101B-BDE5-00AA0044DE52"	\
	rename_namespace("MSOffice") rename("RGB", "OfficeRGB") rename( "SearchPath", "OfficeSearchPath" ) rename("DocumentProperties", "OfficeDocumentProperties") named_guids

// VBA extensibility (VBE6EXT.OLB)
#import "libid:0002E157-0000-0000-C000-000000000046" \
	rename_namespace("VBE6")

// Excel
#import "libid:00020813-0000-0000-C000-000000000046" \
	exclude( "IFont" ) exclude( "IPicture" )\
	rename("DialogBox", "DialogBoxForExcel") rename( "RGB", "RGBForExcel") \
	rename( "CopyFile", "CopyFileForExcel" ) rename( "ReplaceText", "ReplaceTextForExcel" ) \

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