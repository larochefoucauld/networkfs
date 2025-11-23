#ifndef NETWORKFS_MOUNT
#define NETWORKFS_MOUNT

#include <linux/fs.h>
#include <linux/types.h>

void networkfs_kill_sb(struct super_block *);
int networkfs_fill_super(struct super_block *, struct fs_context *);
int networkfs_get_tree(struct fs_context *);

int networkfs_init_fs_context(struct fs_context *);

#endif
