// #include "vtfs.h"
#include "ram_vtfs.h"

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
    struct inode* inode = vtfs_get_inode(sb, NULL, mode, ROOT_INODE);
    inode->i_op = &ram_vtfs_inode_ops;
    inode->i_fop = &ram_vtfs_dir_ops;
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

//inode operations

struct dentry* vtfs_lookup(
    struct inode* parent_inode,  // родительская нода
    struct dentry* child_dentry, // объект, к которому мы пытаемся получить доступ
    unsigned int flag            // неиспользуемое значение
){
  ino_t root = parent_inode->i_ino;
  const char *name = child_dentry->d_name.name;
  

  if (root != ROOT_INODE){
    return NULL;
  }
  if (!strcmp(name, "test.txt")) {
    struct inode *inode = vtfs_get_inode(parent_inode->i_sb, NULL, S_IFREG, 101);
    d_add(child_dentry, inode);
  } else if (!strcmp(name, "dir")) {
    struct inode *inode = vtfs_get_inode(parent_inode->i_sb, NULL, S_IFDIR, 200);
    d_add(child_dentry, inode);
  } else if (!strcmp(name, "new_file.txt")){
    struct inode *inode = vtfs_get_inode(parent_inode->i_sb, NULL, S_IFREG, 102);
    d_add(child_dentry, inode);
  }
  return NULL;
}

int vtfs_create(
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

  if (d_lookup(child_dentry, &child_dentry->d_name))
    return -EEXIST;

  if (root != ROOT_INODE)
    return 0;


  if (!strcmp(name, "test.txt")) {
    inode = vtfs_get_inode(
    parent_inode->i_sb, NULL, S_IFREG | S_IRWXUGO, 101);
    inode->i_op = &vtfs_inode_ops;
    inode->i_fop = NULL;
    d_add(child_dentry, inode);
    mask |= 1;
  } else if (!strcmp(name, "new_file.txt")) {
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
  if (root != ROOT_INODE){
    return 0;
  }
  if (!strcmp(name, "test.txt")) {
      mask &= ~1;
  } else if (!strcmp(name, "new_file.txt")) {
      mask &= ~2;
  }
  return 0;
}

//file operations

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
  return 1;
}


// ===================================
// RAM 
// ===================================

// inode operation
struct dentry* ram_vtfs_lookup(
    struct inode* parent_inode,  // родительская нода
    struct dentry* child_dentry, // объект, к которому мы пытаемся получить доступ
    unsigned int flag            // неиспользуемое значение
){
  const char *name = child_dentry->d_name.name;
  struct ram_vtfs_file *file;

  list_for_each_entry(file, &ram_vtfs_files, list){
    if (!strcmp(file->name, name)){
      struct inode *inode = vtfs_get_inode(parent_inode->i_sb, NULL, file->mode, file->ino); 
      if (inode)
        d_add(child_dentry, inode);
    }
  }
  return NULL;
}

// return 0 on success
int ram_vtfs_create(
  struct mnt_idmap *idmap,
  struct inode *parent_inode, 
  struct dentry *child_dentry, 
  umode_t mode, 
  bool b
) {
  printk(KERN_INFO "Creating file\n");
  const char *name = child_dentry->d_name.name;

  struct ram_vtfs_file *file;

  if (d_lookup(child_dentry, &child_dentry->d_name)){
    printk(KERN_ERR "ram_vtfs_create: file with this name already exists");
    printk(KERN_INFO "ram_vtfs_create: creating failed");
    return -EEXIST;
  }

  file = kmalloc(sizeof(*file), GFP_KERNEL);
  if (!file){
    printk(KERN_ERR "ram_vtfs_create: not enought memory\n");
    printk(KERN_INFO "ram_vtfs_create: reating failed\n");
    return -ENOMEM;
  }

  strncpy(file->name, name, NAME_MAX);
  file->ino = next_ino++;
  file->mode = mode;

  list_add(&file->list, &ram_vtfs_files);

  struct inode *inode = vtfs_get_inode(parent_inode->i_sb, NULL, mode | S_IRWXUGO, file->ino);
  inode->i_op = &ram_vtfs_inode_ops;
  inode->i_fop = &ram_vtfs_file_ops;
  
  d_add(child_dentry, inode);
  printk(KERN_INFO "Finish creating file\n");
  return 0;
}

int ram_vtfs_unlink(struct inode *parent_inode, struct dentry *child_dentry) {
  printk(KERN_INFO "Strart deletting file\n");
  const char *name = child_dentry->d_name.name;
  struct ram_vtfs_file *file, *tmp;

  list_for_each_entry_safe(file, tmp, &ram_vtfs_files, list){
    if (!strcmp(file->name, name)){
      list_del(&file->list);
      kfree(file);
      printk(KERN_INFO "Deletting successe\n");
      return 0;
    }
  }
  printk(KERN_ERR "vtfs_unlink:No entity\n");
  printk(KERN_INFO "Deletting failed\n");
  return -ENOENT;
}

int ram_vtfs_mkdir(
  struct mnt_idmap *idmap,
  struct inode *parent_inode,
  struct dentry *child_dentry,
  umode_t mode
){
  printk(KERN_INFO "Creating dir\n");
  if(!ram_vtfs_create(idmap, parent_inode, child_dentry, mode | S_IFDIR, true)){
    printk(KERN_ERR "ram_vtfs_mkdir: error\n");
    return -1;
  }

  const char *name = child_dentry->d_name.name;
  struct ram_vtfs_file *file;

  list_for_each_entry(file, &ram_vtfs_files, list){
    if (!strcmp(file->name, name)){
      struct inode *inode = vtfs_get_inode(parent_inode->i_sb, NULL, file->mode, file->ino); 
      if (inode)
        inode->i_op = &ram_vtfs_inode_ops;
        inode->i_fop = &ram_vtfs_dir_ops;
    }
  }
  printk(KERN_INFO "ram_vtfs_mkdir: creating done\n");
  return 0;
}


static int count_files(struct dir_context *ctx, const char *name, int namelen, loff_t offset, u64 ino, unsigned int d_type) {
    if (strcmp(name, ".") && strcmp(name, "..")) {
        return -1;
    }
    return 0;
}

// static int dir_empty(struct file *filp){
//   struct dir_context ctx = {
//     .actor = count_files,
//     .pos = 0,
//   };
//   if (iterate_dir(filp, &ctx) < 0){
//     return 0; // dir isn't empty
//   }
//   return 1; // dir empty
// }


int ram_vtfs_rmdir( struct inode *parent_inode, struct dentry *child_dentry ){
  printk(KERN_INFO "ram_vtfs_rmdir: strart\n");
  // проверка на наличие файлов в директории
  // if (!dir_empty(child_dentry)){
  if(!hlist_empty(&child_dentry->d_children)) {
    printk(KERN_ERR "ram_vtfs_rmdir: directory is not empty");
    return -1;
  }
  if(!ram_vtfs_unlink(parent_inode, child_dentry)){
    printk(KERN_ERR "ram_vtfs_rmdir: error in deleting");
    return -1;
  }
  printk(KERN_INFO "ram_vtfs_rmdir: finish\n");
  return 0;
}



//file operation

int ram_vtfs_iterate(struct file* filp, struct dir_context* ctx) {
  // char fsname[20];
  struct dentry* dentry = filp->f_path.dentry;
  struct inode* inode   = dentry->d_inode;
  struct ram_vtfs_file *file;
  unsigned long offset  = ctx->pos;

  // printk(KERN_INFO "Messege: f_pos = %lu\n", ctx->pos);

  if (inode->i_ino != ROOT_INODE) return 0; 

  if (offset == 0) {
    if (!dir_emit(ctx, ".", 1, inode->i_ino, DT_DIR)) 
      return -ENOMEM; 
    ctx->pos++;
  } 
  if (offset == 1) {
    if (!dir_emit(ctx, "..", 2, dentry->d_parent->d_inode->i_ino, DT_DIR)) 
      return -ENOMEM; 
    ctx->pos++;
  }

  if (offset > 1){
    int numb = 2; 
    list_for_each_entry(file, &ram_vtfs_files, list) {
      if (numb >= offset){
        if (!dir_emit(ctx, file->name, strlen(file->name), file->ino, file->mode & S_IFMT)) 
          return -ENOMEM; 
        ctx->pos++;
      }
      numb++;
    }
  }
  return 0;
}





module_init(vtfs_init);
module_exit(vtfs_exit);
