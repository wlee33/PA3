#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "elf.h"

//#define NUM_KEYS (8)


//#define NUM_PAGES (8)

extern char data[];  // defined by kernel.ld
pde_t *kpgdir;  // for use in scheduler()

void* pagevaddresses[NUM_KEYS][NUM_PAGES];
void* pagepaddresses[NUM_KEYS][NUM_PAGES];

int refcounts[NUM_KEYS];
int usedkeys[NUM_KEYS]; 
int pagecounts[NUM_KEYS];

// Set up CPU's kernel segment descriptors.
// Run once on entry on each CPU.
void
seginit(void)
{
  struct cpu *c;

  // Map "logical" addresses to virtual addresses using identity map.
  // Cannot share a CODE descriptor for both kernel and user
  // because it would have to have DPL_USR, but the CPU forbids
  // an interrupt from CPL=0 to DPL=3.
  c = &cpus[cpuid()];
  c->gdt[SEG_KCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, 0);
  c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
  c->gdt[SEG_UCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER);
  c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);
  lgdt(c->gdt, sizeof(c->gdt));
}

// Return the address of the PTE in page table pgdir
// that corresponds to virtual address va.  If alloc!=0,
// create any required page table pages.
static pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
  pde_t *pde;
  pte_t *pgtab;

  pde = &pgdir[PDX(va)];
  if(*pde & PTE_P){
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
  } else {
    if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
      return 0;
    // Make sure all those PTE_P bits are zero.
    memset(pgtab, 0, PGSIZE);
    // The permissions here are overly generous, but they can
    // be further restricted by the permissions in the page table
    // entries, if necessary.
    *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
  }
  return &pgtab[PTX(va)];
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned.
static int
mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
{
  char *a, *last;
  pte_t *pte;

  a = (char*)PGROUNDDOWN((uint)va);
  last = (char*)PGROUNDDOWN(((uint)va) + size - 1);
  for(;;){
    if((pte = walkpgdir(pgdir, a, 1)) == 0)
      return -1;
    if(*pte & PTE_P)
      panic("remap");
    *pte = pa | perm | PTE_P;
    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}

// There is one page table per process, plus one that's used when
// a CPU is not running any process (kpgdir). The kernel uses the
// current process's page table during system calls and interrupts;
// page protection bits prevent user code from using the kernel's
// mappings.
//
// setupkvm() and exec() set up every page table like this:
//
//   0..KERNBASE: user memory (text+data+stack+heap), mapped to
//                phys memory allocated by the kernel
//   KERNBASE..KERNBASE+EXTMEM: mapped to 0..EXTMEM (for I/O space)
//   KERNBASE+EXTMEM..data: mapped to EXTMEM..V2P(data)
//                for the kernel's instructions and r/o data
//   data..KERNBASE+PHYSTOP: mapped to V2P(data)..PHYSTOP,
//                                  rw data + free physical memory
//   0xfe000000..0: mapped direct (devices such as ioapic)
//
// The kernel allocates physical memory for its heap and for user memory
// between V2P(end) and the end of physical memory (PHYSTOP)
// (directly addressable from end..P2V(PHYSTOP)).

// This table defines the kernel's mappings, which are present in
// every process's page table.
static struct kmap {
  void *virt;
  uint phys_start;
  uint phys_end;
  int perm;
} kmap[] = {
 { (void*)KERNBASE, 0,             EXTMEM,    PTE_W}, // I/O space
 { (void*)KERNLINK, V2P(KERNLINK), V2P(data), 0},     // kern text+rodata
 { (void*)data,     V2P(data),     PHYSTOP,   PTE_W}, // kern data+memory
 { (void*)DEVSPACE, DEVSPACE,      0,         PTE_W}, // more devices
};

// Set up kernel part of a page table.
pde_t*
setupkvm(void)
{
  pde_t *pgdir;
  struct kmap *k;

  if((pgdir = (pde_t*)kalloc()) == 0)
    return 0;
  memset(pgdir, 0, PGSIZE);
  if (P2V(PHYSTOP) > (void*)DEVSPACE)
    panic("PHYSTOP too high");
  for(k = kmap; k < &kmap[NELEM(kmap)]; k++)
    if(mappages(pgdir, k->virt, k->phys_end - k->phys_start,
                (uint)k->phys_start, k->perm) < 0) {
      freevm(pgdir);
      return 0;
    }
  return pgdir;
}

// Allocate one page table for the machine for the kernel address
// space for scheduler processes.
void
kvmalloc(void)
{
  kpgdir = setupkvm();
  switchkvm();
}

// Switch h/w page table register to the kernel-only page table,
// for when no process is running.
void
switchkvm(void)
{
  lcr3(V2P(kpgdir));   // switch to the kernel page table
}

// Switch TSS and h/w page table to correspond to process p.
void
switchuvm(struct proc *p)
{
  if(p == 0)
    panic("switchuvm: no process");
  if(p->kstack == 0)
    panic("switchuvm: no kstack");
  if(p->pgdir == 0)
    panic("switchuvm: no pgdir");

  pushcli();
  mycpu()->gdt[SEG_TSS] = SEG16(STS_T32A, &mycpu()->ts,
                                sizeof(mycpu()->ts)-1, 0);
  mycpu()->gdt[SEG_TSS].s = 0;
  mycpu()->ts.ss0 = SEG_KDATA << 3;
  mycpu()->ts.esp0 = (uint)p->kstack + KSTACKSIZE;
  // setting IOPL=0 in eflags *and* iomb beyond the tss segment limit
  // forbids I/O instructions (e.g., inb and outb) from user space
  mycpu()->ts.iomb = (ushort) 0xFFFF;
  ltr(SEG_TSS << 3);
  lcr3(V2P(p->pgdir));  // switch to process's address space
  popcli();
}

// Load the initcode into address 0 of pgdir.
// sz must be less than a page.
void
inituvm(pde_t *pgdir, char *init, uint sz)
{
  char *mem;

  if(sz >= PGSIZE)
    panic("inituvm: more than a page");
  mem = kalloc();
  memset(mem, 0, PGSIZE);
  mappages(pgdir, 0, PGSIZE, V2P(mem), PTE_W|PTE_U);
  memmove(mem, init, sz);
}

// Load a program segment into pgdir.  addr must be page-aligned
// and the pages from addr to addr+sz must already be mapped.
int
loaduvm(pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz)
{
  uint i, pa, n;
  pte_t *pte;

  if((uint) addr % PGSIZE != 0)
    panic("loaduvm: addr must be page aligned");
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walkpgdir(pgdir, addr+i, 0)) == 0)
      panic("loaduvm: address should exist");
    pa = PTE_ADDR(*pte);
    if(sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    if(readi(ip, P2V(pa), offset+i, n) != n)
      return -1;
  }
  return 0;
}

// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
int
allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  char *mem;
  uint a;

  if(newsz >= KERNBASE)
    return 0;
  if(newsz < oldsz)
    return oldsz;

  a = PGROUNDUP(oldsz);
  for(; a < newsz; a += PGSIZE){
    mem = kalloc();
    if(mem == 0){
      cprintf("allocuvm out of memory\n");
      deallocuvm(pgdir, newsz, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    if(mappages(pgdir, (char*)a, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
      cprintf("allocuvm out of memory (2)\n");
      deallocuvm(pgdir, newsz, oldsz);
      kfree(mem);
      return 0;
    }
  }
  return newsz;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
int
deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  pte_t *pte;
  uint a, pa;

  if(newsz >= oldsz)
    return oldsz;

  a = PGROUNDUP(newsz);
  for(; a  < oldsz; a += PGSIZE){
    pte = walkpgdir(pgdir, (char*)a, 0);
    if(!pte)
      a = PGADDR(PDX(a) + 1, 0, 0) - PGSIZE;
    else if((*pte & PTE_P) != 0){
      pa = PTE_ADDR(*pte);
      if(pa == 0){
      	cprintf("dealloc panic\n");
	panic("kfree");
      
      }
      char *v = P2V(pa);
      int i, j;
      for(i=0;i<NUM_KEYS;i++){
      	for(j=0; j<NUM_PAGES; j++){
		if( (uint)pagepaddresses[i][j] == pa ){
			goto outside_of_loop;
		}
	}
      }
outside_of_loop:
      if(i == NUM_KEYS && j==NUM_PAGES){
      	kfree(v);
      }      
      *pte = 0;
    }
  }
  return newsz;
}

// Free a page table and all the physical memory pages
// in the user part.
void
freevm(pde_t *pgdir)
{
  uint i,a;
  uint b=0;

  if(pgdir == 0)
    panic("freevm: no pgdir");
  deallocuvm(pgdir, KERNBASE, 0);
  
  
  for(a=0; a < NUM_KEYS; a++)
  {
  	if(myproc()->keys[a] == 1)
		refcounts[a]--; //reduce the reference count on each shared page for this process
  }

  for(i = 0; i < NPDENTRIES; i++){ //for each page directory entry (Start PD Entry Loop)
    if(pgdir[i] & PTE_P){
      char * v = P2V(PTE_ADDR(pgdir[i]));
      
      for(a=0; a<NUM_KEYS; a++) //For each key
      {
      	if(refcounts[i] != 0)//page being used elsewhere, don't free it
		        break;
      
	      for(b=0; b <NUM_PAGES; b++){//for each page
          if ((char*)PTE_ADDR(pgdir[i]) == pagepaddresses[i][b]) //if address at pgdir[i] is in the stored list of 
			 					       //shared physical page addresses
            break;}
      }
      if(a==NUM_KEYS && b==NUM_PAGES)
      	kfree(v); 
      
    
      //only free page if it wasnt found to be used as shared mem elsewhere (both key and page loops ran through without breaking)

    }
  }
  //end PD Entry loop
  kfree((char*)pgdir);//free pgdir itself
}
  

// Clear PTE_U on a page. Used to create an inaccessible
// page beneath the user stack.
void
clearpteu(pde_t *pgdir, char *uva)
{
  pte_t *pte;

  pte = walkpgdir(pgdir, uva, 0);
  if(pte == 0)
    panic("clearpteu");
  *pte &= ~PTE_U;
}

// Given a parent process's page table, create a copy
// of it for a child.
pde_t*
copyuvm(pde_t *pgdir, uint sz)
{
  pde_t *d;
  pte_t *pte;
  uint pa, i, flags;
  char *mem;

  if((d = setupkvm()) == 0)
    return 0;
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
      panic("copyuvm: pte should exist");
    if(!(*pte & PTE_P))
      panic("copyuvm: page not present");
    pa = PTE_ADDR(*pte);
    flags = PTE_FLAGS(*pte);
    if((mem = kalloc()) == 0)
      goto bad;
    memmove(mem, (char*)P2V(pa), PGSIZE);
    if(mappages(d, (void*)i, PGSIZE, V2P(mem), flags) < 0) {
      kfree(mem);
      goto bad;
    }
  }
  return d;

bad:
  freevm(d);
  return 0;
}

//PAGEBREAK!
// Map user virtual address to kernel address.
char*
uva2ka(pde_t *pgdir, char *uva)
{
  pte_t *pte;

  pte = walkpgdir(pgdir, uva, 0);
  if((*pte & PTE_P) == 0)
    return 0;
  if((*pte & PTE_U) == 0)
    return 0;
  return (char*)P2V(PTE_ADDR(*pte));
}

// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for PTE_U pages.
int
copyout(pde_t *pgdir, uint va, void *p, uint len)
{
  char *buf, *pa0;
  uint n, va0;

  buf = (char*)p;
  while(len > 0){
    va0 = (uint)PGROUNDDOWN(va);
    pa0 = uva2ka(pgdir, (char*)va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (va - va0);
    if(n > len)
      n = len;
    memmove(pa0 + (va - va0), buf, n);
    len -= n;
    buf += n;
    va = va0 + PGSIZE;
  }
  return 0;
}

void*
sharedmempage(int key, int numPages, struct proc* proc)
{
	void* address= 0x0;
  int firstCall = 1; //flag for calling for the first time
	

	//check parameters are valid
  if(key<0||key >NUM_KEYS)
    return (void*)-1;
  
  if(numPages<0||numPages>NUM_PAGES)
    return (void*)-1;

  int a;
	//if first call for a process, start at the top of the process' VA space
  for(a=0;a<NUM_KEYS;a++){
    if(proc->keys[a]==1)
      firstCall = 0;
  }

  if(firstCall)
    proc->top = KERNBASE;


  
	//update the reference count of the shared page if the calling process has not already used this shared page
  if(proc->keys[key]==0){
    proc->keys[key]=1;//and set this key to used for this process
    //cprintf("key set to used with %d\n",proc->keys[key]);
    refcounts[key]++;
  } 

	//allocate memory if key hasn't been used before (kalloc();)
  if(usedkeys[key]==0){
    //first, varify that we are not accessing already allocated memory. It so, throw an error
    if ((proc->top - numPages*PGSIZE) < proc->sz) {
      return (void*)-1;
    }
    //start to allocate physical memory
    char* memory;
    int page;
    for(page=0; page < numPages; page++){ //for each requested page
      memory = kalloc(); //grab physical memory page 
      if(memory==0) {
      	cprintf("All Out Of Memory!\n");
      	return (void*)-1; //throw error
      }
      //set physical page contents to 0 (memcpy)
      memset(memory,0,PGSIZE);
      //store the reference to the physical page
      pagepaddresses[key][page] = memory;
      
      cprintf("PA stored in key %d is %x\n", key, memory);
      
      //change the address of the next avalible virtual page in the calling process' address space ie (oldtop-pagesize*numPages)
      address = (void*)(proc->top-PGSIZE);

      proc->page_va_addr[key][page] = address;
      
      proc->top -= PGSIZE;

      pagevaddresses[key][page] = address;
      //map virtual page to physical page with mappages()
      if(mappages(proc->pgdir, address, PGSIZE, (uint)(memory), PTE_P|PTE_W|PTE_U)<0){
        cprintf("mappages failed.");
        return (void*)-1; 
      }
    }

    //mark key as used in the usedkey array
    usedkeys[key] = 1;
    pagecounts[key] += numPages;
    //cprintf("first time key is used! page count for key %d is %d\n",key,pagecounts[key]);


  }else{ //key is being used by processes
    
    //first, check if the current process is using this key. If not, start mapping for each requested page
    if(firstCall){
      //cprintf("Key is being used by other processes, but it's the first time being called by the current process\n");
      int page;
      for(page=0;page<numPages;page++){

        //change the address of the next avalible virtual page in the calling process' address space 

        address = (void*)(proc->top-PGSIZE);
        //cprintf("address is: %x\n", address);
        proc->page_va_addr[key][page] = address;
        pagevaddresses[key][page] = address;

        //update the current user top
        proc->top -= PGSIZE;

        //cprintf("proc->top is now: %x\n", proc->top);
        
        //cprintf("PA in key %d is %d\n",key, (uint)pagepaddresses[key][page]);
        //map virtual page to physical page with mappages()
        if(mappages(proc->pgdir, address, PGSIZE, (uint)pagepaddresses[key][page]  , PTE_P|PTE_W|PTE_U)<0){
          cprintf("mappages failed.");
          return (void*)-1; 
        }

      }
      pagecounts[key] += numPages;
    }else
    {
      //if this process has already requested with this key, simply return the address stored in proc->page_va_addr[][]
     //forked process comes in here
      if(numPages == pagecounts[key]){
        address = pagevaddresses[key][numPages];
        //cprintf( "Mapping with stored PA %d\n",pagepaddresses[key][numPages-1]);
        if(mappages(proc->pgdir, address, PGSIZE, (uint)pagepaddresses[key][numPages-1], PTE_P|PTE_W|PTE_U)<0){
          cprintf("mappages failed.");
          return (void*)-1; 
        }
        
        cprintf("Current process has requested with the same key before\n");

      }else{
        cprintf("Number of pages requested is too high\n");
        address = pagevaddresses[key][numPages];
      }
      
      //pagecounts[key] += numPages;
    }
    
    
    
  }
	
	cprintf("physical address of mem = %x", pagepaddresses[key][numPages-1]);	
	return address;
}

void
sharedmeminit()
{
	//initialize all of the support storage structures to 0
	//callme in main.c as OS boots
	int i;
	for(i=0; i<NUM_KEYS; i++){
	
		refcounts[i]=0;
		usedkeys[i]=0;
		pagecounts[i]=0;
	
	}
	int q;
	for(i=0; i<NUM_PAGES; i++)
	{
		for(q=0; q<NUM_KEYS; q++)
		{
			pagepaddresses[i][q]=0;
      pagevaddresses[i][q]=0;

		}
	}	
	
	
	return;
}

void
freesharedpage(int key, struct proc* proc)
{
	//verify that the calling process has already called sharedmempage with that key & hasn't been removed already 
	//	(ie, is still mapped in the process' page_va_addr array)
 	//remove page table entry from calling process
	//decrement the reference
	//check if refcount == 0
	//if so, deallocate (kfree)
	
	

	int i;
	int called=0;
	int numPages = pagecounts[key];
	//for(i=0; i<NUM_KEYS; i++)
	//{
		if(proc->keys[key]==1)//if the key is in the calling process' list of keys, its has called sharedmempage before
		{
			called=1;

		}
		if(!called) 
		{
			cprintf("Calling process not associated with that key\n");
			return;
		} //if it hasn't called, nothing to free, return 
	//}
	for(i=0; i<numPages; i++){ //for each page associated with the key, 
		cprintf("In freeing pages loop of freesharedpage\n");	
		pte_t* pte = walkpgdir(proc->pgdir, proc->page_va_addr[key][i],0); //get the pte corropsponding to the VA for that key
		uint pa;
		cprintf("pte is: %p\n", pte);		
		//Do as is done in the deallocuvm, 
		
      		pa = PTE_ADDR(*pte); //convert pte to physical address
     		
		if(pa == 0){
                	cprintf("From freesharepage kfree panic\n"); 
			panic("kfree");
		}
		cprintf("Didn't panic inside freeing pages loop of freesharedpage\n");                
		char *v = P2V(pa); //convert physical address to virtual
	        
		kfree(v); //Free the page
                
		*pte = 0; //clear the page table entry
		
		//to test perhaps check if the process can still communicate via the shared page		
	
	}

	return;
}

//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.

