// Helpers to read resources from the bindless heap. Each shader receives the indexes it
// needs in its push constants and takes textures, buffers and samplers straight from
// ResourceDescriptorHeap and SamplerDescriptorHeap.
#pragma once

// Reads a constant buffer of the heap into a plain value. Keeping the ConstantBuffer itself
// in a local makes DXC write invalid SPIR-V when optimizations are off (debug builds).
template<typename T>
T
loadConstants(uint index)
{
  return (ConstantBuffer<T>)ResourceDescriptorHeap[index];
}
