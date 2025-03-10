#ifndef UTILS
  #define UTILS
  #include <linux/init.h>
  #include <linux/module.h>
  #include <linux/printk.h>
  #include <linux/fs.h>
  #include <linux/stat.h>
#endif

#ifndef FILES_UTILS
  #define FILES_UNILS
  #include <linux/slab.h>
  #include <linux/list.h>
  #include <linux/dcache.h>
  #include <linux/uaccess.h>
#endif

#ifndef VTFS
  #define VTFS
  #include "vtfs.h"
#endif

// #define MAX_NAME_SIZE 64
#define FILE_MAX_SIZE 4096

struct ram_vtfs_file {
    char name[NAME_MAX];
    struct inode *inode; 
    umode_t mode;
    struct list_head list;
};

struct ram_vtfs_dir_list {
  struct list_head children;
};

static ino_t next_ino = ROOT_INODE + 1; 


// funcions

struct dentry* ram_vtfs_lookup(
    struct inode* parent_inode,  // родительская нода
    struct dentry* child_dentry, // объект, к которому мы пытаемся получить доступ
    unsigned int flag            // неиспользуемое значение
);

int ram_vtfs_create(
  struct mnt_idmap *idmap,
  struct inode *parent_inode, 
  struct dentry *child_dentry, 
  umode_t mode, 
  bool b
);

int ram_vtfs_unlink(struct inode *parent_inode, struct dentry *child_dentry);
int ram_vtfs_mkdir(struct mnt_idmap *idmap, struct inode *inode, struct dentry *dentry, umode_t mode);
int ram_vtfs_rmdir(struct inode *inode, struct dentry *dentry);


//file operations 

int ram_vtfs_iterate(struct file* filp, struct dir_context* ctx);

ssize_t ram_vtfs_read(
  struct file *filp, // файловый дескриптор
  char *buffer,      // буфер в user-space для чтения и записи соответственно
  size_t len,        // длина данных для записи
  loff_t *offset     //смещение 
);
ssize_t ram_vtfs_write(
  struct file *filp, 
  const char *buffer, 
  size_t len, 
  loff_t *offset
);
int ram_vtfs_open(struct inode *inode, struct file *filp);
int ram_vtfs_release(struct inode *inode, struct file *filp);



// structs

struct inode_operations ram_vtfs_inode_ops = {
    .lookup = ram_vtfs_lookup,
    .create = ram_vtfs_create,
    .unlink = ram_vtfs_unlink,
    .mkdir  = ram_vtfs_mkdir,
    .rmdir  = ram_vtfs_rmdir,
};

struct file_operations ram_vtfs_dir_ops = {
    .iterate_shared = ram_vtfs_iterate,
};

struct file_operations ram_vtfs_file_ops = {
  .read = ram_vtfs_read,
  .write = ram_vtfs_write,
  .open = ram_vtfs_open,
  .release = ram_vtfs_release,
};