#ifndef MAIN_UTIL_H
#define MAIN_UTIL_H

/* from https://stackoverflow.com/questions/23999797/implementing-strnstr */
char *strnstr(const char *haystack, const char *needle, size_t len);

void
url_decode_me(char *me);

const char *
normalize_directory(char *dir, size_t dir_len);

#endif //MAIN_UTIL_H
