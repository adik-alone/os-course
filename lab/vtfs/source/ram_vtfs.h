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
#endif

#ifndef VTFS
  #define VTFS
  #include "vtfs.h"
#endif

// #define MAX_NAME_SIZE 64

struct ram_vtfs_file {
    char name[NAME_MAX];
    ino_t ino; 
    umode_t mode;
    struct list_head list;
};

static LIST_HEAD(ram_vtfs_files);
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


//file operations 

int ram_vtfs_iterate(struct file* filp, struct dir_context* ctx);
int ram_vtfs_mkdir(struct mnt_idmap *idmap, struct inode *inode, struct dentry *dentry, umode_t mode);
int ram_vtfs_rmdir(struct inode *inode, struct dentry *dentry);




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
    .iterate_shared = ram_vtfs_iterate,
};