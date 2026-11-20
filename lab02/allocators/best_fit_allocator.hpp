#ifndef BEST_FIT_ALLOCATOR_HPP
#define BEST_FIT_ALLOCATOR_HPP
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>

// Best fit allocator: malloc takes the SMALLEST free block that is big enough.
struct best_fit {
    // In front of every block, same as worst fit. 16 bytes, and all sizes are multiples of 16,
    // so the memory that malloc returns is aligned to 16 bytes.
    struct header {
        std::size_t size;  // of the whole block, header included
        header* next;      // next free block (only while this block is free)
    };

    static constexpr std::size_t MMAP_BYTES = 1 << 20;   // map memory in pieces of 1 MB
    static inline header* free_list = nullptr;           // free blocks, sorted by address

    static void* malloc(std::size_t size) {
        std::size_t needed = (sizeof(header) + size + 15) / 16 * 16;

        while (true) {
            // Walk the WHOLE free list and keep the smallest block that is big enough.
            header* best = nullptr;
            header* best_prev = nullptr;
            header* prev = nullptr;
            for (header* b = free_list; b; prev = b, b = b->next) {
                if (b->size < needed) continue;
                if (!best || b->size < best->size) {
                    best = b;
                    best_prev = prev;
                    if (b->size == needed) break;   // exact fit: nothing can be better
                }
            }

            if (best) {
                header* block;
                if (best->size - needed >= 2 * sizeof(header)) {
                    // Big enough to split: cut the new block from the end, so the free block
                    // keeps its place in the list.
                    best->size -= needed;
                    block = (header*)((char*)best + best->size);
                    block->size = needed;
                } else {
                    // The leftover would be too small to be useful: hand out the whole block
                    // and take it out of the free list.
                    if (best_prev) best_prev->next = best->next;
                    else free_list = best->next;
                    block = best;
                }
#ifdef ALLOC_STATS
                malloc_bytes += block->size;
                if (malloc_bytes > peak_malloc_bytes) peak_malloc_bytes = malloc_bytes;
#endif
                return block + 1;
            }

            // No free block is big enough: map a new piece, put it in the list and search again.
            std::size_t bytes = (needed + MMAP_BYTES - 1) / MMAP_BYTES * MMAP_BYTES;
            void* memory = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (memory == MAP_FAILED) {
                std::fprintf(stderr, "best_fit: mmap of %zu bytes failed\n", bytes);
                std::abort();
            }
#ifdef ALLOC_STATS
            mmap_bytes += bytes;
#endif
            header* fresh = (header*)memory;
            fresh->size = bytes;
            insert(fresh);
        }
    }

    static void free(void* pointer) {
        if (!pointer) return;
        header* block = (header*)pointer - 1;  // one header (16 bytes) back
#ifdef ALLOC_STATS
        malloc_bytes -= block->size;
#endif
        insert(block);
    }

    // Puts a block in the free list, in address order, and merges it with the free blocks
    // right before and after it. Same as in worst fit.
    static void insert(header* block) {
        header* before = nullptr;
        header* after = free_list;
        while (after && after < block) {
            before = after;
            after = after->next;
        }
        block->next = after;
        if (before) before->next = block;
        else free_list = block;

        if (after && (char*)block + block->size == (char*)after) {
            block->size += after->size;
            block->next = after->next;
        }
        if (before && (char*)before + before->size == (char*)block) {
            before->size += block->size;
            before->next = block->next;
        }
    }

#ifdef ALLOC_STATS
    // All the bytes you got from mmap. With one huge mmap this is only the size you picked,
    // so map smaller pieces as you need them.
    static inline std::size_t mmap_bytes = 0;
    // The bytes of the blocks that are live now, and the most there ever were at the same time.
    static inline std::size_t malloc_bytes = 0;
    static inline std::size_t peak_malloc_bytes = 0;

    static void report_footprint() {
        std::printf("mmap_bytes=%zu malloc_bytes=%zu\n", mmap_bytes, peak_malloc_bytes);
    }
#endif
};

#endif