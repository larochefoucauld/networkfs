#ifndef NETWORKFS_REQUEST
#define NETWORKFS_REQUEST

#include <linux/fs.h>
#include <linux/types.h>

struct networkfs_dir_entries {
  size_t entries_count;
  struct networkfs_dir_entry {
    unsigned char entry_type;  // DT_DIR (4) or DT_REG (8)
    ino_t ino;
    char name[256];
  } entries[16];
};

struct networkfs_entry_info {
  unsigned char entry_type;  // DT_DIR (4) or DT_REG (8)
  ino_t ino;
};


int64_t networkfs_request_lookup(const struct inode *parent,
                                 const struct dentry *child,
                                 struct networkfs_entry_info *result);
int64_t networkfs_request_iterate(const struct file *filp,
                                  struct networkfs_dir_entries *result);

int64_t networkfs_request_unlink(const struct inode *parent,
                                 const struct dentry *child);

int64_t networkfs_request_create_generic(const struct inode *parent,
                                         const struct dentry *child,
                                         const char *type, ino_t *result);

int64_t networkfs_request_rmdir(const struct inode *parent,
                                const struct dentry *child);

int64_t networkfs_request_read(const struct inode *inode,
                               const struct file *filp, void *buffer,
                               size_t buffer_size);

int64_t networkfs_request_write(const struct file *filp, size_t size);

int64_t networkfs_request_link(struct dentry *target, struct inode *parent,
                               struct dentry *child);

#endif
