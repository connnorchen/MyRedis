#include <string.h>

#define K_MAX_LENGTH 4096

void msg(const char*);
void fd_set_nb(int);
int32_t read_full(int, char*, size_t);
int32_t write_all(int, char*, size_t);
int32_t read_length_from_socket(int, char buf[]);
int32_t read_content_from_socket(int, char[], size_t);
