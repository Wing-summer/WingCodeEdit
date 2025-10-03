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

#include "wingsignaturetooltip.h"

#include <QApplication>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QScreen>
#include <QUrl>

WingSignatureTooltip::WingSignatureTooltip(QWidget *parent)
    : QWidget(parent, Qt::ToolTip), m_labelSignature(new QLabel(this)),
      m_textDoc(new QTextBrowser(this)), m_countLabel(new QLabel(this)),
      m_currentIndex(0), m_sigHelpType(SignatureHelpType::MarkDown) {
    setAttribute(Qt::WA_ShowWithoutActivating);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint |
                   Qt::WindowStaysOnTopHint);
    setFocusPolicy(Qt::NoFocus);
    setContentsMargins(6, 6, 6, 6);

    m_labelSignature->setTextFormat(Qt::RichText);
    m_labelSignature->setWordWrap(true);

    m_textDoc->setReadOnly(true);
    m_textDoc->setOpenExternalLinks(true);
    m_textDoc->setFocusPolicy(Qt::NoFocus);
    m_textDoc->setMaximumHeight(100);
    m_textDoc->setUndoRedoEnabled(false);
    m_textDoc->setWordWrapMode(QTextOption::WordWrap);

    QHBoxLayout *topRow = new QHBoxLayout;
    topRow->addWidget(m_labelSignature);
    topRow->addStretch();
    topRow->addWidget(m_countLabel);

    QVBoxLayout *main = new QVBoxLayout(this);
    main->setContentsMargins(6, 6, 6, 6);
    main->addLayout(topRow);
    main->addWidget(m_textDoc);

    setLayout(main);

    setStyleSheet(
        QStringLiteral("QTextBrowser{background:transparent;border:none;}"));
}

void WingSignatureTooltip::showSignatures(const QPoint &globalPos,
                                      const QList<Signature> &sigs,
                                      qsizetype index) {
    if (sigs.isEmpty()) {
        hideTooltip();
        return;
    }
    m_signatures = sigs;
    m_currentIndex = qBound(0, index, m_signatures.count() - 1);
    updateDisplay();

    QRect screenRect = QApplication::screenAt(globalPos)->availableGeometry();
    adjustSize();
    QPoint pos = globalPos;
    pos.setY(globalPos.y() + 18);
    if (pos.y() + height() > screenRect.bottom()) {
        pos.setY(globalPos.y() - height() - 4);
    }
    if (pos.x() + width() > screenRect.right()) {
        pos.setX(screenRect.right() - width() - 4);
    }
    move(pos);
    show();
}

void WingSignatureTooltip::hideTooltip() {
    hide();
    m_labelSignature->clear();
    m_textDoc->clear();
}

bool WingSignatureTooltip::isDocHelpVisible() const { return m_textDoc; }

void WingSignatureTooltip::nextSignature() {
    if (m_signatures.isEmpty())
        return;
    m_currentIndex = (m_currentIndex + 1) % m_signatures.count();
    updateDisplay();
    emit signatureChanged(m_currentIndex);
}

void WingSignatureTooltip::prevSignature() {
    if (m_signatures.isEmpty())
        return;
    m_currentIndex =
        (m_currentIndex - 1 + m_signatures.count()) % m_signatures.count();
    updateDisplay();
    emit signatureChanged(m_currentIndex);
}

void WingSignatureTooltip::setHelpDocVisible(bool visible) {
    m_textDoc->setVisible(visible);
}

void WingSignatureTooltip::wheelEvent(QWheelEvent *ev) {
    if (ev->angleDelta().y() > 0)
        prevSignature();
    else
        nextSignature();
    ev->accept();
}

void WingSignatureTooltip::updateDisplay() {
    if (m_signatures.isEmpty())
        return;
    const Signature &s = m_signatures.at(m_currentIndex);

    m_labelSignature->setText(s.label);

    if (s.doc.isEmpty()) {
        m_textDoc->setVisible(false);
    } else {
        m_textDoc->clear();
        switch (m_sigHelpType) {
        case SignatureHelpType::PlainText:
            m_textDoc->setPlainText(s.doc);
            break;
        case SignatureHelpType::MarkDown:
            m_textDoc->setMarkdown(s.doc);
            break;
        case SignatureHelpType::Html:
            m_textDoc->setHtml(s.doc);
            break;
        }
        m_textDoc->setVisible(true);
    }

    m_countLabel->setText(QStringLiteral("▴ %1 / %2 ▾")
                              .arg(m_currentIndex + 1)
                              .arg(m_signatures.count()));
    m_countLabel->show();

    adjustSize();
}
