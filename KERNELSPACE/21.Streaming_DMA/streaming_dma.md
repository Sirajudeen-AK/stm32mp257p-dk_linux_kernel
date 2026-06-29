# Program 21 — Streaming DMA

## 1. Goal

Learn the **second DMA model** used by high-performance drivers: allocate normal
memory (`kmalloc`), then map it for a single transfer with `dma_map_single()` and
unmap it with `dma_unmap_single()`. Understand the **CPU cache problem** this
solves and the **DMA direction** flags.

---

## 2. The Two DMA Models

**Model 1 — Coherent DMA** (Programs 19 & 20):

```
cpu = dma_alloc_coherent(...)

CPU ── virtual addr ──► DMA Buffer ◄── DMA addr ── Device
       (kernel keeps CPU cache and device coherent — simple)
```

**Model 2 — Streaming DMA** (this program): use ordinary memory, map per transfer:

```
kmalloc()
   ↓
Kernel Buffer
   ↓
dma_map_single()
   ↓
DMA Address
   ↓
Hardware
   ↓  (after transfer)
dma_unmap_single()
```

---

## 3. Why Two APIs? (performance)

Imagine an Ethernet driver handling a million packets:

```
Packet 1 ... Packet 1,000,000
```

If every packet needed `dma_alloc_coherent()` + `dma_free_coherent()`, it would be
very slow. Streaming DMA reuses one buffer:

```
kmalloc()  (once)
   ↓
Reuse Buffer  ◄─────────────┐
   ↓                        │
dma_map_single()            │
   ↓                        │
DMA transfer                │
   ↓                        │
dma_unmap_single()  ────────┘
```

No repeated allocation → much faster.

---

## 4. The CPU Cache Problem (the key concept)

```c
char buffer[100];
strcpy(buffer, "HELLO");
```

Many CPUs don't write to RAM immediately — the data sits in cache:

```
CPU
 ↓
CPU Cache   ← "HELLO" lives here
 ↓
RAM (updated later)
```

The DMA controller **cannot see the CPU cache** — it only reads RAM:

```
CPU Cache:  HELLO          DMA starts reading RAM
RAM:        (empty)   ──►  Hardware receives GARBAGE
```

### `dma_map_single()` fixes this

It performs the architecture's required **cache maintenance** (flush/invalidate),
then returns a DMA address, so everything is synchronized:

```
CPU Cache  ──flush──►  RAM  ──►  DMA  ──►  Device
```

---

## 5. DMA Directions

You must tell Linux who owns the data so it flushes/invalidates the right way:

| Direction            | Use case                          | Flow                          |
| -------------------- | --------------------------------- | ----------------------------- |
| `DMA_TO_DEVICE`      | CPU → Device (e.g. Ethernet TX)   | Userspace → Driver → Wire     |
| `DMA_FROM_DEVICE`    | Device → CPU (e.g. Camera RX)     | Camera → RAM → Userspace      |
| `DMA_BIDIRECTIONAL`  | Both directions                   | read + write                  |

---

## 6. Code Walkthrough

```c
struct my_device { void *buffer; dma_addr_t dma_addr; };

mydev->buffer = kmalloc(BUF_SIZE, GFP_KERNEL);   // 1. normal memory
strcpy(mydev->buffer, "Hello DMA Mapping");      // 2. CPU fills it

mydev->dma_addr = dma_map_single(&pdev->dev,     // 3. map + cache maintenance
                                 mydev->buffer, BUF_SIZE, DMA_TO_DEVICE);
if (dma_mapping_error(&pdev->dev, mydev->dma_addr)) {  // 4. ALWAYS check
    kfree(mydev->buffer);
    return -EIO;
}

/* ... real hardware DMA would happen here ... */

dma_unmap_single(&pdev->dev, mydev->dma_addr, BUF_SIZE, DMA_TO_DEVICE); // 5.
// later in remove():
kfree(mydev->buffer);                            // 6. free normal memory
```

Execution flow:

```
probe() → kmalloc() → CPU fills buffer → dma_map_single() → DMA address
        → (hardware DMA) → dma_unmap_single() → remove() → kfree()
```

---

## 7. Coherent DMA vs Streaming DMA

| Feature        | Coherent DMA                    | Streaming DMA                               |
| -------------- | ------------------------------- | ------------------------------------------- |
| Allocation     | `dma_alloc_coherent()`          | `kmalloc()`                                 |
| Mapping        | Automatic                       | `dma_map_single()`                          |
| Unmapping      | Automatic                       | `dma_unmap_single()`                        |
| Cache handling | Automatic                       | Done during map/unmap                       |
| Performance    | Simpler                         | Better for frequent transfers               |
| Common use     | DMA descriptors, control blocks | Network packets, storage I/O, USB transfers |

**Real driver usage:**

| Driver               | DMA Type           |
| -------------------- | ------------------ |
| Ethernet / NVMe / USB / PCIe | Streaming DMA |
| Camera frame buffers | Often Coherent DMA |
| DMA descriptor rings | Often Coherent DMA |

> Common pattern: **coherent** DMA for descriptor rings (both CPU and device update
> them constantly), **streaming** DMA for large payloads (packets, frames, blocks).

---

## 8. Commands / Testing

```bash
sudo insmod streaming_dma.ko
dmesg | tail            # see "CPU Buffer", "DMA Address", "Loaded"
sudo rmmod streaming_dma
```

---

## 9. Interview Questions & Answers

**Q1. Why not always use `dma_alloc_coherent()`?**

> Coherent buffers are simpler but have overhead and may be uncached (slower CPU
> access). For buffers allocated once and mapped/unmapped across many transfers —
> especially large or frequent ones — streaming DMA is faster and lets you reuse
> normal cached memory.

**Q2. What happens if you forget `dma_unmap_single()`?**

> The mapping stays active. On many architectures this leaves cache state
> inconsistent (the CPU may read stale data) or leaks IOMMU/mapping resources,
> eventually causing wrong data or resource exhaustion.

**Q3. Why call `dma_mapping_error()`?**

> Mapping can fail — e.g. the IOMMU can't create a mapping or the address isn't
> DMA-reachable. You must verify the returned `dma_addr_t` before using it,
> otherwise the device may DMA to an invalid address.

**Q4. Does `kmalloc()` return a DMA address?**

> No. It returns a CPU virtual address. Only `dma_map_single()` produces a DMA
> address the hardware can use.

**Q5. What is the CPU cache problem in streaming DMA?**

> The CPU may keep freshly written data in cache while RAM is stale. Since the DMA
> engine reads RAM (not the cache), it would transfer garbage. `dma_map_single()`
> performs cache flush/invalidate so RAM and cache agree before the transfer.

**Q6. What does the DMA direction argument control?**

> It tells the kernel which cache maintenance to perform: `DMA_TO_DEVICE` flushes
> CPU writes to RAM before the device reads; `DMA_FROM_DEVICE` invalidates the CPU
> cache so the CPU reads fresh device-written data; `DMA_BIDIRECTIONAL` does both.

**Q7. Why must you not touch the buffer with the CPU while it is mapped?**

> Between `dma_map_single()` and `dma_unmap_single()` the device owns the buffer.
> CPU access can race with the DMA and reintroduce cache incoherency. Ownership
> returns to the CPU only after unmap (or `dma_sync_single_for_cpu()`).