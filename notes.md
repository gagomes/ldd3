Chapter 1
---------

_kernel/module.c_ implements the _init_module_ system call and other functions for loading and unloading kernel modules.

cscope -qkR

make TAGS

Chapter 2
---------

To load a kernel module, the kernel does a job similar to a linker, resolving symbols and adding the module's exported symbols to the kernel symbol table, all in memory. _modprobe_ uses information created by _depmod_ to resolve dependencies. _lsmod_ works by reading _/proc/modules_, which is the older, single-file version of _/sys/module/_. _modprobe -r_ also removes any unused modules on which the given module depends.

For each exported symbol, the EXPORT\_SYMBOL macro defined in _linux/module.h_ records information in the *__ksymtab* ELF section.

The kernel has a _vermagic_ mechanism to prevent loading an incompatible module built against a different kernel. _modinfo_ shows the vermagic string of a module by reading its _.modinfo_ ELF section. From _modprobe_'s man page, it can be used to force loading a module. For more information, see http://hychen.wuweig.org/?p=495 and http://blog.sina.com.cn/s/blog_602f87700100lg8m.html.

_linux/version.h_ has macros for version tests.

_linux/init.h_ has macros to mark functions/data as used only at initialization time or exit time. They work by placing the definitions in special ELF sections, to give hints to the kernel. The comments are worth reading too.

Module parameters can be passed at load time by _insmod_ or _modeprobe_, or better yet, by storing them in configuration file _/etc/modprobe.d/*.conf_ and the older _/etc/modprobe.conf_. See the man page for _modprobe.conf(5)_. Sysfs entries for the parameters can be created under _/sys/module/*/parameters/_ at parameter registration time. If created as writable, the parameters can be changed dynamically by writing to the files.

Kernel code cannot do floating point arithmetic, because saving and restoring the floating point processor's state on each entry and exit of kernel space is too expensive.

The _misc_modules/_ directory contains two example kernel modules _hello_ and _hellop_ (basically Hello World with parameter support), and a skeleton _Makefile_. The _Makefile_ uses a standard trick that locates the kernel source directory, and runs a __second__ make to invoke the kernel build system, which will helpfully move back to our module source directory and build our module. For more information see _Documentation/kbuild/_.

To add a device driver to the kernel source tree, the new files should be stored under a proper directory such as _drivers/char/_. For multiple files we create a directory, say _fishing_. We can make the driver's compilation contingent on a configuration option, say *CONFIG_FISHING_POLE*, by creating a new _Kconfig_ file under _drivers/char/fishing_, and source it from the parent directory, _drivers/char/Kconfig_. Next, we add a line to _drivers/char/Makefile_, *obj-$(CONFIG_FISHING_POLE) += fishing/* to tell the build system to descend into the subdirectory. In our Makefile, we have *obj-$(CONFIG_FISHING_POLE) += fishing.o* and *fishing-objs := fishing-main.o fishing-line.o*. Now, *fishing-main.c* and *fishing-line.c* will be compiled and linked into *fishing.ko*.

Chapter 3
---------

Important __struct__s: __file_operations__ __file__ __inode__ __cdev__

_/proc/devices_ lists all char and block device drivers currently registered. _Documentation/devices.txt_ has a detailed explanation for all statically assigned major numbers. Network devices don't quite qualify for the "everything is a file" philosphy, so they aren't accessed through _/dev/_. A network device receives packets asynchronously from the outside, and pushes them toward the kernel; while a block driver operates only in response to kernel requests.

The sample code provides a *scull_load* script that looks up _/proc/devices_ to find out the major number of the _scull_ driver, and _mknod_ the device nodes using its major number. In practice, _udev_ does the job automaticaly on modern kernels. See http://www.linuxfromscratch.org/lfs/view/development/chapter07/udev.html

The *\__user* annotation notes that a pointer is a user-space address, thereby allowing static analyzers to find bugs that directly dereference *\__user* pointers.

*fs/char_dev.c* provides the infrastructure for char drivers.

*scull_init_module()* registers the char driver, gets the major number, and allocates the *struct scull_dev* (which embeds the standard *struct cdev*) array for managing the devices. It then calls *scull_setup_cdev()* to init the embedded *struct cdev* and add it to the system for each driver.

*scull_open()* can optionaly update the *file->f_op* pointer to provide different implementations depending on the minor number, but still under the same major number. It assigns the *file->private_data* to our *struct scull_dev* metadata, as a way to preserve state information across system calls.

*scull_release()* doesn't free anything, because we want to preserve the memory after the user closes the device file. *scull_cleanup_module()* will return all the resources when the module is unloaded.

*copy_to_user()* and *copy_from_user()* return 0 if successful, and return the amount of memory still to be copied otherwise. *scull_read()* is free to transfer data less than requested for the sake of simplicity, and leave the job of retrying to the user. The *fread()* library function reissues the system call until completion of the requested data transfer. *scull_write()* also does partial transfer, although it should support the model of either failing or succeeding completely for compatibility.

Chapter 4
---------

This chapter starts with a list of configuration options in the kernel for debugging.

Next we learn about _printk_ and loglevel. We indicate the loglevel with a macro, which expands to a string (see _linux/printk.h_), and is prepended to the format string. The log prefix will be parsed and stripped by *log_prefix()* defined in _kernel/printk.c_. The kernel writes messages into a circular buffer which can be read using the _syslog_ system call or _/proc/kmsg_. Reading _/proc/kmsg_ consumes the kernel buffer, so it should be read by only one process, typically the log daemon. The log daemon (_syslog-ng_, _rsyslog_, _klogd/syslogd_, etc.) is configured to dispatch the kernel messages along with other logs according to their facility and priority. Kernel messages are typically found in _/var/log/messages_ or _/var/log/kernel.log_. _dmesg_ can be used to print and control the kernel buffer.

_printk.c_ defines four integers in the *console_printk* array, which is exposed to _/proc/sys/kernel/printk_. See man _proc(5)_ and _syslog(2)_ for their meaning. We are most interested in the first value, *console_loglevel*. If a printk loglevel is smaller (i.e. higher priority) than *console_loglevel*, the message is printed to the console. *console_loglevel* can be changed by writing to _/proc/sys/kernel/printk_, _/proc/sysrq-trigger_, or using _dmesg -n_.

If we are creating a read-only _/proc_ file and the file contains a small number of lines, we can use *create_proc_read_entry()* and supply a *read_proc*. Otherwise, we should call use the *seq_file* interface and call *create_proc_entry()*. See also http://kernelnewbies.org/Documents/SeqFileHowTo

The kernel creates _/proc/kcore_ that represents the running kernel in the format of a core file for gdb. With CONFIG\_DEBUG\_INFO option set, we can examine kernel variables using standard gdb commands. Because gdb caches data, we need to make gdb discard old information with the command *core-file /proc/kcore*. To tell gdb about modules, issue the *add-symbol-file* command using the helper program *misc-progs/gdbline*. One trick is _print *(address)_ that prints the file name and line number for the code corresponding to that address or function pointer.

*WARN_ON()*, *BUG_ON()*, and *dump_stack()* are also very useful.

Chapter 5
---------

__Atomic integers__: *atomic_t*. Very efficient. On old SPARC implementation it's only 24 bits wide.

__Atomic bit operations__: _asm/bitops.h_.

__Spin locks__: *spinlock_t*. To make sure spinlocks are not held for a long time, kernel preemption is disabled on the processor that holds the spinlock, which is enough on uniprocessor machines. Code should never sleep while holding a spinlock. If a thread attempts to acquire a spinlock it already held, it will deadlock. Any code that shares locks with interrupt handlers must disable interrupts before obtaining spinlocks. Likewise, if data is shared between a process context and a bottom half, we must disable bottom halves, but can leave interrupts enabled. Because two tasklets of the same type never run simultaneously, there is no need to protect data used only within a single type of tasklet. If data is shared between two different tasklets, we must obtain a spinlock, but do not need to disable bottom halves because a tasklet never preempts another running tasklet on the same processor. With softirqs, spinlock is needed but still, disabling bottom halves is not.

__Reader-writer spin locks__: *rwlock_t*.

__Semaphores__: *semaphore*. They sleep and therefore cannot be used in interrupt context. Usually we prefer *down_interruptible()* to *down()* because it allows a task to be awakened with a signal from user space. If the sleep is interrupted by a signal, *down_interruptible* will return *-EINTR*. If the semaphore is successfully acquired, it returns 0. We must always check the return value. In the system call implementation of the _scull_ example, we return *-ERESTARTSYS* if *down_interruptible* returns nonzero. Upon seeing this return code, the signal handling layer of the kernel will either restart the call if the signal handler was registered with *SA_RESTART* or return *-EINTR* to the user. If we cannot undo changes to cleanly retry the system call, we must return *-EINTR* instead. *ERESTARTSYS* is used only in kernel and not to be seen by user space except that _strace_ gets to see it between the return of the interrupted system call and the execution of the signal handler.

__Mutexes__: *mutex*. In addition to binary sempaphores, the kernel defines a new mutex type, which is explained in _Documentation/mutex-design.txt_. The *mutex* struct is smaller and faster. *mutex* has a stricter and narrower use case, and in return, is easier to debug and enforce with *CONFIG_DEBUG_MUTEXES_ enabled. Whoever locked a mutex must unlock it. We cannot lock a mutex in one context and then unlock it in another.

__Reader-writer semaphores__: *rw_semaphore*. The count is always 1, unlike semaphores. *down_read()* and *down_write()* use uninterruptible sleep and there is no interruptible variant. A unique method that reader-writer spin locks don't have is *downgrade_writer()*, which atomically converts an acquired write lock to a read lock.

__Completion variables__: *completion*. Useful when one thread intiates some activity outside of the current thread, then waits for that activity to complete. Although we can use semaphores, they are heavily optimized for the _available_ case. Completions are a lightweight mechanism for one thread to tell another that the job is done. An example is the *vfork()* system call that uses *vfork_done* to wake up the parent process when the child execs or exits. Another example is *misc-modules/complete.c*.

__Sequential locks__: *seqlock_t*. A lightweight and scalable lock for many readers and a few writers, that favors writers over readers. The data being accessed should be a simple structure involving no pointers. *write_seqlock()* acquires a spinlock to write exclusively, and then increments a sequence counter. Readers can access quickly without locks, except that they need to check for collisions and retry (sounds like STM!). Before reading the data, readers read the sequence counter with *read_seqbegin()*, which busy waits until the number becomes even, indicating no write is underway. After reading the data, readers compare the saved numer with the current sequence number, and retry the read if there is a mismatch. A prominent example is *xtime_lock* that protects *jiffies_64* on machines that cannot atomically access the 64-bit integer.

__Read-copy-update__ (__RCU__): _linux/rcupdate.h_. An advanced mutual exclusion scheme optimized for many reads and rare writes. The resource being protected should be accessed via pointers. When the data structure (e.g. linked list) needs to be changed, the writer makes a copy, changes the copy, and updates the pointer to the new copy. When no references to the old version remain, it can be garbage collected. More references can be found at https://www.ibm.com/developerworks/cn/linux/l-rcu/ http://en.wikipedia.org/wiki/Read-copy-update http://www.rdrop.com/users/paulmck/RCU/ http://www.linuxjournal.com/article/6993 Documentation/RCU/

It's important that we protect __data__ and not code. It's the actual data inside critical sections that needs protection and not the code. Rather than lock code, always associate shared data with a specific lock. Locking in a device driver is usually straightforward. Start with a single lock that covers everything you do, then slowly move to finer locks, for example create one lock for every device you manage. Usually, functions invoked directly from system calls need to acquire locks, and internal functions can then assume that the locks have been properly acquired. A useful lock ordering rule is that we should take a local lock first before a more global lock; we should obtain semaphores efore spinlocks.

Lock free algorithms exist. For example, _linux/kfifo.h_ defines a circular buffer that needs no locking as long as there are only one reader and one writer.

An interesting book that talks about parallel programming is available at http://kernel.org/pub/linux/kernel/people/paulmck/perfbook/perfbook.html

Chapter 7
---------

CONFIG_NO_HZ tickless kernel http://kerneltrap.org/node/6750 http://lwn.net/Articles/223185/

http://www.csie.nctu.edu.tw/~wjtsai/EmbeddedSystemDesign/Ch3-2.pdf

Chapter 9
---------

*barrier()* prevents the compiler from reordering stores or loads across the compiler barrier. In addition to that, *rmb()* provides a read memory barrier so that no loads are reordered across the call by the processor. *wmb()* provides a write memory barrier, and *mb()* provides both. There are *smp* variants that only insert hardware barriers on SMP kernels.

http://duartes.org/gustavo/blog/post/motherboard-chipsets-memory-map

/proc/iomem

Chapter 10
---------

http://www.alexonlinux.com/smp-affinity-and-proper-interrupt-handling-in-linux http://www.alexonlinux.com/why-interrupt-affinity-with-multiple-cores-is-not-such-a-good-thing http://www.alexonlinux.com/msi-x-the-right-way-to-spread-interrupt-load http://lappwiki01.in2p3.fr/twiki-virgo/pub/VirgoRD/Software/Bibliography/Linux_Interrupts.pdf

Chapter 12
---------

Documentation/PCI/
http://www.redhat.com/mirrors/LDP/HOWTO/Plug-and-Play-HOWTO.html
http://tldp.org/LDP/tlk/dd/pci.html
http://en.wikipedia.org/wiki/PCI_configuration_space
http://xwindow.angelfire.com/
http://www.linux-mips.org/wiki/PCI_Subsystem
http://wiki.osdev.org/PCI
http://www.acm.uiuc.edu/sigops/roll_your_own/7.c.html
http://g2pc1.bu.edu/~qzpeng/manual/pcip.pdf
http://blog.sina.com.cn/s/articlelist_1685243084_3_1.html
http://www.ilinuxkernel.com/files/5/Linux_PCI_Express_Kernel_RW.htm
http://www.linuxforum.net/forum/showflat.php?Cat=&Board=linuxK&Number=251312

_lspci_ _setpci_. _/usr/share/hwdata/pci.ids_ from the _pciutils_ packages provides mapping from VendorID/DeviceID to VendorName/DeviceName.

_/proc/bus/pci/_ _/sys/bus/pci/_  Try the _tree_ command on these directories. _/proc/bus/pci/devices_ is a text file with device information, and _/proc/bus/pci/*/*_ are binary files that report a snapshot of the configuration registers. The individual devices also appear the sysfs tree under _/sys/bus/pci/devices_, which are symbolic links to the directories under _/sys/devices/pci0000:00/_, unless there are multiple PCI domains (which is uncommon on PCs). Under a PCI Device sysfs directory, _config_ is a binary file just like _/proc/bus/pci/*/*_. Other files like vendor, device, class, irq are obtained from the configuration space. The file _resource_ shows the current memory resources alocated.

The module initialization code for a PCI driver usually just calls pci_register_driver(&pci_driver), where the *pci_driver* struct defines an *id_table* and a few callback functions. The *id_table* is an array of *pci_device_id* structs, with an empty structure to mark the end of the table. This table indicates which devices are supported by the driver. Two helper macros should be used to intialize a *pci_device_id* struct. *PCI_DEVICE* matches the specific vendor and device ID, and *PCI_DEVICE_CLASS* matches a specific PCI class. This *id_table* is also exported to user space via the *MODULE_DEVICE_TABLE* macro which creates a local variable *__mod_pci_device_table*. In the kernel build process, _depmod_ searches all modules for that symbol, and adds it to the file */lib/modules/KERNEL_VERSION/modules.pcimap*. This map was used by the hotplug system, but not so much these days. See _Module Loading_ in http://www.linuxfromscratch.org/lfs/view/development/chapter07/udev.html.

Chapter 13
---------

http://www.kroah.com/linux/talks/ols_2005_driver_tutorial/index.html
