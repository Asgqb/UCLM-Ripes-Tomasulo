#pragma once

#include "VSRTL/core/vsrtl_component.h"
#include "VSRTL/core/vsrtl_register.h"

#include "processors/RISC-V/riscv.h"

#include "../rv5s/rv5s_memwb.h"

namespace vsrtl {
namespace core {
using namespace Ripes;

template <unsigned XLEN>
class RV5S_3S_MEMWB : public RV5S_MEMWB<XLEN> {
public:
  RV5S_3S_MEMWB(const std::string &name, SimComponent *parent) : RV5S_MEMWB<XLEN>(name, parent) {
    CONNECT_REGISTERED_INPUT(do_branch);
  }

  REGISTERED_INPUT(do_branch, 1);
};

}
}
