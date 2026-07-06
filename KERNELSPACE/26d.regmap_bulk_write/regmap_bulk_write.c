
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/of.h>

#define MPU_WHO_AM_I      0x75
#define MPU_PWR_MGMT1     0x6B

#define MPU_SMPLRT_DIV      0x19
#define MPU_CONFIG          0x1A
#define MPU_GYRO_CONFIG     0x1B
#define MPU_ACCEL_CONFIG    0x1C

u8 config[] =
{
    0x07,
    0x06,
    0x18,
    0x01
};

struct mpu9250_data
{
    struct i2c_client *client;
    struct regmap *regmap;
};

static const struct regmap_config mpu_regmap_config =
{
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = 0x7F,
};

void mpu9250_read(struct mpu9250_data *data, struct i2c_client *client){

	/* Verify Configuration Data - Bulk */
	u8 verify[4];

	int ret = 0;

	ret = regmap_bulk_read(data->regmap,
						MPU_SMPLRT_DIV,
						verify,
						sizeof(verify));

	if (ret)
	{
		dev_err(&client->dev,
				"Verification Failed\n");
		return;
	}

	dev_info(&client->dev,
			"SMPLRT_DIV  = 0x%02x\n",
			verify[0]);

	dev_info(&client->dev,
			"CONFIG      = 0x%02x\n",
			verify[1]);

	dev_info(&client->dev,
			"GYRO_CONFIG = 0x%02x\n",
			verify[2]);

	dev_info(&client->dev,
			"ACCEL_CFG   = 0x%02x\n",
			verify[3]);

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

	dev_info(&client->dev, "Before Bulk Write\n");
	mpu9250_read(data, client);
	/* Write Configuration Data - Bulk */
	ret = regmap_bulk_write(data->regmap,
						MPU_SMPLRT_DIV,
						config,
						sizeof(config));

	if (ret)
	{
		dev_err(&client->dev,
				"Bulk Write Failed\n");
		return ret;
	}

	dev_info(&client->dev, "After Bulk Write\n");
	mpu9250_read(data, client);

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
MODULE_DESCRIPTION("Regmap Bulk write Example");