/* Copyright [2014] moohoorama@gmail.com Kim.Youn-woo */

#ifndef INCLUDE_YWSERIALIZER_H_
#define INCLUDE_YWSERIALIZER_H_

#include <ywcommon.h>

class ar_print {
};
/* precondition : declare variables fd & size */
#define VAR_WRITE(type, name) do {                           \
        if (sizeof(type) != write(fd, &name, sizeof(type))) {\
            VAR_PRINT(type, name);                           \
            return 0;                                        \
        }                                                    \
    } while (0);
#define VAR_READ(type, name) do {                            \
        if (sizeof(type) != read(fd, &name, sizeof(type))) { \
            VAR_PRINT(type, name);                           \
            return 0;                                        \
        }                                                    \
    } while (0);
#define VAR_PRINT(type, name)  \
    printVar(#name, sizeof(type), reinterpret_cast<char*>(&name));
#define VAR_SIZE(type, name)  size += sizeof(type);

/* example */
int ywsTest();

#endif  // INCLUDE_YWSERIALIZER_H_
