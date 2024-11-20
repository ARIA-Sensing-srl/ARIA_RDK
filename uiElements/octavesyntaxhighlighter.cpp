/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include "octavesyntaxhighlighter.h"
#include "octaveinterface.h"
#include <utility>
#include <octave.h>
#include <ov.h>
#include <interpreter.h>

using namespace octave;

octaveSyntaxHighlighter::octaveSyntaxHighlighter(octaveInterface* dataInterface,QTextDocument *parent)
    : QSyntaxHighlighter(parent), _data_interface(dataInterface)
{
    HighlightingRule rule;

    keywordFormat.setForeground(Qt::darkBlue);
    keywordFormat.setFontWeight(QFont::Bold);
    const QString keywordPatterns[] = {
        QStringLiteral("\\bchar\\b"), QStringLiteral("\\bclass\\b"), QStringLiteral("\\bconst\\b"),
        QStringLiteral("\\bdouble\\b"), QStringLiteral("\\benum\\b"), QStringLiteral("\\bexplicit\\b"),
        QStringLiteral("\\bfriend\\b"), QStringLiteral("\\binline\\b"), QStringLiteral("\\bint\\b"),
        QStringLiteral("\\blong\\b"), QStringLiteral("\\bnamespace\\b"), QStringLiteral("\\boperator\\b"),
        QStringLiteral("\\bprivate\\b"), QStringLiteral("\\bprotected\\b"), QStringLiteral("\\bpublic\\b"),
        QStringLiteral("\\bshort\\b"), QStringLiteral("\\bsignals\\b"), QStringLiteral("\\bsigned\\b"),
        QStringLiteral("\\bslots\\b"), QStringLiteral("\\bstatic\\b"), QStringLiteral("\\bstruct\\b"),
        QStringLiteral("\\btemplate\\b"), QStringLiteral("\\btypedef\\b"), QStringLiteral("\\btypename\\b"),
        QStringLiteral("\\bunion\\b"), QStringLiteral("\\bunsigned\\b"), QStringLiteral("\\bvirtual\\b"),
        QStringLiteral("\\bvoid\\b"), QStringLiteral("\\bvolatile\\b"), QStringLiteral("\\bbool\\b")
    };
    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }


    //! [2]
    classFormat.setFontWeight(QFont::Bold);
    classFormat.setForeground(Qt::darkMagenta);
    rule.pattern = QRegularExpression(QStringLiteral("\\bQ[A-Za-z]+\\b"));
    rule.format = classFormat;
    highlightingRules.append(rule);
    //! [2]

    //! [3]
    singleLineCommentFormat.setForeground(Qt::gray);
    //rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
    rule.pattern = QRegularExpression(QStringLiteral("%[^\n]*"));
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    multiLineCommentFormat.setForeground(Qt::gray);
    //! [3]
    rule.pattern = QRegularExpression(QStringLiteral("##[^\n]*"));
    rule.format = multiLineCommentFormat;
    highlightingRules.append(rule);

    //! [4]
    quotationFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegularExpression(QStringLiteral("\".*\""));
    rule.format = quotationFormat;
    highlightingRules.append(rule);
    //! [4]
    //!
    //!

    //! [5] Functions are separated since we have to check for existence into octave ws
    functionFormat.setFontWeight(QFont::Bold);
    functionFormat.setForeground(Qt::lightGray);
    functionRule.pattern = QRegularExpression("\\b[A-Za-z_]+[A-Za-z0-9_]*\\b(?=\\()");
    functionRule.format = functionFormat;
    //! [5]
    unknownFunctionFormat.setForeground(Qt::red);
    //! Number Format
    numberFormat.setForeground(Qt::cyan);//(\\.[0-9]?){,1}+([eE]([-+]{,1})[0-9]+){,1}
    rule.pattern = QRegularExpression("\\b[+-]{0,1}\\d*(\\.\\d*){0,1}([eE]([-+]{0,1})\\d+){0,1}\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    //! [7] Vars are separated since we have to check for existence into octave ws

    globalVarFormat.setForeground(Qt::white);
    varRule.pattern = QRegularExpression("\\b[A-Za-z_]+[A-Za-z0-9_]*\\b(?!\\()");
    varRule.format = globalVarFormat;

    if (_data_interface==nullptr) return;

    _octave_interpreter = _data_interface->get_octave_engine();

    if (_octave_interpreter==nullptr) return;

    std::list<std::string> varnames = _octave_interpreter->variable_names();

    for (std::list<std::string>::iterator iter = varnames.begin(); iter!=varnames.end(); iter++)
        knownVariables.append(QString::fromStdString(*iter));
    knownVariables.sort();

    varnames = _octave_interpreter->global_variable_names();

    for (std::list<std::string>::iterator iter = varnames.begin(); iter!=varnames.end(); iter++)
        knownGlobalVariables.append(QString::fromStdString(*iter));
    knownGlobalVariables.sort();

    std::list<std::string> funcnames= _octave_interpreter->user_function_names();

    for (std::list<std::string>::iterator iter = funcnames.begin(); iter!=funcnames.end(); iter++)
        knownFunctions.append(QString::fromStdString(*iter));
    knownFunctions.sort();

    globalVarFormat.setFontWeight(QFont::Bold);
    globalVarFormat.setFontItalic(true);
    globalVarFormat.setForeground(Qt::white);

    knownVarFormat.setFontWeight(QFont::Bold);
    knownVarFormat.setForeground(Qt::white);

}


void octaveSyntaxHighlighter::highlightBlock(const QString &text)
{
    // highlight also
    // Search for functions
    QRegularExpressionMatchIterator matchIterator = functionRule.pattern.globalMatch(text);
    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        QString functionCaptured = match.captured();
        bool bFound = false;
        if (knownFunctions.contains(functionCaptured))
            bFound = true;
        else
        {
            octave_value_list toFind;
            toFind.resize(1);
            charNDArray temp(dim_vector(1,functionCaptured.length()));
            temp = functionCaptured.toStdString();
            toFind(0)=temp;

            int res = _octave_interpreter == nullptr? 0 : _octave_interpreter->feval("exist",toFind,1)(0).int_value();

            if (res>0)
            {
                bFound = true;
                knownFunctions.append(functionCaptured);
                knownFunctions.sort();
            }
        }
        setFormat(match.capturedStart(), match.capturedLength(), bFound? functionRule.format : unknownFunctionFormat);
    }

    //! check for variables (simple name with no parentheses)
    matchIterator = varRule.pattern.globalMatch(text);
    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        QString functionCaptured = match.captured();
        int found = 0;
        if (knownGlobalVariables.contains(functionCaptured))
            found = 1;
        if (knownVariables.contains(functionCaptured))
            found = 2;
        switch (found)
        {
        case 1:
            setFormat(match.capturedStart(), match.capturedLength(), globalVarFormat);
            break;
        case 2:
            setFormat(match.capturedStart(), match.capturedLength(), knownVarFormat);
            break;
        default:
            setFormat(match.capturedStart(), match.capturedLength(), varRule.format);
            break;

        }


    }

    for (const HighlightingRule &rule : (highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);

        }
    }
    //! [7] //! [8]
    //! check for functions


    setCurrentBlockState(0);
}
