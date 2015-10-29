#include <stdio.h>
#include "cgic.h"
#include <string.h>
#include <stdlib.h>

#define MAX_CTRL_GROUP	24			//最大时间组数
#define MAX_SERIAL_NUM	200			//一组中最大的控制序列个数
#define MAX_POWER_NUM	127			//最大的电源组数
	
#define MAX_LIGHT_SENSOR    100		//LUX到电压的查找表个数
	
typedef struct pwctrl_time_data {
	unsigned char		hour;
	unsigned char		minute;
	unsigned char		second;
} pwctrl_time;
typedef struct ctrl_serial_data {
	unsigned int		duration_ms;
	unsigned char		power_status[MAX_POWER_NUM];
} ctrl_serial;

typedef struct ctrl_group_data {
	pwctrl_time 		start_time;
	pwctrl_time 		end_time;
	unsigned int		projector_on;
	unsigned int		ctrl_serial_num;
	ctrl_serial 		serial_info[MAX_SERIAL_NUM];
} ctrl_group;
typedef struct light_sensor_data {
	unsigned int		lux;
	unsigned int		voltage;
} light_sensor;

typedef struct net_control_data {
	unsigned int		xml_version;
	unsigned int		stop_flag;
	unsigned int		power_num;
	unsigned int		projector_type;
	unsigned int		group_num;
	ctrl_group			group[MAX_CTRL_GROUP];
	unsigned char		current_power_status[MAX_POWER_NUM];
	int 				wtime_start;
	int 				wtime_end;
	
	//light sensor
	unsigned int		sensor_stop_flag;
	unsigned int		sensor_cnt; 			//光感个数，一路I2C能读3路光感
	unsigned int		sensor_power_num;		//控制电源的路数
	unsigned int		sensor_check_delay;
	unsigned int		sensor_num; 			//senser数组的有效个数
	light_sensor		sensor[MAX_LIGHT_SENSOR];
} pdlc_pwsw_info;

pdlc_pwsw_info pwsw_h={0};


static int OutHead()
{
	fprintf(cgiOut, "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n");
	fprintf(cgiOut, "<head>\n");
	fprintf(cgiOut, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=gb2312\" />\n");
	fprintf(cgiOut, "<title>电源控制设置</title>\n");
	fprintf(cgiOut, "</head>\n");
}



static int OutBodyStart()
{
	fprintf(cgiOut, "<body>\n<center>\n");
	fprintf(cgiOut, "<p>&nbsp;</p>\n");
	//fprintf(cgiOut, "<form id=\"form1\" name=\"form1\" method=\"post\" action=\"\">\n");
}

static int OutBodyEnd()
{
	
	fprintf(cgiOut, "</center>\n</body>\n");
	fprintf(cgiOut, "</html>\n");
}

static void save_to_xml_file()
{
	FILE *xml_file = NULL;
	int i,j,k;
	
	xml_file = fopen("/var/boa/light_tmp.xml", "wb");
	if(xml_file)
	{

		fprintf(xml_file, "<?xml version=\"1.0\" encoding=\"GB2312\" ?>\n\n");
		fprintf(xml_file, "<light_sensor version=\"1\" stop_flag=\"0\" power_num=\"4\" check_lux_delay=\"1000\">\n");
		for(i=0;i<pwsw_h.sensor_num;i++)
		{
			fprintf(xml_file, "        <serial light=\"%d\" voltage=\"%d\"></serial> \n",
				pwsw_h.sensor[i].lux,pwsw_h.sensor[i].voltage);
		}
		fprintf(xml_file, "</light_sensor>\n");
		fclose(xml_file);
		return ;
	}
	return ;
	
}

static int get_lux(int sensor_idx)
{
	int value;
	char name[81];
	memset(name,0,sizeof(name));
	sprintf(name,"lux%d",sensor_idx);

	cgiFormInteger(name, &value, 0);
	return value;
}
static int get_voltage(int sensor_idx)
{
	int value;
	char name[81];
	memset(name,0,sizeof(name));
	sprintf(name,"volt%d",sensor_idx);

	cgiFormInteger(name, &value, 0);
	return value;
}

int check_password()
{
	char username[30];
	char password[30];

	memset(username,0,sizeof(username));
	memset(password,0,sizeof(password));
	if(cgiFormNotFound == cgiFormStringNoNewlines("j_password", password, 30))
	{
		//fprintf(cgiOut, "<p>&nbsp; cgiFormNotFound</p>\n");
		cgiCookieString("admin", password, sizeof(password));
		memcpy(username,"admin",5);
		//fprintf(cgiOut, "<p>&nbsp; ==11%s==%s----%s</p>\n",cgiRemoteAddr,username,password);
	}
	else
	{
		cgiFormStringNoNewlines("j_username", username, 30);	
		cgiFormStringNoNewlines("j_password", password, 30);
	}

	//fprintf(cgiOut, "<p>&nbsp; %s----%s</p>\n",username,password);
	if(!memcmp(username,"admin",5) && !memcmp(password,"asdf1~",6))
	{
		return 1;
	}
	else
	{
		fprintf(cgiOut, "<p>&nbsp;用户未登录或密码错误,请返回重新输入!</p>\n");
		fprintf(cgiOut, "&nbsp;&nbsp;&nbsp;&nbsp;<input type=\"button\" name=\"rest\" onclick=\"javascript:window.location.href='/index.html'\" value=\"返回\" />\n");
		
		fprintf(cgiOut, "</center>\n</body>\n");
		fprintf(cgiOut, "</html>\n");
		return 0;
	}
}

int cgiMain() {
	int sensor_num,sensor_idx;
	int i,j,k;
	char name[81];
	light_sensor *p_sensor ;

	cgiFormInteger("sensor_num", &sensor_num, 0);
	
	if(sensor_num > 100) sensor_num = 100;

	cgiWriteEnvironment("/CHANGE/THIS/PATH/capcgi.dat");
	cgiHeaderContentType("text/html");

	OutHead();
	OutBodyStart();

	if(0 == check_password())
		return 0;

	if(0 == sensor_num)
	{
		fprintf(cgiOut, "<p>数据出错，退出!!</p>\n");
		OutBodyEnd();	
		return 0;
	}
	else
	{
		pwsw_h.sensor_num= sensor_num;

		for(i=0;i<sensor_num;i++)
		{
			p_sensor = &pwsw_h.sensor[i];
			p_sensor->lux = get_lux(i+1);
			p_sensor->voltage = get_voltage(i+1);
		}
	}

	save_to_xml_file();
	fprintf(cgiOut, "<p>保存成功，请返回!</p>\n");
	fprintf(cgiOut, "&nbsp;&nbsp;&nbsp;&nbsp;<input type=\"button\" name=\"rest\" onclick=\"javascript:history.go(-1)\" value=\"重新载入\" />\n");

	OutBodyEnd();

	return 0;
}



