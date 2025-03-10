#include "ram_vtfs.h"

struct inode* vtfs_get_inode(
  struct super_block* sb, 
  const struct inode* dir, 
  umode_t mode, 
  int i_ino
){
  struct inode *inode = new_inode(sb);
  if (inode == NULL) {
    return NULL;
  }
  inode_init_owner(&nop_mnt_idmap, inode, dir, mode);
  inode->i_ino = i_ino;
  inode->i_sb = sb;
  if (S_ISDIR(mode)){
    struct ram_vtfs_dir_list *dir_list = kmalloc(sizeof(struct ram_vtfs_dir_list), GFP_KERNEL);
    if (!dir_list){
      printk(KERN_ERR "vtfs_get_inode: dir list isn't kmalloc");
      return NULL;
    }
    INIT_LIST_HEAD(&dir_list->children);
    inode->i_private = dir_list;

  }else{
    inode->i_private = NULL;
  }
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
  struct ram_vtfs_dir_list *parent_list = parent_inode->i_private;
  struct ram_vtfs_file *file;

  list_for_each_entry(file, &parent_list->children, list){
    if (!strcmp(file->name, name)){
      // struct inode *inode = vtfs_get_inode(parent_inode->i_sb, parent_inode, file->mode, file->ino); 
      struct inode *inode = file->inode;
      if (inode)
        d_add(child_dentry, inode);
        return NULL;
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
  struct ram_vtfs_dir_list *parent_list = parent_inode->i_private;

  if (!parent_list){
    printk(KERN_ERR "ram_vtfs_create: isn't parent list\n");
    return -ENOENT;
  }

  if (d_lookup(child_dentry, &child_dentry->d_name)){
    printk(KERN_ERR "ram_vtfs_create: file with this name already exists\n");
    return -EEXIST;
  }

  file = kmalloc(sizeof(struct ram_vtfs_file), GFP_KERNEL);
  if (!file){
    printk(KERN_ERR "ram_vtfs_create: not enought memory\n");
    printk(KERN_INFO "ram_vtfs_create: reating failed\n");
    return -ENOMEM;
  }

  strncpy(file->name, name, NAME_MAX);
  file->mode = mode;

  list_add(&file->list, &parent_list->children);

  next_ino++;
  struct inode *inode = vtfs_get_inode(parent_inode->i_sb, parent_inode, mode | S_IRWXUGO, next_ino);
  file->inode = inode;
  inode->i_op = &ram_vtfs_inode_ops;
  inode->i_fop = &ram_vtfs_file_ops;
  inode->i_private = kmalloc(FILE_MAX_SIZE, GFP_KERNEL);
  
  
  d_add(child_dentry, inode);
  printk(KERN_INFO "Finish creating file\n");
  return 0;
}

int ram_vtfs_unlink(struct inode *parent_inode, struct dentry *child_dentry) {
  printk(KERN_INFO "Strart deletting file\n");
  const char *name = child_dentry->d_name.name;
  struct ram_vtfs_file *file, *tmp;
  struct ram_vtfs_dir_list *parent_list = parent_inode->i_private;

  list_for_each_entry_safe(file, tmp, &parent_list->children, list){
    if (!strcmp(file->name, name)){
      list_del(&file->list);
      struct inode *inode = file->inode;
      // inode->i_nlink--;
      drop_nlink(inode);
      if (inode->i_nlink == 0){
        if (!inode->i_private){
          kfree(inode->i_private);
        }
        kfree(file);
        printk(KERN_INFO "Deletting successe\n");
        return 0;
      }
    d_drop(child_dentry);
    return 0;
    }
  }
  printk(KERN_ERR "vtfs_unlink:No entity\n");
  printk(KERN_INFO "Deletting failed\n");
  return -ENOENT;
}

// 
// 
// 
int ram_vtfs_mkdir(
  struct mnt_idmap *idmap,
  struct inode *parent_inode,
  struct dentry *child_dentry,
  umode_t mode
){
  printk(KERN_INFO "vtfs_mkdir: Creating dir\n");
  const char *name = child_dentry->d_name.name;
  struct ram_vtfs_file *dir;
  struct ram_vtfs_dir_list *parent_list = parent_inode->i_private;

  if (!parent_list){
    printk(KERN_ERR "mkdir: isn't parent list\n");
    return -ENOENT;
  }

  // проверка на наличите файлов с таким же именем
  if (d_lookup(child_dentry, &child_dentry->d_name)){
    printk(KERN_ERR "ram_vtfs_mkdir: file with this name already exists\n");
    return -EEXIST;
  }
  dir = kmalloc(sizeof(struct ram_vtfs_file), GFP_KERNEL);
  if (!dir){
    printk(KERN_ERR "ram_vtfs_mkdir: no memory\n");
    return -ENOMEM;
  }

  strcpy(dir->name, name);
  // dir->ino = next_ino++;
  dir->mode = mode;
  next_ino++;

  struct inode *inode = vtfs_get_inode(parent_inode->i_sb, parent_inode, S_IFDIR | mode, next_ino);

  if(!inode){
    kfree(dir);
    printk(KERN_ERR "ram_vtfs_mkdir: no memory\n");
    return -ENOMEM;
  }

  dir->inode = inode;

  inode->i_op = &ram_vtfs_inode_ops;
  inode->i_fop = & ram_vtfs_dir_ops;
  // inode->i_private = dir_list;

  //добавляем в лист парента
  list_add(&dir->list, &parent_list->children);

  d_add(child_dentry, inode);

  printk(KERN_INFO "ram_vtfs_mkdir: creating done\n");
  return 0;
}

int ram_vtfs_rmdir( struct inode *parent_inode, struct dentry *child_dentry ){
  printk(KERN_INFO "ram_vtfs_rmdir: strart\n");

  const char *name = child_dentry->d_name.name;
  struct ram_vtfs_dir_list *parent_dir_list = parent_inode->i_private;
  struct ram_vtfs_file *dir, *tmp;

  // проверить пуста ли директория

  list_for_each_entry_safe(dir, tmp, &parent_dir_list->children, list){
    if (!strcmp(dir->name, name)){
      // struct inode *inode = vtfs_get_inode(parent_inode->i_sb, NULL, dir->mode, dir->ino);
      struct inode *inode = dir->inode;
      struct ram_vtfs_dir_list *dir_list = inode->i_private;
      if (!list_empty(&dir_list->children)){
        printk(KERN_ERR "ram_vtfs_rmdir: dir isn't empty\n");
        return -ENOTEMPTY;
      }
      list_del(&dir->list);
      kfree(dir);
      printk(KERN_INFO "ram_vtfs_rmdir: successe\n");
      return 0;
    }
  }
  printk(KERN_ERR "ram_vtfs_rmdir: dir didn't found\n");
  return -ENOENT;

}

int ram_vtfs_link(
  struct dentry *old_dentry,
  struct inode *parent_inode,
  struct dentry *new_dentry
) {
  struct inode *inode = d_inode(old_dentry);

  if (!S_ISREG(inode->i_mode)){
    printk(KERN_ERR "ram_vtfs_link: only for regular files\n");
    return -EPERM;
  }

  ihold(inode);
  d_instantiate(new_dentry, inode);
  return 0;
}

//file operation

int ram_vtfs_iterate(struct file* filp, struct dir_context* ctx) {
  // char fsname[20];
  struct dentry* dentry = filp->f_path.dentry;
  struct inode* inode   = dentry->d_inode;
  struct ram_vtfs_file *file;
  unsigned long offset  = ctx->pos;

  struct ram_vtfs_dir_list *dir_list = inode->i_private;
  if (!dir_list){
    printk(KERN_ERR "vtfs_iterate: not a dir\n");
    return -ENOENT;
  }

  // printk(KERN_INFO "Messege: f_pos = %lu\n", ctx->pos);

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
    list_for_each_entry(file, &dir_list->children, list) {
      if (numb >= offset){
        if (!dir_emit(ctx, file->name, strlen(file->name), file->inode->i_ino, file->mode)) 
          return -ENOMEM; 
        ctx->pos++;
      }
      numb++;
    }
  }
  return 0;
}




ssize_t ram_vtfs_read(
  struct file *filp, // файловый дескриптор
  char *buffer,      // буфер в user-space для чтения и записи соответственно
  size_t len,        // длина данных для записи
  loff_t *offset     //смещение 
){ 
  struct inode *inode = file_inode(filp);
  char *data = inode->i_private;
  size_t data_size;

  if (!data) return 0;

  data_size = strlen(data);

  if (*offset >= data_size) return 0;

  if (*offset + len > data_size) len = data_size - *offset;

  if (copy_to_user(buffer, data + *offset, len)) return -EFAULT;

  *offset += len;
  return len;
}

ssize_t ram_vtfs_write(
  struct file *filp, 
  const char *buffer, 
  size_t len, 
  loff_t *offset
){
  printk(KERN_INFO "vtfs_write: start");
  struct inode *inode = file_inode(filp);
  char *data;
  
  if (len > FILE_MAX_SIZE){
    printk(KERN_ERR "vtfs_write: len > file_size");
    return -ENOMEM;
  } 

  if(!inode->i_private){
    inode->i_private = kmalloc(FILE_MAX_SIZE, GFP_KERNEL);
    if (!inode->i_private){
      printk(KERN_ERR "vtfs_write: no mem");
      return -ENOMEM;
    }
  }

  data = inode->i_private;

  if (copy_from_user(data, buffer, len)) {
    printk(KERN_ERR "vtfs_write: no mem");
    return -EFAULT;
  }
  
  data[len] = '\0';

  printk(KERN_INFO "vtfs_write: finish");
  return len;
}

int ram_vtfs_open(struct inode *inode, struct file *filp){
  filp->private_data = inode->i_private;
  return 0;
}

int ram_vtfs_release(struct inode *inode, struct file *filp){
  return 0;
}

module_init(vtfs_init);
module_exit(vtfs_exit);