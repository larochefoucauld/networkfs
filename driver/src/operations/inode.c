#include "operations/inode.h"

#include <linux/dcache.h>
#include <linux/stat.h>

#include "networkfs.h"
#include "operations/file.h"
#include "remote/request.h"
#include "util.h"

struct inode_operations networkfs_inode_ops = {.lookup = networkfs_lookup,
                                               .create = networkfs_create,
                                               .unlink = networkfs_unlink,
                                               .mkdir = networkfs_mkdir,
                                               .rmdir = networkfs_rmdir,
                                               .setattr = networkfs_setattr,
                                               .link = networkfs_link};

struct inode *networkfs_get_inode(struct super_block *sb,
                                  const struct inode *parent, umode_t mode,
                                  int i_ino) {
  struct inode *inode;
  inode = new_inode(sb);

  if (inode != NULL) {
    inode->i_op = &networkfs_inode_ops;
    inode->i_fop = &networkfs_dir_ops;
    inode->i_ino = i_ino;
    inode_init_owner(&nop_mnt_idmap, inode, parent, mode | NFS_PERM);
  }

  return inode;
}

int networkfs_mkdir(struct mnt_idmap *idmap, struct inode *parent,
                    struct dentry *child, umode_t mode) {
  ino_t new_ino;
  int error =
      networkfs_request_create_generic(parent, child, "directory", &new_ino);
  if (error < 0) {
    return error;
  }

  struct inode *inode =
      networkfs_get_inode(parent->i_sb, parent, S_IFDIR | mode, new_ino);
  if (inode == NULL) {
    return -ENOMEM;
  }
  d_add(child, inode);
  return 0;
}

int networkfs_rmdir(struct inode *parent, struct dentry *child) {
  return networkfs_request_rmdir(parent, child);
}

int networkfs_unlink(struct inode *parent, struct dentry *child) {
  return networkfs_request_unlink(parent, child);
}

int networkfs_create(struct mnt_idmap *idmap, struct inode *parent,
                     struct dentry *child, umode_t mode, bool b) {
  ino_t new_ino;
  int error = networkfs_request_create_generic(parent, child, "file", &new_ino);
  if (error < 0) {
    return error;
  }

  struct inode *inode =
      networkfs_get_inode(parent->i_sb, parent, S_IFREG | mode, new_ino);
  if (inode == NULL) {
    printk(KERN_ERR "networkfs: create: inode alloc failed\n");
    return -ENOMEM;
  }
  d_add(child, inode);
  return 0;
}

struct dentry *networkfs_lookup(struct inode *parent, struct dentry *child,
                                unsigned int flag) {
  struct networkfs_entry_info entry;
  int error = networkfs_request_lookup(parent, child, &entry);
  if (error < 0) {
    return NULL;
  }
  umode_t mode = (entry.entry_type == DT_DIR) ? S_IFDIR : S_IFREG;
  struct inode *inode =
      networkfs_get_inode(parent->i_sb, parent, mode, entry.ino);
  if (inode == NULL) {
    printk(KERN_ERR "networkfs: lookup: inode alloc failed\n");
    return NULL;
  }
  d_add(child, inode);
  return NULL;
}

int networkfs_setattr(struct mnt_idmap *idmap, struct dentry *entry,
                      struct iattr *attr) {
  int error = setattr_prepare(idmap, entry, attr);
  if (error < 0) {
    return error;
  }
  if (attr->ia_valid & ATTR_OPEN) {
    entry->d_inode->i_size = attr->ia_size;
  }

  return 0;
}

int networkfs_link(struct dentry *target, struct inode *parent,
                   struct dentry *child) {
  return networkfs_request_link(target, parent, child);
}
