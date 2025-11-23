#include "remote/request.h"

#include "remote/http.h"
#include "util.h"

static int64_t handle_error(int64_t error_code) {
  if (error_code < 0) {
    printk(KERN_ERR "networkfs: http session failed, error code: %llx\n",
           -error_code);
    if (((-error_code) & ESOCKNOCREATE) != 0) {  // some http session error
      return -EIO;  // generic error for communication failure
    }
  }
  return error_code;  // it is an error from errno-base or response status
}

int64_t networkfs_request_lookup(const struct inode *parent,
                                 const struct dentry *child,
                                 struct networkfs_entry_info *result) {
  const char *token = parent->i_sb->s_fs_info;
  const char *name = child->d_name.name;
  ino_to_string(parent_ino_str, parent->i_ino);
  int64_t http_status = networkfs_http_call(
      token, "lookup", (char *)result, sizeof(struct networkfs_entry_info), 2,
      "parent", parent_ino_str, strlen(parent_ino_str), "name", wstr(name));

  if ((http_status = handle_error(http_status)) < 0) {
    return http_status;
  }

  if (http_status == 1) {
    printk(KERN_ERR
           "networkfs: request_lookup: parent inode %ld not found on server\n",
           parent->i_ino);
    return -ENOENT;
  }

  if (http_status == 3) {
    printk(KERN_ERR
           "networkfs: request_lookup: parent inode %ld is not a directory\n",
           parent->i_ino);
    return -ENOTDIR;
  }

  if (http_status == 4) {
    printk(
        KERN_ERR
        "networkfs: request_lookup: no entry with name %s in directory %ld\n",
        name, parent->i_ino);
    return -ENOENT;
  }

  if (http_status != 0) {
    printk(KERN_ERR
           "networkfs: request_lookup: server returned unknown error %lld\n",
           http_status);
    return -EIO;
  }

  return 0;
}

int64_t networkfs_request_iterate(const struct file *filp,
                                  struct networkfs_dir_entries *result) {
  const struct dentry *dentry = filp->f_path.dentry;
  const struct inode *inode = dentry->d_inode;

  const char *token = inode->i_sb->s_fs_info;
  ino_to_string(ino_str, inode->i_ino);
  int64_t http_status = networkfs_http_call(
      token, "list", (char *)result, sizeof(struct networkfs_dir_entries), 1,
      "inode", wstr(ino_str));

  if ((http_status = handle_error(http_status)) < 0) {
    return http_status;
  }

  if (http_status == 1) {
    printk(KERN_ERR
           "networkfs: request_iterate: inode %ld not found on server\n",
           inode->i_ino);
    return -ENOENT;
  }

  if (http_status == 3) {
    printk(KERN_ERR
           "networkfs: request_iterate: inode %ld is not a directory\n",
           inode->i_ino);
    return -ENOTDIR;
  }

  if (http_status != 0) {
    printk(KERN_ERR
           "networkfs: request_iterate: server returned unknown error %lld\n",
           http_status);
    return -EIO;
  }

  return 0;
}

int64_t networkfs_request_unlink(const struct inode *parent,
                                 const struct dentry *child) {
  const char *token = parent->i_sb->s_fs_info;
  const char *name = child->d_name.name;
  ino_to_string(parent_ino_str, parent->i_ino);
  int64_t http_status =
      networkfs_http_call(token, "unlink", NULL, 0, 2, "parent",
                          wstr(parent_ino_str), "name", wstr(name));

  if ((http_status = handle_error(http_status)) < 0) {
    return http_status;
  }

  if (http_status == 1) {
    printk(KERN_ERR
           "networkfs: request_unlink: inode %ld not found on server\n",
           parent->i_ino);
    return -ENOENT;
  }

  if (http_status == 2) {
    printk(KERN_ERR "networkfs: request_unlink: \"%s\" is not a file\n", name);
    return -ENOENT;
  }

  if (http_status == 3) {
    printk(KERN_ERR "networkfs: request_unlink: inode %ld is not a directory\n",
           parent->i_ino);
    return -ENOTDIR;
  }

  if (http_status == 4) {
    printk(
        KERN_ERR
        "networkfs: request_unlink: no entry \"%s\" found in directory %ld\n",
        name, parent->i_ino);
    return -ENOENT;
  }

  if (http_status != 0) {
    printk(KERN_ERR
           "networkfs: request_unlink: server returned unknown error %lld\n",
           http_status);
    return -EIO;
  }

  return 0;
}

int64_t networkfs_request_create_generic(const struct inode *parent,
                                         const struct dentry *child,
                                         const char *type, ino_t *result) {
  const char *token = parent->i_sb->s_fs_info;
  const char *name = child->d_name.name;
  ino_to_string(parent_ino_str, parent->i_ino);
  int64_t http_status = networkfs_http_call(
      token, "create", (char *)result, sizeof(ino_t), 3, "parent",
      wstr(parent_ino_str), "name", wstr(name), "type", wstr(type));

  if ((http_status = handle_error(http_status)) < 0) {
    return http_status;
  }

  if (http_status == 1) {
    printk(KERN_ERR
           "networkfs: request_create_generic: inode %ld not found on server\n",
           parent->i_ino);
    return -ENOENT;
  }

  if (http_status == 3) {
    printk(KERN_ERR
           "networkfs: request_create_generic: inode %ld is not a directory\n",
           parent->i_ino);
    return -ENOTDIR;
  }

  if (http_status == 5) {
    printk(KERN_ERR
           "networkfs: request_create_generic: entry \"%s\" already exists in "
           "directory %ld\n",
           name, parent->i_ino);
    return -EEXIST;
  }

  if (http_status == 7) {
    printk(KERN_ERR
           "networkfs: request_create_generic: directory size limit exceeded "
           "(directory %ld)\n",
           parent->i_ino);
    return -ENOSPC;
  }

  if (http_status != 0) {
    printk(KERN_ERR
           "networkfs: request_create_generic: server returned unknown error "
           "%lld\n",
           http_status);
    return -EIO;
  }

  return 0;
}

int64_t networkfs_request_rmdir(const struct inode *parent,
                                const struct dentry *child) {
  const char *token = parent->i_sb->s_fs_info;
  const char *name = child->d_name.name;
  ino_to_string(parent_ino_str, parent->i_ino);
  int64_t http_status =
      networkfs_http_call(token, "rmdir", NULL, 0, 2, "parent",
                          wstr(parent_ino_str), "name", wstr(name));

  if ((http_status = handle_error(http_status)) < 0) {
    return http_status;
  }

  if (http_status == 1) {
    printk(KERN_ERR "networkfs: request_rmdir: inode %ld not found on server\n",
           parent->i_ino);
    return -ENOENT;
  }

  if (http_status == 3) {
    printk(KERN_ERR "networkfs: request_rmdir: inode %ld is not a directory\n",
           parent->i_ino);
    return -ENOTDIR;
  }

  if (http_status == 4) {
    printk(KERN_ERR
           "networkfs: request_rmdir: no entry \"%s\" found in directory %ld\n",
           name, parent->i_ino);
    return -ENOENT;
  }

  if (http_status == 8) {
    printk(KERN_ERR "networkfs: request_rmdir: directory \"%s\" is not empty\n",
           name);
    return -EPERM;
  }

  if (http_status != 0) {
    printk(KERN_ERR
           "networkfs: request_rmdir: server returned unknown error %lld\n",
           http_status);
    return -EIO;
  }

  return 0;
}

int64_t networkfs_request_read(const struct inode *inode,
                               const struct file *filp, void *buffer,
                               size_t buffer_size) {
  const char *token = inode->i_sb->s_fs_info;
  ino_to_string(ino_str, inode->i_ino);
  int64_t http_status = networkfs_http_call(token, "read", buffer, buffer_size,
                                            1, "inode", wstr(ino_str));

  if ((http_status = handle_error(http_status)) < 0) {
    return http_status;
  }

  if (http_status == 1) {
    printk(KERN_ERR "networkfs: request_read: inode %ld not found on server\n",
           inode->i_ino);
    return -ENOENT;
  }
  if (http_status == 2) {
    printk(KERN_ERR "networkfs: request_read: \"%s\" is not a file\n",
           filp->f_path.dentry->d_name.name);
    return -ENOENT;
  }

  if (http_status != 0) {
    printk(KERN_ERR
           "networkfs: request_read: server returned unknown error %lld\n",
           http_status);
    return -EIO;
  }

  return 0;
}

int64_t networkfs_request_write(const struct file *filp, size_t size) {
  const char *content = filp->private_data;
  const char *token = filp->f_inode->i_sb->s_fs_info;
  ino_to_string(ino_str, filp->f_inode->i_ino);
  int64_t http_status =
      networkfs_http_call(token, "write", NULL, 0, 2, "inode", wstr(ino_str),
                          "content", content, size);

  if ((http_status = handle_error(http_status)) < 0) {
    return http_status;
  }

  if (http_status == 1) {
    printk(KERN_ERR "networkfs: request_write: inode %ld not found on server\n",
           filp->f_inode->i_ino);
    return -ENOENT;
  }

  if (http_status == 2) {
    printk(KERN_ERR "networkfs: request_write: \"%s\" is not a file\n",
           filp->f_path.dentry->d_name.name);
    return -ENOENT;
  }

  if (http_status == 6) {
    printk(KERN_ERR
           "networkfs: request_write: file size limit exceeded (requested %ld "
           "bytes)\n",
           size);
    return -ENOSPC;
  }

  if (http_status != 0) {
    printk(KERN_ERR
           "networkfs: request_write: server returned unknown error %lld\n",
           http_status);
    return -EIO;
  }

  return 0;
}

int64_t networkfs_request_link(struct dentry *target, struct inode *parent,
                               struct dentry *child) {
  const char *token = target->d_inode->i_sb->s_fs_info;
  const char *name = child->d_name.name;
  ino_to_string(target_ino_str, target->d_inode->i_ino);
  ino_to_string(par_ino_str, parent->i_ino);
  int64_t http_status = networkfs_http_call(
      token, "link", NULL, 0, 3, "source", wstr(target_ino_str), "parent",
      wstr(par_ino_str), "name", wstr(name));

  if ((http_status = handle_error(http_status)) < 0) {
    return http_status;
  }

  if (http_status == 1) {
    printk(KERN_ERR "networkfs: request_link: inode not found on server\n");
    return -ENOENT;
  }

  if (http_status == 2) {
    printk(KERN_ERR "networkfs: request_link: \"%s\" is not a file\n",
           target->d_name.name);
    return -ENOENT;
  }

  if (http_status == 3) {
    printk(KERN_ERR "networkfs: request_link: inode %ld is not a directory\n",
           parent->i_ino);
    return -ENOTDIR;
  }

  if (http_status == 5) {
    printk(KERN_ERR
           "networkfs: request_link: entry \"%s\" already exists in directory "
           "%ld\n",
           child->d_name.name, parent->i_ino);
    return -EEXIST;
  }

  if (http_status == 7) {
    printk(KERN_ERR
           "networkfs: request_link: directory size limit exceeded (directory "
           "%ld)\n",
           parent->i_ino);
    return -ENOSPC;
  }

  if (http_status != 0) {
    printk(KERN_ERR
           "networkfs: request_link: server returned unknown error %lld\n",
           http_status);
    return -EIO;
  }

  return 0;
}
