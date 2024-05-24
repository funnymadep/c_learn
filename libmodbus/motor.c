#include "modbus.h"
#include "motor.h"

void MotorCtlForDsp(){
	// 测试
	// while(1){
	// 	Debug_Out_On = 1;
	// 	Log("stress test\n");
	// 	Debug_Out_On = 0;
	// 	sleep(1);
	// }
	modbus_t *ctx;
    uint16_t tab_rx[64]; // 接收数据缓冲区
    int rc;
	int step = 0, pre_step = 0, sub_step = 0;

	g_nMotorStart = 2;
    // 创建一个RTU上下文，设备地址为1，波特率为9600
    ctx = modbus_new_rtu("/dev/ttyAMA3", 115200, 'N', 8, 1);
    if (ctx == NULL) {
        fprintf(stderr, "无法创建MODBUS上下文\n");
        return -1;
    }

    // 设置调试模式
    modbus_set_debug(ctx, FALSE);

    // 设置超时时间（毫秒）
    modbus_set_response_timeout(ctx, 100, 0);

    // 连接到串口设备
    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "无法连接到MODBUS设备\n");
        modbus_free(ctx);
        return -1;
    }else{
		printf("open serial sucessful\n");
	}

	modbus_set_slave(ctx, 1);
    // 发送数据
	int reg_init[4] = {0x20, 0x21, 0x22, 0x23};
	uint16_t step_init[4] = {0x0A, 0x64, 0x64, 0x01F4};
	uint16_t step_high[] = {0x00, 0x01};
	uint16_t step_low[] = {0x0064, 0x00C8, 0x012c, 0x0190, 0x01f4};
	uint16_t step_ctl[] = {0x0, 0x1};
	uint16_t step_up[] = {0x0};
	uint16_t step_down[] = {0x1};

	modbus_write_registers(ctx, 0x20, 1, &step_init[1]);
	modbus_write_registers(ctx, 0x21, 1, &step_init[2]);
	modbus_write_registers(ctx, 0x22, 1, &step_init[3]);
	modbus_write_registers(ctx, 0x23, 1, &step_init[4]);
	// int 
	printf("open serial sucessful\n");	
	while (1)
	{
		pre_step = step;
		step = PCTargetSteps;
		// printf("PCTargetSteps = %d\n", PCTargetSteps);
		if (step == pre_step || (pre_step == 0 && step == 0))
		{
			continue;
		}

		if (step > pre_step){
			sub_step = step - pre_step;
			modbus_write_registers(ctx, 0x10, 1, step_up);
		}
		else
		{
			sub_step = pre_step - step;
			modbus_write_registers(ctx, 0x10, 1, step_down);
		}
		printf("sub_step = %d\n", sub_step);
		Log("sub_step\n");
		switch (sub_step)
			{
			case  100:
				modbus_write_registers(ctx, 0x24, 1, &step_high[0]);
				modbus_write_registers(ctx, 0x25, 1, &step_low[1]);
				break;
			case  200:
				modbus_write_registers(ctx, 0x24, 1, &step_high[0]);
				modbus_write_registers(ctx, 0x25, 1, &step_low[2]);
				break;
			case  300:
				modbus_write_registers(ctx, 0x24, 1, &step_high[0]);
				modbus_write_registers(ctx, 0x25, 1, &step_low[3]);
				break;
			case  400:
				modbus_write_registers(ctx, 0x24, 1, &step_high[0]);
				modbus_write_registers(ctx, 0x25, 1, &step_low[4]);
				break;
			case  500:
				modbus_write_registers(ctx, 0x24, 1, &step_high[0]);
				modbus_write_registers(ctx, 0x25, 1, &step_low[5]);
				break;
			default:
				break;
			}
			modbus_write_registers(ctx, 0x27, 1, &step_ctl[1]);	
			sleep(2);
			// modbus_write_registers(ctx, 0x28, 1, &step_ctl[1]);	
		}
		printf("PCTargetSteps = %d\n", PCTargetSteps);
		// modbus_write_registers(ctx, 0x05, 1, 0x01);

    // 接收回复数据
    rc = modbus_read_registers(ctx, 0, 1, tab_rx);
    if (rc == -1) {
        fprintf(stderr, "读取回复数据失败: %s\n", modbus_strerror(errno));
        modbus_close(ctx);
        modbus_free(ctx);
        return -1;
    }

    // 打印接收到的数据
    printf("接收到的数据: %d\n", tab_rx[0]);

    // 断开连接并释放上下文
    modbus_close(ctx);
    modbus_free(ctx);

}
