#include "operations/file.h"

#include <linux/dcache.h>
#include <linux/minmax.h>
#include <linux/stat.h>
#include <linux/uaccess.h>
#include <uapi/asm-generic/errno.h>

#include "networkfs.h"
#include "remote/request.h"
#include "util.h"

struct file_operations networkfs_dir_ops = {.iterate_shared = networkfs_iterate,
                                            .open = networkfs_open,
                                            .read = networkfs_read,
                                            .write = networkfs_write,
                                            .flush = networkfs_flush,
                                            .fsync = networkfs_fsync,
                                            .release = networkfs_release,
                                            .llseek = generic_file_llseek};

void networkfs_truncate(struct file *);

int networkfs_iterate(struct file *filp, struct dir_context *ctx) {
  struct dentry *dentry = filp->f_path.dentry;
  struct inode *inode = dentry->d_inode;

  loff_t record_counter = 0;
  if (ctx->pos == 0) {
    if (!dir_emit(ctx, ".", 1, inode->i_ino, DT_DIR)) {
      return record_counter;
    }
    ++ctx->pos;
    ++record_counter;
  }
  if (ctx->pos == 1) {
    struct inode *parent_inode = dentry->d_parent->d_inode;
    if (!dir_emit(ctx, "..", 2, parent_inode->i_ino, DT_DIR)) {
      return record_counter;
    }
    ++ctx->pos;
    ++record_counter;
  }

  // too big for stack allocation
  struct networkfs_dir_entries *dir =
      kzalloc(sizeof(struct networkfs_dir_entries), GFP_KERNEL);

  if (dir == NULL) {
    printk(KERN_ERR "networkfs: iterate: response buf alloc failed\n");
    return -ENOMEM;
  }
  int error = networkfs_request_iterate(filp, dir);
  if (error < 0) {
    kfree(dir);
    return error;
  }

  size_t n_entries = dir->entries_count;
  while (ctx->pos - 2 < n_entries) {
    struct networkfs_dir_entry *entry = &dir->entries[ctx->pos - 2];
    if (entry->entry_type == 0) {
      break;
    }
    if (!dir_emit(ctx, wstr(entry->name), entry->ino, entry->entry_type)) {
      kfree(dir);
      return record_counter;
    }
    ++ctx->pos;
    ++record_counter;
  }
  kfree(dir);
  return record_counter;
}

void networkfs_truncate(struct file *filp) {
  char *file_content = filp->private_data;
  loff_t file_size = filp->f_inode->i_size;
  memset(file_content + file_size, 0, NFS_MAXSZ - file_size);
}

int networkfs_open(struct inode *inode, struct file *filp) {
  int error;

  if (S_ISDIR(inode->i_mode)) {
    return 0;
  }
  size_t buf_size = NFS_MAXSZ + sizeof(u64);
  void *buf = kmalloc(buf_size, GFP_KERNEL);
  if (buf == NULL) {
    printk(KERN_ERR "networkfs: open: response buf alloc failed\n");
    return -ENOMEM;
  }
  error = networkfs_request_read(inode, filp, buf, buf_size);
  if (error < 0) {
    kfree(buf);
    return error;
  }

  void *file_content = buf + sizeof(u64);
  filp->private_data = file_content;
  inode->i_size = *(u64 *)(buf);
  if ((filp->f_flags & O_APPEND) == O_APPEND) {
    generic_file_llseek(filp, 0, SEEK_END);
  }

  return 0;
}

ssize_t networkfs_read(struct file *filp, char *buffer, size_t len,
                       loff_t *offset) {
  if (*offset < 0) {
    printk(KERN_ERR "networkfs: read: negative offset\n");
    return -EIO;
  }
  size_t from = *offset;
  char *file_content = filp->private_data;
  loff_t file_size = filp->f_inode->i_size;

  if (from > file_size) {
    printk(KERN_ERR
           "networkfs: read: invalid offset (%ld) in file %lu of size %lld\n",
           from, filp->f_inode->i_ino, file_size);
    return -EIO;
  }
  len = min(len, file_size - from);
  size_t copied = len - copy_to_user(buffer, file_content + from, len);
  *offset += copied;
  return copied;
}

ssize_t networkfs_write(struct file *filp, const char *buffer, size_t len,
                        loff_t *offset) {
  if (*offset < 0) {
    printk(KERN_ERR "networkfs: write: negative offset\n");
    return -EIO;
  }
  size_t from = *offset;
  char *file_content = filp->private_data;

  if (from >= NFS_MAXSZ) {
    printk(KERN_ERR "networkfs: write: quota exceeded (offset = %lu)\n", from);
    return -EDQUOT;
  }
  len = min(len, NFS_MAXSZ - from);
  size_t copied = len - copy_from_user(file_content + from, buffer, len);
  size_t file_size = filp->f_inode->i_size;
  file_size = max(file_size, from + copied);
  filp->f_inode->i_size = file_size;
  *offset += copied;
  return copied;
}

int networkfs_flush(struct file *filp, fl_owner_t id) {
  if (S_ISDIR(filp->f_inode->i_mode)) {
    return 0;
  }
  return networkfs_request_write(filp, filp->f_inode->i_size);
}

int networkfs_fsync(struct file *filp, loff_t begin, loff_t end, int datasync) {
  if (S_ISDIR(filp->f_inode->i_mode)) {
    return 0;
  }
  return networkfs_request_write(filp, filp->f_inode->i_size);
}

int networkfs_release(struct inode *inode, struct file *filp) {
  if (S_ISDIR(filp->f_inode->i_mode)) {
    return 0;
  }
  kfree(filp->private_data - sizeof(u64));

  return 0;
}
