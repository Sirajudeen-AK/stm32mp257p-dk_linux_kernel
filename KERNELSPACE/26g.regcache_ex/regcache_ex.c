
/*
Driver flow:
	Probe
	↓
	regmap_init()
	↓
	Normal operation
	↓
	Suspend
		regcache_cache_only(true)
	↓
	Power to device removed
	↓
	Resume
		regcache_cache_only(false)
		regcache_mark_dirty()
		regcache_sync()

*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/of.h>

#define MPU_WHO_AM_I      0x75
#define MPU_PWR_MGMT1     0x6B

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

	.cache_type = REGCACHE_RBTREE,
};

static int mpu_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);

    struct mpu9250_data *data =
            i2c_get_clientdata(client);

	/*
	Flow
		Write
		↓
		Cache Only
		↓
		Don't Touch Hardware
	*/
    regcache_cache_only(data->regmap,
                        true);

    return 0;
}

static int mpu_resume(struct device *dev)
{
    struct i2c_client *client =
            to_i2c_client(dev);

    struct mpu9250_data *data =
            i2c_get_clientdata(client);

	/*
	Flow
		Resume
		↓
		Enable Hardware Access
		↓
		Hardware Dirty
		↓
		Rewrite Cached Registers
	*/
    regcache_cache_only(data->regmap,
                        false);

    regcache_mark_dirty(data->regmap);

    return regcache_sync(data->regmap);
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

    ret = regmap_read(data->regmap,
                      MPU_PWR_MGMT1,
                      &val);

    if (ret)
    {
        dev_err(&client->dev,
                "PWR_MGMT_1 read failed\n");
        return ret;
    }

    dev_info(&client->dev,
             "PWR_MGMT_1 before = 0x%02x\n",
             val);

    ret = regmap_write(data->regmap,
                       MPU_PWR_MGMT1,
                       0x01);

    if (ret)
    {
        dev_err(&client->dev,
                "PWR_MGMT_1 write failed\n");
        return ret;
    }

    ret = regmap_read(data->regmap,
                      MPU_PWR_MGMT1,
                      &val);

    if (ret)
    {
        dev_err(&client->dev,
                "PWR_MGMT_1 verify failed\n");
        return ret;
    }

    dev_info(&client->dev,
             "PWR_MGMT_1 after = 0x%02x\n",
             val);

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

static const struct dev_pm_ops mpu_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(mpu_suspend, mpu_resume)
};

static struct i2c_driver mpu9250_driver =
{
    .driver =
    {
        .name = "siraj_mpu9250",
        .of_match_table = mpu9250_of_match,
		.pm = &mpu_pm_ops,
    },

    .probe = mpu9250_probe,

    .remove = mpu9250_remove,
};

module_i2c_driver(mpu9250_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("Regmap Example");