#include "vtfs.h"

#define MODULE_NAME "vtfs"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("secs-dev & adik-alone");
MODULE_DESCRIPTION("A simple FS kernel module");

#define LOG(fmt, ...) pr_info("[" MODULE_NAME "]: " fmt, ##__VA_ARGS__)


struct file_system_type vtfs_fs_type = {
  .name = "vtfs",
  .mount = vtfs_mount,
  .kill_sb = vtfs_kill_sb,
};

struct inode* vtfs_get_inode(
  struct super_block* sb, 
  const struct inode* dir, 
  umode_t mode, 
  int i_ino
){
  struct inode *inode = new_inode(sb);
  if (inode != NULL) {
    inode_init_owner(&nop_mnt_idmap, inode, dir, mode);
  }
  inode->i_ino = i_ino;
  return inode;
}

struct dentry* vtfs_mount(
    struct file_system_type* fs_type,
    int flags,
    const char* token,
    void* data
){
      struct dentry* ret = mount_nodev(fs_type, flags, data, vtfs_fill_super);
      if (ret == NULL) {
        printk(KERN_ERR "Can't mount file system");
      } else {
        printk(KERN_INFO "Mounted successfuly");
      }
      return ret;
}
int vtfs_fill_super(struct super_block *sb, void *data, int silent) {
    umode_t mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
    struct inode* inode = vtfs_get_inode(sb, NULL, mode, 1000);
    inode->i_op = &vtfs_inode_ops;
    inode->i_fop = &vtfs_dir_ops;
    sb->s_root = d_make_root(inode);
    if (sb->s_root == NULL) {
      return -ENOMEM;
    }
    printk(KERN_INFO "return 0\n");
    return 0;
}

void vtfs_kill_sb(struct super_block* sb) {
    printk(KERN_INFO "vtfs super block is destroyed. Unmount successfully.\n");
}


static int __init vtfs_init(void) {
  int reg = register_filesystem(&vtfs_fs_type);
  if (reg != 0){
    LOG("FAIL. VTFS did not register");
    return reg;
  }
  LOG("VTFS joined the kernel\n");
  return 0;
}

static void __exit vtfs_exit(void) {
  int unreg = unregister_filesystem(&vtfs_fs_type);
  if (unreg != 0){
    LOG("FAIL. VTFS did not unregiser");
    return;
  }
  LOG("VTFS left the kernel\n");
}


struct dentry* vtfs_lookup(
    struct inode* parent_inode,  // родительская нода
    struct dentry* child_dentry, // объект, к которому мы пытаемся получить доступ
    unsigned int flag            // неиспользуемое значение
){
  return NULL;
}


int vtfs_iterate(struct file* filp, struct dir_context* ctx) {
  char fsname[10];
  struct dentry* dentry = filp->f_path.dentry;
  struct inode* inode   = dentry->d_inode;
  unsigned long offset  = filp->f_pos;
  int stored            = 0;
  ino_t ino             = inode->i_ino;
  unsigned char ftype;
  ino_t dino;
  while (true) {
    if (ino == 100) {
      if (offset == 0) {
        strcpy(fsname, ".");
        ftype = DT_DIR;
        dino = ino;
      } else if (offset == 1) {
        strcpy(fsname, "..");
        ftype = DT_DIR;
        dino = dentry->d_parent->d_inode->i_ino;
      } else if (offset == 2) {
        strcpy(fsname, "test.txt");
        ftype = DT_REG;
        dino = 100;
      } else {
        return stored;
      }
    }
  }
}







module_init(vtfs_init);
module_exit(vtfs_exit);
