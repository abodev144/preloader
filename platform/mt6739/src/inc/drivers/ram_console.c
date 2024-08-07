#include <string.h>
#include "ram_console.h"

#define RAM_CONSOLE_SIG (0x43474244) /* DBGC */
#define MOD "RAM_CONSOLE"

struct ram_console_buffer {
	u32 sig;
	/* for size comptible */
	u32 off_pl;
	u32 off_lpl; /* last preloader */
	u32 sz_pl;
	u32 off_lk;
	u32 off_llk; /* last lk */
	u32 sz_lk;
	u32 padding[4]; /* size = 4 * 16 = 64 byte */
	u32 off_linux;
	u32 off_console;
	u32 padding2[3];
};

struct reboot_reason_pl {
	u32 wdt_status;
	u32 last_func[RAM_CONSOLE_PL_SIZE]
};

struct reboot_reason_kernel {
	u32 fiq_step;
	u32 exp_type; /* 0xaeedeadX: X=1 (HWT), X=2 (KE), X=3 (nested panic) */
	u32 others[0];
};

static struct ram_console_buffer *ram_console = NULL;
static int ram_console_size;

#define ALIGN(x, size) ((x + size - 1) & ~(size - 1))

void ram_console_init(void)
{
    int i;
    struct reboot_reason_pl *rr_pl;
    ram_console = (struct ram_console_buffer *)RAM_CONSOLE_SRAM_ADDR;

    if (((struct ram_console_buffer *)RAM_CONSOLE_SRAM_ADDR)->sig == RAM_CONSOLE_SIG) {
	print("%s using SRAM\n", MOD);
	ram_console = (struct ram_console_buffer *)RAM_CONSOLE_SRAM_ADDR;
	ram_console_size = RAM_CONSOLE_SRAM_SIZE;
    } else if (((struct ram_console_buffer *)RAM_CONSOLE_DRAM_ADDR)->sig == RAM_CONSOLE_SIG) {
	print("%s using DRAM\n", MOD);
	ram_console = (struct ram_console_buffer *)RAM_CONSOLE_DRAM_ADDR;
	ram_console_size = RAM_CONSOLE_DRAM_SIZE;
    } else {
    	print("%s using default\n", MOD);
	ram_console = (struct ram_console_buffer *)RAM_CONSOLE_DEF_ADDR;
	ram_console_size = RAM_CONSOLE_DEF_SIZE;
    }
    print("%s start: 0x%x, size: 0x%x, sig: 0x%x\n", MOD, ram_console, ram_console_size, ram_console->sig);
    if (ram_console->sig == RAM_CONSOLE_SIG && ram_console->sz_pl == sizeof(struct reboot_reason_pl) && ram_console->off_pl + ALIGN(ram_console->sz_pl, 64) == ram_console->off_lpl) {
	print("%s preloader last status: ", MOD);
	rr_pl = (void*)ram_console + ram_console->off_pl;
	for (i = 0; i < RAM_CONSOLE_PL_SIZE; i++) {
	    print("0x%x ", rr_pl->last_func[i]);
	}
	print("\n");
	memcpy((void *)ram_console + ram_console->off_lpl, (void *)ram_console + ram_console->off_pl, ram_console->sz_pl);
    } else {
	memset(ram_console, 0, ram_console_size);
	ram_console->sig = RAM_CONSOLE_SIG;
	ram_console->off_pl = sizeof(struct ram_console_buffer);
	ram_console->sz_pl = sizeof(struct reboot_reason_pl);
	ram_console->off_lpl = ram_console->off_pl + ALIGN(ram_console->sz_pl, 64);
    }
}

void ram_console_reboot_reason_save(u32 rgu_status)
{
    struct reboot_reason_pl *rr_pl;
    if (ram_console) {
	rr_pl = (void*)ram_console + ram_console->off_pl;
	rr_pl->wdt_status = rgu_status;
	print("%s wdt status (0x%x)=0x%x\n", MOD,
	      rr_pl->wdt_status, rgu_status);
    }
}

void ram_console_pl_save(unsigned int val, int index)
{
    struct reboot_reason_pl *rr_pl;
    if (ram_console) {
	rr_pl = (void*)ram_console + ram_console->off_pl;
	if (index < RAM_CONSOLE_PL_SIZE)
	    rr_pl->last_func[index] = val;
    }
}

#define RE_BOOT_BY_WDT_SW 2
#define RE_BOOT_NORMAL_BOOT 0
#define RE_BOOT_FULL_PMIC 0x800
bool ram_console_is_abnormal_boot(void)
{
	unsigned int fiq_step, wdt_status, exp_type;

	if (!ram_console) {
		ram_console_init();
	}
	if (ram_console && ram_console->off_linux && ram_console->off_linux < ram_console_size && ram_console->off_pl < ram_console_size) {
		wdt_status = ((struct reboot_reason_pl*)((void*)ram_console + ram_console->off_pl))->wdt_status;
		fiq_step = ((struct reboot_reason_kernel*)((void*)ram_console + ram_console->off_linux))->fiq_step;
		exp_type = ((struct reboot_reason_kernel*)((void*)ram_console + ram_console->off_linux))->exp_type;
		print("%s. wdt_status 0x%x, fiq_step 0x%x, exp_type 0x%x\n", MOD, wdt_status, fiq_step, (exp_type ^ 0xaeedead0) < 16 ? exp_type ^ 0xaeedead0 : exp_type);
		if (fiq_step != 0 && (exp_type ^ 0xaeedead0) >= 16) {
			fiq_step = 0;
			((struct reboot_reason_kernel*)((void*)ram_console + ram_console->off_linux))->fiq_step = fiq_step;
		}
		if ((fiq_step == 0 && (wdt_status == RE_BOOT_BY_WDT_SW || wdt_status == RE_BOOT_FULL_PMIC)) || (wdt_status == RE_BOOT_NORMAL_BOOT))
			return false;
		else
			return true;
	}
	return false;
}
