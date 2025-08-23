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
#include "abstracthighlighter_p.h"
#include "definition_p.h"
#include "format.h"
#include "format_p.h"
#include "themedata_p.h"

#include <KSyntaxHighlighting/Theme>
#include <QRegularExpression>
#include <QVector>

Q_DECLARE_METATYPE(QTextBlock)

using namespace KSyntaxHighlighting;

class WingSyntaxHighlighterPrivate : public AbstractHighlighterPrivate {
public:
    static FoldingRegion foldingRegion(const QTextBlock &startBlock);
    void initTextFormat(QTextCharFormat &tf, const Format &format);
    void computeTextFormats();

    struct TextFormat {
        QTextCharFormat tf;
        /**
         * id to check that the format belongs to the definition
         */
        std::intptr_t ptrId;
    };

    QList<FoldingRegion> foldingRegions;
    QVector<TextFormat> tfs;
};

FoldingRegion
WingSyntaxHighlighterPrivate::foldingRegion(const QTextBlock &startBlock) {
    const auto data =
        dynamic_cast<WingTextBlockUserData *>(startBlock.userData());
    if (!data) {
        return FoldingRegion();
    }
    for (int i = data->foldingRegions.size() - 1; i >= 0; --i) {
        if (data->foldingRegions.at(i).type() == FoldingRegion::Begin) {
            return data->foldingRegions.at(i);
        }
    }
    return FoldingRegion();
}

void WingSyntaxHighlighterPrivate::initTextFormat(QTextCharFormat &tf,
                                                  const Format &format) {
    // always set the foreground color to avoid palette issues
    tf.setForeground(format.textColor(m_theme));

    if (format.hasBackgroundColor(m_theme)) {
        tf.setBackground(format.backgroundColor(m_theme));
    }
    if (format.isBold(m_theme)) {
        tf.setFontWeight(QFont::Bold);
    }
    if (format.isItalic(m_theme)) {
        tf.setFontItalic(true);
    }
    if (format.isUnderline(m_theme)) {
        tf.setFontUnderline(true);
    }
    if (format.isStrikeThrough(m_theme)) {
        tf.setFontStrikeOut(true);
    }
}

void WingSyntaxHighlighterPrivate::computeTextFormats() {
    auto definitions = m_definition.includedDefinitions();
    definitions.append(m_definition);

    int maxId = 0;
    for (const auto &definition : std::as_const(definitions)) {
        for (const auto &format :
             std::as_const(DefinitionData::get(definition)->formats)) {
            maxId = qMax(maxId, format.id());
        }
    }
    tfs.clear();
    tfs.resize(maxId + 1);

    // initialize tfs
    for (const auto &definition : std::as_const(definitions)) {
        for (const auto &format :
             std::as_const(DefinitionData::get(definition)->formats)) {
            auto &tf = tfs[format.id()];
            tf.ptrId = FormatPrivate::ptrId(format);
            initTextFormat(tf.tf, format);
        }
    }
}

WingSyntaxHighlighter::WingSyntaxHighlighter(QObject *parent)
    : QSyntaxHighlighter(parent),
      AbstractHighlighter(new WingSyntaxHighlighterPrivate) {
    qRegisterMetaType<QTextBlock>();
}

WingSyntaxHighlighter::WingSyntaxHighlighter(QTextDocument *document)
    : QSyntaxHighlighter(document),
      AbstractHighlighter(new WingSyntaxHighlighterPrivate), m_tabCharSize() {
    qRegisterMetaType<QTextBlock>();
}

WingSyntaxHighlighter::~WingSyntaxHighlighter() {}

void WingSyntaxHighlighter::setDefinition(
    const KSyntaxHighlighting::Definition &def) {
    Q_D(WingSyntaxHighlighter);

    const auto needsRehighlight = d->m_definition != def;
    if (DefinitionData::get(d->m_definition) != DefinitionData::get(def)) {
        d->m_definition = def;
        d->tfs.clear();
    }
    if (needsRehighlight) {
        rehighlight();
    }
}

void WingSyntaxHighlighter::setTheme(const KSyntaxHighlighting::Theme &theme) {
    Q_D(WingSyntaxHighlighter);
    if (ThemeData::get(d->m_theme) != ThemeData::get(theme)) {
        d->m_theme = theme;
        d->tfs.clear();
    }
}

bool WingSyntaxHighlighter::startsFoldingRegion(
    const QTextBlock &startBlock) const {
    return WingSyntaxHighlighterPrivate::foldingRegion(startBlock).type() ==
           FoldingRegion::Begin;
}

QTextBlock WingSyntaxHighlighter::findFoldingRegionEnd(
    const QTextBlock &startBlock) const {
    const auto region = WingSyntaxHighlighterPrivate::foldingRegion(startBlock);

    auto block = startBlock;
    int depth = 1;
    while (block.isValid()) {
        block = block.next();
        const auto data =
            dynamic_cast<WingTextBlockUserData *>(block.userData());
        if (!data) {
            continue;
        }
        for (const auto &foldingRegion : std::as_const(data->foldingRegions)) {
            if (foldingRegion.id() != region.id()) {
                continue;
            }
            if (foldingRegion.type() == FoldingRegion::End) {
                --depth;
            } else if (foldingRegion.type() == FoldingRegion::Begin) {
                ++depth;
            }
            if (depth == 0) {
                return block;
            }
        }
    }

    return QTextBlock();
}

WingTextBlockUserData *WingSyntaxHighlighter::createTextBlockUserData() {
    return new WingTextBlockUserData;
}

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

void WingSyntaxHighlighter::setSymbolMark(QTextBlock &block,
                                          const QString &id) {
    auto data = dynamic_cast<WingTextBlockUserData *>(block.userData());
    if (data) {
        data->symbolID = id;
    } else {
        // first time
        data = createTextBlockUserData();
        data->symbolID = id;
        block.setUserData(data);
    }
}

QString WingSyntaxHighlighter::symbolMarkID(const QTextBlock &block) {
    auto data = dynamic_cast<WingTextBlockUserData *>(block.userData());
    if (data) {
        return data->symbolID;
    }
    return {};
}

bool WingSyntaxHighlighter::containsSymbolMark(QTextBlock &block) {
    auto data = dynamic_cast<WingTextBlockUserData *>(block.userData());
    if (!data) {
        return false;
    }
    return !data->symbolID.isEmpty();
}

void WingSyntaxHighlighter::clearSymbolMark(QTextBlock &block) {
    auto data = dynamic_cast<WingTextBlockUserData *>(block.userData());
    if (data) {
        data->symbolID.clear();
    }
}

void WingSyntaxHighlighter::highlightBlock(const QString &text) {
    Q_D(WingSyntaxHighlighter);

    static const State emptyState;
    const State *previousState = &emptyState;
    if (currentBlock().position() > 0) {
        const auto prevBlock = currentBlock().previous();
        const auto prevData =
            dynamic_cast<WingTextBlockUserData *>(prevBlock.userData());
        if (prevData) {
            previousState = &prevData->state;
        }
    }
    d->foldingRegions.clear();
    auto newState = highlightLine(text, *previousState);

    auto data = dynamic_cast<WingTextBlockUserData *>(currentBlockUserData());
    if (!data) {
        // first time we highlight this
        data = createTextBlockUserData();
        data->state = std::move(newState);
        data->foldingRegions = d->foldingRegions;
        setCurrentBlockUserData(data);
        return;
    }

    if (data->state == newState && data->foldingRegions == d->foldingRegions) {
        // we ended up in the same state, so we are done here
        return;
    }
    data->state = std::move(newState);
    data->foldingRegions = d->foldingRegions;

    const auto nextBlock = currentBlock().next();
    if (nextBlock.isValid()) {
        QMetaObject::invokeMethod(this, "rehighlightBlock",
                                  Qt::QueuedConnection,
                                  Q_ARG(QTextBlock, nextBlock));
    }

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

void WingSyntaxHighlighter::applyFormat(
    int offset, int length, const KSyntaxHighlighting::Format &format) {
    if (length == 0) {
        return;
    }

    Q_D(WingSyntaxHighlighter);

    if (Q_UNLIKELY(d->tfs.empty())) {
        d->computeTextFormats();
    }

    const auto id = static_cast<std::size_t>(format.id());
    // This doesn't happen when format comes from the definition.
    // But as the user can override the function to pass any format, this is a
    // possible scenario.
    if (id < d->tfs.size() &&
        d->tfs[id].ptrId == FormatPrivate::ptrId(format)) {
        QSyntaxHighlighter::setFormat(offset, length, d->tfs[id].tf);
    } else {
        QTextCharFormat tf;
        d->initTextFormat(tf, format);
        QSyntaxHighlighter::setFormat(offset, length, tf);
    }
}

void WingSyntaxHighlighter::applyFolding(
    int offset, int length, KSyntaxHighlighting::FoldingRegion region) {
    Q_UNUSED(offset);
    Q_UNUSED(length);
    Q_D(WingSyntaxHighlighter);

    if (region.type() == FoldingRegion::Begin) {
        d->foldingRegions.push_back(region);
    }

    if (region.type() == FoldingRegion::End) {
        for (int i = d->foldingRegions.size() - 1; i >= 0; --i) {
            if (d->foldingRegions.at(i).id() != region.id() ||
                d->foldingRegions.at(i).type() != FoldingRegion::Begin) {
                continue;
            }
            d->foldingRegions.remove(i);
            return;
        }
        d->foldingRegions.push_back(region);
    }
}
