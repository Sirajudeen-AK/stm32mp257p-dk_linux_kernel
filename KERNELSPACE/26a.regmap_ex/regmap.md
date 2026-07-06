## Interview Questions & Answers — Introduction

**Q1. What is Regmap?**

> A Linux kernel abstraction layer that provides a common API for accessing device
> registers regardless of the underlying bus (I²C, SPI, MMIO, etc.).

**Q2. Why use Regmap?**

> Bus-independent drivers, cleaner code, built-in register caching and locking,
> bulk register access, bitfield updates, DebugFS support, and easier maintenance.

**Q3. Which drivers commonly use Regmap?**

> PMIC drivers, audio codec drivers, sensor drivers, RTC drivers, touchscreen
> controllers, display bridge chips, and GPIO expanders.

**Q4. What buses does Regmap support?**

> I²C, SPI, MMIO, SPMI, SoundWire, and others through backend implementations.

## Interview Questions & Answers — regmap_config

**Q1. Why store struct regmap *?**

> Because all register operations are performed through the Regmap API instead of
> directly calling I²C transfer functions.

**Q2. Why .reg_bits = 8?**

> The MPU-9250 uses 8-bit register addresses (0x00–0x7F).

**Q3. Why .val_bits = 8?**

> Each register stores an 8-bit value.

**Q4. What is max_register used for?**

> Regmap validates accesses. Any attempt to read or write beyond 0x7F is rejected
> before reaching the bus, helping catch bugs early.

## Interview Questions & Answers — Probe & Verify

**Q1. Why devm_regmap_init_i2c() instead of regmap_init_i2c()?**

> devm_ automatically frees the Regmap when the device is removed, reducing cleanup
> code and preventing resource leaks.

**Q2. Why i2c_set_clientdata()?**

> It associates the driver's private data with the i2c_client, allowing it to be
> retrieved later using i2c_get_clientdata() in callbacks such as remove or interrupt
> handlers.

**Q3. Why does regmap_read() use unsigned int instead of u8?**

> The Regmap API uses unsigned int as a generic container regardless of the register
> width. The driver interprets only the valid bits based on .val_bits.

**Q4. Why verify WHO_AM_I during probe?**

> To ensure the driver has bound to the expected hardware and to avoid programming an
> incompatible device.

**Q5. Why read back after writing PWR_MGMT_1?**

> It's a simple verification that the write succeeded and that the device accepted the
> new configuration. This is a common bring-up technique, although production drivers
> may skip read-back for performance once the hardware is trusted.
