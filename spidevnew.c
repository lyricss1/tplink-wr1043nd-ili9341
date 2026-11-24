//#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

// spidevnew.c - adds /dev/spidev0.1 on the SPI flash bus (ath79, 4.14 )
// CS = soft GPIO, flash CS0 isnt tuoched
// insmod spidevnew.ko [bus=0] [chip_select=1] [cs_gpio=15] [speed=10000000] or just insmod spidevnew.ko
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/spi/spi.h>

static int b;
static int cs=1;
static int gp=15;
static unsigned int hz=10000000;

module_param_named(bus,b,int,0444);
module_param_named(chip_select,cs,int,0444);
module_param_named(cs_gpio,gp,int,0444);
module_param_named(speed,hz,uint,0444);

static struct spi_master *m;
static struct spi_device *d;
static u16 on;

static int __init t_init(void){
	int r;
	m=spi_busnum_to_master(b);
	if(!m){
		pr_err("tft_spi: no spi bus %d\n",b);
		return -ENODEV;
	}
	on=m->num_chipselect;

	// ath79-spi has num_chipselect=1 so spi_add_device() rejects cs1
	// the real cs is done by gpio anyway  so just bump the number
	if(cs>=on)
		m->num_chipselect=cs+1;

	d=spi_alloc_device(m);
	if(!d){
		r=-ENOMEM;
		goto bad;
	}

	d->chip_select=cs;
	d->cs_gpio=gp; // must be set before spiadddevice
	d->max_speed_hz=hz;
	d->mode=SPI_MODE_0;
	d->bits_per_word=8;
	strlcpy(d->modalias,"spidev",sizeof(d->modalias));
	r=spi_add_device(d);
	if(r){
		pr_err("tft_spi: add device failed: %d\n",r);
		spi_dev_put(d);
		d=NULL;
		goto bad;
	}
	pr_info("tft_spi: spi%d.%d cs_gpio=%d %uHz\n",b,cs,gp,hz);
	return 0;

bad:
	m->num_chipselect=on;
	spi_master_put(m);
	m=NULL;
	return r;
}

static void __exit t_exit(void){
	if(d)
		spi_unregister_device(d);
	if(m){
		m->num_chipselect=on;
		spi_master_put(m);
	}
	pr_info("tft_spi: bye\n");
}


MODULE_PARM_DESC(bus, "SPI bus num (def 0)");
MODULE_PARM_DESC(chip_select, "Chip sel index (def 1)");
MODULE_PARM_DESC(cs_gpio, "GPIO num soft CS (def 15)");
MODULE_PARM_DESC(speed, "max SPI Hz (def 10000000)");
MODULE_AUTHOR("lyricss");
MODULE_VERSION("2.1");

module_init(t_init);
module_exit(t_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("spidev on ath79 spi bus 0 with gpio cs (ili9341)");
