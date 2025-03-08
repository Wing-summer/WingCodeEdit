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

#ifndef WINGLINEMARGIN_H
#define WINGLINEMARGIN_H

#include <QWidget>

class WingCodeEdit;

class WingLineMargin : public QWidget {
    Q_OBJECT
public:
    explicit WingLineMargin(WingCodeEdit *editor);

    QSize sizeHint() const override;

signals:
    void symbolMarkLineMarginClicked(int line);

protected:
    void paintEvent(QPaintEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void leaveEvent(QEvent *e) override;

private:
    WingCodeEdit *m_editor;
    int m_marginSelectStart;
    int m_foldHoverLine;
};

#endif // WINGLINEMARGIN_H
