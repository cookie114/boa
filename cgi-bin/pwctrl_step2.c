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

static void Out_pwsw_xml()
{
	ctrl_group  *p_group;
	int i,j,k;
	fprintf(cgiOut, "<p>\n\n***********************XML Info**************</p>\n");
	fprintf(cgiOut, "<p>Version:%d power_num=%d prj_type=%d stop_flag=%d</p>\n",pwsw_h.xml_version,pwsw_h.power_num,pwsw_h.projector_type,pwsw_h.stop_flag);
	fprintf(cgiOut, "<p>Group cnt:%d</p>\n",pwsw_h.group_num);
	for(i=0;i<pwsw_h.group_num;i++)
	{
		p_group = &pwsw_h.group[i];
		fprintf(cgiOut, "<p>\n=========================================================</p>\n");
		fprintf(cgiOut, "<p>Start Time:%02d:%02d:%02d  End time:%02d:%02d:%02d serial_num=%d  prj_on=%d</p>\n",
			p_group->start_time.hour,p_group->start_time.minute,p_group->start_time.second,
			p_group->end_time.hour,p_group->end_time.minute,p_group->end_time.second,p_group->ctrl_serial_num,p_group->projector_on);

		for(j=0;j<p_group->ctrl_serial_num;j++)
		{
			fprintf(cgiOut, "<p>Serial[%d] Duration=%d statue:",j,p_group->serial_info[j].duration_ms);
			for(k=0;k<pwsw_h.power_num;k++)
			{
				fprintf(cgiOut, "%d-",p_group->serial_info[j].power_status[k]);
			}
			fprintf(cgiOut, "</p>\n");
		}
	}
}
static void save_to_xml_file()
{
	FILE *xml_file = NULL;
	int i,j,k;
	
	xml_file = fopen("/var/boa/pwctrl_tmp.xml", "wb");
	if(xml_file)
	{

		fprintf(xml_file, "<?xml version=\"1.0\" encoding=\"GB2312\" ?>\n\n");
		fprintf(xml_file, "<power_ctrl version=\"1\"  stop_flag=\"0\" power_num=\"%d\">\n",pwsw_h.power_num);
		for(i=0;i<pwsw_h.group_num;i++)
		{
			fprintf(xml_file, "    <group start_time=\"%02d:%02d:%02d\" end_time=\"%02d:%02d:%02d\">\n",
				pwsw_h.group[i].start_time.hour,pwsw_h.group[i].start_time.minute,pwsw_h.group[i].start_time.second,
				pwsw_h.group[i].end_time.hour,pwsw_h.group[i].end_time.minute,pwsw_h.group[i].end_time.second
				);
			for(j=0;j<pwsw_h.group[i].ctrl_serial_num;j++)
			{
				fprintf(xml_file, "        <ctrl_serial duration_ms=\"%d\">",pwsw_h.group[i].serial_info[j].duration_ms);
				for(k=0;k<pwsw_h.power_num;k++)
				{
					if(k!=pwsw_h.power_num-1)
					{
						fprintf(xml_file, "%d,",pwsw_h.group[i].serial_info[j].power_status[k]);
					}
					else
					{
						fprintf(xml_file, "%d",pwsw_h.group[i].serial_info[j].power_status[k]);						
					}
				}
				fprintf(xml_file, "</ctrl_serial>\n");
			}
			fprintf(xml_file, "    </group>\n");
		}
		fprintf(xml_file, "</power_ctrl>\n");
		fclose(xml_file);
		return ;
	}
	return ;
	
}
static int get_serial_num(char * str,int **v)
{
	int a,b,c,d,e;
	sscanf (str, "%d-%d-%d-%d-%d", &a, &b, &c, &d, &e);
	v[0] = a;
	v[1] = b;
	v[2] = c;
	v[3] = d;
	v[4] = e;
	return 0;
}
static int get_start_time(int group_idx,pwctrl_time *time)
{
	char value[12];
	char name[10];
	memset(name,0,sizeof(name));
	sprintf(name,"starttime%d",group_idx);

	cgiFormString(name, value, 12);
	sscanf (value, "%d:%d:%d", &time->hour,&time->minute,&time->second);
	
	return 0;
}

static int get_end_time(int group_idx,pwctrl_time *time)
{
	char value[12];
	char name[10];
	memset(name,0,sizeof(name));
	sprintf(name,"endtime%d",group_idx);

	cgiFormString(name, value, 12);
	sscanf (value, "%d:%d:%d", &time->hour,&time->minute,&time->second);
	
	return 0;
}

static int get_duration_ms(int group_idx,int serial_idx)
{
	int value;
	char name[81];
	memset(name,0,sizeof(name));
	sprintf(name,"duration%d_%d",group_idx,serial_idx+1);

	cgiFormInteger(name, &value, 0);
	return value;
}
static int get_power_status(int group_idx,int serial_idx,int power_idx)
{
	int value;
	char name[81];
	memset(name,0,sizeof(name));
	sprintf(name,"select%d_%d_%d",group_idx,serial_idx+1,power_idx+1);

	cgiFormInteger(name, &value, 0);
	if(0 == value)
		return 0;
	else
		return 10;
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
	int power_num,group_serial_num[5],group_num,group_idx;
	int i,j,k;
	char name[81];
	ctrl_group *p_group ;

	cgiFormInteger("power_num", &power_num, 0);
	cgiFormInteger("group_num", &group_num, 0);
	memset(name,0,sizeof(name));
	cgiFormStringNoNewlines("serial_num", name, 81);
	get_serial_num(name,&group_serial_num);
	
	if(power_num > 100) power_num = 100;
	if(group_num > 5) group_num = 5;
	if(group_serial_num[0] > 100) group_serial_num[0] = 100;
	if(group_serial_num[1] > 100) group_serial_num[1] = 100;
	if(group_serial_num[2] > 100) group_serial_num[2] = 100;
	if(group_serial_num[3] > 100) group_serial_num[3] = 100;
	if(group_serial_num[4] > 100) group_serial_num[4] = 100;


	cgiWriteEnvironment("/CHANGE/THIS/PATH/capcgi.dat");
	cgiHeaderContentType("text/html");

	OutHead();
	OutBodyStart();

	if(0 == check_password())
		return 0;

	fprintf(cgiOut, "<p>power_num=%d group_num=%d [%d-%d-%d-%d-%d]</p>\n",
		power_num,group_num,group_serial_num[0],group_serial_num[1],
		group_serial_num[2],group_serial_num[3],group_serial_num[4]);

	if(0 == power_num)
	{
		fprintf(cgiOut, "<p>数据出错，退出!!</p>\n");
		OutBodyEnd();	
		return 0;
	}
	else
	{
		pwsw_h.power_num = power_num;
		pwsw_h.group_num = group_num;
		pwsw_h.group[0].ctrl_serial_num = group_serial_num[0];
		pwsw_h.group[1].ctrl_serial_num = group_serial_num[1];
		pwsw_h.group[2].ctrl_serial_num = group_serial_num[2];
		pwsw_h.group[3].ctrl_serial_num = group_serial_num[3];
		pwsw_h.group[4].ctrl_serial_num = group_serial_num[4];

		for(i=0;i<group_num;i++)
		{
			p_group = &pwsw_h.group[i];
			p_group->projector_on = 0;
			p_group->ctrl_serial_num = group_serial_num[i];
			get_start_time(i,&p_group->start_time);
			get_end_time(i,&p_group->end_time);
			for(j=0;j<p_group->ctrl_serial_num;j++)
			{
				p_group->serial_info[j].duration_ms = get_duration_ms(i,j);
				for(k=0;k<power_num;k++)
				{
					p_group->serial_info[j].power_status[k]= get_power_status(i,j,k);
				}
				
			}
		}
	}

	//Out_pwsw_xml();
	save_to_xml_file();
	fprintf(cgiOut, "<p>保存成功，请返回!</p>\n");
	fprintf(cgiOut, "&nbsp;&nbsp;&nbsp;&nbsp;<input type=\"button\" name=\"rest\" onclick=\"javascript:history.go(-1)\" value=\"重新载入\" />\n");

	OutBodyEnd();

	return 0;
}



