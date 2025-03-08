/****************************************************************************
**
** Copyright (C) 2025-2028 WingSummer
**
** This file may be used under the terms of the GNU General Public License
** version 3 as published by the Free Software Foundation.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** You should have received a copy of the GNU General Public License version 3
** along with this program. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "wingsyntaxhighlighter.h"

#include <KSyntaxHighlighting/Theme>
#include <QRegularExpression>

WingSyntaxHighlighter::WingSyntaxHighlighter(QTextDocument *document)
    : KSyntaxHighlighting::SyntaxHighlighter(document), m_tabCharSize() {}

void WingSyntaxHighlighter::setTabWidth(int width) { m_tabCharSize = width; }

int WingSyntaxHighlighter::tabWidth() const { return m_tabCharSize; }

void WingSyntaxHighlighter::hideBlock(QTextBlock block, bool hide) {
    block.setVisible(!hide);
    block.clearLayout();
    block.setLineCount(hide ? 0 : 1);
}

bool WingSyntaxHighlighter::isFolded(const QTextBlock &block) {
    return block.userState() > 0;
}

bool WingSyntaxHighlighter::foldContains(const QTextBlock &foldBlock,
                                         const QTextBlock &targetBlock) const {
    if (!isFoldable(foldBlock))
        return false;
    return (targetBlock.position() >= foldBlock.position()) &&
           (findFoldEnd(foldBlock).position() >= targetBlock.position());
}

void WingSyntaxHighlighter::foldBlock(QTextBlock block) const {
    block.setUserState(1);

    const QTextBlock endBlock = findFoldEnd(block);
    block = block.next();
    while (block.isValid() && block != endBlock) {
        hideBlock(block, true);
        block = block.next();
    }

    // Only hide the last block if it doesn't also start a new fold region
    if (block.isValid() && !isFoldable(block))
        hideBlock(block, true);
}

void WingSyntaxHighlighter::unfoldBlock(QTextBlock block) const {
    block.setUserState(-1);

    const QTextBlock endBlock = findFoldEnd(block);
    block = block.next();
    while (block.isValid() && block != endBlock) {
        hideBlock(block, false);
        if (isFolded(block)) {
            block = findFoldEnd(block);
            if (block.isValid() && !isFoldable(block))
                block = block.next();
        } else {
            block = block.next();
        }
    }

    if (block.isValid() && !isFoldable(block))
        hideBlock(block, false);
}

int WingSyntaxHighlighter::leadingIndentation(const QString &blockText,
                                              int *indentPos) const {
    int leadingIndent = 0;
    int startOfLine = 0;
    for (const auto ch : blockText) {
        if (ch == QLatin1Char('\t')) {
            leadingIndent += (m_tabCharSize - (leadingIndent % m_tabCharSize));
            startOfLine += 1;
        } else if (ch == QLatin1Char(' ')) {
            leadingIndent += 1;
            startOfLine += 1;
        } else {
            break;
        }
    }
    if (indentPos)
        *indentPos = startOfLine;
    return leadingIndent;
}

static QList<QRegularExpression> reCompileAll(const QStringList &regexList) {
    QList<QRegularExpression> compiled;
    compiled.reserve(regexList.size());
    for (const QString &expr : regexList)
        compiled << QRegularExpression(QStringLiteral("^") + expr +
                                       QStringLiteral("$"));
    return compiled;
}

static bool lineEmpty(const QString &text,
                      const QList<QRegularExpression> &regexList) {
    if (text.isEmpty())
        return true;

    return std::any_of(regexList.begin(), regexList.end(),
                       [text](const QRegularExpression &re) {
                           const QRegularExpressionMatch m = re.match(text);
                           return m.hasMatch();
                       });
}

bool WingSyntaxHighlighter::isFoldable(const QTextBlock &block) const {
    if (startsFoldingRegion(block))
        return true;
    if (definition().indentationBasedFoldingEnabled()) {
        const auto emptyList = reCompileAll(definition().foldingIgnoreList());
        if (lineEmpty(block.text(), emptyList))
            return false;

        const int curIndent = leadingIndentation(block.text());
        QTextBlock nextBlock = block.next();
        while (nextBlock.isValid() && lineEmpty(nextBlock.text(), emptyList))
            nextBlock = nextBlock.next();
        if (nextBlock.isValid() &&
            leadingIndentation(nextBlock.text()) > curIndent)
            return true;
    }
    return false;
}

QTextBlock
WingSyntaxHighlighter::findFoldEnd(const QTextBlock &startBlock) const {
    if (startsFoldingRegion(startBlock))
        return findFoldingRegionEnd(startBlock);

    if (definition().indentationBasedFoldingEnabled()) {
        const auto emptyList = reCompileAll(definition().foldingIgnoreList());

        const int curIndent = leadingIndentation(startBlock.text());
        QTextBlock block = startBlock.next();
        QTextBlock endBlock;
        for (;;) {
            while (block.isValid() && lineEmpty(block.text(), emptyList))
                block = block.next();
            if (!block.isValid() ||
                leadingIndentation(block.text()) <= curIndent)
                break;
            endBlock = block;
            block = block.next();
        }
        return endBlock;
    }
    return {};
}

void WingSyntaxHighlighter::highlightBlock(const QString &text) {
    KSyntaxHighlighting::SyntaxHighlighter::highlightBlock(text);

    static const QRegularExpression ws_regex(QStringLiteral("\\s+"));
    auto iter = ws_regex.globalMatch(text);
    QTextCharFormat ws_format;
    const QBrush ws_brush(
        theme().editorColor(KSyntaxHighlighting::Theme::TabMarker));
    ws_format.setForeground(ws_brush);
    while (iter.hasNext()) {
        const auto match = iter.next();
        setFormat(match.capturedStart(), match.capturedLength(), ws_format);
    }
}
