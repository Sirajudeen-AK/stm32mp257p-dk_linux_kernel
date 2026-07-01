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

# 14. Interview Questions

## Basic

1.  Why use mmap()?
2.  Difference between read() and mmap()?
3.  What is zero-copy?
4.  What does remap_pfn_range() do?
5.  Why validate mapping size?
6.  Why validate vm_pgoff?
7.  Does mmap() eliminate copy_to_user()?

## Intermediate

8.  Difference between virtual and physical address?
9.  What is PFN?
10. Why page aligned mappings?
11. Why isn't virt_to_phys() recommended?
12. What is vm_area_struct?
13. Can vmalloc memory be mapped directly?
14. Difference between coherent and streaming DMA?

## Advanced

15. Explain page table changes during mmap().
16. Explain TLB involvement.
17. What happens after munmap()?
18. When should dma_mmap_coherent() be preferred?
19. How are BAR regions mapped?
20. Why does GPU rely heavily on mmap()?

------------------------------------------------------------------------

# 15. Bottleneck Questions

**Q:** Why doesn't every driver expose mmap()?

A: Only drivers benefiting from shared memory or high-throughput
transfers need it.

**Q:** Does mmap() improve latency?

A: It mainly removes copy overhead and CPU utilization; latency may also
improve.

**Q:** Can multiple processes map the same buffer?

A: Yes, depending on driver implementation and synchronization.

**Q:** Is synchronization still required?

A: Yes. mmap() shares memory, not locking.

**Q:** Does mmap() allocate memory?

A: No. It maps existing memory into a process.

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
