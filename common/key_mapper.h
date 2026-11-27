#pragma once

class KeyMapper
{
public:
	virtual ~KeyMapper(){}
	virtual bool map_command(string command) = 0 ;
	// Called from the keyboard hook. Returns false if the keystroke for
	// `command` should go to the Office app instead. Must make no
	// cross-process COM calls; in-process Office object-model calls are allowed.
	virtual bool accepts(const string &command) { command ; return true ; }
};