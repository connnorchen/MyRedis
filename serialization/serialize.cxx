#include <string>
#include <stdio.h>
#include "serialize.h"

void out_nil(std::string &out) {
    out.push_back(SER_NIL);
    printf("%s\n", out.c_str());
}

void out_str(std::string &out, const std::string &val) {
    printf("outing str\n");
    out.push_back(SER_STR);
    uint32_t len = (uint32_t)val.size();
    out.append((char *)&len, 4);
    out.append(val);
}

void out_int(std::string &out, int64_t val) {
    out.push_back(SER_INT);
    out.append((char *) &val, 8);
}

void out_err(std::string &out, int32_t code, const std::string &msg) {
    out.push_back(SER_ERR);
    out.append((char *) &code, 4);
    uint32_t len = (uint32_t)msg.size();
    out.append((char *)&len, 4);
    out.append(msg);
}

void out_arr(std::string &out, uint32_t n) {
    printf("outing arr\n");
    out.push_back(SER_ARR);
    out.append((char *)&n, 4);
}

