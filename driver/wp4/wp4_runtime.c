/*
Copyright 2020 Paul Zanna.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/skbuff.h>  
#include <linux/ftrace.h>

#include "wp4_runtime.h"

// length of the two memory areas
#define FTPAGES      16
#define PBPAGES      16 
#ifndef VM_RESERVED
# define  VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif

//  Local variables
struct wp4_map_def *flow_table;
struct pbuffer *pk_buffer;
struct dentry  *fileret, *dirret;
//struct mmap_info *op_info;

// original pointer for kmalloc'd area as returned by kmalloc
static void *flow_table_ptr;
static void *pk_buffer_ptr;

// Function declarations
void mmap_open(struct vm_area_struct *vma);
void mmap_close(struct vm_area_struct *vma);
static int mmap_mmap(struct file *filp, struct vm_area_struct *vma);
int mmap_kmem_flow_table(struct file *filp, struct vm_area_struct *vma);
int mmap_kmem_pk_buffer(struct file *filp, struct vm_area_struct *vma);

static int mmap_fault(struct vm_fault *vmf);

void dump_rx_packet(u8 *ptr)
{
    int i;
    printk("\n");
    printk("***********************************************\n");
    for (i = 0; i < 64; i = i + 16)
        printk("%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n", *(ptr + i),
        *(ptr + i + 1), *(ptr + i + 2) , *(ptr + i + 3) , *(ptr + i + 4), *(ptr + i + 5), *(ptr + i + 6), *(ptr + i + 7),
        *(ptr + i + 8), *(ptr + i + 9), *(ptr + i + 10) , *(ptr + i + 11) , *(ptr + i + 12), *(ptr + i + 13), *(ptr + i + 14), *(ptr + i + 15));
    printk("***********************************************\n");
    //flow_table->last_entry++;
    //printk("iLastFlow %d\n", flow_table->last_entry);
}


struct mmap_info {
    char *data;
    int reference;
};


// helper function, mmap's the kmalloc'd area which is physically contiguous
int mmap_kmem_flow_table(struct file *filp, struct vm_area_struct *vma)
{
    int ret;
    long length = vma->vm_end - vma->vm_start;

    /* check length - do not allow larger mappings than the number of pages allocated */
    if (length > FTPAGES * PAGE_SIZE) return -EIO;

    /* map the whole physically contiguous area in one piece */
    if ((ret = remap_pfn_range(vma, vma->vm_start, virt_to_phys((void *)flow_table) >> PAGE_SHIFT, length, vma->vm_page_prot)) < 0)
    {
        return ret;
    }
    
    printk("WP4: flow table - vma->vm_start = %lx , vma->vm_end = %lx , length = %ld\n",vma->vm_start, vma->vm_end, length);
    return 0;
}

int mmap_kmem_pk_buffer(struct file *filp, struct vm_area_struct *vma)
{
    int ret;
    long length = vma->vm_end - vma->vm_start;

    /* check length - do not allow larger mappings than the number of pages allocated */
    if (length > PBPAGES * PAGE_SIZE) return -EIO;

    /* set to not cache */
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    //vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
    
    /* map the whole physically contiguous area in one piece */
    if ((ret = remap_pfn_range(vma, vma->vm_start, virt_to_phys((void *)pk_buffer) >> PAGE_SHIFT, length, vma->vm_page_prot)) < 0)
    {
        return ret;
    }
    
    printk("WP4: pk buffer - vma->vm_start = %lx , vma->vm_end = %lx , length = %ld\n",vma->vm_start, vma->vm_end, length);
    return 0;
}

int mmapfop_close(struct inode *inode, struct file *filp)
{
	printk("WP4: mmap file closed\n");
    return 0;
}
 
int mmapfop_open(struct inode *inode, struct file *filp)
{
    printk("WP4: mmap file opened.\n");
    return 0;
}


void mmap_open(struct vm_area_struct *vma)
{
    struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
    info->reference++;
}

void mmap_close(struct vm_area_struct *vma)
{
    struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
    info->reference--;
}

static int mmap_fault(struct vm_fault *vmf)
{
    printk("WP4: mmap fault.\n");
    struct page *page;
    struct mmap_info *info;
    unsigned long address = (unsigned long)vmf->address;
    //if (address > vma->vm_end) {
    //    printk("WP4: mmap no data\n");
    //    return VM_FAULT_SIGBUS;
    //}
    page = virt_to_page(info->data);
    get_page(page);
    vmf->page = page;
    return 0;
}


static const struct file_operations mmap_fops = 
{
    .open = mmapfop_open,
    .release = mmapfop_close,
    .mmap = mmap_mmap,
    .owner = THIS_MODULE,
};


struct vm_operations_struct mmap_vm_ops =
{
    .open = mmap_open,
    .close = mmap_close,
    .fault = mmap_fault,
};


/* character device mmap method */
static int mmap_mmap(struct file *filp, struct vm_area_struct *vma)
{
    printk("WP4: Called mmap - offset = %ld.\n", vma->vm_pgoff);
    /* at offset 0 we map the kmalloc'd area */
    if (vma->vm_pgoff == 0)
    {
        return mmap_kmem_flow_table(filp, vma);
    }
    /* at offset FTPAGES we map the kmalloc'd area */
    if (vma->vm_pgoff == FTPAGES) {
        return mmap_kmem_pk_buffer(filp, vma);
    }

    /* at any other offset we return an error */
    return -EIO;
}

int table_init(void)
{
    /* allocate a memory area with kmalloc. Will be rounded up to a page boundary */
    if ((flow_table_ptr = kmalloc((FTPAGES + 2) * PAGE_SIZE, GFP_KERNEL)) == NULL) 
    {
        return -1;
    }
    /* round it up to the page bondary */
    flow_table = (struct wp4_map_def *)((((unsigned long)flow_table_ptr) + PAGE_SIZE - 1) & PAGE_MASK);
    printk("WP4: flow_table allocated at %p\n", (void*)flow_table);


    /* allocate a memory area with kmalloc. Will be rounded up to a page boundary */
    if ((pk_buffer_ptr = kmalloc((PBPAGES + 2) * PAGE_SIZE, GFP_KERNEL)) == NULL) 
    {
        return -1;
    }
    /* round it up to the page bondary */
    pk_buffer = (struct pbuffer *)((((unsigned long)pk_buffer_ptr) + PAGE_SIZE - 1) & PAGE_MASK);
    printk("WP4: pk_buffer allocated at %p\n", (void*)pk_buffer);


    /* mark the pages reserved */
    for (int i = 0; i < FTPAGES * PAGE_SIZE; i+= PAGE_SIZE)
    {
        SetPageReserved(virt_to_page(((unsigned long)flow_table) + i));
    }

    /* mark the pages reserved */
    for (int i = 0; i < PBPAGES * PAGE_SIZE; i+= PAGE_SIZE)
    {
        SetPageReserved(virt_to_page(((unsigned long)pk_buffer) + i));
    }
    
    memset(flow_table, 0, FTPAGES * PAGE_SIZE);
    memset(pk_buffer, 0, PBPAGES * PAGE_SIZE);

    proc_create("wp4_data", 0, NULL, &mmap_fops);

    return 0;
}

void table_exit(void)
{
    remove_proc_entry("wp4_data", NULL);
    return;
}

/*
 *  Packet poll request
 *
 *  @param skb - pointer to the packet buffer.
 *  @param dev - pointer to the device.
 *
 */
void to_cpu(struct Headers_t headers)
{
    int x;
    int buffer_no =-1;

    for(x=0;x<(PACKET_BUFFER);x++)
        {
            printk("WP4: Buffer %d is set as %d\n", x, pk_buffer->buffer[x].type);
            if(pk_buffer->buffer[x].type == PB_EMPTY) 
            {
                buffer_no = x;
                break;
            }
        }
        if (buffer_no == -1 ) 
        {
            printk("WP4: All buffer are full!\n");
            return;   // All buffers are full
        }
        memcpy(&pk_buffer->buffer[buffer_no].buffer, &headers, sizeof(struct Headers_t));
        pk_buffer->buffer[buffer_no].type = PB_PENDING;
        printk("WP4: Headers loaded into buffer %d \n", buffer_no);         
        return;
}
