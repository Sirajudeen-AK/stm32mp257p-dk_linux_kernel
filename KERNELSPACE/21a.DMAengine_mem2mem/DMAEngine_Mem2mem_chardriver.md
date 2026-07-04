# Lesson 21a — DMAEngine Mem2Mem: Char Driver (with Userspace)

## What this is

A **char driver** (`/dev/dma_mem2mem`) that performs a real hardware
memory-to-memory transfer using the SoC's DMA controller (`stm32-dma3`).

- Userspace `write()` triggers a real streaming DMA copy inside the kernel.
- Userspace `read()` retrieves the DMA-processed result.
- Tests with `dma_test.c` (or any shell command).

---

## Files

| File | Role |
|------|------|
| `dma_mem2mem.c` | Kernel module — char driver + dmaengine API |
| `dma_test.c` | Userspace test program |
| `Makefile` | Kernel build system |

---

## Flow Diagram

```
insmod
  │
  ├─ dma_request_chan_by_mask(DMA_MEMCPY)
  │         └─ acquires stm32-dma3 channel
  │
  ├─ kmalloc(src_buf, 4096)
  ├─ kmalloc(dst_buf, 4096)
  │
  ├─ alloc_chrdev_region()
  ├─ cdev_add()
  ├─ class_create() / device_create()
  └─ /dev/dma_mem2mem ready

Userspace:  echo "hello" > /dev/dma_mem2mem
               │
               ▼
          dev_write() [kernel]
               │
               ├─ copy_from_user(src_buf, ubuf)     ← user data → src_buf
               │
               ├─ dma_map_single(src, DMA_TO_DEVICE)    ← cache flush
               ├─ dma_map_single(dst, DMA_FROM_DEVICE)  ← cache invalidate
               │
               ├─ dmaengine_prep_dma_memcpy(dst_dma, src_dma, len)
               │         └─ builds HW descriptor for DMA controller
               │
               ├─ dmaengine_submit(desc)     ← queue descriptor
               ├─ dma_async_issue_pending()  ← KICK hardware
               │
               │         ┌─────────────────────────────┐
               │         │  STM32 DMA3 hardware copies  │
               │         │  src_buf → dst_buf           │
               │         │  (no CPU involvement)        │
               │         └─────────────────────────────┘
               │
               ├─ wait_for_completion_timeout()   ← sleeps, waiting for IRQ
               │         └─ DMA ISR fires dma_callback() → complete()
               │
               ├─ dma_unmap_single(dst, DMA_FROM_DEVICE)  ← cache sync to CPU
               ├─ dma_unmap_single(src, DMA_TO_DEVICE)
               │
               └─ data_len = len

Userspace:  cat /dev/dma_mem2mem
               │
               ▼
          dev_read() [kernel]
               │
               └─ copy_to_user(ubuf, dst_buf)   ← DMA result → userspace

rmmod
  │
  ├─ device_destroy() / class_destroy()
  ├─ cdev_del()
  ├─ unregister_chrdev_region()
  ├─ kfree(src_buf), kfree(dst_buf)
  └─ dma_release_channel()
```

---

## Key APIs

| API | What it does |
|-----|-------------|
| `dma_request_chan_by_mask(&mask)` | Request a DMA channel supporting `DMA_MEMCPY` |
| `dma_map_single(dev, buf, len, dir)` | Map CPU buffer for DMA (cache flush/invalidate) |
| `dmaengine_prep_dma_memcpy(ch, dst, src, len, flags)` | Build a mem2mem transfer descriptor |
| `dmaengine_submit(desc)` | Queue descriptor on the channel |
| `dma_async_issue_pending(chan)` | Start the hardware transfer |
| `wait_for_completion_timeout()` | Sleep until DMA IRQ fires |
| `dma_unmap_single(dev, dma, len, dir)` | Sync buffer back to CPU |
| `dma_release_channel(chan)` | Return the channel to the pool |

---

## Build & Test

```bash
# Build
make

# Copy to board
scp Ukernel_dma_mem2mem.ko root@192.168.50.2:/root/
scp dma_test.c            root@192.168.50.2:/root/

# On board
insmod Ukernel_dma_mem2mem.ko
dmesg | tail

# Compile userspace tester (on board, if gcc present)
gcc dma_test.c -o dma_test
./dma_test "Hello DMA"

# Or use shell directly
echo "Hello DMA" > /dev/dma_mem2mem
cat /dev/dma_mem2mem

rmmod Ukernel_dma_mem2mem
```

---

## Expected dmesg

```
dma_mem2mem: got DMA channel dma3chan0
dma_mem2mem: loaded, /dev/dma_mem2mem ready
dma_mem2mem: DMA copied 10 bytes (src -> dst)
```

---

## Interview Questions & Answers

**Q1. What is streaming DMA and how is it different from coherent DMA?**

> Streaming DMA (dma_map_single) maps a CPU buffer for a single transfer. The
> CPU owns the buffer normally; during the transfer ownership moves to the device.
> Coherent DMA (dma_alloc_coherent) allocates a buffer that is always coherent
> between CPU and device, removing the need to flush/invalidate caches. Streaming
> is cheaper and preferred for one-shot transfers; coherent is simpler but wastes
> memory and may slow the CPU cache.

**Q2. Why do we call dma_map_single for the destination with DMA_FROM_DEVICE?**

> The direction tells the kernel which cache maintenance to perform.
> DMA_TO_DEVICE flushes the cache so the device sees fresh CPU-written data.
> DMA_FROM_DEVICE invalidates the cache so the CPU sees fresh device-written data
> after unmap. Using the wrong direction can cause data corruption silently.

**Q3. What is a dma_cookie_t and why do we check it?**

> A dma_cookie_t is a handle returned by dmaengine_submit. It uniquely identifies
> the queued transfer. dma_submit_error(cookie) detects immediate failure (channel
> error, full queue). Later dma_async_is_tx_complete(chan, cookie) lets you poll
> whether the specific transfer finished.

**Q4. What happens if we forget to call dma_unmap_single after the transfer?**

> The DMA mapping remains active. The cache line is not invalidated so the CPU may
> read stale data from the destination buffer. The IOMMU mapping is not released,
> causing resource leaks. On DMA debug-enabled kernels the kernel will warn about
> the leaked mapping.

**Q5. Why do we use wait_for_completion_timeout instead of busy-polling?**

> wait_for_completion_timeout puts the calling thread to sleep and is woken up by
> the DMA interrupt handler calling complete(). Busy-polling wastes CPU cycles and
> can block other kernel work. Using a timeout prevents the kernel from sleeping
> forever if the DMA hardware hangs.

**Q6. What does DMA_PREP_INTERRUPT | DMA_CTRL_ACK mean in dmaengine_prep_dma_memcpy?**

> DMA_PREP_INTERRUPT tells the DMA engine to generate a completion interrupt and
> call the descriptor callback when the transfer finishes. Without it the callback
> may never fire. DMA_CTRL_ACK tells the engine it may recycle or free the
> descriptor after the callback executes. Both flags are mandatory for
> interrupt-driven completion with a callback.

**Q7. Why is dmaengine_terminate_sync called on timeout instead of just returning?**

> A timed-out transfer may still be running in hardware. If we return without
> stopping it, the DMA engine may write to dst_buf after we have freed it or
> repurposed it, causing memory corruption. dmaengine_terminate_sync stops the
> channel and waits until any in-progress callback has completed before returning,
> making cleanup safe.

**Q8. What is the role of dma_cap_mask_t and dma_cap_set?**

> dma_cap_mask_t is a bitmask of capabilities the caller requires from a DMA
> channel (DMA_MEMCPY, DMA_SLAVE, DMA_CYCLIC, etc.). dma_cap_set(DMA_MEMCPY, mask)
> sets the bit that means the channel must support memory-to-memory copies.
> dma_request_chan_by_mask then searches all registered DMA controllers and returns
> the first channel that satisfies all bits in the mask.

**Q9. Can two modules request the same DMA channel simultaneously?**

> It depends on whether the controller sets DMA_PRIVATE. If set, channels are
> exclusive and the second request fails. If not set, the dmaengine reference
> counting allows sharing but this is rare for memory-to-memory channels.
> Practically, on STM32 the dma3 channels are treated as private, so one at a time.

**Q10. Why is reinit_completion used inside dev_write instead of init_completion?**

> init_completion initialises a fresh completion object and should only be called
> once. Calling it again while another thread waits would corrupt the wait queue.
> reinit_completion atomically resets the done flag to zero, safely reusing the
> same completion object for the next transfer without reinitialising the wait
> queue. It is the correct pattern for reusable completions inside a function
> called repeatedly.
