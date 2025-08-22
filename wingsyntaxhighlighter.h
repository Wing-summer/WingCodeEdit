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

class WingSyntaxHighlighterPrivate;

class WingSyntaxHighlighter : public QSyntaxHighlighter,
                              public KSyntaxHighlighting::AbstractHighlighter {
public:
    explicit WingSyntaxHighlighter(QObject *parent = nullptr);
    explicit WingSyntaxHighlighter(QTextDocument *document);
    virtual ~WingSyntaxHighlighter();

public:
    void setDefinition(const KSyntaxHighlighting::Definition &def) override;
    void setTheme(const KSyntaxHighlighting::Theme &theme) override;

    /** Returns whether there is a folding region beginning at @p startBlock.
     *  This only considers syntax-based folding regions,
     *  not indention-based ones as e.g. found in Python.
     *
     *  @see findFoldingRegionEnd
     */
    bool startsFoldingRegion(const QTextBlock &startBlock) const;

    /** Finds the end of the folding region starting at @p startBlock.
     *  If multiple folding regions begin at @p startBlock, the end of
     *  the last/innermost one is returned.
     *  This returns an invalid block if no folding region end is found,
     *  which typically indicates an unterminated region and thus folding
     *  until the document end.
     *  This method performs a sequential search starting at @p startBlock
     *  for the matching folding region end, which is a potentially expensive
     *  operation.
     *
     *  @see startsFoldingRegion
     */
    QTextBlock findFoldingRegionEnd(const QTextBlock &startBlock) const;

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

public:
    static void setSymbolMark(QTextBlock &block, const QString &id);
    static QString symbolMarkID(const QTextBlock &block);
    static bool containsSymbolMark(QTextBlock &block);
    static void clearSymbolMark(QTextBlock &block);

protected:
    void highlightBlock(const QString &text) override;
    void applyFormat(int offset, int length,
                     const KSyntaxHighlighting::Format &format) override;
    void applyFolding(int offset, int length,
                      KSyntaxHighlighting::FoldingRegion region) override;

private:
    int m_tabCharSize;

private:
    Q_DECLARE_PRIVATE_D(AbstractHighlighter::d_ptr, WingSyntaxHighlighter)
};

#endif // WINGSYNTAXHIGHLIGHTER_H
