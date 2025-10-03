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

#ifndef WINGSIGNATURETOOLTIP_H
#define WINGSIGNATURETOOLTIP_H

#include <QLabel>
#include <QTextBrowser>
#include <QWidget>

class WingSignatureTooltip : public QWidget {
    Q_OBJECT
public:
    struct Signature {
        QString label; // e.g. "int foo(int a, const QString &b)"
        QString doc;   // documentation
    };

    enum class SignatureHelpType { PlainText, MarkDown, Html };

public:
    explicit WingSignatureTooltip(QWidget *parent = nullptr);
    ~WingSignatureTooltip() override = default;

    bool isDocHelpVisible() const;

public slots:
    void nextSignature();
    void prevSignature();

    void showSignatures(const QPoint &globalPos,
                        const QList<WingSignatureTooltip::Signature> &sigs,
                        qsizetype index = 0);
    void hideTooltip();

    void setHelpDocVisible(bool visible);

signals:
    void signatureChanged(qsizetype newIndex);

protected:
    void wheelEvent(QWheelEvent *ev) override;

private:
    void updateDisplay();

    QLabel *m_labelSignature;
    QTextBrowser *m_textDoc;
    QLabel *m_countLabel;

    QList<Signature> m_signatures;
    int m_currentIndex;
    SignatureHelpType m_sigHelpType;
};

#endif // WINGSIGNATURETOOLTIP_H
