#pragma once

#include <QAction>
#include <QSpinBox>
#include <QTimer>
#include <QToolBar>
#include <QWidget>

#include "isa/isa_types.h"
#include "processors/interface/ripesprocessor.h"
#include "ripestab.h"

class QGroupBox;

namespace vsrtl {
class VSRTLWidget;
class Label;
} // namespace vsrtl
class QResizeEvent;

namespace Ripes {

namespace Ui {
class ProcessorTab;
}

class InstructionModel;
class RegisterModel;
class PipelineDiagramModel;
class TomasuloWidget;
struct Layout;

class ProcessorTab : public RipesTab {
  friend class RunDialog;
  friend class MainWindow;
  Q_OBJECT

public:
  ProcessorTab(QToolBar *controlToolbar, QToolBar *additionalToolbar,
               QWidget *parent = nullptr);
  ~ProcessorTab() override;

  void initRegWidget();

signals:
  void cacheConfigurationChanged();

public slots:
  void pause();
  void restart();
  void reset();
  void reverse();
  void processorFinished();
  void runFinished();
  void updateStatistics();
  void updateInstructionLabels();
  void fitToScreen();

  void processorSelection();
  void cacheSelection();

private slots:
  void run(bool state);
  void autoClock(bool state);
  void autoClockTimeout();
  void setInstructionViewCenterRow(int row);
  void showPipelineDiagram();

private:
  void setupSimulatorActions(QToolBar *controlToolbar);
  void enableSimulatorControls();
  void updateInstructionModel();
  void updateRegisterModel();
  void loadLayout(const Layout &);
  void loadProcessorToWidget(const Layout *);

  void syncTomasuloWithEditor();
  void showTomasuloView();
  void showVSRTLView();

  void setupTomasuloOptionsWidget();
  void resetTomasuloOptionsToDefaults();
  void reloadTomasuloProgram();
  void clockTomasulo();
  void updateTomasuloExecutionInfo();

  Ui::ProcessorTab *m_ui = nullptr;
  InstructionModel *m_instrModel = nullptr;
  PipelineDiagramModel *m_stageModel = nullptr;
  TomasuloWidget *m_tomasuloWidget = nullptr;

  vsrtl::VSRTLWidget *m_vsrtlWidget = nullptr;

  std::map<StageIndex, vsrtl::Label *> m_stageInstructionLabels;

  QTimer *m_statUpdateTimer = nullptr;

  // Tomasulo options panel
  QGroupBox *m_tomasuloOptionsGroup = nullptr;

  // Buffers
  QSpinBox *m_tomasuloEffAddrBuffers = nullptr;
  QSpinBox *m_tomasuloFpAddBuffers = nullptr;
  QSpinBox *m_tomasuloFpMulBuffers = nullptr;
  QSpinBox *m_tomasuloIntBuffers = nullptr;
  QSpinBox *m_tomasuloReorderBuffers = nullptr;

  // Latencies
  QSpinBox *m_tomasuloFpAddLatency = nullptr;
  QSpinBox *m_tomasuloFpSubLatency = nullptr;
  QSpinBox *m_tomasuloFpMulLatency = nullptr;
  QSpinBox *m_tomasuloFpDivLatency = nullptr;

  // Actions
  QAction *m_selectProcessorAction = nullptr;
  QAction *m_selectCacheAction = nullptr;

  QAction *m_clockAction = nullptr;
  QAction *m_autoClockAction = nullptr;
  QAction *m_runAction = nullptr;
  QAction *m_displayValuesAction = nullptr;
  QAction *m_pipelineDiagramAction = nullptr;
  QAction *m_reverseAction = nullptr;
  QAction *m_resetAction = nullptr;
  QAction *m_darkmodeAction = nullptr;
  QTimer *m_autoClockTimer = nullptr;

  QSpinBox *m_autoClockInterval = nullptr;

  bool m_usingTomasulo = true;

protected:
private:

protected:
private:

protected:
private:

protected:
private:

protected:
  void resizeEvent(QResizeEvent *event) override;

private:
  void updateTomasuloProcessorResponsiveLayout();
};

} // namespace Ripes












