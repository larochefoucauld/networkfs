#ifndef NETWORKFS_FILE
#define NETWORKFS_FILE

#include <linux/fs.h>
#include <linux/types.h>

extern struct file_operations networkfs_dir_ops;

int networkfs_iterate(struct file *, struct dir_context *);

int networkfs_open(struct inode *, struct file *);
ssize_t networkfs_read(struct file *, char *, size_t, loff_t *);
ssize_t networkfs_write(struct file *, const char *, size_t, loff_t *);

int networkfs_flush(struct file *, fl_owner_t);
int networkfs_fsync(struct file *, loff_t, loff_t, int);
int networkfs_release(struct inode *, struct file *);

#endif
