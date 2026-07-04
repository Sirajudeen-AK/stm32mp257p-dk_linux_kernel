#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

static struct kobject *my_kobj;

static int value;
static char name[32] = "Linux";

/*
	Function to show the value attribute of the kobject. 
	This function is called when a user reads the "value" attribute in sysfs.

	Arguments:
		1. kobj - pointer to the kobject associated with the attribute.
		2. attr - pointer to the kobj_attribute structure associated with the attribute.
		3. buf - pointer to the buffer where the data will be written.

	The function writes the current value of the value variable to the buffer and returns the number of bytes written.
*/
static ssize_t value_show(struct kobject *kobj,
                          struct kobj_attribute *attr,
                          char *buf)
{
    return sprintf(buf,"%d\n",value);
}

/*
	Function to store the value attribute of the kobject. 
	This function is called when a user writes to the "value" attribute in sysfs.

	Arguments:
		1. kobj - pointer to the kobject associated with the attribute.
		2. attr - pointer to the kobj_attribute structure associated with the attribute.
		3. buf - pointer to the buffer containing the data written by the user.
		4. count - number of bytes written by the user.

	The function reads the data from the buffer, updates the value variable, and prints the new value to the kernel log.
*/
static ssize_t value_store(struct kobject *kobj,
                           struct kobj_attribute *attr,
                           const char *buf,
                           size_t count)
{
    sscanf(buf,"%d",&value);

    printk("value=%d\n",value);

    return count;
}

/*
	Function to show the name attribute of the kobject. 
	This function is called when a user reads the "name" attribute in sysfs.

	Arguments:
		1. kobj - pointer to the kobject associated with the attribute.
		2. attr - pointer to the kobj_attribute structure associated with the attribute.
		3. buf - pointer to the buffer where the data will be written.

	The function writes the current value of the name variable to the buffer and returns the number of bytes written.
*/
static ssize_t name_show(struct kobject *kobj,
                         struct kobj_attribute *attr,
                         char *buf)
{
    return sprintf(buf,"%s\n",name);
}

/*
	Function to store the name attribute of the kobject. 
	This function is called when a user writes to the "name" attribute in sysfs.

	Arguments:
		1. kobj - pointer to the kobject associated with the attribute.
		2. attr - pointer to the kobj_attribute structure associated with the attribute.
		3. buf - pointer to the buffer containing the data written by the user.
		4. count - number of bytes written by the user.

	The function reads the data from the buffer, updates the name variable, and prints the new name to the kernel log.
*/
static ssize_t name_store(struct kobject *kobj,
                          struct kobj_attribute *attr,
                          const char *buf,
                          size_t count)
{
    sscanf(buf,"%31s",name);

    printk("name=%s\n",name);

    return count;
}

/*
	Structure to hold the attributes associated with a kobject. 
	This structure is used to create and manage the attributes in sysfs.

	__ATTR - Macro used to define a kobj_attribute structure. It takes the following arguments:
		1. name of the attribute.
		2. permission bits for the attribute.
		3. pointer to the show function for the attribute.
		4. pointer to the store function for the attribute.
*/
static struct kobj_attribute value_attr =
    __ATTR(value,0664,value_show,value_store);

static struct kobj_attribute name_attr =
    __ATTR(name,0664,name_show,name_store);

/*
	An array of pointers to the attributes associated with the kobject. 
	This array is used to create and manage the attributes in sysfs.
*/
static struct attribute *attrs[] =
{
    &value_attr.attr,
    &name_attr.attr,
    NULL
};

/*
	structure to hold the group of attributes associated with a kobject. 
	This structure is used to create and manage the attributes in sysfs.
*/
static struct attribute_group attr_group =
{
    .attrs = attrs,
};

static int __init my_init(void)
{
    int ret;

	/*
	API - used to create a kobject and add it to the sysfs hierarchy.
	Arguments:
		1. name of the kobject to be created.
		2. parent kobject to which the new kobject is to be added.
	
	*/
    my_kobj = kobject_create_and_add("my_sysfs",
                                     kernel_kobj);

    if(!my_kobj)
        return -ENOMEM;

	/*
	API - used to create a group of attributes associated with a kobject.
	Arguments:
		1. kobject to which the group of attributes is to be added.
		2. pointer to the attribute group structure.
	*/
    ret = sysfs_create_group(my_kobj,
                             &attr_group);

    if(ret)
    {
        kobject_put(my_kobj);
        return ret;
    }

    printk("sysfs created\n");

    return 0;
}

static void __exit my_exit(void)
{
	/*
	API - used to remove a group of attributes associated with a kobject.
	Arguments:
		1. kobject from which the group of attributes is to be removed.
		2. pointer to the attribute group structure.
	*/
    sysfs_remove_group(my_kobj,
                       &attr_group);

	/*
	API - used to decrement the reference count of a kobject. When the reference count reaches zero, the kobject is freed.
	Arguments:
		1. kobject to be released.
	*/
    kobject_put(my_kobj);

    printk("removed\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siraj");
MODULE_DESCRIPTION("A simple sysfs example");