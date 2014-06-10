/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWSYMBOL_H_
#define INCLUDE_YWSYMBOL_H_

#include <ywcommon.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>

#include <map>
#include <string>

class ywSymbol {
    static const char EMPTY_STRING = '\0';

 public:
    explicit ywSymbol() {
    }

    explicit ywSymbol(const char *fn) {
        load(fn);
    }
    void  load(const char *fn, bool print = false) {
        int32_t fd = open(fn, O_RDONLY, S_IRWXU);
        if (fd == -1) {
            perror("file open error:");
            return;
        }

        char  buffer[1024];
        char *ptr = &buffer[0];

        while (read(fd, ptr++, 1)) {
            if (ptr[-1] == '\n') {
                ptr[-1] = '\0';
                readline(buffer, print);
                ptr = &buffer[0];
            }
        }
        ptr[0] = '\0';
        readline(buffer, print);

        close(fd);
    }
    const char *find(intptr_t val) {
        std::map<intptr_t, std::string>::iterator itr;
        itr = _symbol_table.upper_bound(val);

        if (itr == _symbol_table.end() || val == 0) {
            return &EMPTY_STRING;
        }
        itr--;
        return itr->second.c_str();
    }

    void readline(char * buf, bool print = false) {
        intptr_t addr = 0;
        intptr_t val;

        /* find first ' '*/
        while (*buf != ' ') {
            if (*buf == '\0') return;

            if ('0' <= *buf && *buf <='9') {
                val = *buf - '0';
            } else {
                if ('A' <= *buf && *buf <='Z') {
                    val = *buf - 'A' + 10;
                } else {
                    if ('a' <= *buf && *buf <='z') {
                        val = *buf - 'a' + 10;
                    } else {
                        break;
                    }
                }
            }
            addr = addr*16 + val;
            ++buf;
        }

        while (!(buf[0] == ' ' && buf[1] != ' ' && buf[2] == ' ')) {
            if (*buf == '\0') return;
            ++buf;
        }

        buf += 3;

        if (addr) {
            _symbol_table[addr] = std::string(buf);
            if (print) {
                printf("%10" PRIXPTR "  %s\n",
                       addr, _symbol_table[addr].c_str());
            }
        }
    }

    int32_t get_count() {
        return _symbol_table.size();
    }

 private:
    std::map<intptr_t, std::string> _symbol_table;
};

extern ywSymbol *get_default_symbol();

void symbol_basic_test();

#endif  // INCLUDE_YWSYMBOL_H_
