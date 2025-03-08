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

#ifndef WINGSYNTAXHIGHLIGHTER_H
#define WINGSYNTAXHIGHLIGHTER_H

#include <KSyntaxHighlighting/SyntaxHighlighter>

class WingSyntaxHighlighter : public KSyntaxHighlighting::SyntaxHighlighter {
public:
    explicit WingSyntaxHighlighter(QTextDocument *document);

public:
    void setTabWidth(int width);
    int tabWidth() const;

    static void hideBlock(QTextBlock block, bool hide);

    static bool isFolded(const QTextBlock &block);

    bool foldContains(const QTextBlock &foldBlock,
                      const QTextBlock &targetBlock) const;

    void foldBlock(QTextBlock block) const;
    void unfoldBlock(QTextBlock block) const;

    int leadingIndentation(const QString &blockText,
                           int *indentPos = nullptr) const;

    bool isFoldable(const QTextBlock &block) const;
    QTextBlock findFoldEnd(const QTextBlock &startBlock) const;

protected:
    void highlightBlock(const QString &text) override;

private:
    int m_tabCharSize;
};

#endif // WINGSYNTAXHIGHLIGHTER_H
