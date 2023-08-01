#include <vector>
#include <string>

enum {
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,
};


void do_keys(std::vector<std::string> &cmd, std::string &out);
void do_get(std::vector<std::string> &cmd, std::string &out);
void do_set(std::vector<std::string> &cmd, std::string &out);
void do_del(std::vector<std::string> &cmd, std::string &out);
