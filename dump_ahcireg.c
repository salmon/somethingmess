#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define IOPORTS_PATH		"/proc/ioports"
#define IOMEM_PATH	"/proc/iomem"
#define BUF_LEN	512

enum {
	/* HOST_CTL bits */
	HOST_RESET		= (1 << 0),  /* reset controller; self-clear */
	HOST_IRQ_EN		= (1 << 1),  /* global IRQ enable */
	HOST_AHCI_EN		= (1 << 31), /* AHCI enabled */
	HOST_MRSM		= (1 << 2),

	/* HOST_CAP bits */
	HOST_CAP_SXS		= (1 << 5),  /* Supports External SATA */
	HOST_CAP_EMS		= (1 << 6),  /* Enclosure Management support */
	HOST_CAP_CCC		= (1 << 7),  /* Command Completion Coalescing */
	HOST_CAP_PART		= (1 << 13), /* Partial state capable */
	HOST_CAP_SSC		= (1 << 14), /* Slumber state capable */
	HOST_CAP_PIO_MULTI	= (1 << 15), /* PIO multiple DRQ support */
	HOST_CAP_FBS		= (1 << 16), /* FIS-based switching support */
	HOST_CAP_PMP		= (1 << 17), /* Port Multiplier support */
	HOST_CAP_ONLY		= (1 << 18), /* Supports AHCI mode only */
	HOST_CAP_CLO		= (1 << 24), /* Command List Override support */
	HOST_CAP_LED		= (1 << 25), /* Supports activity LED */
	HOST_CAP_ALPM		= (1 << 26), /* Aggressive Link PM support */
	HOST_CAP_SSS		= (1 << 27), /* Staggered Spin-up */
	HOST_CAP_MPS		= (1 << 28), /* Mechanical presence switch */
	HOST_CAP_SNTF		= (1 << 29), /* SNotification register */
	HOST_CAP_NCQ		= (1 << 30), /* Native Command Queueing */
	HOST_CAP_64		= (1 << 31), /* PCI DAC (64-bit DMA) support */

	/* HOST_CAP2 bits */
	HOST_CAP2_BOH		= (1 << 0),  /* BIOS/OS handoff supported */
	HOST_CAP2_NVMHCI	= (1 << 1),  /* NVMHCI supported */
	HOST_CAP2_APST		= (1 << 2),  /* Automatic partial to slumber */

	/* CCC_CTL bits */
	CCC_CTL_EN	= (1 << 0),

	/* BOHC bits */
	BOHC_BOS = (1 << 0),
	BOHC_OOS = (1 << 1),
	BOHC_SOOE = (1 << 2),
	BOHC_OOC = (1 << 3),
	BOHC_BB = (1 << 4),

	/* MASK */
	MASK1 = 0xf,
	MASK2 = 0xff,
	MASK3 = 0xffff,

	/* registers for each SATA port */
	PORT_LST_ADDR		= 0x00, /* command list DMA addr */
	PORT_LST_ADDR_HI	= 0x04, /* command list DMA addr hi */
	PORT_FIS_ADDR		= 0x08, /* FIS rx buf addr */
	PORT_FIS_ADDR_HI	= 0x0c, /* FIS rx buf addr hi */
	PORT_IRQ_STAT		= 0x10, /* interrupt status */
	PORT_IRQ_MASK		= 0x14, /* interrupt enable/disable mask */
	PORT_CMD		= 0x18, /* port command */
	PORT_TFDATA		= 0x20,	/* taskfile data */
	PORT_SIG		= 0x24,	/* device TF signature */
	PORT_CMD_ISSUE		= 0x38, /* command issue */
	PORT_SCR_STAT		= 0x28, /* SATA phy register: SStatus */
	PORT_SCR_CTL		= 0x2c, /* SATA phy register: SControl */
	PORT_SCR_ERR		= 0x30, /* SATA phy register: SError */
	PORT_SCR_ACT		= 0x34, /* SATA phy register: SActive */
	PORT_SCR_NTF		= 0x3c, /* SATA phy register: SNotification */
	PORT_FBS		= 0x40, /* FIS-based Switching */

	/* PORT_IRQ_{STAT,MASK} bits */
	PORT_IRQ_COLD_PRES	= (1 << 31), /* cold presence detect */
	PORT_IRQ_TF_ERR		= (1 << 30), /* task file error */
	PORT_IRQ_HBUS_ERR	= (1 << 29), /* host bus fatal error */
	PORT_IRQ_HBUS_DATA_ERR	= (1 << 28), /* host bus data error */
	PORT_IRQ_IF_ERR		= (1 << 27), /* interface fatal error */
	PORT_IRQ_IF_NONFATAL	= (1 << 26), /* interface non-fatal error */
	PORT_IRQ_OVERFLOW	= (1 << 24), /* xfer exhausted available S/G */
	PORT_IRQ_BAD_PMP	= (1 << 23), /* incorrect port multiplier */

	PORT_IRQ_PHYRDY		= (1 << 22), /* PhyRdy changed */
	PORT_IRQ_DEV_ILCK	= (1 << 7), /* device interlock */
	PORT_IRQ_CONNECT	= (1 << 6), /* port connect change status */
	PORT_IRQ_SG_DONE	= (1 << 5), /* descriptor processed */
	PORT_IRQ_UNK_FIS	= (1 << 4), /* unknown FIS rx'd */
	PORT_IRQ_SDB_FIS	= (1 << 3), /* Set Device Bits FIS rx'd */
	PORT_IRQ_DMAS_FIS	= (1 << 2), /* DMA Setup FIS rx'd */
	PORT_IRQ_PIOS_FIS	= (1 << 1), /* PIO Setup FIS rx'd */
	PORT_IRQ_D2H_REG_FIS	= (1 << 0), /* D2H Register FIS rx'd */
};

struct iobases {
	unsigned long long iomem_start;
	unsigned long long iomem_end;
};

struct ahci_link {
	struct iobases iomem;
	struct ahci_link *next;
};

int find_ahci_line(char *line)
{
	unsigned int idx = 0;
	int i = 0;
	while(line[idx++]) {
		switch (i) {
		case 0:
			if (line[idx] == 'a') {
				i = 1;
			}
			break;
		case 1:
			if (line[idx] == 'h') {
				i = 2;
			} else {
				i = 0;
			}
			break;
		case 2:
			if (line[idx] == 'c') {
				i = 3;
			} else {
				i = 0;
			}
			break;
		case 3:
			if (line[idx] == 'i') {
				return 1;
			} else {
				i = 0;
			}
			break;
		default:
			printf("state mechine error\n");
			exit(1);
			break;
		}
	}
	return 0;
}

void parse_line(char *buf, struct ahci_link *link)
{
	int i = 0;
	unsigned long long tmp_start = 0;
	unsigned long long tmp_end = 0;
	int flag = 0;

	struct ahci_link *ahci_one = malloc(sizeof(struct ahci_link));
	ahci_one->iomem.iomem_start = 0;
	ahci_one->iomem.iomem_end = 0;
	ahci_one->next = NULL;

	while (buf[i]) {
		if (buf[i]>='0' && buf[i]<='9') {
			if (flag == 0) {
				tmp_start <<= 4;
				tmp_start += (buf[i] - '0');
			} else {
				tmp_end <<= 4;
				tmp_end += (buf[i] - '0');
			}
		} else if (buf[i]>='a' && buf[i]<='f') {
			if (flag == 0) {
				tmp_start <<= 4;
				tmp_start += (buf[i]-'a'+10);
			} else {
				tmp_end <<= 4;
				tmp_end += (buf[i]-'a'+10);
			}
		} else if (buf[i] == '-') {
			flag = 1;
		} else if (flag == 1) {
			break;
		}
		i++;
	}

	ahci_one->iomem.iomem_start = tmp_start;
	ahci_one->iomem.iomem_end = tmp_end;

	while (link->next);
	link->next = ahci_one;
}

void getbaseaddr(struct ahci_link *link)
{
	int fd;
	off_t cur;
	char *buf;
	int idx;
	ssize_t rsize;

	buf = malloc(BUF_LEN);
	if (NULL == buf) {
		printf("malloc error\n");
		exit(1);
	}
	fd = open(IOMEM_PATH, O_RDONLY);
	if (fd < 0) {
		printf("open %s error\n", IOMEM_PATH);
		free(buf);
		exit(1);
	}
	while (1) {
		cur = lseek(fd, 0, SEEK_CUR);
		idx = 0;
		rsize = read(fd, buf, BUF_LEN);
		if(rsize <= 0) {
			break;
		}
		while (buf[idx++] != '\n' && idx < rsize);
		if (idx < rsize) {
			lseek(fd, cur+idx, SEEK_SET);
			buf[idx] = 0;
		}
		if (find_ahci_line(buf)) {
			parse_line(buf, link);
		}
	}
	free(buf);
}

void getCAP(int reg)
{
	int spd = 0;
	printf("*HBA Capabilities*\n\n");

	printf("Number of Ports is %d\n", (reg & MASK1));
	printf("Supports External SATA %s\n", (reg & HOST_CAP_SXS ? "True" : "False"));
	printf("Enclosure Management Supported %s\n", (reg & HOST_CAP_EMS ? "True" : "False"));
	printf("Command Completion Coalescing Supported %s\n", (reg & HOST_CAP_CCC ? "True" : "False"));
	printf("Number of Command Slots %d\n", (reg >> 8 & MASK1));
	printf("Partial State Capable %s\n", (reg & HOST_CAP_PART ? "True" : "False"));
	printf("Slumber State Capable %s\n", (reg & HOST_CAP_SSC ? "True" : "False"));
	printf("PIO Multiple DRQ Block %s\n", (reg & HOST_CAP_PIO_MULTI ? "True" : "False"));
	printf("FIS-based Switching Supported %s\n", (reg & HOST_CAP_FBS ? "True" : "False"));
	printf("Supports Port Multiplier %s\n", (reg & HOST_CAP_PMP ? "True" : "False"));
	printf("Supports AHCI mode only %s\n", (reg & HOST_CAP_ONLY ? "True" : "False"));
	spd = reg >> 20 & MASK1;
	printf("Interface Speed Support %d ", spd);
	switch(spd) {
	case 1:
		printf("Gen 1 (1.5 Gbps)\n");
		break;
	case 2:
		printf("Gen 2 (3 Gbps) \n");
		break;
	case 3:
		printf("Gen 3 (6 Gbps)\n");
		break;
	default:
		printf("Reserved\n");
		break;

	}
	
	printf("Supports Command List Override %s\n", (reg & HOST_CAP_CLO ? "True" : "False"));
	printf("Supports Activity LED %s\n", (reg & HOST_CAP_LED ? "True" : "False"));
	printf("Supports Aggressive Link Power Management %s\n", (reg & HOST_CAP_ALPM ? "True" : "False"));
	printf("Supports Staggered Spin-up %s\n", (reg & HOST_CAP_SSS ? "True" : "False"));
	printf("Supports Mechanical Presence Switch %s\n", (reg & HOST_CAP_MPS ? "True" : "False"));
	printf("Supports SNotification Register %s\n", (reg & HOST_CAP_SNTF ? "True" : "False"));
	printf("Supports Native Command Queuing %s\n", (reg & HOST_CAP_NCQ ? "True" : "False"));
	printf("Supports 64-bit Addressing %s\n", (reg & HOST_CAP_64 ? "True" : "False"));
}

void getGHC(int reg)
{
	printf("\n*Global HBA Control*\n\n");
	
	printf("HBA Reset Reg Value %d\n", (reg & HOST_RESET));
	printf("Interrupt Enable %s\n", (reg & HOST_IRQ_EN ? "True" : "False"));
	printf("MSI Revert to Single Message Reg Value %d\n", (reg & HOST_MRSM));
	printf("AHCI Enable %s\n", (reg & HOST_AHCI_EN ? "True" : "False"));
}

void getIS(int reg)
{
	printf("\n*Interrupt Status Register*\n\n");

	printf("Interrupt Pending Status 0x%x\n", reg);
}

unsigned int getPI(int reg)
{
	printf("\n*Ports Implemented*\n\n");
	
	printf("Port Implemented 0x%x\n", reg);
	return reg;
}

void getVS(int reg)
{
	unsigned int maj, min;
	printf("\n*AHCI Version*\n\n");

	min = reg & MASK3;	
	maj = reg >> 16 & MASK3;
	printf("AHCI Version %x.%x", maj, min);
}

void getCCCctl(int reg)
{
	printf("\n*Command Completion Coalescing Control*\n\n");

	if (reg & CCC_CTL_EN) {
		printf("CCC is enabled\n");
		printf("CCC Interrupt 0x%x\n", reg >> 3 & MASK1);
		printf("CCC Command Completions 0x%x\n", reg >> 8 & MASK2);
		printf("CCC Timeout Value 0x%x\n", reg >> 16 & MASK3);
	} else {
		printf("CCC is disabled\n");
	}
}

void getCCCports(int reg)
{
	printf("\n*Command Completion Coalescing Ports*\n\n");
	
	printf("CCC Ports 0x%x\n", reg);
}

void getEMloc(int reg)
{
	printf("\n*Enclosure Management Location*\n\n");
	
	printf("Buffer Size 0x%x\n", reg & MASK3);
	printf("Offset 0x%x\n", reg >> 16 & MASK3);
}

void getEMctl(int reg)
{
	printf("\n*Enclosure Management Control*\n\n");

	printf("0x%x\n", reg);
}

void getCAP2(int reg)
{
	printf("\n*HBA Capabilities Extended*\n\n");

	printf("BIOS/OS Handoff Supported %s\n", (reg & HOST_CAP2_BOH ? "True" : "False"));
	printf("NVMHCI Present Supported %s\n", (reg & HOST_CAP2_NVMHCI ? "True" : "False"));
	printf("Automatic Partial to Slumber Transitions Supported %s\n", (reg & HOST_CAP2_APST ? "True" : "False"));
}

void getBOHC(int reg)
{
	printf("\n*BIOS/OS Handoff Control and Status*\n\n");

	printf("BIOS Owned Semaphore %s\n", (reg & BOHC_BOS ? "True" : "False"));
	printf("OS Owned Semaphore %s\n", (reg & BOHC_OOS ? "True" : "False"));
	/* what's SMI? */
	printf("SMI on OS Ownership Change Enable %s\n", (reg & BOHC_SOOE ? "True" : "False"));
	printf("OS Ownership Change %s\n", (reg & BOHC_OOC ? "True" : "False"));
	printf("BIOS Busy %s\n", (reg & BOHC_BB ? "True" : "False"));
}

void getVendorSpec(int reg, int i)
{
	printf("%x  ", reg);
}

void getinformation(int reg, int i)
{
	/* dump host regs */
	switch (i) {
	case 1:
		getCAP(reg);
		break;
	case 2:
		getGHC(reg);
		break;
	case 3:
		getIS(reg);
		break;
	case 4:
		getPI(reg);
		break;
	case 5:
		getVS(reg);
		break;
	case 6:
		getCCCctl(reg);
		break;
	case 7:
		getCCCports(reg);
		break;
	case 8:
		getEMloc(reg);
		break;
	case 9:
		getEMctl(reg);
		break;
	case 10:
		getCAP2(reg);
	case 11:
		getBOHC(reg);
		break;
	default:
		break;
	}

	/* dump vendor specific regs */
	if(i > 40 && i <= 64) {
		if ( i == 41 ) {
			printf("\n*Vendor Specific Registers*\n\n");
		}
		getVendorSpec(reg, i-40);
	}

}

void getportinfo(int reg, int i)
{
	/* dump port regs */
	switch(i) {
	case 0:
		printf("PxCLB 0x%x\n", reg);
		break;
	case 1:
		printf("PxCLBU 0x%x\n", reg);
		break;
	case 2:
		printf("PxFB 0x%x\n", reg);
		break;
	case 3:
		printf("PxFBU 0x%x\n", reg);
		break;
	case 4:
		printf("PxIS 0x%x\n", reg);
		break;
	case 5:
		printf("PxIE 0x%x\n", reg);
		break;
	case 6:
		printf("PxCMD 0x%x\n", reg);
		break;
	case 7:
		printf("PxTFD 0x%x\n", reg);
		break;
	case 8:
		printf("PxSIG 0x%x\n", reg);
		break;
	case 9:
		printf("PxSSTS 0x%x\n", reg);
		break;
	case 10:
		printf("PxSCTL 0x%x\n", reg);
		break;
	case 11:
		printf("PxSERR 0x%x\n", reg);
		break;
	case 12:
		printf("PxSACT 0x%x\n", reg);
		break;
	case 13:
		printf("PxCI 0x%x\n", reg);
		break;
	case 14:
		printf("PxSNTF 0x%x\n", reg);
		break;
	case 15:
		printf("PxFBS 0x%x\n", reg);
		break;
	case 16:
		printf("*Vendor Specific Registers*\n");
	default:
		getVendorSpec(reg, i);
		break;
	}
}

void getreginfo(unsigned long long iomem_start, unsigned long long iomem_end)
{
	int fd;
	int i, j=0;
	size_t len;
	unsigned int *map_base;
	unsigned int *p;
	unsigned int port_implement = 0;
	
	fd = open("/dev/mem", O_RDONLY);
	if (fd < 0) {
		printf("open error\n");
		exit(1);
	}
	len = (iomem_end - iomem_start);
#if 0
	map_base = mmap64(NULL, len, PROT_READ, MAP_SHARED, fd, iomem_start);
#else
	map_base = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, iomem_start);
#endif
	if ((void *)(-1) == map_base) {
		printf("mmap error\n");
		exit(1);
	}
	p = map_base;
	while (1) {
		j++;
		if (j > 64 || j > len) {
			break;
		}
		getinformation(*p, j);
		p++;
	}
	/*get port info*/
	p = map_base;
	p = p + 3;
	port_implement = getPI(*p);
	for (i=0; i<32; i++) {
		if ((port_implement >> i) & 0x1) {
			printf("\n\n*Port %d regs\n", i);
			if (((i+1)*32 + 64) > len) {
				printf("exceed the memory map bound\n");
				break;
			}
			p = map_base+64+i*32;
			for (j=0; j<32; j++) {
				getportinfo(*p, j);
				p ++;
			}
		}
	}

	close(fd);
	munmap(map_base, len);
}

int main()
{
	struct ahci_link head;
	struct ahci_link *tmp;
	unsigned long long tmp_start=0, tmp_end=0;

	head.next = NULL;
	getbaseaddr(&head);
	if (head.next == NULL) {
		printf("can't find ahci controller\n");
		return 1;
	}
	tmp = &head;
	while (1) {
		tmp = tmp->next;
		tmp_start = tmp->iomem.iomem_start;
		tmp_end = tmp->iomem.iomem_end;
		printf("ahci start %llx end %llx\n", tmp_start, tmp_end);
		getreginfo(tmp_start, tmp_end);
		if (!tmp->next) {
			break;
		}
	}
	return 0;
}
