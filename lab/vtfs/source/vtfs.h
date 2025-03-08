#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/fs.h>
#include <linux/stat.h>
struct dentry* vtfs_mount(struct file_system_type* fs_type, int flags, const char* token,void* data);
void vtfs_kill_sb(struct super_block* sb);
int vtfs_fill_super(struct super_block *sb, void *data, int silent);

struct inode* vtfs_get_inode(
  struct super_block* sb, 
  const struct inode* dir, 
  umode_t mode, 
  int i_ino
);

struct dentry* mount_nodev(
  struct file_system_type* fs_type,
  int flags, 
  void* data, 
  int (*fill_super)(struct super_block*, void*, int)
);


struct dentry* vtfs_lookup(
    struct inode* parent_inode,  // родительская нода
    struct dentry* child_dentry, // объект, к которому мы пытаемся получить доступ
    unsigned int flag            // неиспользуемое значение
);

int vtfs_iterate(struct file* filp, struct dir_context* ctx);







// structs

struct inode_operations vtfs_inode_ops = {
    .lookup = vtfs_lookup,
};

struct file_operations vtfs_dir_ops = {
    .iterate_shared = vtfs_iterate,
};