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

