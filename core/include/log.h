#ifndef UNTITLED1_LOG_H
#define UNTITLED1_LOG_H

#ifdef DEBUG
#define log_d(format, ...) log_d_impl(format, ##__VA_ARGS__)
#else
#define log_d(format, ...)
#endif

#define log_w(format, ...) log_w_impl(format, ##__VA_ARGS__)
#define log_e(format, ...) log_e_impl(format, ##__VA_ARGS__)

void
log_d_impl(const char *format, ...);

void
log_w_impl(const char *format, ...);

void
log_e_impl(const char *format, ...);

#endif //UNTITLED1_LOG_H
