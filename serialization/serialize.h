#include <string>
enum {
    SER_NIL = 0,    // Like `NULL`
    SER_ERR = 1,    // An error code and message
    SER_STR = 2,    // A string
    SER_INT = 3,    // A int64
    SER_ARR = 4,    // Array
};


void out_nil(std::string &);
void out_str(std::string &, const std::string &);
void out_int(std::string &, int64_t);
void out_err(std::string &, int32_t, const std::string &);
void out_arr(std::string &, uint32_t);
