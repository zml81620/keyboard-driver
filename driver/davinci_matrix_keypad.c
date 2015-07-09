#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/param.h>
#include <linux/input.h>
#include <linux/delay.h>
//#include <asm/arch/gpio.h>
#include <linux/gpio.h>

#define ROW_NR 3
#define COL_NR 3
#define DFT_COL_SCAN_UDELAY 5
#define MAX_COL_SCAN_UDELAY 20
#define DEV_DRV_NAME "davinci_matrix_keypad"

#if 1
static const int row_gpio[ROW_NR] = {14,15,16};
static const int col_gpio[COL_NR] = {10,11,12};
#endif

static const int key_matrix[ROW_NR][COL_NR] = {
	[0] = {KEY_LEFT, KEY_RIGHT,KEY_STOP},
	[1] = {KEY_UP, KEY_DOWN,KEY_HOME},
        [2] = {KEY_COPY,KEY_OPEN,KEY_PASTE},
};
static struct timer_list scan_timer;
static struct input_dev *kp_input_dev;
static unsigned char col_status[ROW_NR];

static unsigned long col_scan_udelay = DFT_COL_SCAN_UDELAY;
module_param(col_scan_udelay, ulong, S_IRUGO);
MODULE_PARM_DESC(col_scan_udelay, "udelay before scan column status, "
		"default is 5");


static void matrix_kp_scan(unsigned long data);
static void matrix_kp_setup_scan_timer(struct timer_list * timer,
		unsigned long data)
{
	timer->function = matrix_kp_scan;
	timer->expires = jiffies + HZ/20;
	timer->data = data;
}

static void matrix_kp_get_col_status(unsigned char * status, int cnt)
{
	int row;
	int col;

	memset(status, 0, sizeof(status[0]) * cnt);
	for (row = 0; row < ROW_NR && row < cnt; row++) 
        {
#if 1
		gpio_direction_output(row_gpio[row], 0);
#else
                ret = gpio_direction_output(row_gpio[row], 0);
                printk("\ngpio_direction_output(row_gpio[row], 0) = %d\n",ret);
#endif

		if (col_scan_udelay > 0) 
                {
			udelay(col_scan_udelay);
		}
		for (col = 0; col < COL_NR; col++) 
                {
			if (gpio_get_value(col_gpio[col]) == 0) 
                        {
				status[row] |= (1<<col);
			}
		}
#if 1
		gpio_direction_input(row_gpio[row]);
#else
                ret = gpio_direction_input(row_gpio[row]);
                printk("\ngpio_direction_input(row_gpio[row]) = %d\n",ret);                
#endif
	}
}

static void matrix_kp_scan(unsigned long data)
{
	struct input_dev * dev = (struct input_dev*)data;
	int row;
	int col;
	unsigned char new_status[ROW_NR] = {0};
	unsigned char changed;
	int pressed;



	matrix_kp_get_col_status(new_status, ROW_NR);
       
        // printk(KERN_ERR "start scan key\n");
	for (row = 0; row < ROW_NR; row++) 
        {
		changed = col_status[row] ^ new_status[row];
		if (!changed)
                { 
                    continue;
				}
		for (col = 0; col < COL_NR; col++) 
                {
			if (changed & (1<<col)) 
                        {
				pressed = !!(new_status[row] & (1<<col));
				printk(KERN_ERR "key %d is %s\n", key_matrix[row][col],  //KERN_DEBUG
						(pressed == 1) ? "pressed" : "released");
				input_report_key(dev, key_matrix[row][col], pressed);
                                printk(KERN_ERR "key %d is %s\n", key_matrix[row][col],  //KERN_DEBUG
						(pressed == 1) ? "pressed" : "released");
                             //   printk(KERN_ERR "report scan key\n");
			}
		}
	}
	input_sync(dev);

	memcpy(col_status, new_status, sizeof(col_status));

	matrix_kp_setup_scan_timer(&scan_timer, data);
	add_timer(&scan_timer);
}

static int matrix_kp_open(struct input_dev *dev)
{

    matrix_kp_get_col_status(col_status, ROW_NR);
	matrix_kp_setup_scan_timer(&scan_timer, (unsigned long)dev);
	add_timer(&scan_timer);

	return 0;
}

static void matrix_kp_close(struct input_dev *dev)
{
     del_timer_sync(&scan_timer);
}

static int matrix_kp_probe(struct platform_device *pdev)
{
	int row;
	int col;

	init_timer(&scan_timer);

	kp_input_dev = input_allocate_device();
	if (kp_input_dev == NULL) {
		return -ENOMEM;
	}

	set_bit(EV_KEY, kp_input_dev->evbit);
	for (row = 0; row < ROW_NR; row++) {
		for (col = 0; col < COL_NR; col++) {
			set_bit(key_matrix[row][col], kp_input_dev->keybit);
		}
	}
	kp_input_dev->name = DEV_DRV_NAME;
	kp_input_dev->phys = DEV_DRV_NAME;
	//kp_input_dev->cdev.dev = &pdev->dev;
        kp_input_dev->dev.parent = &pdev->dev;
	kp_input_dev->id.bustype = BUS_HOST;
	kp_input_dev->open = matrix_kp_open;
	kp_input_dev->close = matrix_kp_close;

	if (input_register_device(kp_input_dev) < 0) 
	{
	   printk("\n: register input device fail\n");
	   printk(KERN_ERR DEV_DRV_NAME": register input device fail\n");
	   return -EINVAL;
	}
	printk("\n: register input device ok\n");
	return 0;
}

static int matrix_kp_remove(struct platform_device *pdev)
{
	input_unregister_device(kp_input_dev);
	input_free_device(kp_input_dev);
	return 0;
}

static void matrix_kp_release_dev(struct device * dev)
{
}

static struct platform_device davinci_matrix_keypad_device = {
	.name = DEV_DRV_NAME,
	.id = -1,
	.dev = {
		.platform_data = NULL,
		.release = matrix_kp_release_dev,
	},
	.num_resources = 0,
	.resource = NULL,
};

static struct platform_driver davinci_matrix_keypad_driver = {
	.driver = {
		.name = DEV_DRV_NAME,
	},
	.probe = matrix_kp_probe,
	.remove = matrix_kp_remove,
};

static void matrix_kp_check_params(void)
{
	if (col_scan_udelay > MAX_COL_SCAN_UDELAY) {
		printk(KERN_INFO "invalid col_scan_udelay %lu, "
				"reset it to default delay %d\n",
				col_scan_udelay, DFT_COL_SCAN_UDELAY);
		col_scan_udelay = DFT_COL_SCAN_UDELAY;
	}
}

static int __init matrix_kp_init(void)
{
	int rval = 0;
	int result;

	printk(KERN_INFO "init davinci matrix keypad driver\n");

	matrix_kp_check_params();

	result = platform_device_register(&davinci_matrix_keypad_device);
	if (result == 0) {
		result = platform_driver_register(&davinci_matrix_keypad_driver);
		if (result != 0) {
			printk(KERN_ERR "register davinci matrix kp driver fail\n");
			platform_device_unregister(&davinci_matrix_keypad_device);
			rval = result;
		}
	} else {
		printk(KERN_ERR "register davinci matrix kp device fail\n");
		rval = result;
	}

	return rval;
}

static void __exit matrix_kp_exit(void)
{
	printk(KERN_INFO "exit davinci matrix keypad driver\n");

	platform_driver_unregister(&davinci_matrix_keypad_driver);
	platform_device_unregister(&davinci_matrix_keypad_device);
}

module_init(matrix_kp_init);
module_exit(matrix_kp_exit);

MODULE_AUTHOR("creesec <creesec@sina.com>");
MODULE_DESCRIPTION("davinci matrix keypad driver");
MODULE_LICENSE("GPL");

