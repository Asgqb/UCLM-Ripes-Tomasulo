#include "cacheselectiondialog.h"
#include "ui_cacheselectiondialog.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QTreeWidgetItem>

namespace Ripes {

CacheSelectionDialog::CacheSelectionDialog(QWidget *parent)
    : QDialog(parent), m_ui(new Ui::CacheSelectionDialog) {
  m_ui->setupUi(this);
  setWindowTitle("Select Cache Configuration");

  m_ui->cacheTree->setHeaderHidden(true);

  struct Option {
    QString name;
    CacheConfigType type;
  };

  QList<Option> options = {
      {"L1 Split Cache", CacheConfigType::L1Split},
      {"L1 Unified Cache", CacheConfigType::L1Unified},
      {"Multilevel Cache", CacheConfigType::Multilevel}};

  for (const auto &opt : options) {
    auto *item = new QTreeWidgetItem({opt.name});
    item->setData(0, Qt::UserRole, static_cast<int>(opt.type));
    m_ui->cacheTree->addTopLevelItem(item);
  }

  connect(m_ui->cacheTree, &QTreeWidget::currentItemChanged,
          this, &CacheSelectionDialog::selectionChanged);

  connect(m_ui->cacheTree, &QTreeWidget::itemDoubleClicked,
          this, [=](const QTreeWidgetItem *item, int) {
            if (isCacheItem(item)) {
              accept();
            }
          });

  connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  m_ui->cacheTree->setCurrentItem(m_ui->cacheTree->topLevelItem(0));
}

CacheSelectionDialog::~CacheSelectionDialog() {
  delete m_ui;
}

CacheConfigType CacheSelectionDialog::getSelectedCacheType() const {
  return m_selectedType;
}

bool CacheSelectionDialog::isCacheItem(const QTreeWidgetItem *item) const {
  return item && item->data(0, Qt::UserRole).isValid();
}

void CacheSelectionDialog::selectionChanged(QTreeWidgetItem *current,
                                            QTreeWidgetItem *) {
  if (!isCacheItem(current)) {
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    return;
  }

  m_selectedItem = current;
  m_selectedType = static_cast<CacheConfigType>(
      current->data(0, Qt::UserRole).toInt());
  m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

} // namespace Ripes