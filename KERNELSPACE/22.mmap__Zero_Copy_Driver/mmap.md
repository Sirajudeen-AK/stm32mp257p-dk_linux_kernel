# Linux Kernel `mmap()` -- Complete Interview Guide

> Production-oriented interview notes for Linux Kernel Driver
> Development.

------------------------------------------------------------------------

# 1. Why do we need `mmap()`?

Traditional data transfer:

``` text
Userspace Buffer
      |
copy_from_user()
      |
Kernel Buffer
      |
copy_to_user()
      |
Userspace Buffer
```

Two copies are required.

For a 4K frame (\~8 MB) at 60 FPS:

    8 MB × 60 = 480 MB/s

    User → Kernel = 480 MB/s
    Kernel → User = 480 MB/s

    Total ≈ 960 MB/s

The CPU spends significant time copying memory instead of doing useful
work.

## Solution: Zero-Copy

``` text
          Same Physical Memory

+-------------------------------+
|           DMA Buffer          |
+-------------------------------+
      ^                    ^
      |                    |
 Kernel Virtual      User Virtual
    Address             Address
```

Both virtual addresses refer to the same physical pages.

No `copy_to_user()`. No `copy_from_user()`.

------------------------------------------------------------------------

# 2. Architecture

``` text
Userspace
    |
 open()
    |
 mmap()
    |
---------------- System Call ----------------
    |
 VFS
    |
driver->mmap()
    |
remap_pfn_range()
    |
Page Tables Updated
    |
Same Physical Pages
```

------------------------------------------------------------------------

# 3. Driver Prototype

``` c
static int my_mmap(struct file *file,
                   struct vm_area_struct *vma);
```

Called whenever userspace executes `mmap()`.

------------------------------------------------------------------------

# 4. Typical Flow

``` text
open()
   |
mmap()
   |
Kernel mmap()
   |
Validate size
   |
Validate offset
   |
remap_pfn_range()
   |
Virtual address returned
   |
User accesses memory
   |
Kernel immediately sees updates
```

------------------------------------------------------------------------

# 5. Important APIs

  API                    Purpose
  ---------------------- ------------------------------------------------
  mmap()                 User requests mapping
  remap_pfn_range()      Maps physical pages into process address space
  dma_alloc_coherent()   Allocate coherent DMA memory
  dma_mmap_coherent()    Production DMA mapping helper
  vmalloc_to_page()      vmalloc translation
  virt_to_phys()         Demo/simple physically contiguous memory only

------------------------------------------------------------------------

# 6. Understanding vm_area_struct

Important fields:

  Field          Meaning
  -------------- -----------------------
  vm_start       Start virtual address
  vm_end         End virtual address
  vm_pgoff       Page offset requested
  vm_flags       Mapping permissions
  vm_page_prot   Page protection

Size:

``` c
size = vma->vm_end - vma->vm_start;
```

Reject oversized mappings.

------------------------------------------------------------------------

# 7. Why check vm_pgoff?

Educational drivers usually expose one page.

    Offset 0  -> valid
    Offset >0 -> reject

Prevents mapping invalid pages.

------------------------------------------------------------------------

# 8. remap_pfn_range()

Maps Page Frame Numbers (PFN) into userspace.

``` text
Physical Page
      |
 PFN
      |
remap_pfn_range()
      |
User Virtual Address
```

No memory copy occurs.

------------------------------------------------------------------------

# 9. Demo vs Production

Educational demo:

``` c
virt_to_phys(buffer);
remap_pfn_range(...);
```

Production DMA:

``` c
cpu = dma_alloc_coherent(...);

dma_mmap_coherent(...);
```

Why?

-   Architecture portable
-   Correct cache handling
-   DMA-safe
-   Recommended by kernel

------------------------------------------------------------------------

# 10. Cache Coherency

DMA engines bypass CPU caches.

Without coherent memory:

``` text
CPU Cache != RAM
```

Coherent DMA guarantees CPU and device observe consistent data.

------------------------------------------------------------------------

# 11. Real Driver Examples

-   GPU: VRAM mapped to userspace
-   Camera (V4L2): DMA buffers
-   PCIe FPGA: BAR regions
-   NIC: Packet rings
-   Display: Framebuffers
-   NVMe: Submission/completion queues

------------------------------------------------------------------------

# 12. Common Mistakes

-   Free mapped buffer too early
-   Wrong PFN
-   Mapping beyond allocated size
-   Forgetting page alignment
-   Using virt_to_phys() on vmalloc memory
-   Ignoring cache coherency
-   Missing VMA validation

------------------------------------------------------------------------

# 13. Security

Typical flags:

-   VM_IO
-   VM_DONTEXPAND
-   VM_DONTDUMP

Prevent unsafe behavior such as core dump leakage.

------------------------------------------------------------------------

# 14. Interview Questions & Answers

## Basic

**Q1. Why use mmap()?**

> To map kernel/device memory directly into a process's address space so userspace
> can access it without per-transfer copies, enabling zero-copy and high throughput.

**Q2. Difference between read() and mmap()?**

> read() copies data kernel→user on every call. mmap() maps the same physical
> memory once; afterwards userspace accesses it directly with no syscall or copy.

**Q3. What is zero-copy?**

> Sharing the same physical pages between kernel and userspace so data is never
> duplicated through copy_to_user()/copy_from_user().

**Q4. What does remap_pfn_range() do?**

> It maps a range of physical page frame numbers into a user VMA, building the
> page-table entries that back the mapping.

**Q5. Why validate mapping size?**

> To ensure the process cannot map more than the driver's buffer, preventing
> out-of-bounds access to unrelated physical memory.

**Q6. Why validate vm_pgoff?**

> The offset selects which part of the buffer is mapped; an unchecked offset lets
> userspace map arbitrary physical memory, a serious security hole.

**Q7. Does mmap() eliminate copy_to_user()?**

> Yes for the data path — once mapped, userspace reads/writes the shared pages
> directly, so copy_to_user() is no longer needed for that buffer.

## Intermediate

**Q8. Difference between virtual and physical address?**

> A virtual address is what code uses and is translated by the MMU/page tables to
> a physical address, the actual location in RAM/device memory.

**Q9. What is PFN?**

> Page Frame Number — the physical address shifted right by PAGE_SHIFT, i.e. the
> index of a physical page. remap_pfn_range() works in PFNs.

**Q10. Why page aligned mappings?**

> The MMU maps memory at page granularity, so both the base address and size of a
> mapping must be page aligned.

**Q11. Why isn't virt_to_phys() recommended?**

> It only works for the linear kernel logical mapping (kmalloc), not for vmalloc,
> highmem, or device memory, so it is unsafe as a general translation.

**Q12. What is vm_area_struct?**

> The kernel structure describing one contiguous virtual memory region of a
> process, including its range, permissions, and vm_operations.

**Q13. Can vmalloc memory be mapped directly?**

> Not with remap_pfn_range(), because vmalloc pages are not physically contiguous.
> Use vm_insert_page()/remap_vmalloc_range() page by page instead.

**Q14. Difference between coherent and streaming DMA?**

> Coherent DMA memory is always cache-consistent between CPU and device; streaming
> DMA maps existing buffers per transfer and requires explicit cache maintenance.

## Advanced

**Q15. Explain page table changes during mmap().**

> The kernel creates a new VMA and populates PTEs mapping the user virtual range
> to the target physical PFNs, so subsequent accesses translate without faults.

**Q16. Explain TLB involvement.**

> The MMU caches recent translations in the TLB. New mappings may require TLB
> fills; changing or removing mappings requires TLB invalidation/flush.

**Q17. What happens after munmap()?**

> The VMA is removed, its PTEs are torn down, and the TLB is flushed; further
> access to that range faults (SIGSEGV). Underlying physical memory is freed by
> the owner, not by munmap of a device mapping.

**Q18. When should dma_mmap_coherent() be preferred?**

> When mapping DMA-coherent memory to userspace — it handles the correct PFNs,
> caching attributes, and offset semantics for coherent buffers automatically.

**Q19. How are BAR regions mapped?**

> PCI BAR physical addresses are mapped to userspace with io_remap_pfn_range()
> using non-cached/write-combining attributes appropriate for MMIO.

**Q20. Why does GPU rely heavily on mmap()?**

> GPUs move large framebuffers/textures; mapping that memory into userspace avoids
> huge per-frame copies and gives the application direct, high-bandwidth access.

------------------------------------------------------------------------

# 15. Bottleneck Questions

**Q. Why doesn't every driver expose mmap()?**

> Only drivers benefiting from shared memory or high-throughput transfers need it.

**Q. Does mmap() improve latency?**

> It mainly removes copy overhead and CPU utilization; latency may also improve.

**Q. Can multiple processes map the same buffer?**

> Yes, depending on driver implementation and synchronization.

**Q. Is synchronization still required?**

> Yes. mmap() shares memory, not locking.

**Q. Does mmap() allocate memory?**

> No. It maps existing memory into a process.

------------------------------------------------------------------------

# 16. Best Practices

-   Prefer dma_mmap_coherent() for DMA memory.
-   Validate offsets and sizes.
-   Never expose arbitrary physical memory.
-   Keep mappings page aligned.
-   Protect shared memory with proper synchronization.
-   Clean up mappings and allocations correctly.

------------------------------------------------------------------------

# 17. Quick Revision

  API                   Purpose
  --------------------- ------------------------
  mmap()                Request mapping
  remap_pfn_range()     Map PFN
  dma_mmap_coherent()   Production DMA mapping
  munmap()              Remove mapping

Memory copy comparison:

``` text
read()/write()

User
  |
copy
  |
Kernel
  |
copy
  |
User

mmap()

User VA --------\
                 > Same Physical Memory
Kernel VA ------/
```
