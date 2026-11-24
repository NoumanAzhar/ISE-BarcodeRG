#pragma once

#include "PageBase.h"

namespace pos { class AppServices; }
class NeuSegmented;
class QStackedWidget;

// "Read" and "Generate" barcode tools of the manager under one rail entry, so the
// rail stays short. The two pages themselves are unchanged.
class BarcodesPage : public PageBase {
    Q_OBJECT
public:
    BarcodesPage(pos::AppServices& services, QWidget* parent = nullptr);

    void onActivated() override;
    void onDeactivated() override;

private:
    PageBase* current() const;

    NeuSegmented* m_switch = nullptr;
    QStackedWidget* m_stack = nullptr;       // 0 = read, 1 = generate
    bool m_active = false;
};
