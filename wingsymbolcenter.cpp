#include "wingsymbolcenter.h"

WingSymbolCenter &WingSymbolCenter::instance() {
    static WingSymbolCenter ins;
    return ins;
}

bool WingSymbolCenter::existSymbol(const QString &id) {
    return _caches.contains(id);
}

QPixmap WingSymbolCenter::symbolFromName(const QString &id) {
    return _caches.value(id);
}

void WingSymbolCenter::regsiterSymbol(const QString &id,
                                      const QPixmap &symbol) {
    if (symbol.isNull()) {
        return;
    }
    _caches.insert(id, symbol);
}

WingSymbolCenter::WingSymbolCenter() {}
