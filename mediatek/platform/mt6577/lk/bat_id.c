//<2012/09/18-tedwu, port for lk.
//<2012/08/10-tedwu, Create this file for checking battery ID.

#include <target/board.h>

#include <platform/mt_typedefs.h>
#include <platform/mt_pmic.h>
#include <platform/boot_mode.h>
#include <platform/mt_gpt.h>
#include <platform/mt_sleep.h>
#include <platform/mt_rtc.h>
#include <platform/mt_disp_drv.h>
#include <platform/mtk_wdt.h>
#include <platform/mt_logo.h>

#undef printf

//#define CONFIG_DEBUG_MSG
#define GPT_TIMER // when sleep driver ready, open this define

#include <platform/mt_gpio.h>

#include <platform/bat_id.h>

//<2012/7/9-tedwu, check MOTO battery ID
//<2012/01/02-2106-jessicatseng, Add code to check MOTO battery ID for DS2502 and bq2024
#if defined(CHK_BATTERY_ID__MOTO_ID)
#define BATTERY_CHECK_MAX_NUM		5
static kal_bool Is_battery_valid = KAL_FALSE;
static int Battery_ID_read_count = 0;
static char rom_data[8];
static char memory_data[32*4];
static char Family_Code, Max_Voltage, page2_CheckSum, page3_CheckSum, page4_Str[32+1];
static int  Custom_ID;
//>2012/01/02-2106-jessicatseng
//>2012/7/9-tedwu


void bat_id_init_pin(void)
{
    mt_set_gpio_mode(GPIO_BATTERY_DATA_PIN, GPIO_BATTERY_DATA_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_BATTERY_DATA_PIN, GPIO_DIR_OUT);
	mt_set_gpio_pull_select(GPIO_BATTERY_DATA_PIN, GPIO_PULL_UP);
	mt_set_gpio_pull_enable(GPIO_BATTERY_DATA_PIN, GPIO_PULL_ENABLE);
	mt_set_gpio_out(GPIO_BATTERY_DATA_PIN, GPIO_OUT_ONE);
    gpt_busy_wait_us(550);
}

void bat_id_deinit_pin(void)
{
	mt_set_gpio_dir(GPIO_BATTERY_DATA_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_BATTERY_DATA_PIN, GPIO_PULL_DISABLE);
}

void bat_id_init_variable(void)
{
    Battery_ID_read_count = 0;
    Is_battery_valid = KAL_FALSE;
}


//<2012/7/9-tedwu, check MOTO battery ID
//<2012/01/02-2106-jessicatseng, Add code to check MOTO battery ID for DS2502 and bq2024
static int OneWire_Reset_Bus(void)
{
	int presence_pulse;

    mt_set_gpio_out(GPIO_BATTERY_DATA_PIN, GPIO_OUT_ZERO);
    gpt_busy_wait_us(550);
    mt_set_gpio_out(GPIO_BATTERY_DATA_PIN, GPIO_OUT_ONE);
	gpt_busy_wait_us(60);
    mt_set_gpio_dir(GPIO_BATTERY_DATA_PIN, GPIO_DIR_IN);
	gpt_busy_wait_us(40);
	presence_pulse = mt_get_gpio_in(GPIO_BATTERY_DATA_PIN);
    mt_set_gpio_dir(GPIO_BATTERY_DATA_PIN, GPIO_DIR_OUT);
	gpt_busy_wait_us(400);
    printk("Presence Pulse: %d\n", presence_pulse);

	return presence_pulse;
}

static void OneWire_Write_Byte(char cmd)
{
	int idx;
/*
#if defined(CHK_BATTERY_ID__WITH_INTERNAL_PULL_UP_R)
	mt_set_gpio_pull_enable(GPIO_BATTERY_DATA_PIN, GPIO_PULL_DISABLE);
#endif	
*/
	//mt_set_gpio_dir(GPIO_BATTERY_DATA_PIN, GPIO_DIR_OUT);
		
	for(idx=0; idx<8; idx++)
	{
		mt_set_gpio_out(GPIO_BATTERY_DATA_PIN, 0);
		gpt_busy_wait_us(3);
		
		if((cmd >> idx) & 0x01)
		{
			mt_set_gpio_out(GPIO_BATTERY_DATA_PIN, 1);
			gpt_busy_wait_us(60);
		}
		else
		{
			mt_set_gpio_out(GPIO_BATTERY_DATA_PIN, 0);
			gpt_busy_wait_us(60);
		}			
		
		mt_set_gpio_out(GPIO_BATTERY_DATA_PIN, 1);
		gpt_busy_wait_us(10);	// Trec
	}
}	

static char OneWire_Read_Byte(void)
{
	int idx = 0;
	char retbyte = 0;
	
	//mt_set_gpio_out(GPIO_BATTERY_DATA_PIN, 1);  //to be deleted?
/*
#if defined(CHK_BATTERY_ID__WITH_INTERNAL_PULL_UP_R)
	mt_set_gpio_pull_enable(GPIO_BATTERY_DATA_PIN, GPIO_PULL_DISABLE);
#endif	
*/
	//mt_set_gpio_dir(GPIO_BATTERY_DATA_PIN, GPIO_DIR_OUT);  //to be deleted?
	
	for(idx=0; idx<8; idx++)
	{
		mt_set_gpio_out(GPIO_BATTERY_DATA_PIN, 0);
		
		//This delay has to remove, because Tsu has to be less than 1us for DS2502
		//gpt_busy_wait_us(1 /*3*/);
/*		
#if defined(CHK_BATTERY_ID__WITH_INTERNAL_PULL_UP_R)
		mt_set_gpio_pull_select(GPIO_BATTERY_DATA_PIN, GPIO_PULL_UP);
		mt_set_gpio_pull_enable(GPIO_BATTERY_DATA_PIN, GPIO_PULL_ENABLE);
#endif
*/
		mt_set_gpio_dir(GPIO_BATTERY_DATA_PIN, GPIO_DIR_IN);
		//gpt_busy_wait_us(15);
		gpt_busy_wait_us(18);
		
		if(mt_get_gpio_in(GPIO_BATTERY_DATA_PIN) & 0x01)
		{
			retbyte |= (0x01 << idx);
		}	
		
		gpt_busy_wait_us(60);
		
		mt_set_gpio_out(GPIO_BATTERY_DATA_PIN, 1);
/*
#if defined(CHK_BATTERY_ID__WITH_INTERNAL_PULL_UP_R)
		mt_set_gpio_pull_enable(GPIO_BATTERY_DATA_PIN, GPIO_PULL_DISABLE);
#endif		
*/
		mt_set_gpio_dir(GPIO_BATTERY_DATA_PIN, GPIO_DIR_OUT);

		gpt_busy_wait_us(10);	// Trec
	}
	
	return retbyte;
}
	
void Read_Battery_Info(void)
{
	char read_rom = 0x33, skip_rom = 0xcc, read_memory = 0xF0;
	int idx = 0, page_idx;
		
  printk("[BATTERY_TOP] LOG. lk Read_Battery_Info----------------------------\n");

//<2012/12/26-tedwu, for power consumption and avoid ESD issue on 1-wire pin?.
#if defined(ARIMA_PROJECT_HAWK35)
	bat_id_init_pin();
#endif
//>2012/12/26-tedwu
	if(OneWire_Reset_Bus() == 0)
	{	
		// Read ROM
		OneWire_Write_Byte(read_rom);
		
		for(idx=0; idx<8; idx++)
		{	
			rom_data[idx] = OneWire_Read_Byte();
		}	
/*
        for(idx=0; idx<(sizeof(rom_data)); idx++)
            printk("0x%X, ", rom_data[idx]);

        printk("\n=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
*/
		// Read Memory		
		for(page_idx=0; page_idx<4; page_idx++)
		//for(page_idx=0; page_idx<6; page_idx++)
		//for(page_idx=3; page_idx<4; page_idx++)
		{		
			OneWire_Reset_Bus();
			OneWire_Write_Byte(skip_rom);
			OneWire_Write_Byte(read_memory);
			OneWire_Write_Byte(0x00 + (0x20 * page_idx));
			OneWire_Write_Byte(0x00);						
			
			OneWire_Read_Byte();	// CRC
			
			for(idx=0; idx<32; idx++)
			{
				memory_data[idx + (0x20 * page_idx)] = OneWire_Read_Byte();
			}	
		}

/*      for(idx=0; idx<(sizeof(memory_data)); ) {
            printk("%02X", memory_data[idx++]);
            if (0==idx%32)  printk("\n");
        }
*/
/*
        page_idx=3;
        for(idx=0; idx<32; idx++)
            printk("0x%X, ", memory_data[idx + (0x20 * page_idx)]);
*/
        printk("\n=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");

		// parsing data
        {
			Family_Code = rom_data[0];
			Custom_ID = (int)(((rom_data[6]<<8)|(rom_data[5]&0xF0))>>4);
			Max_Voltage = memory_data[2];
			page2_CheckSum = memory_data[0x20];
			page3_CheckSum = memory_data[0x40];
			
			memset(page4_Str, 0x00, sizeof(page4_Str));
			strncpy(page4_Str, &(memory_data[0x60]), 32);

			printk("Family_Code = %x, Custom_ID = %x, Max_Voltage = %x, page2_CheckSum = %x, page3_CheckSum = %x\n", \
							Family_Code, Custom_ID, Max_Voltage, page2_CheckSum, page3_CheckSum);
			printk("page4_Str = %s\n", page4_Str);					
		}
	}
	else
	{
		printk("[BATTERY_TOP] : presence pulse = high\n");
	}		
	
//<2012/12/26-tedwu, for power consumption and avoid ESD issue on 1-wire pin?.
#if defined(ARIMA_PROJECT_HAWK35)
	bat_id_deinit_pin();
#endif
//>2012/12/26-tedwu
	printk("[BATTERY_TOP] LOG. lk Read_Battery_Info End ---------------------------\n");
}

kal_bool Is_MOTO_Battery(void)
{
	kal_bool isValid = KAL_TRUE;
	//char Family_Code, page2_CheckSum, page3_CheckSum;
	char page4_Str_MOTO[] = "COPR2011MOTOROLA E.P CHARGE ONLY";
	//char page4_Str[32+1];
	//int  Custom_ID;

	printk("lk Is_MOTO_Battery Battery_ID_read_count = %d\n", Battery_ID_read_count);
	if(upmu_is_chr_det() == KAL_TRUE)
	{
		if((Battery_ID_read_count <= BATTERY_CHECK_MAX_NUM) && (Is_battery_valid == KAL_FALSE))
		{
//<2012/10/05-14728-jessicatseng, [Common] To fix the logical error of checking MOTO battery ID in lk mode
            #if defined(BATTERY_PACK_HW4X)
			if((Family_Code == 0x89) && (Max_Voltage == 0x9E) && 
				(((page2_CheckSum == 0xD3) && (page3_CheckSum == 0xDC)) /*|| 
				 ((page2_CheckSum == 0x66) && (page3_CheckSum == 0x83)) ||
				 ((page2_CheckSum == 0x75) && (page3_CheckSum == 0xde)) ||
				 ((page2_CheckSum == 0x75) && (page3_CheckSum == 0x90)) */)
				&& (Custom_ID == 0x500) && (strncmp(&(page4_Str_MOTO[8]), &(page4_Str[8]), 24) == 0))
//<2012/10/03-14605-jessicatseng, [Hawk 4.0] Check MOTO battery ID in power off mode
					#elif defined(BATTERY_PACK_EG30)
//<2012/10/05-14752-jessicatseng, [Hawk 4.0] Add LG and SDI battery ID in lk mode
			if((Family_Code == 0x89) && (Max_Voltage == 0x9E) && 
				(((page2_CheckSum == 0xD2) && (page3_CheckSum == 0x71)) ||
				 ((page2_CheckSum == 0xd4) && (page3_CheckSum == 0x5a)))
				&& (Custom_ID == 0x500) && (strncmp(&(page4_Str_MOTO[8]), &(page4_Str[8]), 24) == 0))
//>2012/10/05-14752-jessicatseng
//>2012/10/03-14605-jessicatseng
            #else
			if((strncmp(&(page4_Str_MOTO[0]), &(page4_Str[0]), 4) == 0)
					&& (strncmp(&(page4_Str_MOTO[8]), &(page4_Str[8]), 24) == 0))
            #endif					
//>2012/10/05-14728-jessicatseng
			{
				//isValid = KAL_TRUE;
				Is_battery_valid = KAL_TRUE;
			}		
			else
			{
				Read_Battery_Info();
				Battery_ID_read_count ++;
			}
		}
		else
		{
			isValid = Is_battery_valid;
		}

		printk("uboot Is_MOTO_Battery %d, %d page4_Str = %s\n",Is_battery_valid, isValid, page4_Str);
	}	
		
	return isValid;			
}

//<2012/11/22-17513-jessicatseng, [Hawk 4.0] Add ZCV table for LG cell
int Get_MOTO_Battery_Vender(void)
{
	if((page2_CheckSum == 0xd4) && (page3_CheckSum == 0x5a))
		return 0x01;		
	else
		return 0x00;	
}
//>2012/11/22-17513-jessicatseng
#endif
//>2012/01/02-2106-jessicatseng
//>2012/7/9-tedwu
//>2012/08/10-tedwu
//>2012/09/18-tedwu
