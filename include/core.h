#ifndef CORE_H
#define CORE_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <cassert>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using usize = size_t;
using f32 = float;
using f64 = double;

using str = const char*;

// Deferring!
template <typename F>
struct privDefer {
  F f;
  privDefer(F f): f(f) {}
  ~privDefer() { f(); }
};

template <typename F>
privDefer<F> defer_func(F f) {
  return privDefer<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define defer(code)   auto DEFER_3(_defer_) = defer_func([&](){code;})

namespace core {
  // Common utility
  template <typename T>
  T Max(T x, T y) {
    return x > y ? x : y;
  }

  template <typename T>
  void Swap(T* a, T* b) {
    T temp = *a;
    *a = *b;
    *b = temp;
  }

  // Arena
  struct ArenaChunk {
    usize capacity{0};
    usize cursor{0};
    u8* buffer{nullptr};
    ArenaChunk* next{nullptr};
  };

  struct Arena {
    ArenaChunk* first{nullptr};
    ArenaChunk* last{nullptr};
    usize capacity{0};
    usize num_chunks{0};
  };

  template <typename T>
  void ZeroOut(T* ptr, usize num_items = 1) {
    if (ptr) {
      memset(ptr, 0, sizeof(T) * num_items);
    }
  }

  Arena NewArena(usize capacity);
  u8* AllocBytes(Arena* arena, usize num_bytes);

  void Free(Arena* arena);
  void Reset(Arena* arena);

  template <typename T>
  T* Alloc(Arena* arena) {
    auto buffer = AllocBytes(arena, sizeof(T));
    return new(buffer) T;
  }

  template <typename T>
  T* Alloc(Arena* arena, usize num_items) {
    auto buffer = AllocBytes(arena, sizeof(T) * num_items);
    return new(buffer) T[num_items];
  }

  // Array
  template <typename T>
  struct ArrayData {
    usize len{0};
    usize capacity{0};
    T* buffer{nullptr};
  };

  template <typename T>
  using Array = ArrayData<T>*;

  template <typename T>
  T* begin(Array<T> array) {
    return array->buffer;
  }

  template <typename T>
  T* end(Array<T> array) {
    return array->buffer + array->len;
  }
  

  template <typename T>
  Array<T> NewEmptyArray(Arena* arena, usize capacity) {
    Array<T> array = Alloc<ArrayData<T>>(arena);
    array->capacity = capacity;
    array->len = 0;
    array->buffer = Alloc<T>(arena, capacity);
    return array;
  }

  template <typename T>
  Array<T> NewFullArray(Arena* arena, usize length) {
    Array<T> array = Alloc<ArrayData<T>>(arena);
    array->capacity = length;
    array->len = length;
    array->buffer = Alloc<T>(arena, length);
    return array;
  }

  template <typename T>
  bool Push(Array<T> array, T value) {
    if (array->len < array->capacity) {
      array->buffer[array->len] = value;
      array->len++;
      return true;
    } else {
      return false;
    }
  }

  template <typename T>
  T Pop(Array<T> array) {
    assert(array->len > 0);
    array->len--;
    auto* ptr = &array->buffer[array->len];
    T value = *ptr;
    ZeroOut(ptr);
    return value;
  }

  template <typename T>
  T* At(Array<T> array, usize idx) {
    if (idx < array->len) {
      return &array->buffer[idx];
    } else {
      return nullptr;
    }
  }

  template <typename T>
  T* Last(Array<T> array) {
    if (array->len == 0) { return nullptr; }
    return At(array, array->len - 1);
  }

  template <typename T>
  T Get(Array<T> array, usize idx) {
    if (auto ptr = At(array, idx)) {
      return *ptr;
    } else {
      return {0};
    }
  }

  template <typename T>
  T* Emplace(Array<T> array, T value) {
    if (Push(array, value)) {
      return At(array, array->len - 1);
    } else {
      return nullptr;
    }
  }


  template <typename T>
  void Clear(Array<T> array) {
    ZeroOut(array->buffer, array->len);
    array->len = 0;
  }

  // String8
  struct String8  {
    const char* ptr = "\0";
    usize len{0};

    friend std::ostream& operator<<(std::ostream& os, const String8& dt);
  };

  // IMPORTANT: Only use with literals!
  String8 Lit(const char* lit);
  String8 SubstringUntil(String8 base, char ch);
  bool IsEmpty(String8 str);
  const char* CStr(Arena* arena, String8 string);

  // Hashing
  u64 Hash(u64 x, u64 y);
  u64 Hash(String8 str);

  // Slotmap
  template <typename T>
  struct SlotMapNode {
    u32 generation{0};
    union {
      T data;
      u32 next_free;
    };
  };

  template <typename K, typename V>
  struct SlotMap {
      usize length{0};
      usize capacity{0};
      SlotMapNode<V>* nodes{nullptr};
      u32 next_free{0};
  };

  template <typename K, typename V>
  void Grow(SlotMap<K, V>* sm, usize capacity) {
    if (capacity <= sm->capacity) {
      return;
    }
    SlotMapNode<V>* nodes = (SlotMapNode<V>*)malloc(sizeof(SlotMapNode<V>) * capacity);
    if (sm->nodes) {
      memcpy(nodes, sm->nodes, sizeof(SlotMapNode<V>) * sm->capacity);
      free(sm->nodes);
    }
    sm->nodes = nodes;
    sm->capacity = capacity;
  }

  template <typename K, typename V>
  K Insert(SlotMap<K, V>* sm, V value) {
    if (sm->next_free >= sm->capacity) {
      auto capacity = Max<usize>(sm->capacity * 2, 100);
      Grow(sm, capacity);
    }

    bool is_new_alloc = sm->next_free == sm->length;
    u32 index = sm->next_free;
    SlotMapNode<V>* node = &sm->nodes[index];
    assert(node->generation % 2 == 0);
    node->generation += 1;

    K id = { .index = index, .generation = node->generation };

    if (is_new_alloc) {
      sm->next_free += 1;
      sm->length += 1;
      node->data = value;
    } else {
      sm->next_free = node->next_free;
      node->data = value;
    }

    return id;
  }

  template <typename K, typename V>
  V* Get(SlotMap<K, V>* sm, K id) {
    if (id.index >= sm->length) {
      return nullptr;
    }
    SlotMapNode<V>* node = &sm->nodes[id.index];
    if (node->generation != id.generation) {
      return nullptr;
    }
    return &node->data;
  }

  template <typename K, typename V>
  bool Contains(SlotMap<K, V>* sm, K id) {
    return Get(sm, id) != nullptr;
  }

  template <typename K, typename V>
  bool Remove(SlotMap<K, V>* sm, K id) {
    if (id.index >= sm->length) {
      return false;
    }
    SlotMapNode<V>* node = &sm->nodes[id.index];
    if (node->generation != id.generation) {
      return false;
    }

    node->generation += 1;
    assert(node->generation % 2 == 0);
    node->next_free = sm->next_free;
    sm->next_free = id.index;
    return true;
  }

  template <typename K, typename V>
  struct SlotMapIter {
    SlotMap<K, V>* sm{nullptr};
    u32 idx{0};
  };

  template <typename K, typename V>
  struct SlotMapEntry {
    K key{0};
    V* value{nullptr};

    explicit operator bool() const {
      return this->value != nullptr;
    }
  };

  template <typename K, typename V>
  SlotMapIter<K, V> Iter(SlotMap <K, V>* sm) {
    assert(sm);
    return {
      .sm = sm,
      .idx = 0,  
    };
  }

  template <typename K, typename V>
  SlotMapEntry<K, V> Next(SlotMapIter<K, V>* iter) {
    while (true) {
      if (iter->idx >= iter->sm->length) {
        return {};
      }
      SlotMapNode<V>* node = &iter->sm->nodes[iter->idx];
      // If the node has some value...
      if (node->generation % 2 == 1) {
        K key = { .index = iter->idx, .generation = node->generation };
        iter->idx++;
        return { .key = key, .value = &node->data };
      } else {
        // Just increment
        iter->idx++;
      }
    }
  }

  
  #define MAKE_SLOTMAP_KEY(NAME) \
  struct NAME { \
    u32 index{0}; \
    u32 generation{0}; \
                       \
    bool operator==(const Entity& other) const { \
      return this->index == other.index &&       \
             this->generation == other.generation; \
    }  \
  };
}
#endif
