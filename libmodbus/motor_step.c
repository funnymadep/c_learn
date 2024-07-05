#include "motorcontrol.h"
#include "main_host.h"
#include "modbus.h"


#define NEGATIVE_LIMIT 1
#define POSITIVE_LIMIT 2
#define ALL_LIMIT 3
#define NO_LIMIT 0

int limitL = NO_LIMIT;
int returnBack = 1;
			
unsigned int RecentlySteps = 0;		
unsigned int PCTargetSteps = 0;		


uint16_t step_num[2]; //限位 第一位是高脉冲位 第二位是低脉冲位

void int_to_hex_step_num(int input) {
    // 将输入的有符号整数转换为32位无符号整数
    uint32_t hex_value = (uint32_t)input;

    // 取高16位和低16位分别存储到数组中
    step_num[0] = (hex_value >> 16) & 0xFFFF;  // 高16位
    step_num[1] = hex_value & 0xFFFF;          // 低16位
}

void step_one_time(modbus_t *ctx, int step)
{
	int_to_hex_step_num(step);
	uint16_t step_ctl[] = {0x0, 0x1};
	modbus_write_registers(ctx, 0x24, 1, &step_num[0]);
	modbus_write_registers(ctx, 0x25, 1, &step_num[1]);
	modbus_write_registers(ctx, 0x27, 1, &step_ctl[1]);	
}

//2024/7/1 hql 读取限位和电机运动状态
void *readLimitAndStatus(void *arg)
{
	modbus_t *ctx = (modbus_t *)arg;
	uint16_t limit_reg; // 限位
	uint16_t state_reg; // 运动状态
	int rc;

	while(1)
	{
		usleep(50000);

		if (step_flag == 1)
			continue;

		rc = modbus_read_registers(ctx, 0x04, 1, &state_reg);
		if (rc == -1) {
			fprintf(stderr, "读取状态数据失败: %s\n", modbus_strerror(errno));
			Log("<INFO>: read motor state error");
			return;	
		} 

		if (state_reg == 0)
		{
			motor_state = MOTOR_STATIC;
		}
		else 
		{
			motor_state = MOTOR_ACTIVE;
		}

		rc = modbus_read_registers(ctx, 0x08, 1, &limit_reg);
		if (rc == -1) {
			Log("<INFO>: motor read limit error");
			fprintf(stderr, "读取限位数据失败: %s\n", modbus_strerror(errno));
			return;	
		} 
		usleep(5000);

		switch (limit_reg)
		{
			case 8:
				limitL = NEGATIVE_LIMIT;
				break;
			case 16:
				limitL = POSITIVE_LIMIT;
				break;
			case 24:
				limitL = ALL_LIMIT;
				break;
			default:
				limitL = NO_LIMIT;
				break;
		}

		if (limitL == ALL_LIMIT)
		{
			// continue;
		}
		else if (limitL == POSITIVE_LIMIT)
		{
			if (returnBack == 1)
			{
				returnBack = 0;
				Log("<INFO>: motor return back to zero over\n");
			}
			step_one_time(ctx, 100);
			Log("<INFO>: up to positive limit\n");
		}
		else if (limitL == NEGATIVE_LIMIT)
		{
			step_one_time(ctx, -100);
			Log("<INFO>: up to negative limit\n");
		}
		
		if(returnBack == 1)
		{
			step_one_time(ctx, -150);
		}

		// printf("limitL = %d, limit_reg = %d,  state = %d\n", limitL, limit_reg, motor_state);
		
	}
}

//2024/7/1 hql 初始化modbus
modbus_t *motor_init()
{
	modbus_t *ctx = modbus_new_rtu("/dev/ttyAMA1", 115200, 'N', 8, 1);
    if (ctx == NULL) {
		Log("<INFO>: modbus read dev error\n");
        fprintf(stderr, "无法创建MODBUS上下文\n");
        return -1;
    }

	uint16_t tab_rx; // 接收数据缓冲区
	int rc;


    // 设置调试模式
    modbus_set_debug(ctx, false);

    // 设置超时时间（毫秒）
    modbus_set_response_timeout(ctx, 100, 0);

    // 连接到串口设备
    if (modbus_connect(ctx) == -1) {
		Log("<INFO>: modbus connect dev error\n");
        fprintf(stderr, "无法连接到MODBUS设备\n");
        modbus_free(ctx);
        return;
    }else{
		printf("open serial sucessful\n");
	}

	modbus_set_slave(ctx, 1);

    // 初始化数据
	uint16_t step_init[4] = {15, 0, 0, 15};
	uint16_t step_min[] = {0};

	modbus_write_registers(ctx, 0x20, 1, &step_init[0]);
	modbus_write_registers(ctx, 0x21, 1, &step_init[1]);
	modbus_write_registers(ctx, 0x22, 1, &step_init[2]);
	modbus_write_registers(ctx, 0x23, 1, &step_init[3]);
	modbus_write_registers(ctx, 0x11, 1, step_min);
	Log("<INFO>: modbus init\n");
	return ctx;
}

//2024/7/1 hql 根据调试软件控制步进电机
int main(){
	modbus_t *ctx = motor_init();
	int step = 0, sub_step = 0 ,temp_step = 0, last_step = 0;

	g_nMotorStart = 1;

	pthread_t limitReadThread;
	int ret = pthread_create(&limitReadThread, NULL, readLimitAndStatus, (void *)ctx);
	if (ret < 0)
	{
		fprintf(stderr, "Error: Unable to create thread\n");
        return;
	}

	while (1)
	{

		// printf("ConnectWithPC = %d, step = %d\n", ConnectWithPC, step_flag);
		if (0 == step_flag || ConnectWithPC == 0 || motor_state == MOTOR_ACTIVE || limitL == ALL_LIMIT){
			usleep(40000);
			continue;
		}

		//2024/7/1 hql 云台停止 返回上一次步进电机位置
		if (cloud_flag == 1){
			temp_step = RecentlySteps;
			RecentlySteps = step;
			step = temp_step;
			cloud_flag = 0;
		}
		//2024/7/1 hql 正常运行
		else
		{
			RecentlySteps = step;
			step = PCTargetSteps;
		}
		
		current_step = PCTargetSteps;
		// printf("PCTargetSteps = %d\n", PCTargetSteps);
		if (step == RecentlySteps)
		{
			continue;
		}

		//2024/7/2 hql 步进电机驱动方式由脉冲方向加正高低脉冲变为正负高低脉冲
		sub_step = step - RecentlySteps;
		step_one_time(ctx, sub_step);
		step_flag = 0;
	}

	pthread_join(readLimitAndStatus, NULL);
    // 断开连接并释放上下文
    modbus_close(ctx);
    modbus_free(ctx);

}