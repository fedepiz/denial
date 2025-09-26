#include <core.h>
#include <cstring>

  namespace core {
  inline static
  usize LastChunkFreeSpace(Arena* arena) {
    if (!arena->last) {
      return 0;
    } else {
      return arena->last->capacity - arena->last->cursor;
    }
  }

  inline static
  ArenaChunk* NewChunk(usize capacity) {
    // Allocate the chunk
    auto* chunk = new ArenaChunk;
    auto* buffer = new u8[capacity];
    memset(buffer, 0, capacity);
    *chunk = {
      .capacity = capacity,
      .cursor = 0,
      .buffer = buffer,
      .next = nullptr,
    };
    return chunk;
  }

  inline static
  void GrowArena(Arena* arena, usize num_bytes) {
      auto* chunk = NewChunk(num_bytes);
      if (arena->last) {
        arena->last->next = chunk;
        arena->last = chunk;
      } else {
        arena->first = chunk;
        arena->last = chunk;
      }
      arena->capacity += num_bytes;
      arena->num_chunks += 1;
  }

  Arena NewArena(usize capacity) {
    Arena arena;
    GrowArena(&arena, capacity);
    return arena;
  }


  u8* AllocBytes(Arena* arena, usize num_bytes) {
    if (LastChunkFreeSpace(arena) < num_bytes) {
      GrowArena(arena, Max(num_bytes, (usize)2048));
    }
    u8* buffer = arena->last->buffer + arena->last->cursor;
    arena->last->cursor += num_bytes;
    return buffer;
  }

  void Free(Arena* arena) {
    auto chunk = arena->first;
    while (chunk) {
      auto next = chunk->next;
      delete[] chunk->buffer;
      delete chunk;
      chunk = next;
    }

    ZeroOut(arena);
  }

  void Reset(Arena* arena) {
    if (arena->num_chunks == 0) { return; }
    else if (arena->num_chunks == 1) {
      auto chunk = arena->first;
      chunk->cursor = 0;
      memset(chunk->buffer, 0, chunk->capacity);
    } else {
      // Free all chunks
      auto capacity = arena->capacity;
      Free(arena);
      GrowArena(arena, capacity);
    }
  }

  // String8
  // 
  String8 Lit(const char* lit) {
    usize len = strlen(lit);
    return {
      .ptr = lit,
      .len = len,
    };
  }

  std::ostream& operator<<(std::ostream& os, const String8& dt)
  {
    os << dt.ptr;
    return os;
  }

  String8 SubstringUntil(String8 base, char ch) {
    usize i = 0;
    while (i < base.len) {
      if (base.ptr[i] == ch) {
        break;
      }
      i++;
    }

    return {
      .len = i,
      .ptr = base.ptr
    };
  }

  bool IsEmpty(String8 str) {
    return str.len == 0;
  }

  const char* CStr(Arena* arena, String8 string) {
    char* ptr = (char*)AllocBytes(arena, string.len + 1);
    memcpy(ptr, string.ptr, string.len);
    ptr[string.len] = '\0';
    return ptr;
  }

  
  u64 Hash(u64 x, u64 y) {
    return x * 13 + y * 17;
  }

  u64 Hash(String8 str) {
    u64 accum = 0;
    for (int i = 0; i < str.len; ++i) {
      char ch = str.ptr[i];
      accum = Hash(accum, ch);
    }
    return accum;
  }
}

