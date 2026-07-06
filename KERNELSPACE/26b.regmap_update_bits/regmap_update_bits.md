Why do we need it?

Suppose register PWR_MGMT_1 (0x6B) currently contains

0x01

00000001

Meaning

CLKSEL = 1

Now you want to set only the SLEEP bit (bit 6).

Expected register

01000001

0x41

Notice

CLKSEL must remain 001.

Wrong Method

Many beginners do

regmap_write(map,
             MPU_PWR_MGMT1,
             0x40);

Result

01000000

Oops!

CLKSEL became 000

Previous configuration is lost.

------------------------------------------------------------------------------------------

Another Method (Manual RMW)
	regmap_read(map,
				MPU_PWR_MGMT1,
				&val);

	val |= BIT(6);

	regmap_write(map,
				MPU_PWR_MGMT1,
				val);

Works.

But there is a problem.

Race Condition

Suppose

	CPU0
	↓
	Read Register
	↓
	Interrupt occurs

ISR

	Changes Bit0

Back to CPU0

	Old Value
	↓
	Write Back

ISR modification is lost.

Classic

	Read
	Modify
	Write
	Race

------------------------------------------------------------------------------------------

Regmap Solution

	regmap_update_bits()

Kernel internally performs

	Lock
	↓
	Read
	↓
	Modify
	↓
	Write
	↓
	Unlock

Atomically.


Set Sleep Bit
Bit6
	ret = regmap_update_bits(data->regmap,
							MPU_PWR_MGMT1,
							BIT(6),
							BIT(6));
Only bit6 changes.


Clear Sleep Bit
	ret = regmap_update_bits(data->regmap,
							MPU_PWR_MGMT1,
							BIT(6),
							0);

Change Clock Source
Bits					2:0
Mask					00000111
Clock = PLL				001

	ret = regmap_update_bits(data->regmap,
							MPU_PWR_MGMT1,
							GENMASK(2,0),
							0x01);


## Interview Questions & Answers

**Q1. Difference between regmap_write() and regmap_update_bits()?**

> regmap_write() overwrites the entire register, while regmap_update_bits() modifies
> only the specified bits and preserves all others.

**Q2. Why not perform a manual read-modify-write?**

> A manual sequence is vulnerable to races if another context modifies the register
> between the read and write. regmap_update_bits() performs the operation under
> Regmap's internal locking.

**Q3. What do mask and val represent?**

> mask selects which bits are allowed to change; val provides the new values for those
> selected bits. Bits outside the mask remain unchanged.

**Q4. What is BIT(n)?**

> A kernel macro that expands to `1UL << n`, making single-bit operations more readable.

**Q5. What is GENMASK(h, l)?**

> A kernel macro that creates a contiguous bitmask from bit l through bit h.
> `GENMASK(2, 0)` produces `0b00000111` and is ideal for multi-bit fields such as CLKSEL.

> **Note:** This API is one of the most frequently used in PMIC, regulator, audio codec,
> RTC, and sensor drivers because hardware control registers almost always contain multiple
> unrelated bit fields that must be preserved while updating only one field.