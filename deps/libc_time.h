#ifndef LIBC_TIME_H_
#define LIBC_TIME_H_

struct musl_tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    /* int tm_isdst; */
    /* long __tm_gmtoff; */
    /* const char *__tm_zone; */
};

int musl_secs_to_tm(long long t, struct musl_tm *tm);
long long musl_tm_to_secs(const struct musl_tm *tm);

#endif /* LIBC_TIME_H_ */
