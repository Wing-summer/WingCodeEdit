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

#include "wingcodeedit.h"
#include "wingcompleter.h"
#include "winglinemargin.h"
#include "wingsymbolcenter.h"
#include "wingsyntaxhighlighter.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QPainter>
#include <QPalette>
#include <QPrinter>
#include <QRegularExpression>
#include <QScrollBar>
#include <QStack>
#include <QStyle>
#include <QStyleHints>
#include <QUndoStack>
#include <QtMath>

#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Theme>

WingCodeEdit::WingCodeEdit(QWidget *parent)
    : QPlainTextEdit(parent), m_tabCharSize(4), m_indentWidth(4),
      m_longLineMarker(80), m_config(), m_indentationMode(),
      m_originalFontSize(), m_completer(nullptr) {
    m_lineMargin = new WingLineMargin(this);
    connect(m_lineMargin, &WingLineMargin::symbolMarkLineMarginClicked, this,
            &WingCodeEdit::symbolMarkLineMarginClicked);

    m_highlighter = new WingSyntaxHighlighter(document());
    m_highlighter->setTabWidth(m_tabCharSize);

    connect(this, &QPlainTextEdit::blockCountChanged, this,
            &WingCodeEdit::updateMargins);
    connect(this, &QPlainTextEdit::updateRequest, this,
            &WingCodeEdit::updateLineNumbers);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this,
            &WingCodeEdit::updateCursor);
    connect(this, &QPlainTextEdit::textChanged, this,
            &WingCodeEdit::updateLiveSearch);

    // Initialize default editor configuration
    QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    setDefaultFont(fixedFont);
    setWordWrap(false);
    setIndentationMode(IndentationMode::IndentSpaces);
    setDefaultTheme();

    QTextOption opt = document()->defaultTextOption();
    opt.setFlags(opt.flags() |
                 QTextOption::AddSpaceForLineAndParagraphSeparators);
    document()->setDefaultTextOption(opt);
}

void WingCodeEdit::deleteSelection() {
    QTextCursor cursor = textCursor();
    cursor.removeSelectedText();
    cursor.setVerticalMovementX(-1);
    setTextCursor(cursor);
}

void WingCodeEdit::deleteLines() {
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection()) {
        const int startPos = cursor.selectionStart();
        const int endPos = cursor.selectionEnd();
        cursor.setPosition(startPos);
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.setPosition(endPos, QTextCursor::KeepAnchor);
        if (!cursor.atBlockStart())
            cursor.movePosition(QTextCursor::NextBlock,
                                QTextCursor::KeepAnchor);
    } else {
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    }
    cursor.removeSelectedText();
    cursor.setVerticalMovementX(-1);
    setTextCursor(cursor);
}

int WingCodeEdit::lineMarginWidth() const {
    qreal margin = 0;
    const QFontMetricsF metrics(font());

    if (showSymbolMark()) {
        margin += metrics.height() + qreal(2.0);
    }

    if (showLineNumbers()) {
        int digits = 1;
        int maxLine = qMax(1, blockCount());
        while (maxLine >= 10) {
            maxLine /= 10;
            ++digits;
        }
        margin += metrics.boundingRect(QString(digits + 1, QLatin1Char('0')))
                      .width() +
                  qreal(2.0);
    }

    if (showFolding()) {
        // Fold markers
        margin +=
            m_foldOpen.width() + (showLineNumbers() ? qreal(2.0) : qreal(4.0));
    }

    return qCeil(margin);
}

int WingCodeEdit::symbolMarkSize() const {
    const QFontMetricsF metrics(font());
    return metrics.height();
}

int WingCodeEdit::realSymbolMarkSizeWithPadding() const {
    constexpr auto pad = 2;
    constexpr auto headeroff = 3;
    constexpr auto endoff = 2;
    if (showSymbolMark()) {
        return symbolMarkSize() + pad + headeroff + endoff;
    }
    return 0;
}

void WingCodeEdit::setShowLineNumbers(bool show) {
    m_config.setFlag(WingCodeEditConfig::ShowLineNumbers, show);
    updateMargins();
    m_lineMargin->update();
}

bool WingCodeEdit::showLineNumbers() const {
    return m_config.testFlag(WingCodeEditConfig::ShowLineNumbers);
}

void WingCodeEdit::setShowFolding(bool show) {
    m_config.setFlag(WingCodeEditConfig::ShowFolding, show);
    updateMargins();
    m_lineMargin->update();
}

bool WingCodeEdit::showFolding() const {
    return m_config.testFlag(WingCodeEditConfig::ShowFolding);
}

void WingCodeEdit::setShowSymbolMark(bool show) {
    m_config.setFlag(WingCodeEditConfig::ShowSymbolMark, show);
    updateMargins();
    m_lineMargin->update();
}

bool WingCodeEdit::showSymbolMark() const {
    return m_config.testFlag(WingCodeEditConfig::ShowSymbolMark);
}

void WingCodeEdit::setShowWhitespace(bool show) {
    QTextOption opt = document()->defaultTextOption();
    auto optflags = opt.flags();
    optflags.setFlag(QTextOption::ShowTabsAndSpaces, show);
    opt.setFlags(optflags);
    document()->setDefaultTextOption(opt);
}

bool WingCodeEdit::showWhitespace() const {
    const QTextOption opt = document()->defaultTextOption();
    return opt.flags().testFlag(QTextOption::ShowTabsAndSpaces);
}

void WingCodeEdit::setScrollPastEndOfFile(bool scroll) {
    // This feature, counter-intuitively, scrolls the document such that the
    // cursor is in the center ONLY when moving the cursor -- it does NOT
    // reposition the cursor when normal scrolling occurs.  Furthermore, this
    // property is the only way to enable scrolling past the last line of
    // the document.  TL;DR: This property is poorly named.
    setCenterOnScroll(scroll);
}

bool WingCodeEdit::scrollPastEndOfFile() const { return centerOnScroll(); }

void WingCodeEdit::setHighlightCurrentLine(bool show) {
    m_config.setFlag(WingCodeEditConfig::HighlightCurLine, show);
    viewport()->update();
}

bool WingCodeEdit::highlightCurrentLine() const {
    return m_config.testFlag(WingCodeEditConfig::HighlightCurLine);
}

void WingCodeEdit::setTabWidth(int width) {
    m_tabCharSize = width;
    m_highlighter->setTabWidth(width);
    updateTabMetrics();
}

int WingCodeEdit::tabWidth() const { return m_tabCharSize; }

void WingCodeEdit::setIndentWidth(int width) {
    m_indentWidth = width;
    if (showIndentGuides())
        viewport()->update();
}

int WingCodeEdit::indentWidth() const { return m_indentWidth; }

static qreal indentAdvance(const QFontMetricsF &fm, int indentChars) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    return fm.horizontalAdvance(QString(indentChars, QLatin1Char(' ')));
#else
    return fm.width(QString(indentChars, QLatin1Char(' ')));
#endif
}

void WingCodeEdit::updateTabMetrics() {
    // setTabStopWidth only allows int widths, which doesn't line up correctly
    // on many fonts.  Hack from QtCreator: Set it in the text option instead
    const qreal tabWidth = indentAdvance(QFontMetricsF(font()), m_tabCharSize);
    QTextOption opt = document()->defaultTextOption();
    opt.setTabStopDistance(tabWidth);
    document()->setDefaultTextOption(opt);
}

void WingCodeEdit::updateTextMetrics() {
    QFontMetricsF metrics(font());
    const qreal box = qMin(metrics.boundingRect(QLatin1Char('x')).width() * 1.5,
                           metrics.height());
    const QVector<QPointF> arrowOpen{
        QPointF(0, box / 4.0),
        QPointF(box - 1, box / 4.0),
        QPointF(box / 2.0, 0.75 * box),
    };
    const QVector<QPointF> arrowClosed{
        QPointF(box / 4.0, 0),
        QPointF(box / 4.0, box - 1),
        QPointF(0.75 * box, box / 2.0),
    };

    QPainter painter;
    m_foldOpen = QPixmap(box, box);
    m_foldOpen.fill(Qt::transparent);
    painter.begin(&m_foldOpen);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(m_lineMarginFg);
    painter.setBrush(m_lineMarginFg);
    painter.drawPolygon(arrowOpen);
    painter.end();

    m_foldClosed = QPixmap(box, box);
    m_foldClosed.fill(Qt::transparent);
    painter.begin(&m_foldClosed);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(m_codeFoldingFg);
    painter.setBrush(m_codeFoldingFg);
    painter.drawPolygon(arrowClosed);
    painter.end();

    updateMargins();
    updateTabMetrics();
}

void WingCodeEdit::setIndentationMode(IndentationMode mode) {
    m_indentationMode = mode;
    if (showIndentGuides())
        viewport()->update();
}

WingCodeEdit::IndentationMode WingCodeEdit::indentationMode() const {
    return m_indentationMode;
}

int WingCodeEdit::textColumn(const QString &block, int positionInBlock) const {
    int column = 0;
    for (int i = 0; i < positionInBlock; ++i) {
        if (block.at(i) == QLatin1Char('\t'))
            column = column - (column % m_tabCharSize) + m_tabCharSize;
        else
            ++column;
    }
    return column;
}

void WingCodeEdit::moveCursorTo(int line, int column) {
    const auto block = document()->findBlockByNumber(line - 1);
    if (!block.isValid() && line > 0) {
        // Just navigate to the end of the file if we don't have the requested
        // line number.
        QTextCursor cursor(document());
        cursor.movePosition(QTextCursor::End);
        setTextCursor(cursor);
        return;
    }

    QTextCursor cursor(block);
    if (column > 0) {
        const QString blockText = block.text();
        int columnIndex = 0, cursorIndex = 0;
        for (; cursorIndex < blockText.size(); ++cursorIndex) {
            if (columnIndex >= column - 1)
                break;
            if (blockText.at(cursorIndex) == QLatin1Char('\t'))
                columnIndex =
                    columnIndex - (columnIndex % m_tabCharSize) + m_tabCharSize;
            else
                ++columnIndex;
        }
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor,
                            cursorIndex);
    }
    setTextCursor(cursor);
}

void WingCodeEdit::moveLines(QTextCursor::MoveOperation op) {
    auto cursor = textCursor();

    const int startPos = cursor.position();
    const int endPos = cursor.anchor();
    cursor.setPosition(qMin(startPos, endPos));
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.setPosition(qMax(startPos, endPos), QTextCursor::KeepAnchor);
    bool moveFromEndOfDocument = false;
    bool moveToEndOfDocument = false;
    if (startPos == endPos || !cursor.atBlockStart()) {
        if (!cursor.movePosition(QTextCursor::NextBlock,
                                 QTextCursor::KeepAnchor)) {
            moveFromEndOfDocument = true;
            cursor.movePosition(QTextCursor::EndOfBlock,
                                QTextCursor::KeepAnchor);
        }
    }

    if (cursor.anchor() == 0 && op == QTextCursor::PreviousBlock)
        return;
    if (moveFromEndOfDocument && cursor.atEnd() && op == QTextCursor::NextBlock)
        return;

    cursor.beginEditBlock();
    const auto moveText = cursor.selectedText();
    cursor.removeSelectedText();
    const int positionStart = cursor.position();
    if ((!cursor.movePosition(op) && op == QTextCursor::NextBlock) ||
        cursor.atEnd()) {
        cursor.movePosition(QTextCursor::End);
        cursor.insertBlock();
        moveToEndOfDocument = true;
    }
    const int postionDelta = cursor.position() - positionStart;

    cursor.insertText(moveText);
    if (moveFromEndOfDocument)
        cursor.insertBlock();
    if (moveFromEndOfDocument || moveToEndOfDocument) {
        // Remove the extra newline from the displaced line
        cursor.movePosition(QTextCursor::End);
        cursor.deletePreviousChar();
    }
    cursor.endEditBlock();

    cursor.setPosition(endPos + postionDelta);
    if (startPos != endPos)
        cursor.setPosition(startPos + postionDelta, QTextCursor::KeepAnchor);

    setTextCursor(cursor);
}

void WingCodeEdit::smartHome(QTextCursor::MoveMode moveMode) {
    auto cursor = textCursor();

    int leadingIndent = 0;
    const QString blockText = cursor.block().text();
    for (const auto ch : blockText) {
        if (ch.isSpace())
            leadingIndent += 1;
        else
            break;
    }
    int cursorPos = cursor.positionInBlock();
    cursor.movePosition(QTextCursor::StartOfLine, moveMode);
    if (cursor.positionInBlock() == 0 && cursorPos != leadingIndent)
        cursor.movePosition(QTextCursor::NextCharacter, moveMode,
                            leadingIndent);

    setTextCursor(cursor);
    updateCursor();
}

void WingCodeEdit::smartEnd(QTextCursor::MoveMode moveMode) {
    auto cursor = textCursor();

    const QString blockText = cursor.block().text();
    int trailingEnd = 0;
    for (auto it = blockText.crbegin(); it != blockText.crend(); ++it) {
        if (it->isSpace())
            trailingEnd += 1;
        else
            break;
    }
    int cursorPos = cursor.positionInBlock();
    cursor.movePosition(QTextCursor::EndOfLine, moveMode);
    if (cursor.positionInBlock() == cursorPos)
        cursor.movePosition(QTextCursor::PreviousCharacter, moveMode,
                            trailingEnd);

    setTextCursor(cursor);
    updateCursor();
}

void WingCodeEdit::setAutoIndent(bool ai) {
    m_config.setFlag(WingCodeEditConfig::AutoIndent, ai);
}

bool WingCodeEdit::autoIndent() const {
    return m_config.testFlag(WingCodeEditConfig::AutoIndent);
}

void WingCodeEdit::setShowLongLineEdge(bool show) {
    m_config.setFlag(WingCodeEditConfig::LongLineEdge, show);
    viewport()->update();
}

bool WingCodeEdit::showLongLineEdge() const {
    return m_config.testFlag(WingCodeEditConfig::LongLineEdge);
}

void WingCodeEdit::setLongLineWidth(int pos) {
    m_longLineMarker = pos;
    viewport()->update();
}

int WingCodeEdit::longLineWidth() const { return m_longLineMarker; }

void WingCodeEdit::setShowIndentGuides(bool show) {
    m_config.setFlag(WingCodeEditConfig::IndentGuides, show);
    viewport()->update();
}

bool WingCodeEdit::showIndentGuides() const {
    return m_config.testFlag(WingCodeEditConfig::IndentGuides);
}

void WingCodeEdit::setWordWrap(bool wrap) {
    setWordWrapMode(wrap ? QTextOption::WrapAtWordBoundaryOrAnywhere
                         : QTextOption::NoWrap);
}

void WingCodeEdit::setAutoCloseChar(bool b) {
    m_config.setFlag(WingCodeEditConfig::AutoCloseChar, b);
}

bool WingCodeEdit::wordWrap() const {
    return wordWrapMode() != QTextOption::NoWrap;
}

bool WingCodeEdit::autoCloseChar() const {
    return m_config.testFlag(WingCodeEditConfig::AutoCloseChar);
}

template <typename Findable>
QTextCursor safeFindNext(QTextDocument *document, const Findable &search,
                         const QTextCursor &start,
                         QTextDocument::FindFlags flags, bool matchFirst) {
    // If a search returns the same starting cursor, we need to skip over it
    // to find the next match (which could be equal to the skipped cursor).
    // Otherwise, certain types of searches could result in an infinite loop.

    QTextCursor cursor = document->find(search, start, flags);
    if (cursor == start && !matchFirst) {
        if (cursor.atEnd())
            return QTextCursor();
        cursor.movePosition(QTextCursor::NextCharacter);
        cursor = document->find(search, cursor, flags);
    }
    return cursor;
}

QTextCursor WingCodeEdit::textSearch(const QTextCursor &start,
                                     const SearchParams &params,
                                     bool matchFirst, bool reverse,
                                     QRegularExpressionMatch *regexMatch) {
    QTextDocument::FindFlags flags;
    if (params.caseSensitive)
        flags |= QTextDocument::FindCaseSensitively;
    if (params.wholeWord)
        flags |= QTextDocument::FindWholeWords;
    if (reverse)
        flags |= QTextDocument::FindBackward;

    if (params.regex) {
        QRegularExpression::PatternOptions csOption =
            params.caseSensitive ? QRegularExpression::NoPatternOption
                                 : QRegularExpression::CaseInsensitiveOption;
        const QRegularExpression re(params.searchText, csOption);
        QTextCursor cursor =
            safeFindNext(document(), re, start, flags, matchFirst);
        if (cursor.isNull())
            return cursor;
        if (regexMatch)
            *regexMatch = re.match(cursor.selectedText());
        return cursor;
    } else {
        return safeFindNext(document(), params.searchText, start, flags,
                            matchFirst);
    }
}

void WingCodeEdit::setLiveSearch(const SearchParams &params) {
    m_liveSearch = params;
    updateLiveSearch();
}

void WingCodeEdit::clearLiveSearch() {
    m_liveSearch.searchText = QString();
    updateLiveSearch();
}

void WingCodeEdit::updateLiveSearch() {
    if (m_searchResults.isEmpty() && m_liveSearch.searchText.isEmpty())
        return;

    m_searchResults.clear();
    if (!m_liveSearch.searchText.isEmpty()) {
        auto searchCursor = textCursor();
        searchCursor.movePosition(QTextCursor::Start);
        searchCursor = textSearch(searchCursor, m_liveSearch, true);
        while (!searchCursor.isNull()) {
            if (searchCursor.hasSelection()) {
                QTextEdit::ExtraSelection selection;
                selection.format.setBackground(m_searchBg);
                selection.cursor = searchCursor;
                m_searchResults.append(selection);
            }
            searchCursor = textSearch(searchCursor, m_liveSearch, false);
        }
    }
    updateExtraSelections();
}

void WingCodeEdit::updateExtraSelections() {
    setExtraSelections(m_braceMatch + m_searchResults);
}

WingSyntaxHighlighter *WingCodeEdit::highlighter() const {
    return m_highlighter;
}

void WingCodeEdit::setMatchBraces(bool match) {
    m_config.setFlag(WingCodeEditConfig::MatchBraces, match);
    updateCursor();
}

bool WingCodeEdit::matchBraces() const {
    return m_config.testFlag(WingCodeEditConfig::MatchBraces);
}

KSyntaxHighlighting::Repository &WingCodeEdit::syntaxRepo() {
    static KSyntaxHighlighting::Repository s_syntaxRepo;
    return s_syntaxRepo;
}

const KSyntaxHighlighting::Definition &WingCodeEdit::nullSyntax() {
    static KSyntaxHighlighting::Definition s_nullSyntax;
    return s_nullSyntax;
}

void WingCodeEdit::setDefaultFont(const QFont &font) {
    // Note:  This will reset the zoom factor to 100%
    setFont(font);
    m_originalFontSize = font.pointSize();
    updateTextMetrics();
}

QFont WingCodeEdit::defaultFont() const {
    QFont baseFont = font();
    baseFont.setPointSize(m_originalFontSize);
    return baseFont;
}

void WingCodeEdit::setCompleter(WingCompleter *completer) {
    if (m_completer)
        m_completer->disconnect(this);

    m_completer = completer;

    if (!m_completer)
        return;

    m_completer->setWidget(this);
    connect(m_completer,
            QOverload<const QString &>::of(&WingCompleter::activated), this,
            &WingCodeEdit::onCompletion);
}

WingCompleter *WingCodeEdit::completer() const { return m_completer; }

void WingCodeEdit::setTheme(const KSyntaxHighlighting::Theme &theme) {
    QPalette pal = palette();
    pal.setColor(QPalette::Text,
                 theme.textColor(KSyntaxHighlighting::Theme::Normal));
    pal.setColor(
        QPalette::Base,
        theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor));
    pal.setColor(QPalette::Highlight,
                 theme.editorColor(KSyntaxHighlighting::Theme::TextSelection));
    pal.setBrush(QPalette::HighlightedText, Qt::NoBrush);
    setPalette(pal);

    const bool darkTheme = pal.color(QPalette::Base).lightness() < 128;

    // Cache other colors used by the widget
    m_lineMarginFg = theme.editorColor(KSyntaxHighlighting::Theme::LineNumbers);
    m_lineMarginBg = theme.editorColor(KSyntaxHighlighting::Theme::IconBorder);
    m_codeFoldingBg =
        theme.editorColor(KSyntaxHighlighting::Theme::CodeFolding);
    m_codeFoldingFg =
        darkTheme ? m_codeFoldingBg.lighter(150) : m_codeFoldingBg.darker(150);
    m_cursorLineBg = theme.editorColor(KSyntaxHighlighting::Theme::CurrentLine);
    m_cursorLineNum =
        theme.editorColor(KSyntaxHighlighting::Theme::CurrentLineNumber);
    m_longLineBg =
        theme.editorColor(KSyntaxHighlighting::Theme::WordWrapMarker);
    m_longLineEdge =
        darkTheme ? m_longLineBg.lighter(120) : m_longLineBg.darker(120);
    m_longLineCursorBg =
        darkTheme ? m_cursorLineBg.lighter(110) : m_cursorLineBg.darker(110);
    m_indentGuideFg =
        theme.editorColor(KSyntaxHighlighting::Theme::IndentationLine);
    m_searchBg = theme.editorColor(KSyntaxHighlighting::Theme::SearchHighlight);
    m_braceMatchBg =
        theme.editorColor(KSyntaxHighlighting::Theme::BracketMatching);
    m_errorBg = theme.editorColor(KSyntaxHighlighting::Theme::MarkError);

#if defined(Q_OS_WIN) && QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    m_styleNeedsBgRepaint =
        QApplication::style()->name() == QStringLiteral("windows11");
    m_editorBg = theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor);
#endif

    m_highlighter->setTheme(theme);
    m_highlighter->rehighlight();

    // Update extra highlights to match the new theme
    for (auto &result : m_searchResults)
        result.format.setBackground(m_searchBg);
    updateTextMetrics();
    updateCursor();
}

QString WingCodeEdit::themeName() const {
    return m_highlighter->theme().name();
}

void WingCodeEdit::setDefaultTheme() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const bool useDarkTheme =
        (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark);
#else
    const bool useDarkTheme =
        (palette().color(QPalette::Base).lightness() < 128);
#endif

    setTheme(useDarkTheme ? syntaxRepo().defaultTheme(
                                KSyntaxHighlighting::Repository::DarkTheme)
                          : syntaxRepo().defaultTheme(
                                KSyntaxHighlighting::Repository::LightTheme));
}

void WingCodeEdit::setSyntax(const KSyntaxHighlighting::Definition &syntax) {
    m_highlighter->setDefinition(syntax);
}

void WingCodeEdit::addSymbolMark(int line, const QString &id) {
    if (!WingSymbolCenter::instance().existSymbol(id)) {
        return;
    }

    auto block = document()->findBlockByNumber(line - 1);
    if (block.isValid()) {
        m_highlighter->setProperty(block, QStringLiteral("sym"), id);
        repaintMargins();
    }
}

QString WingCodeEdit::symbolMark(int line) const {
    auto block = document()->findBlockByNumber(line - 1);
    if (block.isValid()) {
        return m_highlighter->property(block, QStringLiteral("sym")).toString();
    }
    return {};
}

void WingCodeEdit::removeSymbolMark(int line) {
    auto block = document()->findBlockByNumber(line - 1);
    if (block.isValid()) {
        m_highlighter->setProperty(block, QStringLiteral("sym"), {});
        repaintMargins();
    }
}

QString WingCodeEdit::cursorNextChar(const QTextCursor &cursor) {
    auto c = cursor;
    c.clearSelection();
    c.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    return c.selectedText();
}

QString WingCodeEdit::cursorPrevChar(const QTextCursor &cursor) {
    auto c = cursor;
    c.clearSelection();
    c.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
    return c.selectedText();
}

QString WingCodeEdit::textUnderCursor() const {
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::LineUnderCursor);
    auto text = tc.selectedText();
    return text.mid(text.lastIndexOf(' ') + 1);
}

bool WingCodeEdit::isAutoCloseChar(const QString &ch) const {
    // auto close: {} [] () "" ''
    return ch == QStringLiteral("{") || ch == QStringLiteral("[") ||
           ch == QStringLiteral("(");
}

bool WingCodeEdit::isPairedCloseChar(const QString &ch) const {
    // auto close: {} [] () "" ''
    return ch == QStringLiteral("}") || ch == QStringLiteral("]") ||
           ch == QStringLiteral(")");
}

QString WingCodeEdit::getPairedBeginChar(const QString &ch) const {
    // auto close: {} [] () "" ''
    if (ch == QStringLiteral("}")) {
        return QStringLiteral("{");
    } else if (ch == QStringLiteral("]")) {
        return QStringLiteral("[");
    } else if (ch == QStringLiteral(")")) {
        return QStringLiteral("(");
    } else {
        return {};
    }
}

QString WingCodeEdit::getPairedCloseChar(const QString &ch) const {
    // auto close: {} [] () "" ''
    if (ch == QStringLiteral("{")) {
        return QStringLiteral("}");
    } else if (ch == QStringLiteral("[")) {
        return QStringLiteral("]");
    } else if (ch == QStringLiteral("(")) {
        return QStringLiteral(")");
    } else {
        return {};
    }
}

QTextBlock WingCodeEdit::getCursorPositionBlock(int position) const {
    QTextCursor cursor = textCursor();
    cursor.setPosition(position);
    return cursor.block();
}

QString WingCodeEdit::syntaxName() const {
    return m_highlighter->definition().name();
}

void WingCodeEdit::updateMargins() {
    setViewportMargins(lineMarginWidth(), 0, 0, 0);
}

void WingCodeEdit::repaintMargins() {
    update(0, 0, realSymbolMarkSizeWithPadding(), height());
}

void WingCodeEdit::updateLineNumbers(const QRect &rect, int dy) {
    if (dy)
        m_lineMargin->scroll(0, dy);
    else
        m_lineMargin->update(0, rect.y(), m_lineMargin->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateMargins();
}

static bool isQuote(const QChar &ch) {
    return ch == QLatin1Char('"') || ch == QLatin1Char('\'');
}

static bool braceMatch(const QChar &left, const QChar &right) {
    switch (left.unicode()) {
    case '{':
        return right == QLatin1Char('}');
    case '(':
        return right == QLatin1Char(')');
    case '[':
        return right == QLatin1Char(']');
    case '}':
        return right == QLatin1Char('{');
    case ')':
        return right == QLatin1Char('(');
    case ']':
        return right == QLatin1Char('[');
    default:
        return false;
    }
}

static bool isOpenBrace(const QChar &ch) {
    return ch == QLatin1Char('(') || ch == QLatin1Char('[') ||
           ch == QLatin1Char('{');
}

static bool isCloseBrace(const QChar &ch) {
    return ch == QLatin1Char(')') || ch == QLatin1Char(']') ||
           ch == QLatin1Char('}');
}

struct BraceMatchResult {
    BraceMatchResult() : position(-1), validMatch(false) {}
    BraceMatchResult(int pos, bool valid) : position(pos), validMatch(valid) {}

    int position;
    bool validMatch;
};

static BraceMatchResult findNextBrace(QTextBlock block, int position) {
    QStack<QChar> balance;
    do {
        QString text = block.text();
        while (position < text.size()) {
            const QChar ch = text.at(position);
            if (isQuote(ch)) {
                if (!balance.isEmpty() && isQuote(balance.top())) {
                    if (balance.top() == ch)
                        (void)balance.pop();
                } else {
                    balance.push(ch);
                }
            } else if (!balance.isEmpty() && isQuote(balance.top())) {
                /* Don't look for matching braces until we exit the quote */
            } else if (isOpenBrace(ch)) {
                balance.push(ch);
            } else if (isCloseBrace(ch)) {
                if (balance.isEmpty())
                    return BraceMatchResult();
                const QChar match = balance.pop();
                if (balance.isEmpty())
                    return BraceMatchResult(block.position() + position,
                                            braceMatch(match, ch));
            }
            ++position;
        }

        block = block.next();
        position = 0;
    } while (block.isValid());

    // No match found in the document
    return BraceMatchResult();
}

static BraceMatchResult findPrevBrace(QTextBlock block, int position) {
    QStack<QChar> balance;
    do {
        QString text = block.text();
        while (position > 0) {
            --position;
            const QChar ch = text.at(position);
            if (isQuote(ch)) {
                if (!balance.isEmpty() && isQuote(balance.top())) {
                    if (balance.top() == ch)
                        (void)balance.pop();
                } else {
                    balance.push(ch);
                }
            } else if (!balance.isEmpty() && isQuote(balance.top())) {
                /* Don't look for matching braces until we exit the quote */
            } else if (isCloseBrace(ch)) {
                balance.push(ch);
            } else if (isOpenBrace(ch)) {
                if (balance.isEmpty())
                    return BraceMatchResult();
                const QChar match = balance.pop();
                if (balance.isEmpty())
                    return BraceMatchResult(block.position() + position,
                                            braceMatch(match, ch));
            }
        }

        block = block.previous();
        position = block.text().size();
    } while (block.isValid());

    // No match found in the document
    return BraceMatchResult();
}

void WingCodeEdit::updateCursor() {
    m_braceMatch.clear();

    if (matchBraces()) {
        QTextCursor cursor = textCursor();
        cursor.clearSelection();
        const QString blockText = cursor.block().text();
        const int blockPos = cursor.positionInBlock();
        const QChar chPrev = (blockPos > 0)
                                 ? blockText[cursor.positionInBlock() - 1]
                                 : QLatin1Char(0);
        const QChar chNext = (blockPos < blockText.size())
                                 ? blockText[cursor.positionInBlock()]
                                 : QLatin1Char(0);
        BraceMatchResult match;
        if (isOpenBrace(chNext)) {
            match = findNextBrace(cursor.block(), blockPos);
        } else if (isCloseBrace(chPrev)) {
            match = findPrevBrace(cursor.block(), blockPos);
            cursor.movePosition(QTextCursor::PreviousCharacter);
        }

        if (match.position >= 0) {
            QTextEdit::ExtraSelection selection;
            selection.format.setBackground(match.validMatch ? m_braceMatchBg
                                                            : m_errorBg);
            selection.cursor = cursor;
            selection.cursor.movePosition(QTextCursor::NextCharacter,
                                          QTextCursor::KeepAnchor);
            m_braceMatch.append(selection);
            selection.cursor = textCursor();
            selection.cursor.setPosition(match.position);
            selection.cursor.movePosition(QTextCursor::NextCharacter,
                                          QTextCursor::KeepAnchor);
            m_braceMatch.append(selection);
        }
    }

    updateExtraSelections();

    // Ensure the block containing cursor is fully unfolded
    QTextBlock cursorBlock = textCursor().block();
    if (!cursorBlock.isVisible()) {
        QTextBlock block = cursorBlock.previous();
        QStack<QTextBlock> foldStack;
        while (block.isValid()) {
            if (WingSyntaxHighlighter::isFolded(block) &&
                m_highlighter->foldContains(block, cursorBlock))
                foldStack << block;
            block = block.previous();
        }
        while (!foldStack.isEmpty())
            m_highlighter->unfoldBlock(foldStack.pop());
        WingSyntaxHighlighter::hideBlock(cursorBlock, false);
        updateScrollBars();
    }

    // If the previous block is folded but the current block is visible, that
    // means we've inserted a new block after a folded one and need to unfold
    // it. This can happen when pressing return at the end of a folded block.
    QTextBlock previousBlock = cursorBlock.previous();
    if (previousBlock.isValid() &&
        WingSyntaxHighlighter::isFolded(previousBlock)) {
        if (m_highlighter->isFoldable(previousBlock)) {
            m_highlighter->unfoldBlock(previousBlock);
            updateScrollBars();
        } else {
            previousBlock.setUserState(-1);
        }
    }

    // Ensure the fold marker for the current line is correct (e.g. in case
    // of deletion or undo/redo actions)
    QTextBlock nextBlock = cursorBlock.next();
    cursorBlock.setUserState(nextBlock.isVisible() ? -1 : 1);

    // Ensure the entire viewport gets repainted to account for the
    // "current line" highlight change
    viewport()->update();

    // Also update the entire line number margin; otherwise word-wrapped lines
    // may not get the correct block updated
    m_lineMargin->update();
}

void WingCodeEdit::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);

    QRect rect = contentsRect();
    rect.setWidth(lineMarginWidth());
    m_lineMargin->setGeometry(rect);
}

void WingCodeEdit::cutLines() {
    auto cursor = textCursor();
    if (!cursor.hasSelection()) {
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    }
    cut();
}

void WingCodeEdit::copyLines() {
    auto cursor = textCursor();
    if (!cursor.hasSelection()) {
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    }
    copy();
}

void WingCodeEdit::indentSelection() {
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    const int startPos = cursor.selectionStart();
    cursor.setPosition(cursor.selectionEnd());
    const int endBlock = (cursor.position() == cursor.block().position())
                             ? cursor.blockNumber() - 1
                             : cursor.blockNumber();
    cursor.setPosition(startPos);
    do {
        int startOfLine = 0;
        const QString blockText = cursor.block().text();
        const int leadingIndent =
            m_highlighter->leadingIndentation(blockText, &startOfLine);

        if (!blockText.isEmpty()) {
            cursor.movePosition(QTextCursor::StartOfLine);
            cursor.movePosition(QTextCursor::NextCharacter,
                                QTextCursor::KeepAnchor, startOfLine);
            cursor.removeSelectedText();

            const int indent = leadingIndent +
                               (m_indentationMode == IndentationMode::IndentTabs
                                    ? m_tabCharSize
                                    : m_indentWidth);
            if (m_indentationMode == IndentationMode::IndentSpaces) {
                cursor.insertText(QString(indent, QLatin1Char(' ')));
            } else {
                const int tabs = indent / m_tabCharSize;
                const int spaces = indent % m_tabCharSize;
                cursor.insertText(QString(tabs, QLatin1Char('\t')));
                cursor.insertText(QString(spaces, QLatin1Char(' ')));
            }
        }

        if (!cursor.movePosition(QTextCursor::NextBlock))
            break;
    } while (cursor.blockNumber() <= endBlock);

    cursor.endEditBlock();
}

void WingCodeEdit::outdentSelection() {
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    const int startPos = cursor.selectionStart();
    cursor.setPosition(cursor.selectionEnd());
    const int endBlock = (cursor.position() == cursor.block().position())
                             ? cursor.blockNumber() - 1
                             : cursor.blockNumber();
    cursor.setPosition(startPos);
    do {
        int startOfLine = 0;
        const QString blockText = cursor.block().text();
        const int leadingIndent =
            m_highlighter->leadingIndentation(blockText, &startOfLine);

        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                            startOfLine);
        cursor.removeSelectedText();

        const int indent =
            leadingIndent - (m_indentationMode == IndentationMode::IndentTabs
                                 ? m_tabCharSize
                                 : m_indentWidth);
        if (indent > 0) {
            if (m_indentationMode == IndentationMode::IndentSpaces) {
                cursor.insertText(QString(indent, QLatin1Char(' ')));
            } else {
                const int tabs = indent / m_tabCharSize;
                const int spaces = indent % m_tabCharSize;
                cursor.insertText(QString(tabs, QLatin1Char('\t')));
                cursor.insertText(QString(spaces, QLatin1Char(' ')));
            }
        }

        if (!cursor.movePosition(QTextCursor::NextBlock))
            break;
    } while (cursor.blockNumber() <= endBlock);

    cursor.endEditBlock();
}

void WingCodeEdit::foldCurrentLine() {
    QTextCursor cursor = textCursor();
    QTextBlock block = cursor.block();
    while (block.isValid() &&
           !m_highlighter->foldContains(block, cursor.block()))
        block = block.previous();
    if (block.isValid() && !WingSyntaxHighlighter::isFolded(block)) {
        m_highlighter->foldBlock(block);

        // Move the editing cursor if it was in a folded block
        if (cursor.block() != block) {
            cursor.setPosition(block.position());
            setTextCursor(cursor);
        }

        viewport()->update();
        m_lineMargin->update();
        updateScrollBars();
    }
}

void WingCodeEdit::unfoldCurrentLine() {
    const QTextBlock cursorBlock = textCursor().block();
    if (m_highlighter->isFoldable(cursorBlock) &&
        WingSyntaxHighlighter::isFolded(cursorBlock)) {
        m_highlighter->unfoldBlock(cursorBlock);
        viewport()->update();
        m_lineMargin->update();
        updateScrollBars();
    }
}

void WingCodeEdit::foldAll() {
    QTextBlock block = document()->begin();
    while (block.isValid()) {
        if (m_highlighter->isFoldable(block))
            m_highlighter->foldBlock(block);
        block = block.next();
    }

    // Move the editing cursor if it was in a folded block
    QTextCursor cursor = textCursor();
    block = cursor.block();
    while (block.isValid() && !block.isVisible())
        block = block.previous();
    if (block.isValid()) {
        cursor.setPosition(block.position());
        setTextCursor(cursor);
    }

    viewport()->update();
    m_lineMargin->update();
    updateScrollBars();
    ensureCursorVisible();
}

void WingCodeEdit::unfoldAll() {
    QTextBlock block = document()->begin();
    while (block.isValid()) {
        // Just make everything visible/unfolded regardless of what state
        // it was previously in.
        block.setUserState(-1);
        WingSyntaxHighlighter::hideBlock(block, false);
        block = block.next();
    }

    viewport()->update();
    m_lineMargin->update();
    updateScrollBars();
    ensureCursorVisible();
}

void WingCodeEdit::zoomIn() {
    QPlainTextEdit::zoomIn(1);
    updateTextMetrics();
}

void WingCodeEdit::zoomOut() {
    QPlainTextEdit::zoomOut(1);
    updateTextMetrics();
}

void WingCodeEdit::zoomReset() {
    setFont(defaultFont());
    updateTextMetrics();
}

void WingCodeEdit::keyPressEvent(QKeyEvent *e) {
    if (m_completer && m_completer->popup()->isVisible()) {
        // The following keys are forwarded by the completer to the widget
        switch (e->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            e->ignore();
            return; // let the completer do default behavior
        default:
            break;
        }
    }

    if (processKeyShortcut(e)) {
        return;
    }

    switch (e->key()) {
    case Qt::Key_Tab:
        if (textCursor().hasSelection()) {
            indentSelection();
        } else if (m_indentationMode == IndentationMode::IndentTabs) {
            textCursor().insertText(QStringLiteral("\t"));
        } else if (m_indentationMode == IndentationMode::IndentSpaces) {
            QTextCursor cursor = textCursor();
            const QString blockText = cursor.block().text();
            const auto cursorText =
                QStringView(blockText).left(cursor.positionInBlock());
            int vpos = 0;
            for (const auto ch : cursorText) {
                if (ch == QLatin1Char('\t'))
                    vpos += (m_tabCharSize - (vpos % m_tabCharSize));
                else
                    vpos += 1;
            }
            const int spaces = m_indentWidth - (vpos % m_indentWidth);
            cursor.insertText(QString(spaces, QLatin1Char(' ')));
        } else {
            QTextCursor cursor = textCursor();
            const QString blockText = cursor.block().text();
            const auto cursorText =
                QStringView(blockText).left(cursor.positionInBlock());
            int vpos = 0, cpos = 0;
            int wsvStart = 0, wscStart = 0;
            for (const auto ch : cursorText) {
                cpos += 1;
                if (ch == QLatin1Char('\t')) {
                    vpos += (m_tabCharSize - (vpos % m_tabCharSize));
                } else {
                    vpos += 1;
                    if (ch != QLatin1Char(' ')) {
                        wsvStart = vpos;
                        wscStart = cpos;
                    }
                }
            }

            // Fix up only the current block of whitespace up to the
            // cursor position.  This most closely matches vim's mixed
            // indentation (softtabstop+noexpandtab)
            cursor.beginEditBlock();
            cursor.movePosition(QTextCursor::PreviousCharacter,
                                QTextCursor::KeepAnchor, cpos - wscStart);
            cursor.removeSelectedText();

            const int indentTo = vpos + m_indentWidth - (vpos % m_indentWidth);
            vpos = wsvStart;
            const int alignTo = m_tabCharSize - (vpos % m_tabCharSize);
            if (vpos + alignTo <= indentTo) {
                cursor.insertText(QStringLiteral("\t"));
                vpos += alignTo;
            }
            const int tabs = (indentTo - vpos) / m_tabCharSize;
            const int spaces = (indentTo - vpos) % m_tabCharSize;
            cursor.insertText(QString(tabs, QLatin1Char('\t')));
            cursor.insertText(QString(spaces, QLatin1Char(' ')));
            cursor.endEditBlock();
        }
        break;

    case Qt::Key_Backtab:
        outdentSelection();
        break;

    case Qt::Key_Up:
        if (e->modifiers() & Qt::ControlModifier) {
            auto scrollBar = verticalScrollBar();
            scrollBar->setValue(scrollBar->value() - 1);
        } else {
            QPlainTextEdit::keyPressEvent(e);
        }
        break;

    case Qt::Key_Down:
        if (e->modifiers() & Qt::ControlModifier) {
            auto scrollBar = verticalScrollBar();
            scrollBar->setValue(scrollBar->value() + 1);
        } else {
            QPlainTextEdit::keyPressEvent(e);
        }
        break;

    case Qt::Key_Delete:
    case Qt::Key_Backspace:
        // ignore
        break;
    default:
        if (autoCloseChar()) {
            auto text = e->text();
            if (isAutoCloseChar(text)) {
                QTextCursor cursor = textCursor();
                if (cursor.hasSelection()) {
                    QString content =
                        text + cursor.selectedText() + getPairedCloseChar(text);
                    cursor.beginEditBlock();
                    cursor.removeSelectedText();
                    cursor.insertText(content);
                    cursor.endEditBlock();
                } else {
                    cursor.beginEditBlock();
                    cursor.insertText(text + getPairedCloseChar(text));
                    cursor.endEditBlock();
                    cursor.movePosition(QTextCursor::PreviousCharacter,
                                        QTextCursor::MoveAnchor);
                    setTextCursor(cursor);
                }
            } else if (text == QStringLiteral("\"") ||
                       text == QStringLiteral("'")) {
                QTextCursor cursor = textCursor();
                if (cursor.hasSelection()) {
                    auto selstart =
                        getCursorPositionBlock(cursor.selectionStart())
                            .blockNumber();
                    auto selend = getCursorPositionBlock(cursor.selectionEnd())
                                      .blockNumber();
                    if (selstart == selend) {
                        if (cursor.hasSelection()) {
                            QString content =
                                text + cursor.selectedText() + text;
                            cursor.beginEditBlock();
                            cursor.removeSelectedText();
                            cursor.insertText(content);
                            cursor.endEditBlock();
                        } else {
                            auto ch = cursorNextChar(cursor);
                            if (ch == QStringLiteral("\"") ||
                                ch == QStringLiteral("'")) {
                                if (cursorPrevChar(cursor) ==
                                    QStringLiteral("\\")) {
                                    cursor.insertText(text);
                                } else {
                                    cursor.movePosition(
                                        QTextCursor::NextCharacter);
                                    setTextCursor(cursor);
                                }
                            } else {
                                cursor.insertText(text + text);
                                cursor.movePosition(
                                    QTextCursor::PreviousCharacter,
                                    QTextCursor::MoveAnchor);
                                setTextCursor(cursor);
                            }
                        }
                    }
                } else {
                    auto pchar = cursorPrevChar(cursor);
                    if (pchar == text && cursorNextChar(cursor) == text) {
                        cursor.movePosition(QTextCursor::NextCharacter);
                        setTextCursor(cursor);
                    } else {
                        if (pchar == QStringLiteral("\\")) {
                            cursor.insertText(text);
                        } else {
                            cursor.insertText(text + text);
                            cursor.movePosition(QTextCursor::PreviousCharacter,
                                                QTextCursor::MoveAnchor);
                            setTextCursor(cursor);
                        }
                    }
                }
            } else {
                QPlainTextEdit::keyPressEvent(e);
            }
        } else {
            QPlainTextEdit::keyPressEvent(e);
        }
        break;
    }

    updateCursor();

    if (m_completer) {
        const bool hasModifier = e->modifiers() != Qt::NoModifier;
        auto completionContent = textUnderCursor();

        if (completionContent.isEmpty()) {
            m_completer->popup()->hide();
            return;
        }

        if (hasModifier || e->text().isEmpty() ||
            completionContent.length() < m_completer->triggerAmount() ||
            m_completer->wordSeperators().contains(
                completionContent.right(1))) {
            m_completer->popup()->hide();
            return;
        }

        auto triggers = m_completer->triggerList();
        auto r = std::find_if(triggers.cbegin(), triggers.cend(),
                              [completionContent](const QString &trigger) {
                                  return completionContent.endsWith(trigger);
                              });
        auto currect = cursorRect();
        if (r == triggers.cend()) {
            m_completer->trigger({}, completionContent, currect);
        } else {
            auto t = *r;
            completionContent.remove(
                completionContent.length() - t.length() - 1, t.length());
            m_completer->trigger(t, completionContent, currect);
        }
    }
}

void WingCodeEdit::wheelEvent(QWheelEvent *e) {
    if (e->modifiers() & Qt::ControlModifier) {
        // NOTE: This actually changes the font size
        if (e->angleDelta().y() > 0)
            zoomIn();
        else if (e->angleDelta().y() < 0)
            zoomOut();
    } else {
        QPlainTextEdit::wheelEvent(e);
    }
}

void WingCodeEdit::paintEvent(QPaintEvent *e) {
    const QRect eventRect = e->rect();
    const QRect viewRect = viewport()->rect();
    QRectF cursorBlockRect;

#if defined(Q_OS_WIN) && QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    if (m_styleNeedsBgRepaint) {
        // Draw the background.  This should be handled by
        // QPlainTextEdit::paintEvent(), but some styles (notably, Qt's
        // Windows11 style) ignore the provided background color and use their
        // own.
        QPainter p(viewport());
        p.fillRect(eventRect, m_editorBg);
    }
#endif

    const QTextCursor cursor = textCursor();
    if (highlightCurrentLine()) {
        // Highlight current line first, so the long line marker will draw over
        // it Unlike setExtraSelections(), we paint the entire line past even
        // the document margins.
        cursorBlockRect = blockBoundingGeometry(cursor.block());
        cursorBlockRect.translate(contentOffset());
        cursorBlockRect.setLeft(eventRect.left());
        cursorBlockRect.setWidth(eventRect.width());
        if (eventRect.intersects(cursorBlockRect.toAlignedRect())) {
            QPainter p(viewport());
            p.setRenderHint(QPainter::Antialiasing);
            p.setRenderHint(QPainter::TextAntialiasing);
            p.setRenderHint(QPainter::SmoothPixmapTransform);
            p.fillRect(cursorBlockRect, m_cursorLineBg);
        }
    }

    if (showLongLineEdge() && m_longLineMarker > 0) {
        QFontMetricsF fm(font());
        const qreal longLinePos =
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
            fm.horizontalAdvance(QString(m_longLineMarker, QLatin1Char('x')))
#else
            fm.width(QString(m_longLineMarker, QLatin1Char('x')))
#endif
            + contentOffset().x() + document()->documentMargin();
        if (longLinePos < viewRect.width()) {
            QPainter p(viewport());
            p.setRenderHint(QPainter::Antialiasing);
            p.setRenderHint(QPainter::TextAntialiasing);
            p.setRenderHint(QPainter::SmoothPixmapTransform);
            QRectF longLineRect(longLinePos, eventRect.top(),
                                viewRect.width() - longLinePos,
                                eventRect.height());
            p.fillRect(longLineRect, m_longLineBg);
            if (longLineRect.intersects(cursorBlockRect))
                p.fillRect(cursorBlockRect.intersected(longLineRect),
                           m_longLineCursorBg);
            p.setPen(m_longLineEdge);
            p.drawLine(longLinePos, eventRect.top(), longLinePos,
                       eventRect.bottom());
        }
    }

    QTextBlock block = firstVisibleBlock();
    while (block.isValid()) {
        QRectF blockRect =
            blockBoundingGeometry(block).translated(contentOffset());
        if (blockRect.top() > eventRect.bottom())
            break;

        if (m_highlighter->isFoldable(block) &&
            WingSyntaxHighlighter::isFolded(block)) {
            QPainter p(viewport());
            p.setRenderHint(QPainter::Antialiasing);
            p.setRenderHint(QPainter::TextAntialiasing);
            p.setRenderHint(QPainter::SmoothPixmapTransform);
            blockRect.setLeft(eventRect.left());
            blockRect.setRight(eventRect.right());
            p.setPen(QPen(m_codeFoldingBg, 1.0, Qt::DashLine));
            // The line is drawn one pixel up from the bottom to avoid getting
            // missed when scrolling the line into view from the top...
            p.drawLine(blockRect.left(), blockRect.bottom() - 1,
                       blockRect.right(), blockRect.bottom() - 1);
            block = m_highlighter->findFoldEnd(block);
        } else {
            block = block.next();
        }
    }

    QPlainTextEdit::paintEvent(e);

    // Overlay indentation guides after rendering the text
    if (showIndentGuides()) {
        QPainter p(viewport());
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.setPen(m_indentGuideFg);
        block = firstVisibleBlock();
        const QFontMetricsF fm(font());
        const int guideWidth =
            (m_indentationMode == IndentationMode::IndentTabs ? m_tabCharSize
                                                              : m_indentWidth);
        const qreal indentLine = indentAdvance(fm, guideWidth);
        const qreal lineOffset =
            contentOffset().x() + document()->documentMargin();
        while (block.isValid()) {
            QString blockText = block.text();
            int wsColumn = 0;
            bool onlySpaces = true;
            for (auto &ch : blockText) {
                if (ch == QLatin1Char('\t')) {
                    wsColumn =
                        wsColumn - (wsColumn % m_tabCharSize) + m_tabCharSize;
                } else if (ch.isSpace()) {
                    ++wsColumn;
                } else {
                    onlySpaces = false;
                    break;
                }
            }
            if (onlySpaces) {
                // Pretend we have one more column so whitespace-only lines
                // show the indent guideline when applicable
                wsColumn += 1;
            }
            QRectF blockRect = blockBoundingGeometry(block);
            blockRect.translate(contentOffset());
            wsColumn = (wsColumn + guideWidth - 1) / guideWidth;
            for (int i = 1; i < wsColumn; ++i) {
                if (cursor.blockNumber() == block.blockNumber() &&
                    cursor.positionInBlock() == (guideWidth * i))
                    continue;

                const qreal lineX = (indentLine * i) + lineOffset;
                p.drawLine(QPointF(lineX, blockRect.top()),
                           QPointF(lineX, blockRect.bottom()));
            }
            block = block.next();
        }
    }
}

void WingCodeEdit::focusInEvent(QFocusEvent *e) {
    if (m_completer)
        m_completer->setWidget(this);
    QPlainTextEdit::focusInEvent(e);
}

bool WingCodeEdit::processKeyShortcut(QKeyEvent *e) {
    if (e->matches(QKeySequence::Undo)) {
        undo();
        return true;
    }
    if (e->matches(QKeySequence::Redo)) {
        redo();
        return true;
    }

    // Custom versions of Cut and Copy
    if (e->matches(QKeySequence::Cut)) {
        cutLines();
        return true;
    }
    if (e->matches(QKeySequence::Copy)) {
        copyLines();
        return true;
    }

    // Qt's default implementation doesn't correctly adjust the cursor
    // X position after deleting a selection.
    if (e->matches(QKeySequence::Delete)) {
        QTextCursor cursor = textCursor();
        cursor.deleteChar();
        cursor.setVerticalMovementX(-1);
        setTextCursor(cursor);
        return false; // need more processing
    }
    if (e->matches(QKeySequence::Backspace) ||
        (e->key() == Qt::Key_Backspace &&
         (e->modifiers() & ~Qt::ShiftModifier) == 0)) {
        QTextCursor cursor = textCursor();
        cursor.deletePreviousChar();
        cursor.setVerticalMovementX(-1);
        setTextCursor(cursor);
        return false; // need more processing
    }

    // "Smart" home/end keys
    if (e->matches(QKeySequence::MoveToStartOfLine)) {
        smartHome(QTextCursor::MoveAnchor);
        return true;
    }
    if (e->matches(QKeySequence::SelectStartOfLine)) {
        smartHome(QTextCursor::KeepAnchor);
        return true;
    }
    if (e->matches(QKeySequence::MoveToEndOfLine)) {
        smartEnd(QTextCursor::MoveAnchor);
        return true;
    }
    if (e->matches(QKeySequence::SelectEndOfLine)) {
        smartEnd(QTextCursor::KeepAnchor);
        return true;
    }

    // Handle newline insertion
    if (e->matches(QKeySequence::InsertParagraphSeparator)) {
        QTextCursor cursor = textCursor();
        cursor.beginEditBlock();

        QPlainTextEdit::keyPressEvent(e);

        // Simple auto-indent: Just copy the previous non-empty line's
        // leading whitespace
        if (autoIndent()) {
            QTextCursor scanCursor = textCursor();
            int startOfLine = 0;
            while (scanCursor.blockNumber() > 0 && startOfLine == 0) {
                scanCursor.movePosition(QTextCursor::PreviousBlock);

                const QString blockText = scanCursor.block().text();
                for (const auto ch : blockText) {
                    if (ch.isSpace())
                        startOfLine += 1;
                    else
                        break;
                }
                if (startOfLine == 0 && !blockText.isEmpty()) {
                    // No leading whitespace, but line is not empty.
                    // Therefore, current leading indent level is 0.
                    break;
                }
            }
            if (startOfLine != 0) {
                const QString indentLine = scanCursor.block().text();
                const QString leadingIndent = indentLine.left(startOfLine);
                textCursor().insertText(leadingIndent);
                if (indentLine.size() == startOfLine &&
                    scanCursor.blockNumber() ==
                        textCursor().blockNumber() - 1) {
                    // We copied the previous blank (whitespace-only) line...
                    // Now we can clear out that line to clean up unnecessary
                    // trailing whitespace
                    scanCursor.movePosition(QTextCursor::StartOfBlock);
                    scanCursor.movePosition(QTextCursor::EndOfBlock,
                                            QTextCursor::KeepAnchor);
                    scanCursor.removeSelectedText();
                }
            }
        }
        cursor.endEditBlock();

        updateCursor();
        return true;
    }
    if (e->matches(QKeySequence::InsertLineSeparator)) {
        // Don't allow QPlainTextEdit to insert a soft break :(
        QKeyEvent retnEvent(e->type(), Qt::Key_Enter, Qt::NoModifier,
                            e->nativeScanCode(), e->nativeVirtualKey(),
                            e->nativeModifiers(), e->text(), e->isAutoRepeat(),
                            e->count());
        QPlainTextEdit::keyPressEvent(&retnEvent);

        updateCursor();
        return true;
    }

    return false;
}

void WingCodeEdit::onCompletion(const QString &completion) {
    if (m_completer->widget() != this)
        return;
    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::WordLeft, QTextCursor::KeepAnchor);
    tc.removeSelectedText();
    tc.insertText(completion);
    setTextCursor(tc);
}

void WingCodeEdit::printDocument(QPrinter *printer) {
    // Override settings for printing
    auto displayFont = font();
    setFont(defaultFont());
    updateTextMetrics();

    auto displayTheme = m_highlighter->theme();
    auto printingTheme = syntaxRepo().theme(QStringLiteral("Printing"));
    if (!printingTheme.isValid())
        printingTheme = syntaxRepo().defaultTheme(
            KSyntaxHighlighting::Repository::LightTheme);
    if (printingTheme.isValid())
        setTheme(printingTheme);

    auto displayOption = document()->defaultTextOption();
    auto printOption = displayOption;
    printOption.setFlags(printOption.flags() & ~QTextOption::ShowTabsAndSpaces);
    document()->setDefaultTextOption(printOption);

    auto displayWrapMode = wordWrapMode();
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    // Let the document handle its own print formatting
    print(printer);

    // Restore display settings
    setWordWrapMode(displayWrapMode);
    document()->setDefaultTextOption(displayOption);
    setTheme(displayTheme);
    setFont(displayFont);
    updateTextMetrics();
}

void WingCodeEdit::updateScrollBars() {
    // We don't have access to QPlainTextEdit's private APIs for updating
    // document layout or adjusting the scroll bars, but we can invoke
    // them by sending a dummy resize event...
    QResizeEvent dummyResize(size(), size());
    resizeEvent(&dummyResize);
}
