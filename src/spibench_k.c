// spibench_k.c - kernel-module version of spibench (no /dev/mem)
// mode=1 - store speed
// mode=2 - fill screen 10 frames (3writes per bit)
// mode=3 - animation, 3 writes per bit (SET/CLEAR registers)
// mode=4 - animation, 2 writes per bit (one OUT register write sets data+clock)
// secs=N: animation length in seconds (default 10)
// Results go to dmesg. Module intentionally returns -EAGAIN so it never stays loaded ez. 
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/ktime.h>
#include <linux/math64.h>
#include <linux/sched.h>

#define GB 0x18040000UL
#define SC (1u << 19)
#define MO (1u << 18)
#define DC (1u << 12)
#define CS (1u << 15)

static int mode;
module_param(mode, int, 0);
MODULE_PARM_DESC(mode, "1 = store speed, 2 = fill, 3 = anim 3 writes/bit, 4 = anim 2 writes/bit");

static int secs = 10;
module_param(secs, int, 0);
MODULE_PARM_DESC(secs, "animation length in seconds");

static void __iomem *s, *c, *o;

#define SET(x) __raw_writel((x), s)
#define CLR(x) __raw_writel((x), c)

static inline void sb(u8 d)
{
	int i;

	for (i = 7; i >= 0; i--) {
		if ((d >> i) & 1)
			SET(MO);
		else
			CLR(MO);
		SET(SC);
		CLR(SC);
	}
}

static void cm(u8 d) { CLR(CS); CLR(DC); sb(d); SET(DC); SET(CS); }
static void dt(u8 d) { CLR(CS); sb(d); SET(CS); }

static void win(void)
{
	cm(0x2a); dt(0); dt(0); dt(0); dt(239);
	cm(0x2b); dt(0); dt(0); dt(1); dt(63);
	cm(0x2c);
}

static void fl(u8 h, u8 l)
{
	long i;

	win();
	CLR(CS);
	for (i = 0; i < 76800; i++) {
		sb(h);
		sb(l);
	}
	SET(CS);
}

static void anim_frame(int two_writes, int off)
{
	static const u16 pal[6] = { 0xf800, 0xffe0, 0x07e0, 0x07ff, 0x001f, 0xf81f };
	static u8 row[480];
	int x, y, b, i;
	u32 v0, v;
	u8 d;

	for (x = 0; x < 240; x++) {
		u16 col = pal[((x + off) / 20) % 6];

		row[2 * x] = col >> 8;
		row[2 * x + 1] = col & 255;
	}

	win();

	if (!two_writes) {
		CLR(CS);
		for (y = 0; y < 320; y++)
			for (b = 0; b < 480; b++)
				sb(row[b]);
		SET(CS);
	} else {
		v0 = (__raw_readl(o) & ~(MO | SC | CS | DC)) | DC;
		for (y = 0; y < 320; y++) {
			for (b = 0; b < 480; b++) {
				d = row[b];
				for (i = 7; i >= 0; i--) {
					v = v0 | (((d >> i) & 1) ? MO : 0);
					__raw_writel(v, o);       
					__raw_writel(v | SC, o);
				}
			}
		}
		__raw_writel(v0 | CS, o);
	}
}

static void print_rate(const char *name, u32 st10, u32 kk)
{
	u32 v = (u32)div64_u64(2000000ULL, (u64)kk * st10);
	pr_info("spibench: %s %u.%02u Mbit/s\n", name, v / 100, v % 100);
}

static int __init sbk_init(void)
{
	void __iomem *g;
	void __iomem *oe;
	u32 mask;
	u64 t0, t, t_end;
	u32 st10, fps100, mbps100, frames, off;
	long j;
	int i;
	static const u16 col[3] = { 0x07e0, 0xf800, 0x001f };

	if (mode < 1 || mode > 4) {
		pr_info("spibench: mode=1 speed, 2 fill, 3/4 animation (secs=N)\n");
		return -EINVAL;
	}

	g = ioremap(GB, 0x100);
	if (!g)
		return -ENOMEM;
	o = g + 0x08;
	s = g + 0x0c;
	c = g + 0x10;
	
	//GPIOOE:0 - output 
	//1 - input 
	oe = g + 0x00;

	pr_info("spibench: BEFORE OE=%08x OUT=%08x\n",
        	__raw_readl(oe), __raw_readl(o));

	pr_info("spibench: testing GPIO19 mask=%08x\n", SC);

	__raw_writel(SC, s);
	pr_info("spibench: after SET OUT=%08x IN=%08x\n",
	        __raw_readl(o), __raw_readl(g + 0x04));

	__raw_writel(SC, c);
	pr_info("spibench: after CLR OUT=%08x IN=%08x\n",
	        __raw_readl(o), __raw_readl(g + 0x04));

	mask = SC | MO | DC | CS;

	__raw_writel(__raw_readl(oe) & ~mask, oe);
	pr_info("spibench: mapped g=%p oe=%p out=%p set=%p clr=%p\n",
        g, oe, o, s, c);

	pr_info("spibench: OE before=%08x\n", __raw_readl(oe));


	pr_info("spibench: GPIO before init: OE=%08x OUT=%08x IN=%08x\n",
        	__raw_readl(oe), __raw_readl(o), __raw_readl(g + 0x04));
	/* Initial state: clock low, MOSI low, DC high, CS high */
	//CLR(SC | MO);
	//SET(DC | CS);

	CLR(SC | MO);
	pr_info("spibench: OUT after CLR=%08x\n", __raw_readl(o));

	SET(DC | CS);
	pr_info("spibench: OUT after SET=%08x\n", __raw_readl(o));

	pr_info("spibench: OE final=%08x\n", __raw_readl(oe));

	pr_info("spibench: GPIO after init:  OE=%08x OUT=%08x IN=%08x\n",
        	__raw_readl(oe), __raw_readl(o), __raw_readl(g + 0x04));
	if (mode == 1) {
		const long m = 5000000;

		t0 = ktime_get_ns();
		for (j = 0; j < m; j++) {
			SET(SC);
			CLR(SC);
		}
		t = ktime_get_ns() - t0;
		st10 = (u32)div64_u64(t * 10, 2ULL * m);
		pr_info("spibench: store %u.%u ns (total %llu ms)\n",
			st10 / 10, st10 % 10, div64_u64(t, 1000000));
		if (st10) {
			print_rate("2 stores/bit:  ", st10, 4);
			print_rate("2.5 stores/bit:", st10, 5);
			print_rate("3 stores/bit:  ", st10, 6);
		}
	} else if (mode == 2) {
		const int k = 10;

		t0 = ktime_get_ns();
		for (i = 0; i < k; i++)
			fl(col[i % 3] >> 8, col[i % 3] & 255);
		t = ktime_get_ns() - t0;
		if (t) {
			fps100 = (u32)div64_u64(1000000000000ULL, t);
			mbps100 = (u32)div64_u64(1228800000000ULL, t);
			pr_info("spibench: %d frames %llu ms, %u.%02u fps, %u.%02u Mbit/s\n",
				k, div64_u64(t, 1000000),
				fps100 / 100, fps100 % 100,
				mbps100 / 100, mbps100 % 100);
		}
	} else {
		frames = 0;
		off = 0;
		t0 = ktime_get_ns();
		t_end = t0 + (u64)secs * 1000000000ULL;
		do {
			anim_frame(mode == 4, off);
			off = (off + 12) % 120;
			frames++;
			cond_resched();
		} while (ktime_get_ns() < t_end);
		t = ktime_get_ns() - t0;
		fps100 = (u32)div64_u64((u64)frames * 100000000000ULL, t);
		pr_info("spibench: anim mode %d: %u frames in %llu ms = %u.%02u fps\n",
			mode, frames, div64_u64(t, 1000000),
			fps100 / 100, fps100 % 100);
	}

	iounmap(g);
	return 0;
}

static void __exit sbk_exit(void)
{
}

module_exit(sbk_exit);

module_init(sbk_init);
MODULE_LICENSE("GPL");
