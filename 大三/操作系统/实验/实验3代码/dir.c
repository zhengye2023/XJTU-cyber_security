#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/slab.h>

#define DEVICE_NAME "chat_simple"
#define MAX_MSG_LEN 1024

struct chat_dev {
    char msg[MAX_MSG_LEN];
    size_t msg_len;
    unsigned long seq; /* message sequence number, 0 = none yet */
    struct mutex lock;
    wait_queue_head_t readq;
    struct cdev cdev;
    int users;
};

static dev_t devt;
static struct class *chat_class;
static struct chat_dev *gdev;

/* per-open file data */
struct chat_fpriv {
    unsigned long last_seen; /* last sequence seen by this fd */
};

static int chat_open(struct inode *inode, struct file *filp)
{
    struct chat_fpriv *p;

    p = kzalloc(sizeof(*p), GFP_KERNEL);
    if (!p) return -ENOMEM;

    /* initialize last_seen to current seq so new client doesn't get old messages */
    mutex_lock(&gdev->lock);
    p->last_seen = gdev->seq;
    gdev->users++;
    mutex_unlock(&gdev->lock);

    filp->private_data = p;
    return 0;
}

static int chat_release(struct inode *inode, struct file *filp)
{
    struct chat_fpriv *p = filp->private_data;
    mutex_lock(&gdev->lock);
    if(gdev->users>0) gdev->users--;
    mutex_unlock(&gdev->lock);
    if (p) kfree(p);
    filp->private_data = NULL;
    return 0;
}

/* read: if there's a newer message (seq > last_seen) copy it.
 * if not:
 *   - if O_NONBLOCK set: return -EAGAIN
 *   - else: wait until new message arrives
 */
static ssize_t chat_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    struct chat_fpriv *p = filp->private_data;
    ssize_t ret = 0;

    if (!p) return -EFAULT;

    /* wait for new message */
    for (;;) {
        mutex_lock(&gdev->lock);
        if (gdev->seq > p->last_seen && gdev->msg_len > 0) {
            //有新新消息了
            size_t to_copy = min(count, gdev->msg_len);
            if (copy_to_user(buf, gdev->msg, to_copy)) {
                ret = -EFAULT;
            } else {
                p->last_seen = gdev->seq;
                ret = to_copy;
            }
            mutex_unlock(&gdev->lock);
            return ret; 
        }
 
        /* no new data */
        mutex_unlock(&gdev->lock);

        if (filp->f_flags & O_NONBLOCK) return -EAGAIN;

        /* wait for writer to wake us up, interruptible so Ctrl+C works */
        if (wait_event_interruptible(gdev->readq, (gdev->seq > p->last_seen && gdev->msg_len > 0)))
            return -ERESTARTSYS;

        /* loop to re-check condition */
    }
}

/* write: overwrite latest message, bump seq, wake readers */
static ssize_t chat_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{   
    char kbuf[64];////

    if (count == 0) return 0;

    /* 先将输入拷贝到临时缓冲区 */
    size_t copy_len = min(count, sizeof(kbuf) - 1);
    if (copy_from_user(kbuf, buf, copy_len))
        return -EFAULT;
    kbuf[copy_len] = '\0';

    /*  如果用户输入 /users → 返回在线人数 */
    if (strcmp(kbuf, "/users") == 0) {
        char out[64];
        mutex_lock(&gdev->lock);
        snprintf(out, sizeof(out), "Users =%d", gdev->users);
        strcpy(gdev->msg, out);
        gdev->msg_len = strlen(out);
        gdev->seq++;
        if (gdev->seq == 0) gdev->seq = 1;
        mutex_unlock(&gdev->lock);
        wake_up_interruptible(&gdev->readq);
        return copy_len;
    }//

    copy_len = min(count, (size_t)(MAX_MSG_LEN - 1));

    mutex_lock(&gdev->lock);
    if (copy_from_user(gdev->msg, buf, copy_len)) {
        mutex_unlock(&gdev->lock);
        return -EFAULT;
    }
    gdev->msg[copy_len] = '\0';
    gdev->msg_len = copy_len;
    gdev->seq++;             /* new message sequence */
    if (gdev->seq == 0) gdev->seq = 1; /* avoid 0 wrap */
    mutex_unlock(&gdev->lock);

    /* wake up all readers */
    wake_up_interruptible(&gdev->readq);

    return copy_len;
}

static const struct file_operations chat_fops = {
    .owner = THIS_MODULE,
    .open = chat_open,
    .release = chat_release,
    .read = chat_read,
    .write = chat_write,
};

static int __init chat_init(void)
{
    int ret;
    gdev = kzalloc(sizeof(*gdev), GFP_KERNEL);
    if (!gdev) return -ENOMEM;

    mutex_init(&gdev->lock);
    init_waitqueue_head(&gdev->readq);
    gdev->msg_len = 0;
    gdev->seq = 0;
    gdev->users = 0; //

    /* allocate device number dynamically */
    ret = alloc_chrdev_region(&devt, 0, 1, DEVICE_NAME);
    if (ret) {
        pr_err("chat_simple: alloc_chrdev_region failed: %d\n", ret);
        kfree(gdev);
        return ret;
    }

    cdev_init(&gdev->cdev, &chat_fops);
    gdev->cdev.owner = THIS_MODULE;
    ret = cdev_add(&gdev->cdev, devt, 1);
    if (ret) {
        pr_err("chat_simple: cdev_add failed: %d\n", ret);
        unregister_chrdev_region(devt, 1);
        kfree(gdev);
        return ret;
    }

    chat_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(chat_class)) {
        pr_err("chat_simple: class_create failed\n");
        cdev_del(&gdev->cdev);
        unregister_chrdev_region(devt, 1);
        kfree(gdev);
        return PTR_ERR(chat_class);
    }

    device_create(chat_class, NULL, devt, NULL, DEVICE_NAME);

    pr_info("chat_simple: loaded major=%d minor=%d\n", MAJOR(devt), MINOR(devt));
    return 0;
}

static void __exit chat_exit(void)
{
    device_destroy(chat_class, devt);
    class_destroy(chat_class);
    cdev_del(&gdev->cdev);
    unregister_chrdev_region(devt, 1);
    kfree(gdev);
    pr_info("chat_simple: unloaded\n");
}

module_init(chat_init);
module_exit(chat_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chat GPT helper");
MODULE_DESCRIPTION("Simple chat device: stores latest message, write overwrites, read gets newest");
