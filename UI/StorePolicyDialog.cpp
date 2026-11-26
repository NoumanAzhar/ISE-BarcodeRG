#include "StorePolicyDialog.h"

#include "Neu.h"
#include "Widgets.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QString>
#include <QVBoxLayout>

namespace {

// U+1F621 (angry face). Built from its code point so the source file stays plain ASCII.
QString angryFace()
{
    const char32_t face[] = { 0x1F621, 0 };
    return QString::fromUcs4(face);
}

} // namespace

StorePolicyDialog::StorePolicyDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Store Policy");
    setMinimumWidth(420);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);       // the card already holds its shadow room
    layout->setSpacing(6);

    NeuFrame* card = makeCard();
    card->setOutline(NeuOutline::Danger);             // red ring: this is a rule, not a typo
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(28, 24, 28, 22);
    cardLayout->setSpacing(8);

    auto* face = new QLabel(angryFace(), card);
    face->setAlignment(Qt::AlignCenter);
    face->setStyleSheet("font-size: 84px; background: transparent; "
                        "font-family: 'Segoe UI Emoji', 'Apple Color Emoji', 'Noto Color Emoji';");

    auto* heading = new QLabel("STORE POLICY", card);
    heading->setAlignment(Qt::AlignCenter);
    heading->setStyleSheet("font-size: 28px; font-weight: 800; letter-spacing: 4px; "
                           "background: transparent;");

    auto* message = new QLabel("You can't return All the Products " + angryFace(), card);
    message->setAlignment(Qt::AlignCenter);
    message->setWordWrap(true);
    message->setStyleSheet("font-size: 18px; font-weight: 600; background: transparent; "
                           "font-family: 'Segoe UI', 'Segoe UI Emoji', 'Apple Color Emoji', 'Noto Color Emoji';");

    auto* hint = makeLabel("An edited order must keep at least one item.", "muted", card);
    hint->setAlignment(Qt::AlignCenter);
    hint->setWordWrap(true);

    cardLayout->addWidget(face);
    cardLayout->addWidget(heading);
    cardLayout->addSpacing(4);
    cardLayout->addWidget(message);
    cardLayout->addWidget(hint);

    auto* okBtn = new NeuButton("Got it", NeuButton::Variant::Primary, this);
    okBtn->setDefault(true);
    connect(okBtn, &NeuButton::clicked, this, &QDialog::accept);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch(1);
    buttons->addWidget(okBtn);
    buttons->addStretch(1);

    layout->addWidget(card);
    layout->addLayout(buttons);
}

void StorePolicyDialog::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        accept();
        return;
    default:
        QDialog::keyPressEvent(event);
    }
}
