/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWLOGSTORE_H_
#define INCLUDE_YWLOGSTORE_H_

#include <ywcommon.h>
#include <ywsl.h>
#include <ywmempool.h>
#include <ywrcupool.h>
#include <ywkey.h>
#include <ywbarray.h>
#include <ywutil.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <ywchunkmgr.h>

typedef uint64_t ywOff;

class ywChunk {
    static const size_t  CHUNK_SIZE = 16*MB;

 public:
    Byte  body[CHUNK_SIZE];
};

static_assert(sizeof(ywCnkID) == 4, "invalid ywCnkID Size");

class ywLogStore {
 public:
    static const uint32_t MEM_CHUNK_COUNT = 8;
    static const uint32_t CHUNK_SIZE    = sizeof(ywChunk);
    static const intptr_t DIO_ALIGN     = 512;

    /* IO Type */
    static const int32_t  BUFFERED_IO = 0;
    static const int32_t  DIRECT_IO = 1;
    static const int32_t  NO_IO = 2;

    /*CnkID Type*/
    static const uint32_t MASTER_CNK = 0;
    static const uint32_t LOG_CNK    = 1;
    static const uint32_t INFO_CNK   = 2;
    static const uint32_t SORTED_CNK = 3;

 private:
    class ywPos {
        static const uint32_t mask = 0xffffffff;

     public:
        explicit ywPos():val(0) { }
        explicit ywPos(ywOff _val):val(_val) { }
        bool  is_null() { return val == 0;}
        ywOff get()     { return get_idx()*CHUNK_SIZE | get_offset();}
        void set_null()      { set(0); }
        void set(ywOff _val) { val = _val; }
        void set(uint32_t idx, uint32_t offset) {
            val = (static_cast<ywOff>(idx) << 32) |
                  (static_cast<ywOff>(offset));
        }

        template<typename T>
        bool cas(T *ptr, T prev, T next) {
            return __sync_bool_compare_and_swap(ptr, prev, next);
        }

        ywPos alloc(size_t size) {
            ywOff    prev;
            uint32_t off;

            do {
                prev = val;
                off = prev & mask;

                assert(off < CHUNK_SIZE);

                while (off + size > CHUNK_SIZE) {
                    if (next_chunk(prev)) {
                        prev = val;
                        off = prev & mask;
                        break;
                    }
                    prev = val;
                    off = prev & mask;
                }
            } while (!cas(&val, prev, prev + size));

            return ywPos(prev);
        }

        bool next_chunk() { return next_chunk(val); }
        bool next_chunk(ywOff prev) {
            ywOff    new_off = (((prev >> 32) & mask) + 1) << 32;
            return cas(&val, prev, new_off);
        }

        uint64_t get_offset() {return (val >>  0) & mask;}
        uint64_t get_idx() {return (val >> 32) & mask;}

     private:
        ywOff val;
    };

 public:
    explicit ywLogStore(const char * fn, int32_t _io):fd(-1),
    io(_io), done(false), running(false) {
        init(fn, _io);
    }

    ~ywLogStore() {
        done = true;
        if (fd != -1)   { close(fd); }
        while (running) { usleep(100); }

        free(chunk_buffer);

        printf("wait_count : %" PRId64 "\n", wait_count.get());
    }

    bool create_flush_thread();

    template<typename T>
    ywOff append(T value) {
        return append(&value);
    }

    template<typename T>
    ywOff append(T *value) {
        ywPos     pos = append_pos.alloc(value->get_size());
        uint32_t  mem_chunk_idx = pos.get_idx() % MEM_CHUNK_COUNT;
        Byte     *ptr = get_chunk_ptr(pos);
        uint32_t  _wait_count = 0;

        while (chunk_idx[mem_chunk_idx] != pos.get_idx()) {
            ++_wait_count;
            usleep(1);
        }

        value->write(ptr);

        wait_count.mutate(_wait_count);

        return pos.get();
    }

    bool  write_chunk(ywChunk *chunk, int32_t idx) {
        ssize_t ret = pwrite(fd, chunk->body, CHUNK_SIZE, idx * CHUNK_SIZE);
        return ret == CHUNK_SIZE;
    }

    void sync() {
        fsync(fd);
    }

    bool reserve_space();
    bool flush();
    void wait_flush();

    bool read(ssize_t offset, int32_t size, Byte * buf) {
        return (pread(fd, buf, size, offset) == offset);
    }

    void print(int32_t chunk_idx) {
        printf("Chunk : %d\n", chunk_idx);
        // printHex(sizeof(cur_chunk->body), cur_chunk->body, true);
        printHex(1024, chunk_ptr[chunk_idx % MEM_CHUNK_COUNT]->body, true);
    }

    ywOff get_append_pos() {
        return append_pos.get();
    }
    ywOff get_write_pos() {
        return write_pos.get();
    }

 private:
    void init(const char * fn, int32_t _io);
    void init_and_read_master();

    static void *log_flusher(void *arg_ptr);

    Byte *get_chunk_ptr(ywPos pos) {
        return chunk_ptr[pos.get_idx() % MEM_CHUNK_COUNT]->body
               + pos.get_offset();
    }
    bool set_chunk_idx(int32_t cnk_idx) {
        int32_t mem_idx = reserve_mem_idx;

        if(chunk_idx[mem_idx] == -1) {
            chunk_idx[mem_idx] = cnk_idx;
            ++reserve_mem_idx;
            return true;
        }
        return false;
    }

    int32_t                 fd;
    int32_t                 io;
    Byte                   *chunk_buffer;
    ywChunk                *chunk_ptr[MEM_CHUNK_COUNT];
    int32_t                 chunk_idx[MEM_CHUNK_COUNT];
    ywPos                   append_pos;
    ywPos                   write_pos;
    int32_t                 reserve_mem_idx;

    ywChunkMgr;             cnk_mgr;

    ywAccumulator<int64_t>  wait_count;
    bool                    done;
    bool                    running;
};

extern void logstore_basic_test(int32_t io, int32_t thread_count);

#endif  // INCLUDE_YWLOGSTORE_H_
