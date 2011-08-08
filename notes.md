Chapter 1
---------

_kernel/module.c_ implements the _init_module_ system call and other functions for loading and unloading kernel modules.

Chapter 2
---------

To load a kernel module, the kernel does a job similar to a linker, resolving symbols and adding the module's exported symbols to the kernel symbol table, all in memory. _modprobe_ looks at the module to be loaded to see if it references any symbols that are not currently defined, and looks for other modules in the current module search path that define those symbols. _lsmod_ works by reading _/proc/modules_, which is the older, single-file version of _/sys/module/_.

For each exported symbol, the EXPORT_SYMBOL macro defined in _linux/module.h_ records information in the *__ksymtab* ELF section.

The kernel has a _vermagic_ mechanism to prevent loading an incompatible module built against a different kernel. _modinfo_ shows the vermagic string of a module by reading its _.modinfo_ ELF section. From _modprobe_'s man page, it can be used to force loading a module. For more information, see http://hychen.wuweig.org/?p=495 and http://blog.sina.com.cn/s/blog_602f87700100lg8m.html.

_linux/version.h_ has macros for version tests.

_linux/init.h_ has macros to mark functions/data as used only at initialization time or exit time. They work by placing the definitions in special ELF sections, to give hints to the kernel. The comments are worth reading too.

Module parameters can be passed at load time by _insmod_ or _modeprobe_, or better yet, by storing them in configuration file _/etc/modprobe.d/*.conf_ and the older _/etc/modprobe.conf_. See the man page for _modprobe.conf(5)_. Sysfs entries for the parameters can be created under _/sys/module/*/parameters/_ at parameter registration time. If created as writable, the parameters can be changed dynamically by writing to the files.

Where a _printk_ message shows up is configurable. It may appear on the console, and/or on all terminals, and/or in log files such as _/var/log/messages_ or _/var/log/kernel.log_, and in _dmesg_. Default loglevel can be changed by writing to _/proc/sysrq-trigger_.

Kernel code cannot do floating point arithmetic, because saving and restoring the floating point processor's state on each entry and exit of kernel space is too expensive.

The _misc_modules/_ directory contains two example kernel modules _hello_ and _hellop_ (basically Hello World with parameter support), and a skeleton _Makefile_. The _Makefile_ uses a standard trick that locates the kernel source directory, and runs a __second__ make to invoke the kernel build system, which will helpfully move back to our module source directory and build our module. For more information see _Documentation/kbuild/_.

Chapter 3
---------

Important __struct__s: __file_operations__ __file__ __inode__ __cdev__

_/proc/devices_ lists all char and block device drivers currently registered. _Documentation/devices.txt_ has a detailed explanation for all statically assigned major numbers. Network devices don't quite qualify for the "everything is a file" philosphy, so they aren't accessed through _/dev/_. A network device receives packets asynchronously from the outside, and pushes them toward the kernel; while a block driver operates only in response to kernel requests.

The sample code provides a *scull_load* script that looks up _/proc/devices_ to find out the major number of the _scull_ driver, and _mkno_ the device nodes using its major number. In practice, _udev_ does the job automaticaly on modern kernels.

The *\__user* annotation notes that a pointer is a user-space address, thereby allowing static analyzers to find bugs that directly dereference *\__user* pointers.

*fs/char_dev.c* provides the infrastructure for char drivers.

*scull_init_module()* registers the char driver, gets the major number, and allocates the *struct scull_dev* (which embeds the standard *struct cdev*) array for managing the devices. It then calls *scull_setup_cdev()* to init the embedded *struct cdev* and add it to the system for each driver.

*scull_open()* can optionaly update the *file->f_op* pointer to provide different implementations depending on the minor number, but still under the same major number. It assigns the *file->private_data* to our *struct scull_dev* metadata, as a way to preserve state information across system calls.

*scull_release()* doesn't free anything, because we want to preserve the memory after the user closes the device file. *scull_cleanup_module()* will return all the resources when the module is unloaded.

*copy_to_user()* and *copy_from_user()* return 0 if successful, and return the amount of memory still to be copied otherwise. *scull_read()* is free to transfer data less than requested for the sake of simplicity, and the user is expected to retry. *fread()* reissues the system call until completion of the requested data transfer, and the user won't even notice. *scull_write()* also does partial transfer, but should support the model of either failing or succeeding completely for compatibility.

Chapter 4
---------

