
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/of.h>

#define MPU_WHO_AM_I      0x75
#define MPU_PWR_MGMT1     0x6B

#define MPU_INT_PIN_CFG        0x37
#define MPU_INT_ENABLE         0x38
#define MPU_INT_STATUS         0x3A	
#define MPU_ACCEL_XOUT_H       0x3B

struct mpu9250_data
{
    struct i2c_client *client;
    struct regmap *regmap;

    int irq;
};

static const struct regmap_config mpu_regmap_config =
{
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = 0x7F,
};

static irqreturn_t mpu9250_irq(int irq,
                               void *dev_id)
{
    struct mpu9250_data *data = dev_id;

    u8 accel[6];
    unsigned int status;

    s16 x;
    s16 y;
    s16 z;

    regmap_read(data->regmap,
                MPU_INT_STATUS,
                &status);

    if (!(status & 0x01))
        return IRQ_NONE;

    regmap_bulk_read(data->regmap,
                     MPU_ACCEL_XOUT_H,
                     accel,
                     sizeof(accel));

    x = (s16)((accel[0] << 8) | accel[1]);
    y = (s16)((accel[2] << 8) | accel[3]);
    z = (s16)((accel[4] << 8) | accel[5]);

    pr_info("Accel X=%d Y=%d Z=%d\n",
            x,
            y,
            z);

	msleep(1000); // Simulate some processing delay
	
    return IRQ_HANDLED;
}

static int mpu9250_probe(struct i2c_client *client)
{
    struct mpu9250_data *data;
    unsigned int val;
    int ret;

    dev_info(&client->dev, "Probe Started\n");

    data = devm_kzalloc(&client->dev,
                        sizeof(*data),
                        GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->client = client;

    data->regmap = devm_regmap_init_i2c(client,
                                        &mpu_regmap_config);

    if (IS_ERR(data->regmap))
    {
        dev_err(&client->dev,
                "Failed to create regmap\n");
        return PTR_ERR(data->regmap);
    }

    i2c_set_clientdata(client, data);

    ret = regmap_read(data->regmap,
                      MPU_WHO_AM_I,
                      &val);

    if (ret)
    {
        dev_err(&client->dev,
                "WHO_AM_I read failed\n");
        return ret;
    }

    dev_info(&client->dev,
             "WHO_AM_I = 0x%02x\n",
             val);

    if (val != 0x70 && val != 0x71)
    {
        dev_err(&client->dev,
                "Unsupported chip\n");
        return -ENODEV;
    }

	/* Configure interrupt pin:
	* Active High
	* Push-Pull
	* Pulse mode
	* Read INT_STATUS clears interrupt
	*/
	ret = regmap_write(data->regmap,
					MPU_INT_PIN_CFG,
					0x00);
	if (ret)
		return ret;

    /* Enable Interrupt */
	ret = regmap_write(data->regmap,
                   MPU_INT_ENABLE,
                   0x01);

	if (ret)
	{
		dev_err(&client->dev,
				"INT enable failed\n");
		return ret;
	}

	data->irq = client->irq;

	if (data->irq <= 0)
	{
		dev_err(&client->dev, "Invalid IRQ\n");
		return -EINVAL;
	}

	dev_info(&client->dev,
			"IRQ=%d\n",
			data->irq);

	ret = devm_request_threaded_irq(&client->dev,
                                data->irq,
                                NULL,
                                mpu9250_irq,
                                IRQF_ONESHOT |
                                IRQF_TRIGGER_FALLING,
                                "mpu9250_irq",
                                data);

	if (ret)
	{
		dev_err(&client->dev,
				"IRQ Request Failed\n");
		return ret;
	}

    dev_info(&client->dev,
             "Probe Successful\n");

    return 0;
}

static void mpu9250_remove(struct i2c_client *client)
{
    dev_info(&client->dev,
             "Remove Successful\n");
}

static const struct of_device_id mpu9250_of_match[] =
{
    {
        .compatible = "siraj,mpu9250",
    },
    { }
};

MODULE_DEVICE_TABLE(of, mpu9250_of_match);

static struct i2c_driver mpu9250_driver =
{
    .driver =
    {
        .name = "siraj_mpu9250",
        .of_match_table = mpu9250_of_match,
    },

    .probe = mpu9250_probe,

    .remove = mpu9250_remove,
};

module_i2c_driver(mpu9250_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Regmap Update Bits Example - Sleep and Wake Up Device");