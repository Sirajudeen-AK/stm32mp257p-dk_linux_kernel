This API is commonly used in:

	PMIC drivers
	PHY drivers
	Clock drivers
	Flash drivers
	Camera drivers
	Sensor drivers


Why do we need it?

Suppose we issue a reset command.

	CPU
	│
	│ Write RESET bit
	▼
Device starts resetting...

The reset is not instantaneous.

It may take
	100 µs
	500 µs
	5 ms

depending on the hardware.

If we immediately access another register,
	CPU
	│
	│ Write RESET
	│
	├── Immediately Read Register
	▼
	Device still busy

the device may:
	return garbage
	NACK
	ignore the request


Bad Method
Many beginners write
	regmap_write(...);

	msleep(10);

	regmap_read(...);

Problems:
	Sleeps longer than necessary.
	Wastes CPU time.
	Different hardware revisions may need different delays.
	Not scalable.

Another Bad Method
	while (1)
	{
		regmap_read(...);

		if (...)
			break;
	}

Problems:
	Infinite loop if hardware hangs.
	100% CPU usage.
	No timeout.


Linux Solution
	regmap_read_poll_timeout()

It repeatedly reads a register until:
	a condition becomes true, or
	the timeout expires.




Example

Suppose
	PWR_MGMT_1

	Bit6
	↓
	Sleep Bit

We wake the chip.
	regmap_update_bits(data->regmap,
					MPU_PWR_MGMT1,
					BIT(6),
					0);

Now poll until Sleep bit becomes zero.
	unsigned int val;

	ret = regmap_read_poll_timeout(data->regmap,
								MPU_PWR_MGMT1,
								val,
								!(val & BIT(6)),
								1000,
								100000);

Meaning

	Read Register
	↓
	Sleep Bit cleared ?
	↓
	Yes
	↓
	Done

	Else
	↓
	Sleep 1ms
	↓
	Read Again
	↓
	Maximum 100ms

Success

	First Read

	0x41
	↓
	Second Read

	0x41
	↓
	Third Read

	0x01
	↓
	Condition True
	↓
	Return Success

Timeout
	0x41
	↓
	0x41
	↓
	0x41
	↓
	0x41
	↓
	...
	↓
	100 ms
	↓
	Timeout


Why not while()?
Suppose hardware fails.

	while()
	{
		regmap_read();
	}

Never exits.

Kernel thread hangs forever.



## Interview Questions & Answers

**Q1. Why use regmap_read_poll_timeout() instead of msleep()?**

> msleep() waits a fixed duration regardless of when the hardware becomes ready.
> regmap_read_poll_timeout() exits as soon as the condition is met, reducing unnecessary
> delays while still enforcing a timeout.

**Q2. Why not use an infinite polling loop?**

> An infinite loop can hang forever if the hardware never reaches the expected state.
> The timeout API prevents deadlocks.

**Q3. What does sleep_us control?**

> The delay between successive register reads. It balances bus traffic and CPU usage
> against responsiveness.

**Q4. What happens if the timeout expires?**

> The API returns -ETIMEDOUT, allowing the driver to report the failure and abort
> initialization cleanly.

**Q5. When should polling be used instead of interrupts?**

> Polling is appropriate for short, synchronous operations during initialization or
> configuration (such as reset completion or PLL lock). Interrupts are preferred for
> asynchronous events like data-ready or button presses during normal operation.