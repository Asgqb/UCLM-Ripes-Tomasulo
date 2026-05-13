#include "tomasulo.h"

#include <QLabel>
#include <QVBoxLayout>

namespace Ripes {

TomasuloWidget::TomasuloWidget(QWidget* parent)
    : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);

    auto* label = new QLabel("Vista Tomasulo", this);
    layout->addWidget(label);

}

} // namespace Ripes