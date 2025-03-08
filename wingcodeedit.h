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

#include <QPlainTextEdit>

namespace KSyntaxHighlighting {
class Repository;
class Definition;
class Theme;
} // namespace KSyntaxHighlighting

class WingSyntaxHighlighter;
class QPrinter;
class WingCompleter;

class WingCodeEdit : public QPlainTextEdit {
    Q_OBJECT
    friend class WingLineMargin;

public:
    enum class IndentationMode { IndentSpaces, IndentTabs, IndentMixed };

    struct SearchParams {
        QString searchText;
        bool caseSensitive = false;
        bool wholeWord = false;
        bool regex = false;
    };

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

    void setShowLongLineEdge(bool show);
    bool showLongLineEdge() const;
    void setLongLineWidth(int pos);
    int longLineWidth() const;
    void setShowIndentGuides(bool show);
    bool showIndentGuides() const;
    void setWordWrap(bool wrap);
    bool wordWrap() const;

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

    QString textUnderCursor() const;
    QString symbolMark(int line) const;

signals:
    void symbolMarkLineMarginClicked(int line);
    void themeChanged();

public slots:
    void setShowLineNumbers(bool show);
    void setShowFolding(bool show);
    void setShowSymbolMark(bool show);

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

protected:
    void resizeEvent(QResizeEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;

private:
    bool processKeyShortcut(QKeyEvent *e);

private slots:
    void onCompletion(const QString &completion);

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
    void updateExtraSelections();

private:
    WingCompleter *m_completer;
    WingLineMargin *m_lineMargin;
    WingSyntaxHighlighter *m_highlighter;

    QColor m_lineMarginBg, m_lineMarginFg;
    QColor m_codeFoldingBg, m_codeFoldingFg;
    QColor m_cursorLineBg, m_cursorLineNum;
    QColor m_longLineBg, m_longLineEdge, m_longLineCursorBg;
    QColor m_indentGuideFg;
    QColor m_searchBg;
    QColor m_braceMatchBg;
    QColor m_errorBg;
    int m_tabCharSize, m_indentWidth;
    int m_longLineMarker;
    WingCodeEditConfigs m_config;
    IndentationMode m_indentationMode;
    int m_originalFontSize;

#if defined(Q_OS_WIN) && QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    QColor m_editorBg;
    bool m_styleNeedsBgRepaint;
#endif

    QPixmap m_foldOpen, m_foldClosed;

    SearchParams m_liveSearch;
    QList<QTextEdit::ExtraSelection> m_braceMatch;
    QList<QTextEdit::ExtraSelection> m_searchResults;

    QHash<QTextBlock, QString> _symbol;

    void updateScrollBars();
};

#endif // WINGCODEEDIT_H
