# Lesson 21b — DMAEngine Mem2Mem: Kernel-Only (no userspace)

## What this is

A **standalone kernel module** that performs a complete memory-to-memory
streaming DMA transfer entirely inside `module_init`. No char device, no
userspace interaction — just `insmod` and read `dmesg`.

- Fills a source buffer with a string in the CPU.
- Maps both buffers with streaming DMA.
- Commands the SoC DMA controller (`stm32-dma3`) to copy src → dst in hardware.
- Waits for the hardware IRQ callback.
- Unmaps and `memcmp`s to prove the copy was correct.

---

## Files

| File | Role |
|------|------|
| `dma_mem2mem.c` | Kernel module — pure kernel, no /dev node |
| `Makefile` | Kernel build system |

---

## Flow Diagram

```
insmod
  │
  ▼
module_init / dma_m2m_init()
  │
  ├─ dma_cap_zero(mask)
  ├─ dma_cap_set(DMA_MEMCPY, mask)
  ├─ dma_request_chan_by_mask(&mask)
  │         └─ returns stm32-dma3 channel (e.g. "dma3chan0")
  │
  ├─ kmalloc(src_buf, 256)    ← physically contiguous, DMA-able
  ├─ kmalloc(dst_buf, 256)
  │
  ├─ strscpy(src_buf, "STM32MP257F-DK: real streaming DMA...")
  │
  │         ┌─────────── STREAMING DMA MAPPING ───────────┐
  ├─ dma_map_single(dev, src_buf, len, DMA_TO_DEVICE)
  │         │  cache line flushed: device will see CPU data │
  ├─ dma_map_single(dev, dst_buf, len, DMA_FROM_DEVICE)
  │         │  cache invalidated: CPU will see device data  │
  │         └─────────────────────────────────────────────┘
  │
  ├─ dmaengine_prep_dma_memcpy(chan, dst_dma, src_dma, len,
  │                             DMA_PREP_INTERRUPT | DMA_CTRL_ACK)
  │         └─ DMA engine builds a hardware descriptor
  │
  ├─ desc->callback = dma_mem2mem_callback   ← IRQ will call this
  ├─ dmaengine_submit(desc)                  ← queue descriptor
  ├─ dma_async_issue_pending(chan)            ← KICK hardware
  │
  │         ┌──────────────────────────────────────────────┐
  │         │   STM32-DMA3 hardware executes the copy:     │
  │         │   src_dma  ──────────────────►  dst_dma      │
  │         │   (CPU is doing nothing here)                │
  │         └──────────────────────────────────────────────┘
  │
  ├─ wait_for_completion_timeout(&dma_done, 3000ms)
  │         └─ CPU sleeps in this call
  │                   │
  │                   │  DMA controller fires interrupt
  │                   │       │
  │                   │  dma_mem2mem_callback()
  │                   │       └─ complete(&dma_done)   ← wake up!
  │                   │
  │         CPU wakes, continues
  │
  ├─ dma_async_is_tx_complete()   ← verify cookie status
  │
  ├─ dma_unmap_single(dst, DMA_FROM_DEVICE)  ← cache invalidated → CPU sees data
  ├─ dma_unmap_single(src, DMA_TO_DEVICE)
  │
  ├─ pr_info("dst after = %s", dst_buf)
  ├─ memcmp(src_buf, dst_buf) == 0 → "SUCCESS"
  │
  ├─ kfree(src_buf), kfree(dst_buf)
  └─ return 0   (module stays loaded, channel still held)

rmmod
  │
  ▼
module_exit / dma_m2m_exit()
  │
  └─ dma_release_channel(dma_chan)
```

---

## Key APIs

| API | What it does |
|-----|-------------|
| `dma_cap_set(DMA_MEMCPY, mask)` | Request only channels capable of mem-to-mem copy |
| `dma_request_chan_by_mask(&mask)` | Allocate the first matching channel |
| `dma_map_single(dev, buf, len, dir)` | Streaming map: hand buffer to device, perform cache maintenance |
| `dmaengine_prep_dma_memcpy(ch, dst, src, len, flags)` | Build hardware transfer descriptor |
| `dmaengine_submit(desc)` | Enqueue descriptor on the channel |
| `dma_async_issue_pending(chan)` | Signal controller to start queued work |
| `wait_for_completion_timeout(&c, ms)` | Sleep until callback calls complete() |
| `dma_async_is_tx_complete(ch, cookie)` | Confirm the specific transfer finished |
| `dma_unmap_single(dev, dma, len, dir)` | Return buffer ownership to CPU, sync cache |
| `dma_release_channel(chan)` | Free the DMA channel back to the pool |

---

## Build & Test

```bash
# Build (on your PC, cross-compile)
./make_build.sh kb 21b.DMAEngine_Mem2mem/

# Copy to board
scp ___output_files/Ukernel_dma_mem2mem.ko root@192.168.50.2:/root/

# On the board
dmesg -C                          # clear log
insmod Ukernel_dma_mem2mem.ko
dmesg | tail -20
rmmod Ukernel_dma_mem2mem
dmesg | tail -3
```

---

## Expected dmesg

```
dma_m2m: init - requesting a DMA_MEMCPY-capable channel
dma_m2m: acquired channel "dma3chan0"
dma_m2m: src before = "STM32MP257F-DK: real streaming DMA mem2mem transfer!"
dma_m2m: dst before = ""
dma_m2m: transfer issued, waiting for completion...
dma_m2m: dst after  = "STM32MP257F-DK: real streaming DMA mem2mem transfer!"
dma_m2m: SUCCESS - hardware DMA copy verified!
dma_m2m: module loaded
```

After rmmod:
```
dma_m2m: module removed
```

---

## Coherent vs Streaming DMA — Summary

| | Coherent | Streaming (this lesson) |
|--|----------|------------------------|
| API | `dma_alloc_coherent` | `dma_map_single` |
| Buffer ownership | Shared (always coherent) | Exclusive per-transfer |
| Cache maintenance | None needed | Flush/invalidate on map/unmap |
| Memory | Special non-cached allocation | Normal `kmalloc` |
| Use case | Ring buffers, descriptors | One-shot transfers |
| Cost | Higher (non-cached mapping) | Lower (normal memory) |

---

## Interview Questions & Answers

**Q1. What does module_init do in this driver and when does the DMA transfer run?**

> module_init registers dma_m2m_init as the function that runs when the module is
> loaded with insmod. The entire DMA transfer — channel request, buffer allocation,
> mapping, descriptor programming, hardware kick, wait, verify, and free — runs
> inside that single init function. The transfer completes before insmod returns to
> the shell.

**Q2. Why can we use kmalloc for DMA buffers here instead of dma_alloc_coherent?**

> kmalloc returns physically contiguous, DMA-capable memory on most ARM platforms.
> Because we use streaming DMA with dma_map_single, the kernel performs the
> necessary cache maintenance (flush on DMA_TO_DEVICE, invalidate on
> DMA_FROM_DEVICE) at map and unmap time. dma_alloc_coherent would work too but
> allocates from a special non-cached pool which is wasteful for a short one-shot
> transfer.

**Q3. What is the purpose of DMA_TO_DEVICE and DMA_FROM_DEVICE?**

> They tell the kernel which direction data flows so it performs the right cache
> operation. DMA_TO_DEVICE: the CPU has written data, so the kernel flushes the
> cache line to RAM so the DMA controller reads the correct value. DMA_FROM_DEVICE:
> the DMA controller will write data, so the kernel invalidates the cache line so
> the CPU does not read a stale cached value after unmap.

**Q4. What is a dmaengine descriptor and why is DMA_CTRL_ACK needed?**

> A descriptor (struct dma_async_tx_descriptor) packages all parameters of a
> transfer: source address, destination address, length, flags, and callback.
> DMA_CTRL_ACK tells the DMA engine it is safe to free or reuse the descriptor
> memory once the callback has been called. Without it some drivers hold the
> descriptor indefinitely, causing resource leaks.

**Q5. What is dma_async_issue_pending and why is it a separate call from submit?**

> dmaengine_submit only adds the descriptor to the channel's software queue.
> dma_async_issue_pending pushes all queued descriptors to the actual hardware
> channel registers, starting the transfer. This separation allows batching:
> multiple descriptors can be submitted before any hardware work starts, giving
> the DMA controller a chain to execute without software intervention between steps.

**Q6. What happens between dma_async_issue_pending and the callback firing?**

> The DMA controller reads source memory at src_dma, writes it to destination
> memory at dst_dma, entirely without CPU involvement. When the last byte is
> copied, the controller signals a completion interrupt. The interrupt handler in
> the stm32-dma3 driver runs, finds the completed descriptor, and calls its
> callback (dma_mem2mem_callback), which calls complete() to wake the waiting
> thread.

**Q7. What would happen if we read dst_buf between dma_async_issue_pending and dma_unmap_single?**

> It is undefined behaviour. The buffer is owned by the device during the
> transfer. The CPU cache may hold a stale copy. Even if the transfer is done,
> without calling dma_unmap_single (which invalidates the cache for
> DMA_FROM_DEVICE), the CPU may read the old cached value rather than the
> freshly DMA'd data. This is a common source of subtle data-corruption bugs.

**Q8. Why is memcmp used after the transfer instead of strcmp?**

> memcmp compares a fixed number of bytes and detects NUL bytes embedded in the
> buffer. strcmp would stop at the first NUL, missing any corruption after it.
> Since DMA_BUF_SIZE is the true extent of the transfer, memcmp over the full
> buffer size gives a complete correctness check.

**Q9. What is dma_cookie_t and what does dma_async_is_tx_complete tell us?**

> dma_cookie_t is a monotonically increasing integer that the DMA engine assigns
> to each submitted descriptor. dma_async_is_tx_complete(chan, cookie, NULL, NULL)
> queries whether the transfer identified by that cookie has reached DMA_COMPLETE
> status. It is a post-completion sanity check on top of the callback because a
> buggy driver could call the callback before truly finishing the transfer.

**Q10. What is the difference between dmaengine_terminate_async and dmaengine_terminate_sync?**

> dmaengine_terminate_async stops the channel and returns immediately, without
> waiting for any in-progress callback to finish. dmaengine_terminate_sync does the
> same but also waits until any currently running callback has returned before the
> function returns. In our timeout path we must use terminate_sync because we are
> about to free buffers that the callback might still be accessing. Using the async
> version would be a use-after-free race condition.
