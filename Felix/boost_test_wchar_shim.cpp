// Felix is compiled with wchar_t as a typedef for unsigned short
// (TreatWChar_tAsBuiltInType=false, /Zc:wchar_t-), but the prebuilt Boost.Test
// libraries are compiled with wchar_t as a built-in type.
//
// Boost.Test declares an out-of-line printer for wchar_t. Under /Zc:wchar_t-
// that declaration becomes print_log_value<unsigned short>, which the library
// doesn't contain, so any BOOST_CHECK_EQUAL on a WORD (e.g. SYSTEMTIME::wYear)
// fails to link. This supplies the missing definition. It prints the value as a
// number, which is the right output for WORD fields.
//
// Delete this file if the project is switched to a built-in wchar_t.
#include "stdafx.h"

#ifndef _NATIVE_WCHAR_T_DEFINED

#include <boost/test/tools/detail/print_helper.hpp>

namespace boost { namespace test_tools { namespace tt_detail {

void print_log_value<wchar_t>::operator()( std::ostream& ostr, wchar_t t )
{
	ostr << static_cast<unsigned int>(t) ;
}

}}}

#endif
