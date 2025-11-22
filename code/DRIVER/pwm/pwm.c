#include <linux/module.h>
#include <linux/init>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>


static int my_probe(struct platform_device *pdev) {
    #define MYDEV_NAME "mychardev"

static dev_t dev_num;              // 设备号
static struct cdev my_cdev;        // 字符设备结构
static struct class *my_class;     // sysfs 类

// 文件操作函数
static int my_open(struct inode *inode, struct file *file)
{
    pr_info("mychardev: open\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("mychardev: release\n");
    return 0;
}

static ssize_t my_read(struct file *file, char __user *buf,
                       size_t count, loff_t *ppos)
{
    pr_info("mychardev: read\n");
    return 0; // 没有数据
}

static ssize_t my_write(struct file *file, const char __user *buf,
                        size_t count, loff_t *ppos)
{
    pr_info("mychardev: write\n");
    return count; // 假装写成功
}

// 文件操作集
static const struct file_operations my_fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .release = my_release,
    .read    = my_read,
    .write   = my_write,
};

// probe 回调
static int my_probe(struct platform_device *pdev)
{
    int ret;

    pr_info("my_driver: probed\n");

    // 分配设备号
    ret = alloc_chrdev_region(&dev_num, 0, 1, "pwm_device");
    if (ret < 0) {
        pr_err("failed to alloc chrdev region\n");
        return ret;
    }

    // 初始化 cdev
    cdev_init(&my_cdev, &my_fops);
    my_cdev.owner = THIS_MODULE;

    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("failed to add cdev\n");
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    // 创建设备节点
    my_class = class_create(THIS_MODULE, MYDEV_NAME);
    if (IS_ERR(my_class)) {
        pr_err("failed to create class\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(my_class);
    }

    device_create(my_class, NULL, dev_num, NULL, MYDEV_NAME);

    pr_info("my_driver: char device registered, major=%d, minor=%d\n",
            MAJOR(dev_num), MINOR(dev_num));

    //创建pwm设备
    devm_of_

    return 0;
}

static int my_remove(struct platform_device *pdev) {

    pr_info("my_driver: removed\n");
    return 0;
}

static const struct of_device_id my_of_match[] = {
    { .compatible = "myvendor,mydevice" },
    { },
};
MODULE_DEVICE_TABLE(of, my_of_match);

static struct platform_driver my_driver = {
    .driver = {
        .name = "my_driver",
        .of_match_table = my_of_match,
    },
    .probe = my_probe,
    .remove = my_remove,
};

module_platform_driver(my_driver);

MODULE_LICENSE("GPL");
