
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


/*
 * lights.c
 * Auteur : BERNET Quentin, TRABELSI Yassine
 * Version :  1.0.0

 * Driver lights.c
 	Ici sont gérés les différents modes de la guirelande. La guirelande comprends 3 LED. Le 
 	changement est géré via une interruption.
 	Plus d'informations et de documentation sur 
	https://github.com/quentinkb/Guirelande-infrarouge


*/




/*
 *  Déclaration des variables concernant 
 *  le bouton, et les interruptions
*/

irqreturn_t button_action (int irq,void *data);
void tasklet_bouton(void); 
int irq_number ; 

/*
 * Déclaration des variables concernant 
 * la gestion des différents modes de la guirelande
 * et notamment la gestion du TIMER ( pour les clignotements entre autres)
 */

int ret = 0;
int mode_selector = 0 ;
int mod = 0 ;
int value = 0 ; 
int status; 
int current_mode = 0 ;
int leds_status[] = {0,1,1};
int received = 0 ; 
int WAIT_BEFORE_SIGNAL  = 200 ; 
static struct hrtimer timer;
unsigned long sec = 0;
unsigned long interval = 5e8;

/*
 * Déclaration des fonctions concernant 
 * la gestion des differents modes de la guirelande.
 */
void selectMode(int mode); 
void modeZero(void); 
void modeUn(void);
void modeDeux(void);
void reset(void); 



/*
 * Déclaration des variables et fonctions concernant 
 * la gestion de l'interface utilisateur
*/

static int device_write(struct file *f, const char __user *data,size_t size, loff_t *l); 
static int device_read(struct file *f, char __user *data, size_t size, loff_t *l);
static int device_open(struct inode *i,struct file *f); 
static int device_release(struct inode *i, struct file *f) ;
static long device_ioctl(struct file *,unsigned int, unsigned long); 



/*
 * Déclaration de la structure des GPIO
 * 1 : LED BLEU
 * 2 : BOUTON
 * 4 : LED VERTE
 * 5 : LED ROUGE
*/

static struct gpio mygpios[] = {
	{ 1, GPIOF_OUT_INIT_HIGH, "LED BLEUE" },
	{ 2, GPIOF_IN, "BUTTON" },
	{ 4, GPIOF_OUT_INIT_HIGH, "LED VERTE" },
	{ 5, GPIOF_OUT_INIT_HIGH, "LED ROUGE" }, 
};

/*
 * Definition de la structure operation 
 * pour la gestion des fonctions read,write,open,release
 * de l'interface utilisateur */


struct file_operations fops = {
.read = device_read,
.write = device_write,
.unlocked_ioctl = device_ioctl,
.open = device_open,
.release = device_release,
};

int major;
struct device *dev;
static struct class *ledRGB_class;
dev_t devt; 


/*
 * Fonction write
 * on reçoit le mode '0', '1' ou  '2'
 * et on change de mode en fonction
*/

static int device_write(struct file *f, const char __user*data, size_t size, loff_t *l){
  char mode_value; 
  copy_from_user(&mode_value,data,1);
  selectMode(mode_value);
  return size ;
}


static long device_ioctl(struct file * f,unsigned int num, unsigned long num2)
{
 printk(KERN_INFO"ioctl\n");
 return 0;
}


/*
 * fonction d'initialisation tst_init
 * Mise en place du tableau de GIPO ainsi que des IRQ
 */

static int __init tst_init(void)
{
  printk(KERN_INFO"GUIRLANDE INFRAROUGE");
  reset(); 
   int mode = 1 ;
  selectMode(mode); 
  gpio_request_array(mygpios,ARRAY_SIZE(mygpios)); 
  gpio_set_debounce(2, 200);
  irq_number = gpio_to_irq(2); 

  int a ;
  if(a = request_irq(irq_number,button_action,IRQF_TRIGGER_RISING,"Bouton",NULL)==0)
    {
      printk(KERN_INFO"OK");  

    }else
    {
      printk(KERN_INFO"Erreur %d\n",a);
    }
 
  major = register_chrdev(0,"ledRGB", &fops);
  if(major < 0 )
    {
      printk(KERN_INFO "Echec de register_chrdev\n");
      status=major;
      return status;
    }

  ledRGB_class = class_create(THIS_MODULE,"ledRGBAmoi");
  if( IS_ERR(ledRGB_class))
    {
      printk(KERN_INFO "echec class create\n");
      status = PTR_ERR(ledRGB_class);
      return status;
    }
  devt = MKDEV(major,0);
  dev = device_create(ledRGB_class,NULL,devt,NULL,"ledRGB"); 
  status = IS_ERR(dev) ? PTR_ERR(dev) :0 ; 

  if(status !=  0)
    {
      printk(KERN_ERR "Erreur device create\n");
      return status; 
    }
  return 0;
}


/*
 * Action sur l'interruption, 
 * on reçoit un signal infrarouge, et donc 
 * on passe au mode suivant
 */

void tasklet_bouton(void)
{

      printk(KERN_ERR "Infrarouge reçu +  %d\n", gpio_get_value(2));
      mode_selector = (mode_selector +1)%3; 
      selectMode(mode_selector);


}

/*
 * Déclaration du tasklet pour l'interruption
 * sur le bouton ( en réalité, signal infrarouge réçu )
 */

DECLARE_TASKLET(tasklet_bouton_id,tasklet_bouton,0); 

irqreturn_t button_action (int irq,void *data)
{
 
      tasklet_schedule(&tasklet_bouton_id); 
      return IRQ_HANDLED;  


}


/*
 * fonction selectMode
 * param int mode
 * en fonction de "mode", on appelle la fonction qui change de mode 
 * et on cancel le timer courrant. 
 */

void selectMode(int mode)
{
  ret = hrtimer_cancel(&timer);
  switch(mode){
  case 0 :
    modeZero();
    break;
  case 1:
    modeUn(); 
    break;
  case 2:
    modeDeux();
    break;
  default :
    modeZero();
    break; 

  }
}

/*
 * fonction reset
 * on éteint toute les LED et on met 
 * à jour le tableau leds_status à jour 
 */
void reset(void)
{
  gpio_set_value(1,1);
  gpio_set_value(4,1);
  gpio_set_value(5,1);
  int j ;
  for ( j = 0 ; j < 3 ; j++)
    {
      leds_status[j] = 1 ;
    }

}


/*
 * fonction timer_callback 
 * gère le timer, et donc le clignotement des LEDS pour les mods 1 et 2 
 * appelé en boucle tant qu'une nouvelle interruption n'est pa reçu 
 */ 

enum hrtimer_restart timer_callback( struct hrtimer *timer_for_restart){
  ktime_t currtime, i;
  currtime = ktime_get();
  i = ktime_set(sec, interval);
  hrtimer_forward(timer_for_restart, currtime, i);
  reset(); 

  if(current_mode == 1)
    {
      mod = (mod+1)%3 ;
      leds_status[mod] = 0 ; 
      gpio_set_value(1,leds_status[0]); 
      gpio_set_value(4,leds_status[1]);
      gpio_set_value(5,leds_status[2]);
  

    }
  if(current_mode == 2)
    {

      value = 1 - value ; 
      gpio_set_value(1,value); 
      gpio_set_value(4,value);
      gpio_set_value(5,value);

    }
  return HRTIMER_RESTART;
}

/*
 * description du mode ZERO : 
 * les trois LEDS sont allumés constantes
 */
void modeZero(void)
{
  reset(); 
  gpio_set_value(1,0);
  gpio_set_value(4,0);
  gpio_set_value(5,0);
 

}



/*
 * description du mode UN :
 * les trois LEDS clignontent les une après les autres
 */
void modeUn(void)
{
  printk(KERN_INFO"Mode un activé"); 
  reset();
  current_mode = 1 ;
  ktime_t ktime;
  ktime  = ktime_set(0, interval);
  hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  timer.function = &timer_callback;
  hrtimer_start(&timer, ktime, HRTIMER_MODE_REL);


}

/*
 * description du mode deux 
 * les trois LEDS clignotent simultanément
 */ 
void modeDeux(void)
{
  printk(KERN_INFO"Mode deux activé"); 
  reset();
  current_mode = 2 ;
  ktime_t ktime;
  ktime  = ktime_set(0, interval);
  hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  timer.function = &timer_callback;
  hrtimer_start(&timer, ktime, HRTIMER_MODE_REL);

}


/*
 *  Fonction tst_exit,
 * libère les tableaux et les espaces reservés
 */

static void __exit tst_exit(void)
{
  printk(KERN_INFO"Goodbye light controller!\n");
  gpio_free_array(mygpios,ARRAY_SIZE(mygpios)); 
  free_irq(irq_number,NULL);
  device_destroy(ledRGB_class,devt); 
  class_destroy(ledRGB_class); 
  unregister_chrdev(major,"ledRGB");
  ret = hrtimer_cancel(&timer);
}


module_init(tst_init);
module_exit(tst_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BERNET Quentin,TRABELSI Yassine");
MODULE_DESCRIPTION("Lights controller");



