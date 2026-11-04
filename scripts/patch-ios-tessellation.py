#!/usr/bin/env python3
"""Backport the live hull/domain patch layout fix from upstream 72148df; no AOT ABI changes."""
from pathlib import Path
import shutil,re
root=Path('upstream-runtime/gpu/shadps4')
p=root/'shader_recompiler/runtime_info.h';s=p.read_text()
needle='    u32 hs_output_base;';assert s.count(needle)==1
s=s.replace(needle,needle+'\n    u32 num_patch_constants; // complete hull/domain patch layout')
needle='struct SwTessEvalRuntimeInfo {\n    u32 hs_output_cp_stride;';assert s.count(needle)==1
s=s.replace(needle,needle+'\n    u32 num_patch_constants;')
needle='            sw.tcs.hs_output_base = tess_constants.hs_output_base;';assert s.count(needle)==1
s=s.replace(needle,needle+'\n            sw.tcs.num_patch_constants = tess_constants.patch_const_size / 16;')
needle='            sw.tes.hs_output_cp_stride = tess_constants.hs_cp_stride;';assert s.count(needle)==1
s=s.replace(needle,needle+'\n            sw.tes.num_patch_constants = tess_constants.patch_const_size / 16;');p.write_text(s)
p=root/'shader_recompiler/backend/spirv/spirv_emit_context.cpp';s=p.read_text()
needle='''        const u32 patch_base_location = num_attrs;
        for (size_t index = 0; index < 30; ++index) {
            if (!(info.uses_patches & (1U << index))) {'''
assert s.count(needle)==2
for stage in ('tes','tcs'):
    replace=f'''        const u32 patch_base_location = num_attrs;
        const u32 count = runtime_info.sw.{stage}.num_patch_constants;
        const u32 all_patches = info.uses_patches && count <= 30 ? (1U << count) - 1 : 0;
        for (size_t index = 0; index < 30; ++index) {{
            if (!((info.uses_patches | all_patches) & (1U << index))) {{'''
    s=s.replace(needle,replace,1)
p.write_text(s)
p=root/'video_core/renderer_metal/mtl_shader.mm';s=p.read_text();s='#include "bb_ios_tess_inputs.h"\n'+s
needle='''        PerVertexRewrite rewritten;
        if (variant.kind == VariantKind::GeometryKernel) {
            module = geometry.spirv;
        } else if (scan.per_vertex) {'''
assert s.count(needle)==1
s=s.replace(needle,'''        PerVertexRewrite rewritten;
        std::vector<u32> touched;
        if (variant.kind == VariantKind::GeometryKernel) {
            module = geometry.spirv;
        } else if (variant.kind == VariantKind::TessEval) {
            touched = BbIosTess::TouchPatchInputs(spirv);
            module = touched;
        } else if (scan.per_vertex) {''');p.write_text(s)
shutil.copyfile('native/tess_inputs.h',root/'video_core/renderer_metal/bb_ios_tess_inputs.h')
p=root/'video_core/renderer_metal/mtl_shader.h';s=p.read_text();s,n=re.subn(r'constexpr u32 kMslCacheVersion = \d+;', 'constexpr u32 kMslCacheVersion = 108; // iOS complete patch layout',s);assert n==1;p.write_text(s)
p=root/'video_core/renderer_vulkan/vk_pipeline_serialization.h';s=p.read_text();s,n=re.subn(r'inline constexpr u32 ShaderBinaryVersion = \d+u;', 'inline constexpr u32 ShaderBinaryVersion = 128u; // iOS RuntimeInfo patch-count fields',s);assert n==1;p.write_text(s)
print('iOS tessellation: complete control/evaluation patch layout, unused domain input loads, independent cache versions')
