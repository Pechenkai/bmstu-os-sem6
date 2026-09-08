#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/mnt_idmapping.h>
#include <linux/slab.h>
#include <linux/statfs.h>
#include <linux/mount.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Panteleev VM");
MODULE_DESCRIPTION("my_vfs with slab cache");

#define MY_VFS_MAGIC_NUMBER 0x7777777
#define MY_VFS_SLAB_NAME    "myvfs_cache"
#define MY_VFS_CACHE_SIZE   129

struct my_vfs_inode {
	int i_mode;
	unsigned long i_ino;
};

static void **line = NULL;
static int sco = 0;
static struct kmem_cache *cache = NULL;

static void my_vfs_put_super(struct super_block *sb)
{
	printk(KERN_INFO "-- vfs: my_vfs_put_super\n");
}

static const struct super_operations my_vfs_super_ops = {
	.put_super = my_vfs_put_super,
	.statfs    = simple_statfs,
	.drop_inode = generic_delete_inode,
};

static struct inode *my_vfs_make_inode(struct super_block *sb, int mode)
{
	struct inode *ret;

	ret = new_inode(sb);
	if (ret) {
		struct timespec64 cur;

		inode_init_owner(&nop_mnt_idmap, ret, NULL, mode);
		ret->i_size = PAGE_SIZE;

		cur = current_time(ret);
		inode_set_atime_to_ts(ret, cur);
		inode_set_mtime_to_ts(ret, cur);
		inode_set_ctime_to_ts(ret, cur);

		ret->i_ino = 1;
	}
	return ret;
}

static int my_vfs_fill_sb(struct super_block *sb, void *data, int silent)
{
	struct inode *root;

	printk(KERN_INFO "my_vfs: fill_sb\n");

	sb->s_blocksize = PAGE_SIZE;
	sb->s_blocksize_bits = PAGE_SHIFT;
	sb->s_magic = MY_VFS_MAGIC_NUMBER;
	sb->s_op = &my_vfs_super_ops;

	root = my_vfs_make_inode(sb, S_IFDIR | 0755);
	if (!root) {
		printk(KERN_ERR "-- vfs error: my_vfs_make_inode\n");
		return -ENOMEM;
	}

	root->i_op  = &simple_dir_inode_operations;
	root->i_fop = &simple_dir_operations;

	sb->s_root = d_make_root(root);
	if (!sb->s_root) {
		printk(KERN_ERR "-- vfs err: d_make_root\n");
		iput(root);
		return -ENOMEM;
	}

	printk(KERN_INFO "-- vfs: root created\n");
	return 0;
}

static void kill_my_vfs(struct super_block *sb)
{
	printk(KERN_INFO "-- vfs: kill sb\n");
	kill_anon_super(sb);
}

static struct dentry *my_vfs_mount(struct file_system_type *fs_type,
				   int flags, const char *dev_name, void *data)
{
	struct dentry *root = mount_nodev(fs_type, flags, data, my_vfs_fill_sb);
	if (IS_ERR(root)) {
		printk(KERN_ERR "-- vfs err: mount_nodev\n");
		return root;
	}
	return root;
}

static struct file_system_type my_vfs_type = {
	.owner    = THIS_MODULE,
	.name     = "my_vfs",
	.mount    = my_vfs_mount,
	.kill_sb  = kill_my_vfs,
	.fs_flags = 0,
};

static void co(void *p)
{
	*(int *)p = ++sco; 
}

static int __init myfs_init(void)
{
    int ret, i;

    printk(KERN_INFO "-- vfs: init\n");

    ret = register_filesystem(&my_vfs_type);
    if (ret) {
        printk(KERN_ERR "-- vfs err: register_filesystem (%d)\n", ret);
        return ret;
    }

    line = kmalloc_array(MY_VFS_CACHE_SIZE, sizeof(void *), GFP_KERNEL);
    if (!line) {
        printk(KERN_ERR "-- vfs: can't kmalloc line[]\n");
        unregister_filesystem(&my_vfs_type);
        return -ENOMEM;
    }

    for (i = 0; i < MY_VFS_CACHE_SIZE; i++)
        line[i] = NULL;

    cache = kmem_cache_create(MY_VFS_SLAB_NAME,
                              sizeof(struct my_vfs_inode),
                              0, SLAB_HWCACHE_ALIGN, co);
    if (!cache) {
        printk(KERN_ERR "-- vfs: can't kmem_cache_create\n");
        kfree(line);
        line = NULL;
        unregister_filesystem(&my_vfs_type);
        return -ENOMEM;
    }

    for (i = 0; i < MY_VFS_CACHE_SIZE; i++) {
        line[i] = kmem_cache_alloc(cache, GFP_KERNEL);
        if (!line[i]) {
            int j;
            printk(KERN_ERR "-- vfs: can't kmem_cache_alloc at %d\n", i);

            for (j = 0; j < i; j++)
                if (line[j])
                    kmem_cache_free(cache, line[j]);

            kmem_cache_destroy(cache);
            cache = NULL;

            kfree(line);
            line = NULL;

            unregister_filesystem(&my_vfs_type);
            return -ENOMEM;
        }
    }

    printk(KERN_INFO "-- vfs: register_filesystem succeeded\n");
    return 0;
}

static void __exit myfs_exit(void)
{
	int i, ret;

	printk(KERN_INFO "-- vfs: exit\n");

	if (cache && line) {
		for (i = 0; i < MY_VFS_CACHE_SIZE; i++) {
			if (line[i])
				kmem_cache_free(cache, line[i]);
		}
	}

	if (cache) {
		kmem_cache_destroy(cache);
		cache = NULL;
	}

	kfree(line);
	line = NULL;

	ret = unregister_filesystem(&my_vfs_type);
	if (ret)
		printk(KERN_ERR "-- vfs: unregister_filesystem failed (%d)\n", ret);
	else
		printk(KERN_INFO "-- vfs: unregister_filesystem succeeded\n");
}

module_init(myfs_init);
module_exit(myfs_exit);
