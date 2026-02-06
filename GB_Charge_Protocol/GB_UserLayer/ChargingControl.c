/*
 * ChargingControl.c
 *
 *  Created on: 2025-07-24
 *  Author: 83923
 */

#include "ChargingControl.h"


/* 全局变量定义 */
Charging_ControlTypeDef charging_control;
Charging_ParametersTypeDef charging_params;



/**
 * @brief 充电控制模块初始化函数
 */
void ChargingControl_Init(void) {
    // 初始化控制结构体
    memset(&charging_control, 0, sizeof(Charging_ControlTypeDef));
    charging_control.current_stage = CHARGING_STAGE_HANDSHAKE;
    charging_control.charging_allowed = false;
    
    // 初始化参数结构体
    memset(&charging_params, 0, sizeof(Charging_ParametersTypeDef));
    
    // 初始化硬件层定时器
    Hardware_Init();
    
    // 初始化报文发送间隔计数器
    // 已清理未使用的定时器变量
}

/**
 * @brief 获取当前充电阶段
 * @return Charging_StageTypeDef 当前充电阶段
 */
Charging_StageTypeDef ChargingControl_Get_Charging_Stage(void) {
    return charging_control.current_stage;
}

/**
 * @brief 设置当前充电阶段
 * @param stage 充电阶段
 */
void ChargingControl_Set_Charging_Stage(Charging_StageTypeDef stage) {
    charging_control.current_stage = stage;
}

/**
 * @brief 握手阶段处理函数
 */
void ChargingControl_Handshake_Stage(void) {
    Transport_StatusTypeDef status;
    
	uint32_t chm_start_time = Hardware_Get_Timestamp();
	
    // 先接收CHM报文直至接收到或者超时退出
    do {
        status = ApplicationLayer_Read_CHM(&charging_params.chm_data);
        if (Hardware_Get_Timestamp() - chm_start_time > 5000) { // 5秒超时
            ErrorHandler_HandleError(ERROR_TYPE_COMMUNICATION, ERROR_CODE_COMM_CHM_TIMEOUT);
            return;
        }
    } while (status != TRANSPORT_STATUS_OK);
     
	printf("接受到了CHM报文\n");
	
    // BHM报文发送：阻塞式重发BHM直到收到CRM或超时，每次重发间隔250ms
    uint32_t bhm_start_time = Hardware_Get_Timestamp();
    uint32_t last_send_time = 0;
    
    do {
        uint32_t current_time = Hardware_Get_Timestamp();
        
        // 首次发送或达到250ms间隔时重发
        if (current_time - last_send_time >= 250 || last_send_time == 0) {
            ApplicationLayer_Send_BHM(&charging_params.bhm_data);
            last_send_time = current_time;
        }
        
        // 检查是否收到CRM报文并判断SPN2560是否为0x00
        status = ApplicationLayer_Read_CRM(&charging_params.crm_data);
        if (status == TRANSPORT_STATUS_OK && charging_params.crm_data.Recognitio_Result == 0x00) {
            break; // 收到CRM报文，退出循环
        }
        
        // 检查5秒超时
        if (Hardware_Get_Timestamp() - bhm_start_time > 5000) {
            ErrorHandler_HandleError(ERROR_TYPE_COMMUNICATION, ERROR_CODE_COMM_CRM_TIMEOUT);
            return;
        }
        
    } while (1);
    
    // BRM报文发送：阻塞式重发BRM直到收到CRM或超时，每次重发间隔250ms
    uint32_t brm_start_time = Hardware_Get_Timestamp();
    uint32_t brm_last_send_time = 0;
    
    do {
        uint32_t current_time = Hardware_Get_Timestamp();
        
        // 首次发送或达到250ms间隔时重发
        if (current_time - brm_last_send_time >= 250 || brm_last_send_time == 0) {
            status = ApplicationLayer_Send_BRM(&charging_params.brm_data);
            if (status != TRANSPORT_STATUS_OK) {
                return;
            }
            brm_last_send_time = current_time;
        }
        
        // 检查是否收到CRM报文并判断SPN2560是否为0xAA
        status = ApplicationLayer_Read_CRM(&charging_params.crm_data);
        if (status == TRANSPORT_STATUS_OK && charging_params.crm_data.Recognitio_Result == 0xAA) {
            break; // 收到CRM报文，退出循环
        }
        
        // 检查5秒超时
        if (Hardware_Get_Timestamp() - brm_start_time > 5000) {
            ErrorHandler_HandleError(ERROR_TYPE_COMMUNICATION, ERROR_CODE_COMM_CRM_TIMEOUT);
            return;
        }
        
    } while (1);
    
    // 如果所有握手报文都成功发送和接收，进入参数配置阶段
    ChargingControl_Set_Charging_Stage(CHARGING_STAGE_PARAMETER);
}

/**
 * @brief 参数配置阶段处理函数
 */
void ChargingControl_Parameter_Stage(void) {
    Transport_StatusTypeDef status;

    // BCP报文发送：阻塞式重发BCP直到收到CML或超时，每次重发间隔500ms
    uint32_t bcp_start_time = Hardware_Get_Timestamp();
    uint32_t bcp_last_send_time = 0;
    
    do {
        uint32_t current_time = Hardware_Get_Timestamp();
        
        // 首次发送或达到500ms间隔时重发
        if (current_time - bcp_last_send_time >= 500 || bcp_last_send_time == 0) {
            status = ApplicationLayer_Send_BCP(&charging_params.bcp_data);
            if (status != TRANSPORT_STATUS_OK) {
                return;
            }
            bcp_last_send_time = current_time;
        }
        
        //CTS报文接收：接收就行不用阻塞(此条报文无伤大雅)
        status = ApplicationLayer_Read_CTS(&charging_params.cts_data);

        // 检查是否收到CML报文
        status = ApplicationLayer_Read_CML(&charging_params.cml_data);
        if (status == TRANSPORT_STATUS_OK) {
            break; // 收到CML报文，退出循环
        }
        
        // 检查5秒超时
        if (Hardware_Get_Timestamp() - bcp_start_time > 5000) {
            ErrorHandler_HandleError(ERROR_TYPE_COMMUNICATION, ERROR_CODE_COMM_CML_TIMEOUT);
            return;
        }
        
    } while (1);

    // BRO报文发送：阻塞式重发BRO直到收到符合条件的CRO或超时，每次重发间隔250ms
    uint32_t bro_start_time = Hardware_Get_Timestamp();
    uint32_t bro_last_send_time = 0;
    
    do {
        uint32_t current_time = Hardware_Get_Timestamp();
        
        // 首次发送或达到250ms间隔时重发
        if (current_time - bro_last_send_time >= 250 || bro_last_send_time == 0) {
            status = ApplicationLayer_Send_BRO(&charging_params.bro_data);
            if (status != TRANSPORT_STATUS_OK) {
                return;
            }
            bro_last_send_time = current_time;
        }
        
        // 检查是否收到符合条件的CRO报文
        status = ApplicationLayer_Read_CRO(&charging_params.cro_data);
        if (status == TRANSPORT_STATUS_OK && charging_params.cro_data.charging_prepare_status == 0xAA) {
            break; // 收到符合条件的CRO报文，退出循环
        }
        
        // 检查5秒超时
        if (Hardware_Get_Timestamp() - bro_start_time > 5000) {
            ErrorHandler_HandleError(ERROR_TYPE_COMMUNICATION, ERROR_CODE_COMM_CRO_TIMEOUT);
            return;
        }
        
    } while (1);

    // 如果所有参数配置报文都成功发送和接收，进入充电阶段
    ChargingControl_Set_Charging_Stage(CHARGING_STAGE_CHARGING);
}

/**
 * @brief 充电阶段处理函数
 */
void ChargingControl_Charging_Stage(void) {
    Transport_StatusTypeDef status;
    
    // 充电阶段：持续循环收发BCL/BCS/CCS/BSM报文
    uint32_t bcl_last_send_time = 0;
    uint32_t bcs_last_send_time = 0;
    uint32_t bsm_last_send_time = 0;
    uint32_t ccs_timeout_start = 0;  // CCS超时计时起点
    bool waiting_for_ccs = false;    // 是否正在等待CCS响应
    bool sending_bsm_periodically = false;  // 是否正在周期性发送BSM报文
    
    while (1) 
		{
        uint32_t current_time = Hardware_Get_Timestamp();
 
        // 每50ms发送BCL报文
        if (current_time - bcl_last_send_time >= 50 || bcl_last_send_time == 0) {
            // 发送BCL报文
            status = ApplicationLayer_Send_BCL(&charging_params.bcl_data);
            if (status != TRANSPORT_STATUS_OK) {
                return;
            }
            
            // 如果是第一次发送BCL或者收到CCS后第一次发送BCL，启动CCS超时计时
            if (!waiting_for_ccs) {
                ccs_timeout_start = current_time;
                waiting_for_ccs = true;
            }
            
            bcl_last_send_time = current_time;
        }
        
        // 非阻塞式检查CCS响应
        if (waiting_for_ccs) {
            status = ApplicationLayer_Read_CCS(&charging_params.ccs_data);
            if (status == TRANSPORT_STATUS_OK) {
                // 收到CCS报文，更新BCS中的电压和电流测量值
                ChargingControl_Simulate_BCS_From_CCS();
                
                sending_bsm_periodically = true;  // 开始周期性发送BSM报文
                // 重置CCS等待状态，下次发送BCL时重新开始计时
                waiting_for_ccs = false;
            }
            
            // 检查1秒超时
            if (current_time - ccs_timeout_start > 1000) {
                ErrorHandler_HandleError(ERROR_TYPE_COMMUNICATION, ERROR_CODE_COMM_CCS_TIMEOUT);
                return;
            }
        }
        
        // 每250ms发送BCS报文
        if (current_time - bcs_last_send_time >= 250 || bcs_last_send_time == 0) {
            // 发送BCS报文
            status = ApplicationLayer_Send_BCS(&charging_params.bcs_data);
            if (status != TRANSPORT_STATUS_OK) {
                return;
            }
            bcs_last_send_time = current_time;
        }
        
        // 如果收到CCS报文，则每250ms发送BSM报文
        if (sending_bsm_periodically) {
            if (current_time - bsm_last_send_time >= 250 || bsm_last_send_time == 0) {
                // 发送BSM报文
                status = ApplicationLayer_Send_BSM(&charging_params.bsm_data);
                if (status != TRANSPORT_STATUS_OK) {
                    return;
                }
                bsm_last_send_time = current_time;
            }
        }
        
        // 检查是否收到CST报文（非阻塞检查）
        status = ApplicationLayer_Read_CST(&charging_params.cst_data);
        if (status == TRANSPORT_STATUS_OK) {
            // 收到CST报文，进入结束阶段
            ChargingControl_Set_Charging_Stage(CHARGING_STAGE_END);
            return;
        }
        
        // 检查是否需要车辆主动中止充电
        if (charging_params.bsm_data.batVolState == 0x01 ||  // 电池电压过高
            charging_params.bsm_data.batTempOverState == 0x01 ||  // 电池温度过高
            charging_params.bsm_data.batInsulationState == 0x01) { // 电池绝缘故障
            
            // BST报文发送：阻塞式重发BST直到发送成功或超时
            uint32_t bst_start_time = Hardware_Get_Timestamp();
            do {
                status = ApplicationLayer_Send_BST(&charging_params.bst_data);
                if (status == TRANSPORT_STATUS_OK) {
                    break; // 发送成功，退出循环
                }
                
                // 检查5秒超时
                if (Hardware_Get_Timestamp() - bst_start_time > 5000) {
                    return;
                }
            } while (1);
            
            // CST报文接收：阻塞式等待直到收到CST或超时
            uint32_t cst_start_time = Hardware_Get_Timestamp();
            do {
                status = ApplicationLayer_Read_CST(&charging_params.cst_data);
                if (status == TRANSPORT_STATUS_OK) {
                    break; // 收到CST报文，退出循环
                }
                
                // 检查5秒超时
                if (Hardware_Get_Timestamp() - cst_start_time > 5000) {
                    ErrorHandler_HandleError(ERROR_TYPE_COMMUNICATION,ERROR_CODE_COMM_CST_TIMEOUT);
                    return;
                }
            } while (1);
            
            // 中止充电，进入结束阶段
            ChargingControl_Set_Charging_Stage(CHARGING_STAGE_END);
            return;
        }
    }
}

/**
 * @brief 结束阶段处理函数
 */
void ChargingControl_End_Stage(void) {
    Transport_StatusTypeDef status;
    bool end_condition_met = false;
    
    // 结束阶段：阻塞式重发BSD报文，每250ms间隔，最多10次
    uint32_t bsd_last_send_time = 0;
    uint8_t bsd_send_count = 0;
    //uint8_t csd_receive_count = 0;
    
    while (bsd_send_count < 10) {
        uint32_t current_time = Hardware_Get_Timestamp();

        // 首次发送或达到250ms间隔时重发BSD
        if (current_time - bsd_last_send_time >= 250 || bsd_last_send_time == 0) {
            status = ApplicationLayer_Send_BSD(&charging_params.bsd_data);
            if (status != TRANSPORT_STATUS_OK) {
                return;
            } 
            bsd_send_count++;
            bsd_last_send_time = current_time;
        }
        
        // 检查是否收到CSD报文
        status = ApplicationLayer_Read_CSD(&charging_params.csd_data);
        if (status == TRANSPORT_STATUS_OK) {
            end_condition_met = true; // 收到3次CSD，满足结束条件
            break; // 退出循环
        }
    }

    // 如果未满足结束条件（例如，达到最大发送次数但未收到足够CSD），则报告超时
    if (!end_condition_met) {
        ErrorHandler_HandleError(ERROR_TYPE_COMMUNICATION, ERROR_CODE_COMM_CSD_TIMEOUT);
        return;
    }

    // 重置到握手阶段，准备下一次充电
    ChargingControl_Set_Charging_Stage(CHARGING_STAGE_HANDSHAKE);
}

/**
 * @brief 模拟设置BCS中的充电电压测量值和电流测量值等于CCS中的对应值
 * 此函数用于模拟测试，将BCS报文中的充电电压和电流测量值设置为CCS报文中的对应值
 */
void ChargingControl_Simulate_BCS_From_CCS(void) {
    // 将BCS中的充电电压测量值设置为CCS中的输出电压值
    charging_params.bcs_data.chrgVol = charging_params.ccs_data.output_voltage;
    
    // 将BCS中的充电电流测量值设置为CCS中的输出电流值
    charging_params.bcs_data.chrgCur = charging_params.ccs_data.output_current;
    
    // 打印模拟设置的信息
    printf("CCS电压为: %f V\n", (float)charging_params.ccs_data.output_voltage/10.0);
    printf("CCS电流为: %f A\n", (float)charging_params.ccs_data.output_current/10.0 -400.0);
}

/**
 * @brief 根据电池状态动态更新充电参数
 * 此函数根据当前电池状态（SOC、温度等）动态调整充电电压、电流和模式
 */
void ChargingControl_Update_Charging_Parameters(void) {
    // 根据电池SOC调整充电参数
    uint8_t soc = charging_params.bcs_data.batSoc;
    
    // 根据SOC调整充电电流
    if (soc < 30) {
        // SOC较低时，使用较大电流
        charging_params.bcl_data.charging_current_demand = 50; // 5A
        charging_params.bcl_data.GBT_Charging_Mode = GBT_Charging_Mode_Constant_Current;
    } else if (soc < 80) {
        // SOC中等时，适当降低电流
        charging_params.bcl_data.charging_current_demand = 30; // 3A
        charging_params.bcl_data.GBT_Charging_Mode = GBT_Charging_Mode_Constant_Current;
    } else {
        // SOC较高时，切换到恒压模式
        charging_params.bcl_data.GBT_Charging_Mode = GBT_Charging_Mode_Constant_Voltage;
    }
    
    // 根据电池温度调整充电参数
    uint8_t max_temp = charging_params.bsm_data.batTempMax;
    
    // 如果温度过高，降低充电电流
    if (max_temp > 40) {
        // 温度较高时，降低充电电流
        charging_params.bcl_data.charging_current_demand = 
            (charging_params.bcl_data.charging_current_demand * 80) / 100; // 降低到80%
    }
    
    // 确保充电电压不超过最大允许值
    if (charging_params.bcl_data.charging_voltage_demand > charging_params.bcp_data.batPermitChrgVolMax) {
        charging_params.bcl_data.charging_voltage_demand = charging_params.bcp_data.batPermitChrgVolMax;
    }
    
    // 确保充电电流不超过最大允许值
    if (charging_params.bcl_data.charging_current_demand > charging_params.bcp_data.batPermitChrgCurMax) {
        charging_params.bcl_data.charging_current_demand = charging_params.bcp_data.batPermitChrgCurMax;
    }
    
    // 打印当前充电参数
    printf("[BMS] 动态调整充电参数 - 电压: %d V, 电流: %d A, 模式: %d\n", 
           charging_params.bcl_data.charging_voltage_demand,
           charging_params.bcl_data.charging_current_demand,
           charging_params.bcl_data.GBT_Charging_Mode);
}


/**
 * @brief 充电控制主处理函数
 * 实现即插即用的充电控制逻辑
 */
void ChargingControl_Process(void) {
    
    // 检查当前充电阶段并调用相应的处理函数
    switch (ChargingControl_Get_Charging_Stage()) {
        case CHARGING_STAGE_HANDSHAKE:
						printf("握手阶段开始\n");
            ChargingControl_Handshake_Stage();
            break;
        
        case CHARGING_STAGE_PARAMETER:
						printf("参数配置阶段开始\n");
            ChargingControl_Parameter_Stage();
            break;
        
        case CHARGING_STAGE_CHARGING:
						printf("充电阶段开始\n");
            ChargingControl_Charging_Stage();
            break;
        
        case CHARGING_STAGE_END:
						printf("结束阶段开始\n");
            ChargingControl_End_Stage();
            break;
        
        default:
            // 如果阶段状态异常，重置到握手阶段
            //ChargingControl_Set_Charging_Stage(CHARGING_STAGE_HANDSHAKE);
            break;
    }
}
