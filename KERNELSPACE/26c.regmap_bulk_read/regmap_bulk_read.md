This API is heavily used in:

	MPU6050 / MPU9250
	BMI160
	BMA400
	PMICs
	Audio codecs
	RTCs

Why do we need it?
Suppose we want to read
	ACCEL_X
	ACCEL_Y
	ACCEL_Z

Registers
	ACCEL_XOUT_H   0x3B
	ACCEL_XOUT_L   0x3C

	ACCEL_YOUT_H   0x3D
	ACCEL_YOUT_L   0x3E

	ACCEL_ZOUT_H   0x3F
	ACCEL_ZOUT_L   0x40

Without bulk read, we'd do:
	regmap_read(map, 0x3B, &xh);
	regmap_read(map, 0x3C, &xl);

	regmap_read(map, 0x3D, &yh);
	regmap_read(map, 0x3E, &yl);

	regmap_read(map, 0x3F, &zh);
	regmap_read(map, 0x40, &zl);

That is
	6 I²C transactions

Each transaction has:

	START

	Address

	Register

	Repeated START

	Read

	STOP

Lots of overhead.



Better Way

The MPU supports auto-increment register addressing.

If you start reading from

	0x3B

it automatically sends
	0x3B
	↓
	0x3C
	↓
	0x3D
	↓
	0x3E
	↓
	0x3F
	↓
	0x40

Therefore,

	ONE I²C transaction
	↓
	6 bytes


Regmap API
	int regmap_bulk_read(struct regmap *map,
						unsigned int reg,
						void *val,
						size_t val_count);

Advantages:
	Faster
	Lower CPU overhead
	Lower I²C bus utilization
	Keeps X/Y/Z samples coherent (same sampling instant)



## Interview Questions & Answers

**Q1. Why use regmap_bulk_read() instead of multiple regmap_read() calls?**

> It performs a single bus transaction, reducing protocol overhead and improving performance.

**Q2. Why is regmap_bulk_read() ideal for sensors?**

> Sensors store multi-byte measurements in consecutive registers. Bulk reading retrieves
> a coherent snapshot of the measurement.

**Q3. Why combine bytes as (high << 8) | low?**

> The MPU stores measurements in big-endian register order (high byte first, then low byte).

**Q4. Why store the result in s16?**

> The accelerometer outputs signed 16-bit values representing positive and negative acceleration.

**Q5. Can regmap_bulk_read() read non-contiguous registers?**

> No. It reads a contiguous range of registers starting from the specified address.
> For non-contiguous registers, separate reads are required.