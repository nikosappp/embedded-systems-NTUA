#ifndef ARENA_ALLOCATOR_HPP
#define ARENA_ALLOCATOR_HPP
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>

// Arena allocator: malloc moves a position forward through the memory, and free never gives a
// single block back. When all the blocks have been freed, the position goes back to the start.
struct arena {
    // Every piece of memory we get from mmap starts with this header (16 bytes).
    // The pieces are kept in a list, so that after a reset they are reused instead of mapping new ones.
    struct chunk {
        chunk* next;       // the next piece
        std::size_t size;  // size of the whole piece, this header included
    };

    static constexpr std::size_t MMAP_BYTES = 1 << 20;   // 1 MB, same as worst fit
    static inline chunk* first = nullptr;                // the first piece
    static inline chunk* current = nullptr;              // the piece we are filling now
    static inline std::size_t pos = 0;                   // position inside current (bytes from its start)
    static inline std::size_t live = 0;                  // blocks that have not been freed yet

    static void* malloc(std::size_t size) {
        // Round up to a multiple of 16, so the returned memory is aligned to 16 bytes.
        std::size_t needed = (size + 15) / 16 * 16;
        if (needed == 0) needed = 16;

        // If the block does not fit in the current piece, move to the next one or map a new one.
        while (!current || pos + needed > current->size) {
            if (current && current->next && current->next->size >= sizeof(chunk) + needed) {
                current = current->next;          // reuse a piece we already have
                pos = sizeof(chunk);
                continue;
            }
            std::size_t bytes = (sizeof(chunk) + needed + MMAP_BYTES - 1) / MMAP_BYTES * MMAP_BYTES;
            void* memory = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (memory == MAP_FAILED) {
                std::fprintf(stderr, "arena: mmap of %zu bytes failed\n", bytes);
                std::abort();
            }
            chunk* c = (chunk*)memory;
            c->size = bytes;
            if (!current) {                       // the very first piece
                c->next = nullptr;
                first = c;
            } else {                              // goes right after the current one
                c->next = current->next;
                current->next = c;
            }
            current = c;
            pos = sizeof(chunk);
#ifdef ALLOC_STATS
            mmap_bytes += bytes;
#endif
        }

        // Hand out the memory at pos and move the position forward.
        void* block = (char*)current + pos;
        pos += needed;
        live++;
#ifdef ALLOC_STATS
        malloc_bytes += needed;
        if (malloc_bytes > peak_malloc_bytes) peak_malloc_bytes = malloc_bytes;
#endif
        return block;
    }

    static void free(void* pointer) {
        if (!pointer) return;
        live--;
        // When every block has been freed, the position goes back to the start of the first piece.
        if (live == 0) {
            current = first;
            pos = sizeof(chunk);
#ifdef ALLOC_STATS
            malloc_bytes = 0;
#endif
        }
    }

#ifdef ALLOC_STATS
    // All the bytes you got from mmap. With one huge mmap this is only the size you picked,
    // so map smaller pieces as you need them.
    static inline std::size_t mmap_bytes = 0;
    // The bytes used since the position was last at the start, and the most there ever were.
    static inline std::size_t malloc_bytes = 0;
    static inline std::size_t peak_malloc_bytes = 0;

    static void report_footprint() {
        std::printf("mmap_bytes=%zu malloc_bytes=%zu\n", mmap_bytes, peak_malloc_bytes);
    }
#endif
};

#endif