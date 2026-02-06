/*
 * Hardware.h
 *
 *  Created on: 2025-07-24
 *  Author: 83923
 *
 */

#ifndef __HARDWARE_H_
#define __HARDWARE_H_

#include "stm32f10x.h"
#include "ErrorHandling.h"

/*缓冲区定义*/
#define CAN_RX_BUFFER_SIZE  32

static CAN_RxMessage_t can_rx_buffer[CAN_RX_BUFFER_SIZE];
static volatile uint16_t can_rx_head = 0;
static volatile uint16_t can_rx_tail = 0;

/* 定时器接口 */
static TIM_TypeDef* GB_Charging_TIMER = TIM2;
static uint32_t timestamp = 0;

//CAN1接收RX0中断使能
#define CAN_RX0_INT_ENABLE	1		//0,不使能;1,使能.

#define CANx 					CAN1
#define CAN_CLK					RCC_APB1Periph_CAN1

/************引脚定义***************/
#define CAN_TX_GPIO_PROT  		GPIOA
#define CAN_TX_GPIO_PIN   		GPIO_Pin_12

#define  CAN_RX_GPIO_PORT     GPIOA
#define  CAN_RX_GPIO_PIN      GPIO_Pin_11

#define CAN_GPIO_CLK					RCC_APB2Periph_GPIOA

/* CAN状态码 */
typedef enum {
    CAN_STATUS_OK = 0,
    CAN_STATUS_ERROR,
    CAN_STATUS_BUSY,
    CAN_STATUS_TIMEOUT
} CAN_StatusTypeDef;

/* 函数声明 */
void Hardware_Init(void);
void Hardware_CAN_Init(void);
CAN_StatusTypeDef Hardware_CAN_Transmit(uint32_t id, uint8_t *data, uint8_t len);
CAN_StatusTypeDef Hardware_CAN_Receive(uint32_t *id, uint8_t *data, uint8_t *len);

/* 定时器相关函数声明 */
void Hardware_Timer_Init(void);
uint32_t Hardware_Get_Timestamp(void);


#endif /* GB_CHARGING_PROTOCOL_HARDWARE_HARDWARE_H_ */
