#ifndef NETWORKFS_INODE
#define NETWORKFS_INODE

#include <linux/fs.h>
#include <linux/types.h>

struct inode *networkfs_get_inode(struct super_block *, const struct inode *,
                                  umode_t, int);

int networkfs_mkdir(struct mnt_idmap *, struct inode *, struct dentry *,
                    umode_t);
int networkfs_rmdir(struct inode *, struct dentry *);

int networkfs_unlink(struct inode *, struct dentry *);
int networkfs_create(struct mnt_idmap *, struct inode *, struct dentry *,
                     umode_t, bool);
struct dentry *networkfs_lookup(struct inode *, struct dentry *, unsigned int);

int networkfs_setattr(struct mnt_idmap *, struct dentry *, struct iattr *);

int networkfs_link(struct dentry *, struct inode *, struct dentry *);

#endif
