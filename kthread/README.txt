���뻷��:
CentOS 6.2/2.6.30���ں�:
Linux localhost.localdomain 2.6.32-220.el6.i686 #1 SMP Tue Dec 6 16:15:40 GMT 2011 i686 i686 i386 GNU/Linux
GCC�汾:
Using built-in specs.
Target: i686-redhat-linux
Configured with: ../configure --prefix=/usr --mandir=/usr/share/man --infodir=/usr/share/info --with-bugurl=http://bugzilla.redhat.com/bugzilla --enable-bootstrap --enable-shared --enable-threads=posix --enable-checking=release --with-system-zlib --enable-__cxa_atexit --disable-libunwind-exceptions --enable-gnu-unique-object --enable-languages=c,c++,objc,obj-c++,java,fortran,ada --enable-java-awt=gtk --disable-dssi --with-java-home=/usr/lib/jvm/java-1.5.0-gcj-1.5.0.0/jre --enable-libgcj-multifile --enable-java-maintainer-mode --with-ecj-jar=/usr/share/java/eclipse-ecj.jar --disable-libjava-multilib --with-ppl --with-cloog --with-tune=generic --with-arch=i686 --build=i686-redhat-linux
Thread model: posix
gcc version 4.4.6 20110731 (Red Hat 4.4.6-3) (GCC)

���Խű�test.sh:
1. �����ں�
2. ����ģ��
3. �����ַ��豸
4. ���û�̬����

�ں�̬(kthread_test.c):
�����ϸ��汾�е�delayed_queue�ķ���.
1. ʹ���ں��߳������
2. ʹ��waitqueue�ķ������ȴ�ʱ��.
3. ���߳�ͨ����ʱ���ķ��������ѵȴ�����.

�û�̬��������(test_usr.c):
1. �򻺳���д������
2. ͨ��ioctl���û��������ʱ����5��
3. �ӻ�������������,��ӡ����Ļ��
4. ˯��5s��, �ӻ�������������, ��ӡ����Ļ��

��ʾ��������:
write to tdrv, data is "fill buf"
set 5 seconds to clean buffer
read tdrv immadately, data is "fill buf"
after 5 seconds then read tdrv, data is "fill buf"
