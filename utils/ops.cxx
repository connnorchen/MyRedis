#include <string>
#include <map>
#include <vector>
#include "file_ops.h"
#include "ops.h"

std::map<std::string, std::string> g_map;

int32_t do_get(
    const std::vector<std::string> &cmd, uint8_t* res_buf,
    uint32_t *res_len) {
    if (!g_map.count(cmd[1])) {
        return RES_NX;
    }
    std::string &res = g_map[cmd[1]];
    printf("get str %s\n", cmd[1].c_str());
    assert (res.size() < K_MAX_LENGTH);
    memcpy(res_buf, res.data(), res.size());
    printf("value is %s\n", res.c_str());
    *res_len = (uint32_t) res.size();
    return RES_OK;
}

int32_t do_set(
    const std::vector<std::string> &cmd, uint8_t* res_buf,
    uint32_t *res_len) {
    (void) res_buf;
    (void) res_len;
    printf("set %s with %s\n", cmd[1].c_str(), cmd[2].c_str());
    g_map[cmd[1]] = cmd[2];
    return RES_OK;
}

int32_t do_del(
    const std::vector<std::string> &cmd, uint8_t* res_buf,
    uint32_t *res_len) {
    (void) res_buf;
    (void) res_len;
    g_map.erase(cmd[1]);
    return RES_OK;
}
