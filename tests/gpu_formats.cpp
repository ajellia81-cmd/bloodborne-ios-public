// SPDX-License-Identifier: GPL-2.0-or-later
#include <array>
#include <cassert>
#include "video_core/amdgpu/regs_color.h"

// ISA table: all 64 encodings, including existing aliases, not just crash value 15.
constexpr std::array<unsigned,64> expected = {
 0,1,2,3,4,5,6,6,9,9,10,11,12,13,14,0,
 16,17,17,19,20,21,22,0,0,0,0,0,0,0,0,0,
 32,33,34,35,36,37,38,39,40,41,0,0,44,45,46,47,
 48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63
};
constexpr bool remaps_match_isa() {
 for (unsigned i=0;i<64;++i)
  if (static_cast<unsigned>(AmdGpu::RemapDataFormat(static_cast<AmdGpu::DataFormat>(i))) != expected[i])
   return false;
 return true;
}
static_assert(remaps_match_isa());
int main() {
 AmdGpu::ColorBuffer color{};
 color.base_address=0x1000;
 // CB_COLOR_INFO has a 5-bit format field; sampled descriptors use 6 bits.
 for (unsigned i=0;i<32;++i) {
  color.info.format=i;
  assert(static_cast<bool>(color) == (expected[i] != 0));
 }
 color.info.format=10;
 color.base_address=0;
 assert(!color);
}
