/**
 **************************************************************************************
 * @author  Sharath N
 * @file    STM32_Keypad4x4.h
 * @brief   Header file for interfacing 4*4 Matrix Keypad with STM32F411 Discovery Kit
 **************************************************************************************
**/ 


#ifndef STM32_KEYPADDRIVER_H
#define STM32_KEYPADDRIVER_H

#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"

/*------------------------Define your GPIO pins here - START----------------------------*/

/** 
  Keypad	STM32	       	 Description         I/O
     L1			PB3				Row 1           Output
     L2			PB4				Row 2           Output
     L3			PB5				Row 3           Output
     L4			PB6				Row 4           Output
		 
     R1			PB12			Column 1        Input
     R2			PB13	 		Column 2        Input
     R3			PB14			Column 3        Input
     R4			PB15			Column 4        Input
**/
/* Rows  ~ OUTPUT */
/* Row 1 default */
#define KEYPAD_ROW_1_PORT			  GPIOB
#define KEYPAD_ROW_1_PIN			  GPIO_PIN_3
/* Row 2 default */
#define KEYPAD_ROW_2_PORT			  GPIOB
#define KEYPAD_ROW_2_PIN			  GPIO_PIN_4
/* Row 3 default */
#define KEYPAD_ROW_3_PORT			  GPIOB
#define KEYPAD_ROW_3_PIN			  GPIO_PIN_5
/* Row 4 default */
#define KEYPAD_ROW_4_PORT			  GPIOB
#define KEYPAD_ROW_4_PIN			  GPIO_PIN_6

/* Columns ~ INPUT */
/* Column 1 default */
#define KEYPAD_COLUMN_1_PORT		GPIOB
#define KEYPAD_COLUMN_1_PIN			GPIO_PIN_12
/* Column 2 default */
#define KEYPAD_COLUMN_2_PORT		GPIOB
#define KEYPAD_COLUMN_2_PIN			GPIO_PIN_13
/* Column 3 default */
#define KEYPAD_COLUMN_3_PORT		GPIOB
#define KEYPAD_COLUMN_3_PIN			GPIO_PIN_14
/* Column 4 default */
#define KEYPAD_COLUMN_4_PORT		GPIOB
#define KEYPAD_COLUMN_4_PIN			GPIO_PIN_15

/*------------------------Define your GPIO pins here - END----------------------------*/
/* Keypad NOT pressed */
#define KEYPAD_NOT_PRESSED			'\0'


/**
 * @brief  Reads keypad data
 * @param  None
 * @retval Button status. This parameter will be a value of KEYPAD_Button_t enumeration
 */
char KEYPAD_Read(void);

#endif


