#include "SeasonsPage.h"

#include "AppServices.h"
#include "DataTable.h"
#include "Format.h"
#include "Neu.h"
#include "Widgets.h"

#include <QDate>
#include <QDateEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

namespace {

enum class State { Running, Upcoming, Ended, Off };

QString today()
{
    return QDate::currentDate().toString(Qt::ISODate);        // yyyy-MM-dd
}

State stateOf(const pos::Season& s)
{
    if (!s.active)
        return State::Off;
    const QString now = today();
    const QString start = qs(s.startDate);
    const QString end = qs(s.endDate);
    if (now < start)
        return State::Upcoming;
    if (now > end)
        return State::Ended;
    return State::Running;
}

QString percentText(double percent)
{
    return QLocale::c().toString(percent, 'f', percent == int(percent) ? 0 : 2) + "%";
}

QString dateText(const std::string& iso)
{
    const QDate date = QDate::fromString(qs(iso), Qt::ISODate);
    return date.isValid() ? date.toString("d MMM yyyy") : qs(iso);
}

} // namespace

SeasonsPage::SeasonsPage(pos::AppServices& services, QWidget* parent)
    : PageBase(parent), m_services(services)
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(2);                         // cards already hold their shadow room

    m_table = new DataTable(this);
    m_table->setColumns({
        { "From", DataTable::Align::Left, false },
        { "To", DataTable::Align::Left, false },
        { "Discount", DataTable::Align::Right, false },
        { "Status", DataTable::Align::Left, true },
    });
    m_table->setEmptyText("No discount seasons yet.\nAdd one on the right.");

    root->addWidget(m_table, 3);
    root->addWidget(buildSide(), 2);

    connect(m_table, &DataTable::selectionChanged, this, &SeasonsPage::showSelected);
    showSelected();
}

// ---------------------------------------------------------------- building

QWidget* SeasonsPage::buildSide()
{
    NeuFrame* card = makeCard();
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(6);

    // Drop-down calendars: pick the dates instead of typing them.
    m_from = new QDateEdit(QDate::currentDate(), card);
    m_to = new QDateEdit(QDate::currentDate(), card);
    for (QDateEdit* edit : { m_from, m_to }) {
        edit->setCalendarPopup(true);
        edit->setDisplayFormat("d MMM yyyy");
        edit->setMinimumDate(QDate::currentDate());
        edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    // the end can never be before the start
    connect(m_from, &QDateEdit::dateChanged, this, [this](const QDate& start) {
        m_to->setMinimumDate(start);
    });

    m_percent = new NeuLineEdit(card);
    m_percent->setPlaceholderText(QStringLiteral("up to %1")
                                      .arg(QLocale::c().toString(pos::kMaxSeasonPercent, 'f', 0)));
    m_percent->setValidator(new QRegularExpressionValidator(
        QRegularExpression("[0-9]{0,2}([.][0-9]{0,2})?"), m_percent));
    connect(m_percent, &QLineEdit::textChanged, this, [this]() { m_percent->setState(QString()); });
    connect(m_percent, &QLineEdit::returnPressed, this, &SeasonsPage::addSeason);

    auto* dates = new QHBoxLayout;
    dates->setSpacing(2);
    dates->addWidget(makeField("From", m_from, card), 1);
    dates->addWidget(makeField("To", m_to, card), 1);

    auto* addBtn = new NeuButton("Add season", NeuButton::Variant::Primary, card);
    addBtn->setDefault(true);
    connect(addBtn, &NeuButton::clicked, this, &SeasonsPage::addSeason);

    // ---- the selected season: switch it on or off
    m_selectedBox = new QWidget(card);
    auto* selected = new QVBoxLayout(m_selectedBox);
    selected->setContentsMargins(0, 0, 0, 0);
    selected->setSpacing(4);
    m_selectedText = makeLabel(QString(), "fieldLabel", m_selectedBox);
    m_selectedText->setWordWrap(true);
    m_toggleBtn = new NeuButton("Turn off", NeuButton::Variant::Normal, m_selectedBox);
    connect(m_toggleBtn, &NeuButton::clicked, this, &SeasonsPage::toggleSelected);
    selected->addWidget(makeLabel("SELECTED SEASON", "sectionTitle", m_selectedBox));
    selected->addWidget(m_selectedText);
    selected->addWidget(m_toggleBtn, 0, Qt::AlignLeft);

    m_banner = new Banner(card);

    layout->addWidget(makeLabel("NEW SEASON", "sectionTitle", card));
    layout->addLayout(dates);
    layout->addWidget(makeField(QStringLiteral("Discount (%, max %1)")
                                    .arg(QLocale::c().toString(pos::kMaxSeasonPercent, 'f', 0)),
                                m_percent, card));
    layout->addStretch(1);
    layout->addWidget(m_selectedBox);
    layout->addWidget(m_banner);
    layout->addWidget(addBtn, 0, Qt::AlignRight);
    return card;
}

// ------------------------------------------------------------- page hooks

void SeasonsPage::onActivated()
{
    reload();
}

void SeasonsPage::clearForm()
{
    m_from->setDate(QDate::currentDate());
    m_to->setDate(m_from->date());
    m_percent->clear();
    m_percent->setState(QString());
}

void SeasonsPage::resetState()
{
    clearForm();
    m_banner->clearMessage();
    m_table->clearSelection();
}

// -------------------------------------------------------------------- list

void SeasonsPage::reload()
{
    const QString keep = m_table->selectedKey();
    m_seasons = m_services.seasons().all();

    m_table->clearRows();
    for (int i = 0; i < int(m_seasons.size()); ++i) {
        const pos::Season& s = m_seasons[i];
        QString status;
        DataTable::Tone tone = DataTable::Tone::Muted;
        switch (stateOf(s)) {
        case State::Running:  status = "Running now"; tone = DataTable::Tone::Good; break;
        case State::Upcoming: status = "Starts soon"; tone = DataTable::Tone::Warn; break;
        case State::Ended:    status = "Ended"; break;
        case State::Off:      status = "Switched off"; break;
        }
        const int row = m_table->addRow({ dateText(s.startDate), dateText(s.endDate),
                                          percentText(s.percent), status },
                                        QString::number(i));
        m_table->setTone(row, 3, tone);
    }
    m_table->selectKey(keep);                    // refreshes the selected-season box
}

int SeasonsPage::selectedIndex() const
{
    bool ok = false;
    const int index = m_table->selectedKey().toInt(&ok);
    return (ok && index >= 0 && index < int(m_seasons.size())) ? index : -1;
}

void SeasonsPage::showSelected()
{
    const int index = selectedIndex();
    m_selectedBox->setVisible(index >= 0);
    if (index < 0)
        return;

    const pos::Season& s = m_seasons[index];
    m_selectedText->setText(QStringLiteral("%1 %2 %3, %4 off")
                                .arg(dateText(s.startDate), QString(kArrowRight), dateText(s.endDate),
                                     percentText(s.percent)));
    m_toggleBtn->setText(s.active ? "Turn off" : "Turn on");
}

// ----------------------------------------------------------------- actions

void SeasonsPage::addSeason()
{
    m_percent->setState(QString());

    bool ok = false;
    const double percent = QLocale::c().toDouble(m_percent->text().trimmed(), &ok);

    pos::Season season;
    season.startDate = ss(m_from->date().toString(Qt::ISODate));
    season.endDate = ss(m_to->date().toString(Qt::ISODate));
    season.percent = ok ? percent : 0.0;
    season.active = true;

    const pos::SeasonResult result = m_services.seasons().add(season);
    if (!result.ok()) {
        if (result.error == pos::SeasonError::BadPercent)
            m_percent->setState("invalid");
        m_banner->showMessage(qs(result.message), Banner::Error, 4500);
        return;
    }

    clearForm();
    reload();
    m_banner->showMessage(QStringLiteral("Season added: %1 off from %2 to %3.")
                              .arg(percentText(season.percent), dateText(season.startDate),
                                   dateText(season.endDate)),
                          Banner::Success, 4000);
}

void SeasonsPage::toggleSelected()
{
    const int index = selectedIndex();
    if (index < 0)
        return;

    const bool turnOn = !m_seasons[index].active;
    if (!m_services.seasons().setActive(index, turnOn)) {
        m_banner->showMessage("The change could not be saved.", Banner::Error, 3500);
        return;
    }
    reload();
    m_banner->showMessage(turnOn ? "Season turned on." : "Season turned off.", Banner::Success);
}
