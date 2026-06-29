# Program 19 — DMA (Direct Memory Access) Basics

## 1. Goal

Understand what DMA is and allocate a **DMA-capable buffer** with
`dma_alloc_coherent()`. DMA lets a device move data **directly to/from RAM**
without the CPU copying every byte.

---

## 2. Why DMA?

Suppose a camera captures a 4 MB image.

**Without DMA** — the CPU copies every byte:

```
Camera
  │
  ▼
CPU reads 1 byte → CPU writes 1 byte → ... (× 4 MB)
```

Problems: CPU is fully busy, high interrupt load, slow.

**With DMA** — the DMA engine does the copy:

```
Camera
  │
  ▼
DMA Controller
  │
  ▼
 RAM
```

The CPU just says *"copy 4 MB from device to RAM"*, then does other work. When the
transfer finishes:

```
DMA Interrupt
   ↓
Driver notified
   ↓
Userspace notified
```

---

## 3. CPU Copy vs DMA

```
CPU Copy                  DMA
--------                  ---
Device                    Device
  ↓                         ↓
 CPU                     DMA Controller     ← CPU only starts the transfer
  ↓                         ↓
 RAM                       RAM
```

Almost every high-speed peripheral uses DMA:

| Hardware  | Uses DMA? |
| --------- | --------- |
| Ethernet  | ✅        |
| CAN XL    | ✅        |
| USB       | ✅        |
| PCIe      | ✅        |
| SDMMC     | ✅        |
| SPI       | ✅        |
| I2S Audio | ✅        |
| Camera    | ✅        |

---

## 4. Why Two Addresses (CPU vs DMA)?

Normal memory (`char buffer[4096];`) **cannot** reliably be used for DMA. Instead:

```c
cpu_addr = dma_alloc_coherent(dev, BUF_SIZE, &dma_addr, GFP_KERNEL);
```

This returns **two views of the same buffer**:

```
CPU side                         Device side
--------                         -----------
CPU Virtual Address              DMA / Bus Address
0xffff8000....                   0x84000000
   │                                 │
   ▼                                 ▼
  MMU ──► Physical RAM ◄──────── DMA engine
```

- The **CPU** uses the *virtual* address (`cpu_addr`).
- The **hardware** uses the *DMA/bus* address (`dma_addr`).

A device does not understand the CPU's virtual addresses, so it needs a
DMA-capable address — that's why `dma_alloc_coherent()` gives you both.

---

## 5. Code Walkthrough

```c
struct my_device {
    void       *cpu_addr;   // CPU uses this
    dma_addr_t  dma_addr;   // hardware uses this
};

// in probe():
mydev->cpu_addr = dma_alloc_coherent(&pdev->dev, BUF_SIZE,
                                     &mydev->dma_addr, GFP_KERNEL);
if (!mydev->cpu_addr)
    return -ENOMEM;
memset(mydev->cpu_addr, 0, BUF_SIZE);

// in remove() — allocation and free APIs must match:
dma_free_coherent(&pdev->dev, BUF_SIZE, mydev->cpu_addr, mydev->dma_addr);
```

`%pad` is the printk format specifier for a `dma_addr_t`.

---

## 6. Two Kinds of DMA (preview of Programs 20 & 21)

| Coherent DMA (`dma_alloc_coherent`) | Streaming DMA (`dma_map_single`) |
| ----------------------------------- | -------------------------------- |
| CPU cache kept coherent for you     | You manage cache via map/unmap   |
| Simple, no manual flushing          | Faster, less overhead per buffer |
| Slightly slower / more memory       | Used by Ethernet, PCIe, NVMe     |
| Good for descriptors/control blocks | Good for large/frequent payloads |

---

## 7. Architecture (full data path)

```
Userspace
   ↓ write()
Driver
   ↓
DMA Buffer
   ↓
DMA Engine
   ↓
Hardware
   ↓ Interrupt
Driver  →  read()
```

> Note: this program only **allocates** a DMA buffer — there is no hardware
> consuming it yet, so you won't see data actually moving. Program 20 wires the
> buffer to a char device.

---

## 8. Interview Questions & Answers

**Q1. Why can't a device use a kernel virtual address?**

> Kernel virtual addresses are meaningful only to the CPU and its MMU. Hardware
> devices address memory via DMA/bus addresses, not CPU virtual addresses, so they
> need the `dma_addr_t` the DMA API provides.

**Q2. Why not use `kmalloc()` for DMA?**

> `kmalloc()` returns CPU memory but does not give you a DMA address and does not
> guarantee the buffer is suitable/mapped for the device. The DMA APIs return
> memory the device can safely access plus the address it must use.

**Q3. What does `dma_addr_t` represent?**

> It stores the DMA/bus address the hardware uses for transfers — distinct from the
> CPU virtual address.

**Q4. Why use `dma_free_coherent()`?**

> To release memory obtained from `dma_alloc_coherent()`. Allocation and free APIs
> must match; mixing them (e.g. `kfree` on a coherent buffer) corrupts state.

**Q5. What is "coherent" in `dma_alloc_coherent`?**

> The kernel ensures the CPU cache and device see the **same data** without manual
> cache flushes — typically by mapping the buffer uncached or with hardware cache
> coherency, so reads/writes from either side are immediately visible.

**Q6. Does the CPU ever touch the `dma_addr`?**

> No. The CPU always uses `cpu_addr` (virtual). `dma_addr` is handed only to the
> device/DMA engine.