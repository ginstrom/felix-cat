#pragma once

#pragma warning( disable:4146 )	// unary minus operator applied to unsigned type, result still unsigned
#import "libid:2DF8D04C-5BFA-101B-BDE5-00AA0044DE52" \
	rename_namespace("Office") \
	rename("RGB", "OfficeRGB") \
	rename( "DocumentProperties", "OfficeDocumentProperties" ) \
	rename( "SearchPath", "OfficeSearchPath" ) // , named_guids  
#pragma warning( default:4146 )

#import "libid:0002E157-0000-0000-C000-000000000046" \
	rename_namespace("VBE6") 

#import "libid:91493440-5A91-11CF-8700-00AA0060263B" \
	rename_namespace("PowerPoint"), rename( "RGB", "PowerPointRGB") 

#import "libid:00020813-0000-0000-C000-000000000046" \
	rename_namespace("Excel") \
	rename("Range","ExcelRange") \
	rename("DialogBox","ExcelDialogBox") \
	rename("RGB","ExcelRGB") \
	exclude("IFont","IPicture") \
	rename ("CopyFile", "ExcelCopyFile" ) \
	rename ("ReplaceText", "ExcelReplaceText" ) 

using namespace PowerPoint ;
