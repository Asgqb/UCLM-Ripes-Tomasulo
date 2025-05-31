#include "cachetabwidget.h"
#include "ui_cachetabwidget.h"

#include "memorytab.h"
#include "memoryviewerwidget.h"
#include "ripessettings.h"
#include "cachesim/cachetypes.h"
#include "processortab.h"

#include <QTabBar>
#include <QWheelEvent>
#include <QDebug>

namespace Ripes {

class ScrollEventFilter : public QObject {
public:
  ScrollEventFilter(QObject *parent) : QObject(parent) {}

  bool eventFilter(QObject *, QEvent *event) override {
    if (auto *wheelEvent = dynamic_cast<QWheelEvent *>(event)) {
      Q_UNUSED(wheelEvent);
      return true;
    }
    return false;
  }
};

void CacheTabWidget::flipTabs() {
  m_ui->tabWidget->setCurrentIndex(1);
  m_ui->tabWidget->setCurrentIndex(0);
}

CacheTabWidget::CacheTabWidget(QWidget *parent)
    : QWidget(parent), m_ui(new Ui::CacheTabWidget) {
  m_ui->setupUi(this);

  rebuildCacheTabs();

  connect(m_ui->tabWidget, &QTabWidget::currentChanged, this,
          &CacheTabWidget::handleTabIndexChanged);
  connect(m_ui->tabWidget, &QTabWidget::tabCloseRequested, this,
          &CacheTabWidget::handleTabCloseRequest);
  m_ui->tabWidget->setTabsClosable(true);
  m_ui->tabWidget->tabBar()->installEventFilter(new ScrollEventFilter(this));
}

CacheTabWidget::~CacheTabWidget() {
  delete m_ui;
}

void CacheTabWidget::rebuildCacheTabs() {
  qDebug() << "rebuildCacheTabs() called";

  // Limpiar pestañas previas
  while (m_ui->tabWidget->count() > 0) {
    QWidget* widget = m_ui->tabWidget->widget(0);
    m_ui->tabWidget->removeTab(0);
    delete widget;
  }

  int rawType = RipesSettings::value("CacheTypeSelected").toInt();
  CacheConfigType cacheType = static_cast<CacheConfigType>(rawType);
  qDebug() << "Cache type loaded from settings:" << static_cast<int>(cacheType);

  switch (cacheType) {
  case CacheConfigType::L1Unified: {
    m_unifiedShim = std::make_unique<UnifiedCacheShim>(this);
    m_unifiedCacheWidget = std::make_unique<CacheWidget>(this);
    m_unifiedShim->setNextLevelCache(m_unifiedCacheWidget->getCacheSim());

    m_ui->tabWidget->addTab(m_unifiedCacheWidget.get(), "L1 Unified");
    break;
  }
  case CacheConfigType::L1Split:
  default: {
    auto* dataCacheWidget = new CacheWidget(this);
    auto* instrCacheWidget = new CacheWidget(this);

    m_l1dShim = std::make_unique<L1CacheShim>(L1CacheShim::CacheType::DataCache, this);
    m_l1iShim = std::make_unique<L1CacheShim>(L1CacheShim::CacheType::InstrCache, this);

    m_l1dShim->setNextLevelCache(dataCacheWidget->getCacheSim());
    m_l1iShim->setNextLevelCache(instrCacheWidget->getCacheSim());

    m_ui->tabWidget->addTab(dataCacheWidget, "L1 Data");
    m_ui->tabWidget->addTab(instrCacheWidget, "L1 Instr");

    if (auto *tabButton = m_ui->tabWidget->tabBar()->tabButton(0, QTabBar::RightSide)) {
      m_defaultTabButtonSize = tabButton->size();
      tabButton->resize(0, 0);
    }
    if (auto *tabButton2 = m_ui->tabWidget->tabBar()->tabButton(1, QTabBar::RightSide)) {
      tabButton2->resize(0, 0);
    }
    break;
  }
  }
}

void CacheTabWidget::handleTabIndexChanged(int index) {
  auto *widget = dynamic_cast<CacheWidget *>(m_ui->tabWidget->widget(index));
  if (widget) {
    emit cacheFocusChanged(widget);
  }
}

void CacheTabWidget::handleTabCloseRequest(int index) {
#ifdef N_CACHES_ENABLED
  Q_ASSERT(index == m_addTabIdx - 1 && index > InstrCache);
  const int newIndex = index - 1;
  m_ui->tabWidget->setCurrentIndex(newIndex);
  m_ui->tabWidget->removeTab(index);
  m_addTabIdx = m_ui->tabWidget->count() - 1;
  if (newIndex > InstrCache) {
    m_ui->tabWidget->tabBar()
        ->tabButton(newIndex, QTabBar::RightSide)
        ->resize(m_defaultTabButtonSize);
  }
  m_nextCacheLevel--;
  m_ui->tabWidget->setTabText(m_addTabIdx, QString());
#else
  Q_UNUSED(index);
#endif
}

ProcessorTab* CacheTabWidget::findProcessorTab() {
  QWidget* widget = parentWidget();
  while (widget) {
    if (auto* processorTab = qobject_cast<ProcessorTab*>(widget))
      return processorTab;
    widget = widget->parentWidget();
  }
  return nullptr;
}

} // namespace Ripes
