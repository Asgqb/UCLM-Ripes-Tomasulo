#pragma once
#include <QWidget>

#include "cachesim/l1cacheshim.h"
#include "cachesim/unifiedcacheshim.h"
#include "processortab.h"

#include <QPointer>

// #define N_CACHES_ENABLED

namespace Ripes {
class CacheWidget;
class ProcessorTab;

namespace Ui {
class CacheTabWidget;
}

class CacheTabWidget : public QWidget {
  Q_OBJECT

public:
  explicit CacheTabWidget(QWidget *parent = nullptr);
  ~CacheTabWidget();

  /**
   * @brief flipTabs
   * switches the active tabs from 0->1->0. This is a "hack" to resize the cache
   * view to the screen size, when initially starting the application.
   */
  void flipTabs();
  void rebuildCacheTabs();

signals:
  void focusAddressChanged(unsigned address);
  void cacheFocusChanged(Ripes::CacheWidget *cacheInFocus);

private:
  enum FixedCacheIdx { DataCache, InstrCache };
  void connectCacheWidget(CacheWidget *w);
  void handleTabIndexChanged(int index);
  void handleTabCloseRequest(int index);

  Ui::CacheTabWidget *m_ui;

  int m_addTabIdx = -1;
  int m_nextCacheLevel = 2;
  QSize m_defaultTabButtonSize;

  QPointer<L1CacheShim> m_l1dShim;
  QPointer<L1CacheShim> m_l1iShim;

  QPointer<UnifiedCacheShim> m_unifiedShim;
  QPointer<CacheWidget> m_unifiedCacheWidget;

  ProcessorTab* findProcessorTab();
};

} // namespace Ripes
