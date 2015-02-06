#include "partition_define.h"
static const struct excel_info PartInfo_Private[PART_NUM]={
			{"preloader",262144,0x0, EMMC, 0,BOOT_1},
			{"dsp_bl",6029312,0x40000, EMMC, 0,BOOT_1},
			{"mbr",16384,0x600000, EMMC, 0,USER},
			{"ebr1",376832,0x604000, EMMC, 1,USER},
			{"pmt",4194304,0x660000, EMMC, 0,USER},
			{"nvram",3145728,0xa60000, EMMC, 0,USER},
			{"seccfg",131072,0xd60000, EMMC, 0,USER},
			{"uboot",393216,0xd80000, EMMC, 0,USER},
			{"bootimg",6291456,0xde0000, EMMC, 0,USER},
			{"recovery",6291456,0x13e0000, EMMC, 0,USER},
			{"sec_ro",6291456,0x19e0000, EMMC, 5,USER},
			{"misc",393216,0x1fe0000, EMMC, 0,USER},
			{"logo",3145728,0x2040000, EMMC, 0,USER},
			{"expdb",655360,0x2340000, EMMC, 0,USER},
			{"ebr2",16384,0x23e0000, EMMC, 0,USER},
			{"android",537919488,0x23e4000, EMMC, 6,USER},
			{"cache",537919488,0x224e4000, EMMC, 2,USER},
			{"usrdata",1073741824,0x425e4000, EMMC, 3,USER},
			{"fat",0,0x825e4000, EMMC, 4,USER},
			{"bmtpool",10485760,0xFFFF0050, EMMC, 0,USER},
 };
#ifdef  MTK_EMMC_SUPPORT
struct MBR_EBR_struct MBR_EBR_px[MBR_COUNT]={
		{"mbr",{1,2,3,4}},
		{"ebr1",{5,0,0,0}},
		{"ebr2",{6,7,0,0}},
};

EXPORT_SYMBOL(MBR_EBR_px);
#endif
