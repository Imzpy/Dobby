
#include "dobby_internal.h"

#include "PlatformUnifiedInterface/MemoryAllocator.h"

MemBlock *MemoryArena::allocMemBlock(size_t size) {
  MemBlock *result;

  // insufficient memory
  if (this->size - cursor_addr < size) {
    return nullptr;
  }

  result = new MemBlock(cursor_addr, size);
  cursor_addr += size;
  return result;
}


MemoryAllocator  *MemoryAllocator::shared_allocator = nullptr;
MemoryAllocator *MemoryAllocator::SharedAllocator() {
  if (MemoryAllocator::shared_allocator == nullptr) {
    MemoryAllocator::shared_allocator = new MemoryAllocator();
  }
  return MemoryAllocator::shared_allocator;
}


CodeMemoryArena *MemoryAllocator::allocateCodeMemoryArena(uint32_t size) {
  CodeMemoryArena *result = nullptr;

  uint32_t buffer_size = ALIGN_CEIL(size, OSMemory::PageSize());
  void *buffer = OSMemory::Allocate(buffer_size, kNoAccess);
  OSMemory::SetPermission(buffer, buffer_size, kReadExecute);

  result = new CodeMemoryArena((addr_t)buffer, (size_t)buffer_size);
  code_arenas.push_back(result);
  return result;
}

CodeMemBlock *MemoryAllocator::allocateExecBlock(uint32_t size) {
  CodeMemBlock *block = nullptr;
  for (auto iter = code_arenas.begin(); iter != code_arenas.end(); iter++) {
    auto arena = static_cast<CodeMemoryArena *>(*iter);
    block = arena->allocMemBlock(size);
    if (block)
      break;
  }
  if (!block) {
    // allocate new arena
    auto arena = allocateCodeMemoryArena(size);
    block = arena->allocMemBlock(size);
    CHECK_NOT_NULL(block);
  }

  DLOG(0, "[memory allocator] allocate exec memory at: , size: ", block->addr, block->size);
  return block;
}

uint8_t *MemoryAllocator::allocateExecMemory(uint32_t size) {
  auto block = allocateExecBlock(size);
  return (uint8_t *)block->addr;
}
uint8_t *MemoryAllocator::allocateExecMemory(uint8_t *buffer, uint32_t buffer_size) {
  auto mem = allocateExecMemory(buffer_size);
  auto ret = CodePatch(mem, buffer, buffer_size);
  CHECK_EQ(ret, kMemoryOperationSuccess);
  return mem;
}

DataMemoryArena *MemoryAllocator::allocateDataMemoryArena(uint32_t size) {
  DataMemoryArena *result = nullptr;

  uint32_t buffer_size = ALIGN_CEIL(size, OSMemory::PageSize());
  void *buffer = OSMemory::Allocate(buffer_size, kNoAccess);
  OSMemory::SetPermission(buffer, buffer_size, kReadWrite);

  result = new DataMemoryArena((addr_t)buffer, (size_t)buffer_size);
  data_arenas.push_back(result);
  return result;
}

DataMemBlock *MemoryAllocator::allocateDataBlock(uint32_t size) {
  CodeMemBlock *block = nullptr;
  for (auto iter = data_arenas.begin(); iter != data_arenas.end(); iter++) {
    auto arena = static_cast<DataMemoryArena *>(*iter);
    block = arena->allocMemBlock(size);
    if (block)
      break;
  }
  if (!block) {
    // allocate new arena
    auto arena = allocateCodeMemoryArena(size);
    block = arena->allocMemBlock(size);
    CHECK_NOT_NULL(block);
  }

  DLOG(0, "[memory allocator] allocate data memory at: , size: ", block->addr, block->size);
  return block;
}

uint8_t *MemoryAllocator::allocateDataMemory(uint32_t size) {
  auto block = allocateDataBlock(size);
  return (uint8_t *)block->addr;
}

uint8_t *MemoryAllocator::allocateDataMemory(uint8_t *buffer, uint32_t buffer_size) {
  auto mem = allocateDataMemory(buffer_size);
  memcpy(mem,buffer, buffer_size);
  return mem;
}