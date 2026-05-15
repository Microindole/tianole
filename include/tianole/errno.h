#ifndef TIANOLE_ERRNO_H
#define TIANOLE_ERRNO_H

/**
 * EINVAL - Invalid argument.
 */
#define EINVAL 22

/**
 * ENOENT - No such entry or mapping.
 */
#define ENOENT 2

/**
 * ENOMEM - Out of memory.
 */
#define ENOMEM 12

/**
 * EBUSY - Resource is already busy.
 */
#define EBUSY 16

/**
 * EEXIST - Object already exists.
 */
#define EEXIST 17

/**
 * EAGAIN - Resource is temporarily unavailable.
 */
#define EAGAIN 11

/**
 * ENOSPC - No space left in a fixed-size resource.
 */
#define ENOSPC 28

/**
 * ENOTDIR - Path component is not a directory.
 */
#define ENOTDIR 20

/**
 * EISDIR - Operation expected a regular file but found a directory.
 */
#define EISDIR 21

/**
 * ENAMETOOLONG - Path or component name exceeds the supported limit.
 */
#define ENAMETOOLONG 36

/**
 * ETIMEDOUT - Operation timed out.
 */
#define ETIMEDOUT 110

#endif
