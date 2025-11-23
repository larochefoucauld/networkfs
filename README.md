# networkfs
This projects contains a loadable module that provides basic filesystem functionality through the kernel API. The driver uses kernel sockets to communicate with an HTTP server which handles actual file storage (or emulates such behavior) and manipulates file contents and directory tree on demand. 

> This project was completed as part of my undergradute Operating systems course

## Implementation
### Driver
Since this software has been created mostly for educational purposes, there are certain limitations imposed by its' design. First, for simplicity all required data is transmitted from the driver to the server in query parameters of an HTTP GET request. Server sends back raw binary data that can be directly copied into data structures declared in the module (see ABI note below). Second, a maximal number of directory entries, a file content size and a file name length are limited (primarily to comply with aforementioned data transmission approach and to make testing easier).

### Server
Current server implementation ([run_server](server/run_server)) is suitable to run included test suite and manually mount filesystem to explore its' functions. It is a single-threaded HTTP server that manages user tokens and stores filesystem state in internal data structures. It means that filesystem is persistent only until the server is stopped. However, it is quite simple to add serialization and loading of used Python objects on sever shutdown and startup. Server is designed to communicate exclusively with the driver, so it does not perform API checks.

## Usage
> It is recommended to build, load and test kernel module inside a virtual machine
### ABI note
As mentioned, server sends back binary data that matches binary layout of driver data structures. To achieve that, it relies on `ctypes` Python package which uses __native__ byte order and struct member padding rules (i. e. provided by the compiler that was used to build the Python interpreter on the target platform). Therefore, ABI compatibility __will likely break__ if client and server machines' architectures do not match. However, the easiest way to ensure correct data transmission is to deploy the server and the driver on the same machine, which is also convenient for testing.

### System requirements
The following instructions are for Ubuntu.

You will need the kernel version `6.8.0-45` and the corresponding headers to build and load the module.
```shell
$ sudo apt install linux-image-6.8.0-45-generic 
```

Edit GRUB configuration to boot this kernel version and reboot. Check that the correct kernel is loaded:
```shell
$ uname -r
6.8.0-45-generic
```

Install the kernel headers and toolchain
```shell
$ sudo apt-get install build-essential linux-headers-`uname -r` cmake
```

### Build and run
To build the module, in the repository directory use:
```shell
$ mkdir build
$ cd build
$ cmake ..
```

Load compiled module into the kernel:
```shell
$ sudo insmod networkfs.ko
```

To mount and use filesystem, first start the server (in a separate terminal) and obtain user token. By default, both the driver and the test suite expect the server to listen on port 8080:
```shell
$ ./server/run_server 8080
Networkfs server listening on 127.0.0.1 port 8080...
```

```shell
$ curl http://localhost:8080/networkfs/token/issue > ./token
```
Copy received token (for example, `fb375713-6a2b-4192-8f63-4a563a944fd0`) and mount the filesystem:
```shell
$ mkdir /mnt/networkfs
$ sudo mount -t networkfs fb375713-6a2b-4192-8f63-4a563a944fd0 /mnt/networkfs
```

Now you are ready to manage your files! Some are created by default for each new user:
```shell
$ cd /mnt/networkfs
$ ls -l
total 0
-rwxrwxrwx 1 user user 0 Nov 23 21:21 file1
-rwxrwxrwx 1 user user 0 Nov 23 21:21 file2
$ cat file1
hello world from file1
$ echo "New text" > file1
$ cat file1
New text
```
To unmount:
```shell
$ sudo umount /mnt/networkfs
```

The driver is eqipped with verbose error logging, so if something works not as expected, you can inspect the kernel log:
```shell
$ sudo dmesg
[ 1809.949152] networkfs: mounted
[ 1345.247806] networkfs: request_lookup: no entry with name ~ in directory 1000
[ 1345.276555] networkfs: request_lookup: no entry with name ~ in directory 1000
[ 1825.321506] networkfs: superblock is destroyed; token: fb375713-6a2b-4192-8f63-4a563a944fd0
```

### Test
The test suite in [tests/](tests/) covers basic file and directory creation, deletion and listing, file operations (read, write, navigation) and hard links manipulation. To build tests, execute in repository directory:
```shell
$ cd build
$ make networkfs_test
```
Start the server and launch tests (this rebuilds and inserts the module if it is not already inserted):

```shell
$ sudo ctest --preset base --output-on-failure
$ sudo ctest --preset encoding --output-on-failure
$ sudo ctest --preset file --output-on-failure
$ sudo ctest --preset link --output-on-failure
```