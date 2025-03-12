#include "octave_txt_lexer.h"

QString octave_txt_lexer::description (int style) const
{
	if (style == 0)
		return tr ("Default");
	else
		return QString ();
};

const char * octave_txt_lexer::language (void) const
{
	return "Text";
}

const char * octave_txt_lexer::lexer (void) const
{
	return "text";
}
