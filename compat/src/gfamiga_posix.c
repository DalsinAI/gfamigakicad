/*
 * The few POSIX calls libgit2 needs that libnix does not have.
 * pread/pwrite move the file position and put it back; nothing else in
 * KiCad shares a git pack descriptor between threads while it reads.
 * Copyright (c) 2026 AmiCradle Team
 */
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{
    off_t old = lseek(fd, 0, SEEK_CUR);
    ssize_t got;

    if (old == (off_t)-1 || lseek(fd, offset, SEEK_SET) == (off_t)-1)
        return -1;
    got = read(fd, buf, count);
    lseek(fd, old, SEEK_SET);
    return got;
}

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset)
{
    off_t old = lseek(fd, 0, SEEK_CUR);
    ssize_t put;

    if (old == (off_t)-1 || lseek(fd, offset, SEEK_SET) == (off_t)-1)
        return -1;
    put = write(fd, buf, count);
    lseek(fd, old, SEEK_SET);
    return put;
}

/* one session and one process group per task: name them by the task */
pid_t getpgid(pid_t pid)
{
    return pid ? pid : getpid();
}

pid_t getsid(pid_t pid)
{
    return pid ? pid : getpid();
}
