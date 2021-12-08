# Simple I/O System Implementation
**Zhu Chuyan** 119010486 **at** *The Chinese University of Hong Kong, Shenzhen*
## Table of contents
- [Simple I/O System Implementation](#simple-io-system-implementation)
  - [Table of contents](#table-of-contents)
  - [1. How to run the program](#1-how-to-run-the-program)
    - [1.1 Running environments](#11-running-environments)
    - [1.2 Executions](#12-executions)
  - [2. Implementations](#2-implementations)
    - [2.1 Single Layer Filesystem Simulation](#21-single-layer-filesystem-simulation)
      - [Implementation Flowchart](#implementation-flowchart)
      - [init_modules](#init_modules)
        - [](#)
      - [ioctl](#ioctl)
      - [write](#write)
      - [read](#read)
      - [drv_arithmetic_routine](#drv_arithmetic_routine)
  - [3. The results](#3-the-results)
    - [User space test result](#user-space-test-result)
    - [Kernel space log result](#kernel-space-log-result)
  - [4. What I learn from the project](#4-what-i-learn-from-the-project)


## 1. How to run the program
### 1.1 Running environments
* Operating system: ```Linux```
* Linux distribution: ```Ubuntu 5.4.0```
* Linux kernel version: ```Linux version 4.15.0-29-generic```
* Compiler version: ```gcc (Ubuntu 5.4.0-6ubuntu1~16.04.10) 5.4.0 20160609```

### 1.2 Executions
A script has prepare to automatically compile, install module and **mknod** (no need to input major and minor manually) as well:
```shell
sh ./start.sh
```
You can then test the program by executing
```shell
./test
```

You can remove module, clean make, *rmnod* and get output result by:
```shell
sh ./end.sh
```
However, the original command still works (```make``` -> ```dmesg 
| grep chrdev``` to check MAJOR and MINOR -> ```sh ./mkdev.sh MAJOR MINOR``` to mknod -> ```./test``` to test user space program) -> ```make clean``` to clean build and output kernel log results -> ```sh ./rmdev.sh``` to rmnod.

## 2. Implementations
### 2.1 Single Layer Filesystem Simulation

#### Implementation Flowchart

 ![flow_chart](https://raw.githubusercontent.com/BrandoZhang/Operating-System/master/README.assets/image-20181211232810112.png)

***
#### init_modules
* Allocate, initialize, and add device;
* Register device file operation;
* Register irq handler (for bonus) by request_irq()
  Note that as this irq_hanlder is registered in shared mode (IRQF_SHARED) because there is the real keyboard irq handler to co-exist, therefore a unique ```dev_id``` is required. I do the trick as follows:
  ```C
  //define as global variable in the module
  static struct DEVICE_UNIQUE{} dev_unique;
  ```
  Then in the init_modules:
  ```C
  request_irq(IRQ_NUM,(irq_handler_t)irq_handler,IRQF_SHARED,
  "test_keyboard_irq_handler", &dev_unique);
  ```
  Note that since this pointer must be within the driver's memory space, it is ipso facto unique to this driver. As the size of empty struct is a single byte, it is well compatible with void pointer. (An alternative approach is to pass the ```dev_cdev``` pointer, which requires type-cast).

##### 

***
#### ioctl
* HW5_IOCSETBLOCK: use ```get_user()``` to get the block argument and store it to DMA buffer
* HW5_IOCWAITREADABLE: use a while loop with ```msleep``` to wait till the readable flag becomes 1; note that the ```msleep``` is necessary to allow CPU to conduct other work while waiting.
* Other functions are analogous

***
#### write
* Register function routine with ```data``` structure using ```INIT_WORK``` macro
* Fetch data from user space: ```copy_from_user``` is called to copy the DataIn struct data instance to kernel space and then store them to the DMA buffer
* Blocking I/O: call ```schedule_work()``` to add the work to the queue (to run later), then call ```flush_scheduled_work()``` to block till all the scheduled works are completed
* Non-blocking I/O: simply call ```schedule_work()``` to add the work to the queue (to run later), then return.
***
#### read
* Simply use ```put_user()``` to put the answer (retrieved from the DMA buffer) to the user space address
***
#### drv_arithmetic_routine
* Retrieve the data from DMA buffer
* Carry out the calculation
* Put the result back to DMA buffer
* Set readable flag to 1 in DMA buffer

***


## 3. The results
### User space test result
![AAA1111](https://i.imgur.com/TJAv3EX.png)

### Kernel space log result
***Note that I type the keyboard 8 times, resulting to a interrupt count of 16. (Because I type the command through remote ssh terminal in VS Code on my Mac host machine, which is not keyboard interrupt, I can easily seperate them and control how many hits there are by typing EXACTLY that many times in the virtual machine GUI interface).*

![BBB2222](https://i.imgur.com/qog1BdG.png)

## 4. What I learn from the project
I learned how to implement a simple character device driver complying with the Linux api, and implement both blocking I/O and non-blocking I/O, and also learned to implement an simple IRQ handler.