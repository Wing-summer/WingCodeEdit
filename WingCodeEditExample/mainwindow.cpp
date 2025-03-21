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

#include "mainwindow.h"

#include "wingcodeedit.h"
#include "wingcompleter.h"
#include "wingsymbolcenter.h"

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Theme>

#include <QStandardItemModel>
#include <QToolBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    auto toolbar = new QToolBar(this);

    auto ce = new WingCodeEdit(this);
    ce->setAutoIndent(true);
    ce->setMatchBraces(true);
    ce->setShowLongLineEdge(true);
    ce->setShowIndentGuides(true);
    ce->setShowLineNumbers(true);
    ce->setShowFolding(true);
    ce->setShowWhitespace(true);
    ce->setShowSymbolMark(true);
    ce->setAutoCloseChar(true);

    {
        auto a = new QAction("SymbolMarking", this);
        a->setCheckable(true);
        a->setChecked(true);
        connect(a, &QAction::toggled, ce, &WingCodeEdit::setShowSymbolMark);
        toolbar->addAction(a);
    }
    {
        auto a = new QAction("LineNumbers", this);
        a->setCheckable(true);
        a->setChecked(true);
        connect(a, &QAction::toggled, ce, &WingCodeEdit::setShowLineNumbers);
        toolbar->addAction(a);
    }
    {
        auto a = new QAction("Folding", this);
        a->setCheckable(true);
        a->setChecked(true);
        connect(a, &QAction::toggled, ce, &WingCodeEdit::setShowFolding);
        toolbar->addAction(a);
    }
    addToolBar(toolbar);

    ce->setPlainText(R"(#include <iostream>
using namespace std;

int main(){
    std::cout << "hello world!" << std::endl;
    return 0;
}
)");

    WingSymbolCenter::instance().regsiterSymbol(
        "bp", QPixmap(":/WingCodeEditExample/bp.png"));

    ce->setSyntax(ce->syntaxRepo().definitionForName("cpp"));
    ce->setTheme(ce->syntaxRepo().defaultTheme(
        KSyntaxHighlighting::Repository::DarkTheme));

    auto cp = new WingCompleter(ce);
    auto mo = new QStandardItemModel(this);

    auto icon = QIcon(":/WingCodeEditExample/bp.png");

    mo->appendRow(new QStandardItem(icon, "string"));
    mo->appendRow(new QStandardItem(icon, "array"));
    mo->appendRow(new QStandardItem(icon, "arrow"));
    mo->appendRow(new QStandardItem(icon, "absnum"));
    mo->appendRow(new QStandardItem(icon, "buffer"));
    mo->appendRow(new QStandardItem(icon, "printf"));
    cp->setModel(mo);

    connect(ce, &WingCodeEdit::symbolMarkLineMarginClicked, this,
            [ce](int line) {
                auto id = ce->symbolMark(line);
                if (id.isEmpty()) {
                    ce->addSymbolMark(line, "bp");
                } else {
                    ce->removeSymbolMark(line);
                }
            });

    setCentralWidget(ce);

    resize(800, 600);
}

MainWindow::~MainWindow() {}
