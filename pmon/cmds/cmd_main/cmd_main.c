/*
    PMON command main
    Designed by LiuXiangYu(������) USTC 04011
*/
/************************************************************************

 Copyright (C)
 File name:     cmd_main.c
 Author:  ***      Version:  ***      Date: ***
 Description:   
 Others:        
 Function List:
 
 Revision History:
 
 -------------------------------------------------------------------------------------------------------------------------------
  Date                Author             Activity ID               Activity Headline
  2008-05-17    LiuXiangYu      PMON20080517    Create it.
  2009-02-02    QianYuli           PMON20090202    Modified for porting to Fuloong and 8089 platform.
  2009-07-02    QianYuli           PMON20090702    Make cmd_main() function thin
****************************************************************************************************************/
#include <stdio.h>
#include <termio.h>
#include <endian.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif
#include <pmon.h>
#include <exec.h>
#include <file.h>
#include "window.h"

#include "mod_debugger.h"
#include "mod_symbols.h"

#include "sd.h"
#include "wd.h"

#include <time.h>

#define TM_YEAR_BASE 1900
#include "cmd_hist.h"
#include "cmd_more.h"
extern unsigned int memorysize;
extern unsigned int memorysize_high;
extern int screen_height;
extern int screen_width;
int com_counts;

extern int vga_available;

int run(char *cmd)
{
    char t[100];
    strcpy(t,cmd);
    return do_cmd(t);
}
struct _daytime
{
    char *name;
    short x,y;
    char buf[8];
    short buflen,base;
}daytime[6]=    
{
#ifdef  CHINESE   //http://www.cnblogs.com/markjiao/archive/2008/05/20/1203316.html
    {"��",2,5,"",6,TM_YEAR_BASE},
    {"��",12,5,"",4,1},
    {"��",22,5,"",4,0},
    {"Сʱ",2,6,"",4,0},
    {"����",12,6,"",4,0},
    {"��",22,6,"",4,0},
#else
    {"Year",1,5,"",6,TM_YEAR_BASE},
    {"Month",16,5,"",4,1},
    {"Day",29,5,"",4,0},
    {"Hour",1,6,"",4,0},
    {"Min",16,6,"",4,0},
    {"Sec",29,6,"",4,0},
#endif
};

typedef struct _window_info{
    int l_window_width;
    int r_window_width;
    int l_window_height ;
    int r_window_height;
    int number_of_tabs;
}window_info_t, *p_window_info_t;

    char w0[6][50];//buffer of window0"basic"
    char w1[6][50];//buffer of window1"boot"
    char w2[6][50];//buffer of window2"network"
    char sibuf[4][20];

#ifdef  CHINESE
    char *maintabs[]={"��Ҫ","����","����","�˳�"};
    char *f1[]={"wd0","wd1","usb0","usb1","tftp",0};
    char *f1b[]={"ext2","fat",0};
    char *f2[]={"rtl0","rtk0","em0","em1","fxp0",0};
#else
    char *maintabs[]={"Main","Boot","Network","Exit"};
    char *f1[]={"wd0","wd1","usb0","usb1","tftp",0};
    char *f1b[]={"ext2","fat",0};
    char *f2[]={"rtl0","rtk0","em0","em1","fxp0",0};
#endif

    char *message;//message passing through windows

int do_main_tab(p_window_info_t  pwinfo, char *phint,struct tm *p_tm)
{
    char line[100];
    int i;
    com_counts = 1;
#ifdef  CHINESE
            w_window(1,3,pwinfo->l_window_width,pwinfo->l_window_height,"������Ϣ");
            sprintf(phint,"����Ϊ��о������Ļ�����Ϣ.");
            if(w_button(21,4,20,"[���� ʱ��&����]"))w_setpage(DATETIME_WINDOW_ID);
            if(w_focused()) {
                sprintf(phint,"����<�س�>�������޸�ϵͳʱ�������");
            }
            /* Display the current date and time recored in BIOS */  
            w_text(2,4,WA_RIGHT,"ʱ�������:");
#else
            w_window(1,3,pwinfo->l_window_width,pwinfo->l_window_height,"Basic Information");
            sprintf(phint,"This is the basic information of Loongson Computer.");
            if(w_button(21,4,20,"[Modify Time & Date]"))w_setpage(DATETIME_WINDOW_ID);
            if(w_focused()) {
                sprintf(phint,"You can modify Time and Date here.");
            }
            /* Display the current date and time recored in BIOS */
            w_text(2,4,WA_RIGHT,"Time and Date:");
#endif
            for(i=0;i<6;i++) {
                sprintf(line,"%s:%d",daytime[i].name,((int *)(p_tm))[5-i]+daytime[i].base);
                w_text(daytime[i].x,daytime[i].y,WA_RIGHT,line);
            }
            /* Display CPU information */
#ifdef  CHINESE
            sprintf(line,"һ��ָ���: %dkb",CpuPrimaryInstCacheSize / 1024);
            w_bigtext(2,9,pwinfo->l_window_width-7,2,line);
            sprintf(line,"һ�����ݻ���: %dkb ",CpuPrimaryDataCacheSize / 1024);
            w_bigtext(2,10,pwinfo->l_window_width-7,2,line);
            if(CpuSecondaryCacheSize != 0) {
                sprintf(line,"��������: %dkb", CpuSecondaryCacheSize / 1024);
            }
            w_bigtext(2,11,pwinfo->l_window_width-7,2,line);
            if(CpuTertiaryCacheSize != 0) {
                sprintf(line,"��������: %dkb", CpuTertiaryCacheSize / 1024);
            }
            
            sprintf(line,"CPU: %s @ %d MHz",md_cpuname(),tgt_pipefreq()/1000000);
            w_text(2,7,WA_RIGHT,line);
            sprintf (line,"�ڴ�����: %d MB",memorysize+memorysize_high);
            w_bigtext(2,8,pwinfo->l_window_width-7,2,line);
#else
            sprintf(line,"Primary Instruction cache size: %dkb",CpuPrimaryInstCacheSize / 1024);
            w_bigtext(2,9,pwinfo->l_window_width-7,2,line);
            sprintf(line,"Primary Data cache size: %dkb ",CpuPrimaryDataCacheSize / 1024);
            w_bigtext(2,10,pwinfo->l_window_width-7,2,line);
            if(CpuSecondaryCacheSize != 0) {
                sprintf(line,"Secondary cache size: %dkb", CpuSecondaryCacheSize / 1024);
            }
            w_bigtext(2,11,pwinfo->l_window_width-7,2,line);
            if(CpuTertiaryCacheSize != 0) {
                sprintf(line,"Tertiary cache size: %dkb", CpuTertiaryCacheSize / 1024);
            }
            
            sprintf(line,"CPU: %s @ %d MHz",md_cpuname(),tgt_pipefreq()/1000000);
            w_text(2,7,WA_RIGHT,line);
            sprintf (line,"Memory size: %3d MB", (memorysize+memorysize_high)/0x100000);
            w_bigtext(2,8,pwinfo->l_window_width-7,2,line);
#endif
            return 0;
}

int do_boot_tab(p_window_info_t pwinfo,char *phint)
{
    int runstat = 0;
    com_counts = 6;

    #ifdef  CHINESE
            w_window(1,3,pwinfo->l_window_width,pwinfo->l_window_height,"��������ѡ��");
            w_text(2,4,WA_RIGHT," ");
            w_selectinput(24,5,24,"�����ں��ļ����ڵ��豸:",f1,sibuf[0],10);
            if(w_focused()) {
                sprintf(phint,"�����ں��ļ����ڵ��豸:wd0,wd1,usb0,usb1,tftp.\nʹ��<�س�>�����л�ѡ��; �����޸���ֱ����������.");
            }
            w_input(24,6,24,"�����ں��ļ�·��      :",w1[0],50);
            if(w_focused()) {
                sprintf(phint,"ֱ�������ں��ļ�·��");
            }
            w_selectinput(24,7,24,"�����ļ�ϵͳ����      :",f1b,sibuf[1],20);
            if(w_focused()) {
                sprintf(phint,"�����ں��ļ����ڵ��ļ�ϵͳ������(ext2��fat).\nʹ��<�س�>�����л�ѡ��; �����޸���ֱ����������.");
            }
            w_input(24,8,24,"������������          :",w1[1],50);
            if(w_focused()) {
                sprintf(phint,"�����ں������Ĳ���. ֱ�����뼴��.");
            }
            w_input(24,9,24,"����TFTP����IP(��ѡ)  :",w1[2],50);
            if(w_focused()) {
                sprintf(phint,"����IP. ֱ�����뼴��.ע:��TFTP������װ���ں��ļ�ʱ��,�����ѡ�����.");
            }
            if(w_button(22,10,10,"[����ϵͳ]")) {
                sprintf(phint,"");
                if(!strncmp(sibuf[0], "tftp", 4)) {//tftp boot
                    sprintf(line,"load tftp://%s%s",w1[2],w1[0]);
                    runstat = run(line); 
                }  else { //local boot               
                    if(!strncmp(sibuf[1],"fat",3)){//fat file                    
                        sprintf(line,"load /dev/fat/disk@%s%s",sibuf[0],w1[0]);
                        runstat = run(line);
                    } else{ //ext2 file                    
                        sprintf(line,"load /dev/fs/ext2@%s%s",sibuf[0],w1[0]);
                        runstat = run(line);
                    }
                }
                if (runstat == 0) {
                    sprintf(line,"g %s",w1[1]);
                    run(line);  
                }
                message = "����ʧ��"; 
                w_setpage(NOTE_WINDOW_ID);
            }
            if(w_focused()) {
                sprintf(phint,"�����������ں�.");
            }         
#else
            w_window(1,3,pwinfo->l_window_width,pwinfo->l_window_height,"Select boot option");
            w_selectinput(20,5,20,"Set disk         :",f1,sibuf[0],20);
            if(w_focused()) {
                sprintf(phint,"Set the disk where kernel file resides.Use   <Enter> to switch, other keys to modify.");
            }
            w_input(20,6,20,"Set kernel path  :",w1[0],50);
            if(w_focused()) {
                sprintf(phint,"Set kernel path. Just input the path in the  textbox");
            }
            w_selectinput(20,7,20,"Set file type    :",f1b,sibuf[1],20);
            if(w_focused()) {
                sprintf(phint,"Set the file type of kernel file(ext2 or fat).Use <Enter> to switch, other keys to modify.");
            }
            w_input(20,8,20,"Set karg         :",w1[1],50);
            if(w_focused()) {
                sprintf(phint,"Set karg which is to be passed to kernel.Just input the karg in the textbox.");
            }
            w_input(20,9,20,"Set ip(optional) :",w1[2],50);
            if(w_focused()) {
                sprintf(phint,"Set TFTP server ip if kernel file is loaded  from TFTP server. Just input the TFTP server ip in the textbox.");
            }
            if(w_button(20,10,10,"[BOOT NOW]")) {
                sprintf(phint,"");
                if(!strncmp(sibuf[0], "tftp", 4)){//tftp boot                
                    sprintf(line,"load tftp://%s%s",w1[2],w1[0]);
                    runstat = run(line); 
                }
                else  {//local boot               
                    if(!strncmp(sibuf[1],"fat",3)){//fat file                    
                        sprintf(line,"load /dev/fat/disk@%s%s",sibuf[0],w1[0]);
                        runstat = run(line); 
                    }
                    else{ //ext2 file                    
                        sprintf(line,"load /dev/fs/ext2@%s%s",sibuf[0],w1[0]);
                        runstat = run(line); 
                    }
                }
                if (runstat == 0){                
                    sprintf(line,"g %s",w1[1]);
                    run(line); 
                }
                message = "Boot failed"; 
                w_setpage(NOTE_WINDOW_ID);
            }
            if(w_focused()){            
                sprintf(phint,"Booting the kernel.");
            }   
#endif
            return  0;
}

int do_net_tab(p_window_info_t pwinfo,char *phint)
{
    com_counts = 6;

    sprintf(phint,"");
#ifdef  CHINESE
            w_window(1,3,pwinfo->l_window_width,pwinfo->l_window_height,"�޸���������");
            w_text(2,4,WA_RIGHT,"Ϊ��ǰϵͳ����IP��ַ");
            w_selectinput(17,5,20,"ѡ������  :",f2,sibuf[2],20);
            if(w_focused()) {
                sprintf(phint,"ѡ��:rtl0,rtk0,em0,em1,fxp0.\nʹ��<�س�>�����л�ѡ��; �����޸���ֱ����������");
            }
            w_input(17,6,20,"�µ�IP��ַ:",w2[0],50);
            if(w_focused()) {
                sprintf(phint,"ֱ�������µ�IP��ַ.");
            }
            if(w_button(19,7,10,"[ȷ��]")){
                   if( ifconfig(sibuf[2],w2[0])!=0){//configure
                        sprintf(line,"��[%s]�����óɹ�",sibuf[2]);
                    }else{
                        sprintf(line,"��[%s]������ʧ��",sibuf[2]);
                    }
                    message = line;
                    w_setpage(NOTE_WINDOW_ID);
            }
            w_text(2,9,WA_RIGHT,"�޸��������ò����浽CMOS");
            w_selectinput(17,10,20,"ѡ������  :",f2,sibuf[3],20);
            if(w_focused()) {
                sprintf(phint,"ѡ��:rtl0,rtk0,em0,em1,fxp0.\nʹ��<�س�>�����л�ѡ��; �����޸���ֱ����������");
            }
            w_input(17,11,20,"�µ�IP��ַ:",w2[1],50);
            if(w_focused()) {
                sprintf(phint,"ֱ�������µ�IP��ַ.");
            }
            if(w_button(19,12,10,"[ȷ��]")){
                sprintf(line,"set ifconfig %s:%s",sibuf[3],w2[1]);
                printf(line);
                if(run(line)==0){//configure
                    sprintf(line,"��[%s]�� ���óɹ�",sibuf[3]);
                }else{
                    sprintf(line,"��[%s]�� ����ʧ��",sibuf[3]);
                }
                message = line;
                w_setpage(NOTE_WINDOW_ID);
            }
#else
            w_window(1,3,pwinfo->l_window_width,pwinfo->l_window_height,"Modify the Network configuration");
            w_text(2,4,WA_RIGHT,"Set IP for current system");
            w_selectinput(17,5,20,"Select IC      ",f2,sibuf[2],20);
            if(w_focused()) {
                sprintf(phint,"Option:rtl0,rtk0,em0,em1,fxp0.Press Enter to Switch, other keys to modify");
            }
            w_input(17,6,20,"New IP Address:",w2[0],50);
            if(w_focused()) {
                sprintf(phint,"Please input new IP in the textbox.");
            }
            if(w_button(19,7,10,"[Set IP]")){
                if(ifconfig(sibuf[2],w2[0])!=0){//configure       
                    sprintf(line,"The device [%s] now has a new IP",sibuf[2]);
                }else{
                    sprintf(line,"Set new IP failed, input the correct IP and IC!");
                }

                message = line;
                w_setpage(NOTE_WINDOW_ID);
            }
            w_text(2,9,WA_RIGHT,"Set IP for current system and save it to CMOS");
            w_selectinput(17,10,20,"Select IC      ",f2,sibuf[3],20);
            if(w_focused()) {
                sprintf(phint,"Option:rtl0,rtk0,em0,em1,fxp0.Press Enter to Switch, other keys to modify");
            }
            w_input(17,11,20,"New IP Address:",w2[1],50);
            if(w_focused()) {
                sprintf(phint,"Please input new IP in the textbox.");
            }
            if(w_button(19,12,10,"[Set IP]")){
                sprintf(line,"set ifconfig %s:%s",sibuf[3],w2[1]);
                printf(line);
                if(run(line)==0){//configure
                    sprintf(line,"The device [%s] now has a new IP",sibuf[3]);
                }else{
                    sprintf(line,"Set new IP failed, input the correct IP and IC!");
                }
                message = line;
                w_setpage(NOTE_WINDOW_ID);
            }
#endif
            return 0;
}

int do_exit_tab(p_window_info_t pwinfo, char *phint)
{
    com_counts = 4;

    sprintf(phint,"");
#ifdef  CHINESE
            w_window(1,3,pwinfo->l_window_width,pwinfo->l_window_height,"�������� ��/�� ��������ϵͳ");
            if(w_button(3,4,20,"[ �ػ� ]")){
                w_setpage(SHUTDOWN_WARN_WINDOW_ID);
            }
            if(w_focused()){
                sprintf(phint,"����<�س�>ȷ��");
            }
            if(w_button(3,5,20,"[ ��������ϵͳ ]")) {
                w_setpage(RESTART_WARN_WINDOW_ID);
            }
            if(w_focused()){
                sprintf(phint,"����<�س�>ȷ��");
            }
            if(w_button(3,6,20,"[ ִ������ ]")){
                w_setpage(COMMAND_WINDOW_ID);       
            }
            if(w_focused()){
                sprintf(phint,"����<�س�>����PMON������ִ��ҳ��");
            }
            if(w_button(3,7,20,"  [ ����PMON ]  ")) {
                w_enterconsole();
                return(0);
            }
            if(w_focused()){
                sprintf(phint,"<�س�>������PMON�����н���");
            }
#else
            w_window(1,3,pwinfo->l_window_width,pwinfo->l_window_height,"Save configuration and/or Restart the system");
            if(w_button(3,5,20,"[ Power off ]")){
                w_setpage(SHUTDOWN_WARN_WINDOW_ID);
            }
            if(w_focused()){
                sprintf(phint,"<Enter> to power off.");
            }
            if(w_button(3,6,20,"[ Restart the system ]")) {
                w_setpage(RESTART_WARN_WINDOW_ID);
            }
            if(w_focused()){
                sprintf(phint,"<Enter> to restart system.");
            }
            if(w_button(3,7,20,"[ Run Command ]")){
                w_setpage(COMMAND_WINDOW_ID);       
            }
            if(w_focused()){
                sprintf(phint,"<Enter> to run PMON commond");
            }
            if(w_button(3,8,20,"  [ Return to PMON ]  "))  {
                w_enterconsole();
                return(-1);
            }
            if(w_focused()){
                sprintf(phint,"<Enter> to PMON console.");
            }
#endif
            return 0;
}

int do_shutdown_warn_window(int oldwindow)
{
    com_counts = 2;
    w_setcolor(0x60,0x00,0x60);
    #ifdef  CHINESE
            w_window(screen_width/2-25,8,50,8,"����");
            w_text(screen_width/2,10,WA_CENTRE,"�Ƿ�ػ�?");
            if(w_button(screen_width/2-4,12,8,"[ �� ]")){
                w_setpage(SHUTDOWN_ID);
            }
            if(w_button(screen_width/2-4,14,8,"[ �� ]")){
                w_setpage(oldwindow);
            }
#else
            w_window(screen_width/2-25,8,50,8,"WARRNING");
            w_text(screen_width/2,10,WA_CENTRE,"Are you sure to power off?");
            if(w_button(screen_width/2-4,12,8,"[ YES ]")){
                w_setpage(SHUTDOWN_ID);
            }
            if(w_button(screen_width/2-4,14,8,"[ NO  ]")){
                w_setpage(oldwindow);
            }
#endif    
            return 0;
}

int do_restart_warn_window(int oldwindow)
{
    com_counts = 2;
    w_setcolor(0x60,0x00,0x60);
#ifdef  CHINESE
            w_window(screen_width/2-25,8,50,8,"����");
            w_text(screen_width/2,10,WA_CENTRE,"�Ƿ���������ϵͳ?");
            if(w_button(screen_width/2-4,12,8,"[ �� ]")){
                w_setpage(REBOOT_ID);
            }
            if(w_button(screen_width/2-4,14,8,"[ �� ]")){
                w_setpage(oldwindow);
            }
#else
            w_window(screen_width/2-25,8,50,8,"WARRNING");
            w_text(screen_width/2,10,WA_CENTRE,"Are you sure to restart the system?");
            if(w_button(screen_width/2-4,12,8,"[ YES ]")){
                w_setpage(REBOOT_ID);
            }
            if(w_button(screen_width/2-4,14,8,"[ NO  ]")){
                w_setpage(oldwindow);
            }
#endif    
            return 0;
}

int do_datetime_window(p_window_info_t pwinfo, char *phint, struct tm *p_tm,int oldwindow)
 { 
     char line[100];
     int i;  
     com_counts = 7;

#ifdef  CHINESE
            w_window(1,3,pwinfo->l_window_width,pwinfo->l_window_height,"����ʱ�������");

#else
            w_window(1,3,pwinfo->l_window_width,pwinfo->l_window_height,"Modify Time and Date");
#endif             
            for(i=0;i<6;i++){
                if(w_input(daytime[i].x+8,daytime[i].y,6,daytime[i].name,daytime[i].buf,daytime[i].buflen))  {
                    sprintf(line,"date %04s%02s%02s%02s%02s.%02s",
                            daytime[0].buf,daytime[1].buf,daytime[2].buf,daytime[3].buf,daytime[4].buf,daytime[5].buf);
                            run(line);
                    w_setfocusid(w_getfocusid()+1);                    
                }
                if(w_focused()) {
#ifdef CHINESE
                    sprintf(line,"ֱ���������ָ���%s.\n����<�س�>��ȷ��.",daytime[i].name);
#else
                    sprintf(line,"input %s, <Enter> to confirm.",daytime[i].name);
#endif
                    sprintf(phint,"%s",line);
                }  else{
                    sprintf(daytime[i].buf,"%d",((int *)(p_tm))[5-i]+daytime[i].base);
                }
            }   
#ifdef CHINESE
            if(w_button(14,16,22,"[ ���� ]")){
                w_setpage(oldwindow);
            }
            if(w_focused()){
                sprintf(phint,"����<�س�>������һ����. ���·�������޸�ʱ�������");
            }
#else
            if(w_button(14,16,22,"[ Return ]")){
                w_setpage(oldwindow);
            }
            if(w_focused()){
                sprintf(phint,"<Enter> to return. Up Arrow key to modify");
            }
#endif
            return 0;
}

int do_note_window(int oldwindow)
{
        com_counts = 1;
        w_setcolor(0x60,0x00,0x60);
#ifdef CHINESE
            w_window(screen_width/2-25,7,50,8,"ע��");
            w_text(screen_width/2,9,WA_CENTRE,message);
            w_text(screen_width/2,11,WA_CENTRE,"����<�س�>������");
            if(w_button(screen_width/2-6,13,12,"[����]")){
                w_setpage(oldwindow);
            }
#else
            w_window(screen_width/2-25,7,50,8,"Notice");
            w_text(screen_width/2,9,WA_CENTRE,message);
            w_text(screen_width/2,11,WA_CENTRE,"Press Enter to return");
            if(w_button(screen_width/2-6,13,12,"[Return]")){
                w_setpage(oldwindow);
            }
#endif
            return 0;
}

int do_command_window(p_window_info_t pwinfo, char *phint, char *ptinput, int oldwindow)
{
    com_counts = 2;
    w_setcolor(0x10,0x10,0x60);
#ifdef CHINESE
            w_window(1,3,pwinfo->l_window_width,pwinfo->l_window_height,"ִ������");
            w_setcolor(0x60,0x00,0x60);
            if(w_biginput(pwinfo->l_window_width/6,5,pwinfo->l_window_width/3*2,10,"��������",ptinput,256)){
                w_setpage(RUN_COMMAND_ID);
            }
            if(w_focused()){
                sprintf(phint,"��������, ����<�س�>ִ������");
            }
            w_setcolor(0x10,0x10,0x60);
            if(w_button(pwinfo->l_window_width/2-5,17,10,"  [ ���� ]  ")){
                w_setpage(oldwindow);
            }
            if(w_focused()){
                sprintf(phint,"����<�س�>������һ����.");
            }
#else
            w_window(1,3,pwinfo->l_window_width,pwinfo->l_window_height,"Run Command");
            w_setcolor(0x60,0x00,0x60);
            if(w_biginput(pwinfo->l_window_width/6,5,pwinfo->l_window_width/3*2,10,"Input Command line",ptinput,256)){
                w_setpage(RUN_COMMAND_ID);
            }
            if(w_focused()){
                sprintf(phint,"Input commad line in the textbox. <Enter> to run");
            }
            w_setcolor(0x10,0x10,0x60);
            if(w_button(pwinfo->l_window_width/2-5,17,10,"  [ Return ]  ")){
                w_setpage(oldwindow);
            }
            if(w_focused()){
                sprintf(phint,"<Enter> to return. Up Arrow key to Input command line");
            }
#endif
return 0;
}

int do_run_command(char *ptinput)
{
    w_enterconsole();
    run(ptinput);
    w_hitanykey();
    w_leaveconsole();
    w_setpage(COMMAND_WINDOW_ID);
    return 0;
}

int do_reboot()
{
    printf("��������ϵͳ.....");
    printf("Rebooting.....");
    tgt_reboot();
    return 0;
}

int do_shutdown()
{
    printf("���ڹػ�.....");
    printf("Power off.....");
    tgt_poweroff();       
    return 0;
}

void envstr_init(void)
{
    char *envstr;
    char *pstr;

    envstr = getenv("ifconfig");
    if(envstr != NULL) {
        pstr = strchr(envstr, ':');
        if (pstr != NULL) {
            pstr++;
            strcpy(w2[0],pstr);
            strcpy(w2[1],pstr);
        } else {
            strcpy(w2[0],"172.16.1.205");   
            strcpy(w2[1],"192.16.1.205");            
        }
    }
    else  {
        strcpy(w2[0],"172.16.1.205");   
        strcpy(w2[1],"192.16.1.205");            
    }

    envstr = getenv("al");
    if (envstr != NULL) {
        pstr = strchr(envstr, '@');
        envstr = strchr(pstr, '/');
        strcpy(w1[0],envstr);
    } else {
        strcpy(w1[0],"/vmlinux");
    }

    envstr = getenv("karg");

    if (envstr != NULL)  {
        strcpy(w1[1],envstr);
    } else {
        strcpy(w1[1],"console=tty root=/dev/hda1");
    }    

    strcpy(w1[2],"172.16.0.30");
    strcpy(sibuf[0],f1[0]);
    strcpy(sibuf[1],f1b[0]);
    strcpy(sibuf[2],f2[0]);
    strcpy(sibuf[3],f2[0]);

}

int page_update(p_window_info_t pwinfo,char *phint)
{
    int i;
    if(w_getpage()>=0 && w_getpage()<pwinfo->number_of_tabs)  {
        if(w_keydown(RIGHT_KEY_CODE)){//HOOK  keyboard right
            w_setpage((w_getpage()+1)%pwinfo->number_of_tabs);
        }
        if(w_keydown(LEFT_KEY_CODE)){ //HOOK keyboard left          
            if (w_getpage()>=1) {
                w_setpage((w_getpage()-1)%pwinfo->number_of_tabs);
            } else {
                w_setpage(pwinfo->number_of_tabs-1);
            }
        }
    }

    for(i=0;i<pwinfo->number_of_tabs;i++){
        if(w_getpage()==i){
            w_setcolor(0x60,0,0);
        } else {
            w_setcolor(0x10,0x00,0x00);
        }
        w_window(i*(screen_width/pwinfo->number_of_tabs),1,18,1,maintabs[i]);
    }

    w_setcolor(0x10,0x10,0x60);

    if((w_getpage() >=0 && w_getpage()<6) || (w_getpage() == COMMAND_WINDOW_ID)) {
#ifdef  CHINESE
        w_window(pwinfo->l_window_width+2,3,pwinfo->r_window_width,pwinfo->r_window_height,"��ʾ");
#else
        w_window(pwinfo->l_window_width+2,3,pwinfo->r_window_width,pwinfo->r_window_height,"Item Specific Help");
#endif
        w_bigtext(pwinfo->l_window_width+2,4,pwinfo->r_window_width,pwinfo->r_window_height-4,phint);
    }
}

void datetime_init(void)
{
    time_t t;
    struct tm *p_tm;
    int i;

    t = tgt_gettime();
    p_tm = localtime(&t);
    for(i=0;i<6;i++) {
        sprintf(daytime[i].buf,"%d",((int *)(p_tm))[5-i]+daytime[i].base); 
    }
}
int cmd_main __P((int, char *[]));
int cmd_main
(ac, av)
    int ac;
    char *av[];
{
    char hint[256];
    int oldwindow;//save the previous number of oldwindow
    char tinput[256];//input buffer1
    unsigned int a;
    time_t t;//current date and time
    struct tm tm;
    window_info_t window_info ;

    window_info.number_of_tabs = sizeof(maintabs)/sizeof(maintabs[0]);

    w_init();

    window_info.l_window_width = screen_width * 5/8;
    window_info.r_window_width = screen_width - window_info.l_window_width - 3;
    window_info.l_window_height = screen_height - 6;
    window_info.r_window_height = screen_height - 6;
    
    w_setpage(MAIN_TAB_ID);
    strcpy(tinput,"");
    envstr_init();
    datetime_init();

    while(1) {
        page_update(&window_info,hint);
        
        /* Call the tgt_gettime() funciton every 50 cycles to ensure that the bios time could update in time */
        if(a>800) {
            a=0;
             t = tgt_gettime();
             tm = *localtime(&t);
        }
        a++;
        switch(w_getpage())
        {
        case MAIN_TAB_ID://Main window.Also display basic information                   
            oldwindow = MAIN_TAB_ID;            
            do_main_tab(&window_info,hint,&tm);
            break;
        case BOOT_TAB_ID://Boot related functions
            oldwindow = BOOT_TAB_ID;            
            do_boot_tab(&window_info,hint);
        break;
        case NET_TAB_ID:
            oldwindow = NET_TAB_ID;            
            do_net_tab(&window_info,hint);
        break;
        case EXIT_TAB_ID://Save configuration and reboot the system
            oldwindow = EXIT_TAB_ID;            
            if(do_exit_tab(&window_info,hint) == -1)
                return 0;
        break;
        case SHUTDOWN_WARN_WINDOW_ID:           
            do_shutdown_warn_window(oldwindow);
        break;
        case RESTART_WARN_WINDOW_ID:
            do_restart_warn_window(oldwindow);
        break;
        case DATETIME_WINDOW_ID://Modify Time and Date
            oldwindow=MAIN_TAB_ID;
            do_datetime_window(&window_info,hint,&tm,oldwindow);
        break;
        case NOTE_WINDOW_ID:
            do_note_window(oldwindow);
        break;
        case COMMAND_WINDOW_ID://show run command window
            do_command_window(&window_info,hint,tinput,oldwindow);
            break;
        case RUN_COMMAND_ID://run command
            do_run_command(tinput);
            break;
        case REBOOT_ID:
            do_reboot();
            break;
        case SHUTDOWN_ID:
            do_shutdown();
            break;     
        }
        w_present();
    }

    return(0);
}
static const Cmd Cmds[] = {
    {"MainCmds"},
    {"main","",0,"Simulates the MAIN BIOS SETUP",cmd_main, 0, 99, CMD_REPEAT},
    {0, 0}
};
static void init_cmd __P((void)) __attribute__ ((constructor));
static void
init_cmd()
{
    cmdlist_expand(Cmds, 1);
}