#include "unifiedcacheshim.h"
#include "processorhandler.h"

namespace Ripes {

UnifiedCacheShim::UnifiedCacheShim(QObject *parent)
    : CacheInterface(parent) {
  connect(ProcessorHandler::get(), &ProcessorHandler::processorReset, this,
          &UnifiedCacheShim::processorReset);
  connect(ProcessorHandler::get(), &ProcessorHandler::processorClocked, this,
          &UnifiedCacheShim::processorWasClocked, Qt::DirectConnection);
  connect(ProcessorHandler::get(), &ProcessorHandler::processorReversed, this,
          &UnifiedCacheShim::processorReversed);

  processorReset();
}

void UnifiedCacheShim::access(AInt, MemoryAccess::Type) {
  Q_ASSERT(false);  // No debe llamarse directamente
}


void UnifiedCacheShim::processorReset() {
  CacheInterface::reset();

  if (m_nextLevelCache) {
    processorWasClocked();  // Igual que en L1CacheShim
  }
}

void UnifiedCacheShim::processorReversed() {
  CacheInterface::reverse();
}

void UnifiedCacheShim::processorWasClocked() {
  if (!m_nextLevelCache)
    return;

  const auto dataAccess = ProcessorHandler::getProcessor()->dataMemAccess();
  const auto instrAccess = ProcessorHandler::getProcessor()->instrMemAccess();

  if (dataAccess.type == MemoryAccess::Write) {
    m_nextLevelCache->access(dataAccess.address, MemoryAccess::Write);
  } else if (dataAccess.type == MemoryAccess::Read) {
    m_nextLevelCache->access(dataAccess.address, MemoryAccess::Read);
  }

  if (instrAccess.type == MemoryAccess::Read) {
    m_nextLevelCache->access(instrAccess.address, MemoryAccess::Read);
  }
}


} // namespace Ripes
