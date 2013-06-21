#include <linux/module.h>
#include <linux/init.h>

#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/kthread.h>
#include <linux/err.h>

MODULE_LICENSE("GPL");

#define THREAD_WAKEUP 0
#define TDRV_MAJOR 111
#define BUF_SIZE 10
#define TDRV_IOC_MAGIC 'k'
#define TDRV_IOC_SETTIME _IOW(TDRV_IOC_MAGIC, 1, int)

struct tdrv_dev {
	char buf[BUF_SIZE];
	int size;

	struct mutex lock;
	struct cdev cdev;
	struct timer_list timer;
};

struct tdrv_thread {
	struct task_struct *freeb_thread;
	wait_queue_head_t wqueue;
	unsigned long flags;
};

static struct tdrv_dev tdrv_dev;
static struct tdrv_thread thread;

static int tdrv_thread(void *arg)
{
	allow_signal(SIGKILL);

	while (!kthread_should_stop()) {
		if (signal_pending(current))
			flush_signals(current);

		wait_event_interruptible(thread.wqueue,
				test_bit(THREAD_WAKEUP, &thread.flags)
			 	|| kthread_should_stop());

		clear_bit(THREAD_WAKEUP, &thread.flags);
		if (!kthread_should_stop()) {
			printk("free buffer!\n");
			mutex_lock(&tdrv_dev.lock);
			tdrv_dev.size = 0;
			mutex_unlock(&tdrv_dev.lock);
		}
	}

	return 0;
}

static void tdrv_wakeup_thread(void)
{
	set_bit(THREAD_WAKEUP, &thread.flags);
	wake_up(&thread.wqueue);
}

static int create_freeb_thread(void)
{
	thread.flags = 0;
	init_waitqueue_head(&thread.wqueue);
	thread.freeb_thread = kthread_run(tdrv_thread, NULL, "freeb_thread");

	if (IS_ERR(thread.freeb_thread))
		return -1;

	return 0;
}

static void join_freeb_thread(void)
{
	kthread_stop(thread.freeb_thread);
}

static void tdrv_timer_fn(unsigned long arg)
{
	tdrv_wakeup_thread();
}

int tdrv_open(struct inode *inode, struct file *filp)
{
	struct tdrv_dev *dev;

	dev = container_of(inode->i_cdev, struct tdrv_dev, cdev);
	filp->private_data = dev;

	return 0;
}

int tdrv_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t tdrv_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct tdrv_dev *dev = filp->private_data;
	ssize_t ret = 0;
	char *pos;

	mutex_lock(&dev->lock);
	if (*f_pos >= dev->size)
		goto out;
	if (*f_pos + count > dev->size)
		count = dev->size - *f_pos;

	pos = dev->buf + *f_pos;
	if (copy_to_user(buf, pos, count)) {
		ret = -EFAULT;
		goto out;
	}
	*f_pos += count;
	ret = count;
out:
	mutex_unlock(&dev->lock);
	return ret;
}

static ssize_t tdrv_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct tdrv_dev *dev = filp->private_data;
	ssize_t ret = 0;

	mutex_lock(&dev->lock);
	if (*f_pos >= BUF_SIZE)
		goto out;
	if (*f_pos + count > BUF_SIZE)
		count = BUF_SIZE - *f_pos;

	if (copy_from_user(dev->buf + *f_pos, buf, count)) {
		ret = -EFAULT;
		goto out;
	}
	*f_pos += count;
	ret = count;
	if (dev->size < *f_pos)
		dev->size = *f_pos;
out:
	mutex_unlock(&dev->lock);
	return ret;
}

static int tdrv_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct tdrv_dev *dev = filp->private_data;
	unsigned long sec;
	ssize_t ret = 0;

	ret = __get_user(sec, (unsigned long __user *)arg);

	switch(cmd) {
	case TDRV_IOC_SETTIME:
		mod_timer(&dev->timer, jiffies + sec * HZ);
		break;
	default:
		return -ENOTTY;
	}

	return ret;
}

struct file_operations tdrv_fops = {
	.owner = THIS_MODULE,
	.read = tdrv_read,
	.write = tdrv_write,
	.ioctl = tdrv_ioctl,
	.open = tdrv_open,
	.release = tdrv_release,
};

static int __init tdrv_init(void)
{
	int ret = 0;

	printk("tdrv init\n");

	mutex_init(&tdrv_dev.lock);
	tdrv_dev.size = 0;
	init_timer(&tdrv_dev.timer);
	tdrv_dev.timer.function = tdrv_timer_fn;

	ret = create_freeb_thread();
	if (ret)
		return ret;

	cdev_init(&tdrv_dev.cdev, &tdrv_fops);
	if (cdev_add(&tdrv_dev.cdev, MKDEV(TDRV_MAJOR, 0), 1) ||
		register_chrdev_region(MKDEV(TDRV_MAJOR, 0), 1, "/dev/tdrv") < 0)
		return -1;

	return 0;
}

static void __exit tdrv_exit(void)
{
	printk("tdrv exit\n");

	del_timer_sync(&tdrv_dev.timer);
	join_freeb_thread();
	cdev_del(&tdrv_dev.cdev);
	unregister_chrdev_region(MKDEV(TDRV_MAJOR, 0), 1);
}

module_init(tdrv_init);
module_exit(tdrv_exit);
