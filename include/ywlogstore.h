/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWLOGSTORE_H_
#define INCLUDE_YWLOGSTORE_H_

#include <ywcommon.h>
#include <ywsl.h>
#include <ywmempool.h>
#include <ywrcupool.h>
#include <ywkey.h>
#include <ywbarray.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>

typedef uint64_t ywOff;

class ywChunk {
    static const size_t  CHUNK_SIZE = 16*MB;

 public:
    Byte  body[CHUNK_SIZE];
};

class ywLogStore {
    static const uint32_t CHUNK_MAP_MAX = 1024;
    static const uint32_t CHUNK_SIZE    = sizeof(ywChunk);
    static const int32_t  BUFFERED_IO = 0;
    static const int32_t  DIRECT_IO = 1;
    static const int32_t  NO_IO = 2;

    class ywPos {
        static const uint32_t mask = 0xffffffff;

     public:
        explicit ywPos():val(0) {
        }
        explicit ywPos(ywOff _val):val(_val) {
        }
        bool is_null() {
            return val == 0;
        }
        void set_null() {
            set(0);
        }
        ywOff get() {
            return get_idx()*CHUNK_SIZE | get_offset();
        }
        void set(ywOff _val) {
            val = _val;
        }
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
                    uint64_t new_off;

                    new_off = (((prev >> 32) & mask) + 1) << 32;
                    if (cas(&val, prev, new_off)) {
                        return ywPos(prev);
                    }

                    prev = val;
                    off = prev & mask;
                }
            } while (!cas(&val, prev, prev + size));

            return ywPos(prev);
        }

        void trun_chunk() {
        }

        uint64_t get_offset() {
            return (val >>  0) & mask;
        }
        uint64_t get_idx() {
            return (val >> 32) & mask;
        }

        ywOff val;
    };

 public:
    explicit ywLogStore(const char * fn, int32_t _io):fd(-1),
    io(_io) {
        uint32_t flag = O_RDWR | O_CREAT | O_LARGEFILE;

        cur_chunk       = chunk_pool.alloc();
        append_pos.set_null();
        write_pos.set_null();
        flush_pos.set_null();

        if (io == DIRECT_IO) flag |= O_DIRECT;

        fd = open(fn, flag, S_IRWXU);
        if (fd == -1) {
            perror("file open error:");
            assert(fd != -1);
        }
    }
    ~ywLogStore() {
        if (fd != -1) {
            close(fd);
        }
    }

    bool fliping() {
        if (flush()) {
            return true;
        }
        return false;
    }

    template<typename T>
    ywOff append(T *value) {
        ywPos pos = append_pos.alloc(value->get_size());
        Byte *ptr = cur_chunk->body + pos.get_offset();

        if (pos.get_offset() == 0) {
            if (io != NO_IO) {
                fliping();
            }
        }
        value->write(ptr);

        return pos.get();
    }

    bool flush() {
        struct iovec iov[1];
        size_t ret;
        size_t offset = append_pos.get_idx() * CHUNK_SIZE;

        iov[0].iov_base = cur_chunk->body;
        iov[0].iov_len  = CHUNK_SIZE;

        ret = pwritev(fd, iov, 1, offset);
        sync();
        return ret == iov[0].iov_len;
    }

    void sync() {
        fsync(fd);
    }

    bool read() {
        struct iovec iov[1];
        size_t ret;

        iov[0].iov_base = cur_chunk->body;
        iov[0].iov_len  = CHUNK_SIZE;

        ret = preadv(fd, iov, 1, 0);
        return ret == iov[0].iov_len;
    }

    void dump(int32_t chunk_idx) {
        printf("Chunk : %d\n", chunk_idx);
        // printHex(sizeof(cur_chunk->body), cur_chunk->body, true);
        printHex(1024, cur_chunk->body, true);
    }

    ywOff get_append_pos() {
        return append_pos.get();
    }

 private:
    int32_t  fd;
    int32_t  io;
    ywChunk *cur_chunk;
    ywPos    append_pos;
    ywPos    write_pos;
    ywPos    flush_pos;
    int32_t  chunk_map[CHUNK_MAP_MAX];

    ywRcuPool<ywChunk, 256*MB>  chunk_pool;
};

extern void logstore_basic_test(int32_t io, int32_t thread_count);

#endif  // INCLUDE_YWLOGSTORE_H_
