#pragma once

#include "PageBase.h"

#include "season.h"

#include <vector>

namespace pos { class AppServices; }

class Banner;
class DataTable;
class NeuButton;
class NeuLineEdit;
class QDateEdit;
class QLabel;
class QWidget;

// Manager: seasonal discounts (Seasons.txt). A season gives a percentage off
// every bill made between its start and end date while it is switched on.
class SeasonsPage : public PageBase {
    Q_OBJECT
public:
    explicit SeasonsPage(pos::AppServices& services, QWidget* parent = nullptr);

    void onActivated() override;
    void resetState() override;

private:
    QWidget* buildSide();

    void reload();
    void showSelected();
    void addSeason();
    void toggleSelected();
    int selectedIndex() const;          // position in Seasons.txt, -1 when none
    void clearForm();                   // dates back to today, discount empty

    pos::AppServices& m_services;
    std::vector<pos::Season> m_seasons;

    DataTable* m_table = nullptr;
    QDateEdit* m_from = nullptr;
    QDateEdit* m_to = nullptr;
    NeuLineEdit* m_percent = nullptr;
    Banner* m_banner = nullptr;
    QWidget* m_selectedBox = nullptr;
    QLabel* m_selectedText = nullptr;
    NeuButton* m_toggleBtn = nullptr;
};
