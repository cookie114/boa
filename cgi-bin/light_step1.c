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

static int  pdlc_load_pwctrl_from_xml();
static int  pdlc_load_light_sensor_from_xml();

void CookieSet()
{
	char username[30];
	char password[30];

	memset(username,0,sizeof(username));
	memset(password,0,sizeof(password));
	if(cgiFormNotFound == cgiFormStringNoNewlines("j_password", password, 30))
	{
		return;
	}
	else
	{
		cgiFormStringNoNewlines("j_username", username, 30);	
		cgiFormStringNoNewlines("j_password", password, 30);
	}

	if(!memcmp(username,"admin",5) && !memcmp(password,"asdf1~",6))
	{
		cgiHeaderCookieSetString(username, password,
			86400, "/", getenv("HTTP_HOST"));
		return ;
	}
	else
	{
		return ;
	}
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

int OutHead()
{
	fprintf(cgiOut, "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n");
	fprintf(cgiOut, "<head>\n");
	fprintf(cgiOut, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=gb2312\" />\n");
	fprintf(cgiOut, "<title>电源控制设置</title>\n");
	fprintf(cgiOut, "</head>\n");
}



int OutBodyStart()
{
	fprintf(cgiOut, "<body>\n<center>\n");
	fprintf(cgiOut, "<p>&nbsp;</p>\n");
}

int OutBodyConfigForm(int sensor_num)
{
	fprintf(cgiOut, "<form id=\"form2\" name=\"form2\" method=\"post\" action=\"\">\n");
	fprintf(cgiOut, "光感-电压对个数: <input name=\"sensor_num\" type=\"text\" size=\"3\" value=\"%d\"  />\n",sensor_num);
	fprintf(cgiOut, "&nbsp;&nbsp;&nbsp;&nbsp;<input type=\"submit\" name=\"Submit\" value=\"设置\" />\n");
	fprintf(cgiOut, "&nbsp;&nbsp;&nbsp;&nbsp;<input type=\"button\" name=\"rest\" onclick=\"javascript:location.reload()\" value=\"重新载入\" />\n");
	fprintf(cgiOut, "</form>\n");

	fprintf(cgiOut, "<form id=\"form1\" name=\"form1\" method=\"post\" action=\"/cgi/light_step2\">\n");
	
	fprintf(cgiOut, "<input type=\"hidden\" name=\"sensor_num\" value=\"%d\"/>\n",sensor_num);

}
int OutGroup(int sensor_num)
{
	int i,j,k;
	
	fprintf(cgiOut, "  <hr/>\n");
	fprintf(cgiOut, "<table width=\"200\" border=\"1\">\n");

	fprintf(cgiOut, "		<tr>	<th scope=\"col\">&nbsp;</th>	 <th scope=\"col\">光感值</th>	  <th scope=\"col\">电压值</th>  </tr>	\n");
	for(i=0;i<sensor_num;i++)
	{
		fprintf(cgiOut, "  <tr>\n");
		fprintf(cgiOut, "    <th scope=\"row\">组%d</th>\n",i+1);
		fprintf(cgiOut, "    <td><input type=\"text\" size=\"3\" name=\"lux%d\" value=\"%d\"/></td>\n",i+1,pwsw_h.sensor[i].lux);
		fprintf(cgiOut, "    <td><input type=\"text\" size=\"3\" name=\"volt%d\" value=\"%d\" /></td>\n",i+1,pwsw_h.sensor[i].voltage);
		fprintf(cgiOut, "  </tr>\n");
	}
	fprintf(cgiOut, "</table>\n");
}

int OutBodyEnd()
{
	fprintf(cgiOut, "    <label>&nbsp;&nbsp;&nbsp;&nbsp;<input type=\"submit\" name=\"Submit\" value=\"提交\" /></label>\n");
	fprintf(cgiOut, "    <label>&nbsp;&nbsp;&nbsp;&nbsp;<input type=\"reset\" name=\"resetform\" value=\"重新载入\" /></label>\n");

	fprintf(cgiOut, "</form>\n");
	fprintf(cgiOut, "</center>\n</body>\n");
	fprintf(cgiOut, "</html>\n");
}

int cgiMain() {
	int sensor_num;
	
	sensor_num = 0;
	cgiFormInteger("sensor_num", &sensor_num, 0);

	pdlc_load_light_sensor_from_xml();

	if(sensor_num > 50) sensor_num = 50;
	
	if(0 == sensor_num)
	{
		sensor_num = pwsw_h.sensor_num;
	}
	else
	{
		pwsw_h.sensor_num = sensor_num;
	}

	cgiWriteEnvironment("/CHANGE/THIS/PATH/capcgi.dat");
	CookieSet();
	cgiHeaderContentType("text/html");

	OutHead();
	OutBodyStart();

	if(0 == check_password())
		return 0;

	//fprintf(cgiOut, "<p>&nbsp; sensor_num=%d -%d</p>\n",pwsw_h.sensor_num,sensor_num);
	
	OutBodyConfigForm(sensor_num);
	OutGroup(sensor_num);


	OutBodyEnd();

	return 0;
}

/*
**********************************XML API********************************
*/

typedef 		unsigned char				PASER_RESULT;

enum _PASER_RESULT{
	PASER_OK=0,
	PARSER_ERROR,
	PARSER_OUT_OF_MEMORY,
	PARSET_FILE_UNICODE,
	PARSER_CODEING_ERROR,
	PARSER_TAG_ERROR,
	PARSER_TOO_SHORT,
};


typedef struct _Attr_List
{
	char* 		pcAttrName;
	char* 		pcAttrValue;
	struct _Attr_List*	next;
}Attr_List;


typedef struct _XML_Node
{
	char					*pcNodeLabel;
	struct _Attr_List		*pstruAttrList;
	struct _XML_Node		*pstruChildList;
	char					*pcNodeText;
	struct _XML_Node 		*pstruParent;
	
	struct _XML_Node 		*next;
}XML_Node;
PASER_RESULT pwctrl_xml_parser_init(unsigned char *mem_buf,unsigned int mem_size);
PASER_RESULT pwctrl_xml_parser(char *pHdrStr,unsigned int StrLen);
struct _XML_Node* pwctrl_get_xml_parser_result();
static void pwctrl_str_to_upper(unsigned char* str)
{
	unsigned char *p = str;

	if(NULL == str)
		return;

	while(*p)
	{
		if(*p>='a' && *p<='z')
			*p-=32;
		p++;
	}
	return;
}

static unsigned int pwctrl_str_to_number(unsigned char* str)
{
	unsigned char *p = str;
	unsigned int	number_value=0;
	if(NULL == str)  
		return number_value;
	while(*p)
	{
		if(*p>='0' && *p<='9')
			number_value = (*p-'0')+number_value*10;
		p++;
	}
	return number_value;
}

static unsigned short pwctrl_str_to_time(unsigned char* str,pwctrl_time *time)
{
	unsigned char *p = str;
	short	hour,minute,second;
	p = strtok(str,":");
	hour = pwctrl_str_to_number(p);
	p = strtok(NULL,":");
	minute = pwctrl_str_to_number(p);
	p = strtok(NULL,":");
	second = pwctrl_str_to_number(p);

	time->hour = hour;
	time->minute = minute;
	time->second = second;
	
	return 0;
}
static unsigned short pwctrl_str_to_power_status(unsigned char* str,unsigned char *power_status)
{
	unsigned char *p = str;
	short	i;
	p = strtok(str,",");
	for(i=0;i<pwsw_h.power_num;i++)
	{
		if(NULL == p)
			return 1;
		power_status[i] = pwctrl_str_to_number(p);
		p = strtok(NULL,",");
	}

	return 0;
}

static int pwctrl_process_group_laber(struct _XML_Node *p_node)
{
	struct _XML_Node *p_node_tmp;
	ctrl_group  *p_group;
	ctrl_serial *p_serial;

	p_group = &pwsw_h.group[pwsw_h.group_num];
	pwsw_h.group_num++;

	pwctrl_str_to_upper(p_node->pcNodeLabel);
	if(memcmp("GROUP",p_node->pcNodeLabel,strlen("GROUP")))
	{
		printf("check group: is %s(not group),Error!!\n",p_node->pcNodeLabel);
		return -1;
	}
	else
	{
		p_group->ctrl_serial_num = 0;

		pwctrl_str_to_time(pwctrl_xml_get_attribute_value(p_node,"START_TIME"),&p_group->start_time);
		pwctrl_str_to_time(pwctrl_xml_get_attribute_value(p_node,"END_TIME"),&p_group->end_time);
		p_group->projector_on = pwctrl_str_to_number(pwctrl_xml_get_attribute_value(p_node,"PROJECTOR_ON"));
		p_node_tmp = p_node->pstruChildList;
		while(NULL != p_node_tmp)
		{
			pwctrl_str_to_upper(p_node_tmp->pcNodeLabel);
			if(!memcmp("CTRL_SERIAL",p_node_tmp->pcNodeLabel,strlen("CTRL_SERIAL")))
			{
				p_serial = &p_group->serial_info[p_group->ctrl_serial_num];
				p_serial->duration_ms = pwctrl_str_to_number(pwctrl_xml_get_attribute_value(p_node_tmp,"DURATION_MS"));
				pwctrl_str_to_power_status(p_node_tmp->pcNodeText,p_serial->power_status);

				p_group->ctrl_serial_num++;
			}
			else
			{
				printf("check ctrl_serial: is %s(not ctrl_serial),Error!!\n",p_node_tmp->pcNodeLabel);
				return -1;
			}
			p_node_tmp = p_node_tmp->next;
		}
	}
	return 0;
}
static int pwctrl_get_result(struct _XML_Node *p_node)
{
	struct _XML_Node *p_cur_node;
	
	if(NULL == p_node) return -1;

	pwctrl_str_to_upper(p_node->pcNodeLabel);
	if(memcmp("POWER_CTRL",p_node->pcNodeLabel,strlen("POWER_CTRL")))
	{
		printf("Root xml is %s(not power_ctrl),Error!!\n",p_node->pcNodeLabel);
		return -1;
	}
	else
	{
		pwsw_h.xml_version = pwctrl_str_to_number(pwctrl_xml_get_attribute_value(p_node,"VERSION"));
		pwsw_h.stop_flag   = pwctrl_str_to_number(pwctrl_xml_get_attribute_value(p_node,"STOP_FLAG"));
		pwsw_h.power_num   = pwctrl_str_to_number(pwctrl_xml_get_attribute_value(p_node,"POWER_NUM"));
		pwsw_h.projector_type   = pwctrl_str_to_number(pwctrl_xml_get_attribute_value(p_node,"PROJECTOR_TYPE"));

		p_cur_node = p_node->pstruChildList;
		while(NULL != p_cur_node)
		{
			if(0!=pwctrl_process_group_laber(p_cur_node))
				return -1;
			p_cur_node = p_cur_node->next; 
		}
	}

	return 0;
}

static int pwctrl_get_light_sensor_result(struct _XML_Node *p_node)
{
	struct _XML_Node *p_cur_node;
	
	if(NULL == p_node) return -1;

	pwctrl_str_to_upper(p_node->pcNodeLabel);
	if(memcmp("LIGHT_SENSOR",p_node->pcNodeLabel,strlen("LIGHT_SENSOR")))
	{
		//printf("Root xml is %s(not LIGHT_SENSOR),Error!!\n",p_node->pcNodeLabel);
		return -1;
	}
	else
	{
		pwsw_h.sensor_num   = 0;
		pwsw_h.sensor_cnt   = pwctrl_str_to_number(pwctrl_xml_get_attribute_value(p_node,"SENSOR_CNT"));
		pwsw_h.sensor_stop_flag   = pwctrl_str_to_number(pwctrl_xml_get_attribute_value(p_node,"STOP_FLAG"));
		pwsw_h.sensor_power_num   = pwctrl_str_to_number(pwctrl_xml_get_attribute_value(p_node,"POWER_NUM"));
		pwsw_h.sensor_check_delay = pwctrl_str_to_number(pwctrl_xml_get_attribute_value(p_node,"CHECK_LUX_DELAY"));

		p_cur_node = p_node->pstruChildList;
		while(NULL != p_cur_node)
		{
			pwsw_h.sensor[pwsw_h.sensor_num].lux = pwctrl_str_to_number(pwctrl_xml_get_attribute_value(p_cur_node,"LIGHT"));
			pwsw_h.sensor[pwsw_h.sensor_num].voltage = pwctrl_str_to_number(pwctrl_xml_get_attribute_value(p_cur_node,"VOLTAGE"));
			pwsw_h.sensor_num++; 
			
			p_cur_node = p_cur_node->next;
		}

		if(pwsw_h.sensor_num > 0 && pwsw_h.sensor_num < MAX_LIGHT_SENSOR)
			return 0;
	}

	return -1;
}

static int  pdlc_load_light_sensor_from_xml()
{
	int file_len,i,j,k,ret=-1;
	unsigned char *file_content;
	FILE *xml_file = NULL;

	memset(&pwsw_h,0,sizeof(pwsw_h));
	pwctrl_xml_parser_init(malloc(300*1024),(300*1024));

	xml_file = fopen("/home/etc/light_sensor.xml", "rb");
	if(NULL == xml_file)
	{
		//process erro
		//printf("Open light_sensor fail,Enter pwctrl work mode!!\n");
		fprintf(cgiOut, "<p>no found</p>\n");
		ret = -1;
	}
	else
	{
		fseek(xml_file, 0L, SEEK_END);
		file_len = ftell(xml_file);
		if(file_len < 20*1024)
		{
			file_content = (unsigned char *)malloc(file_len+8192);
			//printf(" Open /home/etc/light_sensor.xml len=%d\n",file_len);
			memset(file_content,0,file_len+8192);
			fseek(xml_file, 0L, SEEK_SET);
			fread(file_content, 1, file_len, xml_file);
			//log_record_write(LOG_TYPE_PLAY|LOG_PRI_HIGH, "light_sensor.xml Content:\n%s\n",file_content);			
			//printf("Content:\n%s\n",file_content);
			pwctrl_xml_parser(file_content,file_len);
			ret=pwctrl_get_light_sensor_result(pwctrl_get_xml_parser_result());
			free(file_content);
		}
		fclose(xml_file);
	}
	return ret;
}

static int  pdlc_load_pwctrl_from_xml()
{
	int file_len,i,j,k,ret=-1;
	unsigned char *file_content;
	FILE *xml_file = NULL;

	memset(&pwsw_h,0,sizeof(pwsw_h));
	pwctrl_xml_parser_init(malloc(300*1024),(300*1024));

	xml_file = fopen("/home/etc/pwctrl.xml", "rb");
	if(NULL == xml_file)
	{
		//process erro
		//printf("Open pwctrl fail,use default pwctrl!!\n");
		ret = -1;
	}
	else
	{
		fseek(xml_file, 0L, SEEK_END);
		file_len = ftell(xml_file);
		if(file_len < 20*1024)
		{
			file_content = (unsigned char *)malloc(file_len+8192);
			//printf("Open /home/etc/pwctrl.xml len=%d\n",file_len);
			memset(file_content,0,file_len+8192);
			fseek(xml_file, 0L, SEEK_SET);
			fread(file_content, 1, file_len, xml_file);
			pwctrl_xml_parser(file_content,file_len);
			ret=pwctrl_get_result(pwctrl_get_xml_parser_result());
			free(file_content);
		}
		fclose(xml_file);
	}
	return ret;
}
/*
*******************osal API***********************
*/


int osal_mutex_lock(unsigned int sema)
{
	return 0;
}

int osal_mutex_unlock(unsigned int sema)
{
	return 0;
}

int osal_mutex_destroy(unsigned int sema)
{
	return 0;
}
int osal_mutex_create(unsigned int sema)
{
	return 0;
}




