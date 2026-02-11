#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

#define HEAP_SIZE (50 * 1024 * 1024) // 定义堆大小为 50MB
static char klib_heap[HEAP_SIZE];    // 静态分配的堆内存空间

// 内存块头部结构体
typedef struct BlockHeader {
    size_t size;               // 块的大小（包含头部本身）
    bool free;                 // 标记该块是否空闲
    struct BlockHeader *next;  // 指向链表中下一个内存块的指针
} BlockHeader;

static BlockHeader *heap_start = NULL; // 堆的起始指针

// 初始化堆
static void klib_heap_init() {
    heap_start = (BlockHeader *)klib_heap;
    heap_start->size = HEAP_SIZE;
    heap_start->free = true;
    heap_start->next = NULL;
}

/// @brief 申请堆空间
/// @param size 用户所需的字节数
/// @return 
void *malloc(size_t size) {
  // 如果堆尚未初始化，则进行初始化
  if (heap_start == NULL) {
    klib_heap_init();
  }

  // 字节对齐：将请求的大小向上对齐到 size_t 的倍数（通常是 4 或 8 字节）
  size = (size + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);
  if (size == 0) return NULL; // 不分配 0 字节的内存

  BlockHeader *current = heap_start;
  // 遍历链表寻找合适的空闲块（首次适应算法）
  while (current != NULL) {
    // 如果当前块空闲且空间足够（至少能容纳请求大小 + 头部大小）
    if (current->free && current->size >= size + sizeof(BlockHeader)) {
      
      // 定义分割所需的最小剩余空间：头部大小 + 最小实际数据大小
      const size_t MIN_BLOCK_SIZE_FOR_SPLIT = sizeof(BlockHeader) + sizeof(size_t);
      
      // 如果当前块足够大，可以分割成两个块
      if (current->size > size + sizeof(BlockHeader) + MIN_BLOCK_SIZE_FOR_SPLIT) {
        BlockHeader *new_block = (BlockHeader *)((char *)current + size + sizeof(BlockHeader));
        
        new_block->size = current->size - (size + sizeof(BlockHeader));
        new_block->free = true;
        new_block->next = current->next;
        
        current->next = new_block;
        current->size = size + sizeof(BlockHeader);
      }

      current->free = false; 
      // 指向数据区
      return (void *)(current + 1); 
    }
    current = current->next;
  }
  return NULL;
}

void free(void *ptr) {
}

#endif
