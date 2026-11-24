#pragma once

#include "PageBase.h"

namespace pos { class AppServices; }

class Banner;
class DataTable;
class NeuButton;
class NeuLineEdit;
class QLabel;

// Manager: product categories. The ID is the first two digits of every barcode
// in the category, so it is created automatically and never changes.
class CategoriesPage : public PageBase {
    Q_OBJECT
public:
    explicit CategoriesPage(pos::AppServices& services, QWidget* parent = nullptr);

    void onActivated() override;       // reload (products may have been added)
    void resetState() override;

private:
    QWidget* buildForm();

    void reload();
    void enterNewMode();
    void enterEditMode(const QString& id);
    void startNew();
    void save();
    void clearFieldStates();

    pos::AppServices& m_services;
    bool m_editing = false;

    DataTable* m_table = nullptr;
    QLabel* m_formTitle = nullptr;
    NeuLineEdit* m_id = nullptr;
    NeuLineEdit* m_name = nullptr;
    NeuLineEdit* m_acronym = nullptr;
    NeuButton* m_newBtn = nullptr;
    NeuButton* m_saveBtn = nullptr;
    Banner* m_banner = nullptr;
};
