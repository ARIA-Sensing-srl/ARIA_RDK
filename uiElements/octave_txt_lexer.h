#ifndef OCTAVE_TXT_LEXER_H
#define OCTAVE_TXT_LEXER_H
#include <Qsci/qscilexeroctave.h>

class octave_txt_lexer : public QsciLexerOctave
{
	Q_OBJECT

public:
	octave_txt_lexer(QObject *parent = 0) : QsciLexerOctave(parent) {};
	virtual const char * language (void) const;

	virtual const char * lexer (void) const;

	virtual QString description (int style) const;
};

#endif // OCTAVE_TXT_LEXER_H
