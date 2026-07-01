/*
 * dma_mem2mem.c  -  Lesson 22: REAL streaming DMA in kernel space
 *
 * Board: STM32MP257F-DK   (DMA controller: stm32-dma3 / stm32-mdma)
 *
 * WHAT THIS DOES (kernel-only, no userspace):
 *   1. Requests a DMA channel that can do memory-to-memory copies (DMA_MEMCPY).
 *   2. Allocates a source and a destination buffer with kmalloc.
 *   3. Fills the source buffer with a string.
 *   4. Uses STREAMING DMA mapping (dma_map_single) to hand both buffers to
 *      the DMA engine.
 *   5. Programs the DMA controller to copy src -> dst entirely in hardware.
 *   6. Waits for the hardware to signal completion (via a callback).
 *   7. Unmaps (which syncs the destination back to the CPU) and verifies
 *      that the copy is correct.
 *
 * This is the genuine "do streaming DMA yourself" exercise: YOUR code drives
 * the SoC DMA controller. (The Ethernet path uses the same idea internally,
 * but there stmmac owns the DMA - here you own it.)
 *
 * Streaming DMA vs coherent DMA:
 *   - Coherent (dma_alloc_coherent): buffer kept cache-coherent the whole time.
 *   - Streaming (dma_map_single):    buffer ownership is handed to the device
 *     for one transfer, then handed back to the CPU on unmap. Cheaper, and the
 *     normal choice for one-shot transfers like this.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/completion.h>
#include <linux/string.h>

#define DMA_BUF_SIZE   256
#define DMA_TIMEOUT_MS 3000

static struct dma_chan   *dma_chan;
static char              *src_buf;
static char              *dst_buf;
static struct completion  dma_done;

/* Called by the DMA engine (in interrupt context) when the copy finishes. */
static void dma_mem2mem_callback(void *param)
{
	complete(&dma_done);
}

static int __init dma_m2m_init(void)
{
	dma_cap_mask_t mask;
	struct dma_async_tx_descriptor *desc;
	dma_addr_t src_dma, dst_dma;
	dma_cookie_t cookie;
	enum dma_status status;
	struct device *dev;
	unsigned long tmo;
	int ret = 0;

	pr_info("dma_m2m: init - requesting a DMA_MEMCPY-capable channel\n");

	/* Ask the dmaengine core for any channel that supports memcpy. */
	dma_cap_zero(mask);
	dma_cap_set(DMA_MEMCPY, mask);

	dma_chan = dma_request_chan_by_mask(&mask);
	if (IS_ERR(dma_chan)) {
		ret = PTR_ERR(dma_chan);
		pr_err("dma_m2m: no memcpy-capable DMA channel available (%d)\n",
		       ret);
		dma_chan = NULL;
		return ret;
	}

	dev = dma_chan->device->dev;
	pr_info("dma_m2m: acquired channel \"%s\"\n", dma_chan_name(dma_chan));

	/* kmalloc gives physically contiguous, DMA-able memory. */
	src_buf = kmalloc(DMA_BUF_SIZE, GFP_KERNEL);
	dst_buf = kmalloc(DMA_BUF_SIZE, GFP_KERNEL);
	if (!src_buf || !dst_buf) {
		ret = -ENOMEM;
		goto err_free;
	}

	memset(src_buf, 0, DMA_BUF_SIZE);
	memset(dst_buf, 0, DMA_BUF_SIZE);
	strscpy(src_buf, "STM32MP257F-DK: real streaming DMA mem2mem transfer!",
		DMA_BUF_SIZE);

	pr_info("dma_m2m: src before = \"%s\"\n", src_buf);
	pr_info("dma_m2m: dst before = \"%s\"\n", dst_buf);

	/*
	 * STREAMING DMA MAPPING.
	 * src is read by the device  -> DMA_TO_DEVICE
	 * dst is written by the device -> DMA_FROM_DEVICE
	 * After this call the CPU must NOT touch the buffers until unmap.
	 */
	src_dma = dma_map_single(dev, src_buf, DMA_BUF_SIZE, DMA_TO_DEVICE);
	if (dma_mapping_error(dev, src_dma)) {
		pr_err("dma_m2m: dma_map_single(src) failed\n");
		ret = -ENOMEM;
		goto err_free;
	}

	dst_dma = dma_map_single(dev, dst_buf, DMA_BUF_SIZE, DMA_FROM_DEVICE);
	if (dma_mapping_error(dev, dst_dma)) {
		pr_err("dma_m2m: dma_map_single(dst) failed\n");
		ret = -ENOMEM;
		goto err_unmap_src;
	}

	/* Build a memory-to-memory transfer descriptor. */
	desc = dmaengine_prep_dma_memcpy(dma_chan, dst_dma, src_dma,
					 DMA_BUF_SIZE,
					 DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!desc) {
		pr_err("dma_m2m: dmaengine_prep_dma_memcpy failed\n");
		ret = -EIO;
		goto err_unmap_dst;
	}

	/* Register our completion callback. */
	init_completion(&dma_done);
	desc->callback = dma_mem2mem_callback;
	desc->callback_param = NULL;

	/* Queue the descriptor on the channel. */
	cookie = dmaengine_submit(desc);
	if (dma_submit_error(cookie)) {
		pr_err("dma_m2m: dmaengine_submit failed\n");
		ret = -EIO;
		goto err_unmap_dst;
	}

	/* Tell the controller to start processing queued transfers. */
	dma_async_issue_pending(dma_chan);
	pr_info("dma_m2m: transfer issued, waiting for completion...\n");

	/* Block until the hardware callback fires (or we time out). */
	tmo = wait_for_completion_timeout(&dma_done,
					  msecs_to_jiffies(DMA_TIMEOUT_MS));
	if (tmo == 0) {
		pr_err("dma_m2m: transfer timed out\n");
		dmaengine_terminate_sync(dma_chan);
		ret = -ETIMEDOUT;
		goto err_unmap_dst;
	}

	status = dma_async_is_tx_complete(dma_chan, cookie, NULL, NULL);

	/*
	 * Unmap hands the buffers back to the CPU. For DMA_FROM_DEVICE this
	 * also invalidates the cache so the CPU sees the freshly DMA'd data.
	 */
	dma_unmap_single(dev, dst_dma, DMA_BUF_SIZE, DMA_FROM_DEVICE);
	dma_unmap_single(dev, src_dma, DMA_BUF_SIZE, DMA_TO_DEVICE);

	if (status != DMA_COMPLETE) {
		pr_err("dma_m2m: transfer did not complete (status=%d)\n",
		       status);
		ret = -EIO;
		goto err_free;
	}

	pr_info("dma_m2m: dst after  = \"%s\"\n", dst_buf);

	if (memcmp(src_buf, dst_buf, DMA_BUF_SIZE) == 0)
		pr_info("dma_m2m: SUCCESS - hardware DMA copy verified!\n");
	else
		pr_err("dma_m2m: MISMATCH - DMA copy is incorrect\n");

	/* Buffers were only needed for this transfer. */
	kfree(src_buf);
	kfree(dst_buf);
	src_buf = NULL;
	dst_buf = NULL;

	pr_info("dma_m2m: module loaded\n");
	return 0;

err_unmap_dst:
	dma_unmap_single(dev, dst_dma, DMA_BUF_SIZE, DMA_FROM_DEVICE);
err_unmap_src:
	dma_unmap_single(dev, src_dma, DMA_BUF_SIZE, DMA_TO_DEVICE);
err_free:
	kfree(src_buf);
	kfree(dst_buf);
	src_buf = NULL;
	dst_buf = NULL;
	if (dma_chan) {
		dma_release_channel(dma_chan);
		dma_chan = NULL;
	}
	return ret;
}

static void __exit dma_m2m_exit(void)
{
	if (dma_chan) {
		dma_release_channel(dma_chan);
		dma_chan = NULL;
	}
	pr_info("dma_m2m: module removed\n");
}

module_init(dma_m2m_init);
module_exit(dma_m2m_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Lesson 22: real memory-to-memory streaming DMA using the dmaengine API (stm32-dma3/mdma)");
