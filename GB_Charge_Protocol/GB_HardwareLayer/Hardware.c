/*
 * Hardware.c
 *
 *  Created on: 2025-07-24
 *  Author: 83923
 *
 */
 
#include "Hardware.h"

/* 硬件初始化 */
void Hardware_Init(void) {
    /* 初始化CAN */
    Hardware_CAN_Init();
    
    /* 初始化定时器 */
    Hardware_Timer_Init();
}

/* CAN初始化 */
void Hardware_CAN_Init(void) {
    RCC_APB2PeriphClockCmd(CAN_GPIO_CLK,ENABLE);
	RCC_APB1PeriphClockCmd(CAN_CLK, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//复用推挽输出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//上拉输入
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure); 
	
	CAN_InitTypeDef CAN_InitStructure;
	CAN_InitStructure.CAN_TTCM = DISABLE;			/* 禁止时间触发模式（不生成时间戳), T  */
	CAN_InitStructure.CAN_ABOM = DISABLE;			/* 禁止自动总线关闭管理 */
	CAN_InitStructure.CAN_AWUM = DISABLE;			/* 禁止自动唤醒模式 */
	CAN_InitStructure.CAN_NART = DISABLE;			/* 禁止仲裁丢失或出错后的自动重传功能 */
	CAN_InitStructure.CAN_RFLM = DISABLE;			/* 禁止接收FIFO加锁模式 */
	CAN_InitStructure.CAN_TXFP = DISABLE;			/* 禁止传输FIFO优先级 */
	CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;	/* 设置CAN为回环模式 */
	CAN_InitStructure.CAN_SJW = CAN_SJW_2tq;
	/* 
		CAN 波特率 = RCC_APB1Periph_CAN1 / Prescaler / (SJW + BS1 + BS2);
		
		SJW = synchronisation_jump_width 
		BS = bit_segment
		
		本例中，设置CAN波特率为1Mbps		
		CAN 波特率 = 36M / 16 / (1 + 4 + 4) / = 250 Kbps		
	*/
	
	CAN_InitStructure.CAN_BS1 = CAN_BS1_4tq;
	CAN_InitStructure.CAN_BS2 = CAN_BS2_4tq;
	CAN_InitStructure.CAN_Prescaler = 16;
	CAN_Init(CANx, &CAN_InitStructure);
	//配置默认过滤器
	CAN_FilterInitTypeDef CAN_FilterInitStructure;
	CAN_FilterInitStructure.CAN_FilterNumber = 0;
	CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
	CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
	CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
	CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
	CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
	CAN_FilterInit(&CAN_FilterInitStructure);

#if CAN_RX0_INT_ENABLE
    CAN_ITConfig(CANx, CAN_IT_FMP0, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
#endif
}

/* CAN发送数据 */
CAN_StatusTypeDef Hardware_CAN_Transmit(uint32_t id, uint8_t *data, uint8_t len) {
    CanTxMsg TxMessage;
	TxMessage.StdId = id;//标准ID
	TxMessage.ExtId = id;//扩展ID
	TxMessage.IDE = CAN_Id_Extended;		//标准帧还是扩展帧
	TxMessage.RTR = CAN_RTR_Data;	//遥控帧还是数据帧
	TxMessage.DLC = len;//数据段长度
	//数据段内容
	for (uint8_t i = 0; i < len; i ++)
	{
		TxMessage.Data[i] = data[i];
	}
	//发送报文
	uint8_t TransmitMailbox = CAN_Transmit(CAN1, &TxMessage);
	//超时处理函数
	uint32_t Timeout = 0;
	while (CAN_TransmitStatus(CAN1, TransmitMailbox) != CAN_TxStatus_Ok)
	{
		Timeout ++;
		if (Timeout > 10000)
		{
			return CAN_STATUS_TIMEOUT;
		}
	}
	return CAN_STATUS_OK;
}

/* CAN接收数据 */
CAN_StatusTypeDef Hardware_CAN_Receive(uint32_t *id, uint8_t *data, uint8_t *len) {
    /* 检查接收缓冲区是否为空 */
    if (can_rx_head == can_rx_tail) {
        return CAN_STATUS_BUSY;
    }
    /* 从接收缓冲区获取数据 */
    *id = can_rx_buffer[can_rx_tail].id;
    *len = can_rx_buffer[can_rx_tail].len;
    for (uint8_t i = 0; i < *len; i++) {
        data[i] = can_rx_buffer[can_rx_tail].data[i];
    }
    /* 更新接收缓冲区索引 */
    can_rx_tail = (can_rx_tail + 1) % CAN_RX_BUFFER_SIZE;
    return CAN_STATUS_OK;
}

/* CAN接收中断服务函数 */
void Hardware_CAN_IRQHandler(void) {
    CanRxMsg RxMessage;
    
    if (CAN_GetITStatus(CAN1, CAN_IT_FMP0) != RESET) {
        CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
        
        uint16_t next_head = (can_rx_head + 1) % CAN_RX_BUFFER_SIZE;
        
        if (next_head != can_rx_tail) {
            can_rx_buffer[can_rx_head].id = (RxMessage.IDE == CAN_Id_Standard) ? 
                                             RxMessage.StdId : RxMessage.ExtId;
            can_rx_buffer[can_rx_head].len = RxMessage.DLC;
            for (uint8_t i = 0; i < RxMessage.DLC; i++) {
                can_rx_buffer[can_rx_head].data[i] = RxMessage.Data[i];
            }
            can_rx_head = next_head;
        }
        
        CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
    }
}

/* 定时器初始化 */
void Hardware_Timer_Init(void) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    /* 使能通用定时器时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
		
	/*定时器单元初始化*/
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//时钟分频因子选择为不分频，影响定时器时钟频率为72MHz
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;	//计数器模式选择为向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 1000 - 1; 				// 1ms周期 (72MHz/72000 = 1kHz)
	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1; 				// 72分频 (72MHz/72 = 1MHz) 
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;			//重复计数器，仅高级定时器有效
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);				//将结构体参数赋值给TIM_TimeBaseInit函数初始化TIM2定时器单元	

	/* 使能更新中断 */
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);	
	TIM_ITConfig(GB_Charging_TIMER, TIM_IT_Update, ENABLE);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;				//选择中断NVIC的TIM2通道
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//指定NVIC线路使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//指定NVIC线路的抢占优先级为2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//指定NVIC线路的响应优先级为1
	NVIC_Init(&NVIC_InitStructure);								//将结构体参数赋值给NVIC_Init函数初始化NVIC寄存器
	
    /* 启动定时器 */
    TIM_Cmd(GB_Charging_TIMER, ENABLE);
}

/* 获取时间戳 */
uint32_t Hardware_Get_Timestamp(void) {
    return timestamp;
}

/* 定时器中断服务函数 */
void TIM2_IRQHandler(void) {
    if (TIM_GetITStatus(GB_Charging_TIMER, TIM_IT_Update) != RESET) {
        timestamp++;
        TIM_ClearITPendingBit(GB_Charging_TIMER, TIM_IT_Update);
    }
}