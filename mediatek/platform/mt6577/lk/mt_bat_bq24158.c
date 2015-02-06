#include <target/board.h>

//#define CFG_POWER_CHARGING

#ifdef CFG_POWER_CHARGING
#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_pmic.h>
#include <platform/boot_mode.h>
#include <platform/mt_gpt.h>
#include <platform/mt_sleep.h>
#include <platform/mt_rtc.h>
#include <platform/mt_disp_drv.h>
#include <platform/mtk_wdt.h>
#include <platform/mtk_key.h>
#include <platform/mt_logo.h>
#include <platform/mt_leds.h>
#include <platform/mt_gpio.h>

#include <platform/bq24158.h>
#include <printf.h>
#include <sys/types.h>
#include <target/cust_battery.h>
//<2012/12/06-tedwu, change for new CXD charging animation.
#include <target/cust_display.h>
//>2012/12/06-tedwu

#undef printf

//#define CONFIG_DEBUG_MSG
#define GPT_TIMER

//<2012/7/9-tedwu, check MOTO battery ID
#include <platform/bat_id.h>
//>2012/7/9-tedwu

//<2012/11/12-16768-kevincheng,factory cable detection
#if defined(MOTO_CABLE_DETECTION)
int g_factory_cable_id = 0;
#endif
//>2012/11/12-16768-kevincheng

/*************************************************************************
 * Debugs
 ************************************************************************/
//#define MT_BAT_BQ24158DB
#ifdef MT_BAT_BQ24158DB
#define MT_BAT_BQ24158DB printf
#else
#define MT_BAT_BQ24158DB(x,...)
#endif

/*****************************************************************************
 *  Type define
 ****************************************************************************/
typedef unsigned int       WORD;

typedef enum
{
    USB_SUSPEND = 0,
    USB_UNCONFIGURED,
    USB_CONFIGURED
} usb_state_enum;

///////////////////////////////////////////////////////////////////////////////////////////
//// Extern Functions
///////////////////////////////////////////////////////////////////////////////////////////
extern kal_bool upmu_is_chr_det(void);
extern int mt6329_detect_powerkey(void);
extern int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount);

//extern kal_bool meta_mode_check(void);

extern CHARGER_TYPE mt_charger_type_detection(void);
extern CHARGER_TYPE hw_charger_type_detection(void);

/*****************************************************************************
 *  BATTERY VOLTAGE
 ****************************************************************************/
#define BATTERY_LOWVOL_THRESOLD             3450
#define CHR_OUT_CURRENT                     100

/*****************************************************************************
 *  BATTERY TIMER
 ****************************************************************************/
//#define MAX_CHARGING_TIME                   12*60*60    // 12hr
#define MAX_CHARGING_TIME                   24*60*60    // 24hr
#define MAX_POSTFULL_SAFETY_TIME            1*30*60     // 30mins
#define MAX_PreCC_CHARGING_TIME             1*30*60     // 0.5hr
#define MAX_CV_CHARGING_TIME                3*60*60     // 3hr
#define BAT_TASK_PERIOD                     1           // 1sec
//<2012/06/20-tedwu, add MOTO logo on powerOff charging
#define LOOP_TIME                           200  // in ms
#define LOOP_TIME_SLEEP						LOOP_TIME*3		//EricHsieh,2012/10/17, power off charging power consumption tuning.
#if defined(CXD_ANIMATION_NEW)
    #define LOOP_COUNT                      5
    #define LOGO_DISPLAY_LOOPS              ( 2000/*8000*//LOOP_TIME/LOOP_COUNT)  // 8000 ms  	//MaoyiChou,2013/1/29, Reduce initial charged icon display time on Charge Only Mode to 2 seconds .
#else
    #define LOOP_COUNT                      2
    #define LOGO_DISPLAY_LOOPS              (10000/LOOP_TIME/LOOP_COUNT)  // 10000 ms
#endif
#define EXTENSION_LOOPS                     (10000/LOOP_TIME/LOOP_COUNT)  // 10000 ms
#define TOTAL_EXT_LOOPS                     (LOGO_DISPLAY_LOOPS + EXTENSION_LOOPS)
#if defined(ARIMA_CHARGE_ANIMATION_FOR_MOTO)
#define BL_SWITCH_TIMEOUT                   (16000/LOOP_TIME/LOOP_COUNT)  // 16000 ms
#else
//>2012/06/20-tedwu
#define BL_SWITCH_TIMEOUT                   1*6         // 6s
#endif

#define POWER_ON_TIME                       4*1         // 0.5s

/*****************************************************************************
 *  BATTERY Protection
 ****************************************************************************/
#define charger_OVER_VOL                    1
#define ADC_SAMPLE_TIMES                    5

/*****************************************************************************
 *  Pulse Charging State
 ****************************************************************************/
#define  CHR_PRE                            0x1000
#define  CHR_CC                             0x1001
#define  CHR_TOP_OFF                        0x1002
#define  CHR_POST_FULL                      0x1003
#define  CHR_BATFULL                        0x1004
#define  CHR_ERROR                          0x1005

//<2012/10/09-14860-jessicatseng, [Hawk 4.0] Integrate SOP-0296 partial charging
#if defined(MOTO_SOP_0296_PARTIAL_CHARGE)
#define CHR_PARTIAL													0x1006
#define PARTIAL_CHARGE_MIN_TEMPERATURE  		45	
#define PARTIAL_CHARGE_MAX_TEMPERATURE 			60
#define PARTIAL_MAX_VOLTAGE									4000
#define PARTIAL_RECHARGING_VOLTAGE      		3900
#endif
//>2012/10/09-14860-jessicatseng

///////////////////////////////////////////////////////////////////////////////////////////
//// Smart Battery Structure
///////////////////////////////////////////////////////////////////////////////////////////
//#define UINT32 unsigned long
//#define UINT16 unsigned short
//#define UINT8 unsigned char

typedef struct
{
    kal_bool   	bat_exist;
    kal_bool   	bat_full;
    kal_bool   	bat_low;
    UINT32      bat_charging_state;
    INT32      bat_vol;            
    kal_bool    charger_exist;
    UINT32      pre_charging_current;
    UINT32      charging_current;
    INT32      charger_vol;        
    UINT32      charger_protect_status;
    UINT32      ISENSE;
    INT32      ICharging;
    INT32      temperature;
    UINT32      total_charging_time;
    UINT32      PRE_charging_time;
    UINT32      CC_charging_time;
    UINT32      TOPOFF_charging_time;
    UINT32      POSTFULL_charging_time;
    UINT32      charger_type;
    UINT32      PWR_SRC;
    INT32      SOC;
    INT32      ADC_BAT_SENSE;
    INT32      ADC_I_SENSE;

//<2012/10/09-14860-jessicatseng, [Hawk 4.0] Integrate SOP-0296 partial charging
#if defined(MOTO_SOP_0296_PARTIAL_CHARGE)
		kal_bool	bat_partial_full;
#endif
//>2012/10/09-14860-jessicatseng
    
} PMU_ChargerStruct;

typedef enum
{
    PMU_STATUS_OK = 0,
    PMU_STATUS_FAIL = 1,
} PMU_STATUS;

/////////////////////////////////////////////////////////////////////
//// Global Variable
/////////////////////////////////////////////////////////////////////
static CHARGER_TYPE CHR_Type_num = CHARGER_UNKNOWN;
PMU_ChargerStruct BMT_status;

static unsigned short batteryVoltageBuffer[BATTERY_AVERAGE_SIZE];
static unsigned short batteryCurrentBuffer[BATTERY_AVERAGE_SIZE];
static unsigned short batterySOCBuffer[BATTERY_AVERAGE_SIZE];
static int batteryIndex = 0;
static int batteryVoltageSum = 0;
static int batteryCurrentSum = 0;
static int batterySOCSum = 0;
kal_bool g_bat_full_user_view = KAL_FALSE;
kal_bool g_Battery_Fail = KAL_FALSE;
kal_bool batteryBufferFirst = KAL_FALSE;

int V_PRE2CC_THRES = 3400;
int V_CC2TOPOFF_THRES = 4050;
int V_compensate_EVB = 80;

int g_HW_Charging_Done = 0;
int g_Charging_Over_Time = 0;

int g_SW_CHR_OUT_EN = 0;
int g_bl_on = 1;

int g_thread_count = 10;

// HW CV algorithm
unsigned int CHR_CON_0 = 0x7002FA00;
unsigned int CHR_CON_1 = 0x7002FA04;
unsigned int CHR_CON_2 = 0x7002FA08;
unsigned int CHR_CON_4 = 0x7002FA10;
unsigned int CHR_CON_6 = 0x7002FA18;
unsigned int CHR_CON_9 = 0x7002FA24;
unsigned int CHR_CON_10 = 0x7002FA28;
unsigned int PMIC_RESERVE_CON1 = 0x7002FE84;
volatile unsigned int save_value = 0x0;
volatile unsigned int CSDAC_DAT_MAX = 255;
volatile unsigned int CSDAC_DAT = 0;
volatile unsigned int VBAT_CV_DET = 0x0;
volatile unsigned int CS_DET = 0x0;
//int g_sw_cv_enable = 0;

int CHARGING_FULL_CURRENT = 130;	// mA on phone

int g_bat_temperature_pre=0;

int gADC_BAT_SENSE_temp=0;
int gADC_I_SENSE_temp=0;
int gADC_I_SENSE_offset=0;

kal_uint32 g_eco_version = 0;
#define PMIC6329_E1_CID_CODE    0x0029
int g_E1_vbat_sense = 0;

int g_R_BAT_SENSE = R_BAT_SENSE;
int g_R_I_SENSE = R_I_SENSE;
int g_R_CHARGER_1 = R_CHARGER_1;
int g_R_CHARGER_2 = R_CHARGER_2;

//<2012/11/22-17513-jessicatseng, [Hawk 4.0] Add ZCV table for LG cell
#if defined(CHK_BATTERY_ID__MOTO_ID) && defined(BATTERY_PACK_EG30)
static int g_MOTO_Battery_ID = 0x0;
#endif
//>2012/11/22-17513-jessicatseng

/*****************************************************************************
 * EM
****************************************************************************/
int g_BatteryAverageCurrent = 0;

/*****************************************************************************
 * USB-IF
****************************************************************************/
int g_usb_state = USB_UNCONFIGURED;
int g_temp_CC_value = Cust_CC_0MA;

/*****************************************************************************
 * Logging System
****************************************************************************/
int g_chr_event = 0;
int bat_volt_cp_flag = 0;
int Enable_BATDRV_LOG = 0;
//int Enable_BATDRV_LOG = 1;

/***************************************************
 * UBOOT
****************************************************/
//<2012/12/06-tedwu, change for new CXD charging animation.
#if defined(CXD_ANIMATION_NEW)
extern UINT32 capacity_last;
#endif
//>2012/12/06-tedwu, change for new CXD charging animation.
int prog = 25;
int prog_temp = 0;
int prog_first = 1;
int g_HW_stop_charging = 0;
int bl_switch_timer = -TOTAL_EXT_LOOPS;  //<2012/06/20-tedwu, add MOTO logo on powerOff charging.//>
int bat_volt_check_point = 0;
int getVoltFlag = 0;
int low_bat_boot_display=0;
int charger_ov_boot_display = 0;

kal_bool bl_switch = KAL_FALSE;
kal_bool user_view_flag = KAL_FALSE;

int vbat_compensate_cp = 4185; //4185mV
int vbat_compensate_value = 80; //80mV

//<<EricHsieh,2012/10/17, power off charging power consumption tuning.
#define GPIO_MAX 232

#if defined(ARIMA_PROJECT_HAWK35)
//<<MaoyiChou,2012/10/19, Modiy GPIO table and Power.
#if 0
UINT32 GPIO_NC_Table[] ={
    GPIO8  , GPIO11 , GPIO16 , GPIO17 , GPIO19 , GPIO20 , GPIO21 , GPIO22 , 
	GPIO23 , GPIO24 , GPIO25 , GPIO26 , GPIO27 , GPIO28 , GPIO29 , GPIO30 ,
	GPIO31 , GPIO32 , GPIO33 , GPIO34 , GPIO36 , GPIO37 , GPIO38 , GPIO39 ,
    GPIO40 , GPIO41 , GPIO42 , GPIO43 , GPIO44 , GPIO45 , GPIO46 , GPIO47 ,
    GPIO48 , GPIO49 , GPIO51 , GPIO52 , GPIO53 , GPIO54 , GPIO55 , GPIO56 , 
    GPIO57 , GPIO58 , GPIO59 , GPIO60 , GPIO61 , GPIO62 , GPIO63 , GPIO66 ,
    GPIO68 , GPIO70 , GPIO74 , GPIO77 , GPIO80 , GPIO82 , GPIO83 , GPIO84 , 
    GPIO85 , GPIO86 , GPIO95 , GPIO96 , GPIO99 , GPIO100, GPIO101, GPIO102, 
    GPIO104, GPIO105, GPIO106, GPIO107, GPIO109, GPIO111, GPIO112, GPIO114,
    GPIO118, GPIO119, GPIO120, GPIO121, GPIO124, GPIO125, GPIO130, GPIO136,
    GPIO137, GPIO141, GPIO149, GPIO150, GPIO161, GPIO162, GPIO174, GPIO182,
    GPIO183, GPIO184, GPIO188, GPIO189, GPIO190, GPIO191, GPIO192, GPIO193,
    GPIO194, GPIO195, GPIO197, GPIO198, GPIO199, GPIO200, GPIO202, GPIO203,
    GPIO204, GPIO205, GPIO206, GPIO207, GPIO208, GPIO209, GPIO210, GPIO211,
    GPIO213, GPIO214, GPIO215, GPIO216, GPIO217, GPIO218, GPIO219, GPIO220,
    GPIO223, GPIO224, GPIO225, GPIO226, GPIO227, GPIO229, GPIO230, GPIO231,
    GPIO_MAX
    };
#else
UINT32 GPIO_NC_Table[] ={
    GPIO8  , GPIO11 , GPIO16 , GPIO17 , GPIO19 , GPIO20 , GPIO21 , GPIO22 , 
	GPIO23 , GPIO24 , GPIO25 , GPIO26 , GPIO27 , GPIO28 , GPIO29 , GPIO30 ,
	GPIO31 , GPIO32 , GPIO33 , GPIO34 , GPIO36 , GPIO37 , GPIO38 , GPIO39 ,
    GPIO40 , GPIO41 , GPIO42 , GPIO43 , GPIO44 , GPIO45 , GPIO46 , GPIO48 ,
    GPIO49 , GPIO51 , GPIO52 , GPIO53 , GPIO54 , GPIO55 , GPIO66 , GPIO68 ,
    GPIO70 , GPIO74 , GPIO77 , GPIO80 , GPIO82 , GPIO83 , GPIO84 , GPIO85 ,
    GPIO86 , GPIO95 , GPIO96 , GPIO99 , GPIO100, GPIO101, GPIO102, GPIO104,
    GPIO105, GPIO106, GPIO107, GPIO109, GPIO111, GPIO112, GPIO114, GPIO118,
    GPIO120, GPIO121, GPIO124, GPIO125, GPIO130, GPIO135, GPIO136, GPIO137,
    GPIO141, GPIO149, GPIO150, GPIO152, GPIO153, GPIO154, GPIO155, GPIO161,
    GPIO162, GPIO174, GPIO182, GPIO183, GPIO184, GPIO188, GPIO189, GPIO190,
    GPIO191, GPIO192, GPIO193, GPIO194, GPIO195, GPIO197, GPIO198, GPIO199,
    GPIO200, GPIO202, GPIO203, GPIO204, GPIO205, GPIO206, GPIO207, GPIO208,
    GPIO209, GPIO210, GPIO211, GPIO213, GPIO214, GPIO215, GPIO216, GPIO217,
    GPIO218, GPIO219, GPIO220, GPIO223, GPIO225, GPIO226, GPIO227, GPIO229,
    GPIO230, GPIO231, GPIO_MAX
    };
#endif
//>>MaoyiChou,2012/10/19, Modiy GPIO table and Power.
#else if defined(ARIMA_PROJECT_HAWK40)
UINT32 GPIO_NC_Table[] ={
};
#endif
//>>EricHsieh,2012/10/17, power off charging power consumption tuning.

/**********************************************
 * Battery Temprature Parameters and functions
 ***********************************************/
typedef struct{
    INT32 BatteryTemp;
    INT32 TemperatureR;
} BATT_TEMPERATURE;

/* convert register to temperature  */
INT16 BattThermistorConverTemp(INT32 Res)
{
    int i = 0;
    INT32 RES1 = 0, RES2 = 0;
    INT32 TBatt_Value = -200, TMP1 = 0, TMP2 = 0;

#if defined(BAT_NTC_10_TDK_1)
BATT_TEMPERATURE Batt_Temperature_Table[] = {
 {-20,95327},
 {-15,71746},
 {-10,54564},
 { -5,41813},
 {  0,32330},
 {  5,25194},
 { 10,19785},
 { 15,15651},
 { 20,12468},
 { 25,10000},
 { 30,8072},
 { 35,6556},
 { 40,5356},
 { 45,4401},
 { 50,3635},
 { 55,3019},
 { 60,2521}
};
#endif

#if defined(BAT_NTC_TSM_1)
    BATT_TEMPERATURE Batt_Temperature_Table[] = {
    {-20,70603},    
    {-15,55183},
    {-10,43499},
    { -5,34569},
    {  0,27680},
    {  5,22316},
    { 10,18104},
    { 15,14773},
    { 20,12122},
    { 25,10000},
    { 30,8294},
    { 35,6915},
    { 40,5795},
    { 45,4882},
    { 50,4133},
    { 55,3516},
    { 60,3004}
};
#endif

#if defined(BAT_NTC_10_SEN_1)
BATT_TEMPERATURE Batt_Temperature_Table[] = {
 {-20,74354},
 {-15,57626},
 {-10,45068},
 { -5,35548},
 {  0,28267},
 {  5,22650},
 { 10,18280},
 { 15,14855},
 { 20,12151},
 { 25,10000},
 { 30,8279},
 { 35,6892},
 { 40,5768},
 { 45,4852},
 { 50,4101},
 { 55,3483},
 { 60,2970}
};
#endif

#if (BAT_NTC_10 == 1)
    BATT_TEMPERATURE Batt_Temperature_Table[] = {
//<2012/11/08-tedwu, Integrate the table of thermistor and temperature of BATTERY_PACK_HW4X.
//<2012/10/02-14542-jessicatseng, [Hawk 4.0] Integrate the table of thermistor and temperature for JB
#if defined(ARIMA_PROJECT_HAWK40)
        {-20,95326},
        {-15,71745},
        {-10,54563},
        { -5,41813},
        {  0,32330},
        {  5,25193},
        { 10,19785},
        { 15,15650},
        { 20,12468},
        { 25,10000},
        { 30,8072},
        { 35,6555},
        { 40,5356},
        { 45,4400},
        { 50,3635},
        { 55,3018},
        { 60,2520},
        { 65,2114},
		 		{ 70,1781},
		 		{ 75,1509},
		 		{ 80,1283}
#elif defined(BATTERY_PACK_HW4X)
{-25 ,127800},
{-20 , 95327},
{-18 , 84980},
{-15 , 71750},
{  0 , 32330},
{  5 , 25190},
{ 25 , 10000},
{ 45 ,  4401},
{ 55 ,  3019},
{ 58 ,  2708},
{ 60 ,  2521},
{ 70 ,  1781},
{ 85 ,  1097}
 #else
        {-20,68237},
        {-15,53650},
        {-10,42506},
        { -5,33892},
        {  0,27219},
        {  5,22021},
        { 10,17926},
        { 15,14674},
        { 20,12081},
        { 25,10000},
        { 30,8315},
        { 35,6948},
        { 40,5834},
        { 45,4917},
        { 50,4161},
        { 55,3535},
        { 60,3014}
#endif        
//>2012/10/02-14542-jessicatseng
//>2012/11/08-tedwu
    };
#endif

#if (BAT_NTC_47 == 1)
    BATT_TEMPERATURE Batt_Temperature_Table[] = {
        {-20,483954},
        {-15,360850},
        {-10,271697},
        { -5,206463},
        {  0,158214},
        {  5,122259},
        { 10,95227},
        { 15,74730},
        { 20,59065},
        { 25,47000},
        { 30,37643},
        { 35,30334},
        { 40,24591},
        { 45,20048},
        { 50,16433},
        { 55,13539},
        { 60,11210}
    };
#endif

    if (Enable_BATDRV_LOG == 1) {
        MT_BAT_BQ24158DB("###### %d <-> %d ######\r\n", Batt_Temperature_Table[9].BatteryTemp,
            Batt_Temperature_Table[9].TemperatureR);
    }

    if(Res >= Batt_Temperature_Table[0].TemperatureR)
    {
        #ifdef CONFIG_DEBUG_MSG_NO_BQ27500
        MT_BAT_BQ24158DB("Res >= %d\n", Batt_Temperature_Table[0].TemperatureR);
        #endif
        //<2012/11/08-tedwu, Integrate the table of thermistor and temperature of BATTERY_PACK_HW4X.
        #if defined(BATTERY_PACK_HW4X)
        TBatt_Value = Batt_Temperature_Table[0].BatteryTemp;
        #else
        TBatt_Value = -20;
        #endif
        //>2012/11/08-tedwu
    }
//<2012/10/02-14542-jessicatseng, [Hawk 4.0] Integrate the table of thermistor and temperature for JB
#if defined(ARIMA_PROJECT_HAWK40)
    else if(Res<=Batt_Temperature_Table[20].TemperatureR)
    {
        #ifdef CONFIG_DEBUG_MSG_NO_BQ27500
        MT_BAT_BQ24158DB("Res <= %d\n", Batt_Temperature_Table[20].TemperatureR);
        #endif
        TBatt_Value = Batt_Temperature_Table[20].BatteryTemp;
    }
//<2012/11/08-tedwu, Integrate the table of thermistor and temperature of BATTERY_PACK_HW4X.
#elif defined(BATTERY_PACK_HW4X)
    else if(Res<=Batt_Temperature_Table[12].TemperatureR)
    {
        #ifdef CONFIG_DEBUG_MSG_NO_BQ27500
        MT_BAT_BQ24158DB("Res <= %d\n", Batt_Temperature_Table[12].TemperatureR);
        #endif
        TBatt_Value = Batt_Temperature_Table[12].BatteryTemp;
    }
//>2012/11/08-tedwu
#else
    else if(Res <= Batt_Temperature_Table[16].TemperatureR)
    {
        #ifdef CONFIG_DEBUG_MSG_NO_BQ27500
        MT_BAT_BQ24158DB("Res <= %d\n", Batt_Temperature_Table[16].TemperatureR);
        #endif
        TBatt_Value = 60;
    }
#endif    
//>2012/10/02-14542-jessicatseng     
    else
    {
        RES1 = Batt_Temperature_Table[0].TemperatureR;
        TMP1 = Batt_Temperature_Table[0].BatteryTemp;
        
//<2012/10/02-14542-jessicatseng, [Hawk 4.0] Integrate the table of thermistor and temperature for JB
#if defined(ARIMA_PROJECT_HAWK40)
        for (i = 0; i <= 20; i++)
//<2012/11/08-tedwu, Integrate the table of thermistor and temperature of BATTERY_PACK_HW4X.
#elif defined(BATTERY_PACK_HW4X)
        for (i = 0; i <= 12; i++)
//>2012/11/08-tedwu
#else
        for (i = 0; i <= 16; i++)
#endif        
//>2012/10/02-14542-jessicatseng     
        {
            if(Res >= Batt_Temperature_Table[i].TemperatureR)
            {
                RES2 = Batt_Temperature_Table[i].TemperatureR;
                TMP2 = Batt_Temperature_Table[i].BatteryTemp;
                break;
            }
            else
            {
                RES1 = Batt_Temperature_Table[i].TemperatureR;
                TMP1 = Batt_Temperature_Table[i].BatteryTemp;
            }
        }

        TBatt_Value = (((Res - RES2) * TMP1) + ((RES1 - Res) * TMP2)) / (RES1-RES2);
    }

    #ifdef CONFIG_DEBUG_MSG_NO_BQ27500
    MT_BAT_BQ24158DB("BattThermistorConverTemp() : TBatt_Value = %d\n",TBatt_Value);
    #endif

    return TBatt_Value;
}

/* convert ADC_bat_temp_volt to register */
INT16 BattVoltToTemp(INT32 dwVolt)
{
    INT32 TRes;
    INT32 dwVCriBat = (TBAT_OVER_CRITICAL_LOW * RBAT_PULL_UP_VOLT) / (TBAT_OVER_CRITICAL_LOW + RBAT_PULL_UP_R); //~2000mV
    INT32 sBaTTMP = -100;

    if(dwVolt > dwVCriBat)
        TRes = TBAT_OVER_CRITICAL_LOW;
    else
        TRes = (RBAT_PULL_UP_R*dwVolt) / (RBAT_PULL_UP_VOLT-dwVolt);

    /* convert register to temperature */
    sBaTTMP = BattThermistorConverTemp(TRes);

    #ifdef CONFIG_DEBUG_MSG_NO_BQ27500
    MT_BAT_BQ24158DB("BattVoltToTemp() : TBAT_OVER_CRITICAL_LOW = %d\n", TBAT_OVER_CRITICAL_LOW);
    MT_BAT_BQ24158DB("BattVoltToTemp() : RBAT_PULL_UP_VOLT = %d\n", RBAT_PULL_UP_VOLT);
    MT_BAT_BQ24158DB("BattVoltToTemp() : dwVolt = %d\n", dwVolt);
    MT_BAT_BQ24158DB("BattVoltToTemp() : TRes = %d\n", TRes);
    MT_BAT_BQ24158DB("BattVoltToTemp() : sBaTTMP = %d\n", sBaTTMP);
    #endif

    return sBaTTMP;
}

//////////////////////////////////////////////////////
//// Pulse Charging Algorithm
//////////////////////////////////////////////////////
kal_bool pmic_chrdet_status(void)
{
    if( upmu_is_chr_det() == KAL_TRUE )
    {
        return KAL_TRUE;
    }
    else
    {
        MT_BAT_BQ24158DB("[pmic_chrdet_status] No charger\r\n");
        return KAL_FALSE;
    }
}

//<2012/12/06-tedwu, change for new CXD charging animation.
#if defined(MOTO_SOP_0296_PARTIAL_CHARGE)
kal_bool Is_Partial_charge(void)
{
    if (CHR_PARTIAL == BMT_status.bat_charging_state)
        return KAL_TRUE;
    else
        return KAL_FALSE;
}
#endif
//>2012/12/06-tedwu

void bq24158_set_ac_current(void)
{
	if (Enable_BATDRV_LOG == 1) {
		printk("[BATTERY:bq24158] bq24158_set_ac_charging_current \r\n");
	}
	//bq24158_config_interface_reg(0x01,0xb8);

    #if 0
    //set the current to 1.25A,
    //1). 0x06h->0x70h  // set safety register first,
    //2). 0x01h->0xF8h
    //3). 0x02h->0x8Eh
    //4). 0x04h->0x79h
    //5). 0x05h->0x04h
    bq24158_config_interface_reg(0x06,0x70);
    bq24158_config_interface_reg(0x01,0xF8);
    bq24158_config_interface_reg(0x02,0x8E);
    bq24158_config_interface_reg(0x04,0x79);
    bq24158_config_interface_reg(0x05,0x04);
    #endif

    //<2012/09/16-tedwu, set charge current.
    #if 0
    //set the current to 650mA,
    //1). 0x06h->0x10h  // set safety register first,
    //2). 0x01h->0xF8h
    //3). 0x02h->0x8Eh
    //4). 0x04h->0x19h
    //5). 0x05h->0x04h
    bq24158_config_interface_reg(0x06,0x10); //set ISAFE
    bq24158_config_interface_reg(0x01,0xF8);
    bq24158_config_interface_reg(0x02,0x8E);
    //bq24158_config_interface_reg(0x02,0xAA);  for hv battery test
    bq24158_config_interface_reg(0x04,0x19); //set IOCHARGE
    bq24158_config_interface_reg(0x05,0x04);
    #endif

    #if defined(ARIMA_PROJECT_HAWK35)
    bq24158_config_interface_reg(0x06,0x78); //MaoyiChou,2013/2/1, Changed target voltage from 4.34v to 4.36v
    bq24158_config_interface_reg(0x01,0x88); // 800mA current limit, 3.4V fast charge
    bq24158_config_interface_reg(0x02,0xAE); //MaoyiChou,2013/2/1, Changed target voltage from 4.34v to 4.36v
    bq24158_config_interface_reg(0x04,0x61); // 782mA  //MaoyiChou,2013/1/30, [Hawk35]Charging full on off mode and then plug out charger to power on, fixed battery capacity not full .
    bq24158_config_interface_reg(0x05,0x04);
    #endif
    //>2012/09/16-tedwu
    
//<2012/09/28-14469-jessicatseng, [Hawk 4.0] Modify charging current for JB
    #if defined(ARIMA_PROJECT_HAWK40)
    bq24158_config_interface_reg(0x06,0x78); //set ISAFE  //MaoyiChou,2013/2/1, Changed target voltage from 4.34v to 4.36v
    bq24158_config_interface_reg(0x01,0x88);
    bq24158_config_interface_reg(0x02,0xAE); //MaoyiChou,2013/2/1, Changed target voltage from 4.34v to 4.36v
    bq24158_config_interface_reg(0x04,0x71); //set IOCHARGE, EricHsieh,2013/1/30,set the cut of current to 34mA.
    bq24158_config_interface_reg(0x05,0x04);
    #endif
//>2012/09/28-14469-jessicatseng    
}

void select_charging_curret_bq24158()
{
    if ( BMT_status.charger_type == STANDARD_HOST )
    {
        g_temp_CC_value = USB_CHARGER_CURRENT;
        //<2012/09/16-tedwu, set charge current.
        bq24158_config_interface_reg(0x01,0x48); // 500mA current limit, 3.4V fast charge
//<2012/10/16-15228-jessicatseng, [Hawk 4.0] Modify charging current for USB
#if defined(ARIMA_PROJECT_HAWK40)
		bq24158_config_interface_reg(0x04,0x24); 
#elif defined(ARIMA_PROJECT_HAWK35)
		//bq24158_config_interface_reg(0x04,0x14); //<2012/09/10-tedwu, set charge current.//>  // 442mA
		bq24158_config_interface_reg(0x04,0x31); //<2012/09/10-tedwu, set charge current.//>  // 578mA //MaoyiChou,2013/1/30, [Hawk35]Charging full on off mode and then plug out charger to power on, fixed battery capacity not full .
		//bq24158_config_interface_reg(0x04,0x44); //<2012/09/10-tedwu, set charge current.//>  // 646mA
#endif
//>2012/10/16-15228-jessicatseng
        //>2012/09/16-tedwu
		if (Enable_BATDRV_LOG == 1) {
			MT_BAT_BQ24158DB("[BATTERY:bq24158] BMT_status.charger_type == STANDARD_HOST \r\n");
		}
    }
    else if (BMT_status.charger_type == NONSTANDARD_CHARGER)
    {
        g_temp_CC_value = USB_CHARGER_CURRENT;
        //<2012/09/16-tedwu, set charge current.
		bq24158_config_interface_reg(0x01,0x48); // 500mA current limit, 3.4V fast charge
//<2012/10/16-15228-jessicatseng, [Hawk 4.0] Modify charging current for USB
#if defined(ARIMA_PROJECT_HAWK40)
		bq24158_config_interface_reg(0x04,0x24); 
#elif defined(ARIMA_PROJECT_HAWK35)
		//bq24158_config_interface_reg(0x04,0x14); //<2012/09/10-tedwu, set charge current.//>  // 442mA
		bq24158_config_interface_reg(0x04,0x31); //<2012/09/10-tedwu, set charge current.//>  // 578mA  //MaoyiChou,2013/1/30, [Hawk35]Charging full on off mode and then plug out charger to power on, fixed battery capacity not full .
		//bq24158_config_interface_reg(0x04,0x44); //<2012/09/10-tedwu, set charge current.//>  // 646mA
#endif		
//>2012/10/16-15228-jessicatseng
        //>2012/09/16-tedwu
		if (Enable_BATDRV_LOG == 1) {
			MT_BAT_BQ24158DB("[BATTERY:bq24158] BMT_status.charger_type == NONSTANDARD_CHARGER \r\n");
		}
    }
    else if (BMT_status.charger_type == STANDARD_CHARGER)
    {
        g_temp_CC_value = AC_CHARGER_CURRENT;
        bq24158_set_ac_current();
        if (Enable_BATDRV_LOG == 1) {
			MT_BAT_BQ24158DB("[BATTERY:bq24158] BMT_status.charger_type == STANDARD_CHARGER \r\n");
		}
    }
    else if (BMT_status.charger_type == CHARGING_HOST)
    {
        g_temp_CC_value = AC_CHARGER_CURRENT;
        bq24158_set_ac_current();
        if (Enable_BATDRV_LOG == 1) {
			MT_BAT_BQ24158DB("[BATTERY:bq24158] BMT_status.charger_type == CHARGING_HOST \r\n");
		}
    }
    else
    {
        g_temp_CC_value = USB_CHARGER_CURRENT;
        //<2012/09/16-tedwu, set charge current.
		bq24158_config_interface_reg(0x01,0x48); // 500mA current limit, 3.4V fast charge
//<2012/10/16-15228-jessicatseng, [Hawk 4.0] Modify charging current for USB
#if defined(ARIMA_PROJECT_HAWK40)
		bq24158_config_interface_reg(0x04,0x24); 
#elif defined(ARIMA_PROJECT_HAWK35)
		//bq24158_config_interface_reg(0x04,0x14); //<2012/09/10-tedwu, set charge current.//>  // 442mA
		bq24158_config_interface_reg(0x04,0x31); //<2012/09/10-tedwu, set charge current.//>  // 578mA  //MaoyiChou,2013/1/30, [Hawk35]Charging full on off mode and then plug out charger to power on, fixed battery capacity not full .
		//bq24158_config_interface_reg(0x04,0x44); //<2012/09/10-tedwu, set charge current.//>  // 646mA
#endif		
//>2012/10/16-15228-jessicatseng
        //>2012/09/16-tedwu
		if (Enable_BATDRV_LOG == 1) {
			MT_BAT_BQ24158DB("[BATTERY:bq24158] default set g_temp_CC_value = USB_CHARGER_CURRENT \r\n");
		}
    }
}

//<<EricHsieh,2012/10/17, power off charging power consumption tuning.
void custom_set_gpio_NC(void)
{
	#if defined(ARIMA_PROJECT_HAWK35)
	int i=0;
	for (i=0; i<=GPIO_MAX;i++){
	if(GPIO_NC_Table[i]< GPIO_MAX && GPIO_NC_Table[i]>=GPIO_NC_Table[0]){
	//MT_BAT_BQ24158DB("GPIO_Set_NC, ==>GPIO_NC_Table[%d]=%d \n",i,GPIO_NC_Table[i]);
	mt_set_gpio_mode(GPIO_NC_Table[i], GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_NC_Table[i], GPIO_DIR_IN);
	//mt_set_gpio_pull_select(GPIO_NC_Table[i], GPIO_PULL_DOWN);
	mt_set_gpio_pull_enable(GPIO_NC_Table[i], GPIO_PULL_DISABLE);
	//mt_set_gpio_out(GPIO_NC_Table[i], GPIO_OUT_ZERO);
	}
	}
	#else if defined(ARIMA_PROJECT_HAWK40)
	#endif
}

void custom_set_gpio(void){
#if defined(ARIMA_PROJECT_HAWK35)
		//GPIO35 (CAM_EN) ==> pull low
		mt_set_gpio_mode(GPIO35, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO35, GPIO_DIR_OUT);
		mt_set_gpio_pull_select(GPIO35, GPIO_PULL_DOWN);
		mt_set_gpio_pull_enable(GPIO35, GPIO_PULL_ENABLE);
		mt_set_gpio_out(GPIO35, GPIO_OUT_ZERO);
	
		//GPIO122 (to MT6628) ==> pull low
		mt_set_gpio_mode(GPIO122, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO122, GPIO_DIR_OUT);
		mt_set_gpio_pull_select(GPIO122, GPIO_PULL_DOWN);
		mt_set_gpio_pull_enable(GPIO122, GPIO_PULL_ENABLE);
		mt_set_gpio_out(GPIO122, GPIO_OUT_ZERO);
	
			//GPIO35
		mt_set_gpio_mode(GPIO35, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO35, GPIO_DIR_OUT);
		mt_set_gpio_pull_select(GPIO35, GPIO_PULL_DOWN);
		mt_set_gpio_pull_enable(GPIO35, GPIO_PULL_ENABLE);
		mt_set_gpio_out(GPIO35, GPIO_OUT_ZERO);
	
			//GPIO50
		mt_set_gpio_mode(GPIO50, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO50, GPIO_DIR_OUT);
		mt_set_gpio_pull_select(GPIO50, GPIO_PULL_DOWN);
		mt_set_gpio_pull_enable(GPIO50, GPIO_PULL_ENABLE);
		mt_set_gpio_out(GPIO50, GPIO_OUT_ZERO);
	
			//GPIO123
		mt_set_gpio_mode(GPIO123, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO123, GPIO_DIR_OUT);
		mt_set_gpio_pull_select(GPIO123, GPIO_PULL_DOWN);
		mt_set_gpio_pull_enable(GPIO123, GPIO_PULL_ENABLE);
		mt_set_gpio_out(GPIO123, GPIO_OUT_ZERO);
	
		//GPIO126
		mt_set_gpio_mode(GPIO126, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO126, GPIO_DIR_OUT);
		mt_set_gpio_pull_select(GPIO126, GPIO_PULL_DOWN);
		mt_set_gpio_pull_enable(GPIO126, GPIO_PULL_ENABLE);
		mt_set_gpio_out(GPIO126, GPIO_OUT_ZERO);
	
		//GPIO127
		mt_set_gpio_mode(GPIO127, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO127, GPIO_DIR_OUT);
		mt_set_gpio_pull_select(GPIO127, GPIO_PULL_DOWN);
		mt_set_gpio_pull_enable(GPIO127, GPIO_PULL_ENABLE);
		mt_set_gpio_out(GPIO127, GPIO_OUT_ZERO);
	
		//GPIO186
		mt_set_gpio_mode(GPIO186, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO186, GPIO_DIR_OUT);
		mt_set_gpio_pull_select(GPIO186, GPIO_PULL_DOWN);
		mt_set_gpio_pull_enable(GPIO186, GPIO_PULL_ENABLE);
		mt_set_gpio_out(GPIO186, GPIO_OUT_ZERO);
	
		//GPIO221
		mt_set_gpio_mode(GPIO221, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO221, GPIO_DIR_OUT);
		mt_set_gpio_pull_select(GPIO221, GPIO_PULL_DOWN);
		mt_set_gpio_pull_enable(GPIO221, GPIO_PULL_ENABLE);
		mt_set_gpio_out(GPIO221, GPIO_OUT_ZERO);
	
		
		//GPIO228
		mt_set_gpio_mode(GPIO228, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO228, GPIO_DIR_OUT);
		mt_set_gpio_pull_select(GPIO228, GPIO_PULL_DOWN);
		mt_set_gpio_pull_enable(GPIO228, GPIO_PULL_ENABLE);
		mt_set_gpio_out(GPIO228, GPIO_OUT_ZERO);
	#else if defined(ARIMA_PROJECT_HAWK40)
		
    #endif //defined(ARIMA_PROJECT_HAWK35)

}

void custom_set_pmic(void)
{
    #if defined(ARIMA_PROJECT_HAWK35)
	kal_uint32 ret=0;

	 MT_BAT_BQ24158DB("==>custom_set_pmic \n");
	 
	ret=pmic_config_interface(BANK0_DIGLDO_CON29, 0x0, BANK_0_RG_VMCH_EN_MASK, BANK_0_RG_VMCH_EN_SHIFT);
	ret=pmic_config_interface(BANK0_DIGLDO_CON30, 0x0, BANK_0_RG_VGP2_EN_MASK, BANK_0_RG_VGP2_EN_SHIFT);/* VGP2: for touch panel 1V8 (VTP_1V8) => Disable */
 	ret=pmic_config_interface(BANK0_DIGLDO_CON1C, 0x0, BANK_0_RG_VCAMD_EN_MASK, BANK_0_RG_VCAMD_EN_SHIFT); /* VCAMD: for camera IO => Disable */
	ret=pmic_config_interface(BANK0_DIGLDO_CON1F, 0x0, BANK_0_RG_VCAM_IO_EN_MASK, BANK_0_RG_VCAM_IO_EN_SHIFT); /* VCAM_IO: for camera IO => Disable */
	ret=pmic_config_interface(BANK0_DIGLDO_CON22, 0x0, BANK_0_RG_VCAM_AF_EN_MASK, BANK_0_RG_VCAM_AF_EN_SHIFT); /* VCAM_IO: for camera AF => Disable */
	ret=pmic_config_interface(BANK0_DIGLDO_CON33, 0x0, BANK_0_RG_VIBR_EN_MASK, BANK_0_RG_VIBR_EN_SHIFT); /* VIBR */
	ret=pmic_config_interface(BANK0_DIGLDO_CON13, 0x0, BANK_0_RG_VSIM_EN_MASK, BANK_0_RG_VSIM_EN_SHIFT); /* VIBR */
	ret=pmic_config_interface(BANK0_DIGLDO_CON16, 0x0, BANK_0_RG_VSIM2_EN_MASK, BANK_0_RG_VSIM2_EN_SHIFT); /* VIBR */
	ret=pmic_config_interface(BANK0_ANALDO_CONE, 0x0, BANK_0_RG_VCAMA_EN_MASK, BANK_0_RG_VCAMA_EN_SHIFT); /* VCAMA: for camera IO => Disable */
   	ret=pmic_config_interface(BANK0_ANALDO_CON4, 0x0, BANK_0_RG_VTCXO_EN_MASK, BANK_0_RG_VTCXO_EN_SHIFT);
    ret=pmic_config_interface(BANK0_ANALDO_CONB, 0x0, BANK_0_RG_VA2_EN_MASK, BANK_0_RG_VA2_EN_SHIFT);	
	ret=pmic_config_interface(BANK0_ANALDO_CON1, 0x0, BANK_0_RG_VRF_EN_MASK, BANK_0_RG_VRF_EN_SHIFT);
	ret=pmic_config_interface(BANK0_ANALDO_CON1, 0x0, BANK_0_VRF_ON_CTRL_MASK, BANK_0_VRF_ON_CTRL_SHIFT);
	ret=pmic_config_interface(BANK1_SPK_CON0, 0x0, BANK_1_SPK_EN_L_MASK, BANK_1_SPK_EN_L_SHIFT);
	ret=pmic_config_interface(BANK1_SPK_CON6, 0x0, BANK_1_SPK_EN_R_MASK, BANK_1_SPK_EN_R_SHIFT);
	//<<MaoyiChou,2012/10/19, Modiy GPIO table and Power.
	// Disable VUSB
    ret=pmic_config_interface((kal_uint8)(BANK0_LDO_VUSB+LDO_CON1_OFFSET), 
                            (kal_uint8)(0),
                            (kal_uint8)(BANK_0_RG_VUSB_EN_MASK),
                            (kal_uint8)(BANK_0_RG_VUSB_EN_SHIFT)
                            );
	//>>MaoyiChou,2012/10/19, Modiy GPIO table and Power.

    custom_set_gpio_NC();
	
	#else if defined(ARIMA_PROJECT_HAWK40)
    #endif //defined(ARIMA_PROJECT_HAWK35)
}
//>>EricHsieh,2012/10/17, power off charging power consumption tuning.

//<2012/09/16-tedwu, temporarily put code here to reduce the power consumption.
static void reduce_power_consumption_1(void)
{
    #if defined(ARIMA_PROJECT_HAWK35)
    //GPIO35 (CAM_EN) ==> pull low
    mt_set_gpio_mode(GPIO35, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO35, GPIO_DIR_OUT);
	mt_set_gpio_pull_select(GPIO35, GPIO_PULL_DOWN);
	mt_set_gpio_pull_enable(GPIO35, GPIO_PULL_ENABLE);
	mt_set_gpio_out(GPIO35, GPIO_OUT_ZERO);

    //GPIO122 (to MT6628) ==> pull low
    mt_set_gpio_mode(GPIO122, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO122, GPIO_DIR_OUT);
	mt_set_gpio_pull_select(GPIO122, GPIO_PULL_DOWN);
	mt_set_gpio_pull_enable(GPIO122, GPIO_PULL_ENABLE);
	mt_set_gpio_out(GPIO122, GPIO_OUT_ZERO);
    #endif //defined(ARIMA_PROJECT_HAWK35)
}

static void reduce_power_consumption_2(void)
{
    #if defined(ARIMA_PROJECT_HAWK35)
    // VMCH: for micro-SD
    pmic_config_interface( 	(kal_uint8)(/* BANK0_LDO_VMCH */ 0xAB), 
                            (kal_uint8)(0), /* VMCH: for micro-SD => Disable */
                            (kal_uint8)(BANK_0_RG_VMCH_EN_MASK),
                            (kal_uint8)(BANK_0_RG_VMCH_EN_SHIFT)
                            );
#if 0
    // VGP2: for touch panel 1V8 (VTP_1V8)
    pmic_config_interface( 	(kal_uint8)(/* BANK0_LDO_VGP2 */ 0xB2), 
                            (kal_uint8)(0), /* VGP2: for touch panel 1V8 (VTP_1V8) => Disable */
                            (kal_uint8)(BANK_0_RG_VGP2_EN_MASK),
                            (kal_uint8)(BANK_0_RG_VGP2_EN_SHIFT)
                            );
#endif
    // VCAM_IO: for camera IO
    pmic_config_interface( 	(kal_uint8)(/* BANK0_LDO_VCAM_IO */ 0xA1), 
                            (kal_uint8)(0), /* VCAM_IO: for camera IO => Disable */
                            (kal_uint8)(BANK_0_RG_VCAM_IO_EN_MASK),
                            (kal_uint8)(BANK_0_RG_VCAM_IO_EN_SHIFT)
                            );
    #endif //defined(ARIMA_PROJECT_HAWK35)
}

static void change_supplied_voltage_level(void)
{
    #if defined(ARIMA_PROJECT_HAWK35)
    // VCORE
    pmic_config_interface( BANK0_VCORE_CON5, 
                            (kal_uint8)(8), /* VCORE: 0.9 V */
                            (kal_uint8)(BANK_0_RG_VCORE_VOSEL_MASK),
                            (kal_uint8)(BANK_0_RG_VCORE_VOSEL_SHIFT)
                            );
    pmic_config_interface( BANK0_VCORE_CON7, 
                            (kal_uint8)(0), /* by SW RG_VCORE_VOSEL */
                            (kal_uint8)(BANK_0_RG_VCORE_CTRL_MASK),
                            (kal_uint8)(BANK_0_RG_VCORE_CTRL_SHIFT)
                            );

    // VPROC
    pmic_config_interface( 	(kal_uint8)(BANK0_VPROC_CON5), 
                            (kal_uint8)(8), /* 0.9 V */
                            (kal_uint8)(BANK_0_RG_VPROC_VOSEL_MASK),
                            (kal_uint8)(BANK_0_RG_VPROC_VOSEL_SHIFT)
                            );
    pmic_config_interface( 	(kal_uint8)(BANK0_VPROC_CONC), 
                            (kal_uint8)(0), /* by SW RG_VPROC_VOSEL */
                            (kal_uint8)(BANK_0_RG_VPROC_CTRL_MASK),
                            (kal_uint8)(BANK_0_RG_VPROC_CTRL_SHIFT)
                            );

    // DLDO-VM12_INT: 1.246V -> 0.9V


    // VTCXO: 2.8V -> 0V
    pmic_config_interface( 	(kal_uint8)(/* BANK0_LDO_VTCXO */ 0xC1), 
                            (kal_uint8)(0), /*  */
                            (kal_uint8)(BANK_0_RG_VTCXO_EN_MASK),
                            (kal_uint8)(BANK_0_RG_VTCXO_EN_SHIFT)
                            );

    //VUSB: 3.3V -> 0V


    // AVDD25: 2.48V -> 0V
    pmic_config_interface( 	(kal_uint8)(/* BANK0_LDO_VA2 */ BANK0_ANALDO_CONB), 
                            (kal_uint8)(0), /* 0: disable */
                            (kal_uint8)(BANK_0_RG_VA2_EN_MASK),
                            (kal_uint8)(BANK_0_RG_VA2_EN_SHIFT)
                            );
    #endif //defined(ARIMA_PROJECT_HAWK35)
}

static void restore_back_power(void)
{
    #if defined(ARIMA_PROJECT_HAWK35)
#if 0
    // VGP2: for touch panel 1V8 (VTP_1V8)
    pmic_config_interface( 	(kal_uint8)(/* BANK0_LDO_VGP2 */ 0xB2), 
                            (kal_uint8)(1), /* 1 : enable */
                            (kal_uint8)(BANK_0_RG_VGP2_EN_MASK),
                            (kal_uint8)(BANK_0_RG_VGP2_EN_SHIFT)
                            );
#endif
    #endif //defined(ARIMA_PROJECT_HAWK35)
}
//>2012/09/16-tedwu

static unsigned int temp_init_flag = 0;
void ChargerHwInit_bq24158(void)
{
	if (Enable_BATDRV_LOG == 1) {
        printk("[MT BAT_probe] ChargerHwInit\n" );
	}

	if(temp_init_flag==0)
	{
        //<2012/09/16-tedwu, set charge current.
        #if defined(ARIMA_PROJECT_HAWK35)
        bq24158_config_interface_reg(0x06,0x78); //MaoyiChou,2013/2/1, Changed target voltage from 4.34v to 4.36v
	    bq24158_config_interface_reg(0x00,0x80);
        bq24158_config_interface_reg(0x01,0x88); // 800mA current limit, 3.4V fast charge
        bq24158_config_interface_reg(0x02,0xAE); //MaoyiChou,2013/2/1, Changed target voltage from 4.34v to 4.36v
        bq24158_config_interface_reg(0x04,0x61); // 782mA  //MaoyiChou,2013/1/30, [Hawk35]Charging full on off mode and then plug out charger to power on, fixed battery capacity not full .
        bq24158_config_interface_reg(0x05,0x04);
//<2012/09/28-14469-jessicatseng, [Hawk 4.0] Modify charging current for JB
        #elif defined(ARIMA_PROJECT_HAWK40)
	    bq24158_config_interface_reg(0x06,0x78); //set ISAFE  //MaoyiChou,2013/2/1, Changed target voltage from 4.34v to 4.36v
	    bq24158_config_interface_reg(0x00,0x80);
	    bq24158_config_interface_reg(0x01,0x88);
	    bq24158_config_interface_reg(0x02,0xAE); //MaoyiChou,2013/2/1, Changed target voltage from 4.34v to 4.36v
	    bq24158_config_interface_reg(0x04,0x71); //set IOCHARGE,EricHsieh,2013/1/30,set the cut of current to 34mA.
	    bq24158_config_interface_reg(0x05,0x04);
//>2012/09/28-14469-jessicatseng        
        #else
	    bq24158_config_interface_reg(0x06,0x70);
	    bq24158_config_interface_reg(0x00,0x80);
	    bq24158_config_interface_reg(0x01,0xb1);
	    bq24158_config_interface_reg(0x02,0x8e);
	   // bq24158_config_interface_reg(0x02,0xAA);  for hv battery test
	    bq24158_config_interface_reg(0x05,0x04);
	    bq24158_config_interface_reg(0x04,0x19);
        #endif
        //>2012/09/16-tedwu
	    temp_init_flag =1;	
	}
	else
	{
    	bq24158_config_interface_reg(0x00,0x80);
	}
}

#if 0
int gpio_number   = GPIO162;
int gpio_off_mode = GPIO_MODE_GPIO;
int gpio_off_dir  = GPIO_DIR_OUT;
int gpio_off_out  = GPIO_OUT_ONE;
int gpio_on_mode  = GPIO_MODE_GPIO;
int gpio_on_dir   = GPIO_DIR_OUT;
int gpio_on_out   = GPIO_OUT_ZERO;
#endif
int gpio_on_out   = 0;

void pchr_turn_off_charging_bq24158 (void)
{
    if (Enable_BATDRV_LOG == 1) {
        MT_BAT_BQ24158DB("[BATTERY] pchr_turn_off_charging_bq24158 !\r\n");
    }


#if 0
    mt_set_gpio_mode(gpio_number,gpio_off_mode);  //57 mode0
    mt_set_gpio_dir(gpio_number,gpio_off_dir);
    mt_set_gpio_out(gpio_number,gpio_off_out);
#endif

//<2012/08/09-kylechang, Compiler Error(GPIO_SWCHARGER_EN_PIN undefined)
//#if defined(CONFIG_SWCHARGER_DEFAULT_OFF)
//	   mt_set_gpio_out(GPIO_SWCHARGER_EN_PIN, gpio_on_out);
//#else
//   if( gpio_on_out == 0 )
//	   mt_set_gpio_out(GPIO_SWCHARGER_EN_PIN,1);
//   else
//	   mt_set_gpio_out(GPIO_SWCHARGER_EN_PIN,0);
//#endif
//>2012/08/09-kylechang

//<2012/10/31-15993-jessicatseng, [Hawk 4.0] Don't turn off charging IC temporarily 
#if defined(ARIMA_PROJECT_HAWK40)
	bq24158_config_interface_reg(0x04,0x80);
//<2012/11/22-17459-jessicatseng, [Hawk 4.0] Charger IC is enabled / disabled by GPIO54 for EP2
	mt_set_gpio_mode(GPIO_CHARGER_IC_CD_PIN, GPIO_HEADSET_SW_EN_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CHARGER_IC_CD_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CHARGER_IC_CD_PIN, GPIO_OUT_ONE);
//>2012/11/22-17459-jessicatseng
#else
	bq24158_config_interface_reg(0x01,0xbc);
#endif
//>2012/10/31-15993-jessicatseng
}

void pchr_turn_on_charging_bq24158 (void)
{

#if 0
    mt_set_gpio_mode(gpio_number,gpio_on_mode);  //57 mode0
    mt_set_gpio_dir(gpio_number,gpio_on_dir);
    mt_set_gpio_out(gpio_number,gpio_on_out);
#endif

//<2012/11/22-17459-jessicatseng, [Hawk 4.0] Charger IC is enabled / disabled by GPIO54 for EP2
#if defined(ARIMA_PROJECT_HAWK40)
	mt_set_gpio_mode(GPIO_CHARGER_IC_CD_PIN, GPIO_HEADSET_SW_EN_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CHARGER_IC_CD_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CHARGER_IC_CD_PIN, GPIO_OUT_ZERO);
#endif		
//>2012/11/22-17459-jessicatseng

//<2012/08/09-kylechang, Compiler Error(GPIO_SWCHARGER_EN_PIN undefined)
//#if defined(CONFIG_SWCHARGER_DEFAULT_OFF)
//    if( gpio_on_out == 0 )
//        mt_set_gpio_out(GPIO_SWCHARGER_EN_PIN,1);
//    else
//        mt_set_gpio_out(GPIO_SWCHARGER_EN_PIN,0);
//#else
//        mt_set_gpio_out(GPIO_SWCHARGER_EN_PIN, gpio_on_out);
//#endif
//>2012/08/09-kylechang

    if ( BMT_status.bat_charging_state == CHR_ERROR )
    {
        //if (Enable_BATDRV_LOG == 1) {
            MT_BAT_BQ24158DB("[BATTERY] Charger Error, turn OFF charging !\r\n");
        //}
        pchr_turn_off_charging_bq24158();
    }
    else
    {
        ChargerHwInit_bq24158();

        if (Enable_BATDRV_LOG == 1) {
            MT_BAT_BQ24158DB("[BATTERY] pchr_turn_on_charging_bq24158 !\r\n");
        }

        select_charging_curret_bq24158();

        if( g_temp_CC_value == Cust_CC_0MA)
        {
            pchr_turn_off_charging_bq24158();
            MT_BAT_BQ24158DB("[BATTERY] g_temp_CC_value == Cust_CC_0MA !\r\n");
        }
        else
        {
            if (Enable_BATDRV_LOG == 1) {
				MT_BAT_BQ24158DB("[BATTERY] charger enable !\r\n");
			}
        }
    }
}

int BAT_CheckPMUStatusReg(void)
{
    if( upmu_is_chr_det() == KAL_TRUE )
    {
        BMT_status.charger_exist = TRUE;
    }
    else
    {
        BMT_status.charger_exist = FALSE;

        BMT_status.total_charging_time = 0;
        BMT_status.PRE_charging_time = 0;
        BMT_status.CC_charging_time = 0;
        BMT_status.TOPOFF_charging_time = 0;
        BMT_status.POSTFULL_charging_time = 0;

        BMT_status.bat_charging_state = CHR_PRE;

//<2012/7/9-tedwu, check MOTO battery ID
#if defined(CHK_BATTERY_ID__MOTO_ID)
		bat_id_init_variable();
#endif
//>2012/7/9-tedwu
        
        if (Enable_BATDRV_LOG == 1) {
            MT_BAT_BQ24158DB("[BATTERY] BAT_CheckPMUStatusReg : charger loss \n");
        }

        return PMU_STATUS_FAIL;
    }

    return PMU_STATUS_OK;
}

int g_Get_I_Charging(void)
{
	kal_int32 ADC_BAT_SENSE_tmp[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	kal_int32 ADC_BAT_SENSE_sum=0;
	kal_int32 ADC_BAT_SENSE=0;
	kal_int32 ADC_I_SENSE_tmp[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	kal_int32 ADC_I_SENSE_sum=0;
	kal_int32 ADC_I_SENSE=0;
	int repeat=20;
	int i=0;
	int j=0;
	kal_int32 temp=0;
	int ICharging=0;

	for(i=0 ; i<repeat ; i++)
	{
		ADC_BAT_SENSE_tmp[i] = PMIC_IMM_GetOneChannelValue(AUXADC_BATTERY_VOLTAGE_CHANNEL,1);
		ADC_I_SENSE_tmp[i] = PMIC_IMM_GetOneChannelValue(AUXADC_REF_CURRENT_CHANNEL,1);

		ADC_BAT_SENSE_sum += ADC_BAT_SENSE_tmp[i];
		ADC_I_SENSE_sum += ADC_I_SENSE_tmp[i];
	}

	//sorting	BAT_SENSE
	for(i=0 ; i<repeat ; i++)
	{
		for(j=i; j<repeat ; j++)
		{
			if( ADC_BAT_SENSE_tmp[j] < ADC_BAT_SENSE_tmp[i] )
			{
				temp = ADC_BAT_SENSE_tmp[j];
				ADC_BAT_SENSE_tmp[j] = ADC_BAT_SENSE_tmp[i];
				ADC_BAT_SENSE_tmp[i] = temp;
			}
		}
	}
	if (Enable_BATDRV_LOG == 1) {
		MT_BAT_BQ24158DB("[g_Get_I_Charging:BAT_SENSE]\r\n");
		for(i=0 ; i<repeat ; i++ )
		{
			MT_BAT_BQ24158DB("%d,", ADC_BAT_SENSE_tmp[i]);
		}
		MT_BAT_BQ24158DB("\r\n");
	}

	//sorting	I_SENSE
	for(i=0 ; i<repeat ; i++)
	{
		for(j=i ; j<repeat ; j++)
		{
			if( ADC_I_SENSE_tmp[j] < ADC_I_SENSE_tmp[i] )
			{
				temp = ADC_I_SENSE_tmp[j];
				ADC_I_SENSE_tmp[j] = ADC_I_SENSE_tmp[i];
				ADC_I_SENSE_tmp[i] = temp;
			}
		}
	}
	if (Enable_BATDRV_LOG == 1) {
		MT_BAT_BQ24158DB("[g_Get_I_Charging:I_SENSE]\r\n");
		for(i=0 ; i<repeat ; i++ )
		{
			MT_BAT_BQ24158DB("%d,", ADC_I_SENSE_tmp[i]);
		}
		MT_BAT_BQ24158DB("\r\n");
	}

	ADC_BAT_SENSE_sum -= ADC_BAT_SENSE_tmp[0];
	ADC_BAT_SENSE_sum -= ADC_BAT_SENSE_tmp[1];
	ADC_BAT_SENSE_sum -= ADC_BAT_SENSE_tmp[18];
	ADC_BAT_SENSE_sum -= ADC_BAT_SENSE_tmp[19];
	ADC_BAT_SENSE = ADC_BAT_SENSE_sum / (repeat-4);

	if (Enable_BATDRV_LOG == 1) {
		MT_BAT_BQ24158DB("[g_Get_I_Charging] ADC_BAT_SENSE=%d\r\n", ADC_BAT_SENSE);
	}

	ADC_I_SENSE_sum -= ADC_I_SENSE_tmp[0];
	ADC_I_SENSE_sum -= ADC_I_SENSE_tmp[1];
	ADC_I_SENSE_sum -= ADC_I_SENSE_tmp[18];
	ADC_I_SENSE_sum -= ADC_I_SENSE_tmp[19];
	ADC_I_SENSE = ADC_I_SENSE_sum / (repeat-4);

	if (Enable_BATDRV_LOG == 1) {
		MT_BAT_BQ24158DB("[g_Get_I_Charging] ADC_I_SENSE(Before)=%d\r\n", ADC_I_SENSE);
	}

	ADC_I_SENSE += gADC_I_SENSE_offset;

	if (Enable_BATDRV_LOG == 1) {
		MT_BAT_BQ24158DB("[g_Get_I_Charging] ADC_I_SENSE(After)=%d\r\n", ADC_I_SENSE);
	}

	BMT_status.ADC_BAT_SENSE = ADC_BAT_SENSE;
	BMT_status.ADC_I_SENSE = ADC_I_SENSE;

	if(ADC_I_SENSE > ADC_BAT_SENSE)
	{
    	ICharging = (ADC_I_SENSE - ADC_BAT_SENSE)*10/R_CURRENT_SENSE;
	}
	else
	{
    	ICharging = 0;
	}

	return ICharging;
}

void BAT_Vbat_Compensate(void)
{
    if( (upmu_is_chr_det() == KAL_TRUE) && (g_bat_full_user_view != KAL_TRUE) )
    {
        if(BMT_status.ADC_BAT_SENSE <= vbat_compensate_cp)
        {
            MT_BAT_BQ24158DB("[vbat compensate before] BMT_status.bat_vol = %d\r\n", BMT_status.ADC_BAT_SENSE);
            BMT_status.ADC_BAT_SENSE = BMT_status.ADC_BAT_SENSE - vbat_compensate_value;
            MT_BAT_BQ24158DB("[vbat compensate after ] BMT_status.bat_vol = %d\r\n", BMT_status.ADC_BAT_SENSE);
        }
    }
}

void BAT_GetVoltage_bq24158(void)
{
    int bat_temperature_volt=0;

	 //gpt_busy_wait_us(20000);  //delay 20ms for baton temperature stability

    /* Get V_BAT_SENSE */
    if (g_chr_event == 0)
    {
        BMT_status.ADC_BAT_SENSE = PMIC_IMM_GetOneChannelValue(AUXADC_BATTERY_VOLTAGE_CHANNEL, 1);
    }
    else
    {
        /* Just charger in/out event, same as I_sense */
        g_chr_event = 0;
        BMT_status.ADC_BAT_SENSE = PMIC_IMM_GetOneChannelValue(AUXADC_REF_CURRENT_CHANNEL, 1);
    }
    if (batteryBufferFirst)
    {
    BAT_Vbat_Compensate();
    }
    else
    {
        if( (upmu_is_chr_det() == KAL_TRUE) && (g_bat_full_user_view != KAL_TRUE) )
        {
            if(BMT_status.ADC_BAT_SENSE <= vbat_compensate_cp)
            {
                MT_BAT_BQ24158DB("[vbat first compensate before] BMT_status.bat_vol = %d\r\n", BMT_status.ADC_BAT_SENSE);
                BMT_status.ADC_BAT_SENSE = BMT_status.ADC_BAT_SENSE + 20;
                MT_BAT_BQ24158DB("[vbat first compensate after ] BMT_status.bat_vol = %d\r\n", BMT_status.ADC_BAT_SENSE);
            }
        }
    }
    BMT_status.bat_vol = BMT_status.ADC_BAT_SENSE;

	if (g_eco_version == PMIC6329_E1_CID_CODE)
	{
		g_E1_vbat_sense = BMT_status.ADC_BAT_SENSE;
		if (Enable_BATDRV_LOG == 1) {
			MT_BAT_BQ24158DB("[Charger_E1] Get g_E1_vbat_sense = %d\r\n", g_E1_vbat_sense);
		}
	}

    /* Get V_I_SENSE */
    //BMT_status.ADC_I_SENSE = PMIC_IMM_GetOneChannelValue(AUXADC_REF_CURRENT_CHANNEL, 1);
	//BMT_status.ADC_I_SENSE += gADC_I_SENSE_offset;

    /* Get V_Charger */
    BMT_status.charger_vol = PMIC_IMM_GetOneChannelValue(AUXADC_CHARGER_VOLTAGE_CHANNEL, 5);
    BMT_status.charger_vol = BMT_status.charger_vol / 100;

	/* Get V_BAT_Temperature */
	bat_temperature_volt = PMIC_IMM_GetOneChannelValue(AUXADC_TEMPERATURE_CHANNEL,5);
	if(bat_temperature_volt == 0)
	{
		BMT_status.temperature = g_bat_temperature_pre;
		if (Enable_BATDRV_LOG == 1) {
			MT_BAT_BQ24158DB("[BATTERY] Warning !! bat_temperature_volt == 0, restore temperature value\n\r");
		}
	}
	else
	{
		BMT_status.temperature = BattVoltToTemp(bat_temperature_volt);
		g_bat_temperature_pre = BMT_status.temperature;
	}

    /* Calculate the charging current */
    //if(BMT_status.ADC_I_SENSE > BMT_status.ADC_BAT_SENSE)
    //    BMT_status.ICharging = (BMT_status.ADC_I_SENSE - BMT_status.ADC_BAT_SENSE) * 10 / R_CURRENT_SENSE;
    //else
    //    BMT_status.ICharging = 0;
    //BMT_status.ICharging = g_Get_I_Charging();

    //if (Enable_BATDRV_LOG == 1) {
    //    MT_BAT_BQ24158DB("[BATTERY:ADC] VCHR:%d BAT_SENSE:%d I_SENSE:%d Current:%d\n", BMT_status.charger_vol,
    //    BMT_status.ADC_BAT_SENSE, BMT_status.ADC_I_SENSE, BMT_status.ICharging);
    //}

    g_BatteryAverageCurrent = BMT_status.ICharging;
}

void BAT_GetVoltage_notbat_bq24158(void)
{
    /* Get V_BAT_SENSE */
    if (g_chr_event == 0)
    {
        BMT_status.ADC_BAT_SENSE = PMIC_IMM_GetOneChannelValue(AUXADC_BATTERY_VOLTAGE_CHANNEL, 1);
    }
    else
    {
        /* Just charger in/out event, same as I_sense */
        g_chr_event = 0;
        BMT_status.ADC_BAT_SENSE = PMIC_IMM_GetOneChannelValue(AUXADC_REF_CURRENT_CHANNEL, 1);
    }
    BMT_status.bat_vol = BMT_status.ADC_BAT_SENSE;

	if (g_eco_version == PMIC6329_E1_CID_CODE)
	{
		g_E1_vbat_sense = BMT_status.ADC_BAT_SENSE;
		if (Enable_BATDRV_LOG == 1) {
			MT_BAT_BQ24158DB("[Charger_E1] Get g_E1_vbat_sense = %d\r\n", g_E1_vbat_sense);
		}
	}

    /* Get V_I_SENSE */
    BMT_status.ADC_I_SENSE = PMIC_IMM_GetOneChannelValue(AUXADC_REF_CURRENT_CHANNEL, 1);
	BMT_status.ADC_I_SENSE += gADC_I_SENSE_offset;

    /* Get V_Charger */
    BMT_status.charger_vol = PMIC_IMM_GetOneChannelValue(AUXADC_CHARGER_VOLTAGE_CHANNEL, 2);
    BMT_status.charger_vol = BMT_status.charger_vol / 100;

//<2012/11/07-16416-jessicatseng, [Hawk 4.0][BU2SC00138827] Your Battery is "Over Current" protection during charging
    /* Calculate the charging current */
    if(BMT_status.ADC_I_SENSE > BMT_status.ADC_BAT_SENSE)
	{
#if defined(ARIMA_PROJECT_HAWK40)
        BMT_status.ICharging = (BMT_status.ADC_I_SENSE - BMT_status.ADC_BAT_SENSE) * 1000 / 82;	//R_CURRENT_SENSE;
//<2013/01/02-tedwu, [Hawk3.5][Defect:BU2SC00142076] Pop screen show "your battery is over current protection".
#elif defined(ARIMA_PROJECT_HAWK35)
        BMT_status.ICharging = (BMT_status.ADC_I_SENSE - BMT_status.ADC_BAT_SENSE) * 1000 / 100; //R_CURRENT_SENSE;
//>2013/01/02-tedwu
#else
        BMT_status.ICharging = (BMT_status.ADC_I_SENSE - BMT_status.ADC_BAT_SENSE) * 10 / R_CURRENT_SENSE;
#endif
	}
    else
        BMT_status.ICharging = 0;
//>2012/11/07-16416-jessicatseng

    //if (Enable_BATDRV_LOG == 1) {
        MT_BAT_BQ24158DB("[BAT_GetVoltage_notbat] VCHR:%d BAT_SENSE:%d I_SENSE:%d Current:%d\n", BMT_status.charger_vol,
        BMT_status.ADC_BAT_SENSE, BMT_status.ADC_I_SENSE, BMT_status.ICharging);
    //}

    g_BatteryAverageCurrent = BMT_status.ICharging;
}

//<2012/11/22-17513-jessicatseng, [Hawk 4.0] Add ZCV table for LG cell
#if defined(CHK_BATTERY_ID__MOTO_ID) && defined(BATTERY_PACK_EG30)
void BatteryTableContruct(void)
{
	g_MOTO_Battery_ID = Get_MOTO_Battery_Vender();

	printk("[BatteryTableContruct] g_MOTO_Battery_ID=%d\n", g_MOTO_Battery_ID);
	
	if(g_MOTO_Battery_ID == 0x01)
		memcpy(Batt_VoltToPercent_Table, Batt_VoltToPercent_Table_SDI, sizeof(Batt_VoltToPercent_Table));
	else
		memcpy(Batt_VoltToPercent_Table, Batt_VoltToPercent_Table_LG, sizeof(Batt_VoltToPercent_Table));		
	
	{
		INT16 i, table_size = sizeof(Batt_VoltToPercent_Table) / sizeof(VBAT_TO_PERCENT);
			
		printk("table_size=%d, data=", table_size);
		for(i=0; i<table_size; i++)
		 printk(" %d", Batt_VoltToPercent_Table[i]);
		printk("\n");
	}
}
#endif
//>2012/11/22-17513-jessicatseng

UINT32 BattVoltToPercent(UINT16 dwVoltage)
{
    UINT32 m = 0;
    UINT32 VBAT1 = 0, VBAT2 = 0;
    UINT32 bPercntResult = 0, bPercnt1 = 0, bPercnt2 = 0;

    //if (Enable_BATDRV_LOG == 1) {
    //    MT_BAT_BQ24158DB("###### 100 <-> voltage : %d ######\r\n", Batt_VoltToPercent_Table[10].BattVolt);
    //}

    if(dwVoltage <= Batt_VoltToPercent_Table[0].BattVolt)
    {
        bPercntResult = Batt_VoltToPercent_Table[0].BattPercent;
        return bPercntResult;
    }
    else if (dwVoltage >= Batt_VoltToPercent_Table[10].BattVolt)
    {
        bPercntResult = Batt_VoltToPercent_Table[10].BattPercent;
        return bPercntResult;
    }
    else
    {
        VBAT1 = Batt_VoltToPercent_Table[0].BattVolt;
        bPercnt1 = Batt_VoltToPercent_Table[0].BattPercent;
        for(m = 1; m <= 10; m++)
        {
            if(dwVoltage <= Batt_VoltToPercent_Table[m].BattVolt)
            {
                VBAT2 = Batt_VoltToPercent_Table[m].BattVolt;
                bPercnt2 = Batt_VoltToPercent_Table[m].BattPercent;
                break;
            }
            else
            {
                VBAT1 = Batt_VoltToPercent_Table[m].BattVolt;
                bPercnt1 = Batt_VoltToPercent_Table[m].BattPercent;
            }
        }
    }

    bPercntResult = ( ((dwVoltage - VBAT1) * bPercnt2) + ((VBAT2 - dwVoltage) * bPercnt1) ) / (VBAT2 - VBAT1);

    return bPercntResult;
}

int BAT_CheckBatteryStatus_bq24158(void)
{
    //int BAT_status = PMU_STATUS_OK;
    int i = 0;
    //int ret_check_I_charging = 0;
    //int j = 0;
    //int sw_chr_out_flag = 0;
    //int repeat_times = 10;
    //int iii = 0;
//<2012/10/12-15016-jessicatseng, [Hawk 4.0]Fix battery's voltage is more than 4.11V, SOC will show 100%
	kal_uint32 bq24158_status=0;
//>2012/10/12-15016-jessicatseng

//<2012/7/9-tedwu, check MOTO battery ID
//<2012/01/02-2106-jessicatseng, Add code to check MOTO battery ID for DS2502 and bq2024
#if defined(CHK_BATTERY_ID__MOTO_ID)    
	if(Is_MOTO_Battery() == KAL_FALSE)
	{	
  		BMT_status.bat_charging_state = CHR_ERROR;
   		return PMU_STATUS_FAIL;        
 	}	 
#endif 	    
//>2012/01/02-2106-jessicatseng
//>2012/7/9-tedwu
    
    /* Get Battery Information */
    BAT_GetVoltage_bq24158();

    #if 0
    if ((upmu_is_chr_det() == KAL_TRUE) && (g_HW_Charging_Done == 0) &&
        (BMT_status.bat_charging_state != CHR_ERROR) &&
        (BMT_status.bat_charging_state != CHR_TOP_OFF))
    {
        if ((BMT_status.total_charging_time % 10) == 0)
        {
            g_HW_stop_charging = 1;
            //MT_BAT_BQ24158DB("Disable charging 1s\n");
        }
        else
        {
            g_HW_stop_charging = 0;
            //MT_BAT_BQ24158DB("Charging 1s\n");
        }
    }
    else
    {
        g_HW_stop_charging = 0;
        //MT_BAT_BQ24158DB("SW CV mode do not dis-charging 1s\n");
    }
    #endif

    /* Re-calculate Battery Percentage (SOC) */
    BMT_status.SOC = BattVoltToPercent(BMT_status.bat_vol);
    if (Enable_BATDRV_LOG == 1) {
        MT_BAT_BQ24158DB("===> %d , %d (%d)\r\n", BMT_status.SOC, BMT_status.bat_vol, BATTERY_AVERAGE_SIZE);
    }

    if (bat_volt_cp_flag == 0)
    {
        bat_volt_cp_flag = 1;
        bat_volt_check_point = BMT_status.SOC;
    }
    /* User smooth View when discharging : end */

    /**************** Averaging : START ****************/
    if (!batteryBufferFirst)
    {
        batteryBufferFirst = KAL_TRUE;

        for (i=0; i<BATTERY_AVERAGE_SIZE; i++) {
            batteryVoltageBuffer[i] = BMT_status.bat_vol;
            batteryCurrentBuffer[i] = BMT_status.ICharging;
            batterySOCBuffer[i] = BMT_status.SOC;
        }

        batteryVoltageSum = BMT_status.bat_vol * BATTERY_AVERAGE_SIZE;
        batteryCurrentSum = BMT_status.ICharging * BATTERY_AVERAGE_SIZE;
        batterySOCSum = BMT_status.SOC * BATTERY_AVERAGE_SIZE;
    }

    batteryVoltageSum -= batteryVoltageBuffer[batteryIndex];
    batteryVoltageSum += BMT_status.bat_vol;
    batteryVoltageBuffer[batteryIndex] = BMT_status.bat_vol;

    batteryCurrentSum -= batteryCurrentBuffer[batteryIndex];
    batteryCurrentSum += BMT_status.ICharging;
    batteryCurrentBuffer[batteryIndex] = BMT_status.ICharging;

    if (BMT_status.bat_full)
        BMT_status.SOC = 100;
    if (g_bat_full_user_view)
        BMT_status.SOC = 100;

    batterySOCSum -= batterySOCBuffer[batteryIndex];
    batterySOCSum += BMT_status.SOC;
    batterySOCBuffer[batteryIndex] = BMT_status.SOC;

    BMT_status.bat_vol = batteryVoltageSum / BATTERY_AVERAGE_SIZE;
    BMT_status.ICharging = batteryCurrentSum / BATTERY_AVERAGE_SIZE;
    BMT_status.SOC = batterySOCSum / BATTERY_AVERAGE_SIZE;

    batteryIndex++;
    if (batteryIndex >= BATTERY_AVERAGE_SIZE)
        batteryIndex = 0;
    /**************** Averaging : END ****************/

	//<<MaoyiChou,2013/1/31, Waitting HW notifi battery full.
    if( BMT_status.SOC == 100 && BMT_status.bat_charging_state == CHR_BATFULL) {
        BMT_status.bat_full = KAL_TRUE;
    }
	else{
		BMT_status.SOC--;
        BMT_status.bat_full = KAL_FALSE;
	}
	//>>MaoyiChou,2013/1/31, Waitting HW notifi battery full.


    /**************** For UBOOT : Start ****************/
    if (low_bat_boot_display == 0)
    {

        /* SOC only UP when charging */
        if ( BMT_status.SOC > bat_volt_check_point ) {
            bat_volt_check_point = BMT_status.SOC;
        }

//<2012/10/24-15567-jessicatseng, [Hawk 4.0] Modify tricolor LED for MOTO's definition
#if defined(ARIMA_PROJECT_HAWK40)
        /* UBOOT charging LED */
        if ( (bat_volt_check_point >= 100)  || (user_view_flag == KAL_TRUE) ) {
            leds_battery_full_charging();
        } else if(bat_volt_check_point <= 15) {
            leds_battery_low_charging();
        } else {
            leds_battery_medium_charging();
        }
#else
        /* UBOOT charging LED */
        if ( (bat_volt_check_point >= 100/*90*/)  || (user_view_flag == KAL_TRUE) ) {
            leds_battery_full_charging();
        } else if(bat_volt_check_point <= 15/*10*/) {
            leds_battery_low_charging();
        } else {
            leds_battery_medium_charging();
        }
#endif
//>2012/10/24-15567-jessicatseng

#if 0
        /* UBOOT charging animation */
        if ( (BMT_status.bat_full) || (user_view_flag == KAL_TRUE) )
        {
            if(g_bl_on == 1)
            {
                mt_disp_show_battery_full();    
            }
            user_view_flag = KAL_TRUE;
        }
        else
        {
            if ( (bat_volt_check_point>=0) && (bat_volt_check_point<25) )
            {
                prog_temp = 0;
            }
            else if ( (bat_volt_check_point>=25) && (bat_volt_check_point<50) )
            {
                prog_temp = 25;
            }
            else if ( (bat_volt_check_point>=50) && (bat_volt_check_point<75) )
            {
                prog_temp = 50;
            }
            else if ( (bat_volt_check_point>=75) && (bat_volt_check_point<100) )
            {
                prog_temp = 75;
            }
            else
            {
                prog_temp = 100;
            }

            if (prog_first == 1)
            {
                prog = prog_temp;
                prog_first = 0;
            }
            if(g_bl_on == 1)
            {
                mt_disp_show_battery_capacity(prog);
            }
            prog += 25;
            if (prog > 100) prog = prog_temp;
        }
#endif

#if 0
        /* UBOOT charging idle mode */
        if (!bl_switch) {
            mt6575_sleep(500, KAL_FALSE);
            mt_disp_power(TRUE);
            bl_switch_timer++;
            mt65xx_backlight_on();
            g_bl_on = 1;
        }
        if (mtk_detect_key(BACKLIGHT_KEY)) { 
            bl_switch = KAL_FALSE;
            bl_switch_timer = 0;
            g_bl_on = 1;
            MT_BAT_BQ24158DB("[BATTERY] mt65xx_backlight_on\r\n");
        }
        if (bl_switch_timer > BL_SWITCH_TIMEOUT) {
            bl_switch = KAL_TRUE;
            bl_switch_timer = 0;
            mt65xx_backlight_off();
            mt_disp_power(FALSE);
            g_bl_on = 0;

            // fill the screen with a whole black image
            mt65xx_disp_fill_rect(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT, 0x0);
            mt65xx_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);

            MT_BAT_BQ24158DB("[BATTERY] mt65xx_backlight_off\r\n");
        }
#endif

    }

    /**************** For UBOOT : End ****************/

    //if (Enable_BATDRV_LOG == 1) {
    MT_BAT_BQ24158DB("[BATTERY:AVG(%d,%dmA)] BatTemp:%d Vbat:%d VBatSen:%d SOC:%d ChrDet:%d Vchrin:%d Icharging:%d(%d) ChrType:%d \r\n",
    BATTERY_AVERAGE_SIZE, CHARGING_FULL_CURRENT, BMT_status.temperature ,BMT_status.bat_vol, BMT_status.ADC_BAT_SENSE, BMT_status.SOC,
    upmu_is_chr_det(), BMT_status.charger_vol, BMT_status.ICharging, g_BatteryAverageCurrent, CHR_Type_num );
    //}

    //if (Enable_BATDRV_LOG == 1) {
    //    MT_BAT_BQ24158DB("[BATTERY] CON_9:%x, CON10:%x\n\r", INREG16(CHR_CON_9), INREG16(CHR_CON_10));
    //}

	//for(iii=0x21 ; iii<0x3F ; iii++)
	//{
		//MT_BAT_BQ24158DB("Bank0[0x%x]=0x%x, ", iii, upmu_get_reg_value(iii));
	//}

    /* Protection Check : start*/
    //BAT_status = BAT_CheckPMUStatusReg();
    //if(BAT_status != PMU_STATUS_OK)
    //    return PMU_STATUS_FAIL;

	#if (BAT_TEMP_PROTECT_ENABLE == 1)
    if ((BMT_status.temperature <= MIN_CHARGE_TEMPERATURE) ||
        (BMT_status.temperature == ERR_CHARGE_TEMPERATURE))
    {
        MT_BAT_BQ24158DB(  "[BATTERY] Battery Under Temperature or NTC fail !!\n\r");
		BMT_status.bat_charging_state = CHR_ERROR;
        return PMU_STATUS_FAIL;
    }
	#endif

	if (BMT_status.temperature >= MAX_CHARGE_TEMPERATURE)
    {
        MT_BAT_BQ24158DB(  "[BATTERY] Battery Over Temperature !!\n\r");
		BMT_status.bat_charging_state = CHR_ERROR;
//<2012/10/19-15370-jessicatseng, Show "partial charging" in lk mode		
		BAT_BatteryOverTemperatureAction();
//>2012/10/19-15370-jessicatseng
        return PMU_STATUS_FAIL;
    }

    if (upmu_is_chr_det() == KAL_TRUE)
    {
		#if (V_CHARGER_ENABLE == 1)
        if (BMT_status.charger_vol <= V_CHARGER_MIN )
        {
            MT_BAT_BQ24158DB(  "[BATTERY]Charger under voltage!!\r\n");
            BMT_status.bat_charging_state = CHR_ERROR;
            return PMU_STATUS_FAIL;
        }
		#endif
        if ( BMT_status.charger_vol >= V_CHARGER_MAX )
        {
            MT_BAT_BQ24158DB(  "[BATTERY]Charger over voltage !!\r\n");
            BMT_status.charger_protect_status = charger_OVER_VOL;
            BMT_status.bat_charging_state = CHR_ERROR;
            return PMU_STATUS_FAIL;
        }
    }
    
//<2012/10/19-15385-jessicatseng, Fix "partial charging" issue    
//<2012/10/09-14860-jessicatseng, [Hawk 4.0] Integrate SOP-0296 partial charging
	#if defined(MOTO_SOP_0296_PARTIAL_CHARGE)	
	if((BMT_status.temperature >= PARTIAL_CHARGE_MIN_TEMPERATURE) &&
		 (BMT_status.temperature < PARTIAL_CHARGE_MAX_TEMPERATURE))
	{
		MT_BAT_BQ24158DB("[BATTERY] Enter partial charging BMT_status.temperature = %d\n", BMT_status.temperature);
		if(BMT_status.bat_charging_state != CHR_PARTIAL)
		{
			BMT_status.bat_partial_full = KAL_FALSE;
		}
			
		BMT_status.bat_charging_state = CHR_PARTIAL;
		//return PMU_STATUS_OK;
	}
	#endif
//>2012/10/09-14860-jessicatseng
//>2012/10/19-15385-jessicatseng    
    /* Protection Check : end*/

    if (upmu_is_chr_det() == KAL_TRUE)
    {
//<2012/10/12-15016-jessicatseng, [Hawk 4.0]Fix battery's voltage is more than 4.11V, SOC will show 100%
     	//if ((BMT_status.bat_vol < RECHARGING_VOLTAGE) && (BMT_status.bat_full) && (g_HW_Charging_Done == 1))
  		if((BMT_status.bat_full) && (g_HW_Charging_Done == 1))
      	{
        	bq24158_status = bq24158_get_chip_status();
	
	    	if(bq24158_status == 0x1)
			{
           	 //if (Enable_BATDRV_LOG == 1) {
            	MT_BAT_BQ24158DB("[BATTERY] Check Battery Re-charging & turn on charging!!\n\r");
            	//}
            	
            	BMT_status.bat_full = KAL_FALSE;
            	g_bat_full_user_view = KAL_TRUE;
            	BMT_status.bat_charging_state = CHR_CC;
            	g_HW_Charging_Done = 0;
				pchr_turn_on_charging_bq24158();
           }
			//<<MaoyiChou,2013/2/1, Fixed reboot 100% when charging full.
			if (BMT_status.SOC <= 99 || BMT_status.bat_vol <= 4340)
			{
            	//if (Enable_BATDRV_LOG >= 1) {
            		printk("[BATTERY] Capacity <= 99% Battery Re-charging !!\n");                
            	//}
							
				pchr_turn_off_charging_bq24158();
				bq24158_config_interface_reg(0x04,0x81/*0x64*/); //reset charger ic
				pchr_turn_on_charging_bq24158();
            	BMT_status.bat_full = KAL_FALSE;    
            	g_bat_full_user_view = KAL_TRUE;  //mtk71259  20120630
				//BMT_status.bat_charging_state = CHR_PRE;

				g_HW_Charging_Done = 0;
			}
			//>>MaoyiChou,2013/2/1, Fixed reboot 100% when charging full.
        }
//>2012/10/12-15016-jessicatseng
    }

    return PMU_STATUS_OK;
}

PMU_STATUS BAT_BatteryStatusFailAction(void)
{
    if (Enable_BATDRV_LOG == 1) {
        MT_BAT_BQ24158DB(  "[BATTERY] BAD Battery status... Charging Stop !!\n\r");
    }

    BMT_status.total_charging_time = 0;
    BMT_status.PRE_charging_time = 0;
    BMT_status.CC_charging_time = 0;
    BMT_status.TOPOFF_charging_time = 0;
    BMT_status.POSTFULL_charging_time = 0;

    /*  Disable charger */
    pchr_turn_off_charging_bq24158();

    //g_sw_cv_enable = 0;

    return PMU_STATUS_OK;
}

PMU_STATUS BAT_ChargingOTAction(void)
{
    MT_BAT_BQ24158DB(  "[BATTERY] Charging over time !!\n\r");

    BMT_status.bat_full = KAL_TRUE;
    BMT_status.total_charging_time = 0;
    BMT_status.PRE_charging_time = 0;
    BMT_status.CC_charging_time = 0;
    BMT_status.TOPOFF_charging_time = 0;
    BMT_status.POSTFULL_charging_time = 0;

    g_HW_Charging_Done = 1;
    g_Charging_Over_Time = 1;

    /*  Disable charger*/
    pchr_turn_off_charging_bq24158();

    //g_sw_cv_enable = 0;

    return PMU_STATUS_OK;
}

PMU_STATUS BAT_BatteryFullAction(void)
{
    //if (Enable_BATDRV_LOG == 1) {
    MT_BAT_BQ24158DB(  "[BATTERY] Battery full !!\n\r");
    //}

    BMT_status.bat_full = KAL_TRUE;
    BMT_status.total_charging_time = 0;
    BMT_status.PRE_charging_time = 0;
    BMT_status.CC_charging_time = 0;
    BMT_status.TOPOFF_charging_time = 0;
    BMT_status.POSTFULL_charging_time = 0;

    g_HW_Charging_Done = 1;

    /*  Disable charger */
    //pchr_turn_off_charging_bq24158();

    //g_sw_cv_enable = 0;

    return PMU_STATUS_OK;
}

//<2012/10/09-14860-jessicatseng, [Hawk 4.0] Integrate SOP-0296 partial charging
#if defined(MOTO_SOP_0296_PARTIAL_CHARGE)
PMU_STATUS BAT_BatteryPartialModeAction(void)
{
	if (Enable_BATDRV_LOG == 1) {
		printk("[BATTERY] Partial mode charge, timer=%ld temperature = %d!!\n", BMT_status.total_charging_time, BMT_status.temperature);    
	}

	BMT_status.total_charging_time += BAT_TASK_PERIOD;  
	
	if(BMT_status.temperature < PARTIAL_CHARGE_MIN_TEMPERATURE)
	{
		if(Enable_BATDRV_LOG == 1) {
			printk("[BATTERY] Partial mode charge switch to CC mode temperature = %d\n", BMT_status.temperature);
		}
		
		BMT_status.bat_charging_state = CHR_CC;
		/*  Enable charger */
		pchr_turn_on_charging_bq24158();
	}
	else
	{
		if(Enable_BATDRV_LOG == 1) {
			printk("[BATTERY] Partial mode charge BMT_status.bat_vol = %d, bat_partial_full = %d\n", BMT_status.bat_vol, BMT_status.bat_partial_full);	
		}
				
		if((BMT_status.bat_vol < PARTIAL_MAX_VOLTAGE)&&
			 (BMT_status.bat_partial_full == KAL_FALSE))
		{
			/*  Enable charger */
			pchr_turn_on_charging_bq24158();
		}
		else
		{
			if(BMT_status.bat_vol >= PARTIAL_MAX_VOLTAGE)
			{
				BMT_status.bat_partial_full = KAL_TRUE;
			}
			else if(BMT_status.bat_vol <= PARTIAL_RECHARGING_VOLTAGE)
			{
				BMT_status.bat_partial_full = KAL_FALSE;
			}		
				
			/*  Disable charger */
			pchr_turn_off_charging_bq24158();
		}
	}
	
	return PMU_STATUS_OK;        
} 
#endif
//>2012/10/09-14860-jessicatseng

extern BOOT_ARGUMENT *g_boot_arg;

void check_battery_exist(void)
{
    kal_uint32 baton_count = 0;
    int mode = 0;
    
//<2012/11/06-16325-jessicatseng, [Hawk 4.0] Enable battery detection function
//<2012/10/02-14540-jessicatseng, [Hawk 4.0] Don't do battery detection function for JB temporarily
//#if defined(ARIMA_PROJECT_HAWK40)
		//return;
//#endif
//>2012/10/02-14540-jessicatseng
//>2012/11/06-16325-jessicatseng

    baton_count += upmu_chr_get_baton_undet();
    baton_count += upmu_chr_get_baton_undet();
    baton_count += upmu_chr_get_baton_undet();

    mode = g_boot_arg->boot_mode &= 0x000000FF;
    if (g_boot_arg->maggic_number == BOOT_ARGUMENT_MAGIC)
    {
        if( baton_count >= 3)
        {
            if( (mode == META_BOOT) || (mode == ATE_FACTORY_BOOT) )
            {
                 MT_BAT_BQ24158DB("[BATTERY] boot mode = %d, bypass battery check\n", mode);
            }
            else
            {
                MT_BAT_BQ24158DB("[BATTERY] Battery is not exist, power off bq24158 and system (%d)\n", baton_count);
                pchr_turn_off_charging_bq24158();
                #ifndef NO_POWER_OFF
                mt6575_power_off();
                #endif        
            }
        }
    }
	else
	{
        if( baton_count >= 3)
		{
			MT_BAT_BQ24158DB("[BATTERY] Battery is not exist, power off BQ24158 and system (%d) 2\n", baton_count);
			pchr_turn_off_charging_bq24158();
        #ifndef NO_POWER_OFF
        mt6575_power_off();
        #endif        
		}
    }
}

void bmt_charger_ov_check(void)
{
    if(upmu_chr_get_vcdt_hv_det() == 1)
    {
        pchr_turn_off_charging_bq24158();
        MT_BAT_BQ24158DB("[bmt_charger_ov_check]LK charger ov, turn off charging\r\n");
        while(1)             
        {  
            MT_BAT_BQ24158DB("[bmt_charger_ov_check] mtk_wdt_restart()\n");
            mtk_wdt_restart();
             
            if(charger_ov_boot_display == 0)
            {
                mt_disp_power(TRUE);
                //mt65xx_disp_show_charger_ov_logo(); 
                mt_disp_wait_idle();
                charger_ov_boot_display = 1;
                MT_BAT_BQ24158DB("LK charger ov, Set low brightness\r\n");
                mt65xx_leds_brightness_set(6, 20);
            }
            BMT_status.charger_vol = PMIC_IMM_GetOneChannelValue(AUXADC_CHARGER_VOLTAGE_CHANNEL, 5);
            BMT_status.charger_vol = BMT_status.charger_vol / 100;
            if (BMT_status.charger_vol < 4000) //charger out detection        
            {             
                #ifndef NO_POWER_OFF                
                mt6575_power_off();              
                #endif             
                while(1);            
            }
            
            #ifdef GPT_TIMER                  
                mt6575_sleep(500, KAL_FALSE);          
            #else              
                tmo = get_timer(0);              
                while(get_timer(tmo) <= 500 /* ms */);            
            #endif        
        }
    }    
}

int get_pmic_flag=0;

void BAT_thread_bq24158(void)
{
    int BAT_status = 0;
	kal_uint32 tmp32;
    kal_uint32 bq24158_status=0;

	MT_BAT_BQ24158DB("[BATTERY] mtk_wdt_restart()\n");
    mtk_wdt_restart();

    if (Enable_BATDRV_LOG == 1) {
        MT_BAT_BQ24158DB("[BATTERY] LOG. ---------------------------------------------------------------------\n");
    }

    if ((upmu_is_chr_det() == KAL_TRUE))
    {
    check_battery_exist();
    }

    MT_BAT_BQ24158DB("[BATTERY] SET TMR_RST");
    bq24158_config_interface_reg(0x00,0x80);

    //<2012/09/16-tedwu, set charge current.
    #if defined(ARIMA_PROJECT_HAWK35)
    bq24158_config_interface_reg(0x02,0xAE);  //MaoyiChou,2013/2/1, Changed target voltage from 4.34v to 4.36v
//<2012/09/28-14469-jessicatseng, [Hawk 4.0] Modify charging current for JB
    #elif defined(ARIMA_PROJECT_HAWK40)
    bq24158_config_interface_reg(0x02,0xAE);  //MaoyiChou,2013/2/1, Changed target voltage from 4.34v to 4.36v
//>2012/09/28-14469-jessicatseng    
    #else
    bq24158_config_interface_reg(0x02,0x8e);  //mtk71259 20120711
    #endif
    //>2012/09/16-tedwu

	if(get_pmic_flag == 0)
	{
		/* get pmic version */
		/* Low part of CID */
	    tmp32=upmu_get_cid0();
	    g_eco_version |= tmp32;
	    /* High part of CID */
	    tmp32=upmu_get_cid1();
	    g_eco_version |= (tmp32 << 8);
		if (g_eco_version == PMIC6329_E1_CID_CODE)
	    {
	        MT_BAT_BQ24158DB("[Charger_E1] Get PMIC version = E1\n");
			upmu_chr_vcdt_lv_vth(0); // VCDT_LV=4.2V
			MT_BAT_BQ24158DB("[Charger_E1] Set VCDT_LV=4.2V\n");
		}
		else
		{
			MT_BAT_BQ24158DB("[Battery] Get PMIC version > E1\n");
		}

		get_pmic_flag = 1;
	}

	/* If charger does not exist */
   // if ( upmu_is_chr_det() == KAL_FALSE )
    if(upmu_chr_get_vcdt_hv_det() == 1 || (upmu_is_chr_det() == KAL_FALSE))
    {
        bmt_charger_ov_check();
        BMT_status.charger_type = CHARGER_UNKNOWN;
        BMT_status.bat_full = KAL_FALSE;
        g_bat_full_user_view = KAL_FALSE;
        g_usb_state = USB_UNCONFIGURED;

        g_HW_Charging_Done = 0;
        g_Charging_Over_Time = 0;

        MT_BAT_BQ24158DB("[BATTERY] No Charger, Power OFF !?\n");

        pchr_turn_off_charging_bq24158();

        MT_BAT_BQ24158DB("[BATTERY] mt_power_off !!\n");
        #ifndef NO_POWER_OFF
        mt6575_power_off();
        #endif
        while(1);
    }

    /* Check Battery Status */
    BAT_status = BAT_CheckBatteryStatus_bq24158();
    if( BAT_status == PMU_STATUS_FAIL )
        g_Battery_Fail = KAL_TRUE;
    else
        g_Battery_Fail = KAL_FALSE;

    /* No Charger */
    if(BAT_status == PMU_STATUS_FAIL || g_Battery_Fail)
    {
        BAT_BatteryStatusFailAction();
    }

    /* Battery Full */
    //else if (BMT_status.bat_full)
    /* HW charging done, real stop charging */
    else if (g_HW_Charging_Done == 1)
    {
        if (Enable_BATDRV_LOG == 1) {
            MT_BAT_BQ24158DB("[BATTERY] Battery real full. \n");
        }
        BAT_BatteryFullAction();
    }

    /* Charging Overtime, can not charging */
    else if (g_Charging_Over_Time == 1)
    {
        if (Enable_BATDRV_LOG == 1) {
            MT_BAT_BQ24158DB("[BATTERY] Charging Over Time. \n");
        }
        pchr_turn_off_charging_bq24158();
    }

    /* Battery Not Full and Charger exist : Do Charging */
    else
    {
        if(BMT_status.total_charging_time >= MAX_CHARGING_TIME)
        {
            BMT_status.bat_charging_state = CHR_BATFULL;
            BAT_ChargingOTAction();
            return;
        }

        if ( BMT_status.TOPOFF_charging_time >= MAX_CV_CHARGING_TIME )
        {
            if (Enable_BATDRV_LOG == 1) {
                MT_BAT_BQ24158DB("BMT_status.TOPOFF_charging_time >= %d \r\n", MAX_CV_CHARGING_TIME);
            }
            BMT_status.bat_charging_state = CHR_BATFULL;
            BAT_BatteryFullAction();
            return;
        }

//<2012/10/09-14860-jessicatseng, [Hawk 4.0] Integrate SOP-0296 partial charging
#if defined(MOTO_SOP_0296_PARTIAL_CHARGE)
				if(BMT_status.bat_charging_state == CHR_PARTIAL)
				{
					BAT_BatteryPartialModeAction();
    			return;
				}		
#endif   
//>2012/10/09-14860-jessicatseng

        //MT_BAT_BQ24158DB("[BATTERY] DUMP-start \n");
        //bq24158_dump_register(); maoyi_remove_log
        //MT_BAT_BQ24158DB("[BATTERY] DUMP-end \n");

        bq24158_status = bq24158_get_chip_status();
        /* check battery full */
    	if( bq24158_status == 0x2 )
    	{
    		BMT_status.bat_charging_state = CHR_BATFULL;
    		BMT_status.bat_full = KAL_TRUE;
    		BMT_status.total_charging_time = 0;
    		BMT_status.PRE_charging_time = 0;
    		BMT_status.CC_charging_time = 0;
    		BMT_status.TOPOFF_charging_time = 0;
    		BMT_status.POSTFULL_charging_time = 0;
    		g_HW_Charging_Done = 1;
    		//pchr_turn_off_charging_bq24158();
    		printk("[BATTERY:bq24158] Battery real full and disable charging (%ld) \n", bq24158_status);
    		return;
    	}

        /* Charging flow begin */
    	BMT_status.total_charging_time += BAT_TASK_PERIOD;
    	pchr_turn_on_charging_bq24158();
    	if (Enable_BATDRV_LOG == 1) {
        		printk(  "[BATTERY:bq24158] Total charging timer=%ld, bq24158_status=%ld \n\r",
    				BMT_status.total_charging_time, bq24158_status);
        	}

        }

    g_SW_CHR_OUT_EN = 1;

}

void lk_charging_display()
{
    #define BATTERY_BAR 25

    if ( (BMT_status.bat_full) || (user_view_flag == KAL_TRUE) )
    {
        if(g_bl_on == KAL_TRUE)
        {
//<2012/06/20-tedwu, add MOTO charging animation on powerOff charging.
#if defined(ARIMA_CHARGE_ANIMATION_FOR_MOTO)
            mt_disp_show_battery_full();    
#endif
//>2012/06/20-tedwu
        }
        user_view_flag = KAL_TRUE;
    }
    else
    {
        prog_temp = (bat_volt_check_point/BATTERY_BAR) * BATTERY_BAR;

        if (prog_first == 1)
        {
            prog = prog_temp;
            prog_first = 0;
        }
        if(g_bl_on == 1)
        {
            //<2012/06/20-tedwu, add MOTO charging animation on powerOff charging.
        #if defined(ARIMA_CHARGE_ANIMATION_FOR_MOTO)
            if (bl_switch_timer == -TOTAL_EXT_LOOPS)
            {
                //g_bl_on = KAL_TRUE;  //make sure first time to turn on BL for MOTO logo
                //bat_volt_check_point= BMT_status.bat_vol;  //to avoid to show 0% logo first when pluging in charger.
				//<<MaoyiChou,2013/1/11, Fixed can't show next animation for Hawk40 P2.
                //mt_disp_show_moto_logo();
				//>>MaoyiChou,2013/1/11, Fixed can't show next animation for Hawk40 P2.
            }
            else if (bl_switch_timer >= -EXTENSION_LOOPS) {
              #if !defined(CXD_ANIMATION_NEW)
//<2012/10/19-15370-jessicatseng, Show "partial charging" in lk mode            	
#if defined(MOTO_SOP_0296_PARTIAL_CHARGE)            	
            	if(BMT_status.bat_charging_state ==	CHR_PARTIAL)
            		mt_disp_show_high_temp();
            	else	
#endif            		
//>2012/10/19-15370-jessicatseng
              #endif
                mt_disp_show_battery_capacity(bat_volt_check_point); // Charging Animation
			}
        #elif defined(ANIMATION_NEW)
            mt_disp_show_battery_capacity(bat_volt_check_point);
        #else
            mt_disp_show_battery_capacity(prog);
        #endif
//>2012/06/20-tedwu
        }
        prog += BATTERY_BAR;
        if (prog > 100) prog = prog_temp;
    }

    /* UBOOT charging idle mode */
    if (!bl_switch) {
        //<2012/09/16-tedwu, temporarily put code here to reduce the power consumption.
        #if defined(ARIMA_PROJECT_HAWK35)
        //restore_back_power();
        #endif
        //>2012/09/16-tedwu
        mt_disp_power(TRUE);
        bl_switch_timer++;
        mt65xx_backlight_on();
        g_bl_on = 1;
    }

    if (bl_switch_timer > BL_SWITCH_TIMEOUT) {
        bl_switch = KAL_TRUE;
        bl_switch_timer = 0;
        mt65xx_backlight_off();
        mt_disp_power(FALSE);
        g_bl_on = 0;
        MT_BAT_BQ24158DB("[BATTERY] mt65xx_backlight_off\r\n");
        //<2012/09/16-tedwu, temporarily put code here to reduce the power consumption.
        #if defined(ARIMA_PROJECT_HAWK35)
        //reduce_power_consumption_2();
        #endif
        //>2012/09/16-tedwu
        //<2012/12/06-tedwu, change for new CXD charging animation.
      #if defined(CXD_ANIMATION_NEW)
        capacity_last = BAR_TOP;
      #endif
        //>2012/12/06-tedwu
    }
}

void batdrv_init(void)
{
    int i = 0;
    //int ret = 0;

    /* Initialization BMT Struct */
    for (i=0; i<BATTERY_AVERAGE_SIZE; i++) {
        batteryCurrentBuffer[i] = 0;
        batteryVoltageBuffer[i] = 0;
        batterySOCBuffer[i] = 0;
    }
    batteryVoltageSum = 0;
    batteryCurrentSum = 0;
    batterySOCSum = 0;

    BMT_status.bat_exist = 1;       /* phone must have battery */
    BMT_status.charger_exist = 0;   /* for default, no charger */
    BMT_status.bat_vol = 0;
    BMT_status.ICharging = 0;
    BMT_status.temperature = 0;
    BMT_status.charger_vol = 0;
    //BMT_status.total_charging_time = 0;
    BMT_status.total_charging_time = 1;
    BMT_status.PRE_charging_time = 0;
    BMT_status.CC_charging_time = 0;
    BMT_status.TOPOFF_charging_time = 0;
    BMT_status.POSTFULL_charging_time = 0;

    BMT_status.bat_charging_state = CHR_PRE;

    /* init i2c0 interface*/
    //ret = i2c0_v1_init();
    //MT_BAT_BQ24158DB("Init I2C0: %s(%d)\n", ret ? "FAIL" : "OK", ret);

//<2012/10/24-15651-jessicatseng, [Hawk 4.0] Write register 0x06 of bq24158 in lk mode
//<2012/10/22-tedwu, temp workaround for i2c init failure on power-on mode.
#if !(defined(ARIMA_PROJECT_HAWK35) || defined(ARIMA_PROJECT_HAWK40))
    if ((upmu_is_chr_det() == KAL_TRUE))
#endif
//>2012/10/22-tedwu
//>2012/10/24-15651-jessicatseng
    {
        MT_BAT_BQ24158DB("bq24158 hw init\n");
        bq24158_hw_init();
        //bq24158_dump_register(); maoyi_remove_log
    }

//<2012/08/09-kylechang, Compiler Error(GPIO_SWCHARGER_EN_PIN undefined)
//    #if 0
//    #if defined(CHARGER_IC_GPIO_RESETTING)
//    gpio_number   = cust_gpio_number;
//    gpio_off_mode = cust_gpio_off_mode;
//    gpio_off_dir  = cust_gpio_off_dir;
//    gpio_off_out  = cust_gpio_off_out;
//    gpio_on_mode  = cust_gpio_on_mode;
//    gpio_on_dir   = cust_gpio_on_dir;
//    gpio_on_out   = cust_gpio_on_out;
//    #endif
//  MT_BAT_BQ24158DB("[LK][Charger IC] GPIO setting : %d,%d,%d,%d,%d,%d,%d\n",
//    gpio_number, gpio_off_mode, gpio_off_dir, gpio_off_out,
//    gpio_on_mode, gpio_on_dir, gpio_on_out);
//
//#else
//	 gpio_on_out = mt_get_gpio_out(GPIO_SWCHARGER_EN_PIN);
//	 MT_BAT_BQ24158DB("[UBOOT][Charger IC] GPIO setting : %d,%d .\n", GPIO_SWCHARGER_EN_PIN, mt_get_gpio_out(GPIO_SWCHARGER_EN_PIN));
//#endif
//>2012/08/09-kylechang

    upmu_chr_vcdt_hv_vth(0xA);		//VCDT_HV_VTH, 6.5V

    MT_BAT_BQ24158DB("[BATTERY] batdrv_init : Done\n");
}

//<2012/7/9-tedwu, check MOTO battery ID
//<2012/04/16-5464-brucexiao, display warning message when detect invalid battery
void  invalid_battery_action(void)
{
    ulong tmo;

    //mt_disp_enable_not_fblayer();  //<2012/09/18-tedwu,//>
    //mt_disp_show_invalid_battery();

    #ifdef GPT_TIMER
    if (g_bl_on == KAL_TRUE)
        mt6575_sleep(5000, KAL_FALSE);
    else
        mt6575_sleep(5000, KAL_TRUE);
    #else
        tmo = get_timer(0);			
        while(get_timer(tmo) <= 5000 /* 5000ms=5s */);   
    #endif           
    BMT_status.charger_type = CHARGER_UNKNOWN;
    BMT_status.bat_full = KAL_FALSE;
    g_bat_full_user_view = KAL_FALSE;
    g_usb_state = USB_UNCONFIGURED;

    g_HW_Charging_Done = 0;
    g_Charging_Over_Time = 0;

    CSDAC_DAT_MAX = 255;

    MT_BAT_BQ24158DB("[BATTERY] invalid Charger, Power OFF !?\n");
    pchr_turn_off_charging_bq24158();

    while(1)
    {
        // if charger is off
         if (upmu_is_chr_det() == KAL_FALSE)
        {
                      mt65xx_backlight_off();
                          mt6575_power_off();
        }
    
        mtk_wdt_restart();

        tmo = get_timer(0);
        while(get_timer(tmo) <= 500);
    }
}
//>2012/04/16-5464-brucexiao
//>2012/7/9-tedwu

//<2012/10/19-15370-jessicatseng, Show "partial charging" in lk mode		
void BAT_BatteryOverTemperatureAction(void)
{
    ulong tmo;
		
	//mt_disp_show_over_temp();

#ifdef GPT_TIMER
	if (g_bl_on == KAL_TRUE)
	    mt6575_sleep(5000, KAL_FALSE);
	else
	    mt6575_sleep(5000, KAL_TRUE);
#else
	tmo = get_timer(0);			
	while(get_timer(tmo) <= 5000 /* 5000ms=5s */);   
#endif           
	BMT_status.charger_type = CHARGER_UNKNOWN;
	BMT_status.bat_full = KAL_FALSE;
	g_bat_full_user_view = KAL_FALSE;
	g_usb_state = USB_UNCONFIGURED;

	g_HW_Charging_Done = 0;
	g_Charging_Over_Time = 0;

	CSDAC_DAT_MAX = 255;

	MT_BAT_BQ24158DB("[BATTERY] Battery's temperature is too high !?\n");
	pchr_turn_off_charging_bq24158();                

	while(1)
	{		
		// if charger is off
		if (upmu_is_chr_det() == KAL_FALSE)
		{
			mt65xx_backlight_off();
			mt6575_power_off();
		}
		
        mtk_wdt_restart();

        tmo = get_timer(0);
        while(get_timer(tmo) <= 500);
	}  
}
//>2012/10/19-15370-jessicatseng

extern bool g_boot_menu;

void mt65xx_bat_init(void)
{
    #ifndef GPT_TIMER
    long tmo;
    long tmo2;
    #endif
    //UINT32 i;    
    //BOOL checked = FALSE;
    BOOL print_msg = FALSE;
    //int ret = 0;    
    int press_pwrkey_count=0, loop_count = 0;
    BOOL pwrkey_ready = false;
    BOOL back_to_charging_animation_flag = false;

    #if (CHARGING_PICTURE == 1)
    mt_disp_enter_charging_state();
    #else
    mt_disp_show_boot_logo();
    #endif

    if ((upmu_is_chr_det() == KAL_TRUE))
    {
        check_battery_exist();
    }

    sc_mod_init();
    batdrv_init();

    BMT_status.bat_full = FALSE;
	BAT_GetVoltage_notbat_bq24158();
	BAT_GetVoltage_notbat_bq24158();
//<2012/10/16 ShermanWei, Close TouchPad LED while charging & battery is higher than 3.2V by customer's Requirement
#if defined(ARIMA_PROJECT_HAWK35) || defined(HAWK35_EP0)
	MT_BAT_BQ24158DB("++VBAT++=%d ; %d\n", BMT_status.bat_vol, BATTERY_LOWVOL_THRESOLD);
	if (BMT_status.bat_vol > 3200) {
	    mt_set_gpio_mode(GPIO196, GPIO_MODE_GPIO);	
	    mt_set_gpio_dir(GPIO196, GPIO_DIR_OUT);
	    mt_set_gpio_out(GPIO196, GPIO_OUT_ONE);
	}
#endif
//>2012/10/16 ShermanWei,


//<2012/10/12-15016-jessicatseng, [Hawk 4.0]Fix battery's voltage is more than 4.11V, SOC will show 100%
#if 0
    if ( BMT_status.bat_vol > RECHARGING_VOLTAGE ) {
        user_view_flag = KAL_TRUE;
    } else {
        user_view_flag = KAL_FALSE;
    }
#endif
//>2012/10/12-15016-jessicatseng

    if (mt6329_detect_powerkey())      
         pwrkey_ready = true;
     else
         pwrkey_ready = false;

//<2012/7/9-tedwu, check MOTO battery ID
#if defined(CHK_BATTERY_ID__MOTO_ID)
  //<2012/12/26-tedwu, for power consumption and avoid ESD issue on 1-wire pin?.
  #if !defined(ARIMA_PROJECT_HAWK35)
    bat_id_init_pin();
  #endif
  //>2012/12/26-tedwu
    Read_Battery_Info();
//<2012/11/22-17513-jessicatseng, [Hawk 4.0] Add ZCV table for LG cell
#if defined(BATTERY_PACK_EG30)
    BatteryTableContruct();
#endif
//>2012/11/22-17513-jessicatseng
#endif
//>2012/7/9-tedwu

//<2012/11/12-16768-kevincheng,factory cable detection
#if defined(MOTO_CABLE_DETECTION)
    g_factory_cable_id = g_boot_arg->factory_cable_id &= 0x000000FF;
    MT_BAT_BQ24158DB(">>>> [lk] battery bq24158 init read parameter from preloader : g_factory_cable_id : %d\n",g_factory_cable_id);

    if(mt_get_gpio_in(GPIO_FACTORY_CABLE_DET_PIN) == 1)  //lk double check GPIO status
       g_factory_cable_id |=0x00000001;
    MT_BAT_BQ24158DB(">>>> [lk] battery bq24158 read fixed value of g_factory_cable_id is : %d , and will pass to kernel with ATAC\n",\
            g_factory_cable_id);
#endif
//>2012/11/12-16768-kevincheng
    /* Boot with Charger */
    if ((upmu_is_chr_det() == KAL_TRUE))
    {
        //<2012/09/12-tedwu, temporarily put code here to reduce the power consumption.
        #if defined(ARIMA_PROJECT_HAWK35)
        //reduce_power_consumption_1();	//EricHsieh,2012/10/17, power off charging power consumption tuning.
        //reduce_power_consumption_2();
        //change_supplied_voltage_level();
        #endif //defined(ARIMA_PROJECT_HAWK35)
        //>2012/09/12-tedwu

		CHR_Type_num = mt_charger_type_detection();
        BMT_status.charger_type = CHR_Type_num;
		//BMT_status.charger_type = NONSTANDARD_CHARGER;

        while (1)
        {
            upmu_chr_chrwdt_td(0x0);				// CHRWDT_TD, 4s, check me
            upmu_chr_chrwdt_int_en(1);				// CHRWDT_INT_EN, check me
            upmu_chr_chrwdt_en(1); 					// CHRWDT_EN, check me
            upmu_chr_chrwdt_flag_wr(1);				// CHRWDT_FLAG, check me
            upmu_chr_vcdt_hv_enable(1);		//VCDT_HV_EN
            
            //<2012/7/9-tedwu, check MOTO battery ID
            //<2012/04/16-5464-brucexiao, display warning message when detect invalid battery
            #if defined(CHK_BATTERY_ID__MOTO_ID)
            if(Is_MOTO_Battery() == KAL_FALSE)
            {
            	invalid_battery_action();
                #ifdef GPT_TIMER
                if (g_bl_on == KAL_TRUE)
                    mt6575_sleep(1000, KAL_FALSE);
                else
                    mt6575_sleep(1000, KAL_TRUE);                        
                #else
                tmo2 = get_timer(0);            
                while(get_timer(tmo2) <= 1000 /* ms */);                    
                #endif	
            }
            #endif
            //>2012/04/16-5464-brucexiao
            //>2012/7/9-tedwu

            //add charger ov detection
            bmt_charger_ov_check();

            if (rtc_boot_check(true) || meta_mode_check() || (pwrkey_ready == true)
                || mtk_wdt_boot_check()==WDT_BY_PASS_PWK_REBOOT || g_boot_arg->boot_reason==BR_TOOL_BY_PASS_PWK || g_boot_menu==true )            
            {
                // Low Battery Safety Booting
                //pchr_turn_off_charging_bq24158();
                BMT_status.bat_vol = PMIC_IMM_GetOneChannelValue(AUXADC_BATTERY_VOLTAGE_CHANNEL,1);
                MT_BAT_BQ24158DB("check VBAT=%d mV with %d mV\n", BMT_status.bat_vol, BATTERY_LOWVOL_THRESOLD);
                pchr_turn_on_charging_bq24158();

#if 1
                while ( BMT_status.bat_vol < BATTERY_LOWVOL_THRESOLD )
                {
                    if (low_bat_boot_display == 0)
                    {
                        mt_disp_power(TRUE);
                        mt65xx_backlight_off();
                        MT_BAT_BQ24158DB("Before mt6516_disp_show_low_battery\r\n");
                        mt_disp_show_low_battery();
                        MT_BAT_BQ24158DB("After mt6516_disp_show_low_battery\r\n");
                        mt_disp_wait_idle();
                        MT_BAT_BQ24158DB("After mt6516_disp_wait_idle\r\n");

                        low_bat_boot_display = 1;

                        MT_BAT_BQ24158DB("Set low brightness\r\n");
                        mt65xx_leds_brightness_set(6, 20);
                    }

                    rtc_boot_check(false);
                    BAT_thread_bq24158();
                    MT_BAT_BQ24158DB("-");


					//upmu_chr_baton_tdet_en(0);	//sw workaround: 26M off

                    #ifdef GPT_TIMER
                        if (g_bl_on == KAL_TRUE)
                            mt6575_sleep(1000, KAL_FALSE);
                        else
                            mt6575_sleep(1000, KAL_TRUE);
                    #else
                        tmo2 = get_timer(0);
                        while(get_timer(tmo2) <= 1000 /* ms */);
                    #endif

				    // upmu_chr_baton_tdet_en(1);  //sw workaround: 26M off

                    if((pwrkey_ready ==true) & mt6329_detect_powerkey()==0 )
        		    {
        		        back_to_charging_animation_flag = TRUE;
        		        break;
        		    }
                    else
                    {
                        back_to_charging_animation_flag = false;
                    }

                    //pchr_turn_off_charging_bq24158();
                    BMT_status.bat_vol = PMIC_IMM_GetOneChannelValue(AUXADC_BATTERY_VOLTAGE_CHANNEL,1);
                    MT_BAT_BQ24158DB("VBAT=%d < %d\n", BMT_status.bat_vol, BATTERY_LOWVOL_THRESOLD);
                    pchr_turn_on_charging_bq24158();
                }

                if(back_to_charging_animation_flag == false)
		        {
                    mt_disp_power(KAL_TRUE);

                    if (g_boot_mode != ALARM_BOOT)
                    {
                        //<2012/09/17-tedwu, won't let user glimpse at the previous battery charging screen.
                        #if defined(ARIMA_CHARGE_ANIMATION_FOR_MOTO)
                        mt_disp_disable_not_fblayer();
                        #endif
                        //>2012/09/17-tedwu
                        mt_disp_show_boot_logo();
                        // update twice here to ensure the LCM is ready to show the
                        // boot logo before turn on backlight, OR user may glimpse
                        // at the previous battery charging screen
                        mt_disp_show_boot_logo();
                        mt_disp_wait_idle();
                    }
                    else
                    {
                        MT_BAT_BQ24158DB("[BATTERY] Power off alarm trigger! Boot Linux Kernel!!\n\r");

                        // fill the screen with a whole black image
                        mt_disp_fill_rect(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT, 0x0);
                        mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
                        mt_disp_wait_idle();
                    }

                    MT_BAT_BQ24158DB("Restore brightness\r\n");
                    mt65xx_leds_brightness_set(6, 255);
                    mt65xx_backlight_on();
#endif
                    //pchr_turn_off_charging_bq24158();

                    //restore
                    //<2012/08/09-kylechang, Compiler Error(GPIO_SWCHARGER_EN_PIN undefined)
                    //MT_BAT_BQ24158DB("Restore GPIO_SWCHARGER_EN_PIN=%d \n", gpio_on_out);
                    //mt_set_gpio_out(GPIO_SWCHARGER_EN_PIN, gpio_on_out);
					//>2012/08/09-kylechang

					//<<MaoyiChou,2013/4/3, Restore GPIO 123, 126, 127 is BPI mode otherwise power off charging to boot cause 3G network fail .
					//GPIO123, 126, 127 restore
					mt_set_gpio_mode(GPIO123, GPIO_MODE_01);
					mt_set_gpio_mode(GPIO126, GPIO_MODE_01);
					mt_set_gpio_mode(GPIO127, GPIO_MODE_01);
					//>>MaoyiChou,2013/4/3, Restore GPIO 123, 126, 127 is BPI mode otherwise power off charging to boot cause 3G network fail .

                    sc_mod_exit();
                    return;
                }

                back_to_charging_animation_flag = false;
	            low_bat_boot_display = 0;

            }
            else
            {
                //MT_BAT_BQ24158DB("[BATTERY] U-BOOT Charging !! \n\r");
            }

            if(print_msg==FALSE)
            {
		        #ifdef ANIMATION_NEW
                BAT_CheckBatteryStatus_bq24158();
                #endif
                lk_charging_display();
                MT_BAT_BQ24158DB("[BATTERY] Charging !! Press Power Key to Booting !!! \n\r");
                print_msg = TRUE;
            }

            if (g_thread_count >= 5)  //change for charger ov
            {
                g_thread_count = 1;
                BAT_thread_bq24158();
                MT_BAT_BQ24158DB(".");
            }
            else
            {
                g_thread_count++;
            }

		    // upmu_chr_baton_tdet_en(0);  //sw workaround: 26M off

		    //<2012/06/20-tedwu, add MOTO logo on powerOff charging
            #ifdef GPT_TIMER
                if (g_bl_on == KAL_TRUE)
                    mt6575_sleep(LOOP_TIME, KAL_FALSE);
                else
                    mt6575_sleep(LOOP_TIME_SLEEP, KAL_TRUE);	//EricHsieh,2012/10/17, power off charging power consumption tuning.
            #else
                tmo = get_timer(0);
                while(get_timer(tmo) <= LOOP_TIME /* ms */);
            #endif

		    // upmu_chr_baton_tdet_en(1);  //sw workaround: 26M off

             if (loop_count++ == (LOOP_COUNT*30)) loop_count = 0;
			//>2012/06/20-tedwu

            if (mtk_detect_key(BACKLIGHT_KEY) || (!mt6329_detect_powerkey() && press_pwrkey_count > 0))
             {
                bl_switch = false;
                bl_switch_timer = 0;
                g_bl_on = true;
                MT_BAT_BQ24158DB("[BATTERY] mt65xx_backlight_on\r\n");
             }

            if (mt6329_detect_powerkey())
             {
		        press_pwrkey_count++;
	            MT_BAT_BQ24158DB("[BATTERY] press_pwrkey_count = %d, POWER_ON_TIME = %d\n", press_pwrkey_count, POWER_ON_TIME);
	         }
             else
             {
        		press_pwrkey_count = 0;
             }

            if (press_pwrkey_count > POWER_ON_TIME)
                pwrkey_ready = true;
            else
                pwrkey_ready = false;

            /* if (mt6577_detect_key(BACKLIGHT_KEY) || press_pwrkey_count > 0)
            {
                bl_switch = false;
                bl_switch_timer = 0;
                g_bl_on = true;
                MT_BAT_BQ24158DB("[BATTERY] mt65xx_backlight_on\r\n");
            }*/
         
//<2012/06/20-tedwu, add MOTO logo on powerOff charging
#if defined(ARIMA_CHARGE_ANIMATION_FOR_MOTO)
            if (((loop_count % LOOP_COUNT) == 0) && bl_switch == KAL_FALSE) // update charging screen 
            {
                lk_charging_display();
            }   
#else
            if (((loop_count % 5) == 0) && bl_switch == false) // update charging screen
            {
                if (Enable_BATDRV_LOG == 1)
                {
                    MT_BAT_BQ24158DB("[BATTERY] loop_count = %d\n", loop_count);
                }
                lk_charging_display();
            }
#endif
        }
    }
    else
    {
        bmt_charger_ov_check();

        if ( (rtc_boot_check(false)||mtk_wdt_boot_check()==WDT_BY_PASS_PWK_REBOOT) && BMT_status.bat_vol >= BATTERY_LOWVOL_THRESOLD)
        //if (BMT_status.bat_vol >= BATTERY_LOWVOL_THRESOLD)
        {
            MT_BAT_BQ24158DB("[BATTERY] battery voltage(%dmV) >= CLV ! Boot Linux Kernel !! \n\r",BMT_status.bat_vol);
            sc_mod_exit();
            return;
        }
        else
        {
            MT_BAT_BQ24158DB("[BATTERY] battery voltage(%dmV) <= CLV ! Can not Boot Linux Kernel !! \n\r",BMT_status.bat_vol);
            pchr_turn_off_charging_bq24158();
            #ifndef NO_POWER_OFF
            mt6575_power_off();
            #endif
            while(1)
            {
            	MT_BAT_BQ24158DB("If you see the log, please check with RTC power off API\n\r");
            }
        }
    }

    sc_mod_exit();
    return;
}

#else

#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <printf.h>

void mt65xx_bat_init(void)
{
    MT_BAT_BQ24158DB("[BATTERY] Skip mt65xx_bat_init !!\n\r");
    MT_BAT_BQ24158DB("[BATTERY] If you want to enable power off charging, \n\r");
    MT_BAT_BQ24158DB("[BATTERY] Please #define CFG_POWER_CHARGING!!\n\r");
}

//kal_bool pmic_chrdet_status(void)
//{
//   return KAL_FALSE;
//}

#endif
