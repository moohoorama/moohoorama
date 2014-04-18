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

typedef uint64_t ywOff;

class ywChunk {
    static const size_t  CHUNK_SIZE = 16*MB;

 public:
    Byte  body[CHUNK_SIZE];
};

class ywLogStore {
    static const uint32_t CHUNK_MAP_MAX = 1024;
    static const uint32_t MEM_CHUNK_COUNT = 8;
    static const uint32_t CHUNK_SIZE    = sizeof(ywChunk);
    static const intptr_t DIO_ALIGN     = 512;
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

        bool next_chunk() {
            return next_chunk(val);
        }

        bool next_chunk(ywOff prev) {
            ywOff    new_off = (((prev >> 32) & mask) + 1) << 32;
            return cas(&val, prev, new_off);
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
    io(_io), done(false), running(false) {
        uint32_t  flag = O_RDWR | O_CREAT | O_LARGEFILE;
        uint32_t  i;
        Byte     *aligned_buffer;

        chunk_buffer = reinterpret_cast<Byte*>(malloc(
                MEM_CHUNK_COUNT*CHUNK_SIZE+DIO_ALIGN));
        aligned_buffer = reinterpret_cast<Byte*>(
            align(reinterpret_cast<intptr_t>(chunk_buffer), DIO_ALIGN));

        for (i = 0; i < MEM_CHUNK_COUNT; ++i) {
            chunk_ptr[i] = reinterpret_cast<ywChunk*>(aligned_buffer +
                         i*CHUNK_SIZE);
            chunk_idx[i] = i;
//            chunk_ptr[i] = chunk_pool.alloc();
        }
        append_pos.set_null();
        write_pos.set_null();
        flush_pos.set_null();

        if (io == DIRECT_IO) flag |= O_DIRECT;

        fd = open(fn, flag, S_IRWXU);
        if (fd == -1) {
            perror("file open error:");
            assert(fd != -1);
        }

        assert(create_flush_thread());
    }
    ~ywLogStore() {
        done = true;
        if (fd != -1) {
            close(fd);
        }
        while (running) {
            usleep(100);
        }

        free(chunk_buffer);

        printf("wait_count : %" PRId64 "\n", wait_count.get());
    }

    bool create_flush_thread();

    template<typename T>
    ywOff append(T *value) {
        ywPos     pos = append_pos.alloc(value->get_size());
        uint32_t  mem_chunk_no = pos.get_idx() % MEM_CHUNK_COUNT;
        Byte     *ptr = get_chunk_ptr(pos);
        uint32_t  _wait_count = 0;

        while (chunk_idx[mem_chunk_no] != pos.get_idx()) {
            ++_wait_count;
            usleep(1);
        }

        value->write(ptr);

        wait_count.mutate(_wait_count);

        return pos.get();
    }

    bool flush() {
        struct    iovec iov[MEM_CHUNK_COUNT];
        int32_t   old_idx[MEM_CHUNK_COUNT];
        size_t    ret;
        size_t    offset = write_pos.get_idx() * CHUNK_SIZE;
        ywOff     old_val = write_pos.get();
        int32_t  *chunk_idx_ptr;
        uint32_t  cnt;
        uint32_t  mem_chunk_no = write_pos.get_idx() % MEM_CHUNK_COUNT;
        uint32_t  i;

        for (i = 0, cnt = 0; i < MEM_CHUNK_COUNT; ++i, ++cnt) {
            if (write_pos.get_idx() >= append_pos.get_idx()) {
                break;
            }

            old_idx[i]      = write_pos.get_idx();
            iov[i].iov_base = chunk_ptr[mem_chunk_no+i]->body;
            iov[i].iov_len  = CHUNK_SIZE;
            if (!write_pos.next_chunk()) {
                write_pos.set(old_val);
                return false;
            }
        }

        if (!cnt) return false;

        if (io != NO_IO) {
            ret = pwritev(fd, iov, i, offset);
            //        fsync(fd);
            if (!ret == iov[0].iov_len) {
                write_pos.set(old_val);
                perror("flush error:");
                return false;
            }
        }

        for (i = 0; i < cnt; ++i) {
            chunk_idx_ptr = &chunk_idx[(mem_chunk_no + i) % MEM_CHUNK_COUNT];
            assert(__sync_bool_compare_and_swap(
                    chunk_idx_ptr, old_idx[i], old_idx[i] + MEM_CHUNK_COUNT));
        }
        return true;
    }

    void wait_flush() {
        if (io != NO_IO) {
            while (write_pos.get_idx() <
                   append_pos.get_idx()) {
                usleep(100);
            }
        }
    }

    /*
    bool read() {
        struct iovec iov[1];
        size_t ret;

        iov[0].iov_base = get_chunk_ptr(mem_chunk_idx);
        iov[0].iov_len  = CHUNK_SIZE;

        ret = preadv(fd, iov, 1, 0);
        return ret == iov[0].iov_len;
    }
    */

    void dump(int32_t chunk_idx) {
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
    static void *log_flusher(void *arg_ptr);

    Byte *get_chunk_ptr(ywPos pos) {
        return chunk_ptr[pos.get_idx() % MEM_CHUNK_COUNT]->body
               + pos.get_offset();
    }

    int32_t                 fd;
    int32_t                 io;
    Byte                   *chunk_buffer;
    ywChunk                *chunk_ptr[MEM_CHUNK_COUNT];
    int32_t                 chunk_idx[MEM_CHUNK_COUNT];
    ywPos                   append_pos;
    ywPos                   write_pos;
    ywPos                   flush_pos;
    int32_t                 chunk_map[CHUNK_MAP_MAX];
    ywAccumulator<int64_t>  wait_count;
    bool                    done;
    bool                    running;

//    ywRcuPool<ywChunk, 256*MB>  chunk_pool;
};

extern void logstore_basic_test(int32_t io, int32_t thread_count);

#endif  // INCLUDE_YWLOGSTORE_H_
