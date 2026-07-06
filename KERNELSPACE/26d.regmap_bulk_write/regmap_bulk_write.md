
Why do we need it?

Suppose we need to configure the MPU during probe.

Registers:
| Register     | Address | Value |
| ------------ | ------- | ----: |
| SMPLRT_DIV   | 0x19    |  0x07 |
| CONFIG       | 0x1A    |  0x06 |
| GYRO_CONFIG  | 0x1B    |  0x18 |
| ACCEL_CONFIG | 0x1C    |  0x01 |

Without bulk write

	regmap_write(map, 0x19, 0x07);
	regmap_write(map, 0x1A, 0x06);
	regmap_write(map, 0x1B, 0x18);
	regmap_write(map, 0x1C, 0x01);

This means

	START

	WRITE

	STOP

	START

	WRITE

	STOP

	START

	WRITE

	STOP

	START

	WRITE

	STOP

4 I²C transactions


With Bulk Write

	START

	Address

	0x19

	0x07

	0x06

	0x18

	0x01

	STOP

Only one I²C transaction.


Advantages

Instead of

	4 START

	4 STOP

	4 Address Phase

We get

	1 START

	1 STOP

	1 Address Phase

This reduces:
	Bus overhead
	CPU overhead
	Probe time
	Power consumption


## Interview Questions & Answers

**Q1. When should regmap_bulk_write() be used?**

> When writing multiple consecutive registers in one transaction.

**Q2. Can it write non-consecutive registers?**

> No. Registers must be contiguous.

**Q3. What is the benefit over multiple regmap_write() calls?**

> Fewer I²C/SPI transactions, better performance, lower bus utilization, and lower CPU overhead.

**Q4. Why verify the write?**

> During driver bring-up, verification confirms the device accepted the configuration.
> Production drivers may omit this after validation to save bus transactions.

**Q5. What happens if the starting register is wrong?**

> Every value is written to the wrong register because the device auto-increments from
> the incorrect starting address.