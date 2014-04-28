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
#include <ywpos.h>

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
    static const uint32_t MASTER_CNK = 1;
    static const uint32_t LOG_CNK    = 2;
    static const uint32_t INFO_CNK   = 3;
    static const uint32_t SORTED_CNK = 4;

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
    auto append(T value) {
        return append(&value);
    }

    template<typename T>
    ywPos append(T *value) {
        ywPos     pos = append_pos.alloc<CHUNK_SIZE>(value->get_size());
        Byte     *ptr = get_chunk_ptr(pos);
        uint32_t  _wait_count = 0;

        while (get_cnkid(pos).is_null()) {
            ++_wait_count;
            usleep(1);
        }

        value->write(ptr);

        wait_count.mutate(_wait_count);

        return pos;
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

    uint64_t get_append_pos() { return append_pos.get(); }
    uint64_t get_write_pos()  { return write_pos.get(); }

 private:
    void init(const char * fn, int32_t _io);
    void init_and_read_master();

    static void *log_flusher(void *arg_ptr);

    /*off - cnkid mpa*/
    CnkID get_cnkid(ywPos pos) {
    }
    Byte *get_chunk_ptr(ywPos pos) {
        return chunk_ptr[pos.get_idx() % MEM_CHUNK_COUNT]->body
               + pos.get_offset();
    }
    bool set_chunk_idx(int32_t cnk_idx) {
        int32_t mem_idx = reserve_mem_idx;

        if (chunk_idx[mem_idx] == -1) {
            chunk_idx[mem_idx] = cnk_idx;
            reserve_mem_idx = (reserve_mem_idx+1) % MEM_CHUNK_COUNT;
            return true;
        }
        return false;
    }

    int32_t                 fd;
    int32_t                 io;
    Byte                   *chunk_buffer;
    ywChunk                *chunk_ptr[MEM_CHUNK_COUNT];
    ywCnkID                 off_to_cnkid_map[MEM_CHUNK_COUNT];

    ywPos                   append_pos;
    ywPos                   write_pos;
    int32_t                 reserve_mem_idx;

    ywChunkMgr              cnk_mgr;

    ywAccumulator<int64_t>  wait_count;
    bool                    done;
    bool                    running;
};

extern void logstore_basic_test(int32_t io, int32_t thread_count);

#endif  // INCLUDE_YWLOGSTORE_H_
