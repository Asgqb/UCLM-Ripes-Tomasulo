#pragma once

#include "cachesim/cachesim.h"


namespace Ripes {

/**
 * @brief The UnifiedCacheShim class
 * A shim that bridges between the processor and a unified L1 cache
 * (handling both instruction and data memory accesses).
 */
class UnifiedCacheShim : public CacheInterface {
  Q_OBJECT

public:
  explicit UnifiedCacheShim(QObject *parent = nullptr);

  void access(AInt address, MemoryAccess::Type type) override;


private:
  void processorWasClocked();
  void processorReset();
  void processorReversed();
  std::shared_ptr<CacheSim> m_cacheSim;
};

} // namespace Ripes
