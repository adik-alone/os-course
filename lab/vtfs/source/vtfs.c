#include "vtfs.h"

#define MODULE_NAME "vtfs"
#define ROOT_INODE 1000

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
    struct inode* inode = vtfs_get_inode(sb, NULL, mode, ROOT_INODE);
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
  ino_t root = parent_inode->i_ino;
  const char *name = child_dentry->d_name.name;
  if (root == ROOT_INODE && !strcmp(name, "test.txt")) {
    struct inode *inode = vtfs_get_inode(parent_inode->i_sb, NULL, S_IFREG, 101);
    d_add(child_dentry, inode);
  } else if (root == ROOT_INODE && !strcmp(name, "dir")) {
    struct inode *inode = vtfs_get_inode(parent_inode->i_sb, NULL, S_IFDIR, 200);
    d_add(child_dentry, inode);
  } else if (root == ROOT_INODE && !strcmp(name, "new_file.txt")){
    struct inode *inode = vtfs_get_inode(parent_inode->i_sb, NULL, S_IFREG, 102);
    d_add(child_dentry, inode);
  }
  return NULL;
}

int vtfs_create(
  // struct user_namespase *usr_ns,
  struct mnt_idmap *idmap,
  struct inode *parent_inode, 
  struct dentry *child_dentry, 
  umode_t mode, 
  bool b
) {
  int mask;
  ino_t root = parent_inode->i_ino;
  const char *name = child_dentry->d_name.name;
  struct inode *inode = NULL;
  if (root == ROOT_INODE && !strcmp(name, "test.txt")) {
    inode = vtfs_get_inode(
    parent_inode->i_sb, NULL, S_IFREG | S_IRWXUGO, 101);
    inode->i_op = &vtfs_inode_ops;
    inode->i_fop = NULL;
    d_add(child_dentry, inode);
    mask |= 1;
  } else if (root == ROOT_INODE && !strcmp(name, "new_file.txt")) {
    inode = vtfs_get_inode(
    parent_inode->i_sb, NULL, S_IFREG | S_IRWXUGO, 102);
    inode->i_op = &vtfs_inode_ops;
    inode->i_fop = NULL;
    d_add(child_dentry, inode);
    mask |= 2;
  }
  return 0;
}

int vtfs_unlink(struct inode *parent_inode, struct dentry *child_dentry) {
  const char *name = child_dentry->d_name.name;
  ino_t root = parent_inode->i_ino;
  int mask;
  if (root == ROOT_INODE && !strcmp(name, "test.txt")) {
      mask &= ~1;
  } else if (root == ROOT_INODE && !strcmp(name, "new_file.txt")) {
      mask &= ~2;
  }
  return 0;
}



int vtfs_iterate(struct file* filp, struct dir_context* ctx) {
  char fsname[20];
  struct dentry* dentry = filp->f_path.dentry;
  struct inode* inode   = dentry->d_inode;
  unsigned long offset  = ctx->pos;
  ino_t ino             = inode->i_ino;
  unsigned char ftype;
  ino_t dino;

  printk(KERN_INFO "Messege: f_pos = %lu\n", ctx->pos);

  if (ino != ROOT_INODE) return 0; 

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
    dino = 101;
  } else if (offset == 3) { 
    strcpy(fsname, "new_file.txt");
    ftype = DT_REG;
    dino = 102;
  }else {
    return 0; 
  }
  

  if (!dir_emit(ctx, fsname, strlen(fsname), dino, ftype))
    return -ENOMEM; 

  ctx->pos++; 
  // printk(KERN_INFO "2: f_pos = %lu\n", ctx->pos);
  return 1;
}






module_init(vtfs_init);
module_exit(vtfs_exit);
