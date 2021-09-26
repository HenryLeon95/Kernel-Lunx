#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <linux/hugetlb.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>


#define BUFSIZE 150

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brandon Antony Chitay Coutiño");
MODULE_DESCRIPTION("Porcentaje de uso de la memoria RAM");

struct sysinfo inf;
static int write_proc_file(struct seq_file * archivo,void *v){
     long total_memoria = 0;
     long memoria_libre = 0;
     long memoria_utilizada= 0;
     si_meminfo(&inf);
     total_memoria = inf.totalram * 4;
     memoria_libre = inf.freeram * 4;
     memoria_utilizada = total_memoria-memoria_libre;
     seq_printf(archivo, "{\"memory_usage\": %8lu }",(memoria_utilizada * 100)/total_memoria);
     return 0;
    
}
static int on_open(struct inode *inode, struct file *file){
    return single_open(file, write_proc_file,NULL);
}

static struct file_operations processes = 
{
    .open = on_open,
    .read = seq_read
};

static int __init on_start(void){
    proc_create("memory_usage",0,NULL,&processes);   
    return 0;
}

static void __exit on_exit(void){
    remove_proc_entry("memory_usage",NULL);
}

module_init(on_start);
module_exit(on_exit);