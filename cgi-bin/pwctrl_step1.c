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

int OutBodyConfigForm(int power_num,int group_num,int n1,int n2,int n3,int n4,int n5)
{
	fprintf(cgiOut, "<form id=\"form2\" name=\"form2\" method=\"post\" action=\"\">\n");
	fprintf(cgiOut, "电源路数（最大100）:  <input name=\"power_num\" type=\"text\" size=\"3\" value=\"%d\"/>\n",power_num);
	fprintf(cgiOut, "工作时间段个数（最大5）:  <input name=\"group_num\" type=\"text\" size=\"3\" value=\"%d\" />\n",group_num);
	fprintf(cgiOut, "每个时间断的状态数（最大100）: <input name=\"group1_num\" type=\"text\" size=\"3\" value=\"%d\"  /> <input name=\"group2_num\" type=\"text\" size=\"3\" value=\"%d\" /> <input name=\"group3_num\" type=\"text\" size=\"3\" value=\"%d\"  /> <input name=\"group4_num\" type=\"text\" size=\"3\" value=\"%d\" /> <input name=\"group5_num\" type=\"text\" size=\"3\" value=\"%d\" />\n",n1,n2,n3,n4,n5);
	fprintf(cgiOut, "&nbsp;&nbsp;&nbsp;&nbsp;<input type=\"submit\" name=\"Submit\" value=\"设置\" />\n");
	fprintf(cgiOut, "&nbsp;&nbsp;&nbsp;&nbsp;<input type=\"button\" name=\"rest\" onclick=\"javascript:location.reload()\" value=\"重新载入\" />\n");
	fprintf(cgiOut, "</form>\n");

	fprintf(cgiOut, "<form id=\"form1\" name=\"form1\" method=\"post\" action=\"/cgi/pwctrl_step2\">\n");
	
	fprintf(cgiOut, "<input type=\"hidden\" name=\"power_num\" value=\"%d\"/>\n",power_num);
	fprintf(cgiOut, "<input type=\"hidden\" name=\"group_num\" value=\"%d\"/>\n",group_num);
	fprintf(cgiOut, "<input type=\"hidden\" name=\"serial_num\" value=\"%d-%d-%d-%d-%d\"/>\n",n1,n2,n3,n4,n5);

}
int OutGroup(int power_num,int serial_num,int group_idx)
{
	int i,j,k;
	ctrl_group *p_group = &pwsw_h.group[group_idx];

	fprintf(cgiOut, "  <hr/>\n");
	fprintf(cgiOut, "<p>\n");
	fprintf(cgiOut, "<label>开始时间  <input name=\"starttime%d\" type=\"text\" value=\"%d:%d:%d\" size=\"10\" />  </label>\n",group_idx,p_group->start_time.hour,p_group->start_time.minute,p_group->start_time.second);
	fprintf(cgiOut, "结束时间    <input name=\"endtime%d\" type=\"text\" value=\"%d:%d:%d\" size=\"10\" />\n",group_idx,p_group->end_time.hour,p_group->end_time.minute,p_group->end_time.second);
	if(p_group->projector_on)
		fprintf(cgiOut, "投影机状态	<select name=\"prjstatus%d\" size=\"1\">	  <option value=\"1\" selected=\"selected\">开</option>	  <option value=\"0\">关</option>	</select>\n",group_idx);
	else
		fprintf(cgiOut, "投影机状态	<select name=\"prjstatus%d\" size=\"1\">	  <option value=\"1\">开</option>	  <option value=\"0\" selected=\"selected\">关</option>	</select>\n",group_idx);
		
	fprintf(cgiOut, "</p>\n");

	fprintf(cgiOut, "<table width=\"80%\" height=\"78\" border=\"1\" bordercolor=\"#999999\">\n");

	fprintf(cgiOut, "  <tr>\n");
	fprintf(cgiOut, "  <td width=\"267\" height=\"25\">控制路数&nbsp;</td>");

	for(i=0;i<power_num;i++)
	{
		fprintf(cgiOut, "  <td width=\"75\"><div align=\"center\">%d</div></td>\n",i+1);
	}
	fprintf(cgiOut, "  </tr>\n");

	fprintf(cgiOut, "\n");

	for(j=0;j<serial_num;j++)
	{
		fprintf(cgiOut, "  <tr>\n");
		fprintf(cgiOut, "    <td height=\"23\">持续时间(ms)<input name=\"duration%d_%d\" type=\"text\" id=\"duration\" value=\"%d\" size=\"3\" /></td>\n",group_idx,j+1,p_group->serial_info[j].duration_ms);
		for(i=0;i<power_num;i++)
		{
			if(0 != p_group->serial_info[j].power_status[i])
				fprintf(cgiOut, "	 <td><p align=\"center\"><label><select name=\"select%d_%d_%d\" size=\"1\"><option value=\"1\"  selected=\"selected\">ON</option><option value=\"0\">OFF</option> </select> </label> </p>	  </td>\n",group_idx,j+1,i+1);
			else
				fprintf(cgiOut, "	 <td><p align=\"center\"><label><select name=\"select%d_%d_%d\" size=\"1\"><option value=\"1\">ON</option><option value=\"0\"  selected=\"selected\">OFF</option> </select> </label> </p>	  </td>\n",group_idx,j+1,i+1);
		}
		fprintf(cgiOut, "  </tr>\n");
		fprintf(cgiOut, "\n");
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
	int power_num,group_serial_num[5],group_num,group_idx;

	
	power_num = 8;
	group_num = 6;
	cgiFormInteger("power_num", &power_num, 0);
	cgiFormInteger("group_num", &group_num, 0);
	cgiFormInteger("group1_num", &group_serial_num[0], 0);
	cgiFormInteger("group2_num", &group_serial_num[1], 0);
	cgiFormInteger("group3_num", &group_serial_num[2], 0);
	cgiFormInteger("group4_num", &group_serial_num[3], 0);
	cgiFormInteger("group5_num", &group_serial_num[4], 0);

	pdlc_load_pwctrl_from_xml();

	if(power_num > 100) power_num = 100;
	if(group_num > 5) group_num = 5;
	if(group_serial_num[0] > 100) group_serial_num[0] = 100;
	if(group_serial_num[1] > 100) group_serial_num[1] = 100;
	if(group_serial_num[2] > 100) group_serial_num[2] = 100;
	if(group_serial_num[3] > 100) group_serial_num[3] = 100;
	if(group_serial_num[4] > 100) group_serial_num[4] = 100;
	
	if(0 == power_num)
	{
		power_num = pwsw_h.power_num;
		group_num = pwsw_h.group_num;
		group_serial_num[0] = pwsw_h.group[0].ctrl_serial_num;
		group_serial_num[1] = pwsw_h.group[1].ctrl_serial_num;
		group_serial_num[2] = pwsw_h.group[2].ctrl_serial_num;
		group_serial_num[3] = pwsw_h.group[3].ctrl_serial_num;
		group_serial_num[4] = pwsw_h.group[4].ctrl_serial_num;
	}
	else
	{
#if 1	
		pwsw_h.power_num = power_num;
		pwsw_h.group_num = group_num;
		pwsw_h.group[0].ctrl_serial_num = group_serial_num[0];
		pwsw_h.group[1].ctrl_serial_num = group_serial_num[1];
		pwsw_h.group[2].ctrl_serial_num = group_serial_num[2];
		pwsw_h.group[3].ctrl_serial_num = group_serial_num[3];
		pwsw_h.group[4].ctrl_serial_num = group_serial_num[4];
#endif
	}

	cgiWriteEnvironment("/CHANGE/THIS/PATH/capcgi.dat");
	CookieSet();
	cgiHeaderContentType("text/html");

	OutHead();
	OutBodyStart();

	if(0 == check_password())
		return 0;
	
	OutBodyConfigForm(power_num,group_num,group_serial_num[0],group_serial_num[1],
		group_serial_num[2],group_serial_num[3],group_serial_num[4]);

//	fprintf(cgiOut, "<p>power_num=%d group_num=%d [%d-%d-%d-%d-%d]</p>\n",
//		power_num,group_num,group_serial_num[0],group_serial_num[1],
//		group_serial_num[2],group_serial_num[3],group_serial_num[4]);

	for(group_idx=0;group_idx<pwsw_h.group_num;group_idx++)
	{
		OutGroup(pwsw_h.power_num,pwsw_h.group[group_idx].ctrl_serial_num,group_idx);
	}

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




