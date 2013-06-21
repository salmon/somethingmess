编译环境:
CentOS 6.2/2.6.30的内核:
Linux localhost.localdomain 2.6.32-220.el6.i686 #1 SMP Tue Dec 6 16:15:40 GMT 2011 i686 i686 i386 GNU/Linux
GCC版本:
Using built-in specs.
Target: i686-redhat-linux
Configured with: ../configure --prefix=/usr --mandir=/usr/share/man --infodir=/usr/share/info --with-bugurl=http://bugzilla.redhat.com/bugzilla --enable-bootstrap --enable-shared --enable-threads=posix --enable-checking=release --with-system-zlib --enable-__cxa_atexit --disable-libunwind-exceptions --enable-gnu-unique-object --enable-languages=c,c++,objc,obj-c++,java,fortran,ada --enable-java-awt=gtk --disable-dssi --with-java-home=/usr/lib/jvm/java-1.5.0-gcj-1.5.0.0/jre --enable-libgcj-multifile --enable-java-maintainer-mode --with-ecj-jar=/usr/share/java/eclipse-ecj.jar --disable-libjava-multilib --with-ppl --with-cloog --with-tune=generic --with-arch=i686 --build=i686-redhat-linux
Thread model: posix
gcc version 4.4.6 20110731 (Red Hat 4.4.6-3) (GCC)

测试脚本test.sh:
1. 编译内核
2. 加载模块
3. 创建字符设备
4. 跑用户态进程

内核态(kthread_test.c):
不用上个版本中的delayed_queue的方法.
1. 使用内核线程来清空
2. 使用waitqueue的方法来等待时间.
3. 主线程通过定时器的方法来唤醒等待队列.

用户态流程如下(test_usr.c):
1. 向缓冲区写入数据
2. 通过ioctl设置缓冲区清空时间是5秒
3. 从缓冲区读出数据,打印到屏幕上
4. 睡眠5s后, 从缓冲区读出数据, 打印到屏幕上

显示内容如下:
write to tdrv, data is "fill buf"
set 5 seconds to clean buffer
read tdrv immadately, data is "fill buf"
after 5 seconds then read tdrv, data is "fill buf"
