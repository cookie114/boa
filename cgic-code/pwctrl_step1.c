#include <stdio.h>
#include "cgic.h"
#include <string.h>
#include <stdlib.h>

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
	fprintf(cgiOut, "<body>\n");
	fprintf(cgiOut, "<form id=\"form1\" name=\"form1\" method=\"post\" action=\"\">\n");
	fprintf(cgiOut, "<p>\n");
	fprintf(cgiOut, "<label>开始时间  <input name=\"textfield3\" type=\"text\" value=\"00:00:00\" size=\"10\" />  </label>\n");
	fprintf(cgiOut, "结束时间    <input name=\"textfield2\" type=\"text\" value=\"23:59:59\" size=\"10\" />\n");
	fprintf(cgiOut, "投影机状态	<select name=\"select\" size=\"1\">	  <option value=\"1\">开</option>	  <option value=\"0\">关</option>	</select>\n");

	fprintf(cgiOut, "</p>\n");
}

int OutBodyEnd()
{
	fprintf(cgiOut, "</form>\n");
	fprintf(cgiOut, "</body>\n");
	fprintf(cgiOut, "</html>\n");
}
int cgiMain() {
	int i,j,k;
	int power_num,serial_num;

	power_num = 8;
	serial_num = 6;

	cgiWriteEnvironment("/CHANGE/THIS/PATH/capcgi.dat");
	cgiHeaderContentType("text/html");

	OutHead();
	OutBodyStart();
	fprintf(cgiOut, "<table width=\"765\" height=\"78\" border=\"1\" bordercolor=\"#999999\">\n");

	fprintf(cgiOut, "  <tr>\n");
	fprintf(cgiOut, "<td width=\"267\" height=\"25\">&nbsp;</td>");

	for(i=0;i<power_num;i++)
	{
		fprintf(cgiOut, "<td width=\"75\"><div align=\"center\">%d</div></td>\n",i+1);
	}
	fprintf(cgiOut, "  </tr>\n");

	fprintf(cgiOut, "\n");

	for(j=0;j<serial_num;j++)
	{
		fprintf(cgiOut, "  <tr>\n");


		fprintf(cgiOut, "    <td height=\"23\">持续时间(ms)<input name=\"duration\" type=\"text\" id=\"duration\" value=\"1000\" size=\"3\" /></td>\n");
		for(i=0;i<power_num;i++)
		{
			fprintf(cgiOut, "<td><p align=\"center\"><label><select name=\"select%d\" size=\"1\"><option value=\"1\">ON</option><option value=\"0\">OFF</option> </select> </label> </p>    </td>\n",i+1);
		}
		fprintf(cgiOut, "\n");
		fprintf(cgiOut, "\n");
	}

	fprintf(cgiOut, "</table>\n");
	OutBodyEnd();

	return 0;
}

