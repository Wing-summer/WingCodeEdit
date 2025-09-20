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

#ifndef WINGCODEEDIT_H
#define WINGCODEEDIT_H

#include "wingsignaturetooltip.h"

#include <KSyntaxHighlighting/Theme>
#include <QPlainTextEdit>
#include <QTextBlock>

namespace KSyntaxHighlighting {
class Repository;
class Definition;
class Theme;
} // namespace KSyntaxHighlighting

class WingSyntaxHighlighter;
class WingCompleter;
class WingLineMargin;
class QPrinter;

class WingCodeEdit : public QPlainTextEdit {
    Q_OBJECT
    friend class WingLineMargin;
    friend class WingSquiggleInfoModel;

public:
    enum class IndentationMode { IndentSpaces, IndentTabs, IndentMixed };
    Q_ENUM(IndentationMode)

    struct SearchParams {
        QString searchText;
        bool caseSensitive = false;
        bool wholeWord = false;
        bool regex = false;
    };

public:
    enum class SeverityLevel { Hint, Information, Warning, Error };

private:
    enum class WingCodeEditConfig {
        ShowLineNumbers = (1U << 0),
        AutoIndent = (1U << 1),
        MatchBraces = (1U << 2),
        HighlightCurLine = (1U << 3),
        IndentGuides = (1U << 4),
        LongLineEdge = (1U << 5),
        ShowFolding = (1U << 6),
        ShowSymbolMark = (1U << 7),
        AutoCloseChar = (1U << 8)
    };
    Q_DECLARE_FLAGS(WingCodeEditConfigs, WingCodeEditConfig)

public:
    explicit WingCodeEdit(QWidget *parent = nullptr);

public:
    void deleteSelection();
    void deleteLines();

    int lineMarginWidth() const;
    int symbolMarkSize() const;
    int realSymbolMarkSizeWithPadding() const;

    bool showLineNumbers() const;
    bool showFolding() const;
    bool showSymbolMark() const;
    bool showWhitespace() const;

    bool scrollPastEndOfFile() const;
    bool highlightCurrentLine() const;

    int tabWidth() const;
    int indentWidth() const;

    IndentationMode indentationMode() const;

    int textColumn(const QString &block, int positionInBlock) const;
    void moveCursorTo(int line, int column = 0);

    void moveLines(QTextCursor::MoveOperation op);
    void smartHome(QTextCursor::MoveMode mode);
    void smartEnd(QTextCursor::MoveMode mode);

    void setAutoIndent(bool ai);
    bool autoIndent() const;

    bool showLongLineEdge() const;
    int longLineWidth() const;
    bool showIndentGuides() const;
    bool wordWrap() const;
    bool autoCloseChar() const;

    bool isCurrentLineFolded() const;

    QTextCursor textSearch(const QTextCursor &start, const SearchParams &params,
                           bool matchFirst, bool reverse = false,
                           QRegularExpressionMatch *regexMatch = nullptr);
    void setLiveSearch(const SearchParams &params);
    void clearLiveSearch();

    void setMatchBraces(bool match);
    bool matchBraces() const;

    static KSyntaxHighlighting::Repository &syntaxRepo();
    static const KSyntaxHighlighting::Definition &nullSyntax();

    void setDefaultFont(const QFont &font);
    QString themeName() const;

    QString syntaxName() const;

    QFont defaultFont() const;

    WingCompleter *completer() const;
    WingSyntaxHighlighter *highlighter() const;

    KSyntaxHighlighting::Theme theme() const;

    QString symbolMark(int line) const;

    void setHighlighter(WingSyntaxHighlighter *newHighlighter);

public:
    /**
     * @brief squiggle Puts a underline squiggle under text ranges in Editor
     * @param level defines the color of the underline depending upon the
     * severity
     * @param tooltipMessage The tooltip hover message to show when over
     * selection.
     * @note QPair<int, int>: first -> Line number in 1-based indexing
     *                        second -> Character number in 0-based indexing
     */
    void addSquiggle(SeverityLevel level, const QPair<int, int> &start,
                     const QPair<int, int> &stop,
                     const QString &tooltipMessage);

    void highlightAllSquiggle();

    /**
     * @brief clearSquiggle, Clears complete squiggle from editor
     */
    void clearSquiggle();

signals:
    void symbolMarkLineMarginClicked(int line);
    void squiggleItemChanged();
    void themeChanged();

public slots:
    void setShowLineNumbers(bool show);
    void setShowFolding(bool show);
    void setShowSymbolMark(bool show);

    void setShowLongLineEdge(bool show);
    void setLongLineWidth(int pos);
    void setShowIndentGuides(bool show);
    void setWordWrap(bool wrap);

    void setAutoCloseChar(bool b);
    void setShowWhitespace(bool show);
    void setScrollPastEndOfFile(bool scroll);
    void setHighlightCurrentLine(bool show);

    void setTabWidth(int width);
    void setIndentWidth(int width);

    void setIndentationMode(IndentationMode mode);

    void setCompleter(WingCompleter *completer);
    void setDefaultTheme();

    void setTheme(const KSyntaxHighlighting::Theme &theme);
    void setSyntax(const KSyntaxHighlighting::Definition &syntax);

    void addSymbolMark(int line, const QString &id);
    void removeSymbolMark(int line);

    void ensureLineVisible(int lineNumber);

    void showHelpTooltip(const QList<Signature> &sigs, qsizetype index = 0);

private:
    QString cursorNextChar(const QTextCursor &cursor);
    QString cursorPrevChar(const QTextCursor &cursor);
    QString textUnderCursor() const;

private:
    bool isAutoCloseChar(const QString &ch) const;
    bool isPairedCloseChar(const QString &ch) const;
    QString getPairedBeginChar(const QString &ch) const;
    QString getPairedCloseChar(const QString &ch) const;

    QTextBlock getCursorPositionBlock(int position) const;

protected:
    bool event(QEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;

    void insertFromMimeData(const QMimeData *source) override;

protected:
    bool processCompletionBegin(QKeyEvent *e);
    bool processCompletionEnd(QKeyEvent *e);
    void processDefaultKeyPressEvent(QKeyEvent *e);

private:
    bool processKeyShortcut(QKeyEvent *e);

    struct SquiggleInformation {
        SquiggleInformation(SeverityLevel level, const QPair<int, int> &start,
                            const QPair<int, int> &stop, const QString &text)
            : level(level), start(start), stop(stop), tooltip(text) {}

        SeverityLevel level;
        QPair<int, int> start;
        QPair<int, int> stop;
        QString tooltip;
    };

    void highlightSquiggle(const SquiggleInformation &info);

protected:
    virtual void highlightOccurrences();

protected slots:
    virtual void onCompletion(const QModelIndex &index);

public slots:
    void cutLines();
    void copyLines();
    void indentSelection();
    void outdentSelection();

    void foldCurrentLine();
    void unfoldCurrentLine();
    void foldAll();
    void unfoldAll();

    void zoomIn();  // Hides QPlainTextEdit::zoomIn(int = 1)
    void zoomOut(); // Hides QPlainTextEdit::zoomOut(int = 1)
    void zoomReset();

    void printDocument(QPrinter *printer);

private slots:
    void updateMargins();
    void repaintMargins();
    void updateLineNumbers(const QRect &rect, int dy);
    void updateCursor();
    void updateTabMetrics();
    void updateTextMetrics();
    void updateLiveSearch();

protected slots:
    void updateExtraSelections();

private:
    WingCompleter *m_completer;
    WingLineMargin *m_lineMargin;
    WingSyntaxHighlighter *m_highlighter;
    WingSignatureTooltip *m_sighlp;

    // color caches
    QColor m_lineMarginBg, m_lineMarginFg;
    QColor m_codeFoldingBg, m_codeFoldingFg;
    QColor m_cursorLineBg, m_cursorLineNum;
    QColor m_longLineBg, m_longLineEdge, m_longLineCursorBg;
    QColor m_indentGuideFg;
    QColor m_searchBg;
    QColor m_braceMatchBg;
    QColor m_errorBg;
    QColor m_warnBg;
    QColor m_textSelBg;

    QColor m_errorFg;
    QColor m_warnFg;
    QColor m_infoFg;

    QVector<SquiggleInformation> m_squiggles;

    int m_tabCharSize, m_indentWidth;
    int m_longLineMarker;
    WingCodeEditConfigs m_config;
    IndentationMode m_indentationMode;
    int m_originalFontSize;

    QColor m_editorBg;

    QPixmap m_foldOpen, m_foldClosed;

    SearchParams m_liveSearch;
    QList<QTextEdit::ExtraSelection> m_braceMatch;
    QList<QTextEdit::ExtraSelection> m_searchResults;
    QList<QTextEdit::ExtraSelection> m_squigglesExtraSelections,
        m_squigglesLineExtraSelections;

    void updateScrollBars();

protected:
    QList<QTextEdit::ExtraSelection> m_occurrencesExtraSelections;
};

#endif // WINGCODEEDIT_H
