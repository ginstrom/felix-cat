/*!
 * <File comment goes here!!>
 * 
 * Copyright (c) 2005 by <your name/ organization here>
 */
#pragma once

#import "libid:2DF8D04C-5BFA-101B-BDE5-00AA0044DE52"	\
	rename_namespace("MSOffice") rename("RGB", "OfficeRGB") rename( "SearchPath", "OfficeSearchPath" ) rename("DocumentProperties", "OfficeDocumentProperties") named_guids    

#import "libid:0002E157-0000-0000-C000-000000000046" \
	rename_namespace("VBE6") 

#import "libid:00020813-0000-0000-C000-000000000046" \
	exclude( "IFont" ) exclude( "IPicture" )\
	rename("DialogBox", "DialogBoxForExcel") rename( "RGB", "RGBForExcel") \
	rename( "CopyFile", "CopyFIleForExcel" ) rename( "ReplaceText", "ReplaceTextForExcel" ) \

static const GUID DIID__CommandBarButtonEvents = {0x000C0351,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};


