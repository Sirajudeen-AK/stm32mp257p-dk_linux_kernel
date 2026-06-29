# Program 20 — Coherent DMA Buffer as a Char Device

## 1. Goal

Take the coherent DMA buffer from Program 19 and make it the **data buffer of a
character device**. Userspace writes into the DMA buffer with `write()` and reads
it back with `read()` — the exact buffer you would later hand to real hardware.

---

## 2. Why Is This Useful?

When you later write an Ethernet (or any DMA) driver, the same buffer flows
straight to the hardware — no extra copy:

```
copy_from_user()
      ↓
DMA Buffer        ← allocated once with dma_alloc_coherent()
      ↓
Ethernet MAC DMA
      ↓
Wire
```

The DMA buffer can be handed directly to the device, because it already has a
DMA address the hardware understands.

---

## 3. Data Flow

```
Userspace:  echo "Hello" > /dev/mychar_dma
      │
      ▼
   write()
      │
   copy_from_user()
      │
      ▼
  DMA Buffer  (cpu_addr from dma_alloc_coherent)
      ▲
      │
   copy_to_user()
      │
   read()
      ▲
      │
Userspace:  cat /dev/mychar_dma
```

---

## 4. Driver Architecture

```
probe()
   │
dma_alloc_coherent()      ← get DMA buffer (cpu_addr + dma_addr)
   │
register cdev             ← alloc_chrdev_region + cdev_add + device_create
   │
/dev/mychar_dma
   /          \
write()       read()
   │            │
copy_from_user  copy_to_user
   │            │
        DMA Buffer
```

---

## 5. Code Walkthrough

```c
struct my_device {
    dev_t dev;  struct cdev cdev;  struct class *class;  struct device *device;
    void *cpu_addr;  dma_addr_t dma_addr;  size_t size;
};

// open(): recover per-device struct, store for read/write
static int my_open(struct inode *inode, struct file *file)
{
    file->private_data = container_of(inode->i_cdev, struct my_device, cdev);
    return 0;
}

// write(): user data → DMA buffer
static ssize_t my_write(struct file *file, const char __user *buf,
                        size_t len, loff_t *off)
{
    struct my_device *mydev = file->private_data;
    if (len > BUF_SIZE) len = BUF_SIZE;
    if (copy_from_user(mydev->cpu_addr, buf, len))   // into the DMA buffer
        return -EFAULT;
    mydev->size = len;
    return len;
}

// read(): DMA buffer → user data (with offset handling)
static ssize_t my_read(struct file *file, char __user *buf,
                       size_t len, loff_t *off)
{
    struct my_device *mydev = file->private_data;
    if (*off >= mydev->size) return 0;                 // EOF
    if (len > mydev->size - *off) len = mydev->size - *off;
    if (copy_to_user(buf, mydev->cpu_addr + *off, len))
        return -EFAULT;
    *off += len;
    return len;
}
```

`probe()` uses `goto` error-unwinding (`err_chr` → `err_dma`) so a partial failure
still frees the DMA buffer and char region cleanly.

---

## 6. Commands / Testing

```bash
sudo insmod coherent_DMA.ko
dmesg | tail                       # see CPU addr, BUS addr, major number
echo "Hello DMA" > /dev/mychar_dma # write() → copy_from_user → DMA buffer
cat /dev/mychar_dma                # read()  → copy_to_user
sudo rmmod coherent_DMA
```

---

## 7. Interview Questions & Answers

**Q1. Why allocate a DMA buffer instead of using `kmalloc()`?**

> Because `dma_alloc_coherent()` returns memory suitable for device DMA **and**
> provides the DMA address the hardware needs. `kmalloc()` gives only a CPU
> address, so the device couldn't use it directly.

**Q2. Why are there two addresses (`cpu_addr` and `dma_addr`)?**

> `cpu_addr` is the CPU virtual address used by software (e.g. `copy_to_user`).
> `dma_addr` is the bus address handed to the DMA engine/hardware. They are two
> views of the same physical buffer.

**Q3. Does `copy_to_user()` use the DMA address?**

> No. `copy_to_user()`/`copy_from_user()` operate on the CPU virtual address
> (`cpu_addr`). The DMA address is only for the hardware.

**Q4. Why is `container_of()` used in `open()`?**

> The `cdev` is embedded in `struct my_device`. `container_of()` recovers the full
> device object from `inode->i_cdev`, which is then stored in `file->private_data`
> so `read`/`write` can find the DMA buffer per open.

**Q5. Why do `read()` and `write()` track `size` and `*off`?**

> `write()` records how many valid bytes are in the buffer (`size`); `read()` uses
> `*off` to return data once and then signal EOF (`return 0`), so `cat` terminates
> instead of looping forever.

**Q6. Why use coherent DMA here rather than streaming DMA?**

> The buffer is allocated once and accessed by both CPU and (eventually) the device
> repeatedly. Coherent DMA keeps both views consistent automatically, avoiding
> per-access cache management — simpler for a persistent control/data buffer.