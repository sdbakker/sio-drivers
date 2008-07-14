/* -*-linux-c-*- */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <asm/arch/gpio.h>

#include "sios/resource.h"
#include "sios/sios-gpio.h"

#define BTN_RELEASED 0
#define BTN_PRESSED 1
#define BTN_LONG 0
#define BTN_SHORT 1

static int btn_short_timeout = 2000;
static int btn_long_timeout = 4000;
static spinlock_t btn_state_lock = SPIN_LOCK_UNLOCKED;

static void pwr_button_work(struct work_struct *work);
static DECLARE_WORK(button_pressed_counter, pwr_button_work);

module_param_named(timeout_short, btn_short_timeout, int, 0700);
module_param_named(timeout_long, btn_long_timeout, int, 0700);

static struct pwr_button_state {
	int volatile state;
	int pressed_ms;
	int short_fired;
	int long_fired;
	int flushed;
	unsigned long start;
	struct sios_device *dev;
} pwr_bs;
 
struct sios_resource button_res[] = {
	[0] = {
		.name = "PwrButton",
		.start = GPIO_SIOS_PBST,
		.end = GPIO_SIOS_PBST,
		.type = SIOS_IO_GPIO|SIOS_IRQ,
	},
};
#define PWRBTN_IRQ gpio_to_irq(GPIO_SIOS_PBST)

static void pwr_button_uevent(struct sios_device *sdev, int state)
{
	char *envp[3];
	char statestr[11];

	snprintf(statestr, 11, "STATE=%s", (state) ? "short" : "long");

	envp[0] = "BUTTON=power";
	envp[1] = statestr;
	envp[2] = NULL;

	kobject_uevent_env(&sdev->dev.kobj, KOBJ_CHANGE, envp);
}

static inline void pwr_button_work_reschedule(struct work_struct *work)
{
	unsigned long flags;

	spin_lock_irqsave(&btn_state_lock, flags);
	if (pwr_bs.flushed)
		goto out;
	schedule_work(work);	
out:
	spin_unlock_irqrestore(&btn_state_lock, flags);
}

static void pwr_button_work(struct work_struct *work)
{
	unsigned long flags;
	int state;

	spin_lock_irqsave(&btn_state_lock, flags);
	state = pwr_bs.state;
	spin_unlock_irqrestore(&btn_state_lock, flags);

	if (state == BTN_PRESSED) {
		if (pwr_bs.start)
			pwr_bs.pressed_ms = (jiffies - pwr_bs.start) * 1000 / HZ;
		else
			pwr_bs.start = jiffies;
	} else {
		pwr_bs.start = 0L;
		pwr_bs.pressed_ms = 0;
		pwr_bs.short_fired = 0;
		pwr_bs.long_fired = 0;
		return;
	}

	if (pwr_bs.pressed_ms < btn_short_timeout) {
		pwr_button_work_reschedule(work);
	} else if (!pwr_bs.short_fired) {
		pwr_bs.short_fired = 1;
		pwr_button_work_reschedule(work);	
		pwr_button_uevent(pwr_bs.dev, BTN_SHORT);
	} else if (pwr_bs.pressed_ms >= btn_long_timeout && !pwr_bs.long_fired) {
		pwr_bs.long_fired = 1;
		pwr_button_uevent(pwr_bs.dev, BTN_LONG);
	} else {
		pwr_button_work_reschedule(work);
	}
}

static irqreturn_t pbst_interrupt_handler(int irq, void *dev_id)
{
	unsigned long flags;

	spin_lock_irqsave(&btn_state_lock, flags);
	pwr_bs.state = (gpio_get_value(GPIO_SIOS_PBST)) ? BTN_RELEASED : BTN_PRESSED;
	schedule_work(&button_pressed_counter);
	spin_unlock_irqrestore(&btn_state_lock, flags);

  	return IRQ_HANDLED;
}

static void sios_button_release(struct sios_device *sdev)
{
	return;
}

struct sios_driver button_drv = {
	.version = "$Revision: 1.0 $",
	.module = THIS_MODULE,
	.driver = {
		.name = "sios:button",
	},
};

struct sios_device button_dev = {
	.name = "sios:button",
	.release = sios_button_release,
	.num_resource = 1,
	.resource = button_res,
};

int __init sios_button_init(void)
{
	int error;
 
      	error = sios_driver_register(&button_drv);
	if (error)
		return error;

	error = sios_device_register(&button_dev);
	if (error) {
		sios_driver_unregister(&button_drv);
		return error;
	}

	pwr_bs.dev = &button_dev;
	gpio_direction_input(GPIO_SIOS_PBST);

	set_irq_type(PWRBTN_IRQ, IRQT_BOTHEDGE);
	if (request_irq(PWRBTN_IRQ, pbst_interrupt_handler, SA_INTERRUPT,
			"sios-button-pbst", NULL) != 0) {
		printk(KERN_ERR "Unable to register IRQ %d\n",
		       GPIO_SIOS_PBST);
		goto err;
	}

       return 0;
err:
	sios_device_unregister(&button_dev);
	sios_driver_unregister(&button_drv);

	return error;
}

static void __exit sios_button_exit(void)
{
	free_irq(PWRBTN_IRQ, NULL);

	spin_lock(&btn_state_lock);
	pwr_bs.flushed = 1;
	spin_unlock(&btn_state_lock);

	flush_scheduled_work();
	sios_device_unregister(&button_dev);
	sios_driver_unregister(&button_drv);
}


MODULE_DESCRIPTION("SIOS button driver");
MODULE_AUTHOR("Simon de Bakker <simon@v2.nl>");
MODULE_LICENSE("GPL");

module_init(sios_button_init);
module_exit(sios_button_exit);
