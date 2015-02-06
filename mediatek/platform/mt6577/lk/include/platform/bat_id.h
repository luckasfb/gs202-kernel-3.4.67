//<2012/08/10-tedwu, Create this file for checking battery ID.
#ifndef _BAT_ID_H_
#define _BAT_ID_H_

#include <platform/mt_typedefs.h>

/*--------------------------------------------------------------------*/

void bat_id_init_pin(void);
void bat_id_init_variable(void);
void Read_Battery_Info(void);
kal_bool Is_MOTO_Battery(void);
//<2012/11/22-17513-jessicatseng, [Hawk 4.0] Add ZCV table for LG cell
int Get_MOTO_Battery_Vender(void);
//>2012/11/22-17513-jessicatseng
#endif //ifndef _BAT_ID_H_
//>2012/08/10-tedwu
