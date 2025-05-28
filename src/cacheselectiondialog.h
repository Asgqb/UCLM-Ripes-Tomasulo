#pragma once

#include <QDialog>
#include <QTreeWidget>

namespace Ripes {

namespace Ui {
class CacheSelectionDialog;
}

enum class CacheConfigType {
  L1Split,
  L1Unified,
  Multilevel
};

class CacheSelectionDialog : public QDialog {
  Q_OBJECT

public:
  explicit CacheSelectionDialog(QWidget *parent = nullptr);
  ~CacheSelectionDialog();

  CacheConfigType getSelectedCacheType() const;

private slots:
  void selectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
  Ui::CacheSelectionDialog *m_ui;
  QTreeWidgetItem *m_selectedItem = nullptr;
  CacheConfigType m_selectedType = CacheConfigType::L1Split;

  bool isCacheItem(const QTreeWidgetItem *item) const;
};

} // namespace Ripes

