#include "operations/mount.h"

#include <linux/fs_context.h>

#include "networkfs.h"
#include "operations/inode.h"

int networkfs_fill_super(struct super_block *sb, struct fs_context *fc) {
  sb->s_maxbytes = NFS_MAXSZ;
  char *token = kzalloc(strlen(fc->source) + 1, GFP_KERNEL);
  if (token == NULL) {
    return -ENOMEM;
  }
  memcpy(token, fc->source, strlen(fc->source));
  sb->s_fs_info = token;

  struct inode *inode = networkfs_get_inode(sb, NULL, S_IFDIR, NFS_ROOT);
  if (inode == NULL) {
    return -ENOMEM;
  }
  sb->s_root = d_make_root(inode);

  if (sb->s_root == NULL) {
    return -ENOMEM;
  }

  return 0;
}

int networkfs_get_tree(struct fs_context *fc) {
  int ret = get_tree_nodev(fc, networkfs_fill_super);

  if (ret != 0) {
    printk(KERN_ERR "networkfs: unable to mount: error code %d\n", ret);
  } else {
    printk(KERN_INFO "networkfs: mounted\n");
  }

  return ret;
}

void networkfs_kill_sb(struct super_block *sb) {
  printk(KERN_INFO "networkfs: superblock is destroyed; token: %s\n",
         (char *)sb->s_fs_info);
  kfree(sb->s_fs_info);
}

// File system context

struct fs_context_operations networkfs_context_ops = {.get_tree =
                                                          networkfs_get_tree};

int networkfs_init_fs_context(struct fs_context *fc) {
  fc->ops = &networkfs_context_ops;
  return 0;
}
