What Problem Does Regcache Solve?

Imagine this sequence.

	CPU
	↓
	Write Register
	↓
	Sensor Goes to Suspend
	↓
	Power Removed
	↓
	All Registers Reset
	↓
	Resume

The device has forgotten everything.

Example
Before Suspend
	PWR_MGMT1 = 0x01
	GYRO_CONFIG = 0x18
	ACCEL_CONFIG = 0x01

After Resume
	PWR_MGMT1 = 0x40
	GYRO_CONFIG = 0x00
	ACCEL_CONFIG = 0x00

Everything is lost.



Without Regcache

Driver must remember
	Register A
	Register B
	Register C
	...
	Register N

Then during resume
	Write Register A
	Write Register B
	Write Register C
	...
	Write Register N

Imagine a codec with 500 registers.

Impossible to maintain manually.



Regcache
Regmap internally stores
	Register
	↓
	Value

Whenever you call
	regmap_write()

it stores
	Cache
	+
	Hardware

Later,
	Hardware Lost
	↓
	regcache_sync()
	↓
	Rewrite Everything



Cache Types
REGCACHE_NONE
	Every Read
	↓
	Hardware

REGCACHE_FLAT
Array
	Register
	↓
	Value
Fast
More RAM

REGCACHE_RBTREE ⭐⭐⭐⭐⭐
Red Black Tree
	Register
	↓
	Value
Less RAM
Slightly slower
Most commonly used.

REGCACHE_COMPRESSED
Compressed cache.
Rarely used.



Mark Device Dirty
Suppose power disappeared.
Tell Regmap
	regcache_mark_dirty(data->regmap);

Meaning
	Cache
	Correct
	Hardware
	Wrong


Synchronize
	ret = regcache_sync(data->regmap);
Internally
	Register1
	↓
	Write
	↓
	Register2
	↓
	Write
	↓
	Register3
	↓
	Write


Where is Regcache Used?
Almost everywhere.
Examples
	PMIC Drivers ⭐⭐⭐⭐⭐
	Audio Codec Drivers ⭐⭐⭐⭐⭐
	Camera Sensors ⭐⭐⭐⭐☆
	RTC Drivers ⭐⭐⭐⭐☆
	Touchscreen Controllers ⭐⭐⭐⭐☆
	Display Bridges ⭐⭐⭐⭐☆


Interview Diagram
	Before Suspend
	↓
	CPU
	↓
	Regmap
	↓
	Hardware
	---------------------
	Suspend
	↓
	Hardware OFF
	↓
	Registers Lost
	---------------------
	Resume
	↓
	Regcache
	↓
	Rewrite Registers
	↓
	Hardware Restored


Interview Questions
1. Why Regcache?

To preserve register values across suspend/resume or hardware reset, avoiding manual restoration of every register.

2. Which cache type is most common?
REGCACHE_RBTREE
3. Why call regcache_mark_dirty()?

It tells Regmap that the hardware no longer matches the cached values and must be rewritten during synchronization.

4. What does regcache_sync() do?

It writes the cached register values back to the hardware, restoring the device configuration.

5. Why use regcache_cache_only()?

During suspend or when the hardware is inaccessible, register accesses update only the cache instead of attempting bus transactions.