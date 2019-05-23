#ifndef CONFIG_DEFAULTS_PCBIOS_H
#define CONFIG_DEFAULTS_PCBIOS_H

/** @file
 *
 * Configuration defaults for PCBIOS
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

#define UACCESS_LIBRM
#define IOAPI_X86
#define PCIAPI_PCBIOS
#define TIMER_PCBIOS
#define CONSOLE_PCBIOS
#define NAP_PCBIOS
#define UMALLOC_MEMTOP
#define SMBIOS_PCBIOS
#ifndef VBOX 
#define SANBOOT_PCBIOS
#else
#undef SANBOOT_PCBIOS
#endif

#define ENTROPY_RTC
#define TIME_RTC

#undef	IMAGE_ELF		/* ELF image support */
#undef	IMAGE_MULTIBOOT		/* MultiBoot image support */
#define	IMAGE_PXE		/* PXE image support */
#undef IMAGE_SCRIPT		/* iPXE script image support */
#undef IMAGE_BZIMAGE		/* Linux bzImage image support */

#define PXE_STACK		/* PXE stack in iPXE - required for PXELINUX */
#define PXE_MENU		/* PXE menu booting */

#undef	SANBOOT_PROTO_ISCSI	/* iSCSI protocol */
#undef	SANBOOT_PROTO_AOE	/* AoE protocol */
#undef	SANBOOT_PROTO_IB_SRP	/* Infiniband SCSI RDMA protocol */
#undef	SANBOOT_PROTO_FCP	/* Fibre Channel protocol */

#define	REBOOT_CMD		/* Reboot command */

#endif /* CONFIG_DEFAULTS_PCBIOS_H */
