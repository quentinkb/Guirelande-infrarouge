#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/hardirq.h>
#include <linux/preempt.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/pwm.h>


/*
 * infra.c
 * Auteur : GRANDURY Benjamin, BALDINI Kevin
 * Version :  1.0.0

 * Driver infra.c
 * On gère ici l'interruption déclenchée par le bouton qui va provoquer l'envoi
 * d'un signal infrarouge à la guirlande.


 	Plus d'informations et de documentation sur
	https://github.com/quentinkb/Guirelande-infrarouge

*/

/*
 * Déclaration de la structure des GPIO
 * 1 : Bouton
*/
static struct gpio button_gpio[] = {
	{1, GPIOF_IN, "Button"},
};

/*
 *  Déclaration des variables concernant
 *  le bouton et les interruptions, ainsi
 *  que la PWM
*/
int irq_number;
struct pwm_device * pwm0;
irqreturn_t button_action (int irq,void *data);
void tasklet_bouton(void);

/*
 * Déclaration du tasklet pour l'interruption
 * sur le bouton
 */
DECLARE_TASKLET(tasklet_bouton_id,tasklet_bouton,0);

irqreturn_t button_action (int irq,void *data)
{

  tasklet_schedule(&tasklet_bouton_id);
  return IRQ_HANDLED;

}

/*
 * Action réalisée lors de l'interruption,
 * on active la PWM pendant un court délai
 * afin que la LED envoie un signal infrarouge
 */
void tasklet_bouton(void)
{
  pwm_enable(pwm0);
  mdelay(10);
  pwm_disable(pwm0);
  printk(KERN_INFO"interruptBBB\n");

}

/*
 * fonction d'initialisation tst_init
 * Mise en place du tableau de GIPO ainsi que des IRQ
 * et de la PWM
 */
static int __init tst_init(void)
{
  pwm0 = pwm_request(2,"test-pwm");

  pwm_config(pwm0,13000,26000);
  gpio_request_array(button_gpio, ARRAY_SIZE(button_gpio));
  irq_number = gpio_to_irq(1);
  int a;
  if(a = request_irq(irq_number,button_action,IRQF_TRIGGER_RISING,"Bouton",NULL)==0)
    {
      printk(KERN_INFO"OK");

    }else
    {
      printk(KERN_INFO"Erreur %d\n",a);
    }

  return 0;
}

/*
 *  Fonction tst_exit,
 * libère les tableaux et les espaces reservés
 */
static void __exit tst_exit(void)
{
  pwm_disable(pwm0);
  pwm_free(pwm0);
  free_irq(irq_number,NULL);
  gpio_free_array(button_gpio, ARRAY_SIZE(button_gpio));
  printk(KERN_INFO"Goodbye world!\n");
}

module_init(tst_init);
module_exit(tst_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BALDINI GRANDURY");
MODULE_DESCRIPTION("LIGHTS");
