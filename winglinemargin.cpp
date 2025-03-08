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

#include "winglinemargin.h"

#include "wingcodeedit.h"
#include "wingsymbolcenter.h"
#include "wingsyntaxhighlighter.h"

#include <QPainter>
#include <QTextBlock>

QSize WingLineMargin::sizeHint() const {
    return QSize(m_editor->lineMarginWidth(), 0);
}

void WingLineMargin::wheelEvent(QWheelEvent *e) { m_editor->wheelEvent(e); }

WingLineMargin::WingLineMargin(WingCodeEdit *editor)
    : QWidget(editor), m_editor(editor), m_marginSelectStart(-1),
      m_foldHoverLine(-1) {
    setMouseTracking(true);
}

void WingLineMargin::paintEvent(QPaintEvent *paintEvent) {
    if (!m_editor->showSymbolMark() && !m_editor->showLineNumbers() &&
        !m_editor->showFolding())
        return;

    QPainter painter(this);
    painter.fillRect(paintEvent->rect(), m_editor->m_lineMarginBg);

    QTextBlock block = m_editor->firstVisibleBlock();
    qreal top = m_editor->blockBoundingGeometry(block)
                    .translated(m_editor->contentOffset())
                    .top();
    qreal bottom = top + m_editor->blockBoundingRect(block).height();
    const QFontMetricsF metrics(font());
    auto numWidth = metrics.boundingRect(QLatin1Char('0')).width();
    const int foldPixmapWidth = m_editor->m_foldOpen.width() + 2;
    const int symWidthOff =
        m_editor->showSymbolMark() ? metrics.height() + 2 : 0;
    const qreal numOffset = numWidth / 2.0 +
                            (m_editor->showFolding() ? foldPixmapWidth : 0) +
                            symWidthOff;

    QTextCursor cursor = m_editor->textCursor();

    auto highligher = m_editor->highlighter();
    while (block.isValid() && top <= paintEvent->rect().bottom()) {
        if (block.isVisible()) {
            if (m_editor->showSymbolMark()) {
                auto usrd = highligher->property(block, QStringLiteral("sym"));
                if (usrd.isValid()) {
                    auto symid = usrd.toString();
                    if (!symid.isEmpty()) {
                        auto sym =
                            WingSymbolCenter::instance().symbolFromName(symid);
                        if (!sym.isNull()) {
                            painter.drawPixmap(QRect(3, top, metrics.height(),
                                                     metrics.height()),
                                               sym, sym.rect());
                        }
                    }
                }
            }

            if (m_editor->showLineNumbers() &&
                bottom >= paintEvent->rect().top()) {
                const QString lineNum =
                    QString::number(block.blockNumber() + 1);
                if (block.blockNumber() == cursor.blockNumber())
                    painter.setPen(m_editor->m_cursorLineNum);
                else
                    painter.setPen(m_editor->m_lineMarginFg);
                const QRectF numberRect(symWidthOff, top, width() - numOffset,
                                        metrics.height());
                painter.drawText(numberRect, Qt::AlignRight, lineNum);
            }

            if (m_editor->showFolding() &&
                m_editor->m_highlighter->isFoldable(block)) {
                const bool blockFolded = WingSyntaxHighlighter::isFolded(block);
                if (block.blockNumber() == m_foldHoverLine) {
                    const int foldHighlightLeft =
                        width() - foldPixmapWidth -
                        (m_editor->showLineNumbers() ? 2 : 0);
                    QTextBlock endBlock =
                        m_editor->m_highlighter->findFoldEnd(block);
                    if (!endBlock.isValid())
                        endBlock = m_editor->document()->lastBlock();
                    const qreal foldHighlightBottom =
                        blockFolded ? bottom
                                    : m_editor->blockBoundingGeometry(endBlock)
                                          .translated(m_editor->contentOffset())
                                          .bottom();
                    painter.fillRect(foldHighlightLeft, top, width(),
                                     foldHighlightBottom - top,
                                     m_editor->m_codeFoldingBg);
                }

                const QPixmap &foldPixmap =
                    blockFolded ? m_editor->m_foldClosed : m_editor->m_foldOpen;
                painter.drawPixmap(
                    width() - foldPixmapWidth,
                    top + (metrics.height() - foldPixmap.height()) / 2,
                    foldPixmap);
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + m_editor->blockBoundingRect(block).height();
    }
}

void WingLineMargin::mouseMoveEvent(QMouseEvent *e) {
    const QPoint eventPos = e->pos();
    QTextCursor lineCursor =
        m_editor->cursorForPosition(QPoint(0, eventPos.y()));
    const int foldPixmapWidth = m_editor->m_foldOpen.width() + 4;

    m_foldHoverLine = -1;
    if (m_editor->showFolding()) {
        if (!m_editor->showLineNumbers() ||
            eventPos.x() >= width() - foldPixmapWidth) {
            QTextBlock block = lineCursor.block();
            if (block.isValid() && m_editor->m_highlighter->isFoldable(block))
                m_foldHoverLine = block.blockNumber();
        }
        update();
    }

    if ((e->buttons() & Qt::LeftButton) && m_marginSelectStart >= 0) {
        const int linePosition = lineCursor.position();
        lineCursor.setVisualNavigation(true);
        lineCursor.setPosition(m_marginSelectStart);
        if (linePosition >= m_marginSelectStart) {
            lineCursor.setPosition(linePosition, QTextCursor::KeepAnchor);
            lineCursor.movePosition(QTextCursor::NextBlock,
                                    QTextCursor::KeepAnchor);
        } else {
            lineCursor.movePosition(QTextCursor::NextBlock);
            lineCursor.setPosition(linePosition, QTextCursor::KeepAnchor);
        }
        m_editor->setTextCursor(lineCursor);
    } else {
        m_marginSelectStart = -1;
    }

    QWidget::mouseMoveEvent(e);
}

void WingLineMargin::mousePressEvent(QMouseEvent *e) {
    m_marginSelectStart = -1;
    if (e->button() == Qt::LeftButton) {
        const QPoint eventPos = e->pos();
        const int foldPixmapWidth = m_editor->m_foldOpen.width() + 4;
        QTextCursor lineCursor =
            m_editor->cursorForPosition(QPoint(0, eventPos.y()));

        if (m_editor->showSymbolMark() &&
            eventPos.x() < m_editor->realSymbolMarkSizeWithPadding()) {
            emit symbolMarkLineMarginClicked(lineCursor.blockNumber() + 1);
        } else if (m_editor->showLineNumbers() &&
                   (!m_editor->showFolding() ||
                    eventPos.x() < width() - foldPixmapWidth)) {
            // Clicked in the number margin
            lineCursor.setVisualNavigation(true);
            lineCursor.movePosition(QTextCursor::StartOfBlock);
            m_marginSelectStart = lineCursor.position();
            lineCursor.movePosition(QTextCursor::NextBlock,
                                    QTextCursor::KeepAnchor);
            m_editor->setTextCursor(lineCursor);
        } else if (m_editor->showFolding() &&
                   (!m_editor->showLineNumbers() ||
                    eventPos.x() >= width() - foldPixmapWidth)) {
            // Clicked in the folding margin
            QTextBlock block = lineCursor.block();
            if (block.isValid() && m_editor->m_highlighter->isFoldable(block)) {
                if (WingSyntaxHighlighter::isFolded(block))
                    m_editor->m_highlighter->unfoldBlock(block);
                else
                    m_editor->m_highlighter->foldBlock(block);

                m_editor->viewport()->update();
                update();
                m_editor->updateScrollBars();

                // Move the editing cursor if it was in a folded block
                QTextCursor cursor = m_editor->textCursor();
                if (!cursor.block().isVisible()) {
                    cursor.setPosition(block.position());
                    m_editor->setTextCursor(cursor);
                }
            }
        }
    }

    QWidget::mousePressEvent(e);
}

void WingLineMargin::leaveEvent(QEvent *e) {
    m_foldHoverLine = -1;
    update();
    QWidget::leaveEvent(e);
}
