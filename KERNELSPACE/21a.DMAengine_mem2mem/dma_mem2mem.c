/*
 * dma_mem2mem.c  -  Lesson 22: REAL streaming DMA you write yourself
 *
 * This is the piece that was MISSING from lesson 21.
 *
 * Lesson 21 (streaming_dma.c) only mapped a buffer and unmapped it again -
 * no hardware ever touched the data. Here we use the SoC's DMA controller
 * (stm32-dma3) through the kernel "dmaengine" API to perform a real
 * memory-to-memory transfer:
 *
 *      userspace write()  ->  src_buf
 *                              |
 *                  dma_map_single(DMA_TO_DEVICE)      <- streaming DMA map
 *                              |
 *                  dmaengine_prep_dma_memcpy()        <- program DMA engine
 *                              |
 *                  dma_async_issue_pending()          <- hardware copies bytes
 *                              |
 *                  wait_for_completion()              <- IRQ tells us it's done
 *                              |
 *                  dma_unmap_single(DMA_FROM_DEVICE)  <- CPU sees fresh data
 *                              |
 *      userspace read()   <-  dst_buf
 *
 * Device node: /dev/dma_mem2mem
 *
 * How this connects to your end0 idea:
 *   PC --socket--> board echo program --write()--> THIS driver (real DMA)
 *   PC <--socket-- board echo program <--read()--- THIS driver
 *   The end0 transfers are stmmac's DMA; THIS is the "process with DMA" step
 *   in the middle that you actually wrote.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/completion.h>

#define DEVICE_NAME "dma_mem2mem"
#define CLASS_NAME  "dma_m2m_class"
#define BUF_SIZE    4096

static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;

static struct dma_chan *dma_ch;
static char *src_buf;
static char *dst_buf;
static size_t data_len;

static struct completion dma_done;

/* Called from the DMA controller's interrupt when the copy is finished. */
static void dma_callback(void *param)
{
    complete(&dma_done);
}

/* The heart of the lesson: one real streaming-DMA memcpy of 'len' bytes. */
static int do_dma_transfer(size_t len)
{
    struct device *dev = dma_ch->device->dev;
    dma_addr_t src_dma, dst_dma;
    struct dma_async_tx_descriptor *tx;
    dma_cookie_t cookie;
    enum dma_status status;
    unsigned long tmo;
    int ret = 0;

    /* 1. Map the CPU buffers for the device (cache flush/invalidate happens). */
    src_dma = dma_map_single(dev, src_buf, len, DMA_TO_DEVICE);
    if (dma_mapping_error(dev, src_dma)) {
        pr_err("%s: src dma_map_single failed\n", DEVICE_NAME);
        return -ENOMEM;
    }

    dst_dma = dma_map_single(dev, dst_buf, len, DMA_FROM_DEVICE);
    if (dma_mapping_error(dev, dst_dma)) {
        pr_err("%s: dst dma_map_single failed\n", DEVICE_NAME);
        dma_unmap_single(dev, src_dma, len, DMA_TO_DEVICE);
        return -ENOMEM;
    }

    /* 2. Describe the transfer to the DMA engine. */
    tx = dmaengine_prep_dma_memcpy(dma_ch, dst_dma, src_dma, len,
                                   DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
    if (!tx) {
        pr_err("%s: dmaengine_prep_dma_memcpy failed\n", DEVICE_NAME);
        ret = -EIO;
        goto unmap;
    }

    /* 3. Arm completion + callback, submit, and kick off the hardware. */
    reinit_completion(&dma_done);
    tx->callback = dma_callback;
    tx->callback_param = NULL;

    cookie = dmaengine_submit(tx);
    if (dma_submit_error(cookie)) {
        pr_err("%s: dmaengine_submit error\n", DEVICE_NAME);
        ret = -EIO;
        goto unmap;
    }

    dma_async_issue_pending(dma_ch);

    /* 4. Wait for the DMA-completion interrupt. */
    tmo = wait_for_completion_timeout(&dma_done, msecs_to_jiffies(5000));
    if (tmo == 0) {
        pr_err("%s: DMA transfer timed out\n", DEVICE_NAME);
        dmaengine_terminate_sync(dma_ch);
        ret = -ETIMEDOUT;
        goto unmap;
    }

    status = dma_async_is_tx_complete(dma_ch, cookie, NULL, NULL);
    if (status != DMA_COMPLETE) {
        pr_err("%s: DMA did not complete (status=%d)\n", DEVICE_NAME, status);
        ret = -EIO;
    }

unmap:
    /* 5. Unmap (DMA_FROM_DEVICE invalidates cache so the CPU reads new data). */
    dma_unmap_single(dev, dst_dma, len, DMA_FROM_DEVICE);
    dma_unmap_single(dev, src_dma, len, DMA_TO_DEVICE);
    return ret;
}

static int dev_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    return 0;
}

/* write() = give us data, we DMA-copy it src -> dst */
static ssize_t dev_write(struct file *file, const char __user *ubuf,
                         size_t len, loff_t *off)
{
    int ret;

    if (len > BUF_SIZE)
        len = BUF_SIZE;

    if (copy_from_user(src_buf, ubuf, len))
        return -EFAULT;

    memset(dst_buf, 0, BUF_SIZE);

    ret = do_dma_transfer(len);
    if (ret)
        return ret;

    data_len = len;
    pr_info("%s: DMA copied %zu bytes (src -> dst)\n", DEVICE_NAME, len);
    return len;
}

/* read() = hand back the DMA-processed data */
static ssize_t dev_read(struct file *file, char __user *ubuf,
                        size_t len, loff_t *off)
{
    if (*off >= data_len)
        return 0;                 /* EOF */

    if (len > data_len - *off)
        len = data_len - *off;

    if (copy_to_user(ubuf, dst_buf + *off, len))
        return -EFAULT;

    *off += len;
    return len;
}

static const struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = dev_open,
    .release = dev_release,
    .read    = dev_read,
    .write   = dev_write,
};

static int __init dma_m2m_init(void)
{
    dma_cap_mask_t mask;
    int ret;

    init_completion(&dma_done);

    /* Ask the dmaengine core for any channel that can do memory-to-memory. */
    dma_cap_zero(mask);
    dma_cap_set(DMA_MEMCPY, mask);

    dma_ch = dma_request_chan_by_mask(&mask);
    if (IS_ERR(dma_ch)) {
        ret = PTR_ERR(dma_ch);
        pr_err("%s: no DMA_MEMCPY channel available (%d). "
               "Is mem2mem enabled for stm32-dma3?\n", DEVICE_NAME, ret);
        return ret;
    }
    pr_info("%s: got DMA channel %s\n", DEVICE_NAME, dma_chan_name(dma_ch));

    /* DMA-capable buffers. */
    src_buf = kmalloc(BUF_SIZE, GFP_KERNEL);
    dst_buf = kmalloc(BUF_SIZE, GFP_KERNEL);
    if (!src_buf || !dst_buf) {
        ret = -ENOMEM;
        goto err_bufs;
    }

    /* Char device + /dev node. */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("%s: alloc_chrdev_region failed\n", DEVICE_NAME);
        goto err_bufs;
    }

    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;
    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("%s: cdev_add failed\n", DEVICE_NAME);
        goto err_chrdev;
    }

    my_class = class_create(CLASS_NAME);
    if (IS_ERR(my_class)) {
        ret = PTR_ERR(my_class);
        pr_err("%s: class_create failed\n", DEVICE_NAME);
        goto err_cdev;
    }

    my_device = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(my_device)) {
        ret = PTR_ERR(my_device);
        pr_err("%s: device_create failed\n", DEVICE_NAME);
        goto err_class;
    }

    pr_info("%s: loaded, /dev/%s ready\n", DEVICE_NAME, DEVICE_NAME);
    return 0;

err_class:
    class_destroy(my_class);
err_cdev:
    cdev_del(&my_cdev);
err_chrdev:
    unregister_chrdev_region(dev_num, 1);
err_bufs:
    kfree(src_buf);
    kfree(dst_buf);
    dma_release_channel(dma_ch);
    return ret;
}

static void __exit dma_m2m_exit(void)
{
    device_destroy(my_class, dev_num);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);

    kfree(src_buf);
    kfree(dst_buf);

    dma_release_channel(dma_ch);

    pr_info("%s: removed\n", DEVICE_NAME);
}

module_init(dma_m2m_init);
module_exit(dma_m2m_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Lesson 22: real memory-to-memory streaming DMA via dmaengine");
