/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#ifndef OCTAVESYNTAXHIGHLIGHTER_H
#define OCTAVESYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QStringList>

namespace octave
{
class interpreter;
}

class octaveSyntaxHighlighter : public QSyntaxHighlighter
{
public:
    octaveSyntaxHighlighter(class octaveInterface* dataInterface=nullptr, QTextDocument *parent = nullptr);
protected:
    void highlightBlock(const QString& text) override;
private:
    class octaveInterface    *_data_interface;
    octave::interpreter      *_octave_interpreter;
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };


    QList<HighlightingRule> highlightingRules;
    // For function and vars we have common templates
    HighlightingRule functionRule;
    HighlightingRule varRule;
    HighlightingRule varDeclarationRule;

    // which is then specialized by checking
    QStringList            knownFunctions;
    QStringList            knownGlobalVariables;
    QStringList            knownVariables;

    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat unknownFunctionFormat;
    QTextCharFormat assignmentVarFormat;
    QTextCharFormat globalVarFormat;
    QTextCharFormat knownVarFormat;
    QTextCharFormat varFormat;



};

#endif // OCTAVESYNTAXHIGHLIGHTER_H
