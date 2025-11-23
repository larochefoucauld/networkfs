#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>

#include "operations/mount.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivanov Ivan");
MODULE_VERSION("0.01");

int networkfs_init(void);
void networkfs_exit(void);

struct file_system_type networkfs_fs_type = {
    .name = "networkfs",
    .init_fs_context = networkfs_init_fs_context,
    .kill_sb = networkfs_kill_sb};

int networkfs_init(void) { return register_filesystem(&networkfs_fs_type); }

void networkfs_exit(void) {
  int errcode = unregister_filesystem(&networkfs_fs_type);
  if (errcode != 0)
    printk(KERN_ERR
           "networkfs: unregister_filesystem() failed: error code %d\n",
           errcode);
}

module_init(networkfs_init);
module_exit(networkfs_exit);
