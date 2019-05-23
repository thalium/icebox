; $Id: VBoxBiosAlternative8086.asm $ 
;; @file
; Auto Generated source file. Do not edit.
;

;
; Source file: post.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  BIOS POST routines. Used only during initialization.
;  
;  
;  
;  Copyright (C) 2004-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

;
; Source file: bios.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  PC BIOS - ???
;  
;  
;  
;  Copyright (C) 2006-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  --------------------------------------------------------------------
;  
;  This code is based on:
;  
;   ROM BIOS for use with Bochs/Plex86/QEMU emulation environment
;  
;   Copyright (C) 2002  MandrakeSoft S.A.
;  
;     MandrakeSoft S.A.
;     43, rue d'Aboukir
;     75002 Paris - France
;     http://www.linux-mandrake.com/
;     http://www.mandrakesoft.com/
;  
;   This library is free software; you can redistribute it and/or
;   modify it under the terms of the GNU Lesser General Public
;   License as published by the Free Software Foundation; either
;   version 2 of the License, or (at your option) any later version.
;  
;   This library is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;   Lesser General Public License for more details.
;  
;   You should have received a copy of the GNU Lesser General Public
;   License along with this library; if not, write to the Free Software
;   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
;  

;
; Source file: print.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  PC BIOS - ???
;  
;  
;  
;  Copyright (C) 2006-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  --------------------------------------------------------------------
;  
;  This code is based on:
;  
;   ROM BIOS for use with Bochs/Plex86/QEMU emulation environment
;  
;   Copyright (C) 2002  MandrakeSoft S.A.
;  
;     MandrakeSoft S.A.
;     43, rue d'Aboukir
;     75002 Paris - France
;     http://www.linux-mandrake.com/
;     http://www.mandrakesoft.com/
;  
;   This library is free software; you can redistribute it and/or
;   modify it under the terms of the GNU Lesser General Public
;   License as published by the Free Software Foundation; either
;   version 2 of the License, or (at your option) any later version.
;  
;   This library is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;   Lesser General Public License for more details.
;  
;   You should have received a copy of the GNU Lesser General Public
;   License along with this library; if not, write to the Free Software
;   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
;  

;
; Source file: ata.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  PC BIOS - ???
;  
;  
;  
;  Copyright (C) 2006-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  --------------------------------------------------------------------
;  
;  This code is based on:
;  
;   ROM BIOS for use with Bochs/Plex86/QEMU emulation environment
;  
;   Copyright (C) 2002  MandrakeSoft S.A.
;  
;     MandrakeSoft S.A.
;     43, rue d'Aboukir
;     75002 Paris - France
;     http://www.linux-mandrake.com/
;     http://www.mandrakesoft.com/
;  
;   This library is free software; you can redistribute it and/or
;   modify it under the terms of the GNU Lesser General Public
;   License as published by the Free Software Foundation; either
;   version 2 of the License, or (at your option) any later version.
;  
;   This library is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;   Lesser General Public License for more details.
;  
;   You should have received a copy of the GNU Lesser General Public
;   License along with this library; if not, write to the Free Software
;   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
;  

;
; Source file: floppy.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  PC BIOS - ???
;  
;  
;  
;  Copyright (C) 2006-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  --------------------------------------------------------------------
;  
;  This code is based on:
;  
;   ROM BIOS for use with Bochs/Plex86/QEMU emulation environment
;  
;   Copyright (C) 2002  MandrakeSoft S.A.
;  
;     MandrakeSoft S.A.
;     43, rue d'Aboukir
;     75002 Paris - France
;     http://www.linux-mandrake.com/
;     http://www.mandrakesoft.com/
;  
;   This library is free software; you can redistribute it and/or
;   modify it under the terms of the GNU Lesser General Public
;   License as published by the Free Software Foundation; either
;   version 2 of the License, or (at your option) any later version.
;  
;   This library is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;   Lesser General Public License for more details.
;  
;   You should have received a copy of the GNU Lesser General Public
;   License along with this library; if not, write to the Free Software
;   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
;  

;
; Source file: floppyt.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  Floppy drive tables.
;  
;  
;  
;  Copyright (C) 2011-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

;
; Source file: eltorito.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  PC BIOS - ???
;  
;  
;  
;  Copyright (C) 2006-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  --------------------------------------------------------------------
;  
;  This code is based on:
;  
;   ROM BIOS for use with Bochs/Plex86/QEMU emulation environment
;  
;   Copyright (C) 2002  MandrakeSoft S.A.
;  
;     MandrakeSoft S.A.
;     43, rue d'Aboukir
;     75002 Paris - France
;     http://www.linux-mandrake.com/
;     http://www.mandrakesoft.com/
;  
;   This library is free software; you can redistribute it and/or
;   modify it under the terms of the GNU Lesser General Public
;   License as published by the Free Software Foundation; either
;   version 2 of the License, or (at your option) any later version.
;  
;   This library is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;   Lesser General Public License for more details.
;  
;   You should have received a copy of the GNU Lesser General Public
;   License along with this library; if not, write to the Free Software
;   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
;  

;
; Source file: boot.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  PC BIOS - ???
;  
;  
;  
;  Copyright (C) 2006-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  --------------------------------------------------------------------
;  
;  This code is based on:
;  
;   ROM BIOS for use with Bochs/Plex86/QEMU emulation environment
;  
;   Copyright (C) 2002  MandrakeSoft S.A.
;  
;     MandrakeSoft S.A.
;     43, rue d'Aboukir
;     75002 Paris - France
;     http://www.linux-mandrake.com/
;     http://www.mandrakesoft.com/
;  
;   This library is free software; you can redistribute it and/or
;   modify it under the terms of the GNU Lesser General Public
;   License as published by the Free Software Foundation; either
;   version 2 of the License, or (at your option) any later version.
;  
;   This library is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;   Lesser General Public License for more details.
;  
;   You should have received a copy of the GNU Lesser General Public
;   License along with this library; if not, write to the Free Software
;   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
;  

;
; Source file: keyboard.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  PC BIOS - ???
;  
;  
;  
;  Copyright (C) 2006-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  --------------------------------------------------------------------
;  
;  This code is based on:
;  
;   ROM BIOS for use with Bochs/Plex86/QEMU emulation environment
;  
;   Copyright (C) 2002  MandrakeSoft S.A.
;  
;     MandrakeSoft S.A.
;     43, rue d'Aboukir
;     75002 Paris - France
;     http://www.linux-mandrake.com/
;     http://www.mandrakesoft.com/
;  
;   This library is free software; you can redistribute it and/or
;   modify it under the terms of the GNU Lesser General Public
;   License as published by the Free Software Foundation; either
;   version 2 of the License, or (at your option) any later version.
;  
;   This library is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;   Lesser General Public License for more details.
;  
;   You should have received a copy of the GNU Lesser General Public
;   License along with this library; if not, write to the Free Software
;   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
;  

;
; Source file: disk.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  PC BIOS - ???
;  
;  
;  
;  Copyright (C) 2006-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  --------------------------------------------------------------------
;  
;  This code is based on:
;  
;   ROM BIOS for use with Bochs/Plex86/QEMU emulation environment
;  
;   Copyright (C) 2002  MandrakeSoft S.A.
;  
;     MandrakeSoft S.A.
;     43, rue d'Aboukir
;     75002 Paris - France
;     http://www.linux-mandrake.com/
;     http://www.mandrakesoft.com/
;  
;   This library is free software; you can redistribute it and/or
;   modify it under the terms of the GNU Lesser General Public
;   License as published by the Free Software Foundation; either
;   version 2 of the License, or (at your option) any later version.
;  
;   This library is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;   Lesser General Public License for more details.
;  
;   You should have received a copy of the GNU Lesser General Public
;   License along with this library; if not, write to the Free Software
;   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
;  

;
; Source file: serial.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  PC BIOS - ???
;  
;  
;  
;  Copyright (C) 2006-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  --------------------------------------------------------------------
;  
;  This code is based on:
;  
;   ROM BIOS for use with Bochs/Plex86/QEMU emulation environment
;  
;   Copyright (C) 2002  MandrakeSoft S.A.
;  
;     MandrakeSoft S.A.
;     43, rue d'Aboukir
;     75002 Paris - France
;     http://www.linux-mandrake.com/
;     http://www.mandrakesoft.com/
;  
;   This library is free software; you can redistribute it and/or
;   modify it under the terms of the GNU Lesser General Public
;   License as published by the Free Software Foundation; either
;   version 2 of the License, or (at your option) any later version.
;  
;   This library is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;   Lesser General Public License for more details.
;  
;   You should have received a copy of the GNU Lesser General Public
;   License along with this library; if not, write to the Free Software
;   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
;  

;
; Source file: system.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  PC BIOS - ???
;  
;  
;  
;  Copyright (C) 2006-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  --------------------------------------------------------------------
;  
;  This code is based on:
;  
;   ROM BIOS for use with Bochs/Plex86/QEMU emulation environment
;  
;   Copyright (C) 2002  MandrakeSoft S.A.
;  
;     MandrakeSoft S.A.
;     43, rue d'Aboukir
;     75002 Paris - France
;     http://www.linux-mandrake.com/
;     http://www.mandrakesoft.com/
;  
;   This library is free software; you can redistribute it and/or
;   modify it under the terms of the GNU Lesser General Public
;   License as published by the Free Software Foundation; either
;   version 2 of the License, or (at your option) any later version.
;  
;   This library is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;   Lesser General Public License for more details.
;  
;   You should have received a copy of the GNU Lesser General Public
;   License along with this library; if not, write to the Free Software
;   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
;  

;
; Source file: invop.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  Real mode invalid opcode handler.
;  
;  
;  
;  Copyright (C) 2013-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

;
; Source file: timepci.c
;
;  
;  Copyright (C) 2006-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  --------------------------------------------------------------------
;  
;  This code is based on:
;  
;   ROM BIOS for use with Bochs/Plex86/QEMU emulation environment
;  
;   Copyright (C) 2002  MandrakeSoft S.A.
;  
;     MandrakeSoft S.A.
;     43, rue d'Aboukir
;     75002 Paris - France
;     http://www.linux-mandrake.com/
;     http://www.mandrakesoft.com/
;  
;   This library is free software; you can redistribute it and/or
;   modify it under the terms of the GNU Lesser General Public
;   License as published by the Free Software Foundation; either
;   version 2 of the License, or (at your option) any later version.
;  
;   This library is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;   Lesser General Public License for more details.
;  
;   You should have received a copy of the GNU Lesser General Public
;   License along with this library; if not, write to the Free Software
;   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
;  

;
; Source file: ps2mouse.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  PC BIOS - ???
;  
;  
;  
;  Copyright (C) 2006-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  --------------------------------------------------------------------
;  
;  This code is based on:
;  
;   ROM BIOS for use with Bochs/Plex86/QEMU emulation environment
;  
;   Copyright (C) 2002  MandrakeSoft S.A.
;  
;     MandrakeSoft S.A.
;     43, rue d'Aboukir
;     75002 Paris - France
;     http://www.linux-mandrake.com/
;     http://www.mandrakesoft.com/
;  
;   This library is free software; you can redistribute it and/or
;   modify it under the terms of the GNU Lesser General Public
;   License as published by the Free Software Foundation; either
;   version 2 of the License, or (at your option) any later version.
;  
;   This library is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;   Lesser General Public License for more details.
;  
;   You should have received a copy of the GNU Lesser General Public
;   License along with this library; if not, write to the Free Software
;   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
;  

;
; Source file: parallel.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  PC BIOS - ???
;  
;  
;  
;  Copyright (C) 2006-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  --------------------------------------------------------------------
;  
;  This code is based on:
;  
;   ROM BIOS for use with Bochs/Plex86/QEMU emulation environment
;  
;   Copyright (C) 2002  MandrakeSoft S.A.
;  
;     MandrakeSoft S.A.
;     43, rue d'Aboukir
;     75002 Paris - France
;     http://www.linux-mandrake.com/
;     http://www.mandrakesoft.com/
;  
;   This library is free software; you can redistribute it and/or
;   modify it under the terms of the GNU Lesser General Public
;   License as published by the Free Software Foundation; either
;   version 2 of the License, or (at your option) any later version.
;  
;   This library is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;   Lesser General Public License for more details.
;  
;   You should have received a copy of the GNU Lesser General Public
;   License along with this library; if not, write to the Free Software
;   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
;  

;
; Source file: logo.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  Stuff for drawing the BIOS logo.
;  
;  
;  
;  Copyright (C) 2004-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

;
; Source file: scsi.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  SCSI host adapter driver to boot from SCSI disks
;  
;  
;  
;  Copyright (C) 2004-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

;
; Source file: ahci.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  AHCI host adapter driver to boot from SATA disks.
;  
;  
;  
;  Copyright (C) 2011-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

;
; Source file: apm.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  APM BIOS support. Implements APM version 1.2.
;  
;  
;  
;  Copyright (C) 2004-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

;
; Source file: pcibios.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  PCI BIOS support.
;  
;  
;  
;  Copyright (C) 2004-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

;
; Source file: pciutil.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  Utility routines for calling the PCI BIOS.
;  
;  
;  
;  Copyright (C) 2011-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

;
; Source file: vds.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  Utility routines for calling the Virtual DMA Services.
;  
;  
;  
;  Copyright (C) 2011-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

;
; Source file: __U4M.asm
;
;  $Id: VBoxBiosAlternative8086.asm $
;  Compiler support routines.
;  
;  
;  
;  Copyright (C) 2012-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  

;
; Source file: __U4D.asm
;
;  $Id: VBoxBiosAlternative8086.asm $
;  Compiler support routines.
;  
;  
;  
;  Copyright (C) 2012-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  

;
; Source file: __U8RS.asm
;
;  $Id: VBoxBiosAlternative8086.asm $
;  Compiler support routines.
;  
;  
;  
;  Copyright (C) 2012-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  

;
; Source file: __U8LS.asm
;
;  $Id: VBoxBiosAlternative8086.asm $
;  Compiler support routines.
;  
;  
;  
;  Copyright (C) 2012-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  

;
; Source file: fmemset.asm
;
;  $Id: VBoxBiosAlternative8086.asm $
;  Compiler support routines.
;  
;  
;  
;  Copyright (C) 2012-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  

;
; Source file: fmemcpy.asm
;
;  $Id: VBoxBiosAlternative8086.asm $
;  Compiler support routines.
;  
;  
;  
;  Copyright (C) 2012-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  

;
; Source file: pcibio32.asm
;
;  $Id: VBoxBiosAlternative8086.asm $
;  BIOS32 service directory and 32-bit PCI BIOS entry point
;  
;  
;  
;  Copyright (C) 2006-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  --------------------------------------------------------------------

;
; Source file: apm_pm.asm
;
;  $Id:
;  Protected-mode APM implementation.
;  
;  
;  
;  Copyright (C) 2006-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  

;
; Source file: orgs.asm
;
;  $Id: VBoxBiosAlternative8086.asm $
;  ???
;  
;  
;  
;  Copyright (C) 2006-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  --------------------------------------------------------------------
;  
;  This code is based on:
;  
;   ROM BIOS for use with Bochs/Plex86/QEMU emulation environment
;  
;   Copyright (C) 2002  MandrakeSoft S.A.
;  
;     MandrakeSoft S.A.
;     43, rue d'Aboukir
;     75002 Paris - France
;     http://www.linux-mandrake.com/
;     http://www.mandrakesoft.com/
;  
;   This library is free software; you can redistribute it and/or
;   modify it under the terms of the GNU Lesser General Public
;   License as published by the Free Software Foundation; either
;   version 2 of the License, or (at your option) any later version.
;  
;   This library is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;   Lesser General Public License for more details.
;  
;   You should have received a copy of the GNU Lesser General Public
;   License along with this library; if not, write to the Free Software
;   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
;  

;
; Source file: DoUInt32Div.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  AHCI host adapter driver to boot from SATA disks.
;  
;  
;  
;  Copyright (C) 2011-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

;
; Source file: ASMBitLastSetU16.asm
;
;  $Id: VBoxBiosAlternative8086.asm $
;  BiosCommonCode - ASMBitLastSetU16() - borrowed from IPRT.
;  
;  
;  
;  Copyright (C) 2006-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;  

;
; Source file: pci32.c
;
;  $Id: VBoxBiosAlternative8086.asm $
;  32-bit PCI BIOS wrapper.
;  
;  
;  
;  Copyright (C) 2004-2017 Oracle Corporation
;  
;  This file is part of VirtualBox Open Source Edition (OSE), as
;  available from http://www.virtualbox.org. This file is free software;
;  you can redistribute it and/or modify it under the terms of the GNU
;  General Public License (GPL) as published by the Free Software
;  Foundation, in version 2 as it comes in the "COPYING" file of the
;  VirtualBox OSE distribution. VirtualBox OSE is distributed in the
;  hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.




section _DATA progbits vstart=0x0 align=1 ; size=0xb0 class=DATA group=DGROUP
_fd_parm:                                    ; 0xf0000 LB 0x5b
    db  0dfh, 002h, 025h, 002h, 009h, 02ah, 0ffh, 050h, 0f6h, 00fh, 008h, 027h, 080h, 0dfh, 002h, 025h
    db  002h, 009h, 02ah, 0ffh, 050h, 0f6h, 00fh, 008h, 027h, 040h, 0dfh, 002h, 025h, 002h, 00fh, 01bh
    db  0ffh, 054h, 0f6h, 00fh, 008h, 04fh, 000h, 0dfh, 002h, 025h, 002h, 009h, 02ah, 0ffh, 050h, 0f6h
    db  00fh, 008h, 04fh, 080h, 0afh, 002h, 025h, 002h, 012h, 01bh, 0ffh, 06ch, 0f6h, 00fh, 008h, 04fh
    db  000h, 0afh, 002h, 025h, 002h, 024h, 01bh, 0ffh, 054h, 0f6h, 00fh, 008h, 04fh, 0c0h, 0afh, 002h
    db  025h, 002h, 0ffh, 01bh, 0ffh, 054h, 0f6h, 00fh, 008h, 0ffh, 000h
_fd_map:                                     ; 0xf005b LB 0xf
    db  001h, 000h, 002h, 002h, 003h, 003h, 004h, 004h, 005h, 005h, 00eh, 006h, 00fh, 006h, 000h
_pktacc:                                     ; 0xf006a LB 0xc
    db  000h, 000h, 000h, 000h, 000h, 000h, 0c1h, 02bh, 02bh, 082h, 00ch, 092h
_softrst:                                    ; 0xf0076 LB 0xc
    db  000h, 000h, 000h, 000h, 000h, 000h, 04ch, 02eh, 04eh, 03ch, 04eh, 03ch
_dskacc:                                     ; 0xf0082 LB 0x2e
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 0a5h, 02ah, 066h, 02bh, 000h, 000h, 000h, 000h
    db  059h, 080h, 042h, 081h, 0dah, 090h, 084h, 091h, 000h, 000h, 000h, 000h, 000h, 000h, 05fh, 033h
    db  032h, 05fh, 000h, 0dah, 00fh, 000h, 000h, 001h, 0f3h, 000h, 000h, 000h, 000h, 000h

section CONST progbits vstart=0xb0 align=1 ; size=0xcde class=DATA group=DGROUP
    db   'NMI Handler called', 00ah, 000h
    db   'INT18: BOOT FAILURE', 00ah, 000h
    db   '%s', 00ah, 000h, 000h
    db   'FATAL: ', 000h
    db   'bios_printf: unknown %ll format', 00ah, 000h
    db   'bios_printf: unknown format', 00ah, 000h
    db   'ata-detect: Failed to detect ATA device', 00ah, 000h
    db   'ata%d-%d: PCHS=%u/%u/%u LCHS=%u/%u/%u', 00ah, 000h
    db   'ata-detect: Failed to detect ATAPI device', 00ah, 000h
    db   ' slave', 000h
    db   'master', 000h
    db   'ata%d %s: ', 000h
    db   '%c', 000h
    db   ' ATA-%d Hard-Disk (%lu MBytes)', 00ah, 000h
    db   ' ATAPI-%d CD-ROM/DVD-ROM', 00ah, 000h
    db   ' ATAPI-%d Device', 00ah, 000h
    db   'ata%d %s: Unknown device', 00ah, 000h
    db   'ata_cmd_packet', 000h
    db   '%s: DATA_OUT not supported yet', 00ah, 000h
    db   'set_diskette_current_cyl: drive > 1', 00ah, 000h
    db   'int13_diskette_function', 000h
    db   '%s: drive>1 || head>1 ...', 00ah, 000h
    db   '%s: ctrl not ready', 00ah, 000h
    db   '%s: write error', 00ah, 000h
    db   '%s: bad floppy type', 00ah, 000h
    db   '%s: unsupported AH=%02x', 00ah, 000h, 000h
    db   'int13_eltorito', 000h
    db   '%s: call with AX=%04x not implemented.', 00ah, 000h
    db   '%s: unsupported AH=%02x', 00ah, 000h
    db   'int13_cdemu', 000h
    db   '%s: function %02x, emulation not active for DL= %02x', 00ah, 000h
    db   '%s: function %02x, error %02x !', 00ah, 000h
    db   '%s: function AH=%02x unsupported, returns fail', 00ah, 000h
    db   'int13_cdrom', 000h
    db   '%s: function %02x, ELDL out of range %02x', 00ah, 000h
    db   '%s: function %02x, unmapped device for ELDL=%02x', 00ah, 000h
    db   '%s: function %02x. Can', 027h, 't use 64bits lba', 00ah, 000h
    db   '%s: function %02x, status %02x !', 00ah, 000h, 000h
    db   'Booting from %s...', 00ah, 000h
    db   'Boot from %s failed', 00ah, 000h
    db   'Boot from %s %d failed', 00ah, 000h
    db   'No bootable medium found! System halted.', 00ah, 000h
    db   'Could not read from the boot medium! System halted.', 00ah, 000h
    db   'CDROM boot failure code : %04x', 00ah, 000h
    db   'Boot : bseqnr=%d, bootseq=%x', 00dh, 00ah, 000h, 000h
    db   'Keyboard error:%u', 00ah, 000h
    db   'KBD: int09 handler: AL=0', 00ah, 000h
    db   'KBD: int09h_handler(): unknown scancode read: 0x%02x!', 00ah, 000h
    db   'KBD: int09h_handler(): scancode & asciicode are zero?', 00ah, 000h
    db   'KBD: int16h: out of keyboard input', 00ah, 000h
    db   'KBD: unsupported int 16h function %02x', 00ah, 000h
    db   'AX=%04x BX=%04x CX=%04x DX=%04x ', 00ah, 000h, 000h
    db   'int13_harddisk', 000h
    db   '%s: function %02x, ELDL out of range %02x', 00ah, 000h
    db   '%s: function %02x, unmapped device for ELDL=%02x', 00ah, 000h
    db   '%s: function %02x, count out of range!', 00ah, 000h
    db   '%s: function %02x, disk %02x, parameters out of range %04x/%04x/%04x!', 00ah
    db   000h
    db   '%s: function %02x, error %02x !', 00ah, 000h
    db   'format disk track called', 00ah, 000h
    db   '%s: function %02xh unimplemented, returns success', 00ah, 000h
    db   '%s: function %02xh unsupported, returns fail', 00ah, 000h
    db   'int13_harddisk_ext', 000h
    db   '%s: function %02x. LBA out of range', 00ah, 000h, 000h
    db   'int15: Func 24h, subfunc %02xh, A20 gate control not supported', 00ah, 000h
    db   '*** int 15h function AH=bf not yet supported!', 00ah, 000h
    db   'EISA BIOS not present', 00ah, 000h
    db   '*** int 15h function AX=%04x, BX=%04x not yet supported!', 00ah, 000h
    db   'sendmouse', 000h
    db   'setkbdcomm', 000h
    db   'Mouse reset returned %02x (should be ack)', 00ah, 000h
    db   'Mouse status returned %02x (should be ack)', 00ah, 000h
    db   'INT 15h C2 AL=6, BH=%02x', 00ah, 000h
    db   'INT 15h C2 default case entered', 00ah, 000h, 000h
    db   'Key pressed: %x', 00ah, 000h
    db   00ah, 00ah, '  AHCI controller:', 000h
    db   00ah, '    %d) Hard disk', 000h
    db   00ah, 00ah, '  SCSI controller:', 000h
    db   '  IDE controller:', 000h
    db   00ah, 00ah, 'AHCI controller:', 00ah, 000h
    db   00ah, '    %d) ', 000h
    db   'Secondary ', 000h
    db   'Primary ', 000h
    db   'Slave', 000h
    db   'Master', 000h
    db   'No hard disks found', 000h
    db   00ah, 000h
    db   'Press F12 to select boot device.', 00ah, 000h
    db   00ah, 'VirtualBox temporary boot device selection', 00ah, 00ah, 'Detected H'
    db   'ard disks:', 00ah, 00ah, 000h
    db   00ah, 'Other boot devices:', 00ah, ' f) Floppy', 00ah, ' c) CD-ROM', 00ah
    db   ' l) LAN', 00ah, 00ah, ' b) Continue booting', 00ah, 000h
    db   'Delaying boot for %d seconds:', 000h
    db   ' %d', 000h, 000h
    db   'scsi_read_sectors', 000h
    db   '%s: device_id out of range %d', 00ah, 000h
    db   'scsi_write_sectors', 000h
    db   'scsi_cmd_packet', 000h
    db   '%s: DATA_OUT not supported yet', 00ah, 000h
    db   'scsi_enumerate_attached_devices', 000h
    db   '%s: SCSI_INQUIRY failed', 00ah, 000h
    db   '%s: SCSI_READ_CAPACITY failed', 00ah, 000h
    db   'Disk %d has an unsupported sector size of %u', 00ah, 000h
    db   'SCSI %d-ID#%d: LCHS=%lu/%u/%u 0x%llx sectors', 00ah, 000h
    db   'SCSI %d-ID#%d: CD/DVD-ROM', 00ah, 000h, 000h
    db   'ahci_read_sectors', 000h
    db   '%s: device_id out of range %d', 00ah, 000h
    db   'ahci_write_sectors', 000h
    db   'ahci_cmd_packet', 000h
    db   '%s: DATA_OUT not supported yet', 00ah, 000h
    db   'AHCI %d-P#%d: PCHS=%u/%u/%u LCHS=%u/%u/%u 0x%llx sectors', 00ah, 000h
    db   'Standby', 000h
    db   'Suspend', 000h
    db   'Shutdown', 000h
    db   'APM: Unsupported function AX=%04X BX=%04X called', 00ah, 000h, 000h
    db   'PCI: Unsupported function AX=%04X BX=%04X called', 00ah, 000h

section CONST2 progbits vstart=0xd8e align=1 ; size=0x3fa class=DATA group=DGROUP
_bios_cvs_version_string:                    ; 0xf0d8e LB 0x12
    db  'VirtualBox 5.2.22', 000h
_bios_prefix_string:                         ; 0xf0da0 LB 0x8
    db  'BIOS: ', 000h, 000h
_isotag:                                     ; 0xf0da8 LB 0x6
    db  'CD001', 000h
_eltorito:                                   ; 0xf0dae LB 0x18
    db  'EL TORITO SPECIFICATION', 000h
_drivetypes:                                 ; 0xf0dc6 LB 0x28
    db  046h, 06ch, 06fh, 070h, 070h, 079h, 000h, 000h, 000h, 000h, 048h, 061h, 072h, 064h, 020h, 044h
    db  069h, 073h, 06bh, 000h, 043h, 044h, 02dh, 052h, 04fh, 04dh, 000h, 000h, 000h, 000h, 04ch, 041h
    db  04eh, 000h, 000h, 000h, 000h, 000h, 000h, 000h
_scan_to_scanascii:                          ; 0xf0dee LB 0x37a
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 01bh, 001h, 01bh, 001h, 01bh, 001h
    db  000h, 001h, 000h, 000h, 031h, 002h, 021h, 002h, 000h, 000h, 000h, 078h, 000h, 000h, 032h, 003h
    db  040h, 003h, 000h, 003h, 000h, 079h, 000h, 000h, 033h, 004h, 023h, 004h, 000h, 000h, 000h, 07ah
    db  000h, 000h, 034h, 005h, 024h, 005h, 000h, 000h, 000h, 07bh, 000h, 000h, 035h, 006h, 025h, 006h
    db  000h, 000h, 000h, 07ch, 000h, 000h, 036h, 007h, 05eh, 007h, 01eh, 007h, 000h, 07dh, 000h, 000h
    db  037h, 008h, 026h, 008h, 000h, 000h, 000h, 07eh, 000h, 000h, 038h, 009h, 02ah, 009h, 000h, 000h
    db  000h, 07fh, 000h, 000h, 039h, 00ah, 028h, 00ah, 000h, 000h, 000h, 080h, 000h, 000h, 030h, 00bh
    db  029h, 00bh, 000h, 000h, 000h, 081h, 000h, 000h, 02dh, 00ch, 05fh, 00ch, 01fh, 00ch, 000h, 082h
    db  000h, 000h, 03dh, 00dh, 02bh, 00dh, 000h, 000h, 000h, 083h, 000h, 000h, 008h, 00eh, 008h, 00eh
    db  07fh, 00eh, 000h, 000h, 000h, 000h, 009h, 00fh, 000h, 00fh, 000h, 000h, 000h, 000h, 000h, 000h
    db  071h, 010h, 051h, 010h, 011h, 010h, 000h, 010h, 040h, 000h, 077h, 011h, 057h, 011h, 017h, 011h
    db  000h, 011h, 040h, 000h, 065h, 012h, 045h, 012h, 005h, 012h, 000h, 012h, 040h, 000h, 072h, 013h
    db  052h, 013h, 012h, 013h, 000h, 013h, 040h, 000h, 074h, 014h, 054h, 014h, 014h, 014h, 000h, 014h
    db  040h, 000h, 079h, 015h, 059h, 015h, 019h, 015h, 000h, 015h, 040h, 000h, 075h, 016h, 055h, 016h
    db  015h, 016h, 000h, 016h, 040h, 000h, 069h, 017h, 049h, 017h, 009h, 017h, 000h, 017h, 040h, 000h
    db  06fh, 018h, 04fh, 018h, 00fh, 018h, 000h, 018h, 040h, 000h, 070h, 019h, 050h, 019h, 010h, 019h
    db  000h, 019h, 040h, 000h, 05bh, 01ah, 07bh, 01ah, 01bh, 01ah, 000h, 000h, 000h, 000h, 05dh, 01bh
    db  07dh, 01bh, 01dh, 01bh, 000h, 000h, 000h, 000h, 00dh, 01ch, 00dh, 01ch, 00ah, 01ch, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 061h, 01eh, 041h, 01eh
    db  001h, 01eh, 000h, 01eh, 040h, 000h, 073h, 01fh, 053h, 01fh, 013h, 01fh, 000h, 01fh, 040h, 000h
    db  064h, 020h, 044h, 020h, 004h, 020h, 000h, 020h, 040h, 000h, 066h, 021h, 046h, 021h, 006h, 021h
    db  000h, 021h, 040h, 000h, 067h, 022h, 047h, 022h, 007h, 022h, 000h, 022h, 040h, 000h, 068h, 023h
    db  048h, 023h, 008h, 023h, 000h, 023h, 040h, 000h, 06ah, 024h, 04ah, 024h, 00ah, 024h, 000h, 024h
    db  040h, 000h, 06bh, 025h, 04bh, 025h, 00bh, 025h, 000h, 025h, 040h, 000h, 06ch, 026h, 04ch, 026h
    db  00ch, 026h, 000h, 026h, 040h, 000h, 03bh, 027h, 03ah, 027h, 000h, 000h, 000h, 000h, 000h, 000h
    db  027h, 028h, 022h, 028h, 000h, 000h, 000h, 000h, 000h, 000h, 060h, 029h, 07eh, 029h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 05ch, 02bh
    db  07ch, 02bh, 01ch, 02bh, 000h, 000h, 000h, 000h, 07ah, 02ch, 05ah, 02ch, 01ah, 02ch, 000h, 02ch
    db  040h, 000h, 078h, 02dh, 058h, 02dh, 018h, 02dh, 000h, 02dh, 040h, 000h, 063h, 02eh, 043h, 02eh
    db  003h, 02eh, 000h, 02eh, 040h, 000h, 076h, 02fh, 056h, 02fh, 016h, 02fh, 000h, 02fh, 040h, 000h
    db  062h, 030h, 042h, 030h, 002h, 030h, 000h, 030h, 040h, 000h, 06eh, 031h, 04eh, 031h, 00eh, 031h
    db  000h, 031h, 040h, 000h, 06dh, 032h, 04dh, 032h, 00dh, 032h, 000h, 032h, 040h, 000h, 02ch, 033h
    db  03ch, 033h, 000h, 000h, 000h, 000h, 000h, 000h, 02eh, 034h, 03eh, 034h, 000h, 000h, 000h, 000h
    db  000h, 000h, 02fh, 035h, 03fh, 035h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 02ah, 037h, 02ah, 037h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 020h, 039h, 020h, 039h, 020h, 039h
    db  020h, 039h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 03bh
    db  000h, 054h, 000h, 05eh, 000h, 068h, 000h, 000h, 000h, 03ch, 000h, 055h, 000h, 05fh, 000h, 069h
    db  000h, 000h, 000h, 03dh, 000h, 056h, 000h, 060h, 000h, 06ah, 000h, 000h, 000h, 03eh, 000h, 057h
    db  000h, 061h, 000h, 06bh, 000h, 000h, 000h, 03fh, 000h, 058h, 000h, 062h, 000h, 06ch, 000h, 000h
    db  000h, 040h, 000h, 059h, 000h, 063h, 000h, 06dh, 000h, 000h, 000h, 041h, 000h, 05ah, 000h, 064h
    db  000h, 06eh, 000h, 000h, 000h, 042h, 000h, 05bh, 000h, 065h, 000h, 06fh, 000h, 000h, 000h, 043h
    db  000h, 05ch, 000h, 066h, 000h, 070h, 000h, 000h, 000h, 044h, 000h, 05dh, 000h, 067h, 000h, 071h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 047h, 037h, 047h, 000h, 077h, 000h, 000h, 020h, 000h
    db  000h, 048h, 038h, 048h, 000h, 000h, 000h, 000h, 020h, 000h, 000h, 049h, 039h, 049h, 000h, 084h
    db  000h, 000h, 020h, 000h, 02dh, 04ah, 02dh, 04ah, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 04bh
    db  034h, 04bh, 000h, 073h, 000h, 000h, 020h, 000h, 000h, 04ch, 035h, 04ch, 000h, 000h, 000h, 000h
    db  020h, 000h, 000h, 04dh, 036h, 04dh, 000h, 074h, 000h, 000h, 020h, 000h, 02bh, 04eh, 02bh, 04eh
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 04fh, 031h, 04fh, 000h, 075h, 000h, 000h, 020h, 000h
    db  000h, 050h, 032h, 050h, 000h, 000h, 000h, 000h, 020h, 000h, 000h, 051h, 033h, 051h, 000h, 076h
    db  000h, 000h, 020h, 000h, 000h, 052h, 030h, 052h, 000h, 000h, 000h, 000h, 020h, 000h, 000h, 053h
    db  02eh, 053h, 000h, 000h, 000h, 000h, 020h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 05ch, 056h, 07ch, 056h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 085h, 000h, 087h, 000h, 089h, 000h, 08bh, 000h, 000h
    db  000h, 086h, 000h, 088h, 000h, 08ah, 000h, 08ch, 000h, 000h
_panic_msg_keyb_buffer_full:                 ; 0xf1168 LB 0x20
    db  '%s: keyboard input buffer full', 00ah, 000h

  ; Padding 0x478 bytes at 0xf1188
  times 1144 db 0

section _TEXT progbits vstart=0x1600 align=1 ; size=0x8ed8 class=CODE group=AUTO
rom_scan_:                                   ; 0xf1600 LB 0x52
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    push ax                                   ; 50
    mov bx, ax                                ; 89 c3
    mov di, dx                                ; 89 d7
    cmp bx, di                                ; 39 fb
    jnc short 01649h                          ; 73 38
    xor si, si                                ; 31 f6
    mov dx, bx                                ; 89 da
    mov es, bx                                ; 8e c3
    cmp word [es:si], 0aa55h                  ; 26 81 3c 55 aa
    jne short 01643h                          ; 75 25
    mov word [bp-00ah], bx                    ; 89 5e f6
    mov word [bp-00ch], strict word 00003h    ; c7 46 f4 03 00
    call far [bp-00ch]                        ; ff 5e f4
    cli                                       ; fa
    mov es, bx                                ; 8e c3
    mov al, byte [es:si+002h]                 ; 26 8a 44 02
    add AL, strict byte 003h                  ; 04 03
    and AL, strict byte 0fch                  ; 24 fc
    xor ah, ah                                ; 30 e4
    cwd                                       ; 99
    mov CL, strict byte 002h                  ; b1 02
    sal dx, CL                                ; d3 e2
    db  01bh, 0c2h
    ; sbb ax, dx                                ; 1b c2
    sar ax, CL                                ; d3 f8
    add bx, ax                                ; 01 c3
    jmp short 0160dh                          ; eb ca
    add bx, 00080h                            ; 81 c3 80 00
    jmp short 0160dh                          ; eb c4
    lea sp, [bp-008h]                         ; 8d 66 f8
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
read_byte_:                                  ; 0xf1652 LB 0xe
    push bx                                   ; 53
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov bx, dx                                ; 89 d3
    mov es, ax                                ; 8e c0
    mov al, byte [es:bx]                      ; 26 8a 07
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    retn                                      ; c3
write_byte_:                                 ; 0xf1660 LB 0xe
    push si                                   ; 56
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov si, dx                                ; 89 d6
    mov es, ax                                ; 8e c0
    mov byte [es:si], bl                      ; 26 88 1c
    pop bp                                    ; 5d
    pop si                                    ; 5e
    retn                                      ; c3
read_word_:                                  ; 0xf166e LB 0xe
    push bx                                   ; 53
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov bx, dx                                ; 89 d3
    mov es, ax                                ; 8e c0
    mov ax, word [es:bx]                      ; 26 8b 07
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    retn                                      ; c3
write_word_:                                 ; 0xf167c LB 0xe
    push si                                   ; 56
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov si, dx                                ; 89 d6
    mov es, ax                                ; 8e c0
    mov word [es:si], bx                      ; 26 89 1c
    pop bp                                    ; 5d
    pop si                                    ; 5e
    retn                                      ; c3
read_dword_:                                 ; 0xf168a LB 0x12
    push bx                                   ; 53
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov bx, dx                                ; 89 d3
    mov es, ax                                ; 8e c0
    mov ax, word [es:bx]                      ; 26 8b 07
    mov dx, word [es:bx+002h]                 ; 26 8b 57 02
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    retn                                      ; c3
write_dword_:                                ; 0xf169c LB 0x12
    push si                                   ; 56
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov si, dx                                ; 89 d6
    mov es, ax                                ; 8e c0
    mov word [es:si], bx                      ; 26 89 1c
    mov word [es:si+002h], cx                 ; 26 89 4c 02
    pop bp                                    ; 5d
    pop si                                    ; 5e
    retn                                      ; c3
inb_cmos_:                                   ; 0xf16ae LB 0x1b
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push dx                                   ; 52
    mov AH, strict byte 070h                  ; b4 70
    cmp AL, strict byte 080h                  ; 3c 80
    jc short 016bah                           ; 72 02
    mov AH, strict byte 072h                  ; b4 72
    mov dl, ah                                ; 88 e2
    xor dh, dh                                ; 30 f6
    out DX, AL                                ; ee
    inc dx                                    ; 42
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop dx                                    ; 5a
    pop bp                                    ; 5d
    retn                                      ; c3
outb_cmos_:                                  ; 0xf16c9 LB 0x1d
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    mov bl, dl                                ; 88 d3
    mov AH, strict byte 070h                  ; b4 70
    cmp AL, strict byte 080h                  ; 3c 80
    jc short 016d7h                           ; 72 02
    mov AH, strict byte 072h                  ; b4 72
    mov dl, ah                                ; 88 e2
    xor dh, dh                                ; 30 f6
    out DX, AL                                ; ee
    inc dx                                    ; 42
    mov al, bl                                ; 88 d8
    out DX, AL                                ; ee
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
_dummy_isr_function:                         ; 0xf16e6 LB 0x65
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push ax                                   ; 50
    mov CL, strict byte 0ffh                  ; b1 ff
    mov AL, strict byte 00bh                  ; b0 0b
    mov dx, strict word 00020h                ; ba 20 00
    out DX, AL                                ; ee
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov bx, ax                                ; 89 c3
    mov byte [bp-002h], al                    ; 88 46 fe
    test al, al                               ; 84 c0
    je short 0173ah                           ; 74 3c
    mov AL, strict byte 00bh                  ; b0 0b
    mov dx, 000a0h                            ; ba a0 00
    out DX, AL                                ; ee
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov cx, ax                                ; 89 c1
    test al, al                               ; 84 c0
    je short 01722h                           ; 74 15
    mov dx, 000a1h                            ; ba a1 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov bl, al                                ; 88 c3
    mov al, cl                                ; 88 c8
    or al, bl                                 ; 08 d8
    out DX, AL                                ; ee
    mov AL, strict byte 020h                  ; b0 20
    mov dx, 000a0h                            ; ba a0 00
    out DX, AL                                ; ee
    jmp short 01731h                          ; eb 0f
    mov dx, strict word 00021h                ; ba 21 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and bl, 0fbh                              ; 80 e3 fb
    mov byte [bp-002h], bl                    ; 88 5e fe
    or al, bl                                 ; 08 d8
    out DX, AL                                ; ee
    mov AL, strict byte 020h                  ; b0 20
    mov dx, strict word 00020h                ; ba 20 00
    out DX, AL                                ; ee
    mov cl, byte [bp-002h]                    ; 8a 4e fe
    mov bl, cl                                ; 88 cb
    xor bh, bh                                ; 30 ff
    mov dx, strict word 0006bh                ; ba 6b 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 19 ff
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
_nmi_handler_msg:                            ; 0xf174b LB 0x15
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov ax, 000b0h                            ; b8 b0 00
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 1d 02
    add sp, strict byte 00004h                ; 83 c4 04
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
_int18_panic_msg:                            ; 0xf1760 LB 0x15
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov ax, 000c4h                            ; b8 c4 00
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 08 02
    add sp, strict byte 00004h                ; 83 c4 04
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
_log_bios_start:                             ; 0xf1775 LB 0x24
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 b0 01
    mov ax, 00d8eh                            ; b8 8e 0d
    push ax                                   ; 50
    mov ax, 000d9h                            ; b8 d9 00
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 e4 01
    add sp, strict byte 00006h                ; 83 c4 06
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
_print_bios_banner:                          ; 0xf1799 LB 0x2e
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov dx, strict word 00072h                ; ba 72 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 c9 fe
    mov cx, ax                                ; 89 c1
    xor bx, bx                                ; 31 db
    mov dx, strict word 00072h                ; ba 72 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0167ch                               ; e8 ca fe
    cmp cx, 01234h                            ; 81 f9 34 12
    jne short 017c0h                          ; 75 08
    mov AL, strict byte 003h                  ; b0 03
    mov AH, strict byte 000h                  ; b4 00
    int 010h                                  ; cd 10
    jmp short 017c3h                          ; eb 03
    call 07beeh                               ; e8 2b 64
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
send_:                                       ; 0xf17c7 LB 0x3b
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    mov bx, ax                                ; 89 c3
    mov cl, dl                                ; 88 d1
    test AL, strict byte 008h                 ; a8 08
    je short 017dah                           ; 74 06
    mov al, dl                                ; 88 d0
    mov dx, 00403h                            ; ba 03 04
    out DX, AL                                ; ee
    test bl, 004h                             ; f6 c3 04
    je short 017e5h                           ; 74 06
    mov al, cl                                ; 88 c8
    mov dx, 00504h                            ; ba 04 05
    out DX, AL                                ; ee
    test bl, 002h                             ; f6 c3 02
    je short 017fbh                           ; 74 11
    cmp cl, 00ah                              ; 80 f9 0a
    jne short 017f5h                          ; 75 06
    mov AL, strict byte 00dh                  ; b0 0d
    mov AH, strict byte 00eh                  ; b4 0e
    int 010h                                  ; cd 10
    mov al, cl                                ; 88 c8
    mov AH, strict byte 00eh                  ; b4 0e
    int 010h                                  ; cd 10
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
put_int_:                                    ; 0xf1802 LB 0x63
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    push ax                                   ; 50
    mov si, ax                                ; 89 c6
    mov word [bp-008h], dx                    ; 89 56 f8
    mov di, bx                                ; 89 df
    mov bx, strict word 0000ah                ; bb 0a 00
    mov ax, dx                                ; 89 d0
    cwd                                       ; 99
    idiv bx                                   ; f7 fb
    mov word [bp-006h], ax                    ; 89 46 fa
    test ax, ax                               ; 85 c0
    je short 0182bh                           ; 74 0c
    lea bx, [di-001h]                         ; 8d 5d ff
    mov dx, ax                                ; 89 c2
    mov ax, si                                ; 89 f0
    call 01802h                               ; e8 d9 ff
    jmp short 01846h                          ; eb 1b
    dec di                                    ; 4f
    test di, di                               ; 85 ff
    jle short 0183ah                          ; 7e 0a
    mov dx, strict word 00020h                ; ba 20 00
    mov ax, si                                ; 89 f0
    call 017c7h                               ; e8 8f ff
    jmp short 0182bh                          ; eb f1
    test cx, cx                               ; 85 c9
    je short 01846h                           ; 74 08
    mov dx, strict word 0002dh                ; ba 2d 00
    mov ax, si                                ; 89 f0
    call 017c7h                               ; e8 81 ff
    mov al, byte [bp-006h]                    ; 8a 46 fa
    mov BL, strict byte 00ah                  ; b3 0a
    mul bl                                    ; f6 e3
    mov bl, byte [bp-008h]                    ; 8a 5e f8
    sub bl, al                                ; 28 c3
    add bl, 030h                              ; 80 c3 30
    xor bh, bh                                ; 30 ff
    mov dx, bx                                ; 89 da
    mov ax, si                                ; 89 f0
    call 017c7h                               ; e8 69 ff
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
put_uint_:                                   ; 0xf1865 LB 0x5e
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    push ax                                   ; 50
    mov si, ax                                ; 89 c6
    mov word [bp-008h], dx                    ; 89 56 f8
    mov ax, dx                                ; 89 d0
    xor dx, dx                                ; 31 d2
    mov di, strict word 0000ah                ; bf 0a 00
    div di                                    ; f7 f7
    mov word [bp-006h], ax                    ; 89 46 fa
    test ax, ax                               ; 85 c0
    je short 0188bh                           ; 74 0a
    dec bx                                    ; 4b
    mov dx, ax                                ; 89 c2
    mov ax, si                                ; 89 f0
    call 01865h                               ; e8 dc ff
    jmp short 018a6h                          ; eb 1b
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jle short 0189ah                          ; 7e 0a
    mov dx, strict word 00020h                ; ba 20 00
    mov ax, si                                ; 89 f0
    call 017c7h                               ; e8 2f ff
    jmp short 0188bh                          ; eb f1
    test cx, cx                               ; 85 c9
    je short 018a6h                           ; 74 08
    mov dx, strict word 0002dh                ; ba 2d 00
    mov ax, si                                ; 89 f0
    call 017c7h                               ; e8 21 ff
    mov al, byte [bp-006h]                    ; 8a 46 fa
    mov DL, strict byte 00ah                  ; b2 0a
    mul dl                                    ; f6 e2
    mov dl, byte [bp-008h]                    ; 8a 56 f8
    sub dl, al                                ; 28 c2
    add dl, 030h                              ; 80 c2 30
    xor dh, dh                                ; 30 f6
    mov ax, si                                ; 89 f0
    call 017c7h                               ; e8 0b ff
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
put_luint_:                                  ; 0xf18c3 LB 0x70
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    push ax                                   ; 50
    mov si, ax                                ; 89 c6
    mov word [bp-006h], bx                    ; 89 5e fa
    mov di, dx                                ; 89 d7
    mov ax, bx                                ; 89 d8
    mov dx, cx                                ; 89 ca
    mov bx, strict word 0000ah                ; bb 0a 00
    xor cx, cx                                ; 31 c9
    call 0a170h                               ; e8 93 88
    mov word [bp-008h], ax                    ; 89 46 f8
    mov cx, dx                                ; 89 d1
    mov dx, ax                                ; 89 c2
    or dx, cx                                 ; 09 ca
    je short 018f7h                           ; 74 0f
    push word [bp+004h]                       ; ff 76 04
    lea dx, [di-001h]                         ; 8d 55 ff
    mov bx, ax                                ; 89 c3
    mov ax, si                                ; 89 f0
    call 018c3h                               ; e8 ce ff
    jmp short 01914h                          ; eb 1d
    dec di                                    ; 4f
    test di, di                               ; 85 ff
    jle short 01906h                          ; 7e 0a
    mov dx, strict word 00020h                ; ba 20 00
    mov ax, si                                ; 89 f0
    call 017c7h                               ; e8 c3 fe
    jmp short 018f7h                          ; eb f1
    cmp word [bp+004h], strict byte 00000h    ; 83 7e 04 00
    je short 01914h                           ; 74 08
    mov dx, strict word 0002dh                ; ba 2d 00
    mov ax, si                                ; 89 f0
    call 017c7h                               ; e8 b3 fe
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov DL, strict byte 00ah                  ; b2 0a
    mul dl                                    ; f6 e2
    mov dl, byte [bp-006h]                    ; 8a 56 fa
    sub dl, al                                ; 28 c2
    add dl, 030h                              ; 80 c2 30
    xor dh, dh                                ; 30 f6
    mov ax, si                                ; 89 f0
    call 017c7h                               ; e8 9d fe
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 00002h                               ; c2 02 00
put_str_:                                    ; 0xf1933 LB 0x21
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push dx                                   ; 52
    push si                                   ; 56
    mov si, ax                                ; 89 c6
    mov es, cx                                ; 8e c1
    mov dl, byte [es:bx]                      ; 26 8a 17
    test dl, dl                               ; 84 d2
    je short 0194dh                           ; 74 0a
    xor dh, dh                                ; 30 f6
    mov ax, si                                ; 89 f0
    call 017c7h                               ; e8 7d fe
    inc bx                                    ; 43
    jmp short 0193ah                          ; eb ed
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop bp                                    ; 5d
    retn                                      ; c3
put_str_near_:                               ; 0xf1954 LB 0x22
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    mov cx, ax                                ; 89 c1
    mov bx, dx                                ; 89 d3
    mov al, byte [bx]                         ; 8a 07
    test al, al                               ; 84 c0
    je short 0196fh                           ; 74 0c
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    mov ax, cx                                ; 89 c8
    call 017c7h                               ; e8 5b fe
    inc bx                                    ; 43
    jmp short 0195dh                          ; eb ee
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
bios_printf_:                                ; 0xf1976 LB 0x34f
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 0001ch                ; 83 ec 1c
    lea bx, [bp+008h]                         ; 8d 5e 08
    mov word [bp-016h], bx                    ; 89 5e ea
    mov [bp-014h], ss                         ; 8c 56 ec
    xor cx, cx                                ; 31 c9
    xor di, di                                ; 31 ff
    mov ax, word [bp+004h]                    ; 8b 46 04
    and ax, strict word 00007h                ; 25 07 00
    cmp ax, strict word 00007h                ; 3d 07 00
    jne short 019a7h                          ; 75 0e
    mov ax, 000deh                            ; b8 de 00
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 d2 ff
    add sp, strict byte 00004h                ; 83 c4 04
    mov bx, word [bp+006h]                    ; 8b 5e 06
    mov dl, byte [bx]                         ; 8a 17
    test dl, dl                               ; 84 d2
    je short 01a11h                           ; 74 61
    cmp dl, 025h                              ; 80 fa 25
    jne short 019bdh                          ; 75 08
    mov cx, strict word 00001h                ; b9 01 00
    xor di, di                                ; 31 ff
    jmp near 01ca3h                           ; e9 e6 02
    test cx, cx                               ; 85 c9
    je short 01a14h                           ; 74 53
    cmp dl, 030h                              ; 80 fa 30
    jc short 019e0h                           ; 72 1a
    cmp dl, 039h                              ; 80 fa 39
    jnbe short 019e0h                         ; 77 15
    mov bl, dl                                ; 88 d3
    xor bh, bh                                ; 30 ff
    mov ax, di                                ; 89 f8
    mov dx, strict word 0000ah                ; ba 0a 00
    mul dx                                    ; f7 e2
    sub bx, strict byte 00030h                ; 83 eb 30
    mov di, ax                                ; 89 c7
    add di, bx                                ; 01 df
    jmp near 01ca3h                           ; e9 c3 02
    mov ax, word [bp-014h]                    ; 8b 46 ec
    mov word [bp-014h], ax                    ; 89 46 ec
    add word [bp-016h], strict byte 00002h    ; 83 46 ea 02
    les bx, [bp-016h]                         ; c4 5e ea
    mov ax, word [es:bx-002h]                 ; 26 8b 47 fe
    mov word [bp-00ch], ax                    ; 89 46 f4
    cmp dl, 078h                              ; 80 fa 78
    je short 019feh                           ; 74 05
    cmp dl, 058h                              ; 80 fa 58
    jne short 01a59h                          ; 75 5b
    test di, di                               ; 85 ff
    jne short 01a05h                          ; 75 03
    mov di, strict word 00004h                ; bf 04 00
    cmp dl, 078h                              ; 80 fa 78
    jne short 01a17h                          ; 75 0d
    mov word [bp-00eh], strict word 00061h    ; c7 46 f2 61 00
    jmp short 01a1ch                          ; eb 0b
    jmp near 01ca9h                           ; e9 95 02
    jmp near 01c9bh                           ; e9 84 02
    mov word [bp-00eh], strict word 00041h    ; c7 46 f2 41 00
    lea ax, [di-001h]                         ; 8d 45 ff
    mov word [bp-012h], ax                    ; 89 46 ee
    mov ax, word [bp-012h]                    ; 8b 46 ee
    test ax, ax                               ; 85 c0
    jl short 01a6ah                           ; 7c 41
    mov cx, ax                                ; 89 c1
    sal cx, 1                                 ; d1 e1
    sal cx, 1                                 ; d1 e1
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    shr ax, CL                                ; d3 e8
    xor ah, ah                                ; 30 e4
    and AL, strict byte 00fh                  ; 24 0f
    cmp ax, strict word 00009h                ; 3d 09 00
    jnbe short 01a44h                         ; 77 07
    mov dx, ax                                ; 89 c2
    add dx, strict byte 00030h                ; 83 c2 30
    jmp short 01a4ch                          ; eb 08
    sub ax, strict word 0000ah                ; 2d 0a 00
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    add dx, ax                                ; 01 c2
    xor dh, dh                                ; 30 f6
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 017c7h                               ; e8 73 fd
    dec word [bp-012h]                        ; ff 4e ee
    jmp short 01a22h                          ; eb c9
    cmp dl, 075h                              ; 80 fa 75
    jne short 01a6dh                          ; 75 0f
    xor cx, cx                                ; 31 c9
    mov bx, di                                ; 89 fb
    mov dx, ax                                ; 89 c2
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01865h                               ; e8 fb fd
    jmp near 01c97h                           ; e9 2a 02
    cmp dl, 06ch                              ; 80 fa 6c
    jne short 01a7ah                          ; 75 08
    mov bx, word [bp+006h]                    ; 8b 5e 06
    cmp dl, byte [bx+001h]                    ; 3a 57 01
    je short 01a7dh                           ; 74 03
    jmp near 01b52h                           ; e9 d5 00
    add word [bp+006h], strict byte 00002h    ; 83 46 06 02
    mov bx, word [bp+006h]                    ; 8b 5e 06
    mov dl, byte [bx]                         ; 8a 17
    mov word [bp-026h], ax                    ; 89 46 da
    mov ax, word [bp-014h]                    ; 8b 46 ec
    mov word [bp-014h], ax                    ; 89 46 ec
    add word [bp-016h], strict byte 00002h    ; 83 46 ea 02
    les bx, [bp-016h]                         ; c4 5e ea
    mov ax, word [es:bx-002h]                 ; 26 8b 47 fe
    mov word [bp-024h], ax                    ; 89 46 dc
    mov ax, word [bp-014h]                    ; 8b 46 ec
    mov word [bp-014h], ax                    ; 89 46 ec
    add word [bp-016h], strict byte 00002h    ; 83 46 ea 02
    les bx, [bp-016h]                         ; c4 5e ea
    mov ax, word [es:bx-002h]                 ; 26 8b 47 fe
    mov word [bp-022h], ax                    ; 89 46 de
    mov ax, word [bp-014h]                    ; 8b 46 ec
    mov word [bp-014h], ax                    ; 89 46 ec
    add word [bp-016h], strict byte 00002h    ; 83 46 ea 02
    les bx, [bp-016h]                         ; c4 5e ea
    mov ax, word [es:bx-002h]                 ; 26 8b 47 fe
    mov word [bp-020h], ax                    ; 89 46 e0
    cmp dl, 078h                              ; 80 fa 78
    je short 01acfh                           ; 74 05
    cmp dl, 058h                              ; 80 fa 58
    jne short 01b2ah                          ; 75 5b
    test di, di                               ; 85 ff
    jne short 01ad6h                          ; 75 03
    mov di, strict word 00010h                ; bf 10 00
    cmp dl, 078h                              ; 80 fa 78
    jne short 01ae2h                          ; 75 07
    mov word [bp-00eh], strict word 00061h    ; c7 46 f2 61 00
    jmp short 01ae7h                          ; eb 05
    mov word [bp-00eh], strict word 00041h    ; c7 46 f2 41 00
    lea ax, [di-001h]                         ; 8d 45 ff
    mov word [bp-012h], ax                    ; 89 46 ee
    mov ax, word [bp-012h]                    ; 8b 46 ee
    test ax, ax                               ; 85 c0
    jl short 01b4fh                           ; 7c 5b
    sal ax, 1                                 ; d1 e0
    sal ax, 1                                 ; d1 e0
    mov word [bp-01eh], ax                    ; 89 46 e2
    xor ax, ax                                ; 31 c0
    mov word [bp-01ch], ax                    ; 89 46 e4
    mov word [bp-01ah], ax                    ; 89 46 e6
    mov word [bp-018h], ax                    ; 89 46 e8
    mov ax, word [bp-020h]                    ; 8b 46 e0
    mov bx, word [bp-022h]                    ; 8b 5e de
    mov cx, word [bp-024h]                    ; 8b 4e dc
    mov dx, word [bp-026h]                    ; 8b 56 da
    mov si, word [bp-01eh]                    ; 8b 76 e2
    call 0a1a0h                               ; e8 88 86
    mov ax, dx                                ; 89 d0
    xor ah, dh                                ; 30 f4
    and AL, strict byte 00fh                  ; 24 0f
    cmp ax, strict word 00009h                ; 3d 09 00
    jnbe short 01b2ch                         ; 77 09
    mov dx, ax                                ; 89 c2
    add dx, strict byte 00030h                ; 83 c2 30
    jmp short 01b34h                          ; eb 0a
    jmp short 01b41h                          ; eb 15
    sub ax, strict word 0000ah                ; 2d 0a 00
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    add dx, ax                                ; 01 c2
    xor dh, dh                                ; 30 f6
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 017c7h                               ; e8 8b fc
    dec word [bp-012h]                        ; ff 4e ee
    jmp short 01aedh                          ; eb ac
    mov ax, 000e6h                            ; b8 e6 00
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 2a fe
    add sp, strict byte 00004h                ; 83 c4 04
    jmp near 01c97h                           ; e9 45 01
    lea bx, [di-001h]                         ; 8d 5d ff
    cmp dl, 06ch                              ; 80 fa 6c
    jne short 01bb2h                          ; 75 58
    inc word [bp+006h]                        ; ff 46 06
    mov si, word [bp+006h]                    ; 8b 76 06
    mov dl, byte [si]                         ; 8a 14
    mov ax, word [bp-014h]                    ; 8b 46 ec
    mov word [bp-014h], ax                    ; 89 46 ec
    add word [bp-016h], strict byte 00002h    ; 83 46 ea 02
    les si, [bp-016h]                         ; c4 76 ea
    mov ax, word [es:si-002h]                 ; 26 8b 44 fe
    mov word [bp-010h], ax                    ; 89 46 f0
    cmp dl, 064h                              ; 80 fa 64
    jne short 01babh                          ; 75 30
    test byte [bp-00fh], 080h                 ; f6 46 f1 80
    je short 01b98h                           ; 74 17
    mov ax, strict word 00001h                ; b8 01 00
    push ax                                   ; 50
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    mov cx, word [bp-010h]                    ; 8b 4e f0
    neg cx                                    ; f7 d9
    neg ax                                    ; f7 d8
    sbb cx, strict byte 00000h                ; 83 d9 00
    mov dx, bx                                ; 89 da
    mov bx, ax                                ; 89 c3
    jmp short 01ba3h                          ; eb 0b
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    mov bx, word [bp-00ch]                    ; 8b 5e f4
    mov dx, di                                ; 89 fa
    mov cx, word [bp-010h]                    ; 8b 4e f0
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 018c3h                               ; e8 1a fd
    jmp short 01b4fh                          ; eb a4
    cmp dl, 075h                              ; 80 fa 75
    jne short 01bb4h                          ; 75 04
    jmp short 01b98h                          ; eb e6
    jmp short 01c1bh                          ; eb 67
    cmp dl, 078h                              ; 80 fa 78
    je short 01bbeh                           ; 74 05
    cmp dl, 058h                              ; 80 fa 58
    jne short 01b4fh                          ; 75 91
    test di, di                               ; 85 ff
    jne short 01bc5h                          ; 75 03
    mov di, strict word 00008h                ; bf 08 00
    cmp dl, 078h                              ; 80 fa 78
    jne short 01bd1h                          ; 75 07
    mov word [bp-00eh], strict word 00061h    ; c7 46 f2 61 00
    jmp short 01bd6h                          ; eb 05
    mov word [bp-00eh], strict word 00041h    ; c7 46 f2 41 00
    lea ax, [di-001h]                         ; 8d 45 ff
    mov word [bp-012h], ax                    ; 89 46 ee
    cmp word [bp-012h], strict byte 00000h    ; 83 7e ee 00
    jl short 01c3dh                           ; 7c 5b
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    mov cx, word [bp-012h]                    ; 8b 4e ee
    sal cx, 1                                 ; d1 e1
    sal cx, 1                                 ; d1 e1
    mov dx, word [bp-010h]                    ; 8b 56 f0
    jcxz 01bf7h                               ; e3 06
    shr dx, 1                                 ; d1 ea
    rcr ax, 1                                 ; d1 d8
    loop 01bf1h                               ; e2 fa
    and ax, strict word 0000fh                ; 25 0f 00
    cmp ax, strict word 00009h                ; 3d 09 00
    jnbe short 01c06h                         ; 77 07
    mov dx, ax                                ; 89 c2
    add dx, strict byte 00030h                ; 83 c2 30
    jmp short 01c0eh                          ; eb 08
    sub ax, strict word 0000ah                ; 2d 0a 00
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    add dx, ax                                ; 01 c2
    xor dh, dh                                ; 30 f6
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 017c7h                               ; e8 b1 fb
    dec word [bp-012h]                        ; ff 4e ee
    jmp short 01bdch                          ; eb c1
    cmp dl, 064h                              ; 80 fa 64
    jne short 01c3fh                          ; 75 1f
    test byte [bp-00bh], 080h                 ; f6 46 f5 80
    je short 01c30h                           ; 74 0a
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    neg dx                                    ; f7 da
    mov cx, strict word 00001h                ; b9 01 00
    jmp short 01c37h                          ; eb 07
    xor cx, cx                                ; 31 c9
    mov bx, di                                ; 89 fb
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01802h                               ; e8 c5 fb
    jmp short 01c97h                          ; eb 58
    cmp dl, 073h                              ; 80 fa 73
    jne short 01c51h                          ; 75 0d
    mov cx, ds                                ; 8c d9
    mov bx, word [bp-00ch]                    ; 8b 5e f4
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01933h                               ; e8 e4 fc
    jmp short 01c97h                          ; eb 46
    cmp dl, 053h                              ; 80 fa 53
    jne short 01c77h                          ; 75 21
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    mov word [bp-010h], ax                    ; 89 46 f0
    mov ax, word [bp-014h]                    ; 8b 46 ec
    mov word [bp-014h], ax                    ; 89 46 ec
    add word [bp-016h], strict byte 00002h    ; 83 46 ea 02
    les bx, [bp-016h]                         ; c4 5e ea
    mov ax, word [es:bx-002h]                 ; 26 8b 47 fe
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov bx, ax                                ; 89 c3
    mov cx, word [bp-010h]                    ; 8b 4e f0
    jmp short 01c49h                          ; eb d2
    cmp dl, 063h                              ; 80 fa 63
    jne short 01c89h                          ; 75 0d
    mov dl, byte [bp-00ch]                    ; 8a 56 f4
    xor dh, dh                                ; 30 f6
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 017c7h                               ; e8 40 fb
    jmp short 01c97h                          ; eb 0e
    mov ax, 00107h                            ; b8 07 01
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 e2 fc
    add sp, strict byte 00004h                ; 83 c4 04
    xor cx, cx                                ; 31 c9
    jmp short 01ca3h                          ; eb 08
    xor dh, dh                                ; 30 f6
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 017c7h                               ; e8 24 fb
    inc word [bp+006h]                        ; ff 46 06
    jmp near 019a7h                           ; e9 fe fc
    xor ax, ax                                ; 31 c0
    mov word [bp-016h], ax                    ; 89 46 ea
    mov word [bp-014h], ax                    ; 89 46 ec
    test byte [bp+004h], 001h                 ; f6 46 04 01
    je short 01cbbh                           ; 74 04
    cli                                       ; fa
    hlt                                       ; f4
    jmp short 01cb8h                          ; eb fd
    lea sp, [bp-00ah]                         ; 8d 66 f6
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
_ata_init:                                   ; 0xf1cc5 LB 0xf3
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 9b f9
    mov si, 00122h                            ; be 22 01
    mov di, ax                                ; 89 c7
    xor cl, cl                                ; 30 c9
    jmp short 01ce1h                          ; eb 05
    cmp cl, 004h                              ; 80 f9 04
    jnc short 01d0eh                          ; 73 2d
    mov al, cl                                ; 88 c8
    xor ah, ah                                ; 30 e4
    mov bx, strict word 00006h                ; bb 06 00
    imul bx                                   ; f7 eb
    mov es, di                                ; 8e c7
    mov bx, si                                ; 89 f3
    add bx, ax                                ; 01 c3
    mov byte [es:bx+00204h], 000h             ; 26 c6 87 04 02 00
    mov word [es:bx+00206h], strict word 00000h ; 26 c7 87 06 02 00 00
    mov word [es:bx+00208h], strict word 00000h ; 26 c7 87 08 02 00 00
    mov byte [es:bx+00205h], 000h             ; 26 c6 87 05 02 00
    db  0feh, 0c1h
    ; inc cl                                    ; fe c1
    jmp short 01cdch                          ; eb ce
    xor cl, cl                                ; 30 c9
    jmp short 01d17h                          ; eb 05
    cmp cl, 008h                              ; 80 f9 08
    jnc short 01d82h                          ; 73 6b
    mov al, cl                                ; 88 c8
    xor ah, ah                                ; 30 e4
    mov bx, strict word 0001ch                ; bb 1c 00
    imul bx                                   ; f7 eb
    mov es, di                                ; 8e c7
    mov bx, si                                ; 89 f3
    add bx, ax                                ; 01 c3
    mov word [es:bx+022h], strict word 00000h ; 26 c7 47 22 00 00
    mov word [es:bx+024h], strict word 00000h ; 26 c7 47 24 00 00
    mov byte [es:bx+026h], 000h               ; 26 c6 47 26 00
    mov word [es:bx+028h], 00200h             ; 26 c7 47 28 00 02
    mov byte [es:bx+027h], 000h               ; 26 c6 47 27 00
    mov word [es:bx+02ah], strict word 00000h ; 26 c7 47 2a 00 00
    mov word [es:bx+02ch], strict word 00000h ; 26 c7 47 2c 00 00
    mov word [es:bx+02eh], strict word 00000h ; 26 c7 47 2e 00 00
    mov word [es:bx+030h], strict word 00000h ; 26 c7 47 30 00 00
    mov word [es:bx+032h], strict word 00000h ; 26 c7 47 32 00 00
    mov word [es:bx+034h], strict word 00000h ; 26 c7 47 34 00 00
    mov word [es:bx+03ch], strict word 00000h ; 26 c7 47 3c 00 00
    mov word [es:bx+03ah], strict word 00000h ; 26 c7 47 3a 00 00
    mov word [es:bx+038h], strict word 00000h ; 26 c7 47 38 00 00
    mov word [es:bx+036h], strict word 00000h ; 26 c7 47 36 00 00
    db  0feh, 0c1h
    ; inc cl                                    ; fe c1
    jmp short 01d12h                          ; eb 90
    xor cl, cl                                ; 30 c9
    jmp short 01d8bh                          ; eb 05
    cmp cl, 010h                              ; 80 f9 10
    jnc short 01da3h                          ; 73 18
    mov bl, cl                                ; 88 cb
    xor bh, bh                                ; 30 ff
    mov es, di                                ; 8e c7
    add bx, si                                ; 01 f3
    mov byte [es:bx+001e3h], 010h             ; 26 c6 87 e3 01 10
    mov byte [es:bx+001f4h], 010h             ; 26 c6 87 f4 01 10
    db  0feh, 0c1h
    ; inc cl                                    ; fe c1
    jmp short 01d86h                          ; eb e3
    mov es, di                                ; 8e c7
    mov byte [es:si+001e2h], 000h             ; 26 c6 84 e2 01 00
    mov byte [es:si+001f3h], 000h             ; 26 c6 84 f3 01 00
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
ata_reset_:                                  ; 0xf1db8 LB 0xe5
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    push ax                                   ; 50
    push ax                                   ; 50
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 a2 f8
    mov es, ax                                ; 8e c0
    mov di, 00122h                            ; bf 22 01
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov ax, word [bp-010h]                    ; 8b 46 f0
    shr ax, 1                                 ; d1 e8
    mov ah, byte [bp-010h]                    ; 8a 66 f0
    and ah, 001h                              ; 80 e4 01
    mov byte [bp-00ch], ah                    ; 88 66 f4
    xor ah, ah                                ; 30 e4
    mov dx, strict word 00006h                ; ba 06 00
    imul dx                                   ; f7 ea
    mov bx, ax                                ; 89 c3
    add bx, di                                ; 01 fb
    mov cx, word [es:bx+00206h]               ; 26 8b 8f 06 02
    mov si, word [es:bx+00208h]               ; 26 8b b7 08 02
    lea dx, [si+006h]                         ; 8d 54 06
    mov AL, strict byte 00eh                  ; b0 0e
    out DX, AL                                ; ee
    mov bx, 000ffh                            ; bb ff 00
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 01e11h                          ; 76 0c
    mov dx, cx                                ; 89 ca
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 080h                 ; a8 80
    je short 01e00h                           ; 74 ef
    lea dx, [si+006h]                         ; 8d 54 06
    mov AL, strict byte 00ah                  ; b0 0a
    out DX, AL                                ; ee
    mov ax, word [bp-010h]                    ; 8b 46 f0
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov es, [bp-00eh]                         ; 8e 46 f2
    mov bx, di                                ; 89 fb
    add bx, ax                                ; 01 c3
    cmp byte [es:bx+022h], 000h               ; 26 80 7f 22 00
    je short 01e79h                           ; 74 4c
    cmp byte [bp-00ch], 000h                  ; 80 7e f4 00
    je short 01e38h                           ; 74 05
    mov ax, 000b0h                            ; b8 b0 00
    jmp short 01e3bh                          ; eb 03
    mov ax, 000a0h                            ; b8 a0 00
    mov dx, cx                                ; 89 ca
    add dx, strict byte 00006h                ; 83 c2 06
    out DX, AL                                ; ee
    mov dx, cx                                ; 89 ca
    inc dx                                    ; 42
    inc dx                                    ; 42
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov bx, ax                                ; 89 c3
    mov dx, cx                                ; 89 ca
    add dx, strict byte 00003h                ; 83 c2 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp bl, 001h                              ; 80 fb 01
    jne short 01e79h                          ; 75 22
    cmp al, bl                                ; 38 d8
    jne short 01e79h                          ; 75 1e
    mov bx, strict word 0ffffh                ; bb ff ff
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 01e79h                          ; 76 16
    mov dx, cx                                ; 89 ca
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 080h                 ; a8 80
    je short 01e79h                           ; 74 0a
    mov ax, strict word 0ffffh                ; b8 ff ff
    dec ax                                    ; 48
    test ax, ax                               ; 85 c0
    jnbe short 01e72h                         ; 77 fb
    jmp short 01e5eh                          ; eb e5
    mov bx, strict word 00010h                ; bb 10 00
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 01e8dh                          ; 76 0c
    mov dx, cx                                ; 89 ca
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 040h                 ; a8 40
    je short 01e7ch                           ; 74 ef
    lea dx, [si+006h]                         ; 8d 54 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    lea sp, [bp-00ah]                         ; 8d 66 f6
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
ata_cmd_data_in_:                            ; 0xf1e9d LB 0x2b7
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00010h                ; 83 ec 10
    push ax                                   ; 50
    push dx                                   ; 52
    push bx                                   ; 53
    push cx                                   ; 51
    mov es, dx                                ; 8e c2
    mov bx, ax                                ; 89 c3
    mov al, byte [es:bx+00ch]                 ; 26 8a 47 0c
    mov byte [bp-008h], al                    ; 88 46 f8
    mov bl, al                                ; 88 c3
    xor bh, ah                                ; 30 e7
    mov ax, bx                                ; 89 d8
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    mov dx, strict word 00006h                ; ba 06 00
    imul dx                                   ; f7 ea
    mov di, word [bp-016h]                    ; 8b 7e ea
    add di, ax                                ; 01 c7
    mov ax, word [es:di+00206h]               ; 26 8b 85 06 02
    mov word [bp-00ah], ax                    ; 89 46 f6
    mov ax, word [es:di+00208h]               ; 26 8b 85 08 02
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov ax, bx                                ; 89 d8
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov bx, word [bp-016h]                    ; 8b 5e ea
    add bx, ax                                ; 01 c3
    mov ax, word [es:bx+028h]                 ; 26 8b 47 28
    mov word [bp-00eh], ax                    ; 89 46 f2
    test ax, ax                               ; 85 c0
    jne short 01ef7h                          ; 75 07
    mov word [bp-00eh], 08000h                ; c7 46 f2 00 80
    jmp short 01efah                          ; eb 03
    shr word [bp-00eh], 1                     ; d1 6e f2
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 080h                 ; a8 80
    je short 01f16h                           ; 74 0f
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov ax, strict word 00001h                ; b8 01 00
    jmp near 0214dh                           ; e9 37 02
    mov es, [bp-018h]                         ; 8e 46 e8
    mov di, word [bp-016h]                    ; 8b 7e ea
    mov di, word [es:di+008h]                 ; 26 8b 7d 08
    mov bx, word [bp-016h]                    ; 8b 5e ea
    mov ax, word [es:bx+00ah]                 ; 26 8b 47 0a
    mov word [bp-012h], ax                    ; 89 46 ee
    mov al, byte [es:bx+016h]                 ; 26 8a 47 16
    mov byte [bp-006h], al                    ; 88 46 fa
    mov ax, word [es:bx+012h]                 ; 26 8b 47 12
    mov word [bp-010h], ax                    ; 89 46 f0
    mov bl, byte [es:bx+014h]                 ; 26 8a 5f 14
    mov al, byte [bp-006h]                    ; 8a 46 fa
    test al, al                               ; 84 c0
    je short 01f46h                           ; 74 03
    jmp near 0202bh                           ; e9 e5 00
    xor ah, ah                                ; 30 e4
    xor bx, bx                                ; 31 db
    mov word [bp-014h], bx                    ; 89 5e ec
    mov si, word [bp-016h]                    ; 8b 76 ea
    mov cx, word [es:si]                      ; 26 8b 0c
    add cx, word [bp-01ch]                    ; 03 4e e4
    adc ax, word [es:si+002h]                 ; 26 13 44 02
    adc bx, word [es:si+004h]                 ; 26 13 5c 04
    mov dx, word [es:si+006h]                 ; 26 8b 54 06
    adc dx, word [bp-014h]                    ; 13 56 ec
    test dx, dx                               ; 85 d2
    jnbe short 01f7bh                         ; 77 12
    je short 01f6eh                           ; 74 03
    jmp near 01fe1h                           ; e9 73 00
    test bx, bx                               ; 85 db
    jnbe short 01f7bh                         ; 77 09
    jne short 01fe1h                          ; 75 6d
    cmp ax, 01000h                            ; 3d 00 10
    jnbe short 01f7bh                         ; 77 02
    jne short 01fe1h                          ; 75 66
    mov bx, si                                ; 89 f3
    mov ax, word [es:bx+006h]                 ; 26 8b 47 06
    mov bx, word [es:bx+004h]                 ; 26 8b 5f 04
    mov cx, word [es:si+002h]                 ; 26 8b 4c 02
    mov dx, word [es:si]                      ; 26 8b 14
    mov si, strict word 00018h                ; be 18 00
    call 0a1a0h                               ; e8 0e 82
    xor dh, dh                                ; 30 f6
    mov word [bp-014h], dx                    ; 89 56 ec
    mov bx, word [bp-016h]                    ; 8b 5e ea
    mov ax, word [es:bx+006h]                 ; 26 8b 47 06
    mov bx, word [es:bx+004h]                 ; 26 8b 5f 04
    mov si, word [bp-016h]                    ; 8b 76 ea
    mov cx, word [es:si+002h]                 ; 26 8b 4c 02
    mov dx, word [es:si]                      ; 26 8b 14
    mov si, strict word 00020h                ; be 20 00
    call 0a1a0h                               ; e8 ee 81
    mov bx, dx                                ; 89 d3
    mov word [bp-010h], dx                    ; 89 56 f0
    mov ax, word [bp-01ch]                    ; 8b 46 e4
    mov al, ah                                ; 88 e0
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    inc dx                                    ; 42
    inc dx                                    ; 42
    out DX, AL                                ; ee
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00003h                ; 83 c2 03
    mov al, byte [bp-014h]                    ; 8a 46 ec
    out DX, AL                                ; ee
    xor bh, bh                                ; 30 ff
    mov ax, bx                                ; 89 d8
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00004h                ; 83 c2 04
    out DX, AL                                ; ee
    mov al, byte [bp-00fh]                    ; 8a 46 f1
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00005h                ; 83 c2 05
    out DX, AL                                ; ee
    mov es, [bp-018h]                         ; 8e 46 e8
    mov bx, word [bp-016h]                    ; 8b 5e ea
    mov ax, word [es:bx]                      ; 26 8b 07
    mov byte [bp-006h], al                    ; 88 46 fa
    mov ax, word [es:bx+006h]                 ; 26 8b 47 06
    mov bx, word [es:bx+004h]                 ; 26 8b 5f 04
    mov si, word [bp-016h]                    ; 8b 76 ea
    mov cx, word [es:si+002h]                 ; 26 8b 4c 02
    mov dx, word [es:si]                      ; 26 8b 14
    mov si, strict word 00008h                ; be 08 00
    call 0a1a0h                               ; e8 9b 81
    mov word [bp-010h], dx                    ; 89 56 f0
    mov bx, word [bp-016h]                    ; 8b 5e ea
    mov ax, word [es:bx+006h]                 ; 26 8b 47 06
    mov bx, word [es:bx+004h]                 ; 26 8b 5f 04
    mov si, word [bp-016h]                    ; 8b 76 ea
    mov cx, word [es:si+002h]                 ; 26 8b 4c 02
    mov dx, word [es:si]                      ; 26 8b 14
    mov si, strict word 00018h                ; be 18 00
    call 0a1a0h                               ; e8 7d 81
    mov bl, dl                                ; 88 d3
    and bl, 00fh                              ; 80 e3 0f
    or bl, 040h                               ; 80 cb 40
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 00ah                  ; b0 0a
    out DX, AL                                ; ee
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    inc dx                                    ; 42
    xor al, al                                ; 30 c0
    out DX, AL                                ; ee
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    inc dx                                    ; 42
    inc dx                                    ; 42
    mov al, byte [bp-01ch]                    ; 8a 46 e4
    out DX, AL                                ; ee
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00003h                ; 83 c2 03
    mov al, byte [bp-006h]                    ; 8a 46 fa
    out DX, AL                                ; ee
    mov ax, word [bp-010h]                    ; 8b 46 f0
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00004h                ; 83 c2 04
    out DX, AL                                ; ee
    mov al, byte [bp-00fh]                    ; 8a 46 f1
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00005h                ; 83 c2 05
    out DX, AL                                ; ee
    test byte [bp-008h], 001h                 ; f6 46 f8 01
    je short 0206dh                           ; 74 05
    mov ax, 000b0h                            ; b8 b0 00
    jmp short 02070h                          ; eb 03
    mov ax, 000a0h                            ; b8 a0 00
    mov dl, bl                                ; 88 da
    xor dh, dh                                ; 30 f6
    or ax, dx                                 ; 09 d0
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00006h                ; 83 c2 06
    out DX, AL                                ; ee
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00007h                ; 83 c2 07
    mov al, byte [bp-01ah]                    ; 8a 46 e6
    out DX, AL                                ; ee
    mov ax, word [bp-01ah]                    ; 8b 46 e6
    cmp ax, 000c4h                            ; 3d c4 00
    je short 02094h                           ; 74 05
    cmp ax, strict word 00029h                ; 3d 29 00
    jne short 0209eh                          ; 75 0a
    mov si, word [bp-01ch]                    ; 8b 76 e4
    mov word [bp-01ch], strict word 00001h    ; c7 46 e4 01 00
    jmp short 020a1h                          ; eb 03
    mov si, strict word 00001h                ; be 01 00
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov bl, al                                ; 88 c3
    test AL, strict byte 080h                 ; a8 80
    jne short 020a1h                          ; 75 f1
    test AL, strict byte 001h                 ; a8 01
    je short 020c3h                           ; 74 0f
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov ax, strict word 00002h                ; b8 02 00
    jmp near 0214dh                           ; e9 8a 00
    test bl, 008h                             ; f6 c3 08
    jne short 020d7h                          ; 75 0f
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov ax, strict word 00003h                ; b8 03 00
    jmp near 0214dh                           ; e9 76 00
    sti                                       ; fb
    cmp di, 0f800h                            ; 81 ff 00 f8
    jc short 020ebh                           ; 72 0d
    sub di, 00800h                            ; 81 ef 00 08
    mov ax, word [bp-012h]                    ; 8b 46 ee
    add ax, 00080h                            ; 05 80 00
    mov word [bp-012h], ax                    ; 89 46 ee
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    mov cx, word [bp-00eh]                    ; 8b 4e f2
    mov es, [bp-012h]                         ; 8e 46 ee
    rep insw                                  ; f3 6d
    mov es, [bp-018h]                         ; 8e 46 e8
    mov bx, word [bp-016h]                    ; 8b 5e ea
    add word [es:bx+018h], si                 ; 26 01 77 18
    dec word [bp-01ch]                        ; ff 4e e4
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov bl, al                                ; 88 c3
    test AL, strict byte 080h                 ; a8 80
    jne short 02103h                          ; 75 f1
    cmp word [bp-01ch], strict byte 00000h    ; 83 7e e4 00
    jne short 0212ch                          ; 75 14
    and AL, strict byte 0c9h                  ; 24 c9
    cmp AL, strict byte 040h                  ; 3c 40
    je short 02142h                           ; 74 24
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov ax, strict word 00004h                ; b8 04 00
    jmp short 0214dh                          ; eb 21
    mov al, bl                                ; 88 d8
    and AL, strict byte 0c9h                  ; 24 c9
    cmp AL, strict byte 048h                  ; 3c 48
    je short 020d8h                           ; 74 a4
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov ax, strict word 00005h                ; b8 05 00
    jmp short 0214dh                          ; eb 0b
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    xor ax, ax                                ; 31 c0
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
_ata_detect:                                 ; 0xf2154 LB 0x6c3
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, 00264h                            ; 81 ec 64 02
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 08 f5
    mov word [bp-028h], ax                    ; 89 46 d8
    mov bx, 00122h                            ; bb 22 01
    mov es, ax                                ; 8e c0
    mov word [bp-02eh], bx                    ; 89 5e d2
    mov word [bp-02ah], ax                    ; 89 46 d6
    mov byte [es:bx+00204h], 000h             ; 26 c6 87 04 02 00
    mov word [es:bx+00206h], 001f0h           ; 26 c7 87 06 02 f0 01
    mov word [es:bx+00208h], 003f0h           ; 26 c7 87 08 02 f0 03
    mov byte [es:bx+00205h], 00eh             ; 26 c6 87 05 02 0e
    mov byte [es:bx+0020ah], 000h             ; 26 c6 87 0a 02 00
    mov word [es:bx+0020ch], 00170h           ; 26 c7 87 0c 02 70 01
    mov word [es:bx+0020eh], 00370h           ; 26 c7 87 0e 02 70 03
    mov byte [es:bx+0020bh], 00fh             ; 26 c6 87 0b 02 0f
    xor al, al                                ; 30 c0
    mov byte [bp-012h], al                    ; 88 46 ee
    mov byte [bp-00ah], al                    ; 88 46 f6
    mov byte [bp-014h], al                    ; 88 46 ec
    jmp near 02795h                           ; e9 df 05
    mov ax, 000a0h                            ; b8 a0 00
    lea dx, [bx+006h]                         ; 8d 57 06
    out DX, AL                                ; ee
    lea si, [bx+002h]                         ; 8d 77 02
    mov AL, strict byte 055h                  ; b0 55
    mov dx, si                                ; 89 f2
    out DX, AL                                ; ee
    lea di, [bx+003h]                         ; 8d 7f 03
    mov AL, strict byte 0aah                  ; b0 aa
    mov dx, di                                ; 89 fa
    out DX, AL                                ; ee
    mov dx, si                                ; 89 f2
    out DX, AL                                ; ee
    mov AL, strict byte 055h                  ; b0 55
    mov dx, di                                ; 89 fa
    out DX, AL                                ; ee
    mov dx, si                                ; 89 f2
    out DX, AL                                ; ee
    mov AL, strict byte 0aah                  ; b0 aa
    mov dx, di                                ; 89 fa
    out DX, AL                                ; ee
    mov dx, si                                ; 89 f2
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov cx, ax                                ; 89 c1
    mov dx, di                                ; 89 fa
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp cl, 055h                              ; 80 f9 55
    jne short 0223ah                          ; 75 4c
    cmp AL, strict byte 0aah                  ; 3c aa
    jne short 0223ah                          ; 75 48
    mov al, byte [bp-014h]                    ; 8a 46 ec
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov si, word [bp-02eh]                    ; 8b 76 d2
    add si, ax                                ; 01 c6
    mov byte [es:si+022h], 001h               ; 26 c6 44 22 01
    mov al, byte [bp-014h]                    ; 8a 46 ec
    xor ah, ah                                ; 30 e4
    call 01db8h                               ; e8 a7 fb
    cmp byte [bp-01ah], 000h                  ; 80 7e e6 00
    je short 0221ch                           ; 74 05
    mov ax, 000b0h                            ; b8 b0 00
    jmp short 0221fh                          ; eb 03
    mov ax, 000a0h                            ; b8 a0 00
    lea dx, [bx+006h]                         ; 8d 57 06
    out DX, AL                                ; ee
    lea dx, [bx+002h]                         ; 8d 57 02
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov cx, ax                                ; 89 c1
    lea dx, [bx+003h]                         ; 8d 57 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp cl, 001h                              ; 80 f9 01
    jne short 0227dh                          ; 75 47
    cmp al, cl                                ; 38 c8
    je short 0223ch                           ; 74 02
    jmp short 0227dh                          ; eb 41
    lea dx, [bx+004h]                         ; 8d 57 04
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov cx, ax                                ; 89 c1
    mov ch, al                                ; 88 c5
    lea dx, [bx+005h]                         ; 8d 57 05
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov word [bp-02ch], ax                    ; 89 46 d4
    mov al, byte [bp-02ch]                    ; 8a 46 d4
    mov byte [bp-006h], al                    ; 88 46 fa
    lea dx, [bx+007h]                         ; 8d 57 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp cl, 014h                              ; 80 f9 14
    jne short 0227fh                          ; 75 1f
    cmp byte [bp-02ch], 0ebh                  ; 80 7e d4 eb
    jne short 0227fh                          ; 75 19
    mov al, byte [bp-014h]                    ; 8a 46 ec
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov bx, word [bp-02eh]                    ; 8b 5e d2
    add bx, ax                                ; 01 c3
    mov byte [es:bx+022h], 003h               ; 26 c6 47 22 03
    jmp short 022c7h                          ; eb 48
    test ch, ch                               ; 84 ed
    jne short 022a6h                          ; 75 23
    cmp byte [bp-006h], 000h                  ; 80 7e fa 00
    jne short 022a6h                          ; 75 1d
    test al, al                               ; 84 c0
    je short 022a6h                           ; 74 19
    mov al, byte [bp-014h]                    ; 8a 46 ec
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov bx, word [bp-02eh]                    ; 8b 5e d2
    add bx, ax                                ; 01 c3
    mov byte [es:bx+022h], 002h               ; 26 c6 47 22 02
    jmp short 022c7h                          ; eb 21
    cmp ch, 0ffh                              ; 80 fd ff
    jne short 022c7h                          ; 75 1c
    cmp ch, byte [bp-006h]                    ; 3a 6e fa
    jne short 022c7h                          ; 75 17
    mov al, byte [bp-014h]                    ; 8a 46 ec
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov bx, word [bp-02eh]                    ; 8b 5e d2
    add bx, ax                                ; 01 c3
    mov byte [es:bx+022h], 000h               ; 26 c6 47 22 00
    mov dx, word [bp-032h]                    ; 8b 56 ce
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov al, byte [bp-014h]                    ; 8a 46 ec
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov bx, word [bp-02eh]                    ; 8b 5e d2
    add bx, ax                                ; 01 c3
    mov al, byte [es:bx+022h]                 ; 26 8a 47 22
    mov byte [bp-018h], al                    ; 88 46 e8
    cmp AL, strict byte 002h                  ; 3c 02
    jne short 02339h                          ; 75 4c
    mov byte [es:bx+023h], 0ffh               ; 26 c6 47 23 ff
    mov byte [es:bx+026h], 000h               ; 26 c6 47 26 00
    lea dx, [bp-00268h]                       ; 8d 96 98 fd
    mov bx, word [bp-02eh]                    ; 8b 5e d2
    mov word [es:bx+008h], dx                 ; 26 89 57 08
    mov [es:bx+00ah], ss                      ; 26 8c 57 0a
    mov al, byte [bp-014h]                    ; 8a 46 ec
    mov byte [es:bx+00ch], al                 ; 26 88 47 0c
    mov cx, strict word 00001h                ; b9 01 00
    mov bx, 000ech                            ; bb ec 00
    mov ax, word [bp-02eh]                    ; 8b 46 d2
    mov dx, es                                ; 8c c2
    call 01e9dh                               ; e8 82 fb
    test ax, ax                               ; 85 c0
    je short 0232dh                           ; 74 0e
    mov ax, 00124h                            ; b8 24 01
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 4c f6
    add sp, strict byte 00004h                ; 83 c4 04
    test byte [bp-00268h], 080h               ; f6 86 98 fd 80
    je short 0233ch                           ; 74 08
    mov ax, strict word 00001h                ; b8 01 00
    jmp short 0233eh                          ; eb 05
    jmp near 02540h                           ; e9 04 02
    xor ax, ax                                ; 31 c0
    mov byte [bp-008h], al                    ; 88 46 f8
    mov byte [bp-00eh], 000h                  ; c6 46 f2 00
    mov word [bp-038h], 00200h                ; c7 46 c8 00 02
    mov ax, word [bp-00266h]                  ; 8b 86 9a fd
    mov word [bp-026h], ax                    ; 89 46 da
    mov ax, word [bp-00262h]                  ; 8b 86 9e fd
    mov word [bp-01eh], ax                    ; 89 46 e2
    mov ax, word [bp-0025ch]                  ; 8b 86 a4 fd
    mov word [bp-020h], ax                    ; 89 46 e0
    mov ax, word [bp-001f0h]                  ; 8b 86 10 fe
    mov word [bp-030h], ax                    ; 89 46 d0
    mov si, word [bp-001eeh]                  ; 8b b6 12 fe
    xor ax, ax                                ; 31 c0
    mov word [bp-01ch], ax                    ; 89 46 e4
    mov word [bp-024h], ax                    ; 89 46 dc
    cmp si, 00fffh                            ; 81 fe ff 0f
    jne short 02397h                          ; 75 1f
    cmp word [bp-030h], strict byte 0ffffh    ; 83 7e d0 ff
    jne short 02397h                          ; 75 19
    mov ax, word [bp-0019ah]                  ; 8b 86 66 fe
    mov word [bp-024h], ax                    ; 89 46 dc
    mov ax, word [bp-0019ch]                  ; 8b 86 64 fe
    mov word [bp-01ch], ax                    ; 89 46 e4
    mov si, word [bp-0019eh]                  ; 8b b6 62 fe
    mov ax, word [bp-001a0h]                  ; 8b 86 60 fe
    mov word [bp-030h], ax                    ; 89 46 d0
    mov al, byte [bp-014h]                    ; 8a 46 ec
    cmp AL, strict byte 001h                  ; 3c 01
    jc short 023aah                           ; 72 0c
    jbe short 023b2h                          ; 76 12
    cmp AL, strict byte 003h                  ; 3c 03
    je short 023bah                           ; 74 16
    cmp AL, strict byte 002h                  ; 3c 02
    je short 023b6h                           ; 74 0e
    jmp short 023efh                          ; eb 45
    test al, al                               ; 84 c0
    jne short 023efh                          ; 75 41
    mov BL, strict byte 01eh                  ; b3 1e
    jmp short 023bch                          ; eb 0a
    mov BL, strict byte 026h                  ; b3 26
    jmp short 023bch                          ; eb 06
    mov BL, strict byte 067h                  ; b3 67
    jmp short 023bch                          ; eb 02
    mov BL, strict byte 070h                  ; b3 70
    mov al, bl                                ; 88 d8
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    xor ah, ah                                ; 30 e4
    call 016aeh                               ; e8 e9 f2
    mov dh, al                                ; 88 c6
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    call 016aeh                               ; e8 e0 f2
    mov ah, dh                                ; 88 f4
    mov word [bp-03ch], ax                    ; 89 46 c4
    mov al, bl                                ; 88 d8
    add AL, strict byte 002h                  ; 04 02
    xor ah, ah                                ; 30 e4
    call 016aeh                               ; e8 d2 f2
    xor ah, ah                                ; 30 e4
    mov word [bp-03eh], ax                    ; 89 46 c2
    mov al, bl                                ; 88 d8
    add AL, strict byte 007h                  ; 04 07
    call 016aeh                               ; e8 c6 f2
    xor ah, ah                                ; 30 e4
    mov word [bp-03ah], ax                    ; 89 46 c6
    jmp short 02401h                          ; eb 12
    push word [bp-024h]                       ; ff 76 dc
    push word [bp-01ch]                       ; ff 76 e4
    push si                                   ; 56
    push word [bp-030h]                       ; ff 76 d0
    mov dx, ss                                ; 8c d2
    lea ax, [bp-03eh]                         ; 8d 46 c2
    call 05b9fh                               ; e8 9e 37
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 27 f5
    mov ax, word [bp-03ah]                    ; 8b 46 c6
    push ax                                   ; 50
    mov ax, word [bp-03eh]                    ; 8b 46 c2
    push ax                                   ; 50
    mov ax, word [bp-03ch]                    ; 8b 46 c4
    push ax                                   ; 50
    push word [bp-020h]                       ; ff 76 e0
    push word [bp-01eh]                       ; ff 76 e2
    push word [bp-026h]                       ; ff 76 da
    mov al, byte [bp-01ah]                    ; 8a 46 e6
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    push ax                                   ; 50
    mov ax, 0014dh                            ; b8 4d 01
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 40 f5
    add sp, strict byte 00014h                ; 83 c4 14
    mov al, byte [bp-014h]                    ; 8a 46 ec
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov di, word [bp-02eh]                    ; 8b 7e d2
    add di, ax                                ; 01 c7
    mov byte [es:di+023h], 0ffh               ; 26 c6 45 23 ff
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov byte [es:di+024h], al                 ; 26 88 45 24
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    mov byte [es:di+026h], al                 ; 26 88 45 26
    mov ax, word [bp-038h]                    ; 8b 46 c8
    mov word [es:di+028h], ax                 ; 26 89 45 28
    mov ax, word [bp-01eh]                    ; 8b 46 e2
    mov word [es:di+030h], ax                 ; 26 89 45 30
    mov ax, word [bp-026h]                    ; 8b 46 da
    mov word [es:di+032h], ax                 ; 26 89 45 32
    mov ax, word [bp-020h]                    ; 8b 46 e0
    mov word [es:di+034h], ax                 ; 26 89 45 34
    mov ax, word [bp-024h]                    ; 8b 46 dc
    mov word [es:di+03ch], ax                 ; 26 89 45 3c
    mov ax, word [bp-01ch]                    ; 8b 46 e4
    mov word [es:di+03ah], ax                 ; 26 89 45 3a
    mov word [es:di+038h], si                 ; 26 89 75 38
    mov ax, word [bp-030h]                    ; 8b 46 d0
    mov word [es:di+036h], ax                 ; 26 89 45 36
    lea di, [di+02ah]                         ; 8d 7d 2a
    push DS                                   ; 1e
    push SS                                   ; 16
    pop DS                                    ; 1f
    lea si, [bp-03eh]                         ; 8d 76 c2
    movsw                                     ; a5
    movsw                                     ; a5
    movsw                                     ; a5
    pop DS                                    ; 1f
    mov al, byte [bp-014h]                    ; 8a 46 ec
    cmp AL, strict byte 002h                  ; 3c 02
    jnc short 0250bh                          ; 73 64
    test al, al                               ; 84 c0
    jne short 024b0h                          ; 75 05
    mov di, strict word 0003dh                ; bf 3d 00
    jmp short 024b3h                          ; eb 03
    mov di, strict word 0004dh                ; bf 4d 00
    mov cx, word [bp-028h]                    ; 8b 4e d8
    mov al, byte [bp-014h]                    ; 8a 46 ec
    xor ah, ah                                ; 30 e4
    mov dx, strict word 00005h                ; ba 05 00
    imul dx                                   ; f7 ea
    mov bx, ax                                ; 89 c3
    sal bx, 1                                 ; d1 e3
    sal bx, 1                                 ; d1 e3
    add bx, 00104h                            ; 81 c3 04 01
    xor ax, ax                                ; 31 c0
    mov es, ax                                ; 8e c0
    mov word [es:bx], di                      ; 26 89 3f
    mov word [es:bx+002h], cx                 ; 26 89 4f 02
    mov bx, word [bp-03ch]                    ; 8b 5e c4
    mov es, cx                                ; 8e c1
    mov word [es:di], bx                      ; 26 89 1d
    mov bl, byte [bp-03eh]                    ; 8a 5e c2
    mov byte [es:di+002h], bl                 ; 26 88 5d 02
    mov byte [es:di+003h], 0a0h               ; 26 c6 45 03 a0
    mov al, byte [bp-020h]                    ; 8a 46 e0
    mov byte [es:di+004h], al                 ; 26 88 45 04
    mov ax, word [bp-026h]                    ; 8b 46 da
    mov word [es:di+009h], ax                 ; 26 89 45 09
    mov al, byte [bp-01eh]                    ; 8a 46 e2
    mov byte [es:di+00bh], al                 ; 26 88 45 0b
    mov bl, byte [bp-03ah]                    ; 8a 5e c6
    mov byte [es:di+00eh], bl                 ; 26 88 5d 0e
    xor bl, bl                                ; 30 db
    xor al, al                                ; 30 c0
    jmp short 02511h                          ; eb 06
    jmp short 0252ah                          ; eb 1d
    cmp AL, strict byte 00fh                  ; 3c 0f
    jnc short 02522h                          ; 73 11
    mov dl, al                                ; 88 c2
    xor dh, dh                                ; 30 f6
    mov es, cx                                ; 8e c1
    mov si, di                                ; 89 fe
    add si, dx                                ; 01 d6
    add bl, byte [es:si]                      ; 26 02 1c
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    jmp short 0250dh                          ; eb eb
    neg bl                                    ; f6 db
    mov es, cx                                ; 8e c1
    mov byte [es:di+00fh], bl                 ; 26 88 5d 0f
    mov bl, byte [bp-00ah]                    ; 8a 5e f6
    xor bh, bh                                ; 30 ff
    mov es, [bp-02ah]                         ; 8e 46 d6
    add bx, word [bp-02eh]                    ; 03 5e d2
    mov al, byte [bp-014h]                    ; 8a 46 ec
    mov byte [es:bx+001e3h], al               ; 26 88 87 e3 01
    inc byte [bp-00ah]                        ; fe 46 f6
    cmp byte [bp-018h], 003h                  ; 80 7e e8 03
    jne short 025adh                          ; 75 67
    mov al, byte [bp-014h]                    ; 8a 46 ec
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov bx, word [bp-02eh]                    ; 8b 5e d2
    add bx, ax                                ; 01 c3
    mov byte [es:bx+023h], 005h               ; 26 c6 47 23 05
    mov byte [es:bx+026h], 000h               ; 26 c6 47 26 00
    lea dx, [bp-00268h]                       ; 8d 96 98 fd
    mov bx, word [bp-02eh]                    ; 8b 5e d2
    mov word [es:bx+008h], dx                 ; 26 89 57 08
    mov [es:bx+00ah], ss                      ; 26 8c 57 0a
    mov al, byte [bp-014h]                    ; 8a 46 ec
    mov byte [es:bx+00ch], al                 ; 26 88 47 0c
    mov cx, strict word 00001h                ; b9 01 00
    mov bx, 000a1h                            ; bb a1 00
    mov ax, word [bp-02eh]                    ; 8b 46 d2
    mov dx, es                                ; 8c c2
    call 01e9dh                               ; e8 17 f9
    test ax, ax                               ; 85 c0
    je short 02598h                           ; 74 0e
    mov ax, 00174h                            ; b8 74 01
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 e1 f3
    add sp, strict byte 00004h                ; 83 c4 04
    mov al, byte [bp-00267h]                  ; 8a 86 99 fd
    and AL, strict byte 01fh                  ; 24 1f
    mov byte [bp-010h], al                    ; 88 46 f0
    test byte [bp-00268h], 080h               ; f6 86 98 fd 80
    je short 025afh                           ; 74 07
    mov cx, strict word 00001h                ; b9 01 00
    jmp short 025b1h                          ; eb 04
    jmp short 025ech                          ; eb 3d
    xor cx, cx                                ; 31 c9
    mov al, byte [bp-014h]                    ; 8a 46 ec
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov bx, word [bp-02eh]                    ; 8b 5e d2
    add bx, ax                                ; 01 c3
    mov al, byte [bp-010h]                    ; 8a 46 f0
    mov byte [es:bx+023h], al                 ; 26 88 47 23
    mov byte [es:bx+024h], cl                 ; 26 88 4f 24
    mov byte [es:bx+026h], 000h               ; 26 c6 47 26 00
    mov word [es:bx+028h], 00800h             ; 26 c7 47 28 00 08
    mov bl, byte [bp-012h]                    ; 8a 5e ee
    xor bh, bh                                ; 30 ff
    add bx, word [bp-02eh]                    ; 03 5e d2
    mov al, byte [bp-014h]                    ; 8a 46 ec
    mov byte [es:bx+001f4h], al               ; 26 88 87 f4 01
    inc byte [bp-012h]                        ; fe 46 ee
    mov al, byte [bp-018h]                    ; 8a 46 e8
    cmp AL, strict byte 003h                  ; 3c 03
    je short 02625h                           ; 74 32
    cmp AL, strict byte 002h                  ; 3c 02
    jne short 0264ch                          ; 75 55
    mov al, byte [bp-014h]                    ; 8a 46 ec
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov di, word [bp-02eh]                    ; 8b 7e d2
    add di, ax                                ; 01 c7
    mov ax, word [es:di+03ch]                 ; 26 8b 45 3c
    mov bx, word [es:di+03ah]                 ; 26 8b 5d 3a
    mov cx, word [es:di+038h]                 ; 26 8b 4d 38
    mov dx, word [es:di+036h]                 ; 26 8b 55 36
    mov si, strict word 0000bh                ; be 0b 00
    call 0a1a0h                               ; e8 81 7b
    mov word [bp-022h], dx                    ; 89 56 de
    mov word [bp-036h], cx                    ; 89 4e ca
    mov dh, byte [bp-001c7h]                  ; 8a b6 39 fe
    mov dl, byte [bp-001c8h]                  ; 8a 96 38 fe
    mov byte [bp-016h], 00fh                  ; c6 46 ea 0f
    jmp short 0263ch                          ; eb 09
    dec byte [bp-016h]                        ; fe 4e ea
    cmp byte [bp-016h], 000h                  ; 80 7e ea 00
    jbe short 02648h                          ; 76 0c
    mov cl, byte [bp-016h]                    ; 8a 4e ea
    mov ax, strict word 00001h                ; b8 01 00
    sal ax, CL                                ; d3 e0
    test dx, ax                               ; 85 c2
    je short 02633h                           ; 74 eb
    xor di, di                                ; 31 ff
    jmp short 02653h                          ; eb 07
    jmp short 02682h                          ; eb 34
    cmp di, strict byte 00014h                ; 83 ff 14
    jnl short 02668h                          ; 7d 15
    mov si, di                                ; 89 fe
    sal si, 1                                 ; d1 e6
    mov al, byte [bp+si-00231h]               ; 8a 82 cf fd
    mov byte [bp+si-068h], al                 ; 88 42 98
    mov al, byte [bp+si-00232h]               ; 8a 82 ce fd
    mov byte [bp+si-067h], al                 ; 88 42 99
    inc di                                    ; 47
    jmp short 0264eh                          ; eb e6
    mov byte [bp-040h], 000h                  ; c6 46 c0 00
    mov di, strict word 00027h                ; bf 27 00
    jmp short 02676h                          ; eb 05
    dec di                                    ; 4f
    test di, di                               ; 85 ff
    jle short 02682h                          ; 7e 0c
    cmp byte [bp+di-068h], 020h               ; 80 7b 98 20
    jne short 02682h                          ; 75 06
    mov byte [bp+di-068h], 000h               ; c6 43 98 00
    jmp short 02671h                          ; eb ef
    mov al, byte [bp-018h]                    ; 8a 46 e8
    cmp AL, strict byte 003h                  ; 3c 03
    je short 026efh                           ; 74 66
    cmp AL, strict byte 002h                  ; 3c 02
    je short 02694h                           ; 74 07
    cmp AL, strict byte 001h                  ; 3c 01
    je short 026fah                           ; 74 69
    jmp near 0278ch                           ; e9 f8 00
    cmp byte [bp-01ah], 000h                  ; 80 7e e6 00
    je short 0269fh                           ; 74 05
    mov ax, 0019fh                            ; b8 9f 01
    jmp short 026a2h                          ; eb 03
    mov ax, 001a6h                            ; b8 a6 01
    push ax                                   ; 50
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 001adh                            ; b8 ad 01
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 c2 f2
    add sp, strict byte 00008h                ; 83 c4 08
    xor di, di                                ; 31 ff
    mov al, byte [bp+di-068h]                 ; 8a 43 98
    xor ah, ah                                ; 30 e4
    inc di                                    ; 47
    test ax, ax                               ; 85 c0
    je short 026d4h                           ; 74 11
    push ax                                   ; 50
    mov ax, 001b8h                            ; b8 b8 01
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 a7 f2
    add sp, strict byte 00006h                ; 83 c4 06
    jmp short 026b9h                          ; eb e5
    push word [bp-036h]                       ; ff 76 ca
    push word [bp-022h]                       ; ff 76 de
    mov al, byte [bp-016h]                    ; 8a 46 ea
    push ax                                   ; 50
    mov ax, 001bbh                            ; b8 bb 01
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 8d f2
    add sp, strict byte 0000ah                ; 83 c4 0a
    jmp near 0278ch                           ; e9 9d 00
    cmp byte [bp-01ah], 000h                  ; 80 7e e6 00
    je short 026fch                           ; 74 07
    mov ax, 0019fh                            ; b8 9f 01
    jmp short 026ffh                          ; eb 05
    jmp short 02769h                          ; eb 6d
    mov ax, 001a6h                            ; b8 a6 01
    push ax                                   ; 50
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 001adh                            ; b8 ad 01
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 65 f2
    add sp, strict byte 00008h                ; 83 c4 08
    xor di, di                                ; 31 ff
    mov al, byte [bp+di-068h]                 ; 8a 43 98
    xor ah, ah                                ; 30 e4
    inc di                                    ; 47
    test ax, ax                               ; 85 c0
    je short 02731h                           ; 74 11
    push ax                                   ; 50
    mov ax, 001b8h                            ; b8 b8 01
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 4a f2
    add sp, strict byte 00006h                ; 83 c4 06
    jmp short 02716h                          ; eb e5
    mov al, byte [bp-014h]                    ; 8a 46 ec
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov bx, word [bp-02eh]                    ; 8b 5e d2
    add bx, ax                                ; 01 c3
    cmp byte [es:bx+023h], 005h               ; 26 80 7f 23 05
    jne short 02753h                          ; 75 0b
    mov al, byte [bp-016h]                    ; 8a 46 ea
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 001dbh                            ; b8 db 01
    jmp short 0275ch                          ; eb 09
    mov al, byte [bp-016h]                    ; 8a 46 ea
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 001f5h                            ; b8 f5 01
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 12 f2
    add sp, strict byte 00006h                ; 83 c4 06
    jmp short 0278ch                          ; eb 23
    cmp byte [bp-01ah], 000h                  ; 80 7e e6 00
    je short 02774h                           ; 74 05
    mov ax, 0019fh                            ; b8 9f 01
    jmp short 02777h                          ; eb 03
    mov ax, 001a6h                            ; b8 a6 01
    push ax                                   ; 50
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 00207h                            ; b8 07 02
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 ed f1
    add sp, strict byte 00008h                ; 83 c4 08
    inc byte [bp-014h]                        ; fe 46 ec
    cmp byte [bp-014h], 008h                  ; 80 7e ec 08
    jnc short 027ech                          ; 73 57
    mov bl, byte [bp-014h]                    ; 8a 5e ec
    xor bh, bh                                ; 30 ff
    mov ax, bx                                ; 89 d8
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    mov cx, ax                                ; 89 c1
    mov byte [bp-00ch], al                    ; 88 46 f4
    mov ax, bx                                ; 89 d8
    cwd                                       ; 99
    mov bx, strict word 00002h                ; bb 02 00
    idiv bx                                   ; f7 fb
    mov word [bp-034h], dx                    ; 89 56 cc
    mov al, byte [bp-034h]                    ; 8a 46 cc
    mov byte [bp-01ah], al                    ; 88 46 e6
    mov al, cl                                ; 88 c8
    xor ah, ah                                ; 30 e4
    mov dx, strict word 00006h                ; ba 06 00
    imul dx                                   ; f7 ea
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov si, word [bp-02eh]                    ; 8b 76 d2
    add si, ax                                ; 01 c6
    mov bx, word [es:si+00206h]               ; 26 8b 9c 06 02
    mov ax, word [es:si+00208h]               ; 26 8b 84 08 02
    mov word [bp-032h], ax                    ; 89 46 ce
    mov dx, ax                                ; 89 c2
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 00ah                  ; b0 0a
    out DX, AL                                ; ee
    cmp byte [bp-034h], 000h                  ; 80 7e cc 00
    jne short 027e6h                          ; 75 03
    jmp near 021b6h                           ; e9 d0 f9
    mov ax, 000b0h                            ; b8 b0 00
    jmp near 021b9h                           ; e9 cd f9
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov bx, word [bp-02eh]                    ; 8b 5e d2
    mov byte [es:bx+001e2h], al               ; 26 88 87 e2 01
    mov al, byte [bp-012h]                    ; 8a 46 ee
    mov byte [es:bx+001f3h], al               ; 26 88 87 f3 01
    mov bl, byte [bp-00ah]                    ; 8a 5e f6
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00075h                ; ba 75 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 50 ee
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
ata_cmd_data_out_:                           ; 0xf2817 LB 0x28e
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00020h                ; 83 ec 20
    mov di, ax                                ; 89 c7
    mov word [bp-00ah], dx                    ; 89 56 f6
    mov word [bp-01eh], bx                    ; 89 5e e2
    mov word [bp-01ah], cx                    ; 89 4e e6
    mov es, dx                                ; 8e c2
    mov al, byte [es:di+00ch]                 ; 26 8a 45 0c
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    shr ax, 1                                 ; d1 e8
    and dl, 001h                              ; 80 e2 01
    mov byte [bp-006h], dl                    ; 88 56 fa
    xor ah, ah                                ; 30 e4
    mov dx, strict word 00006h                ; ba 06 00
    imul dx                                   ; f7 ea
    mov bx, di                                ; 89 fb
    add bx, ax                                ; 01 c3
    mov ax, word [es:bx+00206h]               ; 26 8b 87 06 02
    mov word [bp-008h], ax                    ; 89 46 f8
    mov ax, word [es:bx+00208h]               ; 26 8b 87 08 02
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov word [bp-01ch], 00100h                ; c7 46 e4 00 01
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 080h                 ; a8 80
    je short 02878h                           ; 74 0f
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov ax, strict word 00001h                ; b8 01 00
    jmp near 02a9eh                           ; e9 26 02
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov ax, word [es:di+006h]                 ; 26 8b 45 06
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov ax, word [es:di+004h]                 ; 26 8b 45 04
    mov word [bp-020h], ax                    ; 89 46 e0
    mov ax, word [es:di+002h]                 ; 26 8b 45 02
    mov word [bp-010h], ax                    ; 89 46 f0
    mov ax, word [es:di]                      ; 26 8b 05
    mov word [bp-022h], ax                    ; 89 46 de
    mov ax, word [es:di+008h]                 ; 26 8b 45 08
    mov word [bp-016h], ax                    ; 89 46 ea
    mov ax, word [es:di+00ah]                 ; 26 8b 45 0a
    mov word [bp-012h], ax                    ; 89 46 ee
    mov ax, word [es:di+016h]                 ; 26 8b 45 16
    mov word [bp-014h], ax                    ; 89 46 ec
    mov ax, word [es:di+012h]                 ; 26 8b 45 12
    mov word [bp-024h], ax                    ; 89 46 dc
    mov ax, word [es:di+014h]                 ; 26 8b 45 14
    mov word [bp-018h], ax                    ; 89 46 e8
    mov ax, word [bp-014h]                    ; 8b 46 ec
    test ax, ax                               ; 85 c0
    je short 028c3h                           ; 74 03
    jmp near 0298ah                           ; e9 c7 00
    xor bx, bx                                ; 31 db
    xor dx, dx                                ; 31 d2
    mov si, word [bp-022h]                    ; 8b 76 de
    add si, word [bp-01ah]                    ; 03 76 e6
    adc bx, word [bp-010h]                    ; 13 5e f0
    adc dx, word [bp-020h]                    ; 13 56 e0
    adc ax, word [bp-00ch]                    ; 13 46 f4
    test ax, ax                               ; 85 c0
    jnbe short 028eah                         ; 77 10
    jne short 0294dh                          ; 75 71
    test dx, dx                               ; 85 d2
    jnbe short 028eah                         ; 77 0a
    jne short 0294dh                          ; 75 6b
    cmp bx, 01000h                            ; 81 fb 00 10
    jnbe short 028eah                         ; 77 02
    jne short 0294dh                          ; 75 63
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    mov bx, word [bp-020h]                    ; 8b 5e e0
    mov cx, word [bp-010h]                    ; 8b 4e f0
    mov dx, word [bp-022h]                    ; 8b 56 de
    mov si, strict word 00018h                ; be 18 00
    call 0a1a0h                               ; e8 a4 78
    xor dh, dh                                ; 30 f6
    mov word [bp-014h], dx                    ; 89 56 ec
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    mov bx, word [bp-020h]                    ; 8b 5e e0
    mov cx, word [bp-010h]                    ; 8b 4e f0
    mov dx, word [bp-022h]                    ; 8b 56 de
    mov si, strict word 00020h                ; be 20 00
    call 0a1a0h                               ; e8 8d 78
    mov bx, dx                                ; 89 d3
    mov word [bp-024h], dx                    ; 89 56 dc
    mov ax, word [bp-01ah]                    ; 8b 46 e6
    mov al, ah                                ; 88 e0
    mov dx, word [bp-008h]                    ; 8b 56 f8
    inc dx                                    ; 42
    inc dx                                    ; 42
    out DX, AL                                ; ee
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, strict byte 00003h                ; 83 c2 03
    mov al, byte [bp-014h]                    ; 8a 46 ec
    out DX, AL                                ; ee
    xor bh, bh                                ; 30 ff
    mov ax, bx                                ; 89 d8
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, strict byte 00004h                ; 83 c2 04
    out DX, AL                                ; ee
    mov al, byte [bp-023h]                    ; 8a 46 dd
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, strict byte 00005h                ; 83 c2 05
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    mov byte [bp-00fh], bh                    ; 88 7e f1
    mov word [bp-020h], ax                    ; 89 46 e0
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov ax, word [bp-022h]                    ; 8b 46 de
    xor ah, ah                                ; 30 e4
    mov word [bp-014h], ax                    ; 89 46 ec
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    mov bx, word [bp-020h]                    ; 8b 5e e0
    mov cx, word [bp-010h]                    ; 8b 4e f0
    mov dx, word [bp-022h]                    ; 8b 56 de
    mov si, strict word 00008h                ; be 08 00
    call 0a1a0h                               ; e8 39 78
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov word [bp-020h], bx                    ; 89 5e e0
    mov word [bp-010h], cx                    ; 89 4e f0
    mov word [bp-022h], dx                    ; 89 56 de
    mov word [bp-024h], dx                    ; 89 56 dc
    mov si, strict word 00010h                ; be 10 00
    call 0a1a0h                               ; e8 24 78
    mov word [bp-022h], dx                    ; 89 56 de
    mov ax, dx                                ; 89 d0
    xor ah, dh                                ; 30 f4
    and AL, strict byte 00fh                  ; 24 0f
    or AL, strict byte 040h                   ; 0c 40
    mov word [bp-018h], ax                    ; 89 46 e8
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 00ah                  ; b0 0a
    out DX, AL                                ; ee
    mov dx, word [bp-008h]                    ; 8b 56 f8
    inc dx                                    ; 42
    xor al, al                                ; 30 c0
    out DX, AL                                ; ee
    mov dx, word [bp-008h]                    ; 8b 56 f8
    inc dx                                    ; 42
    inc dx                                    ; 42
    mov al, byte [bp-01ah]                    ; 8a 46 e6
    out DX, AL                                ; ee
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, strict byte 00003h                ; 83 c2 03
    mov al, byte [bp-014h]                    ; 8a 46 ec
    out DX, AL                                ; ee
    mov ax, word [bp-024h]                    ; 8b 46 dc
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, strict byte 00004h                ; 83 c2 04
    out DX, AL                                ; ee
    mov al, byte [bp-023h]                    ; 8a 46 dd
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, strict byte 00005h                ; 83 c2 05
    out DX, AL                                ; ee
    cmp byte [bp-006h], 000h                  ; 80 7e fa 00
    je short 029cch                           ; 74 05
    mov ax, 000b0h                            ; b8 b0 00
    jmp short 029cfh                          ; eb 03
    mov ax, 000a0h                            ; b8 a0 00
    mov dl, byte [bp-018h]                    ; 8a 56 e8
    xor dh, dh                                ; 30 f6
    or ax, dx                                 ; 09 d0
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, strict byte 00006h                ; 83 c2 06
    out DX, AL                                ; ee
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, strict byte 00007h                ; 83 c2 07
    mov al, byte [bp-01eh]                    ; 8a 46 e2
    out DX, AL                                ; ee
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov bl, al                                ; 88 c3
    test AL, strict byte 080h                 ; a8 80
    jne short 029e7h                          ; 75 f1
    test AL, strict byte 001h                 ; a8 01
    je short 02a09h                           ; 74 0f
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov ax, strict word 00002h                ; b8 02 00
    jmp near 02a9eh                           ; e9 95 00
    test bl, 008h                             ; f6 c3 08
    jne short 02a1dh                          ; 75 0f
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov ax, strict word 00003h                ; b8 03 00
    jmp near 02a9eh                           ; e9 81 00
    sti                                       ; fb
    mov ax, word [bp-016h]                    ; 8b 46 ea
    cmp ax, 0f800h                            ; 3d 00 f8
    jc short 02a38h                           ; 72 12
    mov dx, ax                                ; 89 c2
    sub dx, 00800h                            ; 81 ea 00 08
    mov ax, word [bp-012h]                    ; 8b 46 ee
    add ax, 00080h                            ; 05 80 00
    mov word [bp-016h], dx                    ; 89 56 ea
    mov word [bp-012h], ax                    ; 89 46 ee
    mov dx, word [bp-008h]                    ; 8b 56 f8
    mov cx, word [bp-01ch]                    ; 8b 4e e4
    mov si, word [bp-016h]                    ; 8b 76 ea
    mov es, [bp-012h]                         ; 8e 46 ee
    db  0f3h, 026h, 06fh
    ; rep es outsw                              ; f3 26 6f
    mov word [bp-016h], si                    ; 89 76 ea
    mov es, [bp-00ah]                         ; 8e 46 f6
    inc word [es:di+018h]                     ; 26 ff 45 18
    dec word [bp-01ah]                        ; ff 4e e6
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov bl, al                                ; 88 c3
    test AL, strict byte 080h                 ; a8 80
    jne short 02a54h                          ; 75 f1
    cmp word [bp-01ah], strict byte 00000h    ; 83 7e e6 00
    jne short 02a7dh                          ; 75 14
    and AL, strict byte 0e9h                  ; 24 e9
    cmp AL, strict byte 040h                  ; 3c 40
    je short 02a93h                           ; 74 24
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov ax, strict word 00006h                ; b8 06 00
    jmp short 02a9eh                          ; eb 21
    mov al, bl                                ; 88 d8
    and AL, strict byte 0c9h                  ; 24 c9
    cmp AL, strict byte 048h                  ; 3c 48
    je short 02a1eh                           ; 74 99
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov ax, strict word 00007h                ; b8 07 00
    jmp short 02a9eh                          ; eb 0b
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    xor ax, ax                                ; 31 c0
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
@ata_read_sectors:                           ; 0xf2aa5 LB 0xc1
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00006h                ; 83 ec 06
    mov si, word [bp+004h]                    ; 8b 76 04
    mov es, [bp+006h]                         ; 8e 46 06
    mov al, byte [es:si+00ch]                 ; 26 8a 44 0c
    mov bx, word [es:si+00eh]                 ; 26 8b 5c 0e
    mov CL, strict byte 009h                  ; b1 09
    mov dx, bx                                ; 89 da
    sal dx, CL                                ; d3 e2
    mov cx, dx                                ; 89 d1
    cmp word [es:si+016h], strict byte 00000h ; 26 83 7c 16 00
    je short 02aedh                           ; 74 23
    xor ah, ah                                ; 30 e4
    mov di, strict word 0001ch                ; bf 1c 00
    imul di                                   ; f7 ef
    mov dx, es                                ; 8c c2
    mov [bp-00ah], es                         ; 8c 46 f6
    mov di, si                                ; 89 f7
    add di, ax                                ; 01 c7
    mov word [es:di+028h], cx                 ; 26 89 4d 28
    mov cx, bx                                ; 89 d9
    mov bx, 000c4h                            ; bb c4 00
    mov ax, si                                ; 89 f0
    call 01e9dh                               ; e8 b5 f3
    mov es, [bp-00ah]                         ; 8e 46 f6
    jmp short 02b57h                          ; eb 6a
    xor di, di                                ; 31 ff
    mov word [bp-008h], di                    ; 89 7e f8
    mov word [bp-00ah], di                    ; 89 7e f6
    mov dx, word [es:si]                      ; 26 8b 14
    add dx, bx                                ; 01 da
    mov word [bp-006h], dx                    ; 89 56 fa
    adc di, word [es:si+002h]                 ; 26 13 7c 02
    mov dx, word [es:si+004h]                 ; 26 8b 54 04
    adc dx, word [bp-008h]                    ; 13 56 f8
    mov word [bp-008h], dx                    ; 89 56 f8
    mov dx, word [es:si+006h]                 ; 26 8b 54 06
    adc dx, word [bp-00ah]                    ; 13 56 f6
    test dx, dx                               ; 85 d2
    jnbe short 02b28h                         ; 77 12
    jne short 02b36h                          ; 75 1e
    cmp word [bp-008h], strict byte 00000h    ; 83 7e f8 00
    jnbe short 02b28h                         ; 77 0a
    jne short 02b36h                          ; 75 16
    cmp di, 01000h                            ; 81 ff 00 10
    jnbe short 02b28h                         ; 77 02
    jne short 02b36h                          ; 75 0e
    mov cx, bx                                ; 89 d9
    mov bx, strict word 00024h                ; bb 24 00
    mov ax, si                                ; 89 f0
    mov dx, es                                ; 8c c2
    call 01e9dh                               ; e8 69 f3
    jmp short 02b5dh                          ; eb 27
    xor ah, ah                                ; 30 e4
    mov di, strict word 0001ch                ; bf 1c 00
    imul di                                   ; f7 ef
    mov dx, es                                ; 8c c2
    mov [bp-006h], es                         ; 8c 46 fa
    mov di, si                                ; 89 f7
    add di, ax                                ; 01 c7
    mov word [es:di+028h], cx                 ; 26 89 4d 28
    mov cx, bx                                ; 89 d9
    mov bx, 000c4h                            ; bb c4 00
    mov ax, si                                ; 89 f0
    call 01e9dh                               ; e8 49 f3
    mov es, [bp-006h]                         ; 8e 46 fa
    mov word [es:di+028h], 00200h             ; 26 c7 45 28 00 02
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 00004h                               ; c2 04 00
@ata_write_sectors:                          ; 0xf2b66 LB 0x5b
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    les si, [bp+004h]                         ; c4 76 04
    mov cx, word [es:si+00eh]                 ; 26 8b 4c 0e
    cmp word [es:si+016h], strict byte 00000h ; 26 83 7c 16 00
    je short 02b86h                           ; 74 0c
    mov bx, strict word 00030h                ; bb 30 00
    mov ax, si                                ; 89 f0
    mov dx, es                                ; 8c c2
    call 02817h                               ; e8 93 fc
    jmp short 02bb8h                          ; eb 32
    xor ax, ax                                ; 31 c0
    xor bx, bx                                ; 31 db
    xor dx, dx                                ; 31 d2
    mov di, word [es:si]                      ; 26 8b 3c
    add di, cx                                ; 01 cf
    mov word [bp-006h], di                    ; 89 7e fa
    adc ax, word [es:si+002h]                 ; 26 13 44 02
    adc bx, word [es:si+004h]                 ; 26 13 5c 04
    adc dx, word [es:si+006h]                 ; 26 13 54 06
    test dx, dx                               ; 85 d2
    jnbe short 02bb3h                         ; 77 0f
    jne short 02b7ah                          ; 75 d4
    test bx, bx                               ; 85 db
    jnbe short 02bb3h                         ; 77 09
    jne short 02b7ah                          ; 75 ce
    cmp ax, 01000h                            ; 3d 00 10
    jnbe short 02bb3h                         ; 77 02
    jne short 02b7ah                          ; 75 c7
    mov bx, strict word 00034h                ; bb 34 00
    jmp short 02b7dh                          ; eb c5
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 00004h                               ; c2 04 00
ata_cmd_packet_:                             ; 0xf2bc1 LB 0x28b
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00010h                ; 83 ec 10
    push ax                                   ; 50
    mov byte [bp-006h], dl                    ; 88 56 fa
    mov si, bx                                ; 89 de
    mov di, cx                                ; 89 cf
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 94 ea
    mov word [bp-010h], 00122h                ; c7 46 f0 22 01
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov ax, word [bp-016h]                    ; 8b 46 ea
    shr ax, 1                                 ; d1 e8
    mov cl, byte [bp-016h]                    ; 8a 4e ea
    and cl, 001h                              ; 80 e1 01
    cmp byte [bp+00ah], 002h                  ; 80 7e 0a 02
    jne short 02c16h                          ; 75 23
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 35 ed
    mov ax, 00221h                            ; b8 21 02
    push ax                                   ; 50
    mov ax, 00230h                            ; b8 30 02
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 69 ed
    add sp, strict byte 00006h                ; 83 c4 06
    mov ax, strict word 00001h                ; b8 01 00
    jmp near 02e43h                           ; e9 2d 02
    test byte [bp+004h], 001h                 ; f6 46 04 01
    jne short 02c10h                          ; 75 f4
    xor ah, ah                                ; 30 e4
    mov dx, strict word 00006h                ; ba 06 00
    imul dx                                   ; f7 ea
    les bx, [bp-010h]                         ; c4 5e f0
    add bx, ax                                ; 01 c3
    mov ax, word [es:bx+00206h]               ; 26 8b 87 06 02
    mov word [bp-012h], ax                    ; 89 46 ee
    mov ax, word [es:bx+00208h]               ; 26 8b 87 08 02
    mov word [bp-008h], ax                    ; 89 46 f8
    xor ax, ax                                ; 31 c0
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov word [bp-00ah], ax                    ; 89 46 f6
    mov al, byte [bp-006h]                    ; 8a 46 fa
    cmp AL, strict byte 00ch                  ; 3c 0c
    jnc short 02c4dh                          ; 73 06
    mov byte [bp-006h], 00ch                  ; c6 46 fa 0c
    jmp short 02c53h                          ; eb 06
    jbe short 02c53h                          ; 76 04
    mov byte [bp-006h], 010h                  ; c6 46 fa 10
    shr byte [bp-006h], 1                     ; d0 6e fa
    les bx, [bp-010h]                         ; c4 5e f0
    mov word [es:bx+018h], strict word 00000h ; 26 c7 47 18 00 00
    mov word [es:bx+01ah], strict word 00000h ; 26 c7 47 1a 00 00
    mov word [es:bx+01ch], strict word 00000h ; 26 c7 47 1c 00 00
    mov dx, word [bp-012h]                    ; 8b 56 ee
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 080h                 ; a8 80
    je short 02c7eh                           ; 74 06
    mov ax, strict word 00002h                ; b8 02 00
    jmp near 02e43h                           ; e9 c5 01
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 00ah                  ; b0 0a
    out DX, AL                                ; ee
    mov dx, word [bp-012h]                    ; 8b 56 ee
    add dx, strict byte 00004h                ; 83 c2 04
    mov AL, strict byte 0f0h                  ; b0 f0
    out DX, AL                                ; ee
    mov dx, word [bp-012h]                    ; 8b 56 ee
    add dx, strict byte 00005h                ; 83 c2 05
    mov AL, strict byte 0ffh                  ; b0 ff
    out DX, AL                                ; ee
    test cl, cl                               ; 84 c9
    je short 02ca2h                           ; 74 05
    mov ax, 000b0h                            ; b8 b0 00
    jmp short 02ca5h                          ; eb 03
    mov ax, 000a0h                            ; b8 a0 00
    mov dx, word [bp-012h]                    ; 8b 56 ee
    add dx, strict byte 00006h                ; 83 c2 06
    out DX, AL                                ; ee
    mov dx, word [bp-012h]                    ; 8b 56 ee
    add dx, strict byte 00007h                ; 83 c2 07
    mov AL, strict byte 0a0h                  ; b0 a0
    out DX, AL                                ; ee
    mov dx, word [bp-012h]                    ; 8b 56 ee
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov bl, al                                ; 88 c3
    test AL, strict byte 080h                 ; a8 80
    jne short 02cb5h                          ; 75 f1
    test AL, strict byte 001h                 ; a8 01
    je short 02cd7h                           ; 74 0f
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov ax, strict word 00003h                ; b8 03 00
    jmp near 02e43h                           ; e9 6c 01
    test bl, 008h                             ; f6 c3 08
    jne short 02cebh                          ; 75 0f
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov ax, strict word 00004h                ; b8 04 00
    jmp near 02e43h                           ; e9 58 01
    sti                                       ; fb
    mov CL, strict byte 004h                  ; b1 04
    mov ax, si                                ; 89 f0
    shr ax, CL                                ; d3 e8
    add di, ax                                ; 01 c7
    and si, strict byte 0000fh                ; 83 e6 0f
    mov cl, byte [bp-006h]                    ; 8a 4e fa
    xor ch, ch                                ; 30 ed
    mov dx, word [bp-012h]                    ; 8b 56 ee
    mov es, di                                ; 8e c7
    db  0f3h, 026h, 06fh
    ; rep es outsw                              ; f3 26 6f
    cmp byte [bp+00ah], 000h                  ; 80 7e 0a 00
    jne short 02d15h                          ; 75 0b
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov bl, al                                ; 88 c3
    jmp near 02e24h                           ; e9 0f 01
    mov dx, word [bp-012h]                    ; 8b 56 ee
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov bl, al                                ; 88 c3
    test AL, strict byte 080h                 ; a8 80
    jne short 02d15h                          ; 75 f1
    test AL, strict byte 088h                 ; a8 88
    je short 02d8ah                           ; 74 62
    test AL, strict byte 001h                 ; a8 01
    je short 02d37h                           ; 74 0b
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    jmp short 02cd1h                          ; eb 9a
    mov al, bl                                ; 88 d8
    and AL, strict byte 0c9h                  ; 24 c9
    cmp AL, strict byte 048h                  ; 3c 48
    je short 02d4ah                           ; 74 0b
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    jmp short 02ce5h                          ; eb 9b
    mov CL, strict byte 004h                  ; b1 04
    mov ax, word [bp+00ch]                    ; 8b 46 0c
    shr ax, CL                                ; d3 e8
    mov dx, word [bp+00eh]                    ; 8b 56 0e
    add dx, ax                                ; 01 c2
    mov ax, word [bp+00ch]                    ; 8b 46 0c
    and ax, strict word 0000fh                ; 25 0f 00
    mov word [bp+00ch], ax                    ; 89 46 0c
    mov word [bp+00eh], dx                    ; 89 56 0e
    mov dx, word [bp-012h]                    ; 8b 56 ee
    add dx, strict byte 00005h                ; 83 c2 05
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov bh, al                                ; 88 c7
    xor bl, bl                                ; 30 db
    mov dx, word [bp-012h]                    ; 8b 56 ee
    add dx, strict byte 00004h                ; 83 c2 04
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    add bx, ax                                ; 01 c3
    mov ax, word [bp+004h]                    ; 8b 46 04
    cmp bx, ax                                ; 39 c3
    jnc short 02d8dh                          ; 73 0c
    mov cx, bx                                ; 89 d9
    sub word [bp+004h], bx                    ; 29 5e 04
    xor bx, bx                                ; 31 db
    jmp short 02d96h                          ; eb 0c
    jmp near 02e24h                           ; e9 97 00
    mov cx, ax                                ; 89 c1
    mov word [bp+004h], strict word 00000h    ; c7 46 04 00 00
    sub bx, ax                                ; 29 c3
    xor ax, ax                                ; 31 c0
    cmp word [bp+008h], strict byte 00000h    ; 83 7e 08 00
    jne short 02db4h                          ; 75 16
    cmp bx, word [bp+006h]                    ; 3b 5e 06
    jbe short 02db4h                          ; 76 11
    sub bx, word [bp+006h]                    ; 2b 5e 06
    mov word [bp-014h], bx                    ; 89 5e ec
    mov bx, word [bp+006h]                    ; 8b 5e 06
    mov word [bp+006h], ax                    ; 89 46 06
    mov word [bp+008h], ax                    ; 89 46 08
    jmp short 02dbdh                          ; eb 09
    mov word [bp-014h], ax                    ; 89 46 ec
    sub word [bp+006h], bx                    ; 29 5e 06
    sbb word [bp+008h], ax                    ; 19 46 08
    mov si, bx                                ; 89 de
    test cl, 003h                             ; f6 c1 03
    test bl, 003h                             ; f6 c3 03
    test byte [bp-014h], 003h                 ; f6 46 ec 03
    test bl, 001h                             ; f6 c3 01
    je short 02ddeh                           ; 74 10
    inc bx                                    ; 43
    cmp word [bp-014h], strict byte 00000h    ; 83 7e ec 00
    jbe short 02ddeh                          ; 76 09
    test byte [bp-014h], 001h                 ; f6 46 ec 01
    je short 02ddeh                           ; 74 03
    dec word [bp-014h]                        ; ff 4e ec
    shr bx, 1                                 ; d1 eb
    shr cx, 1                                 ; d1 e9
    shr word [bp-014h], 1                     ; d1 6e ec
    test cx, cx                               ; 85 c9
    je short 02defh                           ; 74 06
    mov dx, word [bp-012h]                    ; 8b 56 ee
    in ax, DX                                 ; ed
    loop 02dech                               ; e2 fd
    mov dx, word [bp-012h]                    ; 8b 56 ee
    mov cx, bx                                ; 89 d9
    les di, [bp+00ch]                         ; c4 7e 0c
    rep insw                                  ; f3 6d
    mov ax, word [bp-014h]                    ; 8b 46 ec
    test ax, ax                               ; 85 c0
    je short 02e05h                           ; 74 05
    mov cx, ax                                ; 89 c1
    in ax, DX                                 ; ed
    loop 02e02h                               ; e2 fd
    add word [bp+00ch], si                    ; 01 76 0c
    xor ax, ax                                ; 31 c0
    add word [bp-00ch], si                    ; 01 76 f4
    adc word [bp-00ah], ax                    ; 11 46 f6
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    les bx, [bp-010h]                         ; c4 5e f0
    mov word [es:bx+01ah], ax                 ; 26 89 47 1a
    mov ax, word [bp-00ah]                    ; 8b 46 f6
    mov word [es:bx+01ch], ax                 ; 26 89 47 1c
    jmp near 02d15h                           ; e9 f1 fe
    mov al, bl                                ; 88 d8
    and AL, strict byte 0e9h                  ; 24 e9
    cmp AL, strict byte 040h                  ; 3c 40
    je short 02e38h                           ; 74 0c
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    jmp near 02ce5h                           ; e9 ad fe
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    xor ax, ax                                ; 31 c0
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 0000ch                               ; c2 0c 00
ata_soft_reset_:                             ; 0xf2e4c LB 0x84
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push ax                                   ; 50
    mov bx, ax                                ; 89 c3
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 10 e8
    mov es, ax                                ; 8e c0
    mov ax, bx                                ; 89 d8
    shr ax, 1                                 ; d1 e8
    and bl, 001h                              ; 80 e3 01
    mov byte [bp-008h], bl                    ; 88 5e f8
    xor ah, ah                                ; 30 e4
    mov dx, strict word 00006h                ; ba 06 00
    imul dx                                   ; f7 ea
    mov bx, ax                                ; 89 c3
    add bx, 00122h                            ; 81 c3 22 01
    mov cx, word [es:bx+00206h]               ; 26 8b 8f 06 02
    mov bx, word [es:bx+00208h]               ; 26 8b 9f 08 02
    lea dx, [bx+006h]                         ; 8d 57 06
    mov AL, strict byte 00ah                  ; b0 0a
    out DX, AL                                ; ee
    cmp byte [bp-008h], 000h                  ; 80 7e f8 00
    je short 02e92h                           ; 74 05
    mov ax, 000b0h                            ; b8 b0 00
    jmp short 02e95h                          ; eb 03
    mov ax, 000a0h                            ; b8 a0 00
    mov dx, cx                                ; 89 ca
    add dx, strict byte 00006h                ; 83 c2 06
    out DX, AL                                ; ee
    mov dx, cx                                ; 89 ca
    add dx, strict byte 00007h                ; 83 c2 07
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov dx, cx                                ; 89 ca
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 080h                 ; a8 80
    jne short 02ea3h                          ; 75 f4
    and AL, strict byte 0e9h                  ; 24 e9
    cmp AL, strict byte 040h                  ; 3c 40
    je short 02ec0h                           ; 74 0b
    lea dx, [bx+006h]                         ; 8d 57 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov ax, strict word 00001h                ; b8 01 00
    jmp short 02ec8h                          ; eb 08
    lea dx, [bx+006h]                         ; 8d 57 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    xor ax, ax                                ; 31 c0
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
set_diskette_current_cyl_:                   ; 0xf2ed0 LB 0x2c
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    mov bl, al                                ; 88 c3
    cmp AL, strict byte 001h                  ; 3c 01
    jbe short 02ee8h                          ; 76 0e
    mov ax, 00250h                            ; b8 50 02
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 91 ea
    add sp, strict byte 00004h                ; 83 c4 04
    xor bh, bh                                ; 30 ff
    add bx, 00094h                            ; 81 c3 94 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], dl                      ; 26 88 17
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
floppy_wait_for_interrupt_:                  ; 0xf2efc LB 0x23
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    cli                                       ; fa
    mov bx, strict word 0003eh                ; bb 3e 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov al, byte [es:bx]                      ; 26 8a 07
    test AL, strict byte 080h                 ; a8 80
    je short 02f14h                           ; 74 04
    and AL, strict byte 080h                  ; 24 80
    jmp short 02f19h                          ; eb 05
    sti                                       ; fb
    hlt                                       ; f4
    cli                                       ; fa
    jmp short 02f01h                          ; eb e8
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
floppy_wait_for_interrupt_or_timeout_:       ; 0xf2f1f LB 0x38
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    cli                                       ; fa
    mov bx, strict word 00040h                ; bb 40 00
    mov es, bx                                ; 8e c3
    mov al, byte [es:bx]                      ; 26 8a 07
    test al, al                               ; 84 c0
    jne short 02f33h                          ; 75 03
    sti                                       ; fb
    jmp short 02f51h                          ; eb 1e
    mov bx, strict word 0003eh                ; bb 3e 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov al, byte [es:bx]                      ; 26 8a 07
    test AL, strict byte 080h                 ; a8 80
    je short 02f4ch                           ; 74 0a
    mov ah, al                                ; 88 c4
    and ah, 07fh                              ; 80 e4 7f
    mov byte [es:bx], ah                      ; 26 88 27
    jmp short 02f51h                          ; eb 05
    sti                                       ; fb
    hlt                                       ; f4
    cli                                       ; fa
    jmp short 02f24h                          ; eb d3
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
floppy_reset_controller_:                    ; 0xf2f57 LB 0x3f
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    mov cx, ax                                ; 89 c1
    mov dx, 003f2h                            ; ba f2 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov bx, ax                                ; 89 c3
    and AL, strict byte 0fbh                  ; 24 fb
    out DX, AL                                ; ee
    mov al, bl                                ; 88 d8
    or AL, strict byte 004h                   ; 0c 04
    out DX, AL                                ; ee
    mov dx, 003f4h                            ; ba f4 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 0c0h                  ; 24 c0
    cmp AL, strict byte 080h                  ; 3c 80
    jne short 02f6fh                          ; 75 f4
    mov bx, cx                                ; 89 cb
    add bx, 00090h                            ; 81 c3 90 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov al, byte [es:bx]                      ; 26 8a 07
    and AL, strict byte 0efh                  ; 24 ef
    mov byte [es:bx], al                      ; 26 88 07
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
floppy_prepare_controller_:                  ; 0xf2f96 LB 0x75
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push ax                                   ; 50
    mov cx, ax                                ; 89 c1
    mov bx, strict word 0003eh                ; bb 3e 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov al, byte [es:bx]                      ; 26 8a 07
    and AL, strict byte 07fh                  ; 24 7f
    mov byte [es:bx], al                      ; 26 88 07
    mov dx, 003f2h                            ; ba f2 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 004h                  ; 24 04
    mov byte [bp-008h], al                    ; 88 46 f8
    test cx, cx                               ; 85 c9
    je short 02fc2h                           ; 74 04
    mov AL, strict byte 020h                  ; b0 20
    jmp short 02fc4h                          ; eb 02
    mov AL, strict byte 010h                  ; b0 10
    or AL, strict byte 00ch                   ; 0c 0c
    or al, cl                                 ; 08 c8
    mov dx, 003f2h                            ; ba f2 03
    out DX, AL                                ; ee
    mov bx, strict word 00040h                ; bb 40 00
    mov es, bx                                ; 8e c3
    mov byte [es:bx], 025h                    ; 26 c6 07 25
    mov bx, 0008bh                            ; bb 8b 00
    mov al, byte [es:bx]                      ; 26 8a 07
    mov CL, strict byte 006h                  ; b1 06
    shr al, CL                                ; d2 e8
    mov dx, 003f7h                            ; ba f7 03
    out DX, AL                                ; ee
    mov dx, 003f4h                            ; ba f4 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 0c0h                  ; 24 c0
    cmp AL, strict byte 080h                  ; 3c 80
    jne short 02fe3h                          ; 75 f4
    cmp byte [bp-008h], 000h                  ; 80 7e f8 00
    jne short 03003h                          ; 75 0e
    call 02efch                               ; e8 04 ff
    mov bx, strict word 0003eh                ; bb 3e 00
    mov dx, strict word 00040h                ; ba 40 00
    mov es, dx                                ; 8e c2
    mov byte [es:bx], al                      ; 26 88 07
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
floppy_media_known_:                         ; 0xf300b LB 0x4c
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov bx, strict word 0003eh                ; bb 3e 00
    mov dx, strict word 00040h                ; ba 40 00
    mov es, dx                                ; 8e c2
    mov bh, byte [es:bx]                      ; 26 8a 3f
    mov bl, bh                                ; 88 fb
    test ax, ax                               ; 85 c0
    je short 03024h                           ; 74 02
    shr bl, 1                                 ; d0 eb
    and bl, 001h                              ; 80 e3 01
    jne short 0302dh                          ; 75 04
    xor bh, bh                                ; 30 ff
    jmp short 03050h                          ; eb 23
    mov bx, 00090h                            ; bb 90 00
    test ax, ax                               ; 85 c0
    je short 03037h                           ; 74 03
    mov bx, 00091h                            ; bb 91 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov al, byte [es:bx]                      ; 26 8a 07
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 004h                  ; b1 04
    sar ax, CL                                ; d3 f8
    and AL, strict byte 001h                  ; 24 01
    jne short 0304dh                          ; 75 04
    xor bx, bx                                ; 31 db
    jmp short 03050h                          ; eb 03
    mov bx, strict word 00001h                ; bb 01 00
    mov ax, bx                                ; 89 d8
    pop bp                                    ; 5d
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    retn                                      ; c3
floppy_read_id_:                             ; 0xf3057 LB 0x52
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push dx                                   ; 52
    push si                                   ; 56
    mov bx, ax                                ; 89 c3
    call 02f96h                               ; e8 34 ff
    mov AL, strict byte 04ah                  ; b0 4a
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    mov al, bl                                ; 88 d8
    out DX, AL                                ; ee
    call 02efch                               ; e8 8e fe
    xor bx, bx                                ; 31 db
    jmp short 03077h                          ; eb 05
    cmp bx, strict byte 00007h                ; 83 fb 07
    jnl short 0308bh                          ; 7d 14
    mov dx, 003f5h                            ; ba f5 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    lea si, [bx+042h]                         ; 8d 77 42
    mov dx, strict word 00040h                ; ba 40 00
    mov es, dx                                ; 8e c2
    mov byte [es:si], al                      ; 26 88 04
    inc bx                                    ; 43
    jmp short 03072h                          ; eb e7
    mov bx, strict word 00042h                ; bb 42 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov al, byte [es:bx]                      ; 26 8a 07
    test AL, strict byte 0c0h                 ; a8 c0
    je short 0309eh                           ; 74 04
    xor ax, ax                                ; 31 c0
    jmp short 030a1h                          ; eb 03
    mov ax, strict word 00001h                ; b8 01 00
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
floppy_drive_recal_:                         ; 0xf30a9 LB 0x41
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push dx                                   ; 52
    push si                                   ; 56
    mov bx, ax                                ; 89 c3
    call 02f96h                               ; e8 e2 fe
    mov AL, strict byte 007h                  ; b0 07
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    mov al, bl                                ; 88 d8
    out DX, AL                                ; ee
    call 02efch                               ; e8 3c fe
    test bx, bx                               ; 85 db
    je short 030cbh                           ; 74 07
    or AL, strict byte 002h                   ; 0c 02
    mov bx, 00095h                            ; bb 95 00
    jmp short 030d0h                          ; eb 05
    or AL, strict byte 001h                   ; 0c 01
    mov bx, 00094h                            ; bb 94 00
    mov si, strict word 0003eh                ; be 3e 00
    mov dx, strict word 00040h                ; ba 40 00
    mov es, dx                                ; 8e c2
    mov byte [es:si], al                      ; 26 88 04
    mov byte [es:bx], 000h                    ; 26 c6 07 00
    mov ax, strict word 00001h                ; b8 01 00
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
floppy_media_sense_:                         ; 0xf30ea LB 0xe7
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push si                                   ; 56
    push di                                   ; 57
    mov si, ax                                ; 89 c6
    call 030a9h                               ; e8 b2 ff
    test ax, ax                               ; 85 c0
    jne short 03100h                          ; 75 05
    xor dx, dx                                ; 31 d2
    jmp near 031c5h                           ; e9 c5 00
    mov ax, strict word 00010h                ; b8 10 00
    call 016aeh                               ; e8 a8 e5
    test si, si                               ; 85 f6
    jne short 03112h                          ; 75 08
    mov CL, strict byte 004h                  ; b1 04
    mov dl, al                                ; 88 c2
    shr dl, CL                                ; d2 ea
    jmp short 03117h                          ; eb 05
    mov dl, al                                ; 88 c2
    and dl, 00fh                              ; 80 e2 0f
    cmp dl, 001h                              ; 80 fa 01
    jne short 03125h                          ; 75 09
    xor dl, dl                                ; 30 d2
    mov DH, strict byte 015h                  ; b6 15
    mov bx, strict word 00001h                ; bb 01 00
    jmp short 03163h                          ; eb 3e
    cmp dl, 002h                              ; 80 fa 02
    jne short 03130h                          ; 75 06
    xor dl, dl                                ; 30 d2
    mov DH, strict byte 035h                  ; b6 35
    jmp short 03120h                          ; eb f0
    cmp dl, 003h                              ; 80 fa 03
    jne short 0313bh                          ; 75 06
    xor dl, dl                                ; 30 d2
    mov DH, strict byte 017h                  ; b6 17
    jmp short 03120h                          ; eb e5
    cmp dl, 004h                              ; 80 fa 04
    jne short 03146h                          ; 75 06
    xor dl, dl                                ; 30 d2
    mov DH, strict byte 017h                  ; b6 17
    jmp short 03120h                          ; eb da
    cmp dl, 005h                              ; 80 fa 05
    jne short 03151h                          ; 75 06
    mov DL, strict byte 0cch                  ; b2 cc
    mov DH, strict byte 0d7h                  ; b6 d7
    jmp short 03120h                          ; eb cf
    cmp dl, 00eh                              ; 80 fa 0e
    je short 0315bh                           ; 74 05
    cmp dl, 00fh                              ; 80 fa 0f
    jne short 0315dh                          ; 75 02
    jmp short 0314bh                          ; eb ee
    xor dl, dl                                ; 30 d2
    xor dh, dh                                ; 30 f6
    xor bx, bx                                ; 31 db
    mov di, 0008bh                            ; bf 8b 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:di], dl                      ; 26 88 15
    mov ax, si                                ; 89 f0
    call 03057h                               ; e8 e4 fe
    test ax, ax                               ; 85 c0
    jne short 031a9h                          ; 75 32
    mov al, dl                                ; 88 d0
    and AL, strict byte 0c0h                  ; 24 c0
    cmp AL, strict byte 080h                  ; 3c 80
    je short 031a9h                           ; 74 2a
    mov al, dl                                ; 88 d0
    and AL, strict byte 0c0h                  ; 24 c0
    cmp AL, strict byte 0c0h                  ; 3c c0
    je short 03196h                           ; 74 0f
    mov ah, dl                                ; 88 d4
    and ah, 03fh                              ; 80 e4 3f
    cmp AL, strict byte 040h                  ; 3c 40
    je short 031a2h                           ; 74 12
    test al, al                               ; 84 c0
    je short 0319bh                           ; 74 07
    jmp short 03163h                          ; eb cd
    and dl, 03fh                              ; 80 e2 3f
    jmp short 03163h                          ; eb c8
    mov dl, ah                                ; 88 e2
    or dl, 040h                               ; 80 ca 40
    jmp short 03163h                          ; eb c1
    mov dl, ah                                ; 88 e2
    or dl, 080h                               ; 80 ca 80
    jmp short 03163h                          ; eb ba
    test si, si                               ; 85 f6
    jne short 031b2h                          ; 75 05
    mov si, 00090h                            ; be 90 00
    jmp short 031b5h                          ; eb 03
    mov si, 00091h                            ; be 91 00
    mov di, 0008bh                            ; bf 8b 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:di], dl                      ; 26 88 15
    mov byte [es:si], dh                      ; 26 88 34
    mov dx, bx                                ; 89 da
    mov ax, dx                                ; 89 d0
    lea sp, [bp-00ah]                         ; 8d 66 f6
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
floppy_drive_exists_:                        ; 0xf31d1 LB 0x4e
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push cx                                   ; 51
    push dx                                   ; 52
    mov dx, ax                                ; 89 c2
    mov ax, strict word 00010h                ; b8 10 00
    call 016aeh                               ; e8 d0 e4
    test dx, dx                               ; 85 d2
    jne short 031e8h                          ; 75 06
    mov CL, strict byte 004h                  ; b1 04
    shr al, CL                                ; d2 e8
    jmp short 031eah                          ; eb 02
    and AL, strict byte 00fh                  ; 24 0f
    test al, al                               ; 84 c0
    je short 031f3h                           ; 74 05
    mov ax, strict word 00001h                ; b8 01 00
    jmp short 031f5h                          ; eb 02
    xor ah, ah                                ; 30 e4
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bp                                    ; 5d
    retn                                      ; c3
    sbb byte [bx], dl                         ; 18 17
    push SS                                   ; 16
    adc ax, 00508h                            ; 15 08 05
    add AL, strict byte 003h                  ; 04 03
    add al, byte [bx+di]                      ; 02 01
    add dh, dh                                ; 00 f6
    cmp bx, word [di+032h]                    ; 3b 5d 32
    fbstp [bp+si]                             ; df 32
    stc                                       ; f9
    db  032h, 0f9h
    ; xor bh, cl                                ; 32 f9
    db  032h, 0f9h
    ; xor bh, cl                                ; 32 f9
    xor ch, byte [bp+si-071cah]               ; 32 aa 36 8e
    cmp byte [bx+di-033c7h], cl               ; 38 89 39 cc
    cmp word [bx+si], ax                      ; 39 00
    cmp dh, byte [si+03ah]                    ; 3a 74 3a
_int13_diskette_function:                    ; 0xf321f LB 0x9fd
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    sub sp, strict byte 0000eh                ; 83 ec 0e
    or byte [bp+01dh], 002h                   ; 80 4e 1d 02
    mov al, byte [bp+017h]                    ; 8a 46 17
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    cmp ax, strict word 00018h                ; 3d 18 00
    jnbe short 03291h                         ; 77 5c
    push CS                                   ; 0e
    pop ES                                    ; 07
    mov cx, strict word 0000ch                ; b9 0c 00
    mov di, 031fch                            ; bf fc 31
    repne scasb                               ; f2 ae
    sal cx, 1                                 ; d1 e1
    mov di, cx                                ; 89 cf
    mov di, word [cs:di+03207h]               ; 2e 8b bd 07 32
    mov cx, word [bp+016h]                    ; 8b 4e 16
    xor ch, ch                                ; 30 ed
    mov dl, byte [bp+00eh]                    ; 8a 56 0e
    mov ax, word [bp+01ch]                    ; 8b 46 1c
    or AL, strict byte 001h                   ; 0c 01
    mov si, cx                                ; 89 ce
    or si, 00100h                             ; 81 ce 00 01
    jmp di                                    ; ff e7
    mov bl, byte [bp+00eh]                    ; 8a 5e 0e
    cmp bl, 001h                              ; 80 fb 01
    jbe short 0327fh                          ; 76 1a
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 001h                               ; 80 cc 01
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 001h                    ; 26 c6 07 01
    jmp near 03ae1h                           ; e9 62 08
    mov ax, strict word 00010h                ; b8 10 00
    call 016aeh                               ; e8 29 e4
    test bl, bl                               ; 84 db
    jne short 03294h                          ; 75 0b
    mov CL, strict byte 004h                  ; b1 04
    mov dl, al                                ; 88 c2
    shr dl, CL                                ; d2 ea
    jmp short 03299h                          ; eb 08
    jmp near 03bf6h                           ; e9 62 09
    mov dl, al                                ; 88 c2
    and dl, 00fh                              ; 80 e2 0f
    test dl, dl                               ; 84 d2
    jne short 032b7h                          ; 75 1a
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 080h                               ; 80 cc 80
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 080h                    ; 26 c6 07 80
    jmp near 03ae1h                           ; e9 2a 08
    mov si, strict word 0003eh                ; be 3e 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:si], 000h                    ; 26 c6 04 00
    xor al, al                                ; 30 c0
    mov byte [bp+017h], al                    ; 88 46 17
    mov si, strict word 00041h                ; be 41 00
    mov byte [es:si], al                      ; 26 88 04
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    xor dx, dx                                ; 31 d2
    call 02ed0h                               ; e8 f5 fb
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    mov bx, 00441h                            ; bb 41 04
    xor ax, ax                                ; 31 c0
    mov es, ax                                ; 8e c0
    mov dl, byte [es:bx]                      ; 26 8a 17
    mov ch, dl                                ; 88 d5
    mov word [bp+016h], cx                    ; 89 4e 16
    test dl, dl                               ; 84 d2
    je short 032dbh                           ; 74 e5
    jmp near 03ae1h                           ; e9 e8 07
    mov bh, byte [bp+016h]                    ; 8a 7e 16
    mov al, byte [bp+015h]                    ; 8a 46 15
    mov byte [bp-004h], al                    ; 88 46 fc
    mov al, byte [bp+014h]                    ; 8a 46 14
    mov byte [bp-002h], al                    ; 88 46 fe
    mov al, byte [bp+013h]                    ; 8a 46 13
    xor ah, ah                                ; 30 e4
    mov byte [bp-006h], al                    ; 88 46 fa
    mov bl, byte [bp+00eh]                    ; 8a 5e 0e
    cmp bl, 001h                              ; 80 fb 01
    jnbe short 03325h                         ; 77 0d
    cmp AL, strict byte 001h                  ; 3c 01
    jnbe short 03325h                         ; 77 09
    test bh, bh                               ; 84 ff
    je short 03325h                           ; 74 05
    cmp bh, 048h                              ; 80 ff 48
    jbe short 0335ch                          ; 76 37
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 03 e6
    mov ax, 00275h                            ; b8 75 02
    push ax                                   ; 50
    mov ax, 0028dh                            ; b8 8d 02
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 37 e6
    add sp, strict byte 00006h                ; 83 c4 06
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 001h                               ; 80 cc 01
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 001h                    ; 26 c6 07 01
    jmp near 03406h                           ; e9 aa 00
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    call 031d1h                               ; e8 6e fe
    test ax, ax                               ; 85 c0
    jne short 03381h                          ; 75 1a
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 080h                               ; 80 cc 80
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 080h                    ; 26 c6 07 80
    jmp near 03406h                           ; e9 85 00
    mov cl, bl                                ; 88 d9
    xor ch, ch                                ; 30 ed
    mov ax, cx                                ; 89 c8
    call 0300bh                               ; e8 81 fc
    test ax, ax                               ; 85 c0
    jne short 033b4h                          ; 75 26
    mov ax, cx                                ; 89 c8
    call 030eah                               ; e8 57 fd
    test ax, ax                               ; 85 c0
    jne short 033b4h                          ; 75 1d
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 00ch                               ; 80 cc 0c
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 00ch                    ; 26 c6 07 0c
    mov byte [bp+016h], ch                    ; 88 6e 16
    jmp near 03ae1h                           ; e9 2d 07
    mov al, byte [bp+017h]                    ; 8a 46 17
    xor ah, ah                                ; 30 e4
    cmp ax, strict word 00002h                ; 3d 02 00
    jne short 0340dh                          ; 75 4f
    mov CL, strict byte 00ch                  ; b1 0c
    mov ax, word [bp+006h]                    ; 8b 46 06
    shr ax, CL                                ; d3 e8
    mov ch, al                                ; 88 c5
    mov CL, strict byte 004h                  ; b1 04
    mov dx, word [bp+006h]                    ; 8b 56 06
    sal dx, CL                                ; d3 e2
    mov si, word [bp+010h]                    ; 8b 76 10
    add si, dx                                ; 01 d6
    mov word [bp-00ch], si                    ; 89 76 f4
    cmp dx, si                                ; 39 f2
    jbe short 033dch                          ; 76 02
    db  0feh, 0c5h
    ; inc ch                                    ; fe c5
    mov al, bh                                ; 88 f8
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 009h                  ; b1 09
    sal ax, CL                                ; d3 e0
    dec ax                                    ; 48
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    add ax, word [bp-00eh]                    ; 03 46 f2
    cmp ax, word [bp-00ch]                    ; 3b 46 f4
    jnc short 03410h                          ; 73 1d
    mov ax, word [bp+016h]                    ; 8b 46 16
    mov ah, cl                                ; 88 cc
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], cl                      ; 26 88 0f
    mov byte [bp+016h], 000h                  ; c6 46 16 00
    jmp near 03ae1h                           ; e9 d4 06
    jmp near 0355ah                           ; e9 4a 01
    mov AL, strict byte 006h                  ; b0 06
    mov dx, strict word 0000ah                ; ba 0a 00
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    mov dx, strict word 0000ch                ; ba 0c 00
    out DX, AL                                ; ee
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    mov dx, strict word 00004h                ; ba 04 00
    out DX, AL                                ; ee
    mov al, byte [bp-00bh]                    ; 8a 46 f5
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    mov dx, strict word 0000ch                ; ba 0c 00
    out DX, AL                                ; ee
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    mov dx, strict word 00005h                ; ba 05 00
    out DX, AL                                ; ee
    mov al, byte [bp-00dh]                    ; 8a 46 f3
    out DX, AL                                ; ee
    mov AL, strict byte 046h                  ; b0 46
    mov dx, strict word 0000bh                ; ba 0b 00
    out DX, AL                                ; ee
    mov al, ch                                ; 88 e8
    mov dx, 00081h                            ; ba 81 00
    out DX, AL                                ; ee
    mov AL, strict byte 002h                  ; b0 02
    mov dx, strict word 0000ah                ; ba 0a 00
    out DX, AL                                ; ee
    mov cl, bl                                ; 88 d9
    xor ch, ch                                ; 30 ed
    mov ax, cx                                ; 89 c8
    call 02f96h                               ; e8 43 fb
    mov AL, strict byte 0e6h                  ; b0 e6
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    mov al, byte [bp-006h]                    ; 8a 46 fa
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    sal dx, 1                                 ; d1 e2
    sal dx, 1                                 ; d1 e2
    mov al, bl                                ; 88 d8
    or ax, dx                                 ; 09 d0
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    mov al, byte [bp-004h]                    ; 8a 46 fc
    out DX, AL                                ; ee
    mov al, byte [bp-006h]                    ; 8a 46 fa
    out DX, AL                                ; ee
    mov al, byte [bp-002h]                    ; 8a 46 fe
    out DX, AL                                ; ee
    mov AL, strict byte 002h                  ; b0 02
    out DX, AL                                ; ee
    mov al, byte [bp-002h]                    ; 8a 46 fe
    xor ah, ah                                ; 30 e4
    mov dl, bh                                ; 88 fa
    xor dh, dh                                ; 30 f6
    add ax, dx                                ; 01 d0
    dec ax                                    ; 48
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    out DX, AL                                ; ee
    mov AL, strict byte 0ffh                  ; b0 ff
    out DX, AL                                ; ee
    call 02f1fh                               ; e8 8b fa
    test al, al                               ; 84 c0
    jne short 034b7h                          ; 75 1f
    mov ax, cx                                ; 89 c8
    call 02f57h                               ; e8 ba fa
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 080h                               ; 80 cc 80
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 080h                    ; 26 c6 07 80
    jmp near 033aeh                           ; e9 f7 fe
    mov dx, 003f4h                            ; ba f4 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 0c0h                  ; 24 c0
    cmp AL, strict byte 0c0h                  ; 3c c0
    je short 034d5h                           ; 74 12
    mov ax, 00275h                            ; b8 75 02
    push ax                                   ; 50
    mov ax, 002a8h                            ; b8 a8 02
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 a4 e4
    add sp, strict byte 00006h                ; 83 c4 06
    xor cx, cx                                ; 31 c9
    jmp short 034deh                          ; eb 05
    cmp cx, strict byte 00007h                ; 83 f9 07
    jnl short 034f4h                          ; 7d 16
    mov dx, 003f5h                            ; ba f5 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov si, cx                                ; 89 ce
    add si, strict byte 00042h                ; 83 c6 42
    mov dx, strict word 00040h                ; ba 40 00
    mov es, dx                                ; 8e c2
    mov byte [es:si], al                      ; 26 88 04
    inc cx                                    ; 41
    jmp short 034d9h                          ; eb e5
    mov si, strict word 00042h                ; be 42 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov al, byte [es:si]                      ; 26 8a 04
    test AL, strict byte 0c0h                 ; a8 c0
    je short 03524h                           ; 74 21
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    call 02f57h                               ; e8 4d fa
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 020h                               ; 80 cc 20
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 020h                    ; 26 c6 07 20
    jmp near 03406h                           ; e9 e2 fe
    mov al, bh                                ; 88 f8
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 009h                  ; b1 09
    sal ax, CL                                ; d3 e0
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    mov si, word [bp+010h]                    ; 8b 76 10
    mov dx, word [bp+006h]                    ; 8b 56 06
    mov di, si                                ; 89 f7
    mov cx, ax                                ; 89 c1
    mov es, dx                                ; 8e c2
    push DS                                   ; 1e
    mov ds, dx                                ; 8e da
    rep movsw                                 ; f3 a5
    pop DS                                    ; 1f
    mov dl, byte [bp-004h]                    ; 8a 56 fc
    xor dh, dh                                ; 30 f6
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    call 02ed0h                               ; e8 81 f9
    mov byte [bp+017h], 000h                  ; c6 46 17 00
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    jmp near 032dbh                           ; e9 81 fd
    mov al, byte [bp+017h]                    ; 8a 46 17
    xor ah, ah                                ; 30 e4
    cmp ax, strict word 00003h                ; 3d 03 00
    je short 03567h                           ; 74 03
    jmp near 03693h                           ; e9 2c 01
    mov CL, strict byte 00ch                  ; b1 0c
    mov dx, word [bp+006h]                    ; 8b 56 06
    shr dx, CL                                ; d3 ea
    mov ch, dl                                ; 88 d5
    mov CL, strict byte 004h                  ; b1 04
    mov ax, word [bp+006h]                    ; 8b 46 06
    sal ax, CL                                ; d3 e0
    mov si, word [bp+010h]                    ; 8b 76 10
    add si, ax                                ; 01 c6
    mov word [bp-00ch], si                    ; 89 76 f4
    cmp ax, si                                ; 39 f0
    jbe short 03585h                          ; 76 02
    db  0feh, 0c5h
    ; inc ch                                    ; fe c5
    mov al, bh                                ; 88 f8
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 009h                  ; b1 09
    sal ax, CL                                ; d3 e0
    dec ax                                    ; 48
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    add ax, word [bp-00eh]                    ; 03 46 f2
    cmp ax, word [bp-00ch]                    ; 3b 46 f4
    jnc short 0359fh                          ; 73 03
    jmp near 033f3h                           ; e9 54 fe
    mov AL, strict byte 006h                  ; b0 06
    mov dx, strict word 0000ah                ; ba 0a 00
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    mov dx, strict word 0000ch                ; ba 0c 00
    out DX, AL                                ; ee
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    mov dx, strict word 00004h                ; ba 04 00
    out DX, AL                                ; ee
    mov al, byte [bp-00bh]                    ; 8a 46 f5
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    mov dx, strict word 0000ch                ; ba 0c 00
    out DX, AL                                ; ee
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    mov dx, strict word 00005h                ; ba 05 00
    out DX, AL                                ; ee
    mov al, byte [bp-00dh]                    ; 8a 46 f3
    out DX, AL                                ; ee
    mov AL, strict byte 04ah                  ; b0 4a
    mov dx, strict word 0000bh                ; ba 0b 00
    out DX, AL                                ; ee
    mov al, ch                                ; 88 e8
    mov dx, 00081h                            ; ba 81 00
    out DX, AL                                ; ee
    mov AL, strict byte 002h                  ; b0 02
    mov dx, strict word 0000ah                ; ba 0a 00
    out DX, AL                                ; ee
    mov cl, bl                                ; 88 d9
    xor ch, ch                                ; 30 ed
    mov ax, cx                                ; 89 c8
    call 02f96h                               ; e8 b4 f9
    mov AL, strict byte 0c5h                  ; b0 c5
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    mov al, byte [bp-006h]                    ; 8a 46 fa
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    sal dx, 1                                 ; d1 e2
    sal dx, 1                                 ; d1 e2
    mov al, bl                                ; 88 d8
    or ax, dx                                 ; 09 d0
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    mov al, byte [bp-004h]                    ; 8a 46 fc
    out DX, AL                                ; ee
    mov al, byte [bp-006h]                    ; 8a 46 fa
    out DX, AL                                ; ee
    mov al, byte [bp-002h]                    ; 8a 46 fe
    out DX, AL                                ; ee
    mov AL, strict byte 002h                  ; b0 02
    out DX, AL                                ; ee
    mov dl, byte [bp-002h]                    ; 8a 56 fe
    xor dh, dh                                ; 30 f6
    mov al, bh                                ; 88 f8
    xor ah, ah                                ; 30 e4
    add ax, dx                                ; 01 d0
    dec ax                                    ; 48
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    out DX, AL                                ; ee
    mov AL, strict byte 0ffh                  ; b0 ff
    out DX, AL                                ; ee
    call 02f1fh                               ; e8 fc f8
    test al, al                               ; 84 c0
    jne short 0362ah                          ; 75 03
    jmp near 03498h                           ; e9 6e fe
    mov dx, 003f4h                            ; ba f4 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 0c0h                  ; 24 c0
    cmp AL, strict byte 0c0h                  ; 3c c0
    je short 03648h                           ; 74 12
    mov ax, 00275h                            ; b8 75 02
    push ax                                   ; 50
    mov ax, 002a8h                            ; b8 a8 02
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 31 e3
    add sp, strict byte 00006h                ; 83 c4 06
    xor cx, cx                                ; 31 c9
    jmp short 03651h                          ; eb 05
    cmp cx, strict byte 00007h                ; 83 f9 07
    jnl short 03667h                          ; 7d 16
    mov dx, 003f5h                            ; ba f5 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov si, cx                                ; 89 ce
    add si, strict byte 00042h                ; 83 c6 42
    mov dx, strict word 00040h                ; ba 40 00
    mov es, dx                                ; 8e c2
    mov byte [es:si], al                      ; 26 88 04
    inc cx                                    ; 41
    jmp short 0364ch                          ; eb e5
    mov si, strict word 00042h                ; be 42 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov al, byte [es:si]                      ; 26 8a 04
    test AL, strict byte 0c0h                 ; a8 c0
    jne short 03679h                          ; 75 03
    jmp near 03543h                           ; e9 ca fe
    mov bx, strict word 00043h                ; bb 43 00
    mov al, byte [es:bx]                      ; 26 8a 07
    test AL, strict byte 002h                 ; a8 02
    je short 0368bh                           ; 74 08
    mov word [bp+016h], 00300h                ; c7 46 16 00 03
    jmp near 03ae1h                           ; e9 56 04
    mov word [bp+016h], 00100h                ; c7 46 16 00 01
    jmp near 03ae1h                           ; e9 4e 04
    mov dl, byte [bp-004h]                    ; 8a 56 fc
    xor dh, dh                                ; 30 f6
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    call 02ed0h                               ; e8 31 f8
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    mov byte [bp+017h], 000h                  ; c6 46 17 00
    jmp near 032dbh                           ; e9 31 fc
    mov bh, byte [bp+016h]                    ; 8a 7e 16
    mov dl, byte [bp+015h]                    ; 8a 56 15
    xor dh, dh                                ; 30 f6
    mov byte [bp-004h], dl                    ; 88 56 fc
    mov al, byte [bp+013h]                    ; 8a 46 13
    xor ah, ah                                ; 30 e4
    mov byte [bp-006h], al                    ; 88 46 fa
    mov bl, byte [bp+00eh]                    ; 8a 5e 0e
    cmp bl, 001h                              ; 80 fb 01
    jnbe short 036d7h                         ; 77 12
    cmp AL, strict byte 001h                  ; 3c 01
    jnbe short 036d7h                         ; 77 0e
    cmp dl, 04fh                              ; 80 fa 4f
    jnbe short 036d7h                         ; 77 09
    test bh, bh                               ; 84 ff
    je short 036d7h                           ; 74 05
    cmp bh, 012h                              ; 80 ff 12
    jbe short 036f2h                          ; 76 1b
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 001h                               ; 80 cc 01
    mov word [bp+016h], ax                    ; 89 46 16
    mov si, strict word 00041h                ; be 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:si], 001h                    ; 26 c6 04 01
    or byte [bp+01ch], 001h                   ; 80 4e 1c 01
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    call 031d1h                               ; e8 d8 fa
    test ax, ax                               ; 85 c0
    jne short 03717h                          ; 75 1a
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 080h                               ; 80 cc 80
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 080h                    ; 26 c6 07 80
    jmp near 03ae1h                           ; e9 ca 03
    mov cl, bl                                ; 88 d9
    xor ch, ch                                ; 30 ed
    mov ax, cx                                ; 89 c8
    call 0300bh                               ; e8 eb f8
    test ax, ax                               ; 85 c0
    jne short 03730h                          ; 75 0c
    mov ax, cx                                ; 89 c8
    call 030eah                               ; e8 c1 f9
    test ax, ax                               ; 85 c0
    jne short 03730h                          ; 75 03
    jmp near 03397h                           ; e9 67 fc
    mov CL, strict byte 00ch                  ; b1 0c
    mov ax, word [bp+006h]                    ; 8b 46 06
    shr ax, CL                                ; d3 e8
    mov ch, al                                ; 88 c5
    mov CL, strict byte 004h                  ; b1 04
    mov dx, word [bp+006h]                    ; 8b 56 06
    sal dx, CL                                ; d3 e2
    mov si, word [bp+010h]                    ; 8b 76 10
    add si, dx                                ; 01 d6
    mov word [bp-00ch], si                    ; 89 76 f4
    cmp dx, si                                ; 39 f2
    jbe short 0374eh                          ; 76 02
    db  0feh, 0c5h
    ; inc ch                                    ; fe c5
    mov al, bh                                ; 88 f8
    xor ah, ah                                ; 30 e4
    sal ax, 1                                 ; d1 e0
    sal ax, 1                                 ; d1 e0
    dec ax                                    ; 48
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    add ax, word [bp-00eh]                    ; 03 46 f2
    cmp ax, word [bp-00ch]                    ; 3b 46 f4
    jnc short 0377fh                          ; 73 1a
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 009h                               ; 80 cc 09
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 009h                    ; 26 c6 07 09
    jmp near 03406h                           ; e9 87 fc
    mov AL, strict byte 006h                  ; b0 06
    mov dx, strict word 0000ah                ; ba 0a 00
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    mov dx, strict word 0000ch                ; ba 0c 00
    out DX, AL                                ; ee
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    mov dx, strict word 00004h                ; ba 04 00
    out DX, AL                                ; ee
    mov al, byte [bp-00bh]                    ; 8a 46 f5
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    mov dx, strict word 0000ch                ; ba 0c 00
    out DX, AL                                ; ee
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    mov dx, strict word 00005h                ; ba 05 00
    out DX, AL                                ; ee
    mov al, byte [bp-00dh]                    ; 8a 46 f3
    out DX, AL                                ; ee
    mov AL, strict byte 04ah                  ; b0 4a
    mov dx, strict word 0000bh                ; ba 0b 00
    out DX, AL                                ; ee
    mov al, ch                                ; 88 e8
    mov dx, 00081h                            ; ba 81 00
    out DX, AL                                ; ee
    mov AL, strict byte 002h                  ; b0 02
    mov dx, strict word 0000ah                ; ba 0a 00
    out DX, AL                                ; ee
    mov cl, bl                                ; 88 d9
    xor ch, ch                                ; 30 ed
    mov ax, cx                                ; 89 c8
    call 02f96h                               ; e8 d4 f7
    mov AL, strict byte 00fh                  ; b0 0f
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    mov al, byte [bp-006h]                    ; 8a 46 fa
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    sal dx, 1                                 ; d1 e2
    sal dx, 1                                 ; d1 e2
    mov al, bl                                ; 88 d8
    or dx, ax                                 ; 09 c2
    mov word [bp-008h], dx                    ; 89 56 f8
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    mov al, byte [bp-004h]                    ; 8a 46 fc
    out DX, AL                                ; ee
    mov AL, strict byte 04dh                  ; b0 4d
    out DX, AL                                ; ee
    mov al, byte [bp-008h]                    ; 8a 46 f8
    out DX, AL                                ; ee
    mov AL, strict byte 002h                  ; b0 02
    out DX, AL                                ; ee
    mov al, bh                                ; 88 f8
    out DX, AL                                ; ee
    xor al, bh                                ; 30 f8
    out DX, AL                                ; ee
    mov AL, strict byte 0f6h                  ; b0 f6
    out DX, AL                                ; ee
    call 02f1fh                               ; e8 24 f7
    test al, al                               ; 84 c0
    jne short 03807h                          ; 75 08
    mov ax, cx                                ; 89 c8
    call 02f57h                               ; e8 53 f7
    jmp near 036fdh                           ; e9 f6 fe
    mov dx, 003f4h                            ; ba f4 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 0c0h                  ; 24 c0
    cmp AL, strict byte 0c0h                  ; 3c c0
    je short 03825h                           ; 74 12
    mov ax, 00275h                            ; b8 75 02
    push ax                                   ; 50
    mov ax, 002a8h                            ; b8 a8 02
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 54 e1
    add sp, strict byte 00006h                ; 83 c4 06
    xor cx, cx                                ; 31 c9
    jmp short 0382eh                          ; eb 05
    cmp cx, strict byte 00007h                ; 83 f9 07
    jnl short 03844h                          ; 7d 16
    mov dx, 003f5h                            ; ba f5 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov si, cx                                ; 89 ce
    add si, strict byte 00042h                ; 83 c6 42
    mov dx, strict word 00040h                ; ba 40 00
    mov es, dx                                ; 8e c2
    mov byte [es:si], al                      ; 26 88 04
    inc cx                                    ; 41
    jmp short 03829h                          ; eb e5
    mov si, strict word 00042h                ; be 42 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov al, byte [es:si]                      ; 26 8a 04
    test AL, strict byte 0c0h                 ; a8 c0
    je short 03872h                           ; 74 1f
    mov si, strict word 00043h                ; be 43 00
    mov al, byte [es:si]                      ; 26 8a 04
    test AL, strict byte 002h                 ; a8 02
    je short 03860h                           ; 74 03
    jmp near 03683h                           ; e9 23 fe
    mov ax, 00275h                            ; b8 75 02
    push ax                                   ; 50
    mov ax, 002bch                            ; b8 bc 02
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 07 e1
    add sp, strict byte 00006h                ; 83 c4 06
    mov byte [bp+017h], 000h                  ; c6 46 17 00
    mov si, strict word 00041h                ; be 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:si], 000h                    ; 26 c6 04 00
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    xor dx, dx                                ; 31 d2
    call 02ed0h                               ; e8 45 f6
    jmp near 03553h                           ; e9 c5 fc
    mov bl, dl                                ; 88 d3
    cmp dl, 001h                              ; 80 fa 01
    jbe short 038bbh                          ; 76 26
    mov word [bp+016h], strict word 00000h    ; c7 46 16 00 00
    mov word [bp+010h], strict word 00000h    ; c7 46 10 00 00
    mov word [bp+014h], strict word 00000h    ; c7 46 14 00 00
    mov word [bp+012h], strict word 00000h    ; c7 46 12 00 00
    mov word [bp+006h], strict word 00000h    ; c7 46 06 00 00
    mov word [bp+008h], strict word 00000h    ; c7 46 08 00 00
    mov dl, bh                                ; 88 fa
    mov word [bp+012h], dx                    ; 89 56 12
    jmp near 039e0h                           ; e9 25 01
    mov ax, strict word 00010h                ; b8 10 00
    call 016aeh                               ; e8 ed dd
    mov dl, al                                ; 88 c2
    xor bh, bh                                ; 30 ff
    test AL, strict byte 0f0h                 ; a8 f0
    je short 038cbh                           ; 74 02
    mov BH, strict byte 001h                  ; b7 01
    test dl, 00fh                             ; f6 c2 0f
    je short 038d2h                           ; 74 02
    db  0feh, 0c7h
    ; inc bh                                    ; fe c7
    test bl, bl                               ; 84 db
    jne short 038dch                          ; 75 06
    mov CL, strict byte 004h                  ; b1 04
    shr dl, CL                                ; d2 ea
    jmp short 038dfh                          ; eb 03
    and dl, 00fh                              ; 80 e2 0f
    mov byte [bp+011h], 000h                  ; c6 46 11 00
    mov al, dl                                ; 88 d0
    xor ah, ah                                ; 30 e4
    mov word [bp+010h], ax                    ; 89 46 10
    xor al, dl                                ; 30 d0
    mov word [bp+016h], ax                    ; 89 46 16
    mov cx, word [bp+012h]                    ; 8b 4e 12
    mov cl, bh                                ; 88 f9
    mov word [bp+012h], cx                    ; 89 4e 12
    mov ax, cx                                ; 89 c8
    xor ah, ch                                ; 30 ec
    or ah, 001h                               ; 80 cc 01
    mov word [bp+012h], ax                    ; 89 46 12
    cmp dl, 003h                              ; 80 fa 03
    jc short 0391bh                           ; 72 15
    jbe short 03942h                          ; 76 3a
    cmp dl, 005h                              ; 80 fa 05
    jc short 03949h                           ; 72 3c
    jbe short 03950h                          ; 76 41
    cmp dl, 00fh                              ; 80 fa 0f
    je short 0395eh                           ; 74 4a
    cmp dl, 00eh                              ; 80 fa 0e
    je short 03957h                           ; 74 3e
    jmp short 03965h                          ; eb 4a
    cmp dl, 002h                              ; 80 fa 02
    je short 0393bh                           ; 74 1b
    cmp dl, 001h                              ; 80 fa 01
    je short 03934h                           ; 74 0f
    test dl, dl                               ; 84 d2
    jne short 03965h                          ; 75 3c
    mov word [bp+014h], strict word 00000h    ; c7 46 14 00 00
    mov byte [bp+013h], 000h                  ; c6 46 13 00
    jmp short 03977h                          ; eb 43
    mov word [bp+014h], 02709h                ; c7 46 14 09 27
    jmp short 03977h                          ; eb 3c
    mov word [bp+014h], 04f0fh                ; c7 46 14 0f 4f
    jmp short 03977h                          ; eb 35
    mov word [bp+014h], 04f09h                ; c7 46 14 09 4f
    jmp short 03977h                          ; eb 2e
    mov word [bp+014h], 04f12h                ; c7 46 14 12 4f
    jmp short 03977h                          ; eb 27
    mov word [bp+014h], 04f24h                ; c7 46 14 24 4f
    jmp short 03977h                          ; eb 20
    mov word [bp+014h], 0fe3fh                ; c7 46 14 3f fe
    jmp short 03977h                          ; eb 19
    mov word [bp+014h], 0feffh                ; c7 46 14 ff fe
    jmp short 03977h                          ; eb 12
    mov ax, 00275h                            ; b8 75 02
    push ax                                   ; 50
    mov ax, 002cdh                            ; b8 cd 02
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 02 e0
    add sp, strict byte 00006h                ; 83 c4 06
    mov word [bp+006h], 0f000h                ; c7 46 06 00 f0
    mov al, dl                                ; 88 d0
    xor ah, ah                                ; 30 e4
    call 03c1ch                               ; e8 99 02
    mov word [bp+008h], ax                    ; 89 46 08
    jmp near 03553h                           ; e9 ca fb
    mov bl, dl                                ; 88 d3
    cmp dl, 001h                              ; 80 fa 01
    jbe short 03995h                          ; 76 05
    mov word [bp+016h], cx                    ; 89 4e 16
    jmp short 039e0h                          ; eb 4b
    mov ax, strict word 00010h                ; b8 10 00
    call 016aeh                               ; e8 13 dd
    test bl, bl                               ; 84 db
    jne short 039a7h                          ; 75 08
    mov CL, strict byte 004h                  ; b1 04
    mov dl, al                                ; 88 c2
    shr dl, CL                                ; d2 ea
    jmp short 039ach                          ; eb 05
    mov dl, al                                ; 88 c2
    and dl, 00fh                              ; 80 e2 0f
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    test dl, dl                               ; 84 d2
    je short 039c6h                           ; 74 0d
    cmp dl, 001h                              ; 80 fa 01
    jbe short 039c3h                          ; 76 05
    or ah, 002h                               ; 80 cc 02
    jmp short 039c6h                          ; eb 03
    or ah, 001h                               ; 80 cc 01
    mov word [bp+016h], ax                    ; 89 46 16
    jmp near 032dbh                           ; e9 0f f9
    cmp dl, 001h                              ; 80 fa 01
    jbe short 039e6h                          ; 76 15
    mov word [bp+016h], si                    ; 89 76 16
    mov bx, strict word 00041h                ; bb 41 00
    mov dx, strict word 00040h                ; ba 40 00
    mov es, dx                                ; 8e c2
    mov byte [es:bx], 001h                    ; 26 c6 07 01
    mov word [bp+01ch], ax                    ; 89 46 1c
    jmp near 032dbh                           ; e9 f5 f8
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 006h                               ; 80 cc 06
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 006h                    ; 26 c6 07 06
    jmp near 03ae1h                           ; e9 e1 00
    mov bl, dl                                ; 88 d3
    mov cl, byte [bp+016h]                    ; 8a 4e 16
    cmp dl, 001h                              ; 80 fa 01
    jnbe short 039d1h                         ; 77 c7
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    call 031d1h                               ; e8 c0 f7
    test ax, ax                               ; 85 c0
    jne short 03a18h                          ; 75 03
    jmp near 036fdh                           ; e9 e5 fc
    test bl, bl                               ; 84 db
    je short 03a21h                           ; 74 05
    mov bx, 00091h                            ; bb 91 00
    jmp short 03a24h                          ; eb 03
    mov bx, 00090h                            ; bb 90 00
    mov word [bp-00ch], bx                    ; 89 5e f4
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov bl, byte [es:bx]                      ; 26 8a 1f
    and bl, 00fh                              ; 80 e3 0f
    cmp cl, 002h                              ; 80 f9 02
    jc short 03a46h                           ; 72 0f
    jbe short 03a53h                          ; 76 1a
    cmp cl, 004h                              ; 80 f9 04
    je short 03a4eh                           ; 74 10
    cmp cl, 003h                              ; 80 f9 03
    je short 03a58h                           ; 74 15
    jmp near 03265h                           ; e9 1f f8
    cmp cl, 001h                              ; 80 f9 01
    je short 03a4eh                           ; 74 03
    jmp near 03265h                           ; e9 17 f8
    or bl, 090h                               ; 80 cb 90
    jmp short 03a5bh                          ; eb 08
    or bl, 070h                               ; 80 cb 70
    jmp short 03a5bh                          ; eb 03
    or bl, 010h                               ; 80 cb 10
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov si, word [bp-00ch]                    ; 8b 76 f4
    mov byte [es:si], bl                      ; 26 88 1c
    xor al, al                                ; 30 c0
    mov byte [bp+017h], al                    ; 88 46 17
    mov bx, strict word 00041h                ; bb 41 00
    mov byte [es:bx], al                      ; 26 88 07
    jmp near 03553h                           ; e9 df fa
    mov bl, dl                                ; 88 d3
    mov cl, byte [bp+014h]                    ; 8a 4e 14
    mov bh, cl                                ; 88 cf
    and bh, 03fh                              ; 80 e7 3f
    mov byte [bp-00ah], cl                    ; 88 4e f6
    mov byte [bp-009h], dh                    ; 88 76 f7
    mov CL, strict byte 006h                  ; b1 06
    mov di, word [bp-00ah]                    ; 8b 7e f6
    sar di, CL                                ; d3 ff
    mov cx, di                                ; 89 f9
    mov byte [bp-009h], cl                    ; 88 4e f7
    mov byte [bp-00ah], dh                    ; 88 76 f6
    mov cl, byte [bp+015h]                    ; 8a 4e 15
    xor ch, ch                                ; 30 ed
    add cx, word [bp-00ah]                    ; 03 4e f6
    mov byte [bp-004h], cl                    ; 88 4e fc
    cmp dl, 001h                              ; 80 fa 01
    jbe short 03aa6h                          ; 76 03
    jmp near 039d1h                           ; e9 2b ff
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    call 031d1h                               ; e8 24 f7
    test ax, ax                               ; 85 c0
    jne short 03ab4h                          ; 75 03
    jmp near 036fdh                           ; e9 49 fc
    mov cl, bl                                ; 88 d9
    xor ch, ch                                ; 30 ed
    mov ax, cx                                ; 89 c8
    call 0300bh                               ; e8 4e f5
    test ax, ax                               ; 85 c0
    jne short 03ae8h                          ; 75 27
    mov ax, cx                                ; 89 c8
    call 030eah                               ; e8 24 f6
    test ax, ax                               ; 85 c0
    jne short 03ae8h                          ; 75 1e
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 00ch                               ; 80 cc 0c
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 00ch                    ; 26 c6 07 0c
    or byte [bp+01ch], 001h                   ; 80 4e 1c 01
    jmp near 032dbh                           ; e9 f3 f7
    mov ax, strict word 00010h                ; b8 10 00
    call 016aeh                               ; e8 c0 db
    test bl, bl                               ; 84 db
    jne short 03afah                          ; 75 08
    mov CL, strict byte 004h                  ; b1 04
    mov dl, al                                ; 88 c2
    shr dl, CL                                ; d2 ea
    jmp short 03affh                          ; eb 05
    mov dl, al                                ; 88 c2
    and dl, 00fh                              ; 80 e2 0f
    test bl, bl                               ; 84 db
    je short 03b08h                           ; 74 05
    mov si, 00091h                            ; be 91 00
    jmp short 03b0bh                          ; eb 03
    mov si, 00090h                            ; be 90 00
    mov word [bp-00ch], si                    ; 89 76 f4
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov bl, byte [es:si]                      ; 26 8a 1c
    and bl, 00fh                              ; 80 e3 0f
    cmp dl, 003h                              ; 80 fa 03
    jc short 03b39h                           ; 72 1b
    mov al, bl                                ; 88 d8
    or AL, strict byte 090h                   ; 0c 90
    cmp dl, 003h                              ; 80 fa 03
    jbe short 03b75h                          ; 76 4e
    mov ah, bl                                ; 88 dc
    or ah, 010h                               ; 80 cc 10
    cmp dl, 005h                              ; 80 fa 05
    je short 03b73h                           ; 74 42
    cmp dl, 004h                              ; 80 fa 04
    je short 03b82h                           ; 74 4c
    jmp near 03bbah                           ; e9 81 00
    cmp dl, 002h                              ; 80 fa 02
    je short 03b53h                           ; 74 15
    cmp dl, 001h                              ; 80 fa 01
    jne short 03b86h                          ; 75 43
    cmp byte [bp-004h], 027h                  ; 80 7e fc 27
    jne short 03b86h                          ; 75 3d
    cmp bh, 009h                              ; 80 ff 09
    jne short 03b9ch                          ; 75 4e
    or bl, 090h                               ; 80 cb 90
    jmp short 03b9ch                          ; eb 49
    cmp byte [bp-004h], 027h                  ; 80 7e fc 27
    jne short 03b63h                          ; 75 0a
    cmp bh, 009h                              ; 80 ff 09
    jne short 03b63h                          ; 75 05
    or bl, 070h                               ; 80 cb 70
    jmp short 03b9ch                          ; eb 39
    cmp byte [bp-004h], 04fh                  ; 80 7e fc 4f
    jne short 03bbah                          ; 75 51
    cmp bh, 00fh                              ; 80 ff 0f
    jne short 03bbah                          ; 75 4c
    or bl, 010h                               ; 80 cb 10
    jmp short 03bbah                          ; eb 47
    jmp short 03b9eh                          ; eb 29
    cmp byte [bp-004h], 04fh                  ; 80 7e fc 4f
    jne short 03bbah                          ; 75 3f
    cmp bh, 009h                              ; 80 ff 09
    je short 03b84h                           ; 74 04
    jmp short 03bbah                          ; eb 38
    jmp short 03b88h                          ; eb 04
    mov bl, al                                ; 88 c3
    jmp short 03bbah                          ; eb 32
    cmp byte [bp-004h], 04fh                  ; 80 7e fc 4f
    jne short 03bbah                          ; 75 2c
    cmp bh, 009h                              ; 80 ff 09
    jne short 03b95h                          ; 75 02
    jmp short 03b84h                          ; eb ef
    cmp bh, 012h                              ; 80 ff 12
    jne short 03bbah                          ; 75 20
    mov bl, ah                                ; 88 e3
    jmp short 03bbah                          ; eb 1c
    cmp byte [bp-004h], 04fh                  ; 80 7e fc 4f
    jne short 03bbah                          ; 75 16
    cmp bh, 009h                              ; 80 ff 09
    jne short 03babh                          ; 75 02
    jmp short 03b84h                          ; eb d9
    cmp bh, 012h                              ; 80 ff 12
    jne short 03bb2h                          ; 75 02
    jmp short 03b9ah                          ; eb e8
    cmp bh, 024h                              ; 80 ff 24
    jne short 03bbah                          ; 75 03
    or bl, 0d0h                               ; 80 cb d0
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 004h                  ; b1 04
    sar ax, CL                                ; d3 f8
    test AL, strict byte 001h                 ; a8 01
    jne short 03bc9h                          ; 75 03
    jmp near 03acah                           ; e9 01 ff
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov si, word [bp-00ch]                    ; 8b 76 f4
    mov byte [es:si], bl                      ; 26 88 1c
    mov word [bp+006h], 0f000h                ; c7 46 06 00 f0
    mov al, dl                                ; 88 d0
    xor ah, ah                                ; 30 e4
    call 03c1ch                               ; e8 3c 00
    mov word [bp+008h], ax                    ; 89 46 08
    mov byte [bp+017h], 000h                  ; c6 46 17 00
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 000h                    ; 26 c6 07 00
    jmp near 03553h                           ; e9 5d f9
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 32 dd
    mov al, byte [bp+017h]                    ; 8a 46 17
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 00275h                            ; b8 75 02
    push ax                                   ; 50
    mov ax, 002e2h                            ; b8 e2 02
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 60 dd
    add sp, strict byte 00008h                ; 83 c4 08
    jmp near 03265h                           ; e9 49 f6
get_floppy_dpt_:                             ; 0xf3c1c LB 0x32
    push bx                                   ; 53
    push dx                                   ; 52
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov dl, al                                ; 88 c2
    xor ax, ax                                ; 31 c0
    jmp short 03c2dh                          ; eb 06
    inc ax                                    ; 40
    cmp ax, strict word 00007h                ; 3d 07 00
    jnc short 03c47h                          ; 73 1a
    mov bx, ax                                ; 89 c3
    sal bx, 1                                 ; d1 e3
    cmp dl, byte [word bx+0005bh]             ; 3a 97 5b 00
    jne short 03c27h                          ; 75 f0
    mov al, byte [word bx+0005ch]             ; 8a 87 5c 00
    xor ah, ah                                ; 30 e4
    mov bx, strict word 0000dh                ; bb 0d 00
    imul bx                                   ; f7 eb
    add ax, strict word 00000h                ; 05 00 00
    jmp short 03c4ah                          ; eb 03
    mov ax, strict word 00041h                ; b8 41 00
    pop bp                                    ; 5d
    pop dx                                    ; 5a
    pop bx                                    ; 5b
    retn                                      ; c3
dummy_soft_reset_:                           ; 0xf3c4e LB 0x7
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    xor ax, ax                                ; 31 c0
    pop bp                                    ; 5d
    retn                                      ; c3
_cdemu_init:                                 ; 0xf3c55 LB 0x18
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 0d da
    xor bx, bx                                ; 31 db
    mov dx, 00366h                            ; ba 66 03
    call 01660h                               ; e8 f7 d9
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
_cdemu_isactive:                             ; 0xf3c6d LB 0x16
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 f5 d9
    mov dx, 00366h                            ; ba 66 03
    call 01652h                               ; e8 d3 d9
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
_cdemu_emulated_drive:                       ; 0xf3c83 LB 0x16
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 df d9
    mov dx, 00368h                            ; ba 68 03
    call 01652h                               ; e8 bd d9
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
_int13_eltorito:                             ; 0xf3c99 LB 0x191
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 c7 d9
    mov si, 00366h                            ; be 66 03
    mov di, ax                                ; 89 c7
    mov al, byte [bp+017h]                    ; 8a 46 17
    xor ah, ah                                ; 30 e4
    cmp ax, strict word 0004bh                ; 3d 4b 00
    jc short 03cc0h                           ; 72 0a
    jbe short 03cebh                          ; 76 33
    cmp ax, strict word 0004dh                ; 3d 4d 00
    jbe short 03cc5h                          ; 76 08
    jmp near 03df0h                           ; e9 30 01
    cmp ax, strict word 0004ah                ; 3d 4a 00
    jne short 03ce8h                          ; 75 23
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 63 dc
    push word [bp+016h]                       ; ff 76 16
    mov ax, 002fch                            ; b8 fc 02
    push ax                                   ; 50
    mov ax, 0030bh                            ; b8 0b 03
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 94 dc
    add sp, strict byte 00008h                ; 83 c4 08
    jmp near 03e0bh                           ; e9 23 01
    jmp near 03df0h                           ; e9 05 01
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    mov ax, word [bp+004h]                    ; 8b 46 04
    mov bx, strict word 00013h                ; bb 13 00
    call 01660h                               ; e8 69 d9
    mov es, di                                ; 8e c7
    mov bl, byte [es:si+001h]                 ; 26 8a 5c 01
    xor bh, bh                                ; 30 ff
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    inc dx                                    ; 42
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01660h                               ; e8 57 d9
    mov es, di                                ; 8e c7
    mov bl, byte [es:si+002h]                 ; 26 8a 5c 02
    xor bh, bh                                ; 30 ff
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    inc dx                                    ; 42
    inc dx                                    ; 42
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01660h                               ; e8 44 d9
    mov es, di                                ; 8e c7
    mov bl, byte [es:si+003h]                 ; 26 8a 5c 03
    xor bh, bh                                ; 30 ff
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00003h                ; 83 c2 03
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01660h                               ; e8 30 d9
    mov es, di                                ; 8e c7
    mov bx, word [es:si+008h]                 ; 26 8b 5c 08
    mov cx, word [es:si+00ah]                 ; 26 8b 4c 0a
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00004h                ; 83 c2 04
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 0169ch                               ; e8 56 d9
    mov es, di                                ; 8e c7
    mov bx, word [es:si+004h]                 ; 26 8b 5c 04
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00008h                ; 83 c2 08
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 0167ch                               ; e8 24 d9
    mov es, di                                ; 8e c7
    mov bx, word [es:si+006h]                 ; 26 8b 5c 06
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 0000ah                ; 83 c2 0a
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 0167ch                               ; e8 12 d9
    mov es, di                                ; 8e c7
    mov bx, word [es:si+00ch]                 ; 26 8b 5c 0c
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 0000ch                ; 83 c2 0c
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 0167ch                               ; e8 00 d9
    mov es, di                                ; 8e c7
    mov bx, word [es:si+00eh]                 ; 26 8b 5c 0e
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 0000eh                ; 83 c2 0e
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 0167ch                               ; e8 ee d8
    mov es, di                                ; 8e c7
    mov bl, byte [es:si+012h]                 ; 26 8a 5c 12
    xor bh, bh                                ; 30 ff
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00010h                ; 83 c2 10
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01660h                               ; e8 be d8
    mov es, di                                ; 8e c7
    mov bl, byte [es:si+014h]                 ; 26 8a 5c 14
    xor bh, bh                                ; 30 ff
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00011h                ; 83 c2 11
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01660h                               ; e8 aa d8
    mov es, di                                ; 8e c7
    mov bl, byte [es:si+010h]                 ; 26 8a 5c 10
    xor bh, bh                                ; 30 ff
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00012h                ; 83 c2 12
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01660h                               ; e8 96 d8
    test byte [bp+016h], 0ffh                 ; f6 46 16 ff
    jne short 03dd6h                          ; 75 06
    mov es, di                                ; 8e c7
    mov byte [es:si], 000h                    ; 26 c6 04 00
    mov byte [bp+017h], 000h                  ; c6 46 17 00
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 7b d8
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 38 db
    mov al, byte [bp+017h]                    ; 8a 46 17
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 002fch                            ; b8 fc 02
    push ax                                   ; 50
    mov ax, 00333h                            ; b8 33 03
    jmp near 03cdah                           ; e9 cf fe
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 001h                               ; 80 cc 01
    mov word [bp+016h], ax                    ; 89 46 16
    mov bl, byte [bp+017h]                    ; 8a 5e 17
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 3c d8
    or byte [bp+01ch], 001h                   ; 80 4e 1c 01
    jmp short 03de9h                          ; eb bf
device_is_cdrom_:                            ; 0xf3e2a LB 0x3f
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    mov bl, al                                ; 88 c3
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 33 d8
    mov cx, ax                                ; 89 c1
    cmp bl, 010h                              ; 80 fb 10
    jc short 03e46h                           ; 72 04
    xor ax, ax                                ; 31 c0
    jmp short 03e61h                          ; eb 1b
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    mov bx, strict word 0001ch                ; bb 1c 00
    imul bx                                   ; f7 eb
    mov es, cx                                ; 8e c1
    mov bx, ax                                ; 89 c3
    add bx, 00122h                            ; 81 c3 22 01
    cmp byte [es:bx+023h], 005h               ; 26 80 7f 23 05
    jne short 03e42h                          ; 75 e4
    mov ax, strict word 00001h                ; b8 01 00
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
cdrom_boot_:                                 ; 0xf3e69 LB 0x43e
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push si                                   ; 56
    push di                                   ; 57
    sub sp, 0081ch                            ; 81 ec 1c 08
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 f0 d7
    mov word [bp-018h], ax                    ; 89 46 e8
    mov si, 00366h                            ; be 66 03
    mov word [bp-016h], ax                    ; 89 46 ea
    mov word [bp-012h], 00122h                ; c7 46 ee 22 01
    mov word [bp-010h], ax                    ; 89 46 f0
    mov byte [bp-00eh], 000h                  ; c6 46 f2 00
    jmp short 03e9eh                          ; eb 09
    inc byte [bp-00eh]                        ; fe 46 f2
    cmp byte [bp-00eh], 010h                  ; 80 7e f2 10
    jnc short 03eaah                          ; 73 0c
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    xor ah, ah                                ; 30 e4
    call 03e2ah                               ; e8 84 ff
    test ax, ax                               ; 85 c0
    je short 03e95h                           ; 74 eb
    cmp byte [bp-00eh], 010h                  ; 80 7e f2 10
    jc short 03eb6h                           ; 72 06
    mov ax, strict word 00002h                ; b8 02 00
    jmp near 04244h                           ; e9 8e 03
    mov cx, strict word 0000ch                ; b9 0c 00
    xor bx, bx                                ; 31 db
    mov dx, ss                                ; 8c d2
    lea ax, [bp-026h]                         ; 8d 46 da
    call 0a1c0h                               ; e8 fd 62
    mov word [bp-026h], strict word 00028h    ; c7 46 da 28 00
    mov ax, strict word 00011h                ; b8 11 00
    xor dx, dx                                ; 31 d2
    xchg ah, al                               ; 86 c4
    xchg dh, dl                               ; 86 d6
    xchg dx, ax                               ; 92
    mov word [bp-024h], ax                    ; 89 46 dc
    mov word [bp-022h], dx                    ; 89 56 de
    mov ax, strict word 00001h                ; b8 01 00
    xchg ah, al                               ; 86 c4
    mov word [bp-01fh], ax                    ; 89 46 e1
    les bx, [bp-012h]                         ; c4 5e ee
    mov word [es:bx+00eh], strict word 00001h ; 26 c7 47 0e 01 00
    mov word [es:bx+010h], 00800h             ; 26 c7 47 10 00 08
    mov byte [bp-00ch], 000h                  ; c6 46 f4 00
    jmp short 03efeh                          ; eb 09
    inc byte [bp-00ch]                        ; fe 46 f4
    cmp byte [bp-00ch], 004h                  ; 80 7e f4 04
    jnbe short 03f40h                         ; 77 42
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    les bx, [bp-012h]                         ; c4 5e ee
    add bx, ax                                ; 01 c3
    mov al, byte [es:bx+022h]                 ; 26 8a 47 22
    xor ah, ah                                ; 30 e4
    mov di, ax                                ; 89 c7
    sal di, 1                                 ; d1 e7
    lea dx, [bp-00826h]                       ; 8d 96 da f7
    push SS                                   ; 16
    push dx                                   ; 52
    mov ax, strict word 00001h                ; b8 01 00
    push ax                                   ; 50
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    mov ax, 00800h                            ; b8 00 08
    push ax                                   ; 50
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    xor ah, ah                                ; 30 e4
    mov cx, ss                                ; 8c d1
    lea bx, [bp-026h]                         ; 8d 5e da
    mov dx, strict word 0000ch                ; ba 0c 00
    call word [word di+0006ah]                ; ff 95 6a 00
    test ax, ax                               ; 85 c0
    jne short 03ef5h                          ; 75 b5
    test ax, ax                               ; 85 c0
    je short 03f4ah                           ; 74 06
    mov ax, strict word 00003h                ; b8 03 00
    jmp near 04244h                           ; e9 fa 02
    cmp byte [bp-00826h], 000h                ; 80 be da f7 00
    je short 03f57h                           ; 74 06
    mov ax, strict word 00004h                ; b8 04 00
    jmp near 04244h                           ; e9 ed 02
    xor di, di                                ; 31 ff
    jmp short 03f61h                          ; eb 06
    inc di                                    ; 47
    cmp di, strict byte 00005h                ; 83 ff 05
    jnc short 03f71h                          ; 73 10
    mov al, byte [bp+di-00825h]               ; 8a 83 db f7
    cmp al, byte [di+00da8h]                  ; 3a 85 a8 0d
    je short 03f5bh                           ; 74 f0
    mov ax, strict word 00005h                ; b8 05 00
    jmp near 04244h                           ; e9 d3 02
    xor di, di                                ; 31 ff
    jmp short 03f7bh                          ; eb 06
    inc di                                    ; 47
    cmp di, strict byte 00017h                ; 83 ff 17
    jnc short 03f8bh                          ; 73 10
    mov al, byte [bp+di-0081fh]               ; 8a 83 e1 f7
    cmp al, byte [di+00daeh]                  ; 3a 85 ae 0d
    je short 03f75h                           ; 74 f0
    mov ax, strict word 00006h                ; b8 06 00
    jmp near 04244h                           ; e9 b9 02
    mov ax, word [bp-007dfh]                  ; 8b 86 21 f8
    mov dx, word [bp-007ddh]                  ; 8b 96 23 f8
    mov word [bp-026h], strict word 00028h    ; c7 46 da 28 00
    xchg ah, al                               ; 86 c4
    xchg dh, dl                               ; 86 d6
    xchg dx, ax                               ; 92
    mov word [bp-024h], ax                    ; 89 46 dc
    mov word [bp-022h], dx                    ; 89 56 de
    mov ax, strict word 00001h                ; b8 01 00
    xchg ah, al                               ; 86 c4
    mov word [bp-01fh], ax                    ; 89 46 e1
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    les bx, [bp-012h]                         ; c4 5e ee
    add bx, ax                                ; 01 c3
    mov al, byte [es:bx+022h]                 ; 26 8a 47 22
    xor ah, ah                                ; 30 e4
    mov di, ax                                ; 89 c7
    sal di, 1                                 ; d1 e7
    lea dx, [bp-00826h]                       ; 8d 96 da f7
    push SS                                   ; 16
    push dx                                   ; 52
    mov ax, strict word 00001h                ; b8 01 00
    push ax                                   ; 50
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    mov ax, 00800h                            ; b8 00 08
    push ax                                   ; 50
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    xor ah, ah                                ; 30 e4
    mov cx, ss                                ; 8c d1
    lea bx, [bp-026h]                         ; 8d 5e da
    mov dx, strict word 0000ch                ; ba 0c 00
    call word [word di+0006ah]                ; ff 95 6a 00
    test ax, ax                               ; 85 c0
    je short 03ff3h                           ; 74 06
    mov ax, strict word 00007h                ; b8 07 00
    jmp near 04244h                           ; e9 51 02
    cmp byte [bp-00826h], 001h                ; 80 be da f7 01
    je short 04000h                           ; 74 06
    mov ax, strict word 00008h                ; b8 08 00
    jmp near 04244h                           ; e9 44 02
    cmp byte [bp-00825h], 000h                ; 80 be db f7 00
    je short 0400dh                           ; 74 06
    mov ax, strict word 00009h                ; b8 09 00
    jmp near 04244h                           ; e9 37 02
    cmp byte [bp-00808h], 055h                ; 80 be f8 f7 55
    je short 0401ah                           ; 74 06
    mov ax, strict word 0000ah                ; b8 0a 00
    jmp near 04244h                           ; e9 2a 02
    cmp byte [bp-00807h], 0aah                ; 80 be f9 f7 aa
    jne short 04014h                          ; 75 f3
    cmp byte [bp-00806h], 088h                ; 80 be fa f7 88
    je short 0402eh                           ; 74 06
    mov ax, strict word 0000bh                ; b8 0b 00
    jmp near 04244h                           ; e9 16 02
    mov al, byte [bp-00805h]                  ; 8a 86 fb f7
    mov es, [bp-016h]                         ; 8e 46 ea
    mov byte [es:si+001h], al                 ; 26 88 44 01
    cmp byte [bp-00805h], 000h                ; 80 be fb f7 00
    jne short 04047h                          ; 75 07
    mov byte [es:si+002h], 0e0h               ; 26 c6 44 02 e0
    jmp short 0405ah                          ; eb 13
    cmp byte [bp-00805h], 004h                ; 80 be fb f7 04
    jnc short 04055h                          ; 73 07
    mov byte [es:si+002h], 000h               ; 26 c6 44 02 00
    jmp short 0405ah                          ; eb 05
    mov byte [es:si+002h], 080h               ; 26 c6 44 02 80
    mov bl, byte [bp-00eh]                    ; 8a 5e f2
    xor bh, bh                                ; 30 ff
    mov ax, bx                                ; 89 d8
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    mov es, [bp-016h]                         ; 8e 46 ea
    mov byte [es:si+003h], al                 ; 26 88 44 03
    mov ax, bx                                ; 89 d8
    cwd                                       ; 99
    mov bx, strict word 00002h                ; bb 02 00
    idiv bx                                   ; f7 fb
    mov word [es:si+004h], dx                 ; 26 89 54 04
    mov ax, word [bp-00804h]                  ; 8b 86 fc f7
    mov word [bp-014h], ax                    ; 89 46 ec
    test ax, ax                               ; 85 c0
    jne short 04089h                          ; 75 05
    mov word [bp-014h], 007c0h                ; c7 46 ec c0 07
    mov ax, word [bp-014h]                    ; 8b 46 ec
    mov es, [bp-016h]                         ; 8e 46 ea
    mov word [es:si+00ch], ax                 ; 26 89 44 0c
    mov word [es:si+006h], strict word 00000h ; 26 c7 44 06 00 00
    mov bx, word [bp-00800h]                  ; 8b 9e 00 f8
    mov word [es:si+00eh], bx                 ; 26 89 5c 0e
    test bx, bx                               ; 85 db
    je short 040abh                           ; 74 06
    cmp bx, 00400h                            ; 81 fb 00 04
    jbe short 040b1h                          ; 76 06
    mov ax, strict word 0000ch                ; b8 0c 00
    jmp near 04244h                           ; e9 93 01
    mov ax, word [bp-007feh]                  ; 8b 86 02 f8
    mov dx, word [bp-007fch]                  ; 8b 96 04 f8
    mov word [es:si+008h], ax                 ; 26 89 44 08
    mov word [es:si+00ah], dx                 ; 26 89 54 0a
    mov word [bp-026h], strict word 00028h    ; c7 46 da 28 00
    xchg ah, al                               ; 86 c4
    xchg dh, dl                               ; 86 d6
    xchg dx, ax                               ; 92
    mov word [bp-024h], ax                    ; 89 46 dc
    mov word [bp-022h], dx                    ; 89 56 de
    lea dx, [bx-001h]                         ; 8d 57 ff
    shr dx, 1                                 ; d1 ea
    shr dx, 1                                 ; d1 ea
    inc dx                                    ; 42
    mov ax, dx                                ; 89 d0
    xchg ah, al                               ; 86 c4
    mov word [bp-01fh], ax                    ; 89 46 e1
    les di, [bp-012h]                         ; c4 7e ee
    mov word [es:di+00eh], dx                 ; 26 89 55 0e
    mov word [es:di+010h], 00200h             ; 26 c7 45 10 00 02
    mov CL, strict byte 009h                  ; b1 09
    mov ax, bx                                ; 89 d8
    sal ax, CL                                ; d3 e0
    mov dx, 00800h                            ; ba 00 08
    sub dx, ax                                ; 29 c2
    mov ax, dx                                ; 89 d0
    and ah, 007h                              ; 80 e4 07
    mov word [es:di+020h], ax                 ; 26 89 45 20
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    add di, ax                                ; 01 c7
    mov al, byte [es:di+022h]                 ; 26 8a 45 22
    xor ah, ah                                ; 30 e4
    sal ax, 1                                 ; d1 e0
    mov word [bp-01ah], ax                    ; 89 46 e6
    push word [bp-014h]                       ; ff 76 ec
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    mov ax, strict word 00001h                ; b8 01 00
    push ax                                   ; 50
    mov ax, bx                                ; 89 d8
    xor di, di                                ; 31 ff
    mov cx, strict word 00009h                ; b9 09 00
    sal ax, 1                                 ; d1 e0
    rcl di, 1                                 ; d1 d7
    loop 04129h                               ; e2 fa
    push di                                   ; 57
    push ax                                   ; 50
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    xor ah, ah                                ; 30 e4
    mov cx, ss                                ; 8c d1
    lea bx, [bp-026h]                         ; 8d 5e da
    mov dx, strict word 0000ch                ; ba 0c 00
    mov di, word [bp-01ah]                    ; 8b 7e e6
    call word [word di+0006ah]                ; ff 95 6a 00
    les bx, [bp-012h]                         ; c4 5e ee
    mov word [es:bx+020h], strict word 00000h ; 26 c7 47 20 00 00
    test ax, ax                               ; 85 c0
    je short 0415bh                           ; 74 06
    mov ax, strict word 0000dh                ; b8 0d 00
    jmp near 04244h                           ; e9 e9 00
    mov es, [bp-016h]                         ; 8e 46 ea
    mov al, byte [es:si+001h]                 ; 26 8a 44 01
    cmp AL, strict byte 002h                  ; 3c 02
    jc short 04173h                           ; 72 0d
    jbe short 0418eh                          ; 76 26
    cmp AL, strict byte 004h                  ; 3c 04
    je short 0419eh                           ; 74 32
    cmp AL, strict byte 003h                  ; 3c 03
    je short 04196h                           ; 74 26
    jmp near 041ech                           ; e9 79 00
    cmp AL, strict byte 001h                  ; 3c 01
    jne short 041ech                          ; 75 75
    mov es, [bp-016h]                         ; 8e 46 ea
    mov word [es:si+014h], strict word 0000fh ; 26 c7 44 14 0f 00
    mov word [es:si+012h], strict word 00050h ; 26 c7 44 12 50 00
    mov word [es:si+010h], strict word 00002h ; 26 c7 44 10 02 00
    jmp short 041ech                          ; eb 5e
    mov word [es:si+014h], strict word 00012h ; 26 c7 44 14 12 00
    jmp short 04180h                          ; eb ea
    mov word [es:si+014h], strict word 00024h ; 26 c7 44 14 24 00
    jmp short 04180h                          ; eb e2
    mov dx, 001c4h                            ; ba c4 01
    mov ax, word [bp-014h]                    ; 8b 46 ec
    call 01652h                               ; e8 ab d4
    and AL, strict byte 03fh                  ; 24 3f
    xor ah, ah                                ; 30 e4
    mov es, [bp-016h]                         ; 8e 46 ea
    mov word [es:si+014h], ax                 ; 26 89 44 14
    mov dx, 001c4h                            ; ba c4 01
    mov ax, word [bp-014h]                    ; 8b 46 ec
    call 01652h                               ; e8 97 d4
    and ax, 000c0h                            ; 25 c0 00
    mov bx, ax                                ; 89 c3
    sal bx, 1                                 ; d1 e3
    sal bx, 1                                 ; d1 e3
    mov dx, 001c5h                            ; ba c5 01
    mov ax, word [bp-014h]                    ; 8b 46 ec
    call 01652h                               ; e8 85 d4
    xor ah, ah                                ; 30 e4
    add ax, bx                                ; 01 d8
    inc ax                                    ; 40
    mov es, [bp-016h]                         ; 8e 46 ea
    mov word [es:si+012h], ax                 ; 26 89 44 12
    mov dx, 001c3h                            ; ba c3 01
    mov ax, word [bp-014h]                    ; 8b 46 ec
    call 01652h                               ; e8 70 d4
    xor ah, ah                                ; 30 e4
    inc ax                                    ; 40
    mov es, [bp-016h]                         ; 8e 46 ea
    mov word [es:si+010h], ax                 ; 26 89 44 10
    mov es, [bp-016h]                         ; 8e 46 ea
    cmp byte [es:si+001h], 000h               ; 26 80 7c 01 00
    je short 0422dh                           ; 74 37
    cmp byte [es:si+002h], 000h               ; 26 80 7c 02 00
    jne short 04215h                          ; 75 18
    mov dx, strict word 00010h                ; ba 10 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 4c d4
    mov bl, al                                ; 88 c3
    or bl, 041h                               ; 80 cb 41
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00010h                ; ba 10 00
    mov ax, strict word 00040h                ; b8 40 00
    jmp short 0422ah                          ; eb 15
    mov dx, 00304h                            ; ba 04 03
    mov ax, word [bp-018h]                    ; 8b 46 e8
    call 01652h                               ; e8 34 d4
    mov bl, al                                ; 88 c3
    db  0feh, 0c3h
    ; inc bl                                    ; fe c3
    xor bh, bh                                ; 30 ff
    mov dx, 00304h                            ; ba 04 03
    mov ax, word [bp-018h]                    ; 8b 46 e8
    call 01660h                               ; e8 33 d4
    mov es, [bp-016h]                         ; 8e 46 ea
    cmp byte [es:si+001h], 000h               ; 26 80 7c 01 00
    je short 0423bh                           ; 74 04
    mov byte [es:si], 001h                    ; 26 c6 04 01
    mov es, [bp-016h]                         ; 8e 46 ea
    mov ah, byte [es:si+002h]                 ; 26 8a 64 02
    xor al, al                                ; 30 c0
    lea sp, [bp-00ah]                         ; 8d 66 f6
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
    db  050h, 04eh, 049h, 048h, 047h, 046h, 045h, 044h, 043h, 042h, 041h, 018h, 016h, 015h, 014h, 011h
    db  010h, 00dh, 00ch, 00bh, 00ah, 009h, 008h, 005h, 004h, 003h, 002h, 001h, 000h, 05eh, 046h, 04eh
    db  043h, 083h, 043h, 0a8h, 043h, 078h, 043h, 0a8h, 043h, 078h, 043h, 0a7h, 045h, 08dh, 045h, 05eh
    db  046h, 05eh, 046h, 08dh, 045h, 08dh, 045h, 08dh, 045h, 08dh, 045h, 08dh, 045h, 055h, 046h, 08dh
    db  045h, 05eh, 046h, 05eh, 046h, 05eh, 046h, 05eh, 046h, 05eh, 046h, 05eh, 046h, 05eh, 046h, 05eh
    db  046h, 05eh, 046h, 05eh, 046h, 05eh, 046h, 05eh, 046h
_int13_cdemu:                                ; 0xf42a7 LB 0x453
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 0002ch                ; 83 ec 2c
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 b6 d3
    mov di, 00366h                            ; bf 66 03
    mov cx, ax                                ; 89 c1
    mov si, di                                ; 89 fe
    mov word [bp-012h], ax                    ; 89 46 ee
    mov word [bp-018h], 00122h                ; c7 46 e8 22 01
    mov word [bp-016h], ax                    ; 89 46 ea
    mov es, ax                                ; 8e c0
    mov al, byte [es:di+003h]                 ; 26 8a 45 03
    sal al, 1                                 ; d0 e0
    mov byte [bp-006h], al                    ; 88 46 fa
    mov al, byte [es:di+004h]                 ; 26 8a 45 04
    add byte [bp-006h], al                    ; 00 46 fa
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 79 d3
    mov es, cx                                ; 8e c1
    cmp byte [es:di], 000h                    ; 26 80 3d 00
    je short 042feh                           ; 74 0f
    mov al, byte [es:di+002h]                 ; 26 8a 45 02
    xor ah, ah                                ; 30 e4
    mov dx, word [bp+012h]                    ; 8b 56 12
    xor dh, dh                                ; 30 f6
    cmp ax, dx                                ; 39 d0
    je short 04328h                           ; 74 2a
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 2a d6
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov al, byte [bp+017h]                    ; 8a 46 17
    push ax                                   ; 50
    mov ax, 0034ch                            ; b8 4c 03
    push ax                                   ; 50
    mov ax, 00358h                            ; b8 58 03
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 54 d6
    add sp, strict byte 0000ah                ; 83 c4 0a
    jmp near 04681h                           ; e9 59 03
    mov al, byte [bp+017h]                    ; 8a 46 17
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    cmp ax, strict word 00050h                ; 3d 50 00
    jnbe short 043a5h                         ; 77 71
    push CS                                   ; 0e
    pop ES                                    ; 07
    mov cx, strict word 0001eh                ; b9 1e 00
    mov di, 0424eh                            ; bf 4e 42
    repne scasb                               ; f2 ae
    sal cx, 1                                 ; d1 e1
    mov di, cx                                ; 89 cf
    mov ax, word [cs:di+0426bh]               ; 2e 8b 85 6b 42
    mov bx, word [bp+016h]                    ; 8b 5e 16
    xor bh, bh                                ; 30 ff
    jmp ax                                    ; ff e0
    mov al, byte [bp-006h]                    ; 8a 46 fa
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    les bx, [bp-018h]                         ; c4 5e e8
    add bx, ax                                ; 01 c3
    mov bl, byte [es:bx+022h]                 ; 26 8a 5f 22
    xor bh, bh                                ; 30 ff
    sal bx, 1                                 ; d1 e3
    cmp word [word bx+0006ah], strict byte 00000h ; 83 bf 6a 00 00
    je short 04375h                           ; 74 09
    mov al, byte [bp-006h]                    ; 8a 46 fa
    xor ah, ah                                ; 30 e4
    call word [word bx+00076h]                ; ff 97 76 00
    jmp near 0458dh                           ; e9 15 02
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 003h                               ; 80 cc 03
    jmp near 04689h                           ; e9 06 03
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 c6 d2
    mov cl, al                                ; 88 c1
    mov bh, al                                ; 88 c7
    mov word [bp+016h], bx                    ; 89 5e 16
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 c2 d2
    test cl, cl                               ; 84 c9
    je short 04407h                           ; 74 65
    jmp near 0469ah                           ; e9 f5 02
    jmp near 0465eh                           ; e9 b6 02
    mov es, [bp-012h]                         ; 8e 46 ee
    mov di, word [es:si+014h]                 ; 26 8b 7c 14
    mov dx, word [es:si+012h]                 ; 26 8b 54 12
    mov bx, word [es:si+010h]                 ; 26 8b 5c 10
    mov ax, word [es:si+008h]                 ; 26 8b 44 08
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov ax, word [es:si+00ah]                 ; 26 8b 44 0a
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov ax, word [bp+014h]                    ; 8b 46 14
    and ax, strict word 0003fh                ; 25 3f 00
    mov word [bp-00ah], ax                    ; 89 46 f6
    mov cx, word [bp+014h]                    ; 8b 4e 14
    and cx, 000c0h                            ; 81 e1 c0 00
    sal cx, 1                                 ; d1 e1
    sal cx, 1                                 ; d1 e1
    mov al, byte [bp+015h]                    ; 8a 46 15
    or ax, cx                                 ; 09 c8
    mov cl, byte [bp+013h]                    ; 8a 4e 13
    mov byte [bp-008h], cl                    ; 88 4e f8
    mov byte [bp-007h], 000h                  ; c6 46 f9 00
    mov si, word [bp-008h]                    ; 8b 76 f8
    mov cx, word [bp+016h]                    ; 8b 4e 16
    xor ch, ch                                ; 30 ed
    mov word [bp-014h], cx                    ; 89 4e ec
    test cx, cx                               ; 85 c9
    je short 04414h                           ; 74 1d
    cmp di, word [bp-00ah]                    ; 3b 7e f6
    jc short 04404h                           ; 72 08
    cmp ax, dx                                ; 39 d0
    jnc short 04404h                          ; 73 04
    cmp bx, si                                ; 39 f3
    jnbe short 0440ah                         ; 77 06
    jmp near 04681h                           ; e9 7a 02
    jmp near 04591h                           ; e9 87 01
    mov dl, byte [bp+017h]                    ; 8a 56 17
    xor dh, dh                                ; 30 f6
    cmp dx, strict byte 00004h                ; 83 fa 04
    jne short 04417h                          ; 75 03
    jmp near 0458dh                           ; e9 76 01
    mov CL, strict byte 004h                  ; b1 04
    mov dx, word [bp+010h]                    ; 8b 56 10
    shr dx, CL                                ; d3 ea
    mov cx, dx                                ; 89 d1
    mov dx, word [bp+006h]                    ; 8b 56 06
    add dx, cx                                ; 01 ca
    mov word [bp-020h], dx                    ; 89 56 e0
    mov dx, word [bp+010h]                    ; 8b 56 10
    and dx, strict byte 0000fh                ; 83 e2 0f
    mov word [bp-010h], dx                    ; 89 56 f0
    xor dl, dl                                ; 30 d2
    xor cx, cx                                ; 31 c9
    call 0a150h                               ; e8 18 5d
    xor bx, bx                                ; 31 db
    add ax, si                                ; 01 f0
    adc dx, bx                                ; 11 da
    mov bx, di                                ; 89 fb
    xor cx, cx                                ; 31 c9
    call 0a150h                               ; e8 0b 5d
    mov bx, ax                                ; 89 c3
    mov ax, word [bp-00ah]                    ; 8b 46 f6
    dec ax                                    ; 48
    xor cx, cx                                ; 31 c9
    add ax, bx                                ; 01 d8
    adc dx, cx                                ; 11 ca
    mov bx, word [bp+016h]                    ; 8b 5e 16
    xor bl, bl                                ; 30 db
    mov cx, word [bp-014h]                    ; 8b 4e ec
    or cx, bx                                 ; 09 d9
    mov word [bp+016h], cx                    ; 89 4e 16
    mov si, ax                                ; 89 c6
    mov di, dx                                ; 89 d7
    shr di, 1                                 ; d1 ef
    rcr si, 1                                 ; d1 de
    shr di, 1                                 ; d1 ef
    rcr si, 1                                 ; d1 de
    mov word [bp-01eh], di                    ; 89 7e e2
    mov di, ax                                ; 89 c7
    and di, strict byte 00003h                ; 83 e7 03
    xor bh, bh                                ; 30 ff
    add ax, word [bp-014h]                    ; 03 46 ec
    adc dx, bx                                ; 11 da
    add ax, strict word 0ffffh                ; 05 ff ff
    adc dx, strict byte 0ffffh                ; 83 d2 ff
    mov word [bp-024h], ax                    ; 89 46 dc
    mov word [bp-022h], dx                    ; 89 56 de
    shr word [bp-022h], 1                     ; d1 6e de
    rcr word [bp-024h], 1                     ; d1 5e dc
    shr word [bp-022h], 1                     ; d1 6e de
    rcr word [bp-024h], 1                     ; d1 5e dc
    mov cx, strict word 0000ch                ; b9 0c 00
    mov dx, ss                                ; 8c d2
    lea ax, [bp-030h]                         ; 8d 46 d0
    call 0a1c0h                               ; e8 24 5d
    mov word [bp-030h], strict word 00028h    ; c7 46 d0 28 00
    mov ax, word [bp-00eh]                    ; 8b 46 f2
    add ax, si                                ; 01 f0
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    adc dx, word [bp-01eh]                    ; 13 56 e2
    xchg ah, al                               ; 86 c4
    xchg dh, dl                               ; 86 d6
    xchg dx, ax                               ; 92
    mov word [bp-02eh], ax                    ; 89 46 d2
    mov word [bp-02ch], dx                    ; 89 56 d4
    mov ax, word [bp-024h]                    ; 8b 46 dc
    sub ax, si                                ; 29 f0
    inc ax                                    ; 40
    xchg ah, al                               ; 86 c4
    mov word [bp-029h], ax                    ; 89 46 d7
    mov ax, word [bp-014h]                    ; 8b 46 ec
    les bx, [bp-018h]                         ; c4 5e e8
    mov word [es:bx+00eh], ax                 ; 26 89 47 0e
    mov word [es:bx+010h], 00200h             ; 26 c7 47 10 00 02
    mov CL, strict byte 009h                  ; b1 09
    mov ax, di                                ; 89 f8
    sal ax, CL                                ; d3 e0
    mov word [bp-01ah], ax                    ; 89 46 e6
    mov word [es:bx+01eh], ax                 ; 26 89 47 1e
    mov ax, word [bp-014h]                    ; 8b 46 ec
    xor ah, ah                                ; 30 e4
    and AL, strict byte 003h                  ; 24 03
    mov dx, strict word 00004h                ; ba 04 00
    sub dx, ax                                ; 29 c2
    mov ax, dx                                ; 89 d0
    sub ax, di                                ; 29 f8
    sal ax, CL                                ; d3 e0
    and ah, 007h                              ; 80 e4 07
    mov word [es:bx+020h], ax                 ; 26 89 47 20
    mov al, byte [bp-006h]                    ; 8a 46 fa
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    add bx, ax                                ; 01 c3
    mov al, byte [es:bx+022h]                 ; 26 8a 47 22
    xor ah, ah                                ; 30 e4
    sal ax, 1                                 ; d1 e0
    mov word [bp-01ch], ax                    ; 89 46 e4
    push word [bp-020h]                       ; ff 76 e0
    push word [bp-010h]                       ; ff 76 f0
    mov ax, strict word 00001h                ; b8 01 00
    push ax                                   ; 50
    mov si, word [bp-014h]                    ; 8b 76 ec
    xor di, di                                ; 31 ff
    mov cx, strict word 00009h                ; b9 09 00
    sal si, 1                                 ; d1 e6
    rcl di, 1                                 ; d1 d7
    loop 04521h                               ; e2 fa
    push di                                   ; 57
    push si                                   ; 56
    push word [bp-01ah]                       ; ff 76 e6
    mov al, byte [bp-006h]                    ; 8a 46 fa
    xor ah, ah                                ; 30 e4
    mov cx, ss                                ; 8c d1
    lea bx, [bp-030h]                         ; 8d 5e d0
    mov dx, strict word 0000ch                ; ba 0c 00
    mov si, word [bp-01ch]                    ; 8b 76 e4
    call word [word si+0006ah]                ; ff 94 6a 00
    mov dx, ax                                ; 89 c2
    les bx, [bp-018h]                         ; c4 5e e8
    mov word [es:bx+01eh], strict word 00000h ; 26 c7 47 1e 00 00
    mov word [es:bx+020h], strict word 00000h ; 26 c7 47 20 00 00
    test al, al                               ; 84 c0
    je short 0458dh                           ; 74 38
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 d3 d3
    mov al, dl                                ; 88 d0
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov al, byte [bp+017h]                    ; 8a 46 17
    push ax                                   ; 50
    mov ax, 0034ch                            ; b8 4c 03
    push ax                                   ; 50
    mov ax, 0038eh                            ; b8 8e 03
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 fe d3
    add sp, strict byte 0000ah                ; 83 c4 0a
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 002h                               ; 80 cc 02
    mov word [bp+016h], ax                    ; 89 46 16
    mov byte [bp+016h], 000h                  ; c6 46 16 00
    jmp near 0468ch                           ; e9 ff 00
    mov byte [bp+017h], 000h                  ; c6 46 17 00
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 c4 d0
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
    mov es, [bp-012h]                         ; 8e 46 ee
    mov di, word [es:si+014h]                 ; 26 8b 7c 14
    mov dx, word [es:si+012h]                 ; 26 8b 54 12
    dec dx                                    ; 4a
    mov bx, word [es:si+010h]                 ; 26 8b 5c 10
    dec bx                                    ; 4b
    mov byte [bp+016h], 000h                  ; c6 46 16 00
    mov cx, word [bp+010h]                    ; 8b 4e 10
    xor cl, cl                                ; 30 c9
    mov ax, word [bp+014h]                    ; 8b 46 14
    xor ah, ah                                ; 30 e4
    mov word [bp-01ch], ax                    ; 89 46 e4
    mov ax, dx                                ; 89 d0
    xor ah, dh                                ; 30 f4
    mov word [bp-01ah], ax                    ; 89 46 e6
    mov al, byte [bp-01ah]                    ; 8a 46 e6
    mov byte [bp-019h], al                    ; 88 46 e7
    mov byte [bp-01ah], cl                    ; 88 4e e6
    mov ax, word [bp-01ch]                    ; 8b 46 e4
    or ax, word [bp-01ah]                     ; 0b 46 e6
    mov word [bp+014h], ax                    ; 89 46 14
    shr dx, 1                                 ; d1 ea
    shr dx, 1                                 ; d1 ea
    xor dh, dh                                ; 30 f6
    and dl, 0c0h                              ; 80 e2 c0
    mov word [bp-01ah], dx                    ; 89 56 e6
    mov dx, di                                ; 89 fa
    xor dh, dh                                ; 30 f6
    and dl, 03fh                              ; 80 e2 3f
    or dx, word [bp-01ah]                     ; 0b 56 e6
    xor al, al                                ; 30 c0
    or ax, dx                                 ; 09 d0
    mov word [bp+014h], ax                    ; 89 46 14
    mov dx, word [bp+012h]                    ; 8b 56 12
    mov dh, bl                                ; 88 de
    mov word [bp+012h], dx                    ; 89 56 12
    mov ax, dx                                ; 89 d0
    xor al, dl                                ; 30 d0
    or AL, strict byte 002h                   ; 0c 02
    mov word [bp+012h], ax                    ; 89 46 12
    mov al, byte [es:si+001h]                 ; 26 8a 44 01
    mov word [bp+010h], cx                    ; 89 4e 10
    cmp AL, strict byte 003h                  ; 3c 03
    je short 04637h                           ; 74 1c
    cmp AL, strict byte 002h                  ; 3c 02
    je short 0462fh                           ; 74 10
    cmp AL, strict byte 001h                  ; 3c 01
    jne short 0463ch                          ; 75 19
    mov ax, word [bp+010h]                    ; 8b 46 10
    xor al, al                                ; 30 c0
    or AL, strict byte 002h                   ; 0c 02
    mov word [bp+010h], ax                    ; 89 46 10
    jmp short 0463ch                          ; eb 0d
    or cl, 004h                               ; 80 c9 04
    mov word [bp+010h], cx                    ; 89 4e 10
    jmp short 0463ch                          ; eb 05
    or cl, 005h                               ; 80 c9 05
    jmp short 04632h                          ; eb f6
    mov es, [bp-012h]                         ; 8e 46 ee
    cmp byte [es:si+001h], 004h               ; 26 80 7c 01 04
    jc short 04649h                           ; 72 03
    jmp near 0458dh                           ; e9 44 ff
    mov word [bp+008h], 0efc7h                ; c7 46 08 c7 ef
    mov word [bp+006h], 0f000h                ; c7 46 06 00 f0
    jmp short 04646h                          ; eb f1
    or bh, 003h                               ; 80 cf 03
    mov word [bp+016h], bx                    ; 89 5e 16
    jmp near 04591h                           ; e9 33 ff
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 ca d2
    mov al, byte [bp+017h]                    ; 8a 46 17
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 0034ch                            ; b8 4c 03
    push ax                                   ; 50
    mov ax, 003afh                            ; b8 af 03
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 f8 d2
    add sp, strict byte 00008h                ; 83 c4 08
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 001h                               ; 80 cc 01
    mov word [bp+016h], ax                    ; 89 46 16
    mov bl, byte [bp+017h]                    ; 8a 5e 17
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 c6 cf
    or byte [bp+01ch], 001h                   ; 80 4e 1c 01
    jmp near 045a0h                           ; e9 ff fe
    db  050h, 04eh, 049h, 048h, 047h, 046h, 045h, 044h, 043h, 042h, 041h, 018h, 016h, 015h, 014h, 011h
    db  010h, 00dh, 00ch, 00bh, 00ah, 009h, 008h, 005h, 004h, 003h, 002h, 001h, 000h, 0e3h, 047h, 028h
    db  04ch, 0a5h, 047h, 0e3h, 047h, 09ah, 047h, 0e3h, 047h, 09ah, 047h, 0e3h, 047h, 028h, 04ch, 0e3h
    db  047h, 0e3h, 047h, 028h, 04ch, 028h, 04ch, 028h, 04ch, 028h, 04ch, 028h, 04ch, 0c7h, 047h, 028h
    db  04ch, 0e3h, 047h, 0d0h, 047h, 001h, 048h, 09ah, 047h, 001h, 048h, 04ah, 049h, 0eah, 049h, 001h
    db  048h, 014h, 04ah, 042h, 04ch, 04ah, 04ch, 0e3h, 047h
_int13_cdrom:                                ; 0xf46fa LB 0x585
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 0002ah                ; 83 ec 2a
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 63 cf
    mov word [bp-018h], ax                    ; 89 46 e8
    mov word [bp-00eh], 00122h                ; c7 46 f2 22 01
    mov word [bp-00ch], ax                    ; 89 46 f4
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 3f cf
    mov ax, word [bp+010h]                    ; 8b 46 10
    xor ah, ah                                ; 30 e4
    cmp ax, 000e0h                            ; 3d e0 00
    jc short 04730h                           ; 72 05
    cmp ax, 000f0h                            ; 3d f0 00
    jc short 0474fh                           ; 72 1f
    mov ax, word [bp+010h]                    ; 8b 46 10
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov al, byte [bp+019h]                    ; 8a 46 19
    push ax                                   ; 50
    mov ax, 003dfh                            ; b8 df 03
    push ax                                   ; 50
    mov ax, 003ebh                            ; b8 eb 03
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 2d d2
    add sp, strict byte 0000ah                ; 83 c4 0a
    jmp near 04c60h                           ; e9 11 05
    mov ax, word [bp+010h]                    ; 8b 46 10
    xor ah, ah                                ; 30 e4
    les bx, [bp-00eh]                         ; c4 5e f2
    add bx, ax                                ; 01 c3
    mov dl, byte [es:bx+00114h]               ; 26 8a 97 14 01
    mov byte [bp-008h], dl                    ; 88 56 f8
    cmp dl, 010h                              ; 80 fa 10
    jc short 04774h                           ; 72 0e
    push ax                                   ; 50
    mov al, byte [bp+019h]                    ; 8a 46 19
    push ax                                   ; 50
    mov ax, 003dfh                            ; b8 df 03
    push ax                                   ; 50
    mov ax, 00416h                            ; b8 16 04
    jmp short 04741h                          ; eb cd
    mov al, byte [bp+019h]                    ; 8a 46 19
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    cmp ax, strict word 00050h                ; 3d 50 00
    jnbe short 047e3h                         ; 77 63
    push CS                                   ; 0e
    pop ES                                    ; 07
    mov cx, strict word 0001eh                ; b9 1e 00
    mov di, 046a1h                            ; bf a1 46
    repne scasb                               ; f2 ae
    sal cx, 1                                 ; d1 e1
    mov di, cx                                ; 89 cf
    mov ax, word [cs:di+046beh]               ; 2e 8b 85 be 46
    mov bx, word [bp+018h]                    ; 8b 5e 18
    xor bh, bh                                ; 30 ff
    jmp ax                                    ; ff e0
    mov ax, word [bp+018h]                    ; 8b 46 18
    xor ah, ah                                ; 30 e4
    or ah, 003h                               ; 80 cc 03
    jmp near 04c68h                           ; e9 c3 04
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 a4 ce
    mov cl, al                                ; 88 c1
    mov bh, al                                ; 88 c7
    mov word [bp+018h], bx                    ; 89 5e 18
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 a0 ce
    test cl, cl                               ; 84 c9
    je short 047e0h                           ; 74 1c
    jmp near 04c79h                           ; e9 b2 04
    or bh, 002h                               ; 80 cf 02
    mov word [bp+018h], bx                    ; 89 5e 18
    jmp near 04c6bh                           ; e9 9b 04
    mov word [bp+012h], 0aa55h                ; c7 46 12 55 aa
    or bh, 030h                               ; 80 cf 30
    mov word [bp+018h], bx                    ; 89 5e 18
    mov word [bp+016h], strict word 00007h    ; c7 46 16 07 00
    jmp near 04c2ch                           ; e9 49 04
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 45 d1
    mov al, byte [bp+019h]                    ; 8a 46 19
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 003dfh                            ; b8 df 03
    push ax                                   ; 50
    mov ax, 00333h                            ; b8 33 03
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    jmp short 04842h                          ; eb 41
    mov bx, word [bp+00ch]                    ; 8b 5e 0c
    mov es, [bp+006h]                         ; 8e 46 06
    mov di, bx                                ; 89 df
    mov [bp-020h], es                         ; 8c 46 e0
    mov si, word [es:bx+002h]                 ; 26 8b 77 02
    mov ax, word [es:bx+006h]                 ; 26 8b 47 06
    mov word [bp-01ch], ax                    ; 89 46 e4
    mov ax, word [es:bx+004h]                 ; 26 8b 47 04
    mov word [bp-01ah], ax                    ; 89 46 e6
    mov ax, word [es:bx+00ch]                 ; 26 8b 47 0c
    mov word [bp-014h], ax                    ; 89 46 ec
    mov ax, word [es:bx+00eh]                 ; 26 8b 47 0e
    mov word [bp-012h], ax                    ; 89 46 ee
    or ax, word [bp-014h]                     ; 0b 46 ec
    je short 0484ch                           ; 74 1b
    mov al, byte [bp+019h]                    ; 8a 46 19
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 003dfh                            ; b8 df 03
    push ax                                   ; 50
    mov ax, 00448h                            ; b8 48 04
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 30 d1
    add sp, strict byte 00008h                ; 83 c4 08
    jmp near 04c60h                           ; e9 14 04
    mov es, [bp-020h]                         ; 8e 46 e0
    mov ax, word [es:di+008h]                 ; 26 8b 45 08
    mov word [bp-014h], ax                    ; 89 46 ec
    mov ax, word [es:di+00ah]                 ; 26 8b 45 0a
    mov word [bp-012h], ax                    ; 89 46 ee
    mov al, byte [bp+019h]                    ; 8a 46 19
    mov byte [bp-010h], al                    ; 88 46 f0
    mov byte [bp-00fh], 000h                  ; c6 46 f1 00
    mov ax, word [bp-010h]                    ; 8b 46 f0
    cmp ax, strict word 00044h                ; 3d 44 00
    je short 04874h                           ; 74 05
    cmp ax, strict word 00047h                ; 3d 47 00
    jne short 04877h                          ; 75 03
    jmp near 04c28h                           ; e9 b1 03
    mov cx, strict word 0000ch                ; b9 0c 00
    xor bx, bx                                ; 31 db
    mov dx, ss                                ; 8c d2
    lea ax, [bp-02eh]                         ; 8d 46 d2
    call 0a1c0h                               ; e8 3c 59
    mov word [bp-02eh], strict word 00028h    ; c7 46 d2 28 00
    mov ax, word [bp-014h]                    ; 8b 46 ec
    mov dx, word [bp-012h]                    ; 8b 56 ee
    xchg ah, al                               ; 86 c4
    xchg dh, dl                               ; 86 d6
    xchg dx, ax                               ; 92
    mov word [bp-02ch], ax                    ; 89 46 d4
    mov word [bp-02ah], dx                    ; 89 56 d6
    mov ax, si                                ; 89 f0
    xchg ah, al                               ; 86 c4
    mov word [bp-027h], ax                    ; 89 46 d9
    les bx, [bp-00eh]                         ; c4 5e f2
    mov word [es:bx+00eh], si                 ; 26 89 77 0e
    mov word [es:bx+010h], 00800h             ; 26 c7 47 10 00 08
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    add bx, ax                                ; 01 c3
    mov al, byte [es:bx+022h]                 ; 26 8a 47 22
    xor ah, ah                                ; 30 e4
    sal ax, 1                                 ; d1 e0
    mov word [bp-022h], ax                    ; 89 46 de
    push word [bp-01ch]                       ; ff 76 e4
    push word [bp-01ah]                       ; ff 76 e6
    mov ax, strict word 00001h                ; b8 01 00
    push ax                                   ; 50
    mov bx, si                                ; 89 f3
    xor si, si                                ; 31 f6
    mov cx, strict word 0000bh                ; b9 0b 00
    sal bx, 1                                 ; d1 e3
    rcl si, 1                                 ; d1 d6
    loop 048d6h                               ; e2 fa
    push si                                   ; 56
    push bx                                   ; 53
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    mov cx, ss                                ; 8c d1
    lea bx, [bp-02eh]                         ; 8d 5e d2
    mov dx, strict word 0000ch                ; ba 0c 00
    mov si, word [bp-022h]                    ; 8b 76 de
    call word [word si+0006ah]                ; ff 94 6a 00
    mov word [bp-016h], ax                    ; 89 46 ea
    les bx, [bp-00eh]                         ; c4 5e f2
    mov ax, word [es:bx+01ah]                 ; 26 8b 47 1a
    mov dx, word [es:bx+01ch]                 ; 26 8b 57 1c
    mov cx, strict word 0000bh                ; b9 0b 00
    shr dx, 1                                 ; d1 ea
    rcr ax, 1                                 ; d1 d8
    loop 04906h                               ; e2 fa
    mov es, [bp-020h]                         ; 8e 46 e0
    mov word [es:di+002h], ax                 ; 26 89 45 02
    cmp byte [bp-016h], 000h                  ; 80 7e ea 00
    je short 04970h                           ; 74 57
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 0f d0
    mov al, byte [bp-016h]                    ; 8a 46 ea
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    push word [bp-010h]                       ; ff 76 f0
    mov ax, 003dfh                            ; b8 df 03
    push ax                                   ; 50
    mov ax, 00471h                            ; b8 71 04
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 3a d0
    add sp, strict byte 0000ah                ; 83 c4 0a
    mov ax, word [bp+018h]                    ; 8b 46 18
    xor ah, ah                                ; 30 e4
    or ah, 00ch                               ; 80 cc 0c
    jmp near 04c68h                           ; e9 1e 03
    cmp bx, strict byte 00002h                ; 83 fb 02
    jnbe short 049bbh                         ; 77 6c
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    les si, [bp-00eh]                         ; c4 76 f2
    add si, ax                                ; 01 c6
    mov cl, byte [es:si+025h]                 ; 26 8a 4c 25
    cmp bx, strict byte 00002h                ; 83 fb 02
    je short 049d3h                           ; 74 6c
    cmp bx, strict byte 00001h                ; 83 fb 01
    je short 049abh                           ; 74 3f
    test bx, bx                               ; 85 db
    je short 04973h                           ; 74 03
    jmp near 04c28h                           ; e9 b5 02
    cmp cl, 0ffh                              ; 80 f9 ff
    jne short 0498ah                          ; 75 12
    mov ax, word [bp+018h]                    ; 8b 46 18
    xor ah, ah                                ; 30 e4
    or ah, 0b4h                               ; 80 cc b4
    mov word [bp+018h], ax                    ; 89 46 18
    xor al, al                                ; 30 c0
    or AL, strict byte 001h                   ; 0c 01
    jmp near 04c68h                           ; e9 de 02
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    db  0feh, 0c1h
    ; inc cl                                    ; fe c1
    les bx, [bp-00eh]                         ; c4 5e f2
    add bx, ax                                ; 01 c3
    mov byte [es:bx+025h], cl                 ; 26 88 4f 25
    mov ax, word [bp+018h]                    ; 8b 46 18
    xor al, al                                ; 30 c0
    or AL, strict byte 001h                   ; 0c 01
    mov word [bp+018h], ax                    ; 89 46 18
    jmp short 04970h                          ; eb c5
    test cl, cl                               ; 84 c9
    jne short 049beh                          ; 75 0f
    or bh, 0b0h                               ; 80 cf b0
    mov word [bp+018h], bx                    ; 89 5e 18
    mov byte [bp+018h], cl                    ; 88 4e 18
    jmp near 04c6bh                           ; e9 b0 02
    jmp near 04c60h                           ; e9 a2 02
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    db  0feh, 0c9h
    ; dec cl                                    ; fe c9
    les bx, [bp-00eh]                         ; c4 5e f2
    add bx, ax                                ; 01 c3
    mov byte [es:bx+025h], cl                 ; 26 88 4f 25
    test cl, cl                               ; 84 c9
    jne short 049e5h                          ; 75 0e
    xor ax, ax                                ; 31 c0
    mov dx, word [bp+018h]                    ; 8b 56 18
    xor dl, dl                                ; 30 d2
    or dx, ax                                 ; 09 c2
    mov word [bp+018h], dx                    ; 89 56 18
    jmp short 04970h                          ; eb 8b
    mov ax, strict word 00001h                ; b8 01 00
    jmp short 049d9h                          ; eb ef
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    les si, [bp-00eh]                         ; c4 76 f2
    add si, ax                                ; 01 c6
    mov cl, byte [es:si+025h]                 ; 26 8a 4c 25
    test cl, cl                               ; 84 c9
    je short 04a07h                           ; 74 06
    or bh, 0b1h                               ; 80 cf b1
    jmp near 047cah                           ; e9 c3 fd
    je short 04a2eh                           ; 74 25
    mov ax, word [bp+018h]                    ; 8b 46 18
    xor ah, ah                                ; 30 e4
    or ah, 0b1h                               ; 80 cc b1
    jmp near 04c68h                           ; e9 54 02
    mov bx, word [bp+00ch]                    ; 8b 5e 0c
    mov cx, word [bp+006h]                    ; 8b 4e 06
    mov si, bx                                ; 89 de
    mov word [bp-00ah], cx                    ; 89 4e f6
    mov es, cx                                ; 8e c1
    mov ax, word [es:bx]                      ; 26 8b 07
    mov word [bp-01eh], ax                    ; 89 46 e2
    cmp ax, strict word 0001ah                ; 3d 1a 00
    jnc short 04a31h                          ; 73 05
    jmp short 049bbh                          ; eb 8d
    jmp near 04c28h                           ; e9 f7 01
    jc short 04a93h                           ; 72 60
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    les di, [bp-00eh]                         ; c4 7e f2
    add di, ax                                ; 01 c7
    mov ax, word [es:di+028h]                 ; 26 8b 45 28
    mov es, cx                                ; 8e c1
    mov word [es:bx], strict word 0001ah      ; 26 c7 07 1a 00
    mov word [es:bx+002h], strict word 00074h ; 26 c7 47 02 74 00
    mov word [es:bx+004h], strict word 0ffffh ; 26 c7 47 04 ff ff
    mov word [es:bx+006h], strict word 0ffffh ; 26 c7 47 06 ff ff
    mov word [es:bx+008h], strict word 0ffffh ; 26 c7 47 08 ff ff
    mov word [es:bx+00ah], strict word 0ffffh ; 26 c7 47 0a ff ff
    mov word [es:bx+00ch], strict word 0ffffh ; 26 c7 47 0c ff ff
    mov word [es:bx+00eh], strict word 0ffffh ; 26 c7 47 0e ff ff
    mov word [es:bx+018h], ax                 ; 26 89 47 18
    mov word [es:bx+010h], strict word 0ffffh ; 26 c7 47 10 ff ff
    mov word [es:bx+012h], strict word 0ffffh ; 26 c7 47 12 ff ff
    mov word [es:bx+014h], strict word 0ffffh ; 26 c7 47 14 ff ff
    mov word [es:bx+016h], strict word 0ffffh ; 26 c7 47 16 ff ff
    cmp word [bp-01eh], strict byte 0001eh    ; 83 7e e2 1e
    jnc short 04a9ch                          ; 73 03
    jmp near 04b5ch                           ; e9 c0 00
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov word [es:si], strict word 0001eh      ; 26 c7 04 1e 00
    mov ax, word [bp-018h]                    ; 8b 46 e8
    mov word [es:si+01ch], ax                 ; 26 89 44 1c
    mov word [es:si+01ah], 00356h             ; 26 c7 44 1a 56 03
    mov cl, byte [bp-008h]                    ; 8a 4e f8
    xor ch, ch                                ; 30 ed
    mov ax, cx                                ; 89 c8
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    xor ah, ah                                ; 30 e4
    mov dx, strict word 00006h                ; ba 06 00
    imul dx                                   ; f7 ea
    les bx, [bp-00eh]                         ; c4 5e f2
    add bx, ax                                ; 01 c3
    mov ax, word [es:bx+00206h]               ; 26 8b 87 06 02
    mov di, word [es:bx+00208h]               ; 26 8b bf 08 02
    mov dl, byte [es:bx+00205h]               ; 26 8a 97 05 02
    mov byte [bp-006h], dl                    ; 88 56 fa
    mov bx, word [bp-00eh]                    ; 8b 5e f2
    mov word [es:bx+00234h], ax               ; 26 89 87 34 02
    mov word [es:bx+00236h], di               ; 26 89 bf 36 02
    mov ax, cx                                ; 89 c8
    cwd                                       ; 99
    mov bx, strict word 00002h                ; bb 02 00
    idiv bx                                   ; f7 fb
    or dl, 00eh                               ; 80 ca 0e
    mov CL, strict byte 004h                  ; b1 04
    sal dx, CL                                ; d3 e2
    mov bx, word [bp-00eh]                    ; 8b 5e f2
    mov byte [es:bx+00238h], dl               ; 26 88 97 38 02
    mov byte [es:bx+00239h], 0cbh             ; 26 c6 87 39 02 cb
    mov al, byte [bp-006h]                    ; 8a 46 fa
    mov byte [es:bx+0023ah], al               ; 26 88 87 3a 02
    mov byte [es:bx+0023bh], 001h             ; 26 c6 87 3b 02 01
    mov byte [es:bx+0023ch], ch               ; 26 88 af 3c 02
    mov byte [es:bx+0023dh], ch               ; 26 88 af 3d 02
    mov word [es:bx+0023eh], strict word 00070h ; 26 c7 87 3e 02 70 00
    mov word [es:bx+00240h], strict word 00000h ; 26 c7 87 40 02 00 00
    mov byte [es:bx+00242h], 011h             ; 26 c6 87 42 02 11
    xor bl, bl                                ; 30 db
    xor bh, bh                                ; 30 ff
    jmp short 04b3ch                          ; eb 05
    cmp bh, 00fh                              ; 80 ff 0f
    jnc short 04b52h                          ; 73 16
    mov al, bh                                ; 88 f8
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    add dx, 00356h                            ; 81 c2 56 03
    mov ax, word [bp-018h]                    ; 8b 46 e8
    call 01652h                               ; e8 06 cb
    add bl, al                                ; 00 c3
    db  0feh, 0c7h
    ; inc bh                                    ; fe c7
    jmp short 04b37h                          ; eb e5
    neg bl                                    ; f6 db
    les di, [bp-00eh]                         ; c4 7e f2
    mov byte [es:di+00243h], bl               ; 26 88 9d 43 02
    cmp word [bp-01eh], strict byte 00042h    ; 83 7e e2 42
    jnc short 04b65h                          ; 73 03
    jmp near 04c28h                           ; e9 c3 00
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    xor ah, ah                                ; 30 e4
    mov dx, strict word 00006h                ; ba 06 00
    imul dx                                   ; f7 ea
    les bx, [bp-00eh]                         ; c4 5e f2
    add bx, ax                                ; 01 c3
    mov al, byte [es:bx+00204h]               ; 26 8a 87 04 02
    mov dx, word [es:bx+00206h]               ; 26 8b 97 06 02
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov word [es:si], strict word 00042h      ; 26 c7 04 42 00
    mov word [es:si+01eh], 0beddh             ; 26 c7 44 1e dd be
    mov word [es:si+020h], strict word 00024h ; 26 c7 44 20 24 00
    mov word [es:si+022h], strict word 00000h ; 26 c7 44 22 00 00
    test al, al                               ; 84 c0
    jne short 04bafh                          ; 75 0c
    mov word [es:si+024h], 05349h             ; 26 c7 44 24 49 53
    mov word [es:si+026h], 02041h             ; 26 c7 44 26 41 20
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov word [es:si+028h], 05441h             ; 26 c7 44 28 41 54
    mov word [es:si+02ah], 02041h             ; 26 c7 44 2a 41 20
    mov word [es:si+02ch], 02020h             ; 26 c7 44 2c 20 20
    mov word [es:si+02eh], 02020h             ; 26 c7 44 2e 20 20
    test al, al                               ; 84 c0
    jne short 04be4h                          ; 75 16
    mov word [es:si+030h], dx                 ; 26 89 54 30
    mov word [es:si+032h], strict word 00000h ; 26 c7 44 32 00 00
    mov word [es:si+034h], strict word 00000h ; 26 c7 44 34 00 00
    mov word [es:si+036h], strict word 00000h ; 26 c7 44 36 00 00
    mov al, byte [bp-008h]                    ; 8a 46 f8
    and AL, strict byte 001h                  ; 24 01
    xor ah, ah                                ; 30 e4
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov word [es:si+038h], ax                 ; 26 89 44 38
    mov word [es:si+03ah], strict word 00000h ; 26 c7 44 3a 00 00
    mov word [es:si+03ch], strict word 00000h ; 26 c7 44 3c 00 00
    mov word [es:si+03eh], strict word 00000h ; 26 c7 44 3e 00 00
    xor al, al                                ; 30 c0
    mov AH, strict byte 01eh                  ; b4 1e
    jmp short 04c0fh                          ; eb 05
    cmp ah, 040h                              ; 80 fc 40
    jnc short 04c1fh                          ; 73 10
    mov bl, ah                                ; 88 e3
    xor bh, bh                                ; 30 ff
    mov es, [bp-00ah]                         ; 8e 46 f6
    add bx, si                                ; 01 f3
    add al, byte [es:bx]                      ; 26 02 07
    db  0feh, 0c4h
    ; inc ah                                    ; fe c4
    jmp short 04c0ah                          ; eb eb
    neg al                                    ; f6 d8
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov byte [es:si+041h], al                 ; 26 88 44 41
    mov byte [bp+019h], 000h                  ; c6 46 19 00
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 29 ca
    and byte [bp+01eh], 0feh                  ; 80 66 1e fe
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
    or bh, 006h                               ; 80 cf 06
    mov word [bp+018h], bx                    ; 89 5e 18
    jmp short 04c79h                          ; eb 2f
    cmp bx, strict byte 00006h                ; 83 fb 06
    je short 04c28h                           ; 74 d9
    cmp bx, strict byte 00001h                ; 83 fb 01
    jc short 04c60h                           ; 72 0c
    jbe short 04c28h                          ; 76 d2
    cmp bx, strict byte 00003h                ; 83 fb 03
    jc short 04c60h                           ; 72 05
    cmp bx, strict byte 00004h                ; 83 fb 04
    jbe short 04c28h                          ; 76 c8
    mov ax, word [bp+018h]                    ; 8b 46 18
    xor ah, ah                                ; 30 e4
    or ah, 001h                               ; 80 cc 01
    mov word [bp+018h], ax                    ; 89 46 18
    mov bl, byte [bp+019h]                    ; 8a 5e 19
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 e7 c9
    or byte [bp+01eh], 001h                   ; 80 4e 1e 01
    jmp short 04c3bh                          ; eb bc
print_boot_device_:                          ; 0xf4c7f LB 0x51
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push cx                                   ; 51
    test al, al                               ; 84 c0
    je short 04c8ch                           ; 74 05
    mov dx, strict word 00002h                ; ba 02 00
    jmp short 04ca6h                          ; eb 1a
    test dl, dl                               ; 84 d2
    je short 04c95h                           ; 74 05
    mov dx, strict word 00003h                ; ba 03 00
    jmp short 04ca6h                          ; eb 11
    test bl, 080h                             ; f6 c3 80
    jne short 04c9eh                          ; 75 04
    xor dh, dh                                ; 30 f6
    jmp short 04ca6h                          ; eb 08
    test bl, 080h                             ; f6 c3 80
    je short 04ccah                           ; 74 27
    mov dx, strict word 00001h                ; ba 01 00
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 82 cc
    mov ax, dx                                ; 89 d0
    mov dx, strict word 0000ah                ; ba 0a 00
    imul dx                                   ; f7 ea
    add ax, 00dc6h                            ; 05 c6 0d
    push ax                                   ; 50
    mov ax, 00494h                            ; b8 94 04
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 af cc
    add sp, strict byte 00006h                ; 83 c4 06
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop cx                                    ; 59
    pop bp                                    ; 5d
    retn                                      ; c3
print_boot_failure_:                         ; 0xf4cd0 LB 0x9f
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    mov dh, cl                                ; 88 ce
    mov cl, bl                                ; 88 d9
    and cl, 07fh                              ; 80 e1 7f
    xor ch, ch                                ; 30 ed
    mov si, cx                                ; 89 ce
    test al, al                               ; 84 c0
    je short 04d02h                           ; 74 1f
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 45 cc
    mov cx, 00ddah                            ; b9 da 0d
    push cx                                   ; 51
    mov cx, 004a8h                            ; b9 a8 04
    push cx                                   ; 51
    mov cx, strict word 00004h                ; b9 04 00
    push cx                                   ; 51
    call 01976h                               ; e8 79 cc
    add sp, strict byte 00006h                ; 83 c4 06
    jmp short 04d4ah                          ; eb 48
    test dl, dl                               ; 84 d2
    je short 04d16h                           ; 74 10
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 22 cc
    mov cx, 00de4h                            ; b9 e4 0d
    jmp short 04cf1h                          ; eb db
    test bl, 080h                             ; f6 c3 80
    je short 04d2ch                           ; 74 11
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 0d cc
    push si                                   ; 56
    mov cx, 00dd0h                            ; b9 d0 0d
    jmp short 04d3bh                          ; eb 0f
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 fc cb
    push si                                   ; 56
    mov cx, 00dc6h                            ; b9 c6 0d
    push cx                                   ; 51
    mov cx, 004bdh                            ; b9 bd 04
    push cx                                   ; 51
    mov cx, strict word 00004h                ; b9 04 00
    push cx                                   ; 51
    call 01976h                               ; e8 2f cc
    add sp, strict byte 00008h                ; 83 c4 08
    cmp byte [bp+004h], 001h                  ; 80 7e 04 01
    jne short 04d67h                          ; 75 17
    test dh, dh                               ; 84 f6
    jne short 04d59h                          ; 75 05
    mov dx, 004d5h                            ; ba d5 04
    jmp short 04d5ch                          ; eb 03
    mov dx, 004ffh                            ; ba ff 04
    push dx                                   ; 52
    mov dx, strict word 00007h                ; ba 07 00
    push dx                                   ; 52
    call 01976h                               ; e8 12 cc
    add sp, strict byte 00004h                ; 83 c4 04
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 00002h                               ; c2 02 00
print_cdromboot_failure_:                    ; 0xf4d6f LB 0x2a
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    mov dx, ax                                ; 89 c2
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 b1 cb
    push dx                                   ; 52
    mov dx, 00534h                            ; ba 34 05
    push dx                                   ; 52
    mov dx, strict word 00004h                ; ba 04 00
    push dx                                   ; 52
    call 01976h                               ; e8 e8 cb
    add sp, strict byte 00006h                ; 83 c4 06
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
_int19_function:                             ; 0xf4d99 LB 0x28d
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00010h                ; 83 ec 10
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 c4 c8
    mov bx, ax                                ; 89 c3
    mov di, ax                                ; 89 c7
    mov byte [bp-00ch], 000h                  ; c6 46 f4 00
    mov ax, strict word 0003dh                ; b8 3d 00
    call 016aeh                               ; e8 f6 c8
    mov dl, al                                ; 88 c2
    xor dh, dh                                ; 30 f6
    mov word [bp-00eh], dx                    ; 89 56 f2
    mov ax, strict word 00038h                ; b8 38 00
    call 016aeh                               ; e8 e9 c8
    and AL, strict byte 0f0h                  ; 24 f0
    mov byte [bp-010h], al                    ; 88 46 f0
    mov byte [bp-00fh], dh                    ; 88 76 f1
    mov CL, strict byte 004h                  ; b1 04
    mov ax, word [bp-010h]                    ; 8b 46 f0
    sal ax, CL                                ; d3 e0
    or dx, ax                                 ; 09 c2
    mov word [bp-00eh], dx                    ; 89 56 f2
    mov ax, strict word 0003ch                ; b8 3c 00
    call 016aeh                               ; e8 cf c8
    and AL, strict byte 00fh                  ; 24 0f
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 00ch                  ; b1 0c
    sal ax, CL                                ; d3 e0
    or word [bp-00eh], ax                     ; 09 46 f2
    mov dx, 0037dh                            ; ba 7d 03
    mov ax, bx                                ; 89 d8
    call 01652h                               ; e8 60 c8
    test al, al                               ; 84 c0
    je short 04e03h                           ; 74 0d
    mov dx, 0037dh                            ; ba 7d 03
    mov ax, bx                                ; 89 d8
    call 01652h                               ; e8 54 c8
    xor ah, ah                                ; 30 e4
    mov word [bp-00eh], ax                    ; 89 46 f2
    cmp byte [bp+004h], 001h                  ; 80 7e 04 01
    jne short 04e1ah                          ; 75 11
    mov ax, strict word 0003ch                ; b8 3c 00
    call 016aeh                               ; e8 9f c8
    and AL, strict byte 0f0h                  ; 24 f0
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 004h                  ; b1 04
    sar ax, CL                                ; d3 f8
    call 07e3dh                               ; e8 23 30
    cmp byte [bp+004h], 002h                  ; 80 7e 04 02
    jne short 04e25h                          ; 75 05
    mov CL, strict byte 004h                  ; b1 04
    shr word [bp-00eh], CL                    ; d3 6e f2
    cmp byte [bp+004h], 003h                  ; 80 7e 04 03
    jne short 04e33h                          ; 75 08
    mov al, byte [bp-00dh]                    ; 8a 46 f3
    xor ah, ah                                ; 30 e4
    mov word [bp-00eh], ax                    ; 89 46 f2
    cmp byte [bp+004h], 004h                  ; 80 7e 04 04
    jne short 04e3eh                          ; 75 05
    mov CL, strict byte 00ch                  ; b1 0c
    shr word [bp-00eh], CL                    ; d3 6e f2
    cmp word [bp-00eh], strict byte 00010h    ; 83 7e f2 10
    jnc short 04e48h                          ; 73 04
    mov byte [bp-00ch], 001h                  ; c6 46 f4 01
    xor al, al                                ; 30 c0
    mov byte [bp-006h], al                    ; 88 46 fa
    mov byte [bp-008h], al                    ; 88 46 f8
    mov byte [bp-00ah], al                    ; 88 46 f6
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 d5 ca
    push word [bp-00eh]                       ; ff 76 f2
    mov al, byte [bp+004h]                    ; 8a 46 04
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 00554h                            ; b8 54 05
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 04 cb
    add sp, strict byte 00008h                ; 83 c4 08
    mov ax, word [bp-00eh]                    ; 8b 46 f2
    and ax, strict word 0000fh                ; 25 0f 00
    cmp ax, strict word 00002h                ; 3d 02 00
    jc short 04e8eh                           ; 72 0e
    jbe short 04e9dh                          ; 76 1b
    cmp ax, strict word 00004h                ; 3d 04 00
    je short 04ebah                           ; 74 33
    cmp ax, strict word 00003h                ; 3d 03 00
    je short 04eb0h                           ; 74 24
    jmp short 04ee9h                          ; eb 5b
    cmp ax, strict word 00001h                ; 3d 01 00
    jne short 04ee9h                          ; 75 56
    xor al, al                                ; 30 c0
    mov byte [bp-006h], al                    ; 88 46 fa
    mov byte [bp-008h], al                    ; 88 46 f8
    jmp short 04efdh                          ; eb 60
    mov dx, 0037ch                            ; ba 7c 03
    mov ax, di                                ; 89 f8
    call 01652h                               ; e8 ad c7
    add AL, strict byte 080h                  ; 04 80
    mov byte [bp-006h], al                    ; 88 46 fa
    mov byte [bp-008h], 000h                  ; c6 46 f8 00
    jmp short 04efdh                          ; eb 4d
    mov byte [bp-006h], 000h                  ; c6 46 fa 00
    mov byte [bp-008h], 001h                  ; c6 46 f8 01
    jmp short 04ec4h                          ; eb 0a
    mov byte [bp-00ah], 001h                  ; c6 46 f6 01
    cmp byte [bp-008h], 000h                  ; 80 7e f8 00
    je short 04efdh                           ; 74 39
    call 03e69h                               ; e8 a2 ef
    mov bx, ax                                ; 89 c3
    test AL, strict byte 0ffh                 ; a8 ff
    je short 04ef0h                           ; 74 23
    call 04d6fh                               ; e8 9f fe
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov bl, byte [bp-006h]                    ; 8a 5e fa
    xor bh, bh                                ; 30 ff
    mov dl, byte [bp-00ah]                    ; 8a 56 f6
    xor dh, dh                                ; 30 f6
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov cx, strict word 00001h                ; b9 01 00
    call 04cd0h                               ; e8 e7 fd
    xor ax, ax                                ; 31 c0
    xor dx, dx                                ; 31 d2
    jmp near 0501fh                           ; e9 2f 01
    mov dx, 00372h                            ; ba 72 03
    mov ax, di                                ; 89 f8
    call 0166eh                               ; e8 76 c7
    mov si, ax                                ; 89 c6
    mov byte [bp-006h], bh                    ; 88 7e fa
    cmp byte [bp-00ah], 001h                  ; 80 7e f6 01
    jne short 04f58h                          ; 75 55
    xor si, si                                ; 31 f6
    mov ax, 0e200h                            ; b8 00 e2
    mov es, ax                                ; 8e c0
    cmp word [es:si], 0aa55h                  ; 26 81 3c 55 aa
    jne short 04ed0h                          ; 75 bf
    mov cx, ax                                ; 89 c1
    mov si, word [es:si+01ah]                 ; 26 8b 74 1a
    cmp word [es:si+002h], 0506eh             ; 26 81 7c 02 6e 50
    jne short 04ed0h                          ; 75 b1
    cmp word [es:si], 05024h                  ; 26 81 3c 24 50
    jne short 04ed0h                          ; 75 aa
    mov bx, word [es:si+00eh]                 ; 26 8b 5c 0e
    mov dx, word [es:bx]                      ; 26 8b 17
    mov ax, word [es:bx+002h]                 ; 26 8b 47 02
    cmp ax, 06568h                            ; 3d 68 65
    jne short 04f5ah                          ; 75 24
    cmp dx, 07445h                            ; 81 fa 45 74
    jne short 04f5ah                          ; 75 1e
    mov bl, byte [bp-006h]                    ; 8a 5e fa
    xor bh, bh                                ; 30 ff
    mov dl, byte [bp-00ah]                    ; 8a 56 f6
    xor dh, dh                                ; 30 f6
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    call 04c7fh                               ; e8 31 fd
    mov word [bp-014h], strict word 00006h    ; c7 46 ec 06 00
    mov word [bp-012h], cx                    ; 89 4e ee
    jmp short 04f79h                          ; eb 21
    jmp short 04f7fh                          ; eb 25
    mov bl, byte [bp-006h]                    ; 8a 5e fa
    xor bh, bh                                ; 30 ff
    mov dl, byte [bp-00ah]                    ; 8a 56 f6
    xor dh, dh                                ; 30 f6
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    call 04c7fh                               ; e8 13 fd
    sti                                       ; fb
    mov word [bp-012h], cx                    ; 89 4e ee
    mov es, cx                                ; 8e c1
    mov ax, word [es:si+01ah]                 ; 26 8b 44 1a
    mov word [bp-014h], ax                    ; 89 46 ec
    call far [bp-014h]                        ; ff 5e ec
    jmp near 04ed0h                           ; e9 51 ff
    cmp byte [bp-008h], 000h                  ; 80 7e f8 00
    jne short 04fach                          ; 75 27
    cmp byte [bp-00ah], 000h                  ; 80 7e f6 00
    jne short 04fach                          ; 75 21
    mov si, 007c0h                            ; be c0 07
    mov es, si                                ; 8e c6
    mov dl, byte [bp-006h]                    ; 8a 56 fa
    mov ax, 00201h                            ; b8 01 02
    mov DH, strict byte 000h                  ; b6 00
    mov cx, strict word 00001h                ; b9 01 00
    db  033h, 0dbh
    ; xor bx, bx                                ; 33 db
    int 013h                                  ; cd 13
    mov ax, strict word 00000h                ; b8 00 00
    sbb ax, strict byte 00000h                ; 83 d8 00
    test ax, ax                               ; 85 c0
    je short 04fach                           ; 74 03
    jmp near 04ed0h                           ; e9 24 ff
    cmp byte [bp-006h], 000h                  ; 80 7e fa 00
    je short 04fb6h                           ; 74 04
    xor bl, bl                                ; 30 db
    jmp short 04fb8h                          ; eb 02
    mov BL, strict byte 001h                  ; b3 01
    cmp byte [bp-008h], 000h                  ; 80 7e f8 00
    je short 04fc0h                           ; 74 02
    mov BL, strict byte 001h                  ; b3 01
    xor dx, dx                                ; 31 d2
    mov ax, si                                ; 89 f0
    call 0166eh                               ; e8 a7 c6
    mov di, ax                                ; 89 c7
    mov dx, strict word 00004h                ; ba 04 00
    mov ax, si                                ; 89 f0
    call 0166eh                               ; e8 9d c6
    cmp di, ax                                ; 39 c7
    je short 04fe6h                           ; 74 11
    test bl, bl                               ; 84 db
    jne short 04ffeh                          ; 75 25
    mov dx, 001feh                            ; ba fe 01
    mov ax, si                                ; 89 f0
    call 0166eh                               ; e8 8d c6
    cmp ax, 0aa55h                            ; 3d 55 aa
    je short 04ffeh                           ; 74 18
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov bl, byte [bp-006h]                    ; 8a 5e fa
    xor bh, bh                                ; 30 ff
    mov dl, byte [bp-00ah]                    ; 8a 56 f6
    xor dh, dh                                ; 30 f6
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor cx, cx                                ; 31 c9
    jmp near 04ee6h                           ; e9 e8 fe
    mov bl, byte [bp-006h]                    ; 8a 5e fa
    xor bh, bh                                ; 30 ff
    mov dl, byte [bp-00ah]                    ; 8a 56 f6
    xor dh, dh                                ; 30 f6
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    call 04c7fh                               ; e8 6f fc
    mov al, byte [bp-006h]                    ; 8a 46 fa
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    xor bx, bx                                ; 31 db
    xor al, al                                ; 30 c0
    add ax, si                                ; 01 f0
    adc dx, bx                                ; 11 da
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
keyboard_panic_:                             ; 0xf5026 LB 0x16
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push ax                                   ; 50
    mov ax, 00574h                            ; b8 74 05
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 41 c9
    add sp, strict byte 00006h                ; 83 c4 06
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
_keyboard_init:                              ; 0xf503c LB 0x26a
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov AL, strict byte 0aah                  ; b0 aa
    mov dx, strict word 00064h                ; ba 64 00
    out DX, AL                                ; ee
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 002h                 ; a8 02
    je short 0505fh                           ; 74 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 0505fh                          ; 76 08
    xor al, al                                ; 30 c0
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 05048h                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 05068h                          ; 75 05
    xor ax, ax                                ; 31 c0
    call 05026h                               ; e8 be ff
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 05082h                          ; 75 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 05082h                          ; 76 08
    mov AL, strict byte 001h                  ; b0 01
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 0506bh                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 0508ch                          ; 75 06
    mov ax, strict word 00001h                ; b8 01 00
    call 05026h                               ; e8 9a ff
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp ax, strict word 00055h                ; 3d 55 00
    je short 0509dh                           ; 74 06
    mov ax, 003dfh                            ; b8 df 03
    call 05026h                               ; e8 89 ff
    mov AL, strict byte 0abh                  ; b0 ab
    mov dx, strict word 00064h                ; ba 64 00
    out DX, AL                                ; ee
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 002h                 ; a8 02
    je short 050bdh                           ; 74 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 050bdh                          ; 76 08
    mov AL, strict byte 010h                  ; b0 10
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 050a6h                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 050c7h                          ; 75 06
    mov ax, strict word 0000ah                ; b8 0a 00
    call 05026h                               ; e8 5f ff
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 050e1h                          ; 75 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 050e1h                          ; 76 08
    mov AL, strict byte 011h                  ; b0 11
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 050cah                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 050ebh                          ; 75 06
    mov ax, strict word 0000bh                ; b8 0b 00
    call 05026h                               ; e8 3b ff
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test ax, ax                               ; 85 c0
    je short 050fbh                           ; 74 06
    mov ax, 003e0h                            ; b8 e0 03
    call 05026h                               ; e8 2b ff
    mov AL, strict byte 0ffh                  ; b0 ff
    mov dx, strict word 00060h                ; ba 60 00
    out DX, AL                                ; ee
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 002h                 ; a8 02
    je short 0511bh                           ; 74 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 0511bh                          ; 76 08
    mov AL, strict byte 020h                  ; b0 20
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 05104h                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 05125h                          ; 75 06
    mov ax, strict word 00014h                ; b8 14 00
    call 05026h                               ; e8 01 ff
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 0513fh                          ; 75 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 0513fh                          ; 76 08
    mov AL, strict byte 021h                  ; b0 21
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 05128h                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 05149h                          ; 75 06
    mov ax, strict word 00015h                ; b8 15 00
    call 05026h                               ; e8 dd fe
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp ax, 000fah                            ; 3d fa 00
    je short 0515ah                           ; 74 06
    mov ax, 003e1h                            ; b8 e1 03
    call 05026h                               ; e8 cc fe
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 0516ch                          ; 75 08
    mov AL, strict byte 031h                  ; b0 31
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 0515ah                          ; eb ee
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp ax, 000aah                            ; 3d aa 00
    je short 05185h                           ; 74 0e
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp ax, 000aah                            ; 3d aa 00
    je short 05185h                           ; 74 06
    mov ax, 003e2h                            ; b8 e2 03
    call 05026h                               ; e8 a1 fe
    mov AL, strict byte 0f5h                  ; b0 f5
    mov dx, strict word 00060h                ; ba 60 00
    out DX, AL                                ; ee
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 002h                 ; a8 02
    je short 051a5h                           ; 74 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 051a5h                          ; 76 08
    mov AL, strict byte 040h                  ; b0 40
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 0518eh                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 051afh                          ; 75 06
    mov ax, strict word 00028h                ; b8 28 00
    call 05026h                               ; e8 77 fe
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 051c9h                          ; 75 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 051c9h                          ; 76 08
    mov AL, strict byte 041h                  ; b0 41
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 051b2h                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 051d3h                          ; 75 06
    mov ax, strict word 00029h                ; b8 29 00
    call 05026h                               ; e8 53 fe
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp ax, 000fah                            ; 3d fa 00
    je short 051e4h                           ; 74 06
    mov ax, 003e3h                            ; b8 e3 03
    call 05026h                               ; e8 42 fe
    mov AL, strict byte 060h                  ; b0 60
    mov dx, strict word 00064h                ; ba 64 00
    out DX, AL                                ; ee
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 002h                 ; a8 02
    je short 05204h                           ; 74 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 05204h                          ; 76 08
    mov AL, strict byte 050h                  ; b0 50
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 051edh                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 0520eh                          ; 75 06
    mov ax, strict word 00032h                ; b8 32 00
    call 05026h                               ; e8 18 fe
    mov AL, strict byte 065h                  ; b0 65
    mov dx, strict word 00060h                ; ba 60 00
    out DX, AL                                ; ee
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 002h                 ; a8 02
    je short 0522eh                           ; 74 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 0522eh                          ; 76 08
    mov AL, strict byte 060h                  ; b0 60
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 05217h                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 05238h                          ; 75 06
    mov ax, strict word 0003ch                ; b8 3c 00
    call 05026h                               ; e8 ee fd
    mov AL, strict byte 0f4h                  ; b0 f4
    mov dx, strict word 00060h                ; ba 60 00
    out DX, AL                                ; ee
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 002h                 ; a8 02
    je short 05258h                           ; 74 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 05258h                          ; 76 08
    mov AL, strict byte 070h                  ; b0 70
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 05241h                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 05262h                          ; 75 06
    mov ax, strict word 00046h                ; b8 46 00
    call 05026h                               ; e8 c4 fd
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 0527ch                          ; 75 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 0527ch                          ; 76 08
    mov AL, strict byte 071h                  ; b0 71
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 05265h                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 05286h                          ; 75 06
    mov ax, strict word 00046h                ; b8 46 00
    call 05026h                               ; e8 a0 fd
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp ax, 000fah                            ; 3d fa 00
    je short 05297h                           ; 74 06
    mov ax, 003e4h                            ; b8 e4 03
    call 05026h                               ; e8 8f fd
    mov AL, strict byte 0a8h                  ; b0 a8
    mov dx, strict word 00064h                ; ba 64 00
    out DX, AL                                ; ee
    xor ax, ax                                ; 31 c0
    call 0679eh                               ; e8 fc 14
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
enqueue_key_:                                ; 0xf52a6 LB 0x9e
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    mov byte [bp-00ah], al                    ; 88 46 f6
    mov bl, dl                                ; 88 d3
    mov dx, strict word 0001ah                ; ba 1a 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 b2 c3
    mov di, ax                                ; 89 c7
    mov dx, strict word 0001ch                ; ba 1c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 a7 c3
    mov si, ax                                ; 89 c6
    lea cx, [si+002h]                         ; 8d 4c 02
    cmp cx, strict byte 0003eh                ; 83 f9 3e
    jc short 052d4h                           ; 72 03
    mov cx, strict word 0001eh                ; b9 1e 00
    cmp cx, di                                ; 39 f9
    jne short 052dch                          ; 75 04
    xor ax, ax                                ; 31 c0
    jmp short 05306h                          ; eb 2a
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, si                                ; 89 f2
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 76 c3
    mov bl, byte [bp-00ah]                    ; 8a 5e f6
    xor bh, bh                                ; 30 ff
    lea dx, [si+001h]                         ; 8d 54 01
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 68 c3
    mov bx, cx                                ; 89 cb
    mov dx, strict word 0001ch                ; ba 1c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0167ch                               ; e8 79 c3
    mov ax, strict word 00001h                ; b8 01 00
    lea sp, [bp-008h]                         ; 8d 66 f8
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
    db  0d4h, 0c6h, 0c5h, 0bah, 0b8h, 0b6h, 0aah, 09dh, 054h, 053h, 046h, 045h, 03ah, 038h, 036h, 02ah
    db  01dh, 08ah, 056h, 045h, 054h, 0e5h, 053h, 0e5h, 053h, 0d4h, 054h, 0b6h, 053h, 058h, 055h, 0c9h
    db  055h, 070h, 056h, 04eh, 056h, 08ch, 054h, 0e5h, 053h, 0e5h, 053h, 016h, 055h, 0d6h, 053h, 0a9h
    db  055h, 02eh, 056h, 069h, 056h
_int09_function:                             ; 0xf5344 LB 0x4cd
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push di                                   ; 57
    sub sp, strict byte 00010h                ; 83 ec 10
    mov al, byte [bp+014h]                    ; 8a 46 14
    mov byte [bp-004h], al                    ; 88 46 fc
    test al, al                               ; 84 c0
    jne short 05371h                          ; 75 1c
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 d3 c5
    mov ax, 00587h                            ; b8 87 05
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 0b c6
    add sp, strict byte 00004h                ; 83 c4 04
    jmp near 0543fh                           ; e9 ce 00
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 d8 c2
    mov byte [bp-006h], al                    ; 88 46 fa
    mov byte [bp-008h], al                    ; 88 46 f8
    mov dx, 00096h                            ; ba 96 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 c9 c2
    mov byte [bp-00ah], al                    ; 88 46 f6
    mov byte [bp-010h], al                    ; 88 46 f0
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 ba c2
    mov byte [bp-00ch], al                    ; 88 46 f4
    mov byte [bp-00eh], al                    ; 88 46 f2
    mov al, byte [bp-004h]                    ; 8a 46 fc
    push CS                                   ; 0e
    pop ES                                    ; 07
    mov cx, strict word 00012h                ; b9 12 00
    mov di, 0530fh                            ; bf 0f 53
    repne scasb                               ; f2 ae
    sal cx, 1                                 ; d1 e1
    mov di, cx                                ; 89 cf
    mov ax, word [cs:di+05320h]               ; 2e 8b 85 20 53
    jmp ax                                    ; ff e0
    xor byte [bp-00eh], 040h                  ; 80 76 f2 40
    mov bl, byte [bp-00eh]                    ; 8a 5e f2
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 98 c2
    or byte [bp-008h], 040h                   ; 80 4e f8 40
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    jmp near 0565eh                           ; e9 88 02
    mov al, byte [bp-006h]                    ; 8a 46 fa
    and AL, strict byte 0bfh                  ; 24 bf
    mov byte [bp-008h], al                    ; 88 46 f8
    mov bl, al                                ; 88 c3
    xor bh, bh                                ; 30 ff
    jmp near 0565eh                           ; e9 79 02
    test byte [bp-010h], 002h                 ; f6 46 f0 02
    jne short 0541eh                          ; 75 33
    mov al, byte [bp-004h]                    ; 8a 46 fc
    and AL, strict byte 07fh                  ; 24 7f
    cmp AL, strict byte 02ah                  ; 3c 2a
    jne short 053f9h                          ; 75 05
    mov bx, strict word 00002h                ; bb 02 00
    jmp short 053fch                          ; eb 03
    mov bx, strict word 00001h                ; bb 01 00
    test byte [bp-004h], 080h                 ; f6 46 fc 80
    je short 0540bh                           ; 74 09
    mov al, bl                                ; 88 d8
    not al                                    ; f6 d0
    and byte [bp-00eh], al                    ; 20 46 f2
    jmp short 0540eh                          ; eb 03
    or byte [bp-00eh], bl                     ; 08 5e f2
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 42 c2
    mov al, byte [bp-004h]                    ; 8a 46 fc
    and AL, strict byte 07fh                  ; 24 7f
    cmp AL, strict byte 01dh                  ; 3c 1d
    je short 0542bh                           ; 74 04
    and byte [bp-010h], 0feh                  ; 80 66 f0 fe
    and byte [bp-010h], 0fdh                  ; 80 66 f0 fd
    mov al, byte [bp-010h]                    ; 8a 46 f0
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, 00096h                            ; ba 96 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 21 c2
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop di                                    ; 5f
    pop bp                                    ; 5d
    retn                                      ; c3
    test byte [bp-00ah], 001h                 ; f6 46 f6 01
    jne short 0541eh                          ; 75 d3
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    or AL, strict byte 004h                   ; 0c 04
    mov byte [bp-00eh], al                    ; 88 46 f2
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 00 c2
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    test AL, strict byte 002h                 ; a8 02
    je short 05475h                           ; 74 0e
    or AL, strict byte 004h                   ; 0c 04
    mov byte [bp-010h], al                    ; 88 46 f0
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, 00096h                            ; ba 96 00
    jmp short 05484h                          ; eb 0f
    mov al, byte [bp-006h]                    ; 8a 46 fa
    or AL, strict byte 001h                   ; 0c 01
    mov byte [bp-008h], al                    ; 88 46 f8
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 d6 c1
    jmp short 0541eh                          ; eb 92
    test byte [bp-00ah], 001h                 ; f6 46 f6 01
    jne short 0541eh                          ; 75 8c
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    and AL, strict byte 0fbh                  ; 24 fb
    mov byte [bp-00eh], al                    ; 88 46 f2
    mov bl, al                                ; 88 c3
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 b9 c1
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    test AL, strict byte 002h                 ; a8 02
    je short 054bch                           ; 74 0e
    and AL, strict byte 0fbh                  ; 24 fb
    mov byte [bp-010h], al                    ; 88 46 f0
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, 00096h                            ; ba 96 00
    jmp short 054cbh                          ; eb 0f
    mov al, byte [bp-006h]                    ; 8a 46 fa
    and AL, strict byte 0feh                  ; 24 fe
    mov byte [bp-008h], al                    ; 88 46 f8
    mov bl, al                                ; 88 c3
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 8f c1
    jmp near 0541eh                           ; e9 4a ff
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    or AL, strict byte 008h                   ; 0c 08
    mov byte [bp-00eh], al                    ; 88 46 f2
    mov bl, al                                ; 88 c3
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 77 c1
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    test AL, strict byte 002h                 ; a8 02
    je short 054feh                           ; 74 0e
    or AL, strict byte 008h                   ; 0c 08
    mov byte [bp-010h], al                    ; 88 46 f0
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, 00096h                            ; ba 96 00
    jmp short 0550dh                          ; eb 0f
    mov al, byte [bp-006h]                    ; 8a 46 fa
    or AL, strict byte 002h                   ; 0c 02
    mov byte [bp-008h], al                    ; 88 46 f8
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 4d c1
    jmp near 0541eh                           ; e9 08 ff
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    and AL, strict byte 0f7h                  ; 24 f7
    mov byte [bp-00eh], al                    ; 88 46 f2
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 35 c1
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    test AL, strict byte 002h                 ; a8 02
    je short 05540h                           ; 74 0e
    and AL, strict byte 0f7h                  ; 24 f7
    mov byte [bp-010h], al                    ; 88 46 f0
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, 00096h                            ; ba 96 00
    jmp short 0554fh                          ; eb 0f
    mov al, byte [bp-006h]                    ; 8a 46 fa
    and AL, strict byte 0fdh                  ; 24 fd
    mov byte [bp-008h], al                    ; 88 46 f8
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 0b c1
    jmp near 0541eh                           ; e9 c6 fe
    test byte [bp-00ah], 003h                 ; f6 46 f6 03
    jne short 0557bh                          ; 75 1d
    mov al, byte [bp-006h]                    ; 8a 46 fa
    or AL, strict byte 020h                   ; 0c 20
    mov byte [bp-008h], al                    ; 88 46 f8
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 ed c0
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    xor AL, strict byte 020h                  ; 34 20
    jmp near 0561ch                           ; e9 a1 00
    mov al, byte [bp-006h]                    ; 8a 46 fa
    or AL, strict byte 008h                   ; 0c 08
    mov byte [bp-008h], al                    ; 88 46 f8
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 d0 c0
    mov AL, strict byte 0aeh                  ; b0 ae
    mov dx, strict word 00064h                ; ba 64 00
    out DX, AL                                ; ee
    call 0e034h                               ; e8 9b 8a
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 b0 c0
    test AL, strict byte 008h                 ; a8 08
    jne short 05599h                          ; 75 f3
    jmp near 0541eh                           ; e9 75 fe
    test byte [bp-00ah], 003h                 ; f6 46 f6 03
    je short 055b2h                           ; 74 03
    jmp near 0541eh                           ; e9 6c fe
    mov al, byte [bp-006h]                    ; 8a 46 fa
    and AL, strict byte 0dfh                  ; 24 df
    mov byte [bp-008h], al                    ; 88 46 f8
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 99 c0
    jmp short 055afh                          ; eb e6
    test byte [bp-00ah], 002h                 ; f6 46 f6 02
    je short 05602h                           ; 74 33
    mov dx, strict word 0001ah                ; ba 1a 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 96 c0
    mov bx, ax                                ; 89 c3
    mov dx, strict word 0001ch                ; ba 1c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0167ch                               ; e8 99 c0
    mov bx, 00080h                            ; bb 80 00
    mov dx, strict word 00071h                ; ba 71 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 71 c0
    mov AL, strict byte 0aeh                  ; b0 ae
    mov dx, strict word 00064h                ; ba 64 00
    out DX, AL                                ; ee
    push bp                                   ; 55
    int 01bh                                  ; cd 1b
    pop bp                                    ; 5d
    xor dx, dx                                ; 31 d2
    xor ax, ax                                ; 31 c0
    call 052a6h                               ; e8 a6 fc
    jmp short 055afh                          ; eb ad
    mov al, byte [bp-006h]                    ; 8a 46 fa
    or AL, strict byte 010h                   ; 0c 10
    mov byte [bp-008h], al                    ; 88 46 f8
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 49 c0
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    xor AL, strict byte 010h                  ; 34 10
    mov byte [bp-00eh], al                    ; 88 46 f2
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 34 c0
    jmp short 055afh                          ; eb 81
    test byte [bp-00ah], 002h                 ; f6 46 f6 02
    je short 05637h                           ; 74 03
    jmp near 0541eh                           ; e9 e7 fd
    mov al, byte [bp-006h]                    ; 8a 46 fa
    and AL, strict byte 0efh                  ; 24 ef
    mov byte [bp-008h], al                    ; 88 46 f8
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 14 c0
    jmp short 05634h                          ; eb e6
    mov al, byte [bp-006h]                    ; 8a 46 fa
    test AL, strict byte 004h                 ; a8 04
    jne short 05634h                          ; 75 df
    or AL, strict byte 004h                   ; 0c 04
    mov byte [bp-008h], al                    ; 88 46 f8
    mov bl, al                                ; 88 c3
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 f9 bf
    jmp short 05634h                          ; eb cb
    mov al, byte [bp-006h]                    ; 8a 46 fa
    and AL, strict byte 0fbh                  ; 24 fb
    jmp short 05657h                          ; eb e7
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    and AL, strict byte 00ch                  ; 24 0c
    cmp AL, strict byte 00ch                  ; 3c 0c
    jne short 0568ah                          ; 75 11
    mov bx, 01234h                            ; bb 34 12
    mov dx, strict word 00072h                ; ba 72 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0167ch                               ; e8 f7 bf
    jmp far 0f000h:0e05bh                     ; ea 5b e0 00 f0
    test byte [bp-008h], 008h                 ; f6 46 f8 08
    je short 056a1h                           ; 74 11
    and byte [bp-008h], 0f7h                  ; 80 66 f8 f7
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, strict word 00018h                ; ba 18 00
    jmp near 05439h                           ; e9 98 fd
    mov al, byte [bp-004h]                    ; 8a 46 fc
    test AL, strict byte 080h                 ; a8 80
    je short 056dfh                           ; 74 37
    cmp AL, strict byte 0fah                  ; 3c fa
    jne short 056beh                          ; 75 12
    mov dx, 00097h                            ; ba 97 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 9d bf
    mov bl, al                                ; 88 c3
    or bl, 010h                               ; 80 cb 10
    xor bh, bh                                ; 30 ff
    jmp short 056d4h                          ; eb 16
    cmp AL, strict byte 0feh                  ; 3c fe
    je short 056c5h                           ; 74 03
    jmp near 0541eh                           ; e9 59 fd
    mov dx, 00097h                            ; ba 97 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 84 bf
    or AL, strict byte 020h                   ; 0c 20
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov dx, 00097h                            ; ba 97 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 83 bf
    jmp short 056c2h                          ; eb e3
    cmp byte [bp-004h], 058h                  ; 80 7e fc 58
    jbe short 05707h                          ; 76 22
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 43 c2
    mov al, byte [bp-004h]                    ; 8a 46 fc
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 005a1h                            ; b8 a1 05
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 75 c2
    add sp, strict byte 00006h                ; 83 c4 06
    jmp near 0543fh                           ; e9 38 fd
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    test AL, strict byte 008h                 ; a8 08
    je short 05728h                           ; 74 1a
    mov al, byte [bp-004h]                    ; 8a 46 fc
    xor ah, ah                                ; 30 e4
    mov bx, strict word 0000ah                ; bb 0a 00
    imul bx                                   ; f7 eb
    mov bx, ax                                ; 89 c3
    mov al, byte [bx+00df4h]                  ; 8a 87 f4 0d
    mov byte [bp-012h], al                    ; 88 46 ee
    mov al, byte [bx+00df5h]                  ; 8a 87 f5 0d
    jmp near 057dah                           ; e9 b2 00
    test AL, strict byte 004h                 ; a8 04
    je short 05746h                           ; 74 1a
    mov al, byte [bp-004h]                    ; 8a 46 fc
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0000ah                ; ba 0a 00
    imul dx                                   ; f7 ea
    mov bx, ax                                ; 89 c3
    mov al, byte [bx+00df2h]                  ; 8a 87 f2 0d
    mov byte [bp-012h], al                    ; 88 46 ee
    mov al, byte [bx+00df3h]                  ; 8a 87 f3 0d
    jmp near 057dah                           ; e9 94 00
    mov al, byte [bp-010h]                    ; 8a 46 f0
    and AL, strict byte 002h                  ; 24 02
    test al, al                               ; 84 c0
    jbe short 05769h                          ; 76 1a
    mov al, byte [bp-004h]                    ; 8a 46 fc
    cmp AL, strict byte 047h                  ; 3c 47
    jc short 05769h                           ; 72 13
    cmp AL, strict byte 053h                  ; 3c 53
    jnbe short 05769h                         ; 77 0f
    mov byte [bp-012h], 0e0h                  ; c6 46 ee e0
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0000ah                ; ba 0a 00
    imul dx                                   ; f7 ea
    mov bx, ax                                ; 89 c3
    jmp short 057d6h                          ; eb 6d
    test byte [bp-00eh], 003h                 ; f6 46 f2 03
    je short 057a7h                           ; 74 38
    mov al, byte [bp-004h]                    ; 8a 46 fc
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0000ah                ; ba 0a 00
    imul dx                                   ; f7 ea
    mov bx, ax                                ; 89 c3
    mov al, byte [bx+00df6h]                  ; 8a 87 f6 0d
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    test ax, dx                               ; 85 d0
    je short 05797h                           ; 74 0d
    mov al, byte [bx+00deeh]                  ; 8a 87 ee 0d
    mov byte [bp-012h], al                    ; 88 46 ee
    mov al, byte [bx+00defh]                  ; 8a 87 ef 0d
    jmp short 057a2h                          ; eb 0b
    mov al, byte [bx+00df0h]                  ; 8a 87 f0 0d
    mov byte [bp-012h], al                    ; 88 46 ee
    mov al, byte [bx+00df1h]                  ; 8a 87 f1 0d
    mov byte [bp-004h], al                    ; 88 46 fc
    jmp short 057ddh                          ; eb 36
    mov al, byte [bp-004h]                    ; 8a 46 fc
    xor ah, ah                                ; 30 e4
    mov bx, strict word 0000ah                ; bb 0a 00
    imul bx                                   ; f7 eb
    mov bx, ax                                ; 89 c3
    mov al, byte [bx+00df6h]                  ; 8a 87 f6 0d
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    test ax, dx                               ; 85 d0
    je short 057cfh                           ; 74 0d
    mov al, byte [bx+00df0h]                  ; 8a 87 f0 0d
    mov byte [bp-012h], al                    ; 88 46 ee
    mov al, byte [bx+00df1h]                  ; 8a 87 f1 0d
    jmp short 057dah                          ; eb 0b
    mov al, byte [bx+00deeh]                  ; 8a 87 ee 0d
    mov byte [bp-012h], al                    ; 88 46 ee
    mov al, byte [bx+00defh]                  ; 8a 87 ef 0d
    mov byte [bp-004h], al                    ; 88 46 fc
    cmp byte [bp-004h], 000h                  ; 80 7e fc 00
    jne short 05802h                          ; 75 1f
    cmp byte [bp-012h], 000h                  ; 80 7e ee 00
    jne short 05802h                          ; 75 19
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 3f c1
    mov ax, 005d8h                            ; b8 d8 05
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 77 c1
    add sp, strict byte 00004h                ; 83 c4 04
    mov bl, byte [bp-012h]                    ; 8a 5e ee
    xor bh, bh                                ; 30 ff
    mov al, byte [bp-004h]                    ; 8a 46 fc
    xor ah, ah                                ; 30 e4
    mov dx, bx                                ; 89 da
    jmp near 055fdh                           ; e9 ec fd
dequeue_key_:                                ; 0xf5811 LB 0x94
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    push ax                                   ; 50
    mov di, ax                                ; 89 c7
    mov word [bp-006h], dx                    ; 89 56 fa
    mov si, bx                                ; 89 de
    mov word [bp-008h], cx                    ; 89 4e f8
    mov dx, strict word 0001ah                ; ba 1a 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 43 be
    mov bx, ax                                ; 89 c3
    mov dx, strict word 0001ch                ; ba 1c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 38 be
    cmp bx, ax                                ; 39 c3
    je short 05877h                           ; 74 3d
    mov dx, bx                                ; 89 da
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 10 be
    mov cl, al                                ; 88 c1
    lea dx, [bx+001h]                         ; 8d 57 01
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 05 be
    mov es, [bp-008h]                         ; 8e 46 f8
    mov byte [es:si], cl                      ; 26 88 0c
    mov es, [bp-006h]                         ; 8e 46 fa
    mov byte [es:di], al                      ; 26 88 05
    cmp word [bp+004h], strict byte 00000h    ; 83 7e 04 00
    je short 05872h                           ; 74 13
    inc bx                                    ; 43
    inc bx                                    ; 43
    cmp bx, strict byte 0003eh                ; 83 fb 3e
    jc short 05869h                           ; 72 03
    mov bx, strict word 0001eh                ; bb 1e 00
    mov dx, strict word 0001ah                ; ba 1a 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0167ch                               ; e8 0a be
    mov ax, strict word 00001h                ; b8 01 00
    jmp short 05879h                          ; eb 02
    xor ax, ax                                ; 31 c0
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 00002h                               ; c2 02 00
    mov byte [01292h], AL                     ; a2 92 12
    adc word [bx+si], dx                      ; 11 10
    or cl, byte [bx+di]                       ; 0a 09
    add ax, 00102h                            ; 05 02 01
    add byte [di+06859h], ch                  ; 00 ad 59 68
    pop cx                                    ; 59
    std                                       ; fd
    pop cx                                    ; 59
    inc dx                                    ; 42
    pop dx                                    ; 5a
    push bp                                   ; 55
    pop dx                                    ; 5a
    jl short 058f3h                           ; 7c 5a
    xchg byte [bp+si-00bh], bl                ; 86 5a f5
    pop dx                                    ; 5a
    sub AL, strict byte 05bh                  ; 2c 5b
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    xchg sp, ax                               ; 94
    pop bx                                    ; 5b
    db  0f7h
    pop cx                                    ; 59
_int16_function:                             ; 0xf58a5 LB 0x2fa
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push di                                   ; 57
    sub sp, strict byte 00006h                ; 83 ec 06
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 9d bd
    mov cl, al                                ; 88 c1
    mov bl, al                                ; 88 c3
    mov dx, 00097h                            ; ba 97 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 90 bd
    mov bh, al                                ; 88 c7
    mov dl, cl                                ; 88 ca
    xor dh, dh                                ; 30 f6
    mov CL, strict byte 004h                  ; b1 04
    sar dx, CL                                ; d3 fa
    and dl, 007h                              ; 80 e2 07
    and AL, strict byte 007h                  ; 24 07
    xor ah, ah                                ; 30 e4
    xor al, dl                                ; 30 d0
    test ax, ax                               ; 85 c0
    je short 05947h                           ; 74 6e
    cli                                       ; fa
    mov AL, strict byte 0edh                  ; b0 ed
    mov dx, strict word 00060h                ; ba 60 00
    out DX, AL                                ; ee
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 058f2h                          ; 75 08
    mov AL, strict byte 021h                  ; b0 21
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 058e0h                          ; eb ee
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp ax, 000fah                            ; 3d fa 00
    jne short 05946h                          ; 75 49
    and bh, 0c8h                              ; 80 e7 c8
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 004h                  ; b1 04
    sar ax, CL                                ; d3 f8
    mov cx, ax                                ; 89 c1
    xor ch, ah                                ; 30 e5
    and cl, 007h                              ; 80 e1 07
    mov al, bh                                ; 88 f8
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    or dx, cx                                 ; 09 ca
    mov bh, dl                                ; 88 d7
    mov al, dl                                ; 88 d0
    and AL, strict byte 007h                  ; 24 07
    mov dx, strict word 00060h                ; ba 60 00
    out DX, AL                                ; ee
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 05933h                          ; 75 08
    mov AL, strict byte 021h                  ; b0 21
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 05921h                          ; eb ee
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov bl, bh                                ; 88 fb
    xor bh, bh                                ; 30 ff
    mov dx, 00097h                            ; ba 97 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 1a bd
    sti                                       ; fb
    mov CL, strict byte 008h                  ; b1 08
    mov ax, word [bp+012h]                    ; 8b 46 12
    shr ax, CL                                ; d3 e8
    cmp ax, 000a2h                            ; 3d a2 00
    jnbe short 059adh                         ; 77 5a
    push CS                                   ; 0e
    pop ES                                    ; 07
    mov cx, strict word 0000ch                ; b9 0c 00
    mov di, 05882h                            ; bf 82 58
    repne scasb                               ; f2 ae
    sal cx, 1                                 ; d1 e1
    mov di, cx                                ; 89 cf
    mov ax, word [cs:di+0588dh]               ; 2e 8b 85 8d 58
    jmp ax                                    ; ff e0
    mov ax, strict word 00001h                ; b8 01 00
    push ax                                   ; 50
    mov cx, ss                                ; 8c d1
    lea bx, [bp-008h]                         ; 8d 5e f8
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 05811h                               ; e8 98 fe
    test ax, ax                               ; 85 c0
    jne short 0598bh                          ; 75 0e
    mov ax, 0060fh                            ; b8 0f 06
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 ee bf
    add sp, strict byte 00004h                ; 83 c4 04
    cmp byte [bp-006h], 000h                  ; 80 7e fa 00
    je short 05997h                           ; 74 06
    cmp byte [bp-008h], 0f0h                  ; 80 7e f8 f0
    je short 0599dh                           ; 74 06
    cmp byte [bp-008h], 0e0h                  ; 80 7e f8 e0
    jne short 059a1h                          ; 75 04
    mov byte [bp-008h], 000h                  ; c6 46 f8 00
    mov ah, byte [bp-006h]                    ; 8a 66 fa
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov word [bp+012h], ax                    ; 89 46 12
    jmp near 059f7h                           ; e9 4a 00
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 7b bf
    mov CL, strict byte 008h                  ; b1 08
    mov ax, word [bp+012h]                    ; 8b 46 12
    shr ax, CL                                ; d3 e8
    push ax                                   ; 50
    mov ax, 00633h                            ; b8 33 06
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 ab bf
    add sp, strict byte 00006h                ; 83 c4 06
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 5a bf
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    push ax                                   ; 50
    mov ax, word [bp+010h]                    ; 8b 46 10
    push ax                                   ; 50
    mov ax, word [bp+00ch]                    ; 8b 46 0c
    push ax                                   ; 50
    mov ax, word [bp+012h]                    ; 8b 46 12
    push ax                                   ; 50
    mov ax, 0065bh                            ; b8 5b 06
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 82 bf
    add sp, strict byte 0000ch                ; 83 c4 0c
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop di                                    ; 5f
    pop bp                                    ; 5d
    retn                                      ; c3
    mov ax, word [bp+018h]                    ; 8b 46 18
    mov word [bp+01eh], ax                    ; 89 46 1e
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    mov cx, ss                                ; 8c d1
    lea bx, [bp-008h]                         ; 8d 5e f8
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 05811h                               ; e8 fe fd
    test ax, ax                               ; 85 c0
    jne short 05a1dh                          ; 75 06
    or word [bp+01eh], strict byte 00040h     ; 83 4e 1e 40
    jmp short 059f7h                          ; eb da
    cmp byte [bp-006h], 000h                  ; 80 7e fa 00
    je short 05a29h                           ; 74 06
    cmp byte [bp-008h], 0f0h                  ; 80 7e f8 f0
    je short 05a2fh                           ; 74 06
    cmp byte [bp-008h], 0e0h                  ; 80 7e f8 e0
    jne short 05a33h                          ; 75 04
    mov byte [bp-008h], 000h                  ; c6 46 f8 00
    mov dh, byte [bp-006h]                    ; 8a 76 fa
    mov dl, byte [bp-008h]                    ; 8a 56 f8
    mov word [bp+012h], dx                    ; 89 56 12
    and word [bp+01eh], strict byte 0ffbfh    ; 83 66 1e bf
    jmp short 059f7h                          ; eb b5
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 07 bc
    mov dx, word [bp+012h]                    ; 8b 56 12
    mov dl, al                                ; 88 c2
    mov word [bp+012h], dx                    ; 89 56 12
    jmp short 059f7h                          ; eb a2
    mov dl, byte [bp+010h]                    ; 8a 56 10
    xor dh, dh                                ; 30 f6
    mov CL, strict byte 008h                  ; b1 08
    mov ax, word [bp+010h]                    ; 8b 46 10
    shr ax, CL                                ; d3 e8
    xor ah, ah                                ; 30 e4
    call 052a6h                               ; e8 40 f8
    test ax, ax                               ; 85 c0
    jne short 05a74h                          ; 75 0a
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor al, al                                ; 30 c0
    or AL, strict byte 001h                   ; 0c 01
    jmp near 059a7h                           ; e9 33 ff
    and word [bp+012h], 0ff00h                ; 81 66 12 00 ff
    jmp near 059f7h                           ; e9 7b ff
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor al, al                                ; 30 c0
    or AL, strict byte 030h                   ; 0c 30
    jmp near 059a7h                           ; e9 21 ff
    mov byte [bp-004h], 002h                  ; c6 46 fc 02
    xor cx, cx                                ; 31 c9
    cli                                       ; fa
    mov AL, strict byte 0f2h                  ; b0 f2
    mov dx, strict word 00060h                ; ba 60 00
    out DX, AL                                ; ee
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 05aadh                          ; 75 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 05aadh                          ; 76 08
    mov dx, 00080h                            ; ba 80 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    jmp short 05a96h                          ; eb e9
    test bx, bx                               ; 85 db
    jbe short 05aefh                          ; 76 3e
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp ax, 000fah                            ; 3d fa 00
    jne short 05aefh                          ; 75 33
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 05ad6h                          ; 75 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 05ad6h                          ; 76 08
    mov dx, 00080h                            ; ba 80 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    jmp short 05abfh                          ; eb e9
    test bx, bx                               ; 85 db
    jbe short 05ae6h                          ; 76 0c
    mov bl, ch                                ; 88 eb
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov ch, al                                ; 88 c5
    mov cl, bl                                ; 88 d9
    dec byte [bp-004h]                        ; fe 4e fc
    cmp byte [bp-004h], 000h                  ; 80 7e fc 00
    jnbe short 05abch                         ; 77 cd
    mov word [bp+00ch], cx                    ; 89 4e 0c
    jmp near 059f7h                           ; e9 02 ff
    mov ax, strict word 00001h                ; b8 01 00
    push ax                                   ; 50
    mov cx, ss                                ; 8c d1
    lea bx, [bp-008h]                         ; 8d 5e f8
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 05811h                               ; e8 0b fd
    test ax, ax                               ; 85 c0
    jne short 05b18h                          ; 75 0e
    mov ax, 0060fh                            ; b8 0f 06
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 61 be
    add sp, strict byte 00004h                ; 83 c4 04
    cmp byte [bp-006h], 000h                  ; 80 7e fa 00
    jne short 05b21h                          ; 75 03
    jmp near 059a1h                           ; e9 80 fe
    cmp byte [bp-008h], 0f0h                  ; 80 7e f8 f0
    jne short 05b2ah                          ; 75 03
    jmp near 0599dh                           ; e9 73 fe
    jmp short 05b1eh                          ; eb f2
    mov ax, word [bp+018h]                    ; 8b 46 18
    mov word [bp+01eh], ax                    ; 89 46 1e
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    mov cx, ss                                ; 8c d1
    lea bx, [bp-008h]                         ; 8d 5e f8
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 05811h                               ; e8 cf fc
    test ax, ax                               ; 85 c0
    jne short 05b49h                          ; 75 03
    jmp near 05a17h                           ; e9 ce fe
    cmp byte [bp-006h], 000h                  ; 80 7e fa 00
    jne short 05b52h                          ; 75 03
    jmp near 05a33h                           ; e9 e1 fe
    cmp byte [bp-008h], 0f0h                  ; 80 7e f8 f0
    jne short 05b5bh                          ; 75 03
    jmp near 05a2fh                           ; e9 d4 fe
    jmp short 05b4fh                          ; eb f2
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 ec ba
    mov dx, word [bp+012h]                    ; 8b 56 12
    mov dl, al                                ; 88 c2
    mov word [bp+012h], dx                    ; 89 56 12
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 db ba
    mov bl, al                                ; 88 c3
    and bl, 073h                              ; 80 e3 73
    mov dx, 00096h                            ; ba 96 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 cd ba
    mov ah, al                                ; 88 c4
    and ah, 00ch                              ; 80 e4 0c
    or ah, bl                                 ; 08 dc
    mov dx, word [bp+012h]                    ; 8b 56 12
    mov dh, ah                                ; 88 e6
    jmp near 05a50h                           ; e9 bc fe
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    or ah, 080h                               ; 80 cc 80
    jmp near 059a7h                           ; e9 08 fe
set_geom_lba_:                               ; 0xf5b9f LB 0xeb
    push bx                                   ; 53
    push cx                                   ; 51
    push si                                   ; 56
    push di                                   ; 57
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    sub sp, strict byte 00008h                ; 83 ec 08
    mov di, ax                                ; 89 c7
    mov es, dx                                ; 8e c2
    mov word [bp-008h], strict word 00000h    ; c7 46 f8 00 00
    mov word [bp-006h], strict word 0007eh    ; c7 46 fa 7e 00
    mov word [bp-002h], 000ffh                ; c7 46 fe ff 00
    mov ax, word [bp+012h]                    ; 8b 46 12
    mov bx, word [bp+010h]                    ; 8b 5e 10
    mov cx, word [bp+00eh]                    ; 8b 4e 0e
    mov dx, word [bp+00ch]                    ; 8b 56 0c
    mov si, strict word 00020h                ; be 20 00
    call 0a1a0h                               ; e8 d2 45
    test ax, ax                               ; 85 c0
    jne short 05bdeh                          ; 75 0c
    test bx, bx                               ; 85 db
    jne short 05bdeh                          ; 75 08
    test cx, cx                               ; 85 c9
    jne short 05bdeh                          ; 75 04
    test dx, dx                               ; 85 d2
    je short 05be5h                           ; 74 07
    mov bx, strict word 0ffffh                ; bb ff ff
    mov si, bx                                ; 89 de
    jmp short 05bebh                          ; eb 06
    mov bx, word [bp+00ch]                    ; 8b 5e 0c
    mov si, word [bp+00eh]                    ; 8b 76 0e
    mov word [bp-004h], bx                    ; 89 5e fc
    xor bx, bx                                ; 31 db
    jmp short 05bf7h                          ; eb 05
    cmp bx, strict byte 00004h                ; 83 fb 04
    jnl short 05c1ah                          ; 7d 23
    mov ax, word [bp-006h]                    ; 8b 46 fa
    cmp si, ax                                ; 39 c6
    jc short 05c08h                           ; 72 0a
    jne short 05c11h                          ; 75 11
    mov ax, word [bp-004h]                    ; 8b 46 fc
    cmp ax, word [bp-008h]                    ; 3b 46 f8
    jnbe short 05c11h                         ; 77 09
    mov ax, word [bp-002h]                    ; 8b 46 fe
    inc ax                                    ; 40
    shr ax, 1                                 ; d1 e8
    mov word [bp-002h], ax                    ; 89 46 fe
    shr word [bp-006h], 1                     ; d1 6e fa
    rcr word [bp-008h], 1                     ; d1 5e f8
    inc bx                                    ; 43
    jmp short 05bf2h                          ; eb d8
    mov ax, word [bp-002h]                    ; 8b 46 fe
    xor dx, dx                                ; 31 d2
    mov bx, strict word 0003fh                ; bb 3f 00
    xor cx, cx                                ; 31 c9
    call 0a150h                               ; e8 29 45
    mov bx, ax                                ; 89 c3
    mov cx, dx                                ; 89 d1
    mov ax, word [bp-004h]                    ; 8b 46 fc
    mov dx, si                                ; 89 f2
    call 0a170h                               ; e8 3d 45
    mov word [es:di+002h], ax                 ; 26 89 45 02
    cmp ax, 00400h                            ; 3d 00 04
    jbe short 05c42h                          ; 76 06
    mov word [es:di+002h], 00400h             ; 26 c7 45 02 00 04
    mov ax, word [bp-002h]                    ; 8b 46 fe
    mov word [es:di], ax                      ; 26 89 05
    mov word [es:di+004h], strict word 0003fh ; 26 c7 45 04 3f 00
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bx                                    ; 5b
    retn 00008h                               ; c2 08 00
    push SS                                   ; 16
    pop bp                                    ; 5d
    daa                                       ; 27
    pop bp                                    ; 5d
    dec di                                    ; 4f
    pop bp                                    ; 5d
    dec di                                    ; 4f
    pop bp                                    ; 5d
    dec di                                    ; 4f
    pop bp                                    ; 5d
    cmp ax, 0795fh                            ; 3d 5f 79
    pushaw                                    ; 60
    jns short 05cc8h                          ; 79 60
    insw                                      ; 6d
    pop di                                    ; 5f
    push bx                                   ; 53
    pushaw                                    ; 60
    jns short 05cceh                          ; 79 60
    jns short 05cd0h                          ; 79 60
    push bx                                   ; 53
    pushaw                                    ; 60
    push bx                                   ; 53
    pushaw                                    ; 60
    jns short 05cd6h                          ; 79 60
    jns short 05cd8h                          ; 79 60
    rcr word [bx+053h], CL                    ; d3 5f 53
    pushaw                                    ; 60
    jns short 05cdeh                          ; 79 60
    jns short 05ce0h                          ; 79 60
    push bx                                   ; 53
    pushaw                                    ; 60
    add AL, strict byte 060h                  ; 04 60
    jns short 05ce6h                          ; 79 60
    jns short 05ce8h                          ; 79 60
    jns short 05ceah                          ; 79 60
_int13_harddisk:                             ; 0xf5c8a LB 0x44a
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    sub sp, strict byte 00010h                ; 83 ec 10
    or byte [bp+01dh], 002h                   ; 80 4e 1d 02
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 d1 b9
    mov si, 00122h                            ; be 22 01
    mov word [bp-006h], ax                    ; 89 46 fa
    xor bx, bx                                ; 31 db
    mov dx, 0008eh                            ; ba 8e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 b2 b9
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    xor ah, ah                                ; 30 e4
    cmp ax, 00080h                            ; 3d 80 00
    jc short 05cbdh                           ; 72 05
    cmp ax, 00090h                            ; 3d 90 00
    jc short 05cdch                           ; 72 1f
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov al, byte [bp+017h]                    ; 8a 46 17
    push ax                                   ; 50
    mov ax, 0067eh                            ; b8 7e 06
    push ax                                   ; 50
    mov ax, 0068dh                            ; b8 8d 06
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 a0 bc
    add sp, strict byte 0000ah                ; 83 c4 0a
    jmp near 06094h                           ; e9 b8 03
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    xor ah, ah                                ; 30 e4
    mov es, [bp-006h]                         ; 8e 46 fa
    mov bx, si                                ; 89 f3
    add bx, ax                                ; 01 c3
    mov dl, byte [es:bx+00163h]               ; 26 8a 97 63 01
    mov byte [bp-002h], dl                    ; 88 56 fe
    cmp dl, 010h                              ; 80 fa 10
    jc short 05d03h                           ; 72 0e
    push ax                                   ; 50
    mov al, byte [bp+017h]                    ; 8a 46 17
    push ax                                   ; 50
    mov ax, 0067eh                            ; b8 7e 06
    push ax                                   ; 50
    mov ax, 006b8h                            ; b8 b8 06
    jmp short 05cceh                          ; eb cb
    mov al, byte [bp+017h]                    ; 8a 46 17
    xor ah, ah                                ; 30 e4
    cmp ax, strict word 00018h                ; 3d 18 00
    jnbe short 05d4ch                         ; 77 3f
    mov bx, ax                                ; 89 c3
    sal bx, 1                                 ; d1 e3
    jmp word [cs:bx+05c58h]                   ; 2e ff a7 58 5c
    cmp byte [bp-002h], 008h                  ; 80 7e fe 08
    jnc short 05d24h                          ; 73 08
    mov al, byte [bp-002h]                    ; 8a 46 fe
    xor ah, ah                                ; 30 e4
    call 01db8h                               ; e8 94 c0
    jmp near 05f56h                           ; e9 2f 02
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 22 b9
    mov cl, al                                ; 88 c1
    mov dx, word [bp+016h]                    ; 8b 56 16
    mov dh, al                                ; 88 c6
    mov word [bp+016h], dx                    ; 89 56 16
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 1b b9
    test cl, cl                               ; 84 c9
    je short 05dabh                           ; 74 62
    jmp near 060adh                           ; e9 61 03
    jmp near 06079h                           ; e9 2a 03
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    mov word [bp-00ah], ax                    ; 89 46 f6
    mov al, byte [bp+015h]                    ; 8a 46 15
    mov dx, word [bp+014h]                    ; 8b 56 14
    xor dh, dh                                ; 30 f6
    sal dx, 1                                 ; d1 e2
    sal dx, 1                                 ; d1 e2
    and dh, 003h                              ; 80 e6 03
    mov ah, dh                                ; 88 f4
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov di, word [bp+014h]                    ; 8b 7e 14
    and di, strict byte 0003fh                ; 83 e7 3f
    mov al, byte [bp+013h]                    ; 8a 46 13
    xor ah, dh                                ; 30 f4
    mov word [bp-008h], ax                    ; 89 46 f8
    mov ax, word [bp-00ah]                    ; 8b 46 f6
    cmp ax, 00080h                            ; 3d 80 00
    jnbe short 05d85h                         ; 77 04
    test ax, ax                               ; 85 c0
    jne short 05daeh                          ; 75 29
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 a3 bb
    mov al, byte [bp+017h]                    ; 8a 46 17
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 0067eh                            ; b8 7e 06
    push ax                                   ; 50
    mov ax, 006eah                            ; b8 ea 06
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 d1 bb
    add sp, strict byte 00008h                ; 83 c4 08
    jmp near 06094h                           ; e9 e9 02
    jmp near 05f5ah                           ; e9 ac 01
    mov al, byte [bp-002h]                    ; 8a 46 fe
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov es, [bp-006h]                         ; 8e 46 fa
    mov bx, si                                ; 89 f3
    add bx, ax                                ; 01 c3
    mov ax, word [es:bx+02ch]                 ; 26 8b 47 2c
    mov cx, word [es:bx+02ah]                 ; 26 8b 4f 2a
    mov dx, word [es:bx+02eh]                 ; 26 8b 57 2e
    mov word [bp-00eh], dx                    ; 89 56 f2
    cmp ax, word [bp-00ch]                    ; 3b 46 f4
    jbe short 05ddch                          ; 76 09
    cmp cx, word [bp-008h]                    ; 3b 4e f8
    jbe short 05ddch                          ; 76 04
    cmp di, dx                                ; 39 d7
    jbe short 05e0dh                          ; 76 31
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 4c bb
    push di                                   ; 57
    push word [bp-008h]                       ; ff 76 f8
    push word [bp-00ch]                       ; ff 76 f4
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov al, byte [bp+017h]                    ; 8a 46 17
    push ax                                   ; 50
    mov ax, 0067eh                            ; b8 7e 06
    push ax                                   ; 50
    mov ax, 00712h                            ; b8 12 07
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 6f bb
    add sp, strict byte 00010h                ; 83 c4 10
    jmp near 06094h                           ; e9 87 02
    mov al, byte [bp+017h]                    ; 8a 46 17
    xor ah, ah                                ; 30 e4
    cmp ax, strict word 00004h                ; 3d 04 00
    je short 05e37h                           ; 74 20
    mov al, byte [bp-002h]                    ; 8a 46 fe
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov es, [bp-006h]                         ; 8e 46 fa
    mov bx, si                                ; 89 f3
    add bx, ax                                ; 01 c3
    cmp cx, word [es:bx+030h]                 ; 26 3b 4f 30
    jne short 05e40h                          ; 75 14
    mov ax, word [es:bx+034h]                 ; 26 8b 47 34
    cmp ax, word [bp-00eh]                    ; 3b 46 f2
    je short 05e3ah                           ; 74 05
    jmp short 05e40h                          ; eb 09
    jmp near 05f56h                           ; e9 1c 01
    cmp byte [bp-002h], 008h                  ; 80 7e fe 08
    jc short 05e6fh                           ; 72 2f
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    xor dx, dx                                ; 31 d2
    mov bx, cx                                ; 89 cb
    xor cx, cx                                ; 31 c9
    call 0a150h                               ; e8 04 43
    xor bx, bx                                ; 31 db
    add ax, word [bp-008h]                    ; 03 46 f8
    adc dx, bx                                ; 11 da
    mov bx, word [bp-00eh]                    ; 8b 5e f2
    xor cx, cx                                ; 31 c9
    call 0a150h                               ; e8 f5 42
    xor bx, bx                                ; 31 db
    add ax, di                                ; 01 f8
    adc dx, bx                                ; 11 da
    add ax, strict word 0ffffh                ; 05 ff ff
    mov word [bp-010h], ax                    ; 89 46 f0
    adc dx, strict byte 0ffffh                ; 83 d2 ff
    mov word [bp-004h], dx                    ; 89 56 fc
    xor di, di                                ; 31 ff
    mov es, [bp-006h]                         ; 8e 46 fa
    mov word [es:si+018h], strict word 00000h ; 26 c7 44 18 00 00
    mov word [es:si+01ah], strict word 00000h ; 26 c7 44 1a 00 00
    mov word [es:si+01ch], strict word 00000h ; 26 c7 44 1c 00 00
    mov ax, word [bp-010h]                    ; 8b 46 f0
    mov word [es:si], ax                      ; 26 89 04
    mov ax, word [bp-004h]                    ; 8b 46 fc
    mov word [es:si+002h], ax                 ; 26 89 44 02
    mov word [es:si+004h], strict word 00000h ; 26 c7 44 04 00 00
    mov word [es:si+006h], strict word 00000h ; 26 c7 44 06 00 00
    mov dx, word [bp+010h]                    ; 8b 56 10
    mov ax, word [bp+006h]                    ; 8b 46 06
    mov word [es:si+008h], dx                 ; 26 89 54 08
    mov word [es:si+00ah], ax                 ; 26 89 44 0a
    mov ax, word [bp-00ah]                    ; 8b 46 f6
    mov word [es:si+00eh], ax                 ; 26 89 44 0e
    mov word [es:si+010h], 00200h             ; 26 c7 44 10 00 02
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    mov word [es:si+012h], ax                 ; 26 89 44 12
    mov ax, word [bp-008h]                    ; 8b 46 f8
    mov word [es:si+014h], ax                 ; 26 89 44 14
    mov word [es:si+016h], di                 ; 26 89 7c 16
    mov al, byte [bp-002h]                    ; 8a 46 fe
    mov byte [es:si+00ch], al                 ; 26 88 44 0c
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov bx, si                                ; 89 f3
    add bx, ax                                ; 01 c3
    mov al, byte [es:bx+022h]                 ; 26 8a 47 22
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    sal bx, 1                                 ; d1 e3
    sal bx, 1                                 ; d1 e3
    mov al, byte [bp+017h]                    ; 8a 46 17
    sal ax, 1                                 ; d1 e0
    add bx, ax                                ; 01 c3
    push ES                                   ; 06
    push si                                   ; 56
    call word [word bx+0007eh]                ; ff 97 7e 00
    mov dx, ax                                ; 89 c2
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor al, al                                ; 30 c0
    mov es, [bp-006h]                         ; 8e 46 fa
    mov bx, word [es:si+018h]                 ; 26 8b 5c 18
    or bx, ax                                 ; 09 c3
    mov word [bp+016h], bx                    ; 89 5e 16
    test dl, dl                               ; 84 d2
    je short 05f56h                           ; 74 4a
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 1c ba
    mov al, dl                                ; 88 d0
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov al, byte [bp+017h]                    ; 8a 46 17
    push ax                                   ; 50
    mov ax, 0067eh                            ; b8 7e 06
    push ax                                   ; 50
    mov ax, 00759h                            ; b8 59 07
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 47 ba
    add sp, strict byte 0000ah                ; 83 c4 0a
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 00ch                               ; 80 cc 0c
    jmp near 0609ch                           ; e9 5f 01
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 eb b9
    mov ax, 0077ah                            ; b8 7a 07
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 23 ba
    add sp, strict byte 00004h                ; 83 c4 04
    mov byte [bp+017h], 000h                  ; c6 46 17 00
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 fb b6
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
    mov al, byte [bp-002h]                    ; 8a 46 fe
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov es, [bp-006h]                         ; 8e 46 fa
    mov bx, si                                ; 89 f3
    add bx, ax                                ; 01 c3
    mov di, word [es:bx+02ch]                 ; 26 8b 7f 2c
    mov cx, word [es:bx+02ah]                 ; 26 8b 4f 2a
    mov ax, word [es:bx+02eh]                 ; 26 8b 47 2e
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov dl, byte [es:si+001e2h]               ; 26 8a 94 e2 01
    xor dh, dh                                ; 30 f6
    mov byte [bp+016h], dh                    ; 88 76 16
    mov bx, word [bp+014h]                    ; 8b 5e 14
    dec di                                    ; 4f
    mov ax, di                                ; 89 f8
    mov bh, al                                ; 88 c7
    mov word [bp+014h], bx                    ; 89 5e 14
    shr di, 1                                 ; d1 ef
    shr di, 1                                 ; d1 ef
    and di, 000c0h                            ; 81 e7 c0 00
    mov ax, word [bp-00eh]                    ; 8b 46 f2
    and ax, strict word 0003fh                ; 25 3f 00
    or ax, di                                 ; 09 f8
    xor bl, bl                                ; 30 db
    or bx, ax                                 ; 09 c3
    mov word [bp+014h], bx                    ; 89 5e 14
    mov bx, word [bp+012h]                    ; 8b 5e 12
    xor bh, bh                                ; 30 ff
    mov ah, cl                                ; 88 cc
    xor al, al                                ; 30 c0
    sub ax, 00100h                            ; 2d 00 01
    or bx, ax                                 ; 09 c3
    mov word [bp+012h], bx                    ; 89 5e 12
    mov ax, bx                                ; 89 d8
    xor al, bl                                ; 30 d8
    or ax, dx                                 ; 09 d0
    mov word [bp+012h], ax                    ; 89 46 12
    jmp short 05f56h                          ; eb 83
    mov al, byte [bp-002h]                    ; 8a 46 fe
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    mov dx, strict word 00006h                ; ba 06 00
    imul dx                                   ; f7 ea
    mov es, [bp-006h]                         ; 8e 46 fa
    add si, ax                                ; 01 c6
    mov dx, word [es:si+00206h]               ; 26 8b 94 06 02
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 0c0h                  ; 24 c0
    cmp AL, strict byte 040h                  ; 3c 40
    jne short 05ff9h                          ; 75 03
    jmp near 05f56h                           ; e9 5d ff
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 0aah                               ; 80 cc aa
    jmp near 0609ch                           ; e9 98 00
    mov al, byte [bp-002h]                    ; 8a 46 fe
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov es, [bp-006h]                         ; 8e 46 fa
    add si, ax                                ; 01 c6
    mov ax, word [es:si+032h]                 ; 26 8b 44 32
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov ax, word [es:si+030h]                 ; 26 8b 44 30
    mov word [bp-008h], ax                    ; 89 46 f8
    mov di, word [es:si+034h]                 ; 26 8b 7c 34
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    xor dx, dx                                ; 31 d2
    mov bx, word [bp-008h]                    ; 8b 5e f8
    xor cx, cx                                ; 31 c9
    call 0a150h                               ; e8 1e 41
    mov bx, di                                ; 89 fb
    xor cx, cx                                ; 31 c9
    call 0a150h                               ; e8 17 41
    mov word [bp-010h], ax                    ; 89 46 f0
    mov word [bp-004h], dx                    ; 89 56 fc
    mov word [bp+014h], dx                    ; 89 56 14
    mov word [bp+012h], ax                    ; 89 46 12
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 003h                               ; 80 cc 03
    mov word [bp+016h], ax                    ; 89 46 16
    jmp near 05f5ah                           ; e9 07 ff
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 d5 b8
    mov al, byte [bp+017h]                    ; 8a 46 17
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 0067eh                            ; b8 7e 06
    push ax                                   ; 50
    mov ax, 00794h                            ; b8 94 07
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 03 b9
    add sp, strict byte 00008h                ; 83 c4 08
    jmp near 05f56h                           ; e9 dd fe
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 af b8
    mov al, byte [bp+017h]                    ; 8a 46 17
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 0067eh                            ; b8 7e 06
    push ax                                   ; 50
    mov ax, 007c7h                            ; b8 c7 07
    jmp near 05d9dh                           ; e9 09 fd
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 001h                               ; 80 cc 01
    mov word [bp+016h], ax                    ; 89 46 16
    mov bl, byte [bp+017h]                    ; 8a 5e 17
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 b3 b5
    or byte [bp+01ch], 001h                   ; 80 4e 1c 01
    jmp near 05f69h                           ; e9 b5 fe
    outsb                                     ; 6e
    popaw                                     ; 61
    mov ax, word [0a161h]                     ; a1 61 a1
    popaw                                     ; 61
    mov ax, word [0b361h]                     ; a1 61 b3
    db  065h, 016h
    ; gs push SS                                ; 65 16
    arpl [bx+di+01c61h], sp                   ; 63 a1 61 1c
    arpl [bp+di-0799bh], si                   ; 63 b3 65 86
    popaw                                     ; 61
    xchg byte [bx+di-07ah], ah                ; 86 61 86
    popaw                                     ; 61
    xchg byte [bx+di-036h], ah                ; 86 61 ca
    xchg byte [gs:bx+di-07ah], ah             ; 65 86 61 86
    popaw                                     ; 61
_int13_harddisk_ext:                         ; 0xf60d4 LB 0x52b
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    sub sp, strict byte 00028h                ; 83 ec 28
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 8b b5
    mov word [bp-018h], ax                    ; 89 46 e8
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 7f b5
    mov word [bp-008h], 00122h                ; c7 46 f8 22 01
    mov word [bp-006h], ax                    ; 89 46 fa
    xor bx, bx                                ; 31 db
    mov dx, 0008eh                            ; ba 8e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 5e b5
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    xor ah, ah                                ; 30 e4
    cmp ax, 00080h                            ; 3d 80 00
    jc short 06111h                           ; 72 05
    cmp ax, 00090h                            ; 3d 90 00
    jc short 06130h                           ; 72 1f
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov al, byte [bp+017h]                    ; 8a 46 17
    push ax                                   ; 50
    mov ax, 007f5h                            ; b8 f5 07
    push ax                                   ; 50
    mov ax, 0068dh                            ; b8 8d 06
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 4c b8
    add sp, strict byte 0000ah                ; 83 c4 0a
    jmp near 065e0h                           ; e9 b0 04
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    xor ah, ah                                ; 30 e4
    les bx, [bp-008h]                         ; c4 5e f8
    add bx, ax                                ; 01 c3
    mov dl, byte [es:bx+00163h]               ; 26 8a 97 63 01
    mov byte [bp-004h], dl                    ; 88 56 fc
    cmp dl, 010h                              ; 80 fa 10
    jc short 06155h                           ; 72 0e
    push ax                                   ; 50
    mov al, byte [bp+017h]                    ; 8a 46 17
    push ax                                   ; 50
    mov ax, 007f5h                            ; b8 f5 07
    push ax                                   ; 50
    mov ax, 006b8h                            ; b8 b8 06
    jmp short 06122h                          ; eb cd
    mov bl, byte [bp+017h]                    ; 8a 5e 17
    xor bh, bh                                ; 30 ff
    sub bx, strict byte 00041h                ; 83 eb 41
    cmp bx, strict byte 0000fh                ; 83 fb 0f
    jnbe short 06186h                         ; 77 24
    sal bx, 1                                 ; d1 e3
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    jmp word [cs:bx+060b4h]                   ; 2e ff a7 b4 60
    mov word [bp+010h], 0aa55h                ; c7 46 10 55 aa
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 030h                               ; 80 cc 30
    mov word [bp+016h], ax                    ; 89 46 16
    mov word [bp+014h], strict word 00007h    ; c7 46 14 07 00
    jmp near 065b7h                           ; e9 31 04
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 a2 b7
    mov al, byte [bp+017h]                    ; 8a 46 17
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 007f5h                            ; b8 f5 07
    push ax                                   ; 50
    mov ax, 007c7h                            ; b8 c7 07
    jmp near 0623eh                           ; e9 9d 00
    mov di, word [bp+00ah]                    ; 8b 7e 0a
    mov es, [bp+004h]                         ; 8e 46 04
    mov word [bp-024h], di                    ; 89 7e dc
    mov [bp-01eh], es                         ; 8c 46 e2
    mov ax, word [es:di+002h]                 ; 26 8b 45 02
    mov word [bp-016h], ax                    ; 89 46 ea
    mov ax, word [es:di+006h]                 ; 26 8b 45 06
    mov word [bp-01ch], ax                    ; 89 46 e4
    mov ax, word [es:di+004h]                 ; 26 8b 45 04
    mov word [bp-01ah], ax                    ; 89 46 e6
    mov dx, word [es:di+00ch]                 ; 26 8b 55 0c
    mov cx, word [es:di+00eh]                 ; 26 8b 4d 0e
    xor ax, ax                                ; 31 c0
    xor bx, bx                                ; 31 db
    mov si, strict word 00020h                ; be 20 00
    call 0a1b0h                               ; e8 dc 3f
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov word [bp-010h], bx                    ; 89 5e f0
    mov si, dx                                ; 89 d6
    mov dx, word [es:di+008h]                 ; 26 8b 55 08
    mov ax, word [es:di+00ah]                 ; 26 8b 45 0a
    or si, dx                                 ; 09 d6
    or cx, ax                                 ; 09 c1
    mov al, byte [bp-004h]                    ; 8a 46 fc
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    les bx, [bp-008h]                         ; c4 5e f8
    add bx, ax                                ; 01 c3
    mov al, byte [es:bx+022h]                 ; 26 8a 47 22
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    cmp dx, word [es:bx+03ch]                 ; 26 3b 57 3c
    jnbe short 06226h                         ; 77 22
    jne short 0624ch                          ; 75 46
    mov dx, word [bp-010h]                    ; 8b 56 f0
    cmp dx, word [es:bx+03ah]                 ; 26 3b 57 3a
    jnbe short 06226h                         ; 77 17
    mov dx, word [bp-010h]                    ; 8b 56 f0
    cmp dx, word [es:bx+03ah]                 ; 26 3b 57 3a
    jne short 0624ch                          ; 75 34
    cmp cx, word [es:bx+038h]                 ; 26 3b 4f 38
    jnbe short 06226h                         ; 77 08
    jne short 0624ch                          ; 75 2c
    cmp si, word [es:bx+036h]                 ; 26 3b 77 36
    jc short 0624ch                           ; 72 26
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 02 b7
    mov al, byte [bp+017h]                    ; 8a 46 17
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 007f5h                            ; b8 f5 07
    push ax                                   ; 50
    mov ax, 00808h                            ; b8 08 08
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 30 b7
    add sp, strict byte 00008h                ; 83 c4 08
    jmp near 065e0h                           ; e9 94 03
    mov ah, byte [bp+017h]                    ; 8a 66 17
    mov byte [bp-012h], ah                    ; 88 66 ee
    mov byte [bp-011h], 000h                  ; c6 46 ef 00
    cmp word [bp-012h], strict byte 00044h    ; 83 7e ee 44
    je short 06262h                           ; 74 06
    cmp word [bp-012h], strict byte 00047h    ; 83 7e ee 47
    jne short 06265h                          ; 75 03
    jmp near 065b3h                           ; e9 4e 03
    les bx, [bp-008h]                         ; c4 5e f8
    mov word [es:bx+018h], strict word 00000h ; 26 c7 47 18 00 00
    mov word [es:bx+01ah], strict word 00000h ; 26 c7 47 1a 00 00
    mov word [es:bx+01ch], strict word 00000h ; 26 c7 47 1c 00 00
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    mov word [es:bx+006h], dx                 ; 26 89 57 06
    mov dx, word [bp-010h]                    ; 8b 56 f0
    mov word [es:bx+004h], dx                 ; 26 89 57 04
    mov word [es:bx+002h], cx                 ; 26 89 4f 02
    mov word [es:bx], si                      ; 26 89 37
    mov dx, word [bp-01ah]                    ; 8b 56 e6
    mov word [es:bx+008h], dx                 ; 26 89 57 08
    mov dx, word [bp-01ch]                    ; 8b 56 e4
    mov word [es:bx+00ah], dx                 ; 26 89 57 0a
    mov dx, word [bp-016h]                    ; 8b 56 ea
    mov word [es:bx+00eh], dx                 ; 26 89 57 0e
    mov word [es:bx+010h], 00200h             ; 26 c7 47 10 00 02
    mov word [es:bx+016h], strict word 00000h ; 26 c7 47 16 00 00
    mov ah, byte [bp-004h]                    ; 8a 66 fc
    mov byte [es:bx+00ch], ah                 ; 26 88 67 0c
    mov bx, word [bp-012h]                    ; 8b 5e ee
    sal bx, 1                                 ; d1 e3
    xor ah, ah                                ; 30 e4
    sal ax, 1                                 ; d1 e0
    sal ax, 1                                 ; d1 e0
    add bx, ax                                ; 01 c3
    push ES                                   ; 06
    push word [bp-008h]                       ; ff 76 f8
    call word [word bx-00002h]                ; ff 97 fe ff
    mov dx, ax                                ; 89 c2
    les bx, [bp-008h]                         ; c4 5e f8
    mov ax, word [es:bx+018h]                 ; 26 8b 47 18
    mov word [bp-016h], ax                    ; 89 46 ea
    mov es, [bp-01eh]                         ; 8e 46 e2
    mov bx, word [bp-024h]                    ; 8b 5e dc
    mov word [es:bx+002h], ax                 ; 26 89 47 02
    test dl, dl                               ; 84 d2
    je short 0633ah                           ; 74 54
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 42 b6
    mov al, dl                                ; 88 d0
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    push word [bp-012h]                       ; ff 76 ee
    mov ax, 007f5h                            ; b8 f5 07
    push ax                                   ; 50
    mov ax, 00759h                            ; b8 59 07
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 6e b6
    add sp, strict byte 0000ah                ; 83 c4 0a
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 00ch                               ; 80 cc 0c
    jmp near 065e8h                           ; e9 d2 02
    or ah, 0b2h                               ; 80 cc b2
    jmp near 065e8h                           ; e9 cc 02
    mov bx, word [bp+00ah]                    ; 8b 5e 0a
    mov ax, word [bp+004h]                    ; 8b 46 04
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov di, bx                                ; 89 df
    mov word [bp-00ah], ax                    ; 89 46 f6
    mov es, ax                                ; 8e c0
    mov ax, word [es:bx]                      ; 26 8b 07
    mov word [bp-014h], ax                    ; 89 46 ec
    cmp ax, strict word 0001ah                ; 3d 1a 00
    jnc short 0633dh                          ; 73 06
    jmp near 065e0h                           ; e9 a6 02
    jmp near 065b3h                           ; e9 76 02
    jnc short 06342h                          ; 73 03
    jmp near 063d5h                           ; e9 93 00
    mov al, byte [bp-004h]                    ; 8a 46 fc
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    les bx, [bp-008h]                         ; c4 5e f8
    add bx, ax                                ; 01 c3
    mov ax, word [es:bx+032h]                 ; 26 8b 47 32
    mov dx, word [es:bx+030h]                 ; 26 8b 57 30
    mov word [bp-026h], dx                    ; 89 56 da
    mov dx, word [es:bx+034h]                 ; 26 8b 57 34
    mov word [bp-028h], dx                    ; 89 56 d8
    mov dx, word [es:bx+03ch]                 ; 26 8b 57 3c
    mov word [bp-00eh], dx                    ; 89 56 f2
    mov dx, word [es:bx+03ah]                 ; 26 8b 57 3a
    mov word [bp-010h], dx                    ; 89 56 f0
    mov cx, word [es:bx+038h]                 ; 26 8b 4f 38
    mov si, word [es:bx+036h]                 ; 26 8b 77 36
    mov dx, word [es:bx+028h]                 ; 26 8b 57 28
    mov es, [bp-00ch]                         ; 8e 46 f4
    mov bx, di                                ; 89 fb
    mov word [es:bx], strict word 0001ah      ; 26 c7 07 1a 00
    mov word [es:bx+002h], strict word 00002h ; 26 c7 47 02 02 00
    mov word [es:bx+004h], ax                 ; 26 89 47 04
    mov word [es:bx+006h], strict word 00000h ; 26 c7 47 06 00 00
    mov ax, word [bp-026h]                    ; 8b 46 da
    mov word [es:bx+008h], ax                 ; 26 89 47 08
    mov word [es:bx+00ah], strict word 00000h ; 26 c7 47 0a 00 00
    mov ax, word [bp-028h]                    ; 8b 46 d8
    mov word [es:bx+00ch], ax                 ; 26 89 47 0c
    mov word [es:bx+00eh], strict word 00000h ; 26 c7 47 0e 00 00
    mov word [es:bx+018h], dx                 ; 26 89 57 18
    mov word [es:bx+010h], si                 ; 26 89 77 10
    mov word [es:bx+012h], cx                 ; 26 89 4f 12
    mov ax, word [bp-00eh]                    ; 8b 46 f2
    mov bx, word [bp-010h]                    ; 8b 5e f0
    mov dx, si                                ; 89 f2
    mov si, strict word 00020h                ; be 20 00
    call 0a1a0h                               ; e8 d5 3d
    mov bx, di                                ; 89 fb
    mov word [es:bx+014h], dx                 ; 26 89 57 14
    mov word [es:bx+016h], cx                 ; 26 89 4f 16
    cmp word [bp-014h], strict byte 0001eh    ; 83 7e ec 1e
    jc short 06447h                           ; 72 6c
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov word [es:di], strict word 0001eh      ; 26 c7 05 1e 00
    mov ax, word [bp-018h]                    ; 8b 46 e8
    mov word [es:di+01ch], ax                 ; 26 89 45 1c
    mov word [es:di+01ah], 00356h             ; 26 c7 45 1a 56 03
    mov cl, byte [bp-004h]                    ; 8a 4e fc
    xor ch, ch                                ; 30 ed
    mov ax, cx                                ; 89 c8
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    xor ah, ah                                ; 30 e4
    mov dx, strict word 00006h                ; ba 06 00
    imul dx                                   ; f7 ea
    les bx, [bp-008h]                         ; c4 5e f8
    add bx, ax                                ; 01 c3
    mov ax, word [es:bx+00206h]               ; 26 8b 87 06 02
    mov word [bp-022h], ax                    ; 89 46 de
    mov ax, word [es:bx+00208h]               ; 26 8b 87 08 02
    mov word [bp-020h], ax                    ; 89 46 e0
    mov al, byte [es:bx+00205h]               ; 26 8a 87 05 02
    mov byte [bp-002h], al                    ; 88 46 fe
    mov ax, cx                                ; 89 c8
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov bx, word [bp-008h]                    ; 8b 5e f8
    add bx, ax                                ; 01 c3
    mov al, byte [es:bx+027h]                 ; 26 8a 47 27
    test al, al                               ; 84 c0
    jne short 06438h                          ; 75 04
    xor si, si                                ; 31 f6
    jmp short 0643bh                          ; eb 03
    mov si, strict word 00008h                ; be 08 00
    or si, strict byte 00010h                 ; 83 ce 10
    cmp AL, strict byte 001h                  ; 3c 01
    jne short 0644ah                          ; 75 08
    mov dx, strict word 00001h                ; ba 01 00
    jmp short 0644ch                          ; eb 05
    jmp near 064e2h                           ; e9 98 00
    xor dx, dx                                ; 31 d2
    or si, dx                                 ; 09 d6
    cmp AL, strict byte 003h                  ; 3c 03
    jne short 06457h                          ; 75 05
    mov ax, strict word 00003h                ; b8 03 00
    jmp short 06459h                          ; eb 02
    xor ax, ax                                ; 31 c0
    or si, ax                                 ; 09 c6
    mov ax, word [bp-022h]                    ; 8b 46 de
    les bx, [bp-008h]                         ; c4 5e f8
    mov word [es:bx+00234h], ax               ; 26 89 87 34 02
    mov ax, word [bp-020h]                    ; 8b 46 e0
    mov word [es:bx+00236h], ax               ; 26 89 87 36 02
    mov al, byte [bp-004h]                    ; 8a 46 fc
    xor ah, ah                                ; 30 e4
    cwd                                       ; 99
    mov bx, strict word 00002h                ; bb 02 00
    idiv bx                                   ; f7 fb
    or dl, 00eh                               ; 80 ca 0e
    mov ax, dx                                ; 89 d0
    mov CL, strict byte 004h                  ; b1 04
    sal ax, CL                                ; d3 e0
    mov bx, word [bp-008h]                    ; 8b 5e f8
    mov byte [es:bx+00238h], al               ; 26 88 87 38 02
    mov byte [es:bx+00239h], 0cbh             ; 26 c6 87 39 02 cb
    mov al, byte [bp-002h]                    ; 8a 46 fe
    mov byte [es:bx+0023ah], al               ; 26 88 87 3a 02
    mov word [es:bx+0023bh], strict word 00001h ; 26 c7 87 3b 02 01 00
    mov byte [es:bx+0023dh], 000h             ; 26 c6 87 3d 02 00
    mov word [es:bx+0023eh], si               ; 26 89 b7 3e 02
    mov word [es:bx+00240h], strict word 00000h ; 26 c7 87 40 02 00 00
    mov byte [es:bx+00242h], 011h             ; 26 c6 87 42 02 11
    xor bl, bl                                ; 30 db
    xor bh, bh                                ; 30 ff
    jmp short 064c2h                          ; eb 05
    cmp bh, 00fh                              ; 80 ff 0f
    jnc short 064d8h                          ; 73 16
    mov al, bh                                ; 88 f8
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    add dx, 00356h                            ; 81 c2 56 03
    mov ax, word [bp-018h]                    ; 8b 46 e8
    call 01652h                               ; e8 80 b1
    add bl, al                                ; 00 c3
    db  0feh, 0c7h
    ; inc bh                                    ; fe c7
    jmp short 064bdh                          ; eb e5
    neg bl                                    ; f6 db
    les si, [bp-008h]                         ; c4 76 f8
    mov byte [es:si+00243h], bl               ; 26 88 9c 43 02
    cmp word [bp-014h], strict byte 00042h    ; 83 7e ec 42
    jnc short 064ebh                          ; 73 03
    jmp near 065b3h                           ; e9 c8 00
    mov al, byte [bp-004h]                    ; 8a 46 fc
    xor ah, ah                                ; 30 e4
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    xor ah, ah                                ; 30 e4
    mov dx, strict word 00006h                ; ba 06 00
    imul dx                                   ; f7 ea
    les bx, [bp-008h]                         ; c4 5e f8
    add bx, ax                                ; 01 c3
    mov al, byte [es:bx+00204h]               ; 26 8a 87 04 02
    mov dx, word [es:bx+00206h]               ; 26 8b 97 06 02
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov word [es:di], strict word 00042h      ; 26 c7 05 42 00
    mov word [es:di+01eh], 0beddh             ; 26 c7 45 1e dd be
    mov word [es:di+020h], strict word 00024h ; 26 c7 45 20 24 00
    mov word [es:di+022h], strict word 00000h ; 26 c7 45 22 00 00
    test al, al                               ; 84 c0
    jne short 06535h                          ; 75 0c
    mov word [es:di+024h], 05349h             ; 26 c7 45 24 49 53
    mov word [es:di+026h], 02041h             ; 26 c7 45 26 41 20
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov word [es:di+028h], 05441h             ; 26 c7 45 28 41 54
    mov word [es:di+02ah], 02041h             ; 26 c7 45 2a 41 20
    mov word [es:di+02ch], 02020h             ; 26 c7 45 2c 20 20
    mov word [es:di+02eh], 02020h             ; 26 c7 45 2e 20 20
    test al, al                               ; 84 c0
    jne short 0656ah                          ; 75 16
    mov word [es:di+030h], dx                 ; 26 89 55 30
    mov word [es:di+032h], strict word 00000h ; 26 c7 45 32 00 00
    mov word [es:di+034h], strict word 00000h ; 26 c7 45 34 00 00
    mov word [es:di+036h], strict word 00000h ; 26 c7 45 36 00 00
    mov al, byte [bp-004h]                    ; 8a 46 fc
    and AL, strict byte 001h                  ; 24 01
    xor ah, ah                                ; 30 e4
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov word [es:di+038h], ax                 ; 26 89 45 38
    mov word [es:di+03ah], strict word 00000h ; 26 c7 45 3a 00 00
    mov word [es:di+03ch], strict word 00000h ; 26 c7 45 3c 00 00
    mov word [es:di+03eh], strict word 00000h ; 26 c7 45 3e 00 00
    xor bl, bl                                ; 30 db
    mov BH, strict byte 01eh                  ; b7 1e
    jmp short 06595h                          ; eb 05
    cmp bh, 040h                              ; 80 ff 40
    jnc short 065aah                          ; 73 15
    mov al, bh                                ; 88 f8
    xor ah, ah                                ; 30 e4
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, ax                                ; 01 c2
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01652h                               ; e8 ae b0
    add bl, al                                ; 00 c3
    db  0feh, 0c7h
    ; inc bh                                    ; fe c7
    jmp short 06590h                          ; eb e6
    neg bl                                    ; f6 db
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov byte [es:di+041h], bl                 ; 26 88 5d 41
    mov byte [bp+017h], 000h                  ; c6 46 17 00
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 9e b0
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
    cmp ax, strict word 00006h                ; 3d 06 00
    je short 065b3h                           ; 74 e4
    cmp ax, strict word 00001h                ; 3d 01 00
    jc short 065e0h                           ; 72 0c
    jbe short 065b3h                          ; 76 dd
    cmp ax, strict word 00003h                ; 3d 03 00
    jc short 065e0h                           ; 72 05
    cmp ax, strict word 00004h                ; 3d 04 00
    jbe short 065b3h                          ; 76 d3
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 001h                               ; 80 cc 01
    mov word [bp+016h], ax                    ; 89 46 16
    mov bl, byte [bp+017h]                    ; 8a 5e 17
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 67 b0
    or byte [bp+01ch], 001h                   ; 80 4e 1c 01
    jmp short 065c6h                          ; eb c7
_int14_function:                             ; 0xf65ff LB 0x15c
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    sti                                       ; fb
    mov dx, word [bp+00eh]                    ; 8b 56 0e
    sal dx, 1                                 ; d1 e2
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 5f b0
    mov si, ax                                ; 89 c6
    mov bx, ax                                ; 89 c3
    mov dx, word [bp+00eh]                    ; 8b 56 0e
    add dx, strict byte 0007ch                ; 83 c2 7c
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 33 b0
    mov cl, al                                ; 88 c1
    cmp word [bp+00eh], strict byte 00004h    ; 83 7e 0e 04
    jnc short 0662bh                          ; 73 04
    test si, si                               ; 85 f6
    jnbe short 0662eh                         ; 77 03
    jmp near 06751h                           ; e9 23 01
    mov al, byte [bp+013h]                    ; 8a 46 13
    cmp AL, strict byte 001h                  ; 3c 01
    jc short 06642h                           ; 72 0d
    jbe short 066a2h                          ; 76 6b
    cmp AL, strict byte 003h                  ; 3c 03
    je short 0669ah                           ; 74 5f
    cmp AL, strict byte 002h                  ; 3c 02
    je short 0669dh                           ; 74 5e
    jmp near 0674bh                           ; e9 09 01
    test al, al                               ; 84 c0
    jne short 0669fh                          ; 75 59
    lea dx, [bx+003h]                         ; 8d 57 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    or AL, strict byte 080h                   ; 0c 80
    out DX, AL                                ; ee
    lea si, [bx+001h]                         ; 8d 77 01
    mov al, byte [bp+012h]                    ; 8a 46 12
    test AL, strict byte 0e0h                 ; a8 e0
    jne short 06665h                          ; 75 0c
    mov AL, strict byte 017h                  ; b0 17
    mov dx, bx                                ; 89 da
    out DX, AL                                ; ee
    mov AL, strict byte 004h                  ; b0 04
    mov dx, si                                ; 89 f2
    out DX, AL                                ; ee
    jmp short 0667ch                          ; eb 17
    and AL, strict byte 0e0h                  ; 24 e0
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 005h                  ; b1 05
    sar ax, CL                                ; d3 f8
    mov cx, ax                                ; 89 c1
    mov ax, 00600h                            ; b8 00 06
    sar ax, CL                                ; d3 f8
    mov dx, bx                                ; 89 da
    out DX, AL                                ; ee
    mov al, ah                                ; 88 e0
    mov dx, si                                ; 89 f2
    out DX, AL                                ; ee
    mov al, byte [bp+012h]                    ; 8a 46 12
    and AL, strict byte 01fh                  ; 24 1f
    lea dx, [bx+003h]                         ; 8d 57 03
    out DX, AL                                ; ee
    lea dx, [bx+005h]                         ; 8d 57 05
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov byte [bp+013h], al                    ; 88 46 13
    lea dx, [bx+006h]                         ; 8d 57 06
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov byte [bp+012h], al                    ; 88 46 12
    jmp near 0672ch                           ; e9 92 00
    jmp near 0673ah                           ; e9 9d 00
    jmp short 066f0h                          ; eb 51
    jmp near 0674bh                           ; e9 a9 00
    mov dx, strict word 0006ch                ; ba 6c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 c3 af
    mov si, ax                                ; 89 c6
    lea dx, [bx+005h]                         ; 8d 57 05
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and ax, strict word 00060h                ; 25 60 00
    cmp ax, strict word 00060h                ; 3d 60 00
    je short 066d2h                           ; 74 17
    test cl, cl                               ; 84 c9
    je short 066d2h                           ; 74 13
    mov dx, strict word 0006ch                ; ba 6c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 a6 af
    cmp ax, si                                ; 39 f0
    je short 066adh                           ; 74 e1
    mov si, ax                                ; 89 c6
    db  0feh, 0c9h
    ; dec cl                                    ; fe c9
    jmp short 066adh                          ; eb db
    test cl, cl                               ; 84 c9
    je short 066dch                           ; 74 06
    mov al, byte [bp+012h]                    ; 8a 46 12
    mov dx, bx                                ; 89 da
    out DX, AL                                ; ee
    lea dx, [bx+005h]                         ; 8d 57 05
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov byte [bp+013h], al                    ; 88 46 13
    test cl, cl                               ; 84 c9
    jne short 0672ch                          ; 75 43
    or AL, strict byte 080h                   ; 0c 80
    mov byte [bp+013h], al                    ; 88 46 13
    jmp short 0672ch                          ; eb 3c
    mov dx, strict word 0006ch                ; ba 6c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 75 af
    mov si, ax                                ; 89 c6
    lea dx, [bx+005h]                         ; 8d 57 05
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 0671ch                          ; 75 17
    test cl, cl                               ; 84 c9
    je short 0671ch                           ; 74 13
    mov dx, strict word 0006ch                ; ba 6c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 5c af
    cmp ax, si                                ; 39 f0
    je short 066fbh                           ; 74 e5
    mov si, ax                                ; 89 c6
    db  0feh, 0c9h
    ; dec cl                                    ; fe c9
    jmp short 066fbh                          ; eb df
    test cl, cl                               ; 84 c9
    je short 06732h                           ; 74 12
    mov byte [bp+013h], 000h                  ; c6 46 13 00
    mov dx, bx                                ; 89 da
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov byte [bp+012h], al                    ; 88 46 12
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    jmp short 06755h                          ; eb 23
    lea dx, [bx+005h]                         ; 8d 57 05
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    jmp short 066ebh                          ; eb b1
    lea dx, [si+005h]                         ; 8d 54 05
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov byte [bp+013h], al                    ; 88 46 13
    lea dx, [si+006h]                         ; 8d 54 06
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    jmp short 06729h                          ; eb de
    or byte [bp+01ch], 001h                   ; 80 4e 1c 01
    jmp short 06755h                          ; eb 04
    or byte [bp+01ch], 001h                   ; 80 4e 1c 01
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
timer_wait_:                                 ; 0xf675b LB 0x43
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push ax                                   ; 50
    mov bx, strict word 0000fh                ; bb 0f 00
    xor cx, cx                                ; 31 c9
    call 0a170h                               ; e8 07 3a
    mov cx, ax                                ; 89 c1
    mov bx, dx                                ; 89 d3
    mov dx, strict word 00061h                ; ba 61 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 010h                  ; 24 10
    mov byte [bp-006h], al                    ; 88 46 fa
    add cx, strict byte 0ffffh                ; 83 c1 ff
    adc bx, strict byte 0ffffh                ; 83 d3 ff
    cmp bx, strict byte 0ffffh                ; 83 fb ff
    jne short 06788h                          ; 75 05
    cmp cx, strict byte 0ffffh                ; 83 f9 ff
    je short 06797h                           ; 74 0f
    mov dx, strict word 00061h                ; ba 61 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 010h                  ; 24 10
    cmp al, byte [bp-006h]                    ; 3a 46 fa
    jne short 06788h                          ; 75 f3
    jmp short 06778h                          ; eb e1
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
set_enable_a20_:                             ; 0xf679e LB 0x30
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    mov bx, ax                                ; 89 c3
    mov dx, 00092h                            ; ba 92 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov cl, al                                ; 88 c1
    test bx, bx                               ; 85 db
    je short 067b7h                           ; 74 05
    or AL, strict byte 002h                   ; 0c 02
    out DX, AL                                ; ee
    jmp short 067bah                          ; eb 03
    and AL, strict byte 0fdh                  ; 24 fd
    out DX, AL                                ; ee
    test cl, 002h                             ; f6 c1 02
    je short 067c4h                           ; 74 05
    mov ax, strict word 00001h                ; b8 01 00
    jmp short 067c6h                          ; eb 02
    xor ax, ax                                ; 31 c0
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
set_e820_range_:                             ; 0xf67ce LB 0x88
    push si                                   ; 56
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov si, dx                                ; 89 d6
    mov es, ax                                ; 8e c0
    mov word [es:si], bx                      ; 26 89 1c
    mov word [es:si+002h], cx                 ; 26 89 4c 02
    mov al, byte [bp+00ah]                    ; 8a 46 0a
    xor ah, ah                                ; 30 e4
    mov word [es:si+004h], ax                 ; 26 89 44 04
    mov word [es:si+006h], strict word 00000h ; 26 c7 44 06 00 00
    sub word [bp+006h], bx                    ; 29 5e 06
    sbb word [bp+008h], cx                    ; 19 4e 08
    sub byte [bp+00ch], al                    ; 28 46 0c
    mov ax, word [bp+006h]                    ; 8b 46 06
    mov word [es:si+008h], ax                 ; 26 89 44 08
    mov ax, word [bp+008h]                    ; 8b 46 08
    mov word [es:si+00ah], ax                 ; 26 89 44 0a
    mov al, byte [bp+00ch]                    ; 8a 46 0c
    xor ah, ah                                ; 30 e4
    mov word [es:si+00ch], ax                 ; 26 89 44 0c
    mov word [es:si+00eh], strict word 00000h ; 26 c7 44 0e 00 00
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    mov word [es:si+010h], ax                 ; 26 89 44 10
    mov word [es:si+012h], strict word 00000h ; 26 c7 44 12 00 00
    pop bp                                    ; 5d
    pop si                                    ; 5e
    retn 0000ah                               ; c2 0a 00
    db  0ech, 0e9h, 0d8h, 0c1h, 0c0h, 0bfh, 091h, 090h, 089h, 088h, 083h, 052h, 04fh, 041h, 024h, 000h
    db  063h, 06bh, 092h, 068h, 0a6h, 068h, 042h, 069h, 048h, 069h, 04dh, 069h, 052h, 069h, 0fah, 069h
    db  01eh, 06ah, 03bh, 069h, 03bh, 069h, 0e8h, 06ah, 013h, 06bh, 026h, 06bh, 035h, 06bh, 042h, 069h
    db  03eh, 06bh
_int15_function:                             ; 0xf6856 LB 0x343
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    mov al, byte [bp+013h]                    ; 8a 46 13
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    cmp ax, 000ech                            ; 3d ec 00
    jnbe short 0689ch                         ; 77 35
    push CS                                   ; 0e
    pop ES                                    ; 07
    mov cx, strict word 00011h                ; b9 11 00
    mov di, 06824h                            ; bf 24 68
    repne scasb                               ; f2 ae
    sal cx, 1                                 ; d1 e1
    mov di, cx                                ; 89 cf
    mov si, word [cs:di+06834h]               ; 2e 8b b5 34 68
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    mov cx, word [bp+018h]                    ; 8b 4e 18
    and cl, 0feh                              ; 80 e1 fe
    mov dx, word [bp+018h]                    ; 8b 56 18
    or dl, 001h                               ; 80 ca 01
    mov bx, ax                                ; 89 c3
    or bh, 086h                               ; 80 cf 86
    jmp si                                    ; ff e6
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    cmp ax, 000c0h                            ; 3d c0 00
    je short 0689fh                           ; 74 03
    jmp near 06b63h                           ; e9 c4 02
    or byte [bp+018h], 001h                   ; 80 4e 18 01
    jmp near 06b0ah                           ; e9 64 02
    mov dx, ax                                ; 89 c2
    cmp ax, strict word 00001h                ; 3d 01 00
    jc short 068bbh                           ; 72 0e
    jbe short 068cfh                          ; 76 20
    cmp ax, strict word 00003h                ; 3d 03 00
    je short 06900h                           ; 74 4c
    cmp ax, strict word 00002h                ; 3d 02 00
    je short 068dfh                           ; 74 26
    jmp short 0690dh                          ; eb 52
    test ax, ax                               ; 85 c0
    jne short 0690dh                          ; 75 4e
    xor ax, ax                                ; 31 c0
    call 0679eh                               ; e8 da fe
    and byte [bp+018h], 0feh                  ; 80 66 18 fe
    mov byte [bp+013h], 000h                  ; c6 46 13 00
    jmp near 0693bh                           ; e9 6c 00
    mov ax, strict word 00001h                ; b8 01 00
    call 0679eh                               ; e8 c9 fe
    and byte [bp+018h], 0feh                  ; 80 66 18 fe
    mov byte [bp+013h], dh                    ; 88 76 13
    jmp near 0693bh                           ; e9 5c 00
    mov dx, 00092h                            ; ba 92 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov dx, ax                                ; 89 c2
    shr dx, 1                                 ; d1 ea
    and dl, 001h                              ; 80 e2 01
    mov ax, word [bp+012h]                    ; 8b 46 12
    mov al, dl                                ; 88 d0
    mov word [bp+012h], ax                    ; 89 46 12
    and byte [bp+018h], 0feh                  ; 80 66 18 fe
    xor al, dl                                ; 30 d0
    mov byte [bp+013h], al                    ; 88 46 13
    jmp near 0693bh                           ; e9 3b 00
    and byte [bp+018h], 0feh                  ; 80 66 18 fe
    mov byte [bp+013h], ah                    ; 88 66 13
    mov word [bp+00ch], ax                    ; 89 46 0c
    jmp near 0693bh                           ; e9 2e 00
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 1b b0
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 0082eh                            ; b8 2e 08
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 4d b0
    add sp, strict byte 00006h                ; 83 c4 06
    or byte [bp+018h], 001h                   ; 80 4e 18 01
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    or ah, 086h                               ; 80 cc 86
    mov word [bp+012h], ax                    ; 89 46 12
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
    mov word [bp+018h], dx                    ; 89 56 18
    jmp near 069f4h                           ; e9 ac 00
    mov word [bp+018h], dx                    ; 89 56 18
    jmp short 0693bh                          ; eb ee
    mov word [bp+018h], cx                    ; 89 4e 18
    jmp short 06938h                          ; eb e6
    test byte [bp+012h], 0ffh                 ; f6 46 12 ff
    jne short 069c7h                          ; 75 6f
    mov dx, 000a0h                            ; ba a0 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 f1 ac
    test AL, strict byte 001h                 ; a8 01
    jne short 069c4h                          ; 75 5f
    mov bx, strict word 00001h                ; bb 01 00
    mov dx, 000a0h                            ; ba a0 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 ef ac
    mov bx, word [bp+014h]                    ; 8b 5e 14
    mov dx, 00098h                            ; ba 98 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0167ch                               ; e8 ff ac
    mov bx, word [bp+00ch]                    ; 8b 5e 0c
    mov dx, 0009ah                            ; ba 9a 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0167ch                               ; e8 f3 ac
    mov bx, word [bp+00eh]                    ; 8b 5e 0e
    mov dx, 0009ch                            ; ba 9c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0167ch                               ; e8 e7 ac
    mov bx, word [bp+010h]                    ; 8b 5e 10
    mov dx, 0009eh                            ; ba 9e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0167ch                               ; e8 db ac
    and byte [bp+018h], 0feh                  ; 80 66 18 fe
    mov dx, 000a1h                            ; ba a1 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 0feh                  ; 24 fe
    out DX, AL                                ; ee
    mov ax, strict word 0000bh                ; b8 0b 00
    call 016aeh                               ; e8 fa ac
    mov dl, al                                ; 88 c2
    or dl, 040h                               ; 80 ca 40
    xor dh, dh                                ; 30 f6
    mov ax, strict word 0000bh                ; b8 0b 00
    call 016c9h                               ; e8 08 ad
    jmp near 0693bh                           ; e9 77 ff
    jmp near 06b01h                           ; e9 3a 01
    cmp ax, strict word 00001h                ; 3d 01 00
    jne short 069e8h                          ; 75 1c
    xor bx, bx                                ; 31 db
    mov dx, 000a0h                            ; ba a0 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 89 ac
    and byte [bp+018h], 0feh                  ; 80 66 18 fe
    mov ax, strict word 0000bh                ; b8 0b 00
    call 016aeh                               ; e8 cd ac
    mov dl, al                                ; 88 c2
    and dl, 0bfh                              ; 80 e2 bf
    jmp short 069b9h                          ; eb d1
    mov word [bp+018h], dx                    ; 89 56 18
    mov ax, bx                                ; 89 d8
    xor ah, bh                                ; 30 fc
    xor bl, bl                                ; 30 db
    dec ax                                    ; 48
    or bx, ax                                 ; 09 c3
    mov word [bp+012h], bx                    ; 89 5e 12
    jmp near 0693bh                           ; e9 41 ff
    mov ax, strict word 00031h                ; b8 31 00
    call 016aeh                               ; e8 ae ac
    mov dh, al                                ; 88 c6
    mov ax, strict word 00030h                ; b8 30 00
    call 016aeh                               ; e8 a6 ac
    mov dl, al                                ; 88 c2
    mov word [bp+012h], dx                    ; 89 56 12
    cmp dx, strict byte 0ffc0h                ; 83 fa c0
    jbe short 06a17h                          ; 76 05
    mov word [bp+012h], strict word 0ffc0h    ; c7 46 12 c0 ff
    and byte [bp+018h], 0feh                  ; 80 66 18 fe
    jmp near 0693bh                           ; e9 1d ff
    cli                                       ; fa
    mov ax, strict word 00001h                ; b8 01 00
    call 0679eh                               ; e8 79 fd
    mov dx, word [bp+006h]                    ; 8b 56 06
    add dx, strict byte 00038h                ; 83 c2 38
    mov ax, word [bp+014h]                    ; 8b 46 14
    mov bx, strict word 0ffffh                ; bb ff ff
    call 0167ch                               ; e8 48 ac
    mov dx, word [bp+006h]                    ; 8b 56 06
    add dx, strict byte 0003ah                ; 83 c2 3a
    mov ax, word [bp+014h]                    ; 8b 46 14
    xor bx, bx                                ; 31 db
    call 0167ch                               ; e8 3a ac
    mov dx, word [bp+006h]                    ; 8b 56 06
    add dx, strict byte 0003ch                ; 83 c2 3c
    mov ax, word [bp+014h]                    ; 8b 46 14
    mov bx, strict word 0000fh                ; bb 0f 00
    call 01660h                               ; e8 0f ac
    mov dx, word [bp+006h]                    ; 8b 56 06
    add dx, strict byte 0003dh                ; 83 c2 3d
    mov ax, word [bp+014h]                    ; 8b 46 14
    mov bx, 0009bh                            ; bb 9b 00
    call 01660h                               ; e8 00 ac
    mov dx, word [bp+006h]                    ; 8b 56 06
    add dx, strict byte 0003eh                ; 83 c2 3e
    mov ax, word [bp+014h]                    ; 8b 46 14
    xor bx, bx                                ; 31 db
    call 0167ch                               ; e8 0e ac
    mov AL, strict byte 011h                  ; b0 11
    mov dx, strict word 00020h                ; ba 20 00
    out DX, AL                                ; ee
    mov dx, 000a0h                            ; ba a0 00
    out DX, AL                                ; ee
    mov al, byte [bp+00dh]                    ; 8a 46 0d
    mov dx, strict word 00021h                ; ba 21 00
    out DX, AL                                ; ee
    mov ax, word [bp+00ch]                    ; 8b 46 0c
    mov dx, 000a1h                            ; ba a1 00
    out DX, AL                                ; ee
    mov AL, strict byte 004h                  ; b0 04
    mov dx, strict word 00021h                ; ba 21 00
    out DX, AL                                ; ee
    mov AL, strict byte 002h                  ; b0 02
    mov dx, 000a1h                            ; ba a1 00
    out DX, AL                                ; ee
    mov AL, strict byte 001h                  ; b0 01
    mov dx, strict word 00021h                ; ba 21 00
    out DX, AL                                ; ee
    mov dx, 000a1h                            ; ba a1 00
    out DX, AL                                ; ee
    mov AL, strict byte 0ffh                  ; b0 ff
    mov dx, strict word 00021h                ; ba 21 00
    out DX, AL                                ; ee
    mov dx, 000a1h                            ; ba a1 00
    out DX, AL                                ; ee
    mov si, word [bp+006h]                    ; 8b 76 06
    call 06aach                               ; e8 00 00
    pop di                                    ; 5f
    add di, strict byte 00018h                ; 83 c7 18
    push strict byte 00038h                   ; 6a 38
    push di                                   ; 57
    lgdt [es:si+008h]                         ; 26 0f 01 54 08
    lidt [es:si+010h]                         ; 26 0f 01 5c 10
    mov ax, strict word 00001h                ; b8 01 00
    lmsw ax                                   ; 0f 01 f0
    retf                                      ; cb
    mov ax, strict word 00028h                ; b8 28 00
    mov ss, ax                                ; 8e d0
    mov ax, strict word 00018h                ; b8 18 00
    mov ds, ax                                ; 8e d8
    mov ax, strict word 00020h                ; b8 20 00
    mov es, ax                                ; 8e c0
    lea ax, [bp+004h]                         ; 8d 46 04
    db  08bh, 0e0h
    ; mov sp, ax                                ; 8b e0
    popaw                                     ; 61
    add sp, strict byte 00006h                ; 83 c4 06
    pop cx                                    ; 59
    pop ax                                    ; 58
    pop ax                                    ; 58
    mov ax, strict word 00030h                ; b8 30 00
    push ax                                   ; 50
    push cx                                   ; 51
    retf                                      ; cb
    jmp near 0693bh                           ; e9 53 fe
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 40 ae
    mov ax, 0086eh                            ; b8 6e 08
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 78 ae
    add sp, strict byte 00004h                ; 83 c4 04
    or byte [bp+018h], 001h                   ; 80 4e 18 01
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    or ah, 086h                               ; 80 cc 86
    mov word [bp+012h], ax                    ; 89 46 12
    jmp near 0693bh                           ; e9 28 fe
    mov word [bp+018h], cx                    ; 89 4e 18
    mov word [bp+012h], ax                    ; 89 46 12
    mov word [bp+00ch], 0e6f5h                ; c7 46 0c f5 e6
    mov word [bp+014h], 0f000h                ; c7 46 14 00 f0
    jmp near 0693bh                           ; e9 15 fe
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 3f ab
    mov word [bp+014h], ax                    ; 89 46 14
    jmp near 06a17h                           ; e9 e2 fe
    mov ax, 0089dh                            ; b8 9d 08
    push ax                                   ; 50
    mov ax, strict word 00008h                ; b8 08 00
    jmp short 06afah                          ; eb bc
    test byte [bp+012h], 0ffh                 ; f6 46 12 ff
    jne short 06b63h                          ; 75 1f
    mov word [bp+012h], ax                    ; 89 46 12
    mov ax, word [bp+00ch]                    ; 8b 46 0c
    xor ah, ah                                ; 30 e4
    cmp ax, strict word 00001h                ; 3d 01 00
    jc short 06b5ch                           ; 72 0b
    cmp ax, strict word 00003h                ; 3d 03 00
    jnbe short 06b5ch                         ; 77 06
    mov word [bp+018h], cx                    ; 89 4e 18
    jmp near 0693bh                           ; e9 df fd
    or byte [bp+018h], 001h                   ; 80 4e 18 01
    jmp near 0693bh                           ; e9 d8 fd
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 c5 ad
    push word [bp+00ch]                       ; ff 76 0c
    push word [bp+012h]                       ; ff 76 12
    mov ax, 008b4h                            ; b8 b4 08
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 f7 ad
    add sp, strict byte 00008h                ; 83 c4 08
    jmp near 06b01h                           ; e9 7c ff
    add ch, byte [di+02ch]                    ; 02 6d 2c
    insw                                      ; 6d
    test byte [di-057h], ch                   ; 84 6d a9
    insw                                      ; 6d
    leave                                     ; c9
    insw                                      ; 6d
    call 079ffh                               ; e8 6d 0e
    outsb                                     ; 6e
    xor AL, strict byte 06eh                  ; 34 6e
    jno short 06c05h                          ; 71 6e
    movsw                                     ; a5
    outsb                                     ; 6e
_int15_function32:                           ; 0xf6b99 LB 0x39e
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    sub sp, strict byte 00008h                ; 83 ec 08
    mov al, byte [bp+021h]                    ; 8a 46 21
    xor ah, ah                                ; 30 e4
    mov bx, word [bp+028h]                    ; 8b 5e 28
    and bl, 0feh                              ; 80 e3 fe
    mov dx, word [bp+020h]                    ; 8b 56 20
    xor dh, dh                                ; 30 f6
    cmp ax, 000e8h                            ; 3d e8 00
    je short 06bffh                           ; 74 4a
    cmp ax, 000d0h                            ; 3d d0 00
    je short 06bcch                           ; 74 12
    cmp ax, 00086h                            ; 3d 86 00
    jne short 06bfdh                          ; 75 3e
    sti                                       ; fb
    mov dx, word [bp+01ch]                    ; 8b 56 1c
    mov ax, word [bp+018h]                    ; 8b 46 18
    call 0675bh                               ; e8 92 fb
    jmp near 06d7eh                           ; e9 b2 01
    cmp dx, strict byte 0004fh                ; 83 fa 4f
    jne short 06bfdh                          ; 75 2c
    cmp word [bp+016h], 05052h                ; 81 7e 16 52 50
    jne short 06c33h                          ; 75 5b
    cmp word [bp+014h], 04f43h                ; 81 7e 14 43 4f
    jne short 06c33h                          ; 75 54
    cmp word [bp+01eh], 04d4fh                ; 81 7e 1e 4f 4d
    jne short 06c33h                          ; 75 4d
    cmp word [bp+01ch], 04445h                ; 81 7e 1c 45 44
    jne short 06c33h                          ; 75 46
    mov ax, word [bp+00ah]                    ; 8b 46 0a
    or ax, word [bp+008h]                     ; 0b 46 08
    jne short 06c33h                          ; 75 3e
    mov ax, word [bp+006h]                    ; 8b 46 06
    or ax, word [bp+004h]                     ; 0b 46 04
    je short 06c01h                           ; 74 04
    jmp short 06c33h                          ; eb 34
    jmp short 06c29h                          ; eb 28
    mov word [bp+028h], bx                    ; 89 5e 28
    mov ax, word [bp+014h]                    ; 8b 46 14
    mov word [bp+008h], ax                    ; 89 46 08
    mov ax, word [bp+016h]                    ; 8b 46 16
    mov word [bp+00ah], ax                    ; 89 46 0a
    mov ax, word [bp+01ch]                    ; 8b 46 1c
    mov word [bp+004h], ax                    ; 89 46 04
    mov ax, word [bp+01eh]                    ; 8b 46 1e
    mov word [bp+006h], ax                    ; 89 46 06
    mov word [bp+020h], 03332h                ; c7 46 20 32 33
    mov word [bp+022h], 04941h                ; c7 46 22 41 49
    jmp near 06d7eh                           ; e9 55 01
    cmp dx, strict byte 00020h                ; 83 fa 20
    je short 06c39h                           ; 74 0b
    cmp dx, strict byte 00001h                ; 83 fa 01
    je short 06c36h                           ; 74 03
    jmp near 06d51h                           ; e9 1b 01
    jmp near 06ef7h                           ; e9 be 02
    cmp word [bp+01ah], 0534dh                ; 81 7e 1a 4d 53
    jne short 06c33h                          ; 75 f3
    cmp word [bp+018h], 04150h                ; 81 7e 18 50 41
    jne short 06c33h                          ; 75 ec
    mov ax, strict word 00035h                ; b8 35 00
    call 016aeh                               ; e8 61 aa
    mov bl, al                                ; 88 c3
    xor bh, bh                                ; 30 ff
    xor dx, dx                                ; 31 d2
    mov cx, strict word 00008h                ; b9 08 00
    sal bx, 1                                 ; d1 e3
    rcl dx, 1                                 ; d1 d2
    loop 06c56h                               ; e2 fa
    mov ax, strict word 00034h                ; b8 34 00
    call 016aeh                               ; e8 4c aa
    xor ah, ah                                ; 30 e4
    mov dx, bx                                ; 89 da
    or dx, ax                                 ; 09 c2
    xor bx, bx                                ; 31 db
    add bx, bx                                ; 01 db
    adc dx, 00100h                            ; 81 d2 00 01
    cmp dx, 00100h                            ; 81 fa 00 01
    jc short 06c7ch                           ; 72 06
    jne short 06caah                          ; 75 32
    test bx, bx                               ; 85 db
    jnbe short 06caah                         ; 77 2e
    mov ax, strict word 00031h                ; b8 31 00
    call 016aeh                               ; e8 2c aa
    mov bl, al                                ; 88 c3
    xor bh, bh                                ; 30 ff
    xor dx, dx                                ; 31 d2
    mov cx, strict word 00008h                ; b9 08 00
    sal bx, 1                                 ; d1 e3
    rcl dx, 1                                 ; d1 d2
    loop 06c8bh                               ; e2 fa
    mov ax, strict word 00030h                ; b8 30 00
    call 016aeh                               ; e8 17 aa
    xor ah, ah                                ; 30 e4
    or bx, ax                                 ; 09 c3
    mov cx, strict word 0000ah                ; b9 0a 00
    sal bx, 1                                 ; d1 e3
    rcl dx, 1                                 ; d1 d2
    loop 06c9eh                               ; e2 fa
    add bx, strict byte 00000h                ; 83 c3 00
    adc dx, strict byte 00010h                ; 83 d2 10
    mov ax, strict word 00062h                ; b8 62 00
    call 016aeh                               ; e8 fe a9
    xor ah, ah                                ; 30 e4
    mov word [bp-00ah], ax                    ; 89 46 f6
    xor al, al                                ; 30 c0
    mov word [bp-008h], ax                    ; 89 46 f8
    mov cx, strict word 00008h                ; b9 08 00
    sal word [bp-00ah], 1                     ; d1 66 f6
    rcl word [bp-008h], 1                     ; d1 56 f8
    loop 06cbdh                               ; e2 f8
    mov ax, strict word 00061h                ; b8 61 00
    call 016aeh                               ; e8 e3 a9
    xor ah, ah                                ; 30 e4
    or word [bp-00ah], ax                     ; 09 46 f6
    mov ax, word [bp-00ah]                    ; 8b 46 f6
    mov word [bp-008h], ax                    ; 89 46 f8
    mov word [bp-00ah], strict word 00000h    ; c7 46 f6 00 00
    mov ax, strict word 00063h                ; b8 63 00
    call 016aeh                               ; e8 cd a9
    mov byte [bp-004h], al                    ; 88 46 fc
    mov byte [bp-006h], al                    ; 88 46 fa
    mov ax, word [bp+014h]                    ; 8b 46 14
    cmp ax, strict word 00009h                ; 3d 09 00
    jnbe short 06d51h                         ; 77 62
    mov si, ax                                ; 89 c6
    sal si, 1                                 ; d1 e6
    mov ax, bx                                ; 89 d8
    add ax, strict word 00000h                ; 05 00 00
    mov cx, dx                                ; 89 d1
    adc cx, strict byte 0ffffh                ; 83 d1 ff
    jmp word [cs:si+06b85h]                   ; 2e ff a4 85 6b
    mov ax, strict word 00001h                ; b8 01 00
    push ax                                   ; 50
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    push ax                                   ; 50
    mov ax, strict word 00009h                ; b8 09 00
    push ax                                   ; 50
    mov ax, 0fc00h                            ; b8 00 fc
    push ax                                   ; 50
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+024h]                    ; 8b 46 24
    xor bx, bx                                ; 31 db
    xor cx, cx                                ; 31 c9
    call 067ceh                               ; e8 af fa
    mov word [bp+014h], strict word 00001h    ; c7 46 14 01 00
    mov word [bp+016h], strict word 00000h    ; c7 46 16 00 00
    jmp near 06edch                           ; e9 b0 01
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    push ax                                   ; 50
    mov ax, strict word 0000ah                ; b8 0a 00
    push ax                                   ; 50
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+024h]                    ; 8b 46 24
    mov bx, 0fc00h                            ; bb 00 fc
    mov cx, strict word 00009h                ; b9 09 00
    call 067ceh                               ; e8 84 fa
    mov word [bp+014h], strict word 00002h    ; c7 46 14 02 00
    jmp short 06d24h                          ; eb d3
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 d7 ab
    push word [bp+014h]                       ; ff 76 14
    push word [bp+020h]                       ; ff 76 20
    mov ax, 008b4h                            ; b8 b4 08
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 09 ac
    add sp, strict byte 00008h                ; 83 c4 08
    or byte [bp+028h], 001h                   ; 80 4e 28 01
    mov ax, word [bp+020h]                    ; 8b 46 20
    xor al, al                                ; 30 c0
    or AL, strict byte 086h                   ; 0c 86
    mov word [bp+020h], ax                    ; 89 46 20
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    push ax                                   ; 50
    mov ax, strict word 00010h                ; b8 10 00
    push ax                                   ; 50
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+024h]                    ; 8b 46 24
    xor bx, bx                                ; 31 db
    mov cx, strict word 0000fh                ; b9 0f 00
    call 067ceh                               ; e8 2d fa
    mov word [bp+014h], strict word 00003h    ; c7 46 14 03 00
    jmp near 06d24h                           ; e9 7b ff
    mov dx, strict word 00001h                ; ba 01 00
    push dx                                   ; 52
    xor dx, dx                                ; 31 d2
    push dx                                   ; 52
    push dx                                   ; 52
    push cx                                   ; 51
    push ax                                   ; 50
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+024h]                    ; 8b 46 24
    xor bx, bx                                ; 31 db
    mov cx, strict word 00010h                ; b9 10 00
    call 067ceh                               ; e8 0d fa
    mov word [bp+014h], strict word 00004h    ; c7 46 14 04 00
    jmp near 06d24h                           ; e9 5b ff
    mov si, strict word 00003h                ; be 03 00
    push si                                   ; 56
    xor si, si                                ; 31 f6
    push si                                   ; 56
    push si                                   ; 56
    push dx                                   ; 52
    push bx                                   ; 53
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov si, word [bp+024h]                    ; 8b 76 24
    mov bx, ax                                ; 89 c3
    mov ax, si                                ; 89 f0
    call 067ceh                               ; e8 ee f9
    mov word [bp+014h], strict word 00005h    ; c7 46 14 05 00
    jmp near 06d24h                           ; e9 3c ff
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    push ax                                   ; 50
    mov ax, 0fec0h                            ; b8 c0 fe
    push ax                                   ; 50
    mov ax, 01000h                            ; b8 00 10
    push ax                                   ; 50
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+024h]                    ; 8b 46 24
    xor bx, bx                                ; 31 db
    mov cx, 0fec0h                            ; b9 c0 fe
    call 067ceh                               ; e8 c8 f9
    mov word [bp+014h], strict word 00006h    ; c7 46 14 06 00
    jmp near 06d24h                           ; e9 16 ff
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    push ax                                   ; 50
    mov ax, 0fee0h                            ; b8 e0 fe
    push ax                                   ; 50
    mov ax, 01000h                            ; b8 00 10
    push ax                                   ; 50
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+024h]                    ; 8b 46 24
    xor bx, bx                                ; 31 db
    mov cx, 0fee0h                            ; b9 e0 fe
    call 067ceh                               ; e8 a2 f9
    mov word [bp+014h], strict word 00007h    ; c7 46 14 07 00
    jmp near 06d24h                           ; e9 f0 fe
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    push ax                                   ; 50
    push ax                                   ; 50
    push ax                                   ; 50
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+024h]                    ; 8b 46 24
    xor bx, bx                                ; 31 db
    mov cx, strict word 0fffch                ; b9 fc ff
    call 067ceh                               ; e8 82 f9
    cmp byte [bp-004h], 000h                  ; 80 7e fc 00
    jne short 06e59h                          ; 75 07
    mov ax, word [bp-008h]                    ; 8b 46 f8
    test ax, ax                               ; 85 c0
    je short 06e69h                           ; 74 10
    mov word [bp+014h], strict word 00009h    ; c7 46 14 09 00
    jmp near 06d24h                           ; e9 c3 fe
    mov word [bp+014h], strict word 00008h    ; c7 46 14 08 00
    jmp near 06d24h                           ; e9 bb fe
    mov word [bp+014h], ax                    ; 89 46 14
    mov word [bp+016h], ax                    ; 89 46 16
    jmp short 06edch                          ; eb 6b
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    push ax                                   ; 50
    push ax                                   ; 50
    push ax                                   ; 50
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+024h]                    ; 8b 46 24
    xor bx, bx                                ; 31 db
    xor cx, cx                                ; 31 c9
    call 067ceh                               ; e8 46 f9
    cmp byte [bp-004h], 000h                  ; 80 7e fc 00
    jne short 06e95h                          ; 75 07
    mov ax, word [bp-008h]                    ; 8b 46 f8
    test ax, ax                               ; 85 c0
    je short 06e9dh                           ; 74 08
    mov word [bp+014h], strict word 00009h    ; c7 46 14 09 00
    jmp near 06d24h                           ; e9 87 fe
    mov word [bp+014h], ax                    ; 89 46 14
    mov word [bp+016h], ax                    ; 89 46 16
    jmp short 06edch                          ; eb 37
    cmp byte [bp-004h], 000h                  ; 80 7e fc 00
    jne short 06eb1h                          ; 75 06
    cmp word [bp-008h], strict byte 00000h    ; 83 7e f8 00
    je short 06edch                           ; 74 2b
    mov ax, strict word 00001h                ; b8 01 00
    push ax                                   ; 50
    mov al, byte [bp-006h]                    ; 8a 46 fa
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, strict word 00001h                ; b8 01 00
    push ax                                   ; 50
    push word [bp-008h]                       ; ff 76 f8
    push word [bp-00ah]                       ; ff 76 f6
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+024h]                    ; 8b 46 24
    xor bx, bx                                ; 31 db
    xor cx, cx                                ; 31 c9
    call 067ceh                               ; e8 fa f8
    xor ax, ax                                ; 31 c0
    mov word [bp+014h], ax                    ; 89 46 14
    mov word [bp+016h], ax                    ; 89 46 16
    mov word [bp+020h], 04150h                ; c7 46 20 50 41
    mov word [bp+022h], 0534dh                ; c7 46 22 4d 53
    mov word [bp+01ch], strict word 00014h    ; c7 46 1c 14 00
    mov word [bp+01eh], strict word 00000h    ; c7 46 1e 00 00
    and byte [bp+028h], 0feh                  ; 80 66 28 fe
    jmp near 06d7eh                           ; e9 87 fe
    mov word [bp+028h], bx                    ; 89 5e 28
    mov ax, strict word 00031h                ; b8 31 00
    call 016aeh                               ; e8 ae a7
    mov dh, al                                ; 88 c6
    mov ax, strict word 00030h                ; b8 30 00
    call 016aeh                               ; e8 a6 a7
    mov dl, al                                ; 88 c2
    mov word [bp+01ch], dx                    ; 89 56 1c
    cmp dx, 03c00h                            ; 81 fa 00 3c
    jbe short 06f18h                          ; 76 05
    mov word [bp+01ch], 03c00h                ; c7 46 1c 00 3c
    mov ax, strict word 00035h                ; b8 35 00
    call 016aeh                               ; e8 90 a7
    mov dh, al                                ; 88 c6
    mov ax, strict word 00034h                ; b8 34 00
    call 016aeh                               ; e8 88 a7
    mov dl, al                                ; 88 c2
    mov word [bp+018h], dx                    ; 89 56 18
    mov ax, word [bp+01ch]                    ; 8b 46 1c
    mov word [bp+020h], ax                    ; 89 46 20
    mov word [bp+014h], dx                    ; 89 56 14
    jmp near 06d7eh                           ; e9 47 fe
_inv_op_handler:                             ; 0xf6f37 LB 0x18f
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    push ax                                   ; 50
    les bx, [bp+018h]                         ; c4 5e 18
    cmp byte [es:bx], 0f0h                    ; 26 80 3f f0
    jne short 06f4dh                          ; 75 06
    inc word [bp+018h]                        ; ff 46 18
    jmp near 070bfh                           ; e9 72 01
    cmp word [es:bx], 0050fh                  ; 26 81 3f 0f 05
    je short 06f57h                           ; 74 03
    jmp near 070bbh                           ; e9 64 01
    mov si, 00800h                            ; be 00 08
    xor ax, ax                                ; 31 c0
    mov word [bp-008h], ax                    ; 89 46 f8
    mov word [bp-006h], ax                    ; 89 46 fa
    mov es, ax                                ; 8e c0
    mov bx, word [es:si+02ch]                 ; 26 8b 5c 2c
    sub bx, strict byte 00006h                ; 83 eb 06
    mov dx, word [es:si+020h]                 ; 26 8b 54 20
    mov ax, word [es:si+01ah]                 ; 26 8b 44 1a
    mov es, dx                                ; 8e c2
    mov word [es:bx], ax                      ; 26 89 07
    mov es, [bp-008h]                         ; 8e 46 f8
    mov ax, word [es:si+022h]                 ; 26 8b 44 22
    mov es, dx                                ; 8e c2
    mov word [es:bx+002h], ax                 ; 26 89 47 02
    mov es, [bp-008h]                         ; 8e 46 f8
    mov ax, word [es:si+018h]                 ; 26 8b 44 18
    mov es, dx                                ; 8e c2
    mov word [es:bx+004h], ax                 ; 26 89 47 04
    mov es, [bp-008h]                         ; 8e 46 f8
    mov bl, byte [es:si+038h]                 ; 26 8a 5c 38
    xor bh, bh                                ; 30 ff
    mov di, word [es:si+036h]                 ; 26 8b 7c 36
    mov ax, word [es:si+024h]                 ; 26 8b 44 24
    xor dx, dx                                ; 31 d2
    mov cx, strict word 00004h                ; b9 04 00
    sal ax, 1                                 ; d1 e0
    rcl dx, 1                                 ; d1 d2
    loop 06fa8h                               ; e2 fa
    cmp bx, dx                                ; 39 d3
    jne short 06fb6h                          ; 75 04
    cmp di, ax                                ; 39 c7
    je short 06fbbh                           ; 74 05
    mov word [bp-006h], strict word 00001h    ; c7 46 fa 01 00
    mov es, [bp-008h]                         ; 8e 46 f8
    mov bl, byte [es:si+04ah]                 ; 26 8a 5c 4a
    xor bh, bh                                ; 30 ff
    mov di, word [es:si+048h]                 ; 26 8b 7c 48
    mov ax, word [es:si+01eh]                 ; 26 8b 44 1e
    xor dx, dx                                ; 31 d2
    mov cx, strict word 00004h                ; b9 04 00
    sal ax, 1                                 ; d1 e0
    rcl dx, 1                                 ; d1 d2
    loop 06fd1h                               ; e2 fa
    cmp bx, dx                                ; 39 d3
    jne short 06fdfh                          ; 75 04
    cmp di, ax                                ; 39 c7
    je short 06fe3h                           ; 74 04
    or byte [bp-006h], 002h                   ; 80 4e fa 02
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    mov ax, 00800h                            ; b8 00 08
    push ax                                   ; 50
    mov ax, strict word 0001fh                ; b8 1f 00
    push ax                                   ; 50
    db  08bh, 0dch
    ; mov bx, sp                                ; 8b dc
    lgdt [ss:bx]                              ; 36 0f 01 17
    add sp, strict byte 00006h                ; 83 c4 06
    mov es, [bp-008h]                         ; 8e 46 f8
    mov ax, word [es:si+03ah]                 ; 26 8b 44 3a
    mov word [es:si+008h], ax                 ; 26 89 44 08
    mov ax, word [es:si+036h]                 ; 26 8b 44 36
    mov word [es:si+00ah], ax                 ; 26 89 44 0a
    mov dh, byte [es:si+039h]                 ; 26 8a 74 39
    mov dl, byte [es:si+038h]                 ; 26 8a 54 38
    mov word [es:si+00ch], dx                 ; 26 89 54 0c
    mov word [es:si+00eh], strict word 00000h ; 26 c7 44 0e 00 00
    mov ax, word [es:si+04ch]                 ; 26 8b 44 4c
    mov word [es:si], ax                      ; 26 89 04
    mov ax, word [es:si+048h]                 ; 26 8b 44 48
    mov word [es:si+002h], ax                 ; 26 89 44 02
    mov dh, byte [es:si+04bh]                 ; 26 8a 74 4b
    mov dl, byte [es:si+04ah]                 ; 26 8a 54 4a
    xor ah, ah                                ; 30 e4
    mov word [es:si+004h], dx                 ; 26 89 54 04
    mov al, byte [es:si+05ch]                 ; 26 8a 44 5c
    mov dx, word [es:si+05ah]                 ; 26 8b 54 5a
    push ax                                   ; 50
    push dx                                   ; 52
    push word [es:si+05eh]                    ; 26 ff 74 5e
    db  08bh, 0dch
    ; mov bx, sp                                ; 8b dc
    lidt [ss:bx]                              ; 36 0f 01 1f
    add sp, strict byte 00006h                ; 83 c4 06
    mov cx, word [bp-006h]                    ; 8b 4e fa
    mov ax, 00080h                            ; b8 80 00
    mov ss, ax                                ; 8e d0
    mov ax, word [ss:0001eh]                  ; 36 a1 1e 00
    mov ds, ax                                ; 8e d8
    mov ax, word [ss:00024h]                  ; 36 a1 24 00
    mov es, ax                                ; 8e c0
    smsw ax                                   ; 0f 01 e0
    inc ax                                    ; 40
    lmsw ax                                   ; 0f 01 f0
    mov ax, strict word 00008h                ; b8 08 00
    test cx, strict word 00001h               ; f7 c1 01 00
    je near 07078h                            ; 0f 84 02 00
    mov es, ax                                ; 8e c0
    test cx, strict word 00002h               ; f7 c1 02 00
    je near 070a0h                            ; 0f 84 20 00
    mov bx, word [word ss:00000h]             ; 36 8b 1e 00 00
    mov word [word ss:00008h], bx             ; 36 89 1e 08 00
    mov bx, word [word ss:00002h]             ; 36 8b 1e 02 00
    mov word [word ss:0000ah], bx             ; 36 89 1e 0a 00
    mov bx, word [word ss:00004h]             ; 36 8b 1e 04 00
    mov word [word ss:0000ch], bx             ; 36 89 1e 0c 00
    mov ds, ax                                ; 8e d8
    mov eax, cr0                              ; 0f 20 c0
    dec ax                                    ; 48
    mov cr0, eax                              ; 0f 22 c0
    mov sp, strict word 00026h                ; bc 26 00
    popaw                                     ; 61
    mov sp, word [word ss:0002ch]             ; 36 8b 26 2c 00
    sub sp, strict byte 00006h                ; 83 ec 06
    mov ss, [word ss:00020h]                  ; 36 8e 16 20 00
    iret                                      ; cf
    jmp short 070bfh                          ; eb 04
    sti                                       ; fb
    hlt                                       ; f4
    jmp short 070bch                          ; eb fd
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
init_rtc_:                                   ; 0xf70c6 LB 0x28
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push dx                                   ; 52
    mov dx, strict word 00026h                ; ba 26 00
    mov ax, strict word 0000ah                ; b8 0a 00
    call 016c9h                               ; e8 f6 a5
    mov dx, strict word 00002h                ; ba 02 00
    mov ax, strict word 0000bh                ; b8 0b 00
    call 016c9h                               ; e8 ed a5
    mov ax, strict word 0000ch                ; b8 0c 00
    call 016aeh                               ; e8 cc a5
    mov ax, strict word 0000dh                ; b8 0d 00
    call 016aeh                               ; e8 c6 a5
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop dx                                    ; 5a
    pop bp                                    ; 5d
    retn                                      ; c3
rtc_updating_:                               ; 0xf70ee LB 0x21
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push dx                                   ; 52
    mov dx, 061a8h                            ; ba a8 61
    dec dx                                    ; 4a
    je short 07106h                           ; 74 0e
    mov ax, strict word 0000ah                ; b8 0a 00
    call 016aeh                               ; e8 b0 a5
    test AL, strict byte 080h                 ; a8 80
    jne short 070f5h                          ; 75 f3
    xor ax, ax                                ; 31 c0
    jmp short 07109h                          ; eb 03
    mov ax, strict word 00001h                ; b8 01 00
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop dx                                    ; 5a
    pop bp                                    ; 5d
    retn                                      ; c3
_int70_function:                             ; 0xf710f LB 0xbf
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push ax                                   ; 50
    mov ax, strict word 0000bh                ; b8 0b 00
    call 016aeh                               ; e8 94 a5
    mov bl, al                                ; 88 c3
    mov byte [bp-004h], al                    ; 88 46 fc
    mov ax, strict word 0000ch                ; b8 0c 00
    call 016aeh                               ; e8 89 a5
    mov dl, al                                ; 88 c2
    test bl, 060h                             ; f6 c3 60
    jne short 0712fh                          ; 75 03
    jmp near 071b5h                           ; e9 86 00
    test AL, strict byte 020h                 ; a8 20
    je short 07137h                           ; 74 04
    sti                                       ; fb
    int 04ah                                  ; cd 4a
    cli                                       ; fa
    test dl, 040h                             ; f6 c2 40
    je short 0719fh                           ; 74 63
    mov dx, 000a0h                            ; ba a0 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 0d a5
    test al, al                               ; 84 c0
    je short 071b5h                           ; 74 6c
    mov dx, 0009ch                            ; ba 9c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0168ah                               ; e8 38 a5
    test dx, dx                               ; 85 d2
    jne short 071a1h                          ; 75 4b
    cmp ax, 003d1h                            ; 3d d1 03
    jnc short 071a1h                          ; 73 46
    mov dx, 00098h                            ; ba 98 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 0a a5
    mov si, ax                                ; 89 c6
    mov dx, 0009ah                            ; ba 9a 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 ff a4
    mov cx, ax                                ; 89 c1
    xor bx, bx                                ; 31 db
    mov dx, 000a0h                            ; ba a0 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 e4 a4
    mov dl, byte [bp-004h]                    ; 8a 56 fc
    and dl, 037h                              ; 80 e2 37
    xor dh, dh                                ; 30 f6
    mov ax, strict word 0000bh                ; b8 0b 00
    call 016c9h                               ; e8 3f a5
    mov dx, cx                                ; 89 ca
    mov ax, si                                ; 89 f0
    call 01652h                               ; e8 c1 a4
    mov bl, al                                ; 88 c3
    or bl, 080h                               ; 80 cb 80
    xor bh, bh                                ; 30 ff
    mov dx, cx                                ; 89 ca
    mov ax, si                                ; 89 f0
    call 01660h                               ; e8 c1 a4
    jmp short 071b5h                          ; eb 14
    mov bx, ax                                ; 89 c3
    add bx, 0fc2fh                            ; 81 c3 2f fc
    mov cx, dx                                ; 89 d1
    adc cx, strict byte 0ffffh                ; 83 d1 ff
    mov dx, 0009ch                            ; ba 9c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0169ch                               ; e8 e7 a4
    call 0e030h                               ; e8 78 6e
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
    in AL, strict byte 071h                   ; e4 71
    or si, word [bp+si+030h]                  ; 0b 72 30
    jc short 07231h                           ; 72 6c
    jc short 07185h                           ; 72 be
    jc short 071beh                           ; 72 f5
    jc short 07207h                           ; 72 3c
    jnc short 07164h                          ; 73 97
    db  073h
_int1a_function:                             ; 0xf71ce LB 0x1d9
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    sti                                       ; fb
    mov al, byte [bp+013h]                    ; 8a 46 13
    cmp AL, strict byte 007h                  ; 3c 07
    jnbe short 07237h                         ; 77 5e
    mov bl, al                                ; 88 c3
    xor bh, bh                                ; 30 ff
    sal bx, 1                                 ; d1 e3
    jmp word [cs:bx+071beh]                   ; 2e ff a7 be 71
    cli                                       ; fa
    mov bx, 0046eh                            ; bb 6e 04
    xor ax, ax                                ; 31 c0
    mov es, ax                                ; 8e c0
    mov ax, word [es:bx]                      ; 26 8b 07
    mov word [bp+010h], ax                    ; 89 46 10
    mov bx, 0046ch                            ; bb 6c 04
    mov ax, word [es:bx]                      ; 26 8b 07
    mov word [bp+00eh], ax                    ; 89 46 0e
    mov bx, 00470h                            ; bb 70 04
    mov al, byte [es:bx]                      ; 26 8a 07
    mov byte [bp+012h], al                    ; 88 46 12
    mov byte [es:bx], 000h                    ; 26 c6 07 00
    sti                                       ; fb
    jmp short 07237h                          ; eb 2c
    cli                                       ; fa
    mov bx, 0046eh                            ; bb 6e 04
    xor ax, ax                                ; 31 c0
    mov es, ax                                ; 8e c0
    mov ax, word [bp+010h]                    ; 8b 46 10
    mov word [es:bx], ax                      ; 26 89 07
    mov bx, 0046ch                            ; bb 6c 04
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    mov word [es:bx], ax                      ; 26 89 07
    mov bx, 00470h                            ; bb 70 04
    mov byte [es:bx], 000h                    ; 26 c6 07 00
    sti                                       ; fb
    mov byte [bp+013h], 000h                  ; c6 46 13 00
    jmp short 07237h                          ; eb 07
    call 070eeh                               ; e8 bb fe
    test ax, ax                               ; 85 c0
    je short 0723ah                           ; 74 03
    jmp near 07268h                           ; e9 2e 00
    xor ax, ax                                ; 31 c0
    call 016aeh                               ; e8 6f a4
    mov byte [bp+00fh], al                    ; 88 46 0f
    mov ax, strict word 00002h                ; b8 02 00
    call 016aeh                               ; e8 66 a4
    mov byte [bp+010h], al                    ; 88 46 10
    mov ax, strict word 00004h                ; b8 04 00
    call 016aeh                               ; e8 5d a4
    mov dl, al                                ; 88 c2
    mov byte [bp+011h], al                    ; 88 46 11
    mov ax, strict word 0000bh                ; b8 0b 00
    call 016aeh                               ; e8 52 a4
    and AL, strict byte 001h                  ; 24 01
    mov byte [bp+00eh], al                    ; 88 46 0e
    mov byte [bp+013h], 000h                  ; c6 46 13 00
    mov byte [bp+012h], dl                    ; 88 56 12
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
    call 070eeh                               ; e8 7f fe
    test ax, ax                               ; 85 c0
    je short 07276h                           ; 74 03
    call 070c6h                               ; e8 50 fe
    mov dl, byte [bp+00fh]                    ; 8a 56 0f
    xor dh, dh                                ; 30 f6
    xor ax, ax                                ; 31 c0
    call 016c9h                               ; e8 49 a4
    mov dl, byte [bp+010h]                    ; 8a 56 10
    xor dh, dh                                ; 30 f6
    mov ax, strict word 00002h                ; b8 02 00
    call 016c9h                               ; e8 3e a4
    mov dl, byte [bp+011h]                    ; 8a 56 11
    xor dh, dh                                ; 30 f6
    mov ax, strict word 00004h                ; b8 04 00
    call 016c9h                               ; e8 33 a4
    mov ax, strict word 0000bh                ; b8 0b 00
    call 016aeh                               ; e8 12 a4
    mov bl, al                                ; 88 c3
    and bl, 060h                              ; 80 e3 60
    or bl, 002h                               ; 80 cb 02
    mov al, byte [bp+00eh]                    ; 8a 46 0e
    and AL, strict byte 001h                  ; 24 01
    or bl, al                                 ; 08 c3
    mov dl, bl                                ; 88 da
    xor dh, dh                                ; 30 f6
    mov ax, strict word 0000bh                ; b8 0b 00
    call 016c9h                               ; e8 14 a4
    mov byte [bp+013h], 000h                  ; c6 46 13 00
    mov byte [bp+012h], bl                    ; 88 5e 12
    jmp short 07268h                          ; eb aa
    mov byte [bp+013h], 000h                  ; c6 46 13 00
    call 070eeh                               ; e8 29 fe
    test ax, ax                               ; 85 c0
    je short 072cbh                           ; 74 02
    jmp short 07268h                          ; eb 9d
    mov ax, strict word 00009h                ; b8 09 00
    call 016aeh                               ; e8 dd a3
    mov byte [bp+010h], al                    ; 88 46 10
    mov ax, strict word 00008h                ; b8 08 00
    call 016aeh                               ; e8 d4 a3
    mov byte [bp+00fh], al                    ; 88 46 0f
    mov ax, strict word 00007h                ; b8 07 00
    call 016aeh                               ; e8 cb a3
    mov byte [bp+00eh], al                    ; 88 46 0e
    mov ax, strict word 00032h                ; b8 32 00
    call 016aeh                               ; e8 c2 a3
    mov byte [bp+011h], al                    ; 88 46 11
    mov byte [bp+012h], al                    ; 88 46 12
    jmp near 07268h                           ; e9 73 ff
    call 070eeh                               ; e8 f6 fd
    test ax, ax                               ; 85 c0
    je short 07302h                           ; 74 06
    call 070c6h                               ; e8 c7 fd
    jmp near 07268h                           ; e9 66 ff
    mov dl, byte [bp+010h]                    ; 8a 56 10
    xor dh, dh                                ; 30 f6
    mov ax, strict word 00009h                ; b8 09 00
    call 016c9h                               ; e8 bc a3
    mov dl, byte [bp+00fh]                    ; 8a 56 0f
    xor dh, dh                                ; 30 f6
    mov ax, strict word 00008h                ; b8 08 00
    call 016c9h                               ; e8 b1 a3
    mov dl, byte [bp+00eh]                    ; 8a 56 0e
    xor dh, dh                                ; 30 f6
    mov ax, strict word 00007h                ; b8 07 00
    call 016c9h                               ; e8 a6 a3
    mov dl, byte [bp+011h]                    ; 8a 56 11
    xor dh, dh                                ; 30 f6
    mov ax, strict word 00032h                ; b8 32 00
    call 016c9h                               ; e8 9b a3
    mov ax, strict word 0000bh                ; b8 0b 00
    call 016aeh                               ; e8 7a a3
    mov bl, al                                ; 88 c3
    and bl, 07fh                              ; 80 e3 7f
    jmp near 072abh                           ; e9 6f ff
    mov ax, strict word 0000bh                ; b8 0b 00
    call 016aeh                               ; e8 6c a3
    mov bl, al                                ; 88 c3
    mov word [bp+012h], strict word 00000h    ; c7 46 12 00 00
    test AL, strict byte 020h                 ; a8 20
    je short 07350h                           ; 74 03
    jmp near 07268h                           ; e9 18 ff
    call 070eeh                               ; e8 9b fd
    test ax, ax                               ; 85 c0
    je short 0735ah                           ; 74 03
    call 070c6h                               ; e8 6c fd
    mov dl, byte [bp+00fh]                    ; 8a 56 0f
    xor dh, dh                                ; 30 f6
    mov ax, strict word 00001h                ; b8 01 00
    call 016c9h                               ; e8 64 a3
    mov dl, byte [bp+010h]                    ; 8a 56 10
    xor dh, dh                                ; 30 f6
    mov ax, strict word 00003h                ; b8 03 00
    call 016c9h                               ; e8 59 a3
    mov dl, byte [bp+011h]                    ; 8a 56 11
    xor dh, dh                                ; 30 f6
    mov ax, strict word 00005h                ; b8 05 00
    call 016c9h                               ; e8 4e a3
    mov dx, 000a1h                            ; ba a1 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 0feh                  ; 24 fe
    out DX, AL                                ; ee
    mov dl, bl                                ; 88 da
    and dl, 05fh                              ; 80 e2 5f
    or dl, 020h                               ; 80 ca 20
    xor dh, dh                                ; 30 f6
    mov ax, strict word 0000bh                ; b8 0b 00
    call 016c9h                               ; e8 35 a3
    jmp near 07268h                           ; e9 d1 fe
    mov ax, strict word 0000bh                ; b8 0b 00
    call 016aeh                               ; e8 11 a3
    mov bl, al                                ; 88 c3
    mov dl, al                                ; 88 c2
    and dl, 057h                              ; 80 e2 57
    jmp near 072adh                           ; e9 06 ff
send_to_mouse_ctrl_:                         ; 0xf73a7 LB 0x38
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push dx                                   ; 52
    mov bl, al                                ; 88 c3
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 002h                 ; a8 02
    je short 073cah                           ; 74 12
    mov ax, 008eeh                            ; b8 ee 08
    push ax                                   ; 50
    mov ax, 01168h                            ; b8 68 11
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 af a5
    add sp, strict byte 00006h                ; 83 c4 06
    mov AL, strict byte 0d4h                  ; b0 d4
    mov dx, strict word 00064h                ; ba 64 00
    out DX, AL                                ; ee
    mov al, bl                                ; 88 d8
    mov dx, strict word 00060h                ; ba 60 00
    out DX, AL                                ; ee
    xor al, bl                                ; 30 d8
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop dx                                    ; 5a
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
get_mouse_data_:                             ; 0xf73df LB 0x5d
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push ax                                   ; 50
    mov bx, ax                                ; 89 c3
    mov es, dx                                ; 8e c2
    mov cx, 02710h                            ; b9 10 27
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and ax, strict word 00021h                ; 25 21 00
    cmp ax, strict word 00021h                ; 3d 21 00
    je short 07422h                           ; 74 28
    test cx, cx                               ; 85 c9
    je short 07422h                           ; 74 24
    mov dx, strict word 00061h                ; ba 61 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 010h                  ; 24 10
    mov byte [bp-006h], al                    ; 88 46 fa
    mov dx, strict word 00061h                ; ba 61 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov dx, ax                                ; 89 c2
    xor dh, ah                                ; 30 e6
    and dl, 010h                              ; 80 e2 10
    mov al, byte [bp-006h]                    ; 8a 46 fa
    xor ah, ah                                ; 30 e4
    cmp dx, ax                                ; 39 c2
    je short 07409h                           ; 74 ea
    dec cx                                    ; 49
    jmp short 073ech                          ; eb ca
    test cx, cx                               ; 85 c9
    jne short 0742ah                          ; 75 04
    mov AL, strict byte 001h                  ; b0 01
    jmp short 07435h                          ; eb 0b
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov byte [es:bx], al                      ; 26 88 07
    xor al, al                                ; 30 c0
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
set_kbd_command_byte_:                       ; 0xf743c LB 0x36
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push dx                                   ; 52
    mov bl, al                                ; 88 c3
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 002h                 ; a8 02
    je short 0745fh                           ; 74 12
    mov ax, 008f8h                            ; b8 f8 08
    push ax                                   ; 50
    mov ax, 01168h                            ; b8 68 11
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 1a a5
    add sp, strict byte 00006h                ; 83 c4 06
    mov AL, strict byte 060h                  ; b0 60
    mov dx, strict word 00064h                ; ba 64 00
    out DX, AL                                ; ee
    mov al, bl                                ; 88 d8
    mov dx, strict word 00060h                ; ba 60 00
    out DX, AL                                ; ee
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop dx                                    ; 5a
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
_int74_function:                             ; 0xf7472 LB 0xd2
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    sub sp, strict byte 00008h                ; 83 ec 08
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 ed a1
    mov cx, ax                                ; 89 c1
    mov word [bp+004h], strict word 00000h    ; c7 46 04 00 00
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 021h                  ; 24 21
    cmp AL, strict byte 021h                  ; 3c 21
    jne short 074b6h                          ; 75 22
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov bl, al                                ; 88 c3
    mov dx, strict word 00026h                ; ba 26 00
    mov ax, cx                                ; 89 c8
    call 01652h                               ; e8 ae a1
    mov byte [bp-002h], al                    ; 88 46 fe
    mov dx, strict word 00027h                ; ba 27 00
    mov ax, cx                                ; 89 c8
    call 01652h                               ; e8 a3 a1
    mov byte [bp-006h], al                    ; 88 46 fa
    test AL, strict byte 080h                 ; a8 80
    jne short 074b9h                          ; 75 03
    jmp near 07530h                           ; e9 77 00
    mov al, byte [bp-006h]                    ; 8a 46 fa
    and AL, strict byte 007h                  ; 24 07
    mov byte [bp-004h], al                    ; 88 46 fc
    mov al, byte [bp-002h]                    ; 8a 46 fe
    and AL, strict byte 007h                  ; 24 07
    mov byte [bp-008h], al                    ; 88 46 f8
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov dx, ax                                ; 89 c2
    add dx, strict byte 00028h                ; 83 c2 28
    mov ax, cx                                ; 89 c8
    call 01660h                               ; e8 84 a1
    mov al, byte [bp-008h]                    ; 8a 46 f8
    cmp al, byte [bp-004h]                    ; 3a 46 fc
    jc short 07520h                           ; 72 3c
    mov dx, strict word 00028h                ; ba 28 00
    mov ax, cx                                ; 89 c8
    call 01652h                               ; e8 66 a1
    xor ah, ah                                ; 30 e4
    mov word [bp+00ch], ax                    ; 89 46 0c
    mov dx, strict word 00029h                ; ba 29 00
    mov ax, cx                                ; 89 c8
    call 01652h                               ; e8 59 a1
    xor ah, ah                                ; 30 e4
    mov word [bp+00ah], ax                    ; 89 46 0a
    mov dx, strict word 0002ah                ; ba 2a 00
    mov ax, cx                                ; 89 c8
    call 01652h                               ; e8 4c a1
    xor ah, ah                                ; 30 e4
    mov word [bp+008h], ax                    ; 89 46 08
    xor al, al                                ; 30 c0
    mov word [bp+006h], ax                    ; 89 46 06
    mov byte [bp-002h], ah                    ; 88 66 fe
    test byte [bp-006h], 080h                 ; f6 46 fa 80
    je short 07523h                           ; 74 0a
    mov word [bp+004h], strict word 00001h    ; c7 46 04 01 00
    jmp short 07523h                          ; eb 03
    inc byte [bp-002h]                        ; fe 46 fe
    mov bl, byte [bp-002h]                    ; 8a 5e fe
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00026h                ; ba 26 00
    mov ax, cx                                ; 89 c8
    call 01660h                               ; e8 30 a1
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
    xchg byte [di+002h], dh                   ; 86 75 02
    jbe short 074bbh                          ; 76 82
    jbe short 07552h                          ; 76 17
    jnbe short 074c6h                         ; 77 89
    jnbe short 0750ch                         ; 77 cd
    jne short 074f2h                          ; 75 b1
    jnbe short 075c1h                         ; 77 7e
    db  078h
_int15_function_mouse:                       ; 0xf7544 LB 0x3a0
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    sub sp, strict byte 00006h                ; 83 ec 06
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 1a a1
    mov cx, ax                                ; 89 c1
    cmp byte [bp+012h], 007h                  ; 80 7e 12 07
    jbe short 07567h                          ; 76 0b
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 001h                  ; c6 46 13 01
    jmp near 078deh                           ; e9 77 03
    mov ax, strict word 00065h                ; b8 65 00
    call 0743ch                               ; e8 cf fe
    and word [bp+018h], strict byte 0fffeh    ; 83 66 18 fe
    mov byte [bp+013h], 000h                  ; c6 46 13 00
    mov bl, byte [bp+012h]                    ; 8a 5e 12
    cmp bl, 007h                              ; 80 fb 07
    jnbe short 075dbh                         ; 77 5e
    xor bh, bh                                ; 30 ff
    sal bx, 1                                 ; d1 e3
    jmp word [cs:bx+07534h]                   ; 2e ff a7 34 75
    cmp byte [bp+00dh], 001h                  ; 80 7e 0d 01
    jnbe short 075deh                         ; 77 52
    mov dx, strict word 00027h                ; ba 27 00
    mov ax, cx                                ; 89 c8
    call 01652h                               ; e8 be a0
    test AL, strict byte 080h                 ; a8 80
    jne short 075a3h                          ; 75 0b
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 005h                  ; c6 46 13 05
    jmp near 078d8h                           ; e9 35 03
    cmp byte [bp+00dh], 000h                  ; 80 7e 0d 00
    jne short 075adh                          ; 75 04
    mov AL, strict byte 0f5h                  ; b0 f5
    jmp short 075afh                          ; eb 02
    mov AL, strict byte 0f4h                  ; b0 f4
    xor ah, ah                                ; 30 e4
    call 073a7h                               ; e8 f3 fd
    test al, al                               ; 84 c0
    jne short 075e1h                          ; 75 29
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 073dfh                               ; e8 1f fe
    test al, al                               ; 84 c0
    je short 075cah                           ; 74 06
    cmp byte [bp-006h], 0fah                  ; 80 7e fa fa
    jne short 075e1h                          ; 75 17
    jmp near 078d8h                           ; e9 0b 03
    mov al, byte [bp+00dh]                    ; 8a 46 0d
    cmp AL, strict byte 001h                  ; 3c 01
    jc short 075d8h                           ; 72 04
    cmp AL, strict byte 008h                  ; 3c 08
    jbe short 075e4h                          ; 76 0c
    jmp near 0777fh                           ; e9 a4 01
    jmp near 078c2h                           ; e9 e4 02
    jmp near 078d0h                           ; e9 ef 02
    jmp near 07856h                           ; e9 72 02
    mov dx, strict word 00027h                ; ba 27 00
    mov ax, cx                                ; 89 c8
    call 01652h                               ; e8 66 a0
    mov ah, byte [bp+00dh]                    ; 8a 66 0d
    db  0feh, 0cch
    ; dec ah                                    ; fe cc
    mov bl, al                                ; 88 c3
    and bl, 0f8h                              ; 80 e3 f8
    or bl, ah                                 ; 08 e3
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00027h                ; ba 27 00
    mov ax, cx                                ; 89 c8
    call 01660h                               ; e8 5e a0
    mov dx, strict word 00026h                ; ba 26 00
    mov ax, cx                                ; 89 c8
    call 01652h                               ; e8 48 a0
    mov bl, al                                ; 88 c3
    and bl, 0f8h                              ; 80 e3 f8
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00026h                ; ba 26 00
    mov ax, cx                                ; 89 c8
    call 01660h                               ; e8 47 a0
    mov ax, 000ffh                            ; b8 ff 00
    call 073a7h                               ; e8 88 fd
    test al, al                               ; 84 c0
    jne short 075e1h                          ; 75 be
    mov dx, ss                                ; 8c d2
    lea ax, [bp-008h]                         ; 8d 46 f8
    call 073dfh                               ; e8 b4 fd
    mov cl, al                                ; 88 c1
    cmp byte [bp-008h], 0feh                  ; 80 7e f8 fe
    jne short 0763dh                          ; 75 0a
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 004h                  ; c6 46 13 04
    jmp short 075cah                          ; eb 8d
    cmp byte [bp-008h], 0fah                  ; 80 7e f8 fa
    je short 07657h                           ; 74 14
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 00903h                            ; b8 03 09
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 22 a3
    add sp, strict byte 00006h                ; 83 c4 06
    test cl, cl                               ; 84 c9
    jne short 075e1h                          ; 75 86
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 073dfh                               ; e8 7c fd
    test al, al                               ; 84 c0
    jne short 076bdh                          ; 75 56
    mov dx, ss                                ; 8c d2
    lea ax, [bp-004h]                         ; 8d 46 fc
    call 073dfh                               ; e8 70 fd
    test al, al                               ; 84 c0
    jne short 076bdh                          ; 75 4a
    mov al, byte [bp-006h]                    ; 8a 46 fa
    mov byte [bp+00ch], al                    ; 88 46 0c
    mov al, byte [bp-004h]                    ; 8a 46 fc
    mov byte [bp+00dh], al                    ; 88 46 0d
    jmp near 078d8h                           ; e9 56 02
    mov al, byte [bp+00dh]                    ; 8a 46 0d
    cmp AL, strict byte 003h                  ; 3c 03
    jc short 07699h                           ; 72 10
    jbe short 076b7h                          ; 76 2c
    cmp AL, strict byte 006h                  ; 3c 06
    je short 076cch                           ; 74 3d
    cmp AL, strict byte 005h                  ; 3c 05
    je short 076c6h                           ; 74 33
    cmp AL, strict byte 004h                  ; 3c 04
    je short 076c0h                           ; 74 29
    jmp short 076d2h                          ; eb 39
    cmp AL, strict byte 002h                  ; 3c 02
    je short 076b1h                           ; 74 14
    cmp AL, strict byte 001h                  ; 3c 01
    je short 076abh                           ; 74 0a
    test al, al                               ; 84 c0
    jne short 076d2h                          ; 75 2d
    mov byte [bp-006h], 00ah                  ; c6 46 fa 0a
    jmp short 076d6h                          ; eb 2b
    mov byte [bp-006h], 014h                  ; c6 46 fa 14
    jmp short 076d6h                          ; eb 25
    mov byte [bp-006h], 028h                  ; c6 46 fa 28
    jmp short 076d6h                          ; eb 1f
    mov byte [bp-006h], 03ch                  ; c6 46 fa 3c
    jmp short 076d6h                          ; eb 19
    jmp near 07856h                           ; e9 96 01
    mov byte [bp-006h], 050h                  ; c6 46 fa 50
    jmp short 076d6h                          ; eb 10
    mov byte [bp-006h], 064h                  ; c6 46 fa 64
    jmp short 076d6h                          ; eb 0a
    mov byte [bp-006h], 0c8h                  ; c6 46 fa c8
    jmp short 076d6h                          ; eb 04
    mov byte [bp-006h], 000h                  ; c6 46 fa 00
    cmp byte [bp-006h], 000h                  ; 80 7e fa 00
    jbe short 0770ch                          ; 76 30
    mov ax, 000f3h                            ; b8 f3 00
    call 073a7h                               ; e8 c5 fc
    test al, al                               ; 84 c0
    jne short 07701h                          ; 75 1b
    mov dx, ss                                ; 8c d2
    lea ax, [bp-004h]                         ; 8d 46 fc
    call 073dfh                               ; e8 f1 fc
    mov al, byte [bp-006h]                    ; 8a 46 fa
    xor ah, ah                                ; 30 e4
    call 073a7h                               ; e8 b1 fc
    mov dx, ss                                ; 8c d2
    lea ax, [bp-004h]                         ; 8d 46 fc
    call 073dfh                               ; e8 e1 fc
    jmp near 078d8h                           ; e9 d7 01
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 003h                  ; c6 46 13 03
    jmp near 078d8h                           ; e9 cc 01
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 002h                  ; c6 46 13 02
    jmp near 078d8h                           ; e9 c1 01
    cmp byte [bp+00dh], 004h                  ; 80 7e 0d 04
    jnc short 0777fh                          ; 73 62
    mov ax, 000e8h                            ; b8 e8 00
    call 073a7h                               ; e8 84 fc
    test al, al                               ; 84 c0
    jne short 07775h                          ; 75 4e
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 073dfh                               ; e8 b0 fc
    cmp byte [bp-006h], 0fah                  ; 80 7e fa fa
    je short 07749h                           ; 74 14
    mov al, byte [bp-006h]                    ; 8a 46 fa
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 0092eh                            ; b8 2e 09
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 30 a2
    add sp, strict byte 00006h                ; 83 c4 06
    mov al, byte [bp+00dh]                    ; 8a 46 0d
    xor ah, ah                                ; 30 e4
    call 073a7h                               ; e8 56 fc
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 073dfh                               ; e8 86 fc
    cmp byte [bp-006h], 0fah                  ; 80 7e fa fa
    je short 077aeh                           ; 74 4f
    mov al, byte [bp-006h]                    ; 8a 46 fa
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 0092eh                            ; b8 2e 09
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 06 a2
    add sp, strict byte 00006h                ; 83 c4 06
    jmp short 077aeh                          ; eb 39
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 003h                  ; c6 46 13 03
    jmp short 077aeh                          ; eb 2f
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 002h                  ; c6 46 13 02
    jmp short 077aeh                          ; eb 25
    mov ax, 000f2h                            ; b8 f2 00
    call 073a7h                               ; e8 18 fc
    test al, al                               ; 84 c0
    jne short 077a6h                          ; 75 13
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 073dfh                               ; e8 44 fc
    mov dx, ss                                ; 8c d2
    lea ax, [bp-004h]                         ; 8d 46 fc
    call 073dfh                               ; e8 3c fc
    jmp near 07679h                           ; e9 d3 fe
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 003h                  ; c6 46 13 03
    jmp near 078d8h                           ; e9 27 01
    mov al, byte [bp+00dh]                    ; 8a 46 0d
    test al, al                               ; 84 c0
    jbe short 077c1h                          ; 76 09
    cmp AL, strict byte 002h                  ; 3c 02
    jbe short 077bfh                          ; 76 03
    jmp near 07860h                           ; e9 a1 00
    jmp short 0782bh                          ; eb 6a
    mov ax, 000e9h                            ; b8 e9 00
    call 073a7h                               ; e8 e0 fb
    test al, al                               ; 84 c0
    jne short 07834h                          ; 75 69
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 073dfh                               ; e8 0c fc
    mov cl, al                                ; 88 c1
    cmp byte [bp-006h], 0fah                  ; 80 7e fa fa
    je short 077efh                           ; 74 14
    mov al, byte [bp-006h]                    ; 8a 46 fa
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 0092eh                            ; b8 2e 09
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 8a a1
    add sp, strict byte 00006h                ; 83 c4 06
    test cl, cl                               ; 84 c9
    jne short 07856h                          ; 75 63
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 073dfh                               ; e8 e4 fb
    test al, al                               ; 84 c0
    jne short 07856h                          ; 75 57
    mov dx, ss                                ; 8c d2
    lea ax, [bp-004h]                         ; 8d 46 fc
    call 073dfh                               ; e8 d8 fb
    test al, al                               ; 84 c0
    jne short 07856h                          ; 75 4b
    mov dx, ss                                ; 8c d2
    lea ax, [bp-008h]                         ; 8d 46 f8
    call 073dfh                               ; e8 cc fb
    test al, al                               ; 84 c0
    jne short 07856h                          ; 75 3f
    mov al, byte [bp-006h]                    ; 8a 46 fa
    mov byte [bp+00ch], al                    ; 88 46 0c
    mov al, byte [bp-004h]                    ; 8a 46 fc
    mov byte [bp+010h], al                    ; 88 46 10
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov byte [bp+00eh], al                    ; 88 46 0e
    jmp short 077aeh                          ; eb 83
    cmp AL, strict byte 001h                  ; 3c 01
    jne short 07836h                          ; 75 07
    mov ax, 000e6h                            ; b8 e6 00
    jmp short 07839h                          ; eb 05
    jmp short 07856h                          ; eb 20
    mov ax, 000e7h                            ; b8 e7 00
    call 073a7h                               ; e8 6b fb
    mov cl, al                                ; 88 c1
    test cl, cl                               ; 84 c9
    jne short 07852h                          ; 75 10
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 073dfh                               ; e8 95 fb
    cmp byte [bp-006h], 0fah                  ; 80 7e fa fa
    je short 07852h                           ; 74 02
    mov CL, strict byte 001h                  ; b1 01
    test cl, cl                               ; 84 c9
    je short 078c0h                           ; 74 6a
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 003h                  ; c6 46 13 03
    jmp short 078c0h                          ; eb 60
    mov al, byte [bp+00dh]                    ; 8a 46 0d
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 0095ah                            ; b8 5a 09
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 05 a1
    add sp, strict byte 00006h                ; 83 c4 06
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 001h                  ; c6 46 13 01
    jmp short 078d8h                          ; eb 5a
    mov si, word [bp+00ch]                    ; 8b 76 0c
    mov bx, si                                ; 89 f3
    mov dx, strict word 00022h                ; ba 22 00
    mov ax, cx                                ; 89 c8
    call 0167ch                               ; e8 f1 9d
    mov bx, word [bp+014h]                    ; 8b 5e 14
    mov dx, strict word 00024h                ; ba 24 00
    mov ax, cx                                ; 89 c8
    call 0167ch                               ; e8 e6 9d
    mov dx, strict word 00027h                ; ba 27 00
    mov ax, cx                                ; 89 c8
    call 01652h                               ; e8 b4 9d
    mov ah, al                                ; 88 c4
    test si, si                               ; 85 f6
    jne short 078b2h                          ; 75 0e
    cmp word [bp+014h], strict byte 00000h    ; 83 7e 14 00
    jne short 078b2h                          ; 75 08
    test AL, strict byte 080h                 ; a8 80
    je short 078b4h                           ; 74 06
    and AL, strict byte 07fh                  ; 24 7f
    jmp short 078b4h                          ; eb 02
    or AL, strict byte 080h                   ; 0c 80
    mov bl, al                                ; 88 c3
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00027h                ; ba 27 00
    mov ax, cx                                ; 89 c8
    call 01660h                               ; e8 a0 9d
    jmp short 078d8h                          ; eb 16
    mov ax, 00974h                            ; b8 74 09
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 a9 a0
    add sp, strict byte 00004h                ; 83 c4 04
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 001h                  ; c6 46 13 01
    mov ax, strict word 00047h                ; b8 47 00
    call 0743ch                               ; e8 5e fb
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
_int17_function:                             ; 0xf78e4 LB 0xac
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push ax                                   ; 50
    sti                                       ; fb
    mov dx, word [bp+00eh]                    ; 8b 56 0e
    sal dx, 1                                 ; d1 e2
    add dx, strict byte 00008h                ; 83 c2 08
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 76 9d
    mov bx, ax                                ; 89 c3
    mov si, ax                                ; 89 c6
    cmp byte [bp+013h], 003h                  ; 80 7e 13 03
    jnc short 0790eh                          ; 73 0c
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    cmp ax, strict word 00003h                ; 3d 03 00
    jnc short 0790eh                          ; 73 04
    test bx, bx                               ; 85 db
    jnbe short 07911h                         ; 77 03
    jmp near 07986h                           ; e9 75 00
    mov dx, ax                                ; 89 c2
    add dx, strict byte 00078h                ; 83 c2 78
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 36 9d
    mov ch, al                                ; 88 c5
    xor cl, cl                                ; 30 c9
    cmp byte [bp+013h], 000h                  ; 80 7e 13 00
    jne short 07952h                          ; 75 2c
    mov al, byte [bp+012h]                    ; 8a 46 12
    mov dx, bx                                ; 89 da
    out DX, AL                                ; ee
    lea dx, [bx+002h]                         ; 8d 57 02
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov word [bp-004h], ax                    ; 89 46 fc
    mov al, byte [bp-004h]                    ; 8a 46 fc
    or AL, strict byte 001h                   ; 0c 01
    out DX, AL                                ; ee
    mov al, byte [bp-004h]                    ; 8a 46 fc
    and AL, strict byte 0feh                  ; 24 fe
    out DX, AL                                ; ee
    lea dx, [si+001h]                         ; 8d 54 01
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 040h                 ; a8 40
    je short 07952h                           ; 74 07
    test cx, cx                               ; 85 c9
    je short 07952h                           ; 74 03
    dec cx                                    ; 49
    jmp short 07941h                          ; eb ef
    cmp byte [bp+013h], 001h                  ; 80 7e 13 01
    jne short 0796dh                          ; 75 15
    lea dx, [si+002h]                         ; 8d 54 02
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov word [bp-004h], ax                    ; 89 46 fc
    mov al, byte [bp-004h]                    ; 8a 46 fc
    and AL, strict byte 0fbh                  ; 24 fb
    out DX, AL                                ; ee
    mov al, byte [bp-004h]                    ; 8a 46 fc
    or AL, strict byte 004h                   ; 0c 04
    out DX, AL                                ; ee
    lea dx, [si+001h]                         ; 8d 54 01
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    xor AL, strict byte 048h                  ; 34 48
    mov byte [bp+013h], al                    ; 88 46 13
    test cx, cx                               ; 85 c9
    jne short 07980h                          ; 75 04
    or byte [bp+013h], 001h                   ; 80 4e 13 01
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    jmp short 0798ah                          ; eb 04
    or byte [bp+01ch], 001h                   ; 80 4e 1c 01
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
wait_:                                       ; 0xf7990 LB 0xb2
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 0000ah                ; 83 ec 0a
    mov si, ax                                ; 89 c6
    mov byte [bp-00ah], dl                    ; 88 56 f6
    mov byte [bp-00ch], 000h                  ; c6 46 f4 00
    pushfw                                    ; 9c
    pop ax                                    ; 58
    mov word [bp-010h], ax                    ; 89 46 f0
    sti                                       ; fb
    xor cx, cx                                ; 31 c9
    mov dx, 0046ch                            ; ba 6c 04
    xor ax, ax                                ; 31 c0
    call 0168ah                               ; e8 d7 9c
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov bx, dx                                ; 89 d3
    hlt                                       ; f4
    mov dx, 0046ch                            ; ba 6c 04
    xor ax, ax                                ; 31 c0
    call 0168ah                               ; e8 c9 9c
    mov word [bp-012h], ax                    ; 89 46 ee
    mov di, dx                                ; 89 d7
    cmp dx, bx                                ; 39 da
    jnbe short 079d1h                         ; 77 07
    jne short 079d8h                          ; 75 0c
    cmp ax, word [bp-00eh]                    ; 3b 46 f2
    jbe short 079d8h                          ; 76 07
    sub ax, word [bp-00eh]                    ; 2b 46 f2
    sbb dx, bx                                ; 19 da
    jmp short 079e3h                          ; eb 0b
    cmp dx, bx                                ; 39 da
    jc short 079e3h                           ; 72 07
    jne short 079e7h                          ; 75 09
    cmp ax, word [bp-00eh]                    ; 3b 46 f2
    jnc short 079e7h                          ; 73 04
    sub si, ax                                ; 29 c6
    sbb cx, dx                                ; 19 d1
    mov ax, word [bp-012h]                    ; 8b 46 ee
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov bx, di                                ; 89 fb
    mov ax, 00100h                            ; b8 00 01
    int 016h                                  ; cd 16
    je short 079fbh                           ; 74 05
    mov AL, strict byte 001h                  ; b0 01
    jmp near 079fdh                           ; e9 02 00
    db  032h, 0c0h
    ; xor al, al                                ; 32 c0
    test al, al                               ; 84 c0
    je short 07a27h                           ; 74 26
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    int 016h                                  ; cd 16
    xchg ah, al                               ; 86 c4
    mov dl, al                                ; 88 c2
    mov byte [bp-00ch], al                    ; 88 46 f4
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 00996h                            ; b8 96 09
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 5c 9f
    add sp, strict byte 00006h                ; 83 c4 06
    cmp byte [bp-00ah], 000h                  ; 80 7e f6 00
    je short 07a27h                           ; 74 04
    mov al, dl                                ; 88 d0
    jmp short 07a39h                          ; eb 12
    test cx, cx                               ; 85 c9
    jnle short 079b8h                         ; 7f 8d
    jne short 07a31h                          ; 75 04
    test si, si                               ; 85 f6
    jnbe short 079b8h                         ; 77 87
    mov ax, word [bp-010h]                    ; 8b 46 f0
    push ax                                   ; 50
    popfw                                     ; 9d
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    lea sp, [bp-008h]                         ; 8d 66 f8
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
read_logo_byte_:                             ; 0xf7a42 LB 0x16
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push dx                                   ; 52
    xor ah, ah                                ; 30 e4
    or ah, 001h                               ; 80 cc 01
    mov dx, 003b8h                            ; ba b8 03
    out DX, ax                                ; ef
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop dx                                    ; 5a
    pop bp                                    ; 5d
    retn                                      ; c3
read_logo_word_:                             ; 0xf7a58 LB 0x14
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push dx                                   ; 52
    xor ah, ah                                ; 30 e4
    or ah, 001h                               ; 80 cc 01
    mov dx, 003b8h                            ; ba b8 03
    out DX, ax                                ; ef
    in ax, DX                                 ; ed
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop dx                                    ; 5a
    pop bp                                    ; 5d
    retn                                      ; c3
print_detected_harddisks_:                   ; 0xf7a6c LB 0x15a
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push si                                   ; 56
    sub sp, strict byte 00006h                ; 83 ec 06
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 ef 9b
    mov si, ax                                ; 89 c6
    mov byte [bp-00eh], 000h                  ; c6 46 f2 00
    xor ch, ch                                ; 30 ed
    mov byte [bp-00ch], ch                    ; 88 6e f4
    mov dx, 00304h                            ; ba 04 03
    call 01652h                               ; e8 c2 9b
    mov byte [bp-00ah], al                    ; 88 46 f6
    xor cl, cl                                ; 30 c9
    cmp cl, byte [bp-00ah]                    ; 3a 4e f6
    jnc short 07af8h                          ; 73 5e
    mov al, cl                                ; 88 c8
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    add dx, 00305h                            ; 81 c2 05 03
    mov ax, si                                ; 89 f0
    call 01652h                               ; e8 a9 9b
    mov bl, al                                ; 88 c3
    cmp AL, strict byte 00ch                  ; 3c 0c
    jc short 07adah                           ; 72 2b
    test ch, ch                               ; 84 ed
    jne short 07ac3h                          ; 75 10
    mov ax, 009a7h                            ; b8 a7 09
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 b8 9e
    add sp, strict byte 00004h                ; 83 c4 04
    mov CH, strict byte 001h                  ; b5 01
    mov al, cl                                ; 88 c8
    xor ah, ah                                ; 30 e4
    inc ax                                    ; 40
    push ax                                   ; 50
    mov ax, 009bch                            ; b8 bc 09
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 a2 9e
    add sp, strict byte 00006h                ; 83 c4 06
    jmp near 07b8ch                           ; e9 b2 00
    cmp AL, strict byte 008h                  ; 3c 08
    jc short 07afbh                           ; 72 1d
    cmp byte [bp-00ch], 000h                  ; 80 7e f4 00
    jne short 07af6h                          ; 75 12
    mov ax, 009cfh                            ; b8 cf 09
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 87 9e
    add sp, strict byte 00004h                ; 83 c4 04
    mov byte [bp-00ch], 001h                  ; c6 46 f4 01
    jmp short 07ac3h                          ; eb cb
    jmp near 07b91h                           ; e9 96 00
    cmp AL, strict byte 004h                  ; 3c 04
    jnc short 07b19h                          ; 73 1a
    cmp byte [bp-00eh], 000h                  ; 80 7e f2 00
    jne short 07b19h                          ; 75 14
    mov ax, 009e4h                            ; b8 e4 09
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 66 9e
    add sp, strict byte 00004h                ; 83 c4 04
    mov byte [bp-00eh], 001h                  ; c6 46 f2 01
    jmp short 07b32h                          ; eb 19
    cmp bl, 004h                              ; 80 fb 04
    jc short 07b32h                           ; 72 14
    test ch, ch                               ; 84 ed
    jne short 07b32h                          ; 75 10
    mov ax, 009f6h                            ; b8 f6 09
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 49 9e
    add sp, strict byte 00004h                ; 83 c4 04
    mov CH, strict byte 001h                  ; b5 01
    mov al, cl                                ; 88 c8
    xor ah, ah                                ; 30 e4
    inc ax                                    ; 40
    push ax                                   ; 50
    mov ax, 00a0ah                            ; b8 0a 0a
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 33 9e
    add sp, strict byte 00006h                ; 83 c4 06
    cmp bl, 004h                              ; 80 fb 04
    jc short 07b4eh                           ; 72 03
    sub bl, 004h                              ; 80 eb 04
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    test ax, ax                               ; 85 c0
    je short 07b60h                           ; 74 05
    mov ax, 00a14h                            ; b8 14 0a
    jmp short 07b63h                          ; eb 03
    mov ax, 00a1fh                            ; b8 1f 0a
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 0b 9e
    add sp, strict byte 00004h                ; 83 c4 04
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    mov bx, strict word 00002h                ; bb 02 00
    cwd                                       ; 99
    idiv bx                                   ; f7 fb
    test dx, dx                               ; 85 d2
    je short 07b81h                           ; 74 05
    mov ax, 00a28h                            ; b8 28 0a
    jmp short 07b84h                          ; eb 03
    mov ax, 00a2eh                            ; b8 2e 0a
    push ax                                   ; 50
    push bx                                   ; 53
    call 01976h                               ; e8 ed 9d
    add sp, strict byte 00004h                ; 83 c4 04
    db  0feh, 0c1h
    ; inc cl                                    ; fe c1
    jmp near 07a95h                           ; e9 04 ff
    cmp byte [bp-00eh], 000h                  ; 80 7e f2 00
    jne short 07bafh                          ; 75 18
    test ch, ch                               ; 84 ed
    jne short 07bafh                          ; 75 14
    cmp byte [bp-00ch], 000h                  ; 80 7e f4 00
    jne short 07bafh                          ; 75 0e
    mov ax, 00a35h                            ; b8 35 0a
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 ca 9d
    add sp, strict byte 00004h                ; 83 c4 04
    mov ax, 00a49h                            ; b8 49 0a
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 bc 9d
    add sp, strict byte 00004h                ; 83 c4 04
    lea sp, [bp-008h]                         ; 8d 66 f8
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
get_boot_drive_:                             ; 0xf7bc6 LB 0x28
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push dx                                   ; 52
    mov bl, al                                ; 88 c3
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 98 9a
    mov dx, 00304h                            ; ba 04 03
    call 01652h                               ; e8 76 9a
    sub bl, 002h                              ; 80 eb 02
    cmp bl, al                                ; 38 c3
    jc short 07be5h                           ; 72 02
    mov BL, strict byte 0ffh                  ; b3 ff
    mov al, bl                                ; 88 d8
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop dx                                    ; 5a
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
show_logo_:                                  ; 0xf7bee LB 0x24f
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 0000eh                ; 83 ec 0e
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 6c 9a
    mov si, ax                                ; 89 c6
    mov byte [bp-00ch], 000h                  ; c6 46 f4 00
    xor cx, cx                                ; 31 c9
    mov AL, strict byte 034h                  ; b0 34
    out strict byte 043h, AL                  ; e6 43
    mov AL, strict byte 0d3h                  ; b0 d3
    out strict byte 040h, AL                  ; e6 40
    mov AL, strict byte 048h                  ; b0 48
    out strict byte 040h, AL                  ; e6 40
    mov al, cl                                ; 88 c8
    xor ah, ah                                ; 30 e4
    call 07a58h                               ; e8 3b fe
    cmp ax, 066bbh                            ; 3d bb 66
    jne short 07c34h                          ; 75 12
    push SS                                   ; 16
    pop ES                                    ; 07
    lea di, [bp-018h]                         ; 8d 7e e8
    mov ax, 04f03h                            ; b8 03 4f
    int 010h                                  ; cd 10
    mov word [es:di], bx                      ; 26 89 1d
    cmp ax, strict word 0004fh                ; 3d 4f 00
    je short 07c37h                           ; 74 03
    jmp near 07d0ch                           ; e9 d5 00
    mov al, cl                                ; 88 c8
    add AL, strict byte 004h                  ; 04 04
    xor ah, ah                                ; 30 e4
    call 07a42h                               ; e8 02 fe
    mov bl, al                                ; 88 c3
    mov byte [bp-014h], al                    ; 88 46 ec
    mov al, cl                                ; 88 c8
    add AL, strict byte 005h                  ; 04 05
    xor ah, ah                                ; 30 e4
    call 07a42h                               ; e8 f4 fd
    mov bh, al                                ; 88 c7
    mov byte [bp-012h], al                    ; 88 46 ee
    mov al, cl                                ; 88 c8
    add AL, strict byte 002h                  ; 04 02
    xor ah, ah                                ; 30 e4
    call 07a58h                               ; e8 fc fd
    mov dx, ax                                ; 89 c2
    mov word [bp-016h], ax                    ; 89 46 ea
    mov al, cl                                ; 88 c8
    add AL, strict byte 006h                  ; 04 06
    xor ah, ah                                ; 30 e4
    call 07a42h                               ; e8 d8 fd
    mov byte [bp-010h], al                    ; 88 46 f0
    test bl, bl                               ; 84 db
    jne short 07c79h                          ; 75 08
    test bh, bh                               ; 84 ff
    jne short 07c79h                          ; 75 04
    test dx, dx                               ; 85 d2
    je short 07c34h                           ; 74 bb
    mov bx, 00142h                            ; bb 42 01
    mov ax, 04f02h                            ; b8 02 4f
    int 010h                                  ; cd 10
    cmp byte [bp-014h], 000h                  ; 80 7e ec 00
    je short 07cach                           ; 74 25
    xor cx, cx                                ; 31 c9
    jmp short 07c91h                          ; eb 06
    inc cx                                    ; 41
    cmp cx, strict byte 00010h                ; 83 f9 10
    jnbe short 07cb3h                         ; 77 22
    mov ax, cx                                ; 89 c8
    or ah, 002h                               ; 80 cc 02
    mov dx, 003b8h                            ; ba b8 03
    out DX, ax                                ; ef
    xor dx, dx                                ; 31 d2
    mov ax, strict word 00001h                ; b8 01 00
    call 07990h                               ; e8 ee fc
    cmp AL, strict byte 086h                  ; 3c 86
    jne short 07c8bh                          ; 75 e5
    mov byte [bp-00ch], 001h                  ; c6 46 f4 01
    jmp short 07cb3h                          ; eb 07
    mov ax, 00210h                            ; b8 10 02
    mov dx, 003b8h                            ; ba b8 03
    out DX, ax                                ; ef
    cmp byte [bp-00ch], 000h                  ; 80 7e f4 00
    jne short 07cceh                          ; 75 15
    mov CL, strict byte 004h                  ; b1 04
    mov ax, word [bp-016h]                    ; 8b 46 ea
    shr ax, CL                                ; d3 e8
    mov dx, strict word 00001h                ; ba 01 00
    call 07990h                               ; e8 ca fc
    cmp AL, strict byte 086h                  ; 3c 86
    jne short 07cceh                          ; 75 04
    mov byte [bp-00ch], 001h                  ; c6 46 f4 01
    cmp byte [bp-012h], 000h                  ; 80 7e ee 00
    je short 07cffh                           ; 74 2b
    cmp byte [bp-00ch], 000h                  ; 80 7e f4 00
    jne short 07cffh                          ; 75 25
    mov cx, strict word 00010h                ; b9 10 00
    jmp short 07ce4h                          ; eb 05
    dec cx                                    ; 49
    test cx, cx                               ; 85 c9
    jbe short 07d0ch                          ; 76 28
    mov ax, cx                                ; 89 c8
    or ah, 002h                               ; 80 cc 02
    mov dx, 003b8h                            ; ba b8 03
    out DX, ax                                ; ef
    xor dx, dx                                ; 31 d2
    mov ax, strict word 00001h                ; b8 01 00
    call 07990h                               ; e8 9b fc
    cmp AL, strict byte 086h                  ; 3c 86
    jne short 07cdfh                          ; 75 e6
    mov byte [bp-00ch], 001h                  ; c6 46 f4 01
    jmp short 07d0ch                          ; eb 0d
    cmp byte [bp-00ch], 000h                  ; 80 7e f4 00
    jne short 07d0ch                          ; 75 07
    mov ax, 00200h                            ; b8 00 02
    mov dx, 003b8h                            ; ba b8 03
    out DX, ax                                ; ef
    xor bx, bx                                ; 31 db
    mov dx, 0037dh                            ; ba 7d 03
    mov ax, si                                ; 89 f0
    call 01660h                               ; e8 4a 99
    mov AL, strict byte 003h                  ; b0 03
    mov AH, strict byte 000h                  ; b4 00
    int 010h                                  ; cd 10
    cmp byte [bp-010h], 000h                  ; 80 7e f0 00
    je short 07d36h                           ; 74 14
    cmp byte [bp-014h], 000h                  ; 80 7e ec 00
    jne short 07d64h                          ; 75 3c
    cmp byte [bp-012h], 000h                  ; 80 7e ee 00
    jne short 07d64h                          ; 75 36
    cmp word [bp-016h], strict byte 00000h    ; 83 7e ea 00
    je short 07d39h                           ; 74 05
    jmp short 07d64h                          ; eb 2e
    jmp near 07e20h                           ; e9 e7 00
    cmp byte [bp-010h], 002h                  ; 80 7e f0 02
    jne short 07d4dh                          ; 75 0e
    mov ax, 00a4bh                            ; b8 4b 0a
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 2c 9c
    add sp, strict byte 00004h                ; 83 c4 04
    cmp byte [bp-00ch], 000h                  ; 80 7e f4 00
    jne short 07d64h                          ; 75 11
    mov dx, strict word 00001h                ; ba 01 00
    mov ax, 000c0h                            ; b8 c0 00
    call 07990h                               ; e8 34 fc
    cmp AL, strict byte 086h                  ; 3c 86
    jne short 07d64h                          ; 75 04
    mov byte [bp-00ch], 001h                  ; c6 46 f4 01
    cmp byte [bp-00ch], 000h                  ; 80 7e f4 00
    je short 07d36h                           ; 74 cc
    mov byte [bp-00eh], 000h                  ; c6 46 f2 00
    mov ax, 00100h                            ; b8 00 01
    mov cx, 01000h                            ; b9 00 10
    int 010h                                  ; cd 10
    mov ax, 00700h                            ; b8 00 07
    mov BH, strict byte 007h                  ; b7 07
    db  033h, 0c9h
    ; xor cx, cx                                ; 33 c9
    mov dx, 0184fh                            ; ba 4f 18
    int 010h                                  ; cd 10
    mov ax, 00200h                            ; b8 00 02
    db  033h, 0dbh
    ; xor bx, bx                                ; 33 db
    db  033h, 0d2h
    ; xor dx, dx                                ; 33 d2
    int 010h                                  ; cd 10
    mov ax, 00a6dh                            ; b8 6d 0a
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 e0 9b
    add sp, strict byte 00004h                ; 83 c4 04
    call 07a6ch                               ; e8 d0 fc
    mov ax, 00ab1h                            ; b8 b1 0a
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 cf 9b
    add sp, strict byte 00004h                ; 83 c4 04
    mov dx, strict word 00001h                ; ba 01 00
    mov ax, strict word 00040h                ; b8 40 00
    call 07990h                               ; e8 dd fb
    mov cl, al                                ; 88 c1
    test al, al                               ; 84 c0
    je short 07daah                           ; 74 f1
    cmp AL, strict byte 030h                  ; 3c 30
    je short 07e0dh                           ; 74 50
    cmp cl, 002h                              ; 80 f9 02
    jc short 07de6h                           ; 72 24
    cmp cl, 009h                              ; 80 f9 09
    jnbe short 07de6h                         ; 77 1f
    mov al, cl                                ; 88 c8
    xor ah, ah                                ; 30 e4
    call 07bc6h                               ; e8 f8 fd
    cmp AL, strict byte 0ffh                  ; 3c ff
    jne short 07dd4h                          ; 75 02
    jmp short 07daah                          ; eb d6
    mov bl, al                                ; 88 c3
    xor bh, bh                                ; 30 ff
    mov dx, 0037ch                            ; ba 7c 03
    mov ax, si                                ; 89 f0
    call 01660h                               ; e8 80 98
    mov byte [bp-00eh], 002h                  ; c6 46 f2 02
    jmp short 07e0dh                          ; eb 27
    cmp cl, 02eh                              ; 80 f9 2e
    je short 07dfbh                           ; 74 10
    cmp cl, 026h                              ; 80 f9 26
    je short 07e01h                           ; 74 11
    cmp cl, 021h                              ; 80 f9 21
    jne short 07e07h                          ; 75 12
    mov byte [bp-00eh], 001h                  ; c6 46 f2 01
    jmp short 07e0dh                          ; eb 12
    mov byte [bp-00eh], 003h                  ; c6 46 f2 03
    jmp short 07e0dh                          ; eb 0c
    mov byte [bp-00eh], 004h                  ; c6 46 f2 04
    jmp short 07e0dh                          ; eb 06
    cmp byte [bp-00eh], 000h                  ; 80 7e f2 00
    je short 07daah                           ; 74 9d
    mov bl, byte [bp-00eh]                    ; 8a 5e f2
    xor bh, bh                                ; 30 ff
    mov dx, 0037dh                            ; ba 7d 03
    mov ax, si                                ; 89 f0
    call 01660h                               ; e8 46 98
    mov AL, strict byte 003h                  ; b0 03
    mov AH, strict byte 000h                  ; b4 00
    int 010h                                  ; cd 10
    mov AL, strict byte 034h                  ; b0 34
    out strict byte 043h, AL                  ; e6 43
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    out strict byte 040h, AL                  ; e6 40
    out strict byte 040h, AL                  ; e6 40
    push bp                                   ; 55
    push DS                                   ; 1e
    mov ds, ax                                ; 8e d8
    call 0ede3h                               ; e8 b2 6f
    pop DS                                    ; 1f
    pop bp                                    ; 5d
    lea sp, [bp-00ah]                         ; 8d 66 f6
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
delay_boot_:                                 ; 0xf7e3d LB 0x72
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push si                                   ; 56
    push di                                   ; 57
    mov bx, ax                                ; 89 c3
    test ax, ax                               ; 85 c0
    je short 07ea5h                           ; 74 5a
    mov AL, strict byte 034h                  ; b0 34
    out strict byte 043h, AL                  ; e6 43
    mov AL, strict byte 0d3h                  ; b0 d3
    out strict byte 040h, AL                  ; e6 40
    mov AL, strict byte 048h                  ; b0 48
    out strict byte 040h, AL                  ; e6 40
    push bx                                   ; 53
    mov ax, 00afbh                            ; b8 fb 0a
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 13 9b
    add sp, strict byte 00006h                ; 83 c4 06
    test bx, bx                               ; 85 db
    jbe short 07e84h                          ; 76 1a
    push bx                                   ; 53
    mov ax, 00b19h                            ; b8 19 0b
    push ax                                   ; 50
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 01976h                               ; e8 00 9b
    add sp, strict byte 00006h                ; 83 c4 06
    xor dx, dx                                ; 31 d2
    mov ax, strict word 00040h                ; b8 40 00
    call 07990h                               ; e8 0f fb
    dec bx                                    ; 4b
    jmp short 07e66h                          ; eb e2
    mov bx, 00a49h                            ; bb 49 0a
    push bx                                   ; 53
    mov bx, strict word 00002h                ; bb 02 00
    push bx                                   ; 53
    call 01976h                               ; e8 e7 9a
    add sp, strict byte 00004h                ; 83 c4 04
    mov AL, strict byte 034h                  ; b0 34
    out strict byte 043h, AL                  ; e6 43
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    out strict byte 040h, AL                  ; e6 40
    out strict byte 040h, AL                  ; e6 40
    push bp                                   ; 55
    push DS                                   ; 1e
    mov ds, ax                                ; 8e d8
    call 0ede3h                               ; e8 40 6f
    pop DS                                    ; 1f
    pop bp                                    ; 5d
    lea sp, [bp-00ah]                         ; 8d 66 f6
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
scsi_cmd_data_in_:                           ; 0xf7eaf LB 0xd5
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00006h                ; 83 ec 06
    mov si, ax                                ; 89 c6
    mov byte [bp-006h], dl                    ; 88 56 fa
    mov word [bp-00ah], bx                    ; 89 5e f6
    mov word [bp-008h], cx                    ; 89 4e f8
    mov bx, word [bp+00ah]                    ; 8b 5e 0a
    mov dx, si                                ; 89 f2
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 07ec5h                          ; 75 f7
    mov al, byte [bp+004h]                    ; 8a 46 04
    cmp AL, strict byte 010h                  ; 3c 10
    jne short 07ed9h                          ; 75 04
    xor ax, ax                                ; 31 c0
    jmp short 07edbh                          ; eb 02
    xor ah, ah                                ; 30 e4
    mov di, ax                                ; 89 c7
    mov ax, bx                                ; 89 d8
    mov dx, word [bp+00ch]                    ; 8b 56 0c
    mov cx, strict word 0000ch                ; b9 0c 00
    shr dx, 1                                 ; d1 ea
    rcr ax, 1                                 ; d1 d8
    loop 07ee5h                               ; e2 fa
    mov cx, ax                                ; 89 c1
    and cx, 000f0h                            ; 81 e1 f0 00
    or cx, di                                 ; 09 f9
    mov al, byte [bp-006h]                    ; 8a 46 fa
    mov dx, si                                ; 89 f2
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    out DX, AL                                ; ee
    mov al, cl                                ; 88 c8
    out DX, AL                                ; ee
    mov al, bl                                ; 88 d8
    out DX, AL                                ; ee
    mov ax, bx                                ; 89 d8
    mov dx, word [bp+00ch]                    ; 8b 56 0c
    mov cx, strict word 00008h                ; b9 08 00
    shr dx, 1                                 ; d1 ea
    rcr ax, 1                                 ; d1 d8
    loop 07f0ah                               ; e2 fa
    mov dx, si                                ; 89 f2
    out DX, AL                                ; ee
    xor cx, cx                                ; 31 c9
    mov al, byte [bp+004h]                    ; 8a 46 04
    xor ah, ah                                ; 30 e4
    cmp cx, ax                                ; 39 c1
    jnc short 07f2ch                          ; 73 0e
    les di, [bp-00ah]                         ; c4 7e f6
    add di, cx                                ; 01 cf
    mov al, byte [es:di]                      ; 26 8a 05
    mov dx, si                                ; 89 f2
    out DX, AL                                ; ee
    inc cx                                    ; 41
    jmp short 07f15h                          ; eb e9
    mov dx, si                                ; 89 f2
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 07f2ch                          ; 75 f7
    test AL, strict byte 002h                 ; a8 02
    je short 07f47h                           ; 74 0e
    lea dx, [si+003h]                         ; 8d 54 03
    xor al, al                                ; 30 c0
    out DX, AL                                ; ee
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov di, strict word 00004h                ; bf 04 00
    jmp short 07f79h                          ; eb 32
    lea dx, [si+001h]                         ; 8d 54 01
    cmp word [bp+00ch], strict byte 00000h    ; 83 7e 0c 00
    jne short 07f56h                          ; 75 06
    cmp bx, 08000h                            ; 81 fb 00 80
    jbe short 07f70h                          ; 76 1a
    mov cx, 08000h                            ; b9 00 80
    les di, [bp+006h]                         ; c4 7e 06
    rep insb                                  ; f3 6c
    add bx, 08000h                            ; 81 c3 00 80
    adc word [bp+00ch], strict byte 0ffffh    ; 83 56 0c ff
    mov ax, es                                ; 8c c0
    add ax, 00800h                            ; 05 00 08
    mov word [bp+008h], ax                    ; 89 46 08
    jmp short 07f47h                          ; eb d7
    mov cx, bx                                ; 89 d9
    les di, [bp+006h]                         ; c4 7e 06
    rep insb                                  ; f3 6c
    xor di, di                                ; 31 ff
    mov ax, di                                ; 89 f8
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 0000ah                               ; c2 0a 00
scsi_cmd_data_out_:                          ; 0xf7f84 LB 0xd5
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00006h                ; 83 ec 06
    mov di, ax                                ; 89 c7
    mov byte [bp-006h], dl                    ; 88 56 fa
    mov word [bp-00ah], bx                    ; 89 5e f6
    mov word [bp-008h], cx                    ; 89 4e f8
    mov bx, word [bp+00ah]                    ; 8b 5e 0a
    mov dx, di                                ; 89 fa
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 07f9ah                          ; 75 f7
    mov al, byte [bp+004h]                    ; 8a 46 04
    cmp AL, strict byte 010h                  ; 3c 10
    jne short 07faeh                          ; 75 04
    xor ax, ax                                ; 31 c0
    jmp short 07fb0h                          ; eb 02
    xor ah, ah                                ; 30 e4
    mov si, ax                                ; 89 c6
    mov ax, bx                                ; 89 d8
    mov dx, word [bp+00ch]                    ; 8b 56 0c
    mov cx, strict word 0000ch                ; b9 0c 00
    shr dx, 1                                 ; d1 ea
    rcr ax, 1                                 ; d1 d8
    loop 07fbah                               ; e2 fa
    mov cx, ax                                ; 89 c1
    and cx, 000f0h                            ; 81 e1 f0 00
    or cx, si                                 ; 09 f1
    mov al, byte [bp-006h]                    ; 8a 46 fa
    mov dx, di                                ; 89 fa
    out DX, AL                                ; ee
    mov AL, strict byte 001h                  ; b0 01
    out DX, AL                                ; ee
    mov al, cl                                ; 88 c8
    out DX, AL                                ; ee
    mov al, bl                                ; 88 d8
    out DX, AL                                ; ee
    mov ax, bx                                ; 89 d8
    mov dx, word [bp+00ch]                    ; 8b 56 0c
    mov cx, strict word 00008h                ; b9 08 00
    shr dx, 1                                 ; d1 ea
    rcr ax, 1                                 ; d1 d8
    loop 07fdfh                               ; e2 fa
    mov dx, di                                ; 89 fa
    out DX, AL                                ; ee
    xor cx, cx                                ; 31 c9
    mov al, byte [bp+004h]                    ; 8a 46 04
    xor ah, ah                                ; 30 e4
    cmp cx, ax                                ; 39 c1
    jnc short 08001h                          ; 73 0e
    les si, [bp-00ah]                         ; c4 76 f6
    add si, cx                                ; 01 ce
    mov al, byte [es:si]                      ; 26 8a 04
    mov dx, di                                ; 89 fa
    out DX, AL                                ; ee
    inc cx                                    ; 41
    jmp short 07feah                          ; eb e9
    lea dx, [di+001h]                         ; 8d 55 01
    cmp word [bp+00ch], strict byte 00000h    ; 83 7e 0c 00
    jne short 08010h                          ; 75 06
    cmp bx, 08000h                            ; 81 fb 00 80
    jbe short 0802bh                          ; 76 1b
    mov cx, 08000h                            ; b9 00 80
    les si, [bp+006h]                         ; c4 76 06
    db  0f3h, 026h, 06eh
    ; rep es outsb                              ; f3 26 6e
    add bx, 08000h                            ; 81 c3 00 80
    adc word [bp+00ch], strict byte 0ffffh    ; 83 56 0c ff
    mov ax, es                                ; 8c c0
    add ax, 00800h                            ; 05 00 08
    mov word [bp+008h], ax                    ; 89 46 08
    jmp short 08001h                          ; eb d6
    mov cx, bx                                ; 89 d9
    les si, [bp+006h]                         ; c4 76 06
    db  0f3h, 026h, 06eh
    ; rep es outsb                              ; f3 26 6e
    mov dx, di                                ; 89 fa
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 08033h                          ; 75 f7
    test AL, strict byte 002h                 ; a8 02
    je short 0804eh                           ; 74 0e
    lea dx, [di+003h]                         ; 8d 55 03
    xor al, al                                ; 30 c0
    out DX, AL                                ; ee
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov ax, strict word 00004h                ; b8 04 00
    jmp short 08050h                          ; eb 02
    xor ax, ax                                ; 31 c0
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 0000ah                               ; c2 0a 00
@scsi_read_sectors:                          ; 0xf8059 LB 0xe9
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00016h                ; 83 ec 16
    mov si, word [bp+004h]                    ; 8b 76 04
    mov es, [bp+006h]                         ; 8e 46 06
    mov al, byte [es:si+00ch]                 ; 26 8a 44 0c
    sub AL, strict byte 008h                  ; 2c 08
    mov byte [bp-006h], al                    ; 88 46 fa
    cmp AL, strict byte 004h                  ; 3c 04
    jbe short 08089h                          ; 76 15
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 00b1eh                            ; b8 1e 0b
    push ax                                   ; 50
    mov ax, 00b30h                            ; b8 30 0b
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 f0 98
    add sp, strict byte 00008h                ; 83 c4 08
    mov es, [bp+006h]                         ; 8e 46 06
    mov di, word [es:si+00eh]                 ; 26 8b 7c 0e
    mov word [bp-01ah], 00088h                ; c7 46 e6 88 00
    mov ax, word [es:si+006h]                 ; 26 8b 44 06
    mov bx, word [es:si+004h]                 ; 26 8b 5c 04
    mov cx, word [es:si+002h]                 ; 26 8b 4c 02
    mov dx, word [es:si]                      ; 26 8b 14
    xchg ah, al                               ; 86 c4
    xchg bh, bl                               ; 86 df
    xchg ch, cl                               ; 86 cd
    xchg dh, dl                               ; 86 d6
    xchg dx, ax                               ; 92
    xchg bx, cx                               ; 87 cb
    mov word [bp-012h], ax                    ; 89 46 ee
    mov word [bp-014h], bx                    ; 89 5e ec
    mov word [bp-016h], cx                    ; 89 4e ea
    mov word [bp-018h], dx                    ; 89 56 e8
    mov byte [bp-00ch], 000h                  ; c6 46 f4 00
    mov ax, di                                ; 89 f8
    xor dx, dx                                ; 31 d2
    xchg ah, al                               ; 86 c4
    xchg dh, dl                               ; 86 d6
    xchg dx, ax                               ; 92
    mov word [bp-010h], ax                    ; 89 46 f0
    mov word [bp-00eh], dx                    ; 89 56 f2
    mov byte [bp-00bh], 000h                  ; c6 46 f5 00
    mov al, byte [bp-006h]                    ; 8a 46 fa
    xor ah, ah                                ; 30 e4
    sal ax, 1                                 ; d1 e0
    sal ax, 1                                 ; d1 e0
    mov bx, si                                ; 89 f3
    add bx, ax                                ; 01 c3
    mov ax, word [es:bx+0021ch]               ; 26 8b 87 1c 02
    mov bl, byte [es:bx+0021eh]               ; 26 8a 9f 1e 02
    mov word [bp-00ah], di                    ; 89 7e f6
    mov word [bp-008h], strict word 00000h    ; c7 46 f8 00 00
    mov cx, strict word 00009h                ; b9 09 00
    sal word [bp-00ah], 1                     ; d1 66 f6
    rcl word [bp-008h], 1                     ; d1 56 f8
    loop 080f4h                               ; e2 f8
    push word [bp-008h]                       ; ff 76 f8
    push word [bp-00ah]                       ; ff 76 f6
    push word [es:si+00ah]                    ; 26 ff 74 0a
    push word [es:si+008h]                    ; 26 ff 74 08
    mov dx, strict word 00010h                ; ba 10 00
    push dx                                   ; 52
    mov dl, bl                                ; 88 da
    xor dh, dh                                ; 30 f6
    mov cx, ss                                ; 8c d1
    lea bx, [bp-01ah]                         ; 8d 5e e6
    call 07eafh                               ; e8 95 fd
    mov ah, al                                ; 88 c4
    test al, al                               ; 84 c0
    jne short 08135h                          ; 75 15
    mov es, [bp+006h]                         ; 8e 46 06
    mov word [es:si+018h], di                 ; 26 89 7c 18
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    mov word [es:si+01ah], dx                 ; 26 89 54 1a
    mov dx, word [bp-008h]                    ; 8b 56 f8
    mov word [es:si+01ch], dx                 ; 26 89 54 1c
    mov al, ah                                ; 88 e0
    xor ah, ah                                ; 30 e4
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 00004h                               ; c2 04 00
@scsi_write_sectors:                         ; 0xf8142 LB 0xe9
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00016h                ; 83 ec 16
    mov si, word [bp+004h]                    ; 8b 76 04
    mov es, [bp+006h]                         ; 8e 46 06
    mov al, byte [es:si+00ch]                 ; 26 8a 44 0c
    sub AL, strict byte 008h                  ; 2c 08
    mov byte [bp-006h], al                    ; 88 46 fa
    cmp AL, strict byte 004h                  ; 3c 04
    jbe short 08172h                          ; 76 15
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 00b4fh                            ; b8 4f 0b
    push ax                                   ; 50
    mov ax, 00b30h                            ; b8 30 0b
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 07 98
    add sp, strict byte 00008h                ; 83 c4 08
    mov es, [bp+006h]                         ; 8e 46 06
    mov di, word [es:si+00eh]                 ; 26 8b 7c 0e
    mov word [bp-01ah], 0008ah                ; c7 46 e6 8a 00
    mov ax, word [es:si+006h]                 ; 26 8b 44 06
    mov bx, word [es:si+004h]                 ; 26 8b 5c 04
    mov cx, word [es:si+002h]                 ; 26 8b 4c 02
    mov dx, word [es:si]                      ; 26 8b 14
    xchg ah, al                               ; 86 c4
    xchg bh, bl                               ; 86 df
    xchg ch, cl                               ; 86 cd
    xchg dh, dl                               ; 86 d6
    xchg dx, ax                               ; 92
    xchg bx, cx                               ; 87 cb
    mov word [bp-012h], ax                    ; 89 46 ee
    mov word [bp-014h], bx                    ; 89 5e ec
    mov word [bp-016h], cx                    ; 89 4e ea
    mov word [bp-018h], dx                    ; 89 56 e8
    mov byte [bp-00ch], 000h                  ; c6 46 f4 00
    mov ax, di                                ; 89 f8
    xor dx, dx                                ; 31 d2
    xchg ah, al                               ; 86 c4
    xchg dh, dl                               ; 86 d6
    xchg dx, ax                               ; 92
    mov word [bp-010h], ax                    ; 89 46 f0
    mov word [bp-00eh], dx                    ; 89 56 f2
    mov byte [bp-00bh], 000h                  ; c6 46 f5 00
    mov al, byte [bp-006h]                    ; 8a 46 fa
    xor ah, ah                                ; 30 e4
    sal ax, 1                                 ; d1 e0
    sal ax, 1                                 ; d1 e0
    mov bx, si                                ; 89 f3
    add bx, ax                                ; 01 c3
    mov ax, word [es:bx+0021ch]               ; 26 8b 87 1c 02
    mov bl, byte [es:bx+0021eh]               ; 26 8a 9f 1e 02
    mov word [bp-00ah], di                    ; 89 7e f6
    mov word [bp-008h], strict word 00000h    ; c7 46 f8 00 00
    mov cx, strict word 00009h                ; b9 09 00
    sal word [bp-00ah], 1                     ; d1 66 f6
    rcl word [bp-008h], 1                     ; d1 56 f8
    loop 081ddh                               ; e2 f8
    push word [bp-008h]                       ; ff 76 f8
    push word [bp-00ah]                       ; ff 76 f6
    push word [es:si+00ah]                    ; 26 ff 74 0a
    push word [es:si+008h]                    ; 26 ff 74 08
    mov dx, strict word 00010h                ; ba 10 00
    push dx                                   ; 52
    mov dl, bl                                ; 88 da
    xor dh, dh                                ; 30 f6
    mov cx, ss                                ; 8c d1
    lea bx, [bp-01ah]                         ; 8d 5e e6
    call 07f84h                               ; e8 81 fd
    mov ah, al                                ; 88 c4
    test al, al                               ; 84 c0
    jne short 0821eh                          ; 75 15
    mov es, [bp+006h]                         ; 8e 46 06
    mov word [es:si+018h], di                 ; 26 89 7c 18
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    mov word [es:si+01ah], dx                 ; 26 89 54 1a
    mov dx, word [bp-008h]                    ; 8b 56 f8
    mov word [es:si+01ch], dx                 ; 26 89 54 1c
    mov al, ah                                ; 88 e0
    xor ah, ah                                ; 30 e4
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 00004h                               ; c2 04 00
scsi_cmd_packet_:                            ; 0xf822b LB 0x170
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 0000ch                ; 83 ec 0c
    mov di, ax                                ; 89 c7
    mov byte [bp-006h], dl                    ; 88 56 fa
    mov word [bp-00eh], bx                    ; 89 5e f2
    mov word [bp-00ah], cx                    ; 89 4e f6
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 27 94
    mov si, 00122h                            ; be 22 01
    mov word [bp-00ch], ax                    ; 89 46 f4
    cmp byte [bp+00ah], 002h                  ; 80 7e 0a 02
    jne short 08276h                          ; 75 23
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 d5 96
    mov ax, 00b62h                            ; b8 62 0b
    push ax                                   ; 50
    mov ax, 00b72h                            ; b8 72 0b
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 09 97
    add sp, strict byte 00006h                ; 83 c4 06
    mov dx, strict word 00001h                ; ba 01 00
    jmp near 08390h                           ; e9 1a 01
    sub di, strict byte 00008h                ; 83 ef 08
    sal di, 1                                 ; d1 e7
    sal di, 1                                 ; d1 e7
    sub byte [bp-006h], 002h                  ; 80 6e fa 02
    mov es, [bp-00ch]                         ; 8e 46 f4
    add di, si                                ; 01 f7
    mov bx, word [es:di+0021ch]               ; 26 8b 9d 1c 02
    mov al, byte [es:di+0021eh]               ; 26 8a 85 1e 02
    mov byte [bp-008h], al                    ; 88 46 f8
    mov dx, bx                                ; 89 da
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 08293h                          ; 75 f7
    xor dx, bx                                ; 31 da
    mov ax, word [bp+006h]                    ; 8b 46 06
    add ax, word [bp+004h]                    ; 03 46 04
    mov cx, word [bp+008h]                    ; 8b 4e 08
    adc cx, dx                                ; 11 d1
    mov es, [bp-00ch]                         ; 8e 46 f4
    mov dx, word [es:si+020h]                 ; 26 8b 54 20
    xor di, di                                ; 31 ff
    add ax, dx                                ; 01 d0
    mov word [bp-010h], ax                    ; 89 46 f0
    adc di, cx                                ; 11 cf
    mov dx, di                                ; 89 fa
    mov cx, strict word 0000ch                ; b9 0c 00
    shr dx, 1                                 ; d1 ea
    rcr ax, 1                                 ; d1 d8
    loop 082beh                               ; e2 fa
    and ax, 000f0h                            ; 25 f0 00
    mov cl, byte [bp-006h]                    ; 8a 4e fa
    xor ch, ch                                ; 30 ed
    or cx, ax                                 ; 09 c1
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov dx, bx                                ; 89 da
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    out DX, AL                                ; ee
    mov al, cl                                ; 88 c8
    out DX, AL                                ; ee
    mov al, byte [bp-010h]                    ; 8a 46 f0
    out DX, AL                                ; ee
    mov ax, word [bp-010h]                    ; 8b 46 f0
    mov dx, di                                ; 89 fa
    mov cx, strict word 00008h                ; b9 08 00
    shr dx, 1                                 ; d1 ea
    rcr ax, 1                                 ; d1 d8
    loop 082e6h                               ; e2 fa
    mov dx, bx                                ; 89 da
    out DX, AL                                ; ee
    xor cx, cx                                ; 31 c9
    mov al, byte [bp-006h]                    ; 8a 46 fa
    xor ah, ah                                ; 30 e4
    cmp cx, ax                                ; 39 c1
    jnc short 0830bh                          ; 73 11
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov di, word [bp-00eh]                    ; 8b 7e f2
    add di, cx                                ; 01 cf
    mov al, byte [es:di]                      ; 26 8a 05
    mov dx, bx                                ; 89 da
    out DX, AL                                ; ee
    inc cx                                    ; 41
    jmp short 082f1h                          ; eb e6
    mov dx, bx                                ; 89 da
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 0830bh                          ; 75 f7
    test AL, strict byte 002h                 ; a8 02
    je short 08326h                           ; 74 0e
    lea dx, [bx+003h]                         ; 8d 57 03
    xor al, al                                ; 30 c0
    out DX, AL                                ; ee
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov dx, strict word 00003h                ; ba 03 00
    jmp short 08390h                          ; eb 6a
    mov ax, word [bp+004h]                    ; 8b 46 04
    test ax, ax                               ; 85 c0
    je short 08335h                           ; 74 08
    lea dx, [bx+001h]                         ; 8d 57 01
    mov cx, ax                                ; 89 c1
    in AL, DX                                 ; ec
    loop 08332h                               ; e2 fd
    mov ax, word [bp+006h]                    ; 8b 46 06
    mov es, [bp-00ch]                         ; 8e 46 f4
    mov word [es:si+01ah], ax                 ; 26 89 44 1a
    mov ax, word [bp+008h]                    ; 8b 46 08
    mov word [es:si+01ch], ax                 ; 26 89 44 1c
    lea ax, [bx+001h]                         ; 8d 47 01
    cmp word [bp+008h], strict byte 00000h    ; 83 7e 08 00
    jne short 08356h                          ; 75 07
    cmp word [bp+006h], 08000h                ; 81 7e 06 00 80
    jbe short 08373h                          ; 76 1d
    mov dx, ax                                ; 89 c2
    mov cx, 08000h                            ; b9 00 80
    les di, [bp+00ch]                         ; c4 7e 0c
    rep insb                                  ; f3 6c
    add word [bp+006h], 08000h                ; 81 46 06 00 80
    adc word [bp+008h], strict byte 0ffffh    ; 83 56 08 ff
    mov ax, es                                ; 8c c0
    add ax, 00800h                            ; 05 00 08
    mov word [bp+00eh], ax                    ; 89 46 0e
    jmp short 08346h                          ; eb d3
    mov dx, ax                                ; 89 c2
    mov cx, word [bp+006h]                    ; 8b 4e 06
    les di, [bp+00ch]                         ; c4 7e 0c
    rep insb                                  ; f3 6c
    mov es, [bp-00ch]                         ; 8e 46 f4
    cmp word [es:si+020h], strict byte 00000h ; 26 83 7c 20 00
    je short 0838eh                           ; 74 07
    mov cx, word [es:si+020h]                 ; 26 8b 4c 20
    in AL, DX                                 ; ec
    loop 0838bh                               ; e2 fd
    xor dx, dx                                ; 31 d2
    mov ax, dx                                ; 89 d0
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 0000ch                               ; c2 0c 00
scsi_enumerate_attached_devices_:            ; 0xf839b LB 0x4cb
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push si                                   ; 56
    push di                                   ; 57
    sub sp, 0023eh                            ; 81 ec 3e 02
    push ax                                   ; 50
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 bd 92
    mov di, 00122h                            ; bf 22 01
    mov word [bp-016h], ax                    ; 89 46 ea
    mov word [bp-014h], strict word 00000h    ; c7 46 ec 00 00
    jmp near 087d9h                           ; e9 1a 04
    cmp AL, strict byte 004h                  ; 3c 04
    jc short 083c6h                           ; 72 03
    jmp near 0885ch                           ; e9 96 04
    mov cx, strict word 00010h                ; b9 10 00
    xor bx, bx                                ; 31 db
    mov dx, ss                                ; 8c d2
    lea ax, [bp-048h]                         ; 8d 46 b8
    call 0a1c0h                               ; e8 ed 1d
    mov byte [bp-048h], 09eh                  ; c6 46 b8 9e
    mov byte [bp-047h], 010h                  ; c6 46 b9 10
    mov byte [bp-03bh], 020h                  ; c6 46 c5 20
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    mov ax, strict word 00020h                ; b8 20 00
    push ax                                   ; 50
    lea dx, [bp-00248h]                       ; 8d 96 b8 fd
    push SS                                   ; 16
    push dx                                   ; 52
    mov ax, strict word 00010h                ; b8 10 00
    push ax                                   ; 50
    mov dl, byte [bp-014h]                    ; 8a 56 ec
    xor dh, dh                                ; 30 f6
    mov cx, ss                                ; 8c d1
    lea bx, [bp-048h]                         ; 8d 5e b8
    mov ax, word [bp-0024ah]                  ; 8b 86 b6 fd
    call 07eafh                               ; e8 ae fa
    test al, al                               ; 84 c0
    je short 08417h                           ; 74 12
    mov ax, 00b92h                            ; b8 92 0b
    push ax                                   ; 50
    mov ax, 00bcbh                            ; b8 cb 0b
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 62 95
    add sp, strict byte 00006h                ; 83 c4 06
    mov ax, word [bp-00242h]                  ; 8b 86 be fd
    mov bx, word [bp-00244h]                  ; 8b 9e bc fd
    mov cx, word [bp-00246h]                  ; 8b 8e ba fd
    mov dx, word [bp-00248h]                  ; 8b 96 b8 fd
    xchg ah, al                               ; 86 c4
    xchg bh, bl                               ; 86 df
    xchg ch, cl                               ; 86 cd
    xchg dh, dl                               ; 86 d6
    xchg dx, ax                               ; 92
    xchg bx, cx                               ; 87 cb
    add dx, strict byte 00001h                ; 83 c2 01
    mov word [bp-024h], dx                    ; 89 56 dc
    adc cx, strict byte 00000h                ; 83 d1 00
    mov word [bp-012h], cx                    ; 89 4e ee
    adc bx, strict byte 00000h                ; 83 d3 00
    mov word [bp-022h], bx                    ; 89 5e de
    adc ax, strict word 00000h                ; 15 00 00
    mov word [bp-020h], ax                    ; 89 46 e0
    mov dh, byte [bp-00240h]                  ; 8a b6 c0 fd
    xor dl, dl                                ; 30 d2
    mov al, byte [bp-0023fh]                  ; 8a 86 c1 fd
    xor ah, ah                                ; 30 e4
    xor bx, bx                                ; 31 db
    mov si, dx                                ; 89 d6
    or si, ax                                 ; 09 c6
    mov al, byte [bp-0023eh]                  ; 8a 86 c2 fd
    xor dh, dh                                ; 30 f6
    mov cx, strict word 00008h                ; b9 08 00
    sal ax, 1                                 ; d1 e0
    rcl dx, 1                                 ; d1 d2
    loop 08465h                               ; e2 fa
    or bx, ax                                 ; 09 c3
    or dx, si                                 ; 09 f2
    mov al, byte [bp-0023dh]                  ; 8a 86 c3 fd
    xor ah, ah                                ; 30 e4
    or bx, ax                                 ; 09 c3
    mov word [bp-01ch], bx                    ; 89 5e e4
    test dx, dx                               ; 85 d2
    jne short 08484h                          ; 75 06
    cmp bx, 00200h                            ; 81 fb 00 02
    je short 084a7h                           ; 74 23
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 a4 94
    push dx                                   ; 52
    push word [bp-01ch]                       ; ff 76 e4
    push word [bp-014h]                       ; ff 76 ec
    mov ax, 00beah                            ; b8 ea 0b
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 d5 94
    add sp, strict byte 0000ah                ; 83 c4 0a
    jmp near 087cdh                           ; e9 26 03
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    cmp AL, strict byte 001h                  ; 3c 01
    jc short 084bah                           ; 72 0c
    jbe short 084c2h                          ; 76 12
    cmp AL, strict byte 003h                  ; 3c 03
    je short 084cah                           ; 74 16
    cmp AL, strict byte 002h                  ; 3c 02
    je short 084c6h                           ; 74 0e
    jmp short 08511h                          ; eb 57
    test al, al                               ; 84 c0
    jne short 08511h                          ; 75 53
    mov BL, strict byte 090h                  ; b3 90
    jmp short 084cch                          ; eb 0a
    mov BL, strict byte 098h                  ; b3 98
    jmp short 084cch                          ; eb 06
    mov BL, strict byte 0a0h                  ; b3 a0
    jmp short 084cch                          ; eb 02
    mov BL, strict byte 0a8h                  ; b3 a8
    mov cl, bl                                ; 88 d9
    add cl, 007h                              ; 80 c1 07
    xor ch, ch                                ; 30 ed
    mov ax, cx                                ; 89 c8
    call 016aeh                               ; e8 d6 91
    test al, al                               ; 84 c0
    je short 08511h                           ; 74 35
    mov al, bl                                ; 88 d8
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    xor ah, ah                                ; 30 e4
    call 016aeh                               ; e8 c9 91
    mov dh, al                                ; 88 c6
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    call 016aeh                               ; e8 c0 91
    mov ah, dh                                ; 88 f4
    cwd                                       ; 99
    mov si, ax                                ; 89 c6
    mov word [bp-026h], dx                    ; 89 56 da
    mov al, bl                                ; 88 d8
    add AL, strict byte 002h                  ; 04 02
    xor ah, ah                                ; 30 e4
    call 016aeh                               ; e8 af 91
    xor ah, ah                                ; 30 e4
    mov word [bp-028h], ax                    ; 89 46 d8
    mov ax, cx                                ; 89 c8
    call 016aeh                               ; e8 a5 91
    xor ah, ah                                ; 30 e4
    mov word [bp-01ah], ax                    ; 89 46 e6
    jmp near 085feh                           ; e9 ed 00
    mov ax, word [bp-020h]                    ; 8b 46 e0
    mov bx, word [bp-022h]                    ; 8b 5e de
    mov cx, word [bp-012h]                    ; 8b 4e ee
    mov dx, word [bp-024h]                    ; 8b 56 dc
    mov si, strict word 0000ch                ; be 0c 00
    call 0a1a0h                               ; e8 7d 1c
    mov word [bp-02ah], ax                    ; 89 46 d6
    mov word [bp-02ch], bx                    ; 89 5e d4
    mov word [bp-010h], cx                    ; 89 4e f0
    mov word [bp-018h], dx                    ; 89 56 e8
    mov ax, word [bp-020h]                    ; 8b 46 e0
    test ax, ax                               ; 85 c0
    jnbe short 0854bh                         ; 77 15
    je short 0853bh                           ; 74 03
    jmp near 085beh                           ; e9 83 00
    cmp word [bp-022h], strict byte 00000h    ; 83 7e de 00
    jnbe short 0854bh                         ; 77 0a
    jne short 08538h                          ; 75 f5
    cmp word [bp-012h], strict byte 00040h    ; 83 7e ee 40
    jnbe short 0854bh                         ; 77 02
    jne short 085beh                          ; 75 73
    mov word [bp-028h], 000ffh                ; c7 46 d8 ff 00
    mov word [bp-01ah], strict word 0003fh    ; c7 46 e6 3f 00
    mov bx, word [bp-022h]                    ; 8b 5e de
    mov cx, word [bp-012h]                    ; 8b 4e ee
    mov dx, word [bp-024h]                    ; 8b 56 dc
    mov si, strict word 00006h                ; be 06 00
    call 0a1a0h                               ; e8 3c 1c
    mov si, word [bp-018h]                    ; 8b 76 e8
    add si, dx                                ; 01 d6
    mov word [bp-01eh], si                    ; 89 76 e2
    mov dx, word [bp-010h]                    ; 8b 56 f0
    adc dx, cx                                ; 11 ca
    mov word [bp-036h], dx                    ; 89 56 ca
    mov dx, word [bp-02ch]                    ; 8b 56 d4
    adc dx, bx                                ; 11 da
    mov word [bp-038h], dx                    ; 89 56 c8
    mov dx, word [bp-02ah]                    ; 8b 56 d6
    adc dx, ax                                ; 11 c2
    mov word [bp-034h], dx                    ; 89 56 cc
    mov ax, dx                                ; 89 d0
    mov bx, word [bp-038h]                    ; 8b 5e c8
    mov cx, word [bp-036h]                    ; 8b 4e ca
    mov dx, si                                ; 89 f2
    mov si, strict word 00008h                ; be 08 00
    call 0a1a0h                               ; e8 0c 1c
    mov word [bp-02eh], bx                    ; 89 5e d2
    mov word [bp-030h], cx                    ; 89 4e d0
    mov word [bp-032h], dx                    ; 89 56 ce
    mov ax, word [bp-034h]                    ; 8b 46 cc
    mov bx, word [bp-038h]                    ; 8b 5e c8
    mov cx, word [bp-036h]                    ; 8b 4e ca
    mov dx, word [bp-01eh]                    ; 8b 56 e2
    mov si, strict word 00010h                ; be 10 00
    call 0a1a0h                               ; e8 f1 1b
    mov si, word [bp-032h]                    ; 8b 76 ce
    add si, dx                                ; 01 d6
    adc cx, word [bp-030h]                    ; 13 4e d0
    mov ax, word [bp-02eh]                    ; 8b 46 d2
    adc ax, bx                                ; 11 d8
    jmp short 085fbh                          ; eb 3d
    test ax, ax                               ; 85 c0
    jnbe short 085d4h                         ; 77 12
    jne short 085e0h                          ; 75 1c
    cmp word [bp-022h], strict byte 00000h    ; 83 7e de 00
    jnbe short 085d4h                         ; 77 0a
    jne short 085e0h                          ; 75 14
    cmp word [bp-012h], strict byte 00020h    ; 83 7e ee 20
    jnbe short 085d4h                         ; 77 02
    jne short 085e0h                          ; 75 0c
    mov word [bp-028h], 00080h                ; c7 46 d8 80 00
    mov word [bp-01ah], strict word 00020h    ; c7 46 e6 20 00
    jmp short 085f9h                          ; eb 19
    mov word [bp-028h], strict word 00040h    ; c7 46 d8 40 00
    mov word [bp-01ah], strict word 00020h    ; c7 46 e6 20 00
    mov bx, word [bp-022h]                    ; 8b 5e de
    mov cx, word [bp-012h]                    ; 8b 4e ee
    mov dx, word [bp-024h]                    ; 8b 56 dc
    mov si, strict word 0000bh                ; be 0b 00
    call 0a1a0h                               ; e8 a7 1b
    mov si, dx                                ; 89 d6
    mov word [bp-026h], cx                    ; 89 4e da
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    add AL, strict byte 008h                  ; 04 08
    mov byte [bp-00eh], al                    ; 88 46 f2
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    xor ah, ah                                ; 30 e4
    sal ax, 1                                 ; d1 e0
    sal ax, 1                                 ; d1 e0
    mov es, [bp-016h]                         ; 8e 46 ea
    mov bx, di                                ; 89 fb
    add bx, ax                                ; 01 c3
    mov ax, word [bp-0024ah]                  ; 8b 86 b6 fd
    mov word [es:bx+0021ch], ax               ; 26 89 87 1c 02
    mov al, byte [bp-014h]                    ; 8a 46 ec
    mov byte [es:bx+0021eh], al               ; 26 88 87 1e 02
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov bx, di                                ; 89 fb
    add bx, ax                                ; 01 c3
    mov word [es:bx+022h], 0ff04h             ; 26 c7 47 22 04 ff
    mov word [es:bx+024h], strict word 00000h ; 26 c7 47 24 00 00
    mov ax, word [bp-01ch]                    ; 8b 46 e4
    mov word [es:bx+028h], ax                 ; 26 89 47 28
    mov byte [es:bx+027h], 001h               ; 26 c6 47 27 01
    mov ax, word [bp-028h]                    ; 8b 46 d8
    mov word [es:bx+02ah], ax                 ; 26 89 47 2a
    mov ax, word [bp-01ah]                    ; 8b 46 e6
    mov word [es:bx+02eh], ax                 ; 26 89 47 2e
    mov ax, word [bp-028h]                    ; 8b 46 d8
    mov word [es:bx+030h], ax                 ; 26 89 47 30
    mov ax, word [bp-01ah]                    ; 8b 46 e6
    mov word [es:bx+034h], ax                 ; 26 89 47 34
    cmp word [bp-026h], strict byte 00000h    ; 83 7e da 00
    jne short 08675h                          ; 75 06
    cmp si, 00400h                            ; 81 fe 00 04
    jbe short 08683h                          ; 76 0e
    mov word [es:bx+02ch], 00400h             ; 26 c7 47 2c 00 04
    mov word [es:bx+032h], 00400h             ; 26 c7 47 32 00 04
    jmp short 0868bh                          ; eb 08
    mov word [es:bx+02ch], si                 ; 26 89 77 2c
    mov word [es:bx+032h], si                 ; 26 89 77 32
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 9d 92
    push word [bp-020h]                       ; ff 76 e0
    push word [bp-022h]                       ; ff 76 de
    push word [bp-012h]                       ; ff 76 ee
    push word [bp-024h]                       ; ff 76 dc
    push word [bp-01ah]                       ; ff 76 e6
    push word [bp-028h]                       ; ff 76 d8
    push word [bp-026h]                       ; ff 76 da
    push si                                   ; 56
    push word [bp-014h]                       ; ff 76 ec
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 00c18h                            ; b8 18 0c
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 b6 92
    add sp, strict byte 00018h                ; 83 c4 18
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov es, [bp-016h]                         ; 8e 46 ea
    mov bx, di                                ; 89 fb
    add bx, ax                                ; 01 c3
    mov ax, word [bp-020h]                    ; 8b 46 e0
    mov word [es:bx+03ch], ax                 ; 26 89 47 3c
    mov ax, word [bp-022h]                    ; 8b 46 de
    mov word [es:bx+03ah], ax                 ; 26 89 47 3a
    mov ax, word [bp-012h]                    ; 8b 46 ee
    mov word [es:bx+038h], ax                 ; 26 89 47 38
    mov ax, word [bp-024h]                    ; 8b 46 dc
    mov word [es:bx+036h], ax                 ; 26 89 47 36
    mov al, byte [es:di+001e2h]               ; 26 8a 85 e2 01
    mov ah, byte [bp-00ch]                    ; 8a 66 f4
    add ah, 008h                              ; 80 c4 08
    mov bl, al                                ; 88 c3
    xor bh, bh                                ; 30 ff
    add bx, di                                ; 01 fb
    mov byte [es:bx+001e3h], ah               ; 26 88 a7 e3 01
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    mov byte [es:di+001e2h], al               ; 26 88 85 e2 01
    mov dx, strict word 00075h                ; ba 75 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 3c 8f
    mov bl, al                                ; 88 c3
    db  0feh, 0c3h
    ; inc bl                                    ; fe c3
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00075h                ; ba 75 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 3b 8f
    inc byte [bp-00ch]                        ; fe 46 f4
    jmp near 087c2h                           ; e9 97 00
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 fd 91
    push word [bp-014h]                       ; ff 76 ec
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, 00c46h                            ; b8 46 0c
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 2c 92
    add sp, strict byte 00008h                ; 83 c4 08
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    add AL, strict byte 008h                  ; 04 08
    mov byte [bp-00eh], al                    ; 88 46 f2
    test byte [bp-00247h], 080h               ; f6 86 b9 fd 80
    je short 08761h                           ; 74 05
    mov cx, strict word 00001h                ; b9 01 00
    jmp short 08763h                          ; eb 02
    xor cx, cx                                ; 31 c9
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    xor ah, ah                                ; 30 e4
    sal ax, 1                                 ; d1 e0
    sal ax, 1                                 ; d1 e0
    mov es, [bp-016h]                         ; 8e 46 ea
    mov bx, di                                ; 89 fb
    add bx, ax                                ; 01 c3
    mov ax, word [bp-0024ah]                  ; 8b 86 b6 fd
    mov word [es:bx+0021ch], ax               ; 26 89 87 1c 02
    mov al, byte [bp-014h]                    ; 8a 46 ec
    mov byte [es:bx+0021eh], al               ; 26 88 87 1e 02
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov bx, di                                ; 89 fb
    add bx, ax                                ; 01 c3
    mov word [es:bx+022h], 00504h             ; 26 c7 47 22 04 05
    mov byte [es:bx+024h], cl                 ; 26 88 4f 24
    mov word [es:bx+028h], 00800h             ; 26 c7 47 28 00 08
    mov al, byte [es:di+001f3h]               ; 26 8a 85 f3 01
    mov ah, byte [bp-00ch]                    ; 8a 66 f4
    add ah, 008h                              ; 80 c4 08
    mov bl, al                                ; 88 c3
    xor bh, bh                                ; 30 ff
    add bx, di                                ; 01 fb
    mov byte [es:bx+001f4h], ah               ; 26 88 a7 f4 01
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    mov byte [es:di+001f3h], al               ; 26 88 85 f3 01
    inc byte [bp-00ch]                        ; fe 46 f4
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    mov es, [bp-016h]                         ; 8e 46 ea
    mov byte [es:di+0022ch], al               ; 26 88 85 2c 02
    inc word [bp-014h]                        ; ff 46 ec
    cmp word [bp-014h], strict byte 00010h    ; 83 7e ec 10
    jl short 087d9h                           ; 7c 03
    jmp near 0885ch                           ; e9 83 00
    mov byte [bp-048h], 012h                  ; c6 46 b8 12
    xor al, al                                ; 30 c0
    mov byte [bp-047h], al                    ; 88 46 b9
    mov byte [bp-046h], al                    ; 88 46 ba
    mov byte [bp-045h], al                    ; 88 46 bb
    mov byte [bp-044h], 005h                  ; c6 46 bc 05
    mov byte [bp-043h], al                    ; 88 46 bd
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, strict word 00005h                ; b8 05 00
    push ax                                   ; 50
    lea dx, [bp-00248h]                       ; 8d 96 b8 fd
    push SS                                   ; 16
    push dx                                   ; 52
    mov ax, strict word 00006h                ; b8 06 00
    push ax                                   ; 50
    mov dl, byte [bp-014h]                    ; 8a 56 ec
    xor dh, dh                                ; 30 f6
    mov cx, ss                                ; 8c d1
    lea bx, [bp-048h]                         ; 8d 5e b8
    mov ax, word [bp-0024ah]                  ; 8b 86 b6 fd
    call 07eafh                               ; e8 9e f6
    test al, al                               ; 84 c0
    je short 08827h                           ; 74 12
    mov ax, 00b92h                            ; b8 92 0b
    push ax                                   ; 50
    mov ax, 00bb2h                            ; b8 b2 0b
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 52 91
    add sp, strict byte 00006h                ; 83 c4 06
    mov es, [bp-016h]                         ; 8e 46 ea
    mov al, byte [es:di+0022ch]               ; 26 8a 85 2c 02
    mov byte [bp-00ch], al                    ; 88 46 f4
    test byte [bp-00248h], 0e0h               ; f6 86 b8 fd e0
    jne short 08843h                          ; 75 0a
    test byte [bp-00248h], 01fh               ; f6 86 b8 fd 1f
    jne short 08843h                          ; 75 03
    jmp near 083bfh                           ; e9 7c fb
    test byte [bp-00248h], 0e0h               ; f6 86 b8 fd e0
    je short 0884dh                           ; 74 03
    jmp near 087c2h                           ; e9 75 ff
    mov al, byte [bp-00248h]                  ; 8a 86 b8 fd
    and AL, strict byte 01fh                  ; 24 1f
    cmp AL, strict byte 005h                  ; 3c 05
    jne short 0885ah                          ; 75 03
    jmp near 0872bh                           ; e9 d1 fe
    jmp short 0884ah                          ; eb ee
    lea sp, [bp-00ah]                         ; 8d 66 f6
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
scsi_pci_init_:                              ; 0xf8866 LB 0x2b
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push si                                   ; 56
    call 09fa4h                               ; e8 35 17
    mov dx, ax                                ; 89 c2
    cmp ax, strict word 0ffffh                ; 3d ff ff
    je short 08889h                           ; 74 13
    mov al, dl                                ; 88 d0
    xor ah, ah                                ; 30 e4
    mov si, ax                                ; 89 c6
    mov al, dh                                ; 88 f0
    mov cx, strict word 00007h                ; b9 07 00
    mov bx, strict word 00004h                ; bb 04 00
    mov dx, si                                ; 89 f2
    call 0a052h                               ; e8 c9 17
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
_scsi_init:                                  ; 0xf8891 LB 0x81
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 d1 8d
    mov bx, 00122h                            ; bb 22 01
    mov es, ax                                ; 8e c0
    mov byte [es:bx+0022ch], 000h             ; 26 c6 87 2c 02 00
    mov AL, strict byte 055h                  ; b0 55
    mov dx, 00432h                            ; ba 32 04
    out DX, AL                                ; ee
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp AL, strict byte 055h                  ; 3c 55
    jne short 088cah                          ; 75 15
    xor al, al                                ; 30 c0
    mov dx, 00433h                            ; ba 33 04
    out DX, AL                                ; ee
    mov ax, 00430h                            ; b8 30 04
    call 0839bh                               ; e8 da fa
    mov dx, 01040h                            ; ba 40 10
    mov ax, 0104bh                            ; b8 4b 10
    call 08866h                               ; e8 9c ff
    mov AL, strict byte 055h                  ; b0 55
    mov dx, 00436h                            ; ba 36 04
    out DX, AL                                ; ee
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp AL, strict byte 055h                  ; 3c 55
    jne short 088ech                          ; 75 15
    xor al, al                                ; 30 c0
    mov dx, 00437h                            ; ba 37 04
    out DX, AL                                ; ee
    mov ax, 00434h                            ; b8 34 04
    call 0839bh                               ; e8 b8 fa
    mov dx, strict word 00030h                ; ba 30 00
    mov ax, 01000h                            ; b8 00 10
    call 08866h                               ; e8 7a ff
    mov AL, strict byte 055h                  ; b0 55
    mov dx, 0043ah                            ; ba 3a 04
    out DX, AL                                ; ee
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp AL, strict byte 055h                  ; 3c 55
    jne short 0890eh                          ; 75 15
    xor al, al                                ; 30 c0
    mov dx, 0043bh                            ; ba 3b 04
    out DX, AL                                ; ee
    mov ax, 00438h                            ; b8 38 04
    call 0839bh                               ; e8 96 fa
    mov dx, strict word 00054h                ; ba 54 00
    mov ax, 01000h                            ; b8 00 10
    call 08866h                               ; e8 58 ff
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
ahci_ctrl_extract_bits_:                     ; 0xf8912 LB 0x1c
    push si                                   ; 56
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov si, ax                                ; 89 c6
    and ax, bx                                ; 21 d8
    and dx, cx                                ; 21 ca
    mov cl, byte [bp+006h]                    ; 8a 4e 06
    xor ch, ch                                ; 30 ed
    jcxz 08929h                               ; e3 06
    shr dx, 1                                 ; d1 ea
    rcr ax, 1                                 ; d1 d8
    loop 08923h                               ; e2 fa
    pop bp                                    ; 5d
    pop si                                    ; 5e
    retn 00002h                               ; c2 02 00
ahci_addr_to_phys_:                          ; 0xf892e LB 0x1e
    push bx                                   ; 53
    push cx                                   ; 51
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov bx, ax                                ; 89 c3
    mov ax, dx                                ; 89 d0
    xor dx, dx                                ; 31 d2
    mov cx, strict word 00004h                ; b9 04 00
    sal ax, 1                                 ; d1 e0
    rcl dx, 1                                 ; d1 d2
    loop 0893ch                               ; e2 fa
    xor cx, cx                                ; 31 c9
    add ax, bx                                ; 01 d8
    adc dx, cx                                ; 11 ca
    pop bp                                    ; 5d
    pop cx                                    ; 59
    pop bx                                    ; 5b
    retn                                      ; c3
ahci_port_cmd_sync_:                         ; 0xf894c LB 0x159
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push cx                                   ; 51
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    mov si, ax                                ; 89 c6
    mov cx, dx                                ; 89 d1
    mov dl, bl                                ; 88 da
    mov es, cx                                ; 8e c1
    mov al, byte [es:si+00262h]               ; 26 8a 84 62 02
    mov byte [bp-008h], al                    ; 88 46 f8
    mov bx, word [es:si+00260h]               ; 26 8b 9c 60 02
    cmp AL, strict byte 0ffh                  ; 3c ff
    jne short 0896fh                          ; 75 03
    jmp near 08a9dh                           ; e9 2e 01
    mov al, byte [es:si+00263h]               ; 26 8a 84 63 02
    xor ah, ah                                ; 30 e4
    xor di, di                                ; 31 ff
    or di, 00080h                             ; 81 cf 80 00
    xor dh, dh                                ; 30 f6
    or di, dx                                 ; 09 d7
    mov word [es:si], di                      ; 26 89 3c
    mov word [es:si+002h], ax                 ; 26 89 44 02
    mov word [es:si+004h], strict word 00000h ; 26 c7 44 04 00 00
    mov word [es:si+006h], strict word 00000h ; 26 c7 44 06 00 00
    lea ax, [si+00080h]                       ; 8d 84 80 00
    mov dx, cx                                ; 89 ca
    call 0892eh                               ; e8 92 ff
    mov es, cx                                ; 8e c1
    mov word [es:si+008h], ax                 ; 26 89 44 08
    mov word [es:si+00ah], dx                 ; 26 89 54 0a
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 007h                  ; b1 07
    mov di, ax                                ; 89 c7
    sal di, CL                                ; d3 e7
    lea ax, [di+00118h]                       ; 8d 85 18 01
    xor cx, cx                                ; 31 c9
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea si, [bx+004h]                         ; 8d 77 04
    mov dx, si                                ; 89 f2
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    or AL, strict byte 011h                   ; 0c 11
    mov cx, dx                                ; 89 d1
    mov dx, si                                ; 89 f2
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea ax, [di+00138h]                       ; 8d 85 38 01
    cwd                                       ; 99
    mov cx, dx                                ; 89 d1
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov ax, strict word 00001h                ; b8 01 00
    xor cx, cx                                ; 31 c9
    mov dx, si                                ; 89 f2
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 007h                  ; b1 07
    sal ax, CL                                ; d3 e0
    add ax, 00110h                            ; 05 10 01
    xor cx, cx                                ; 31 c9
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea dx, [bx+004h]                         ; 8d 57 04
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    test dh, 040h                             ; f6 c6 40
    jne short 08a2fh                          ; 75 04
    test AL, strict byte 001h                 ; a8 01
    je short 08a33h                           ; 74 04
    mov AL, strict byte 001h                  ; b0 01
    jmp short 08a35h                          ; eb 02
    xor al, al                                ; 30 c0
    test al, al                               ; 84 c0
    je short 08a01h                           ; 74 c8
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 007h                  ; b1 07
    mov di, ax                                ; 89 c7
    sal di, CL                                ; d3 e7
    lea ax, [di+00110h]                       ; 8d 85 10 01
    xor cx, cx                                ; 31 c9
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea si, [bx+004h]                         ; 8d 77 04
    mov dx, si                                ; 89 f2
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    or AL, strict byte 001h                   ; 0c 01
    mov cx, dx                                ; 89 d1
    mov dx, si                                ; 89 f2
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea ax, [di+00118h]                       ; 8d 85 18 01
    xor cx, cx                                ; 31 c9
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov dx, si                                ; 89 f2
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    and AL, strict byte 0feh                  ; 24 fe
    mov cx, dx                                ; 89 d1
    mov dx, si                                ; 89 f2
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bp                                    ; 5d
    retn                                      ; c3
ahci_cmd_data_:                              ; 0xf8aa5 LB 0x262
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push cx                                   ; 51
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 0000ch                ; 83 ec 0c
    push ax                                   ; 50
    push dx                                   ; 52
    mov byte [bp-008h], bl                    ; 88 5e f8
    xor di, di                                ; 31 ff
    mov es, dx                                ; 8e c2
    mov bx, ax                                ; 89 c3
    mov ax, word [es:bx+00232h]               ; 26 8b 87 32 02
    mov word [bp-00ah], ax                    ; 89 46 f6
    mov word [bp-00eh], di                    ; 89 7e f2
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov ax, word [es:bx+00eh]                 ; 26 8b 47 0e
    mov word [bp-010h], ax                    ; 89 46 f0
    mov ax, word [es:bx+010h]                 ; 26 8b 47 10
    mov word [bp-012h], ax                    ; 89 46 ee
    mov cx, strict word 00040h                ; b9 40 00
    xor bx, bx                                ; 31 db
    mov ax, 00080h                            ; b8 80 00
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    call 0a1c0h                               ; e8 dd 16
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov word [es:di+00080h], 08027h           ; 26 c7 85 80 00 27 80
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov byte [es:di+00082h], al               ; 26 88 85 82 00
    mov byte [es:di+00083h], 000h             ; 26 c6 85 83 00 00
    mov es, [bp-016h]                         ; 8e 46 ea
    mov bx, word [bp-014h]                    ; 8b 5e ec
    mov ax, word [es:bx]                      ; 26 8b 07
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov byte [es:di+00084h], al               ; 26 88 85 84 00
    mov es, [bp-016h]                         ; 8e 46 ea
    mov ax, word [es:bx+006h]                 ; 26 8b 47 06
    mov bx, word [es:bx+004h]                 ; 26 8b 5f 04
    mov si, word [bp-014h]                    ; 8b 76 ec
    mov cx, word [es:si+002h]                 ; 26 8b 4c 02
    mov dx, word [es:si]                      ; 26 8b 14
    mov si, strict word 00008h                ; be 08 00
    call 0a1a0h                               ; e8 79 16
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov byte [es:di+00085h], dl               ; 26 88 95 85 00
    mov es, [bp-016h]                         ; 8e 46 ea
    mov bx, word [bp-014h]                    ; 8b 5e ec
    mov ax, word [es:bx+006h]                 ; 26 8b 47 06
    mov bx, word [es:bx+004h]                 ; 26 8b 5f 04
    mov si, word [bp-014h]                    ; 8b 76 ec
    mov cx, word [es:si+002h]                 ; 26 8b 4c 02
    mov dx, word [es:si]                      ; 26 8b 14
    mov si, strict word 00010h                ; be 10 00
    call 0a1a0h                               ; e8 53 16
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov byte [es:di+00086h], dl               ; 26 88 95 86 00
    mov byte [es:di+00087h], 040h             ; 26 c6 85 87 00 40
    mov es, [bp-016h]                         ; 8e 46 ea
    mov bx, word [bp-014h]                    ; 8b 5e ec
    mov ax, word [es:bx+006h]                 ; 26 8b 47 06
    mov bx, word [es:bx+004h]                 ; 26 8b 5f 04
    mov si, word [bp-014h]                    ; 8b 76 ec
    mov cx, word [es:si+002h]                 ; 26 8b 4c 02
    mov dx, word [es:si]                      ; 26 8b 14
    mov si, strict word 00018h                ; be 18 00
    call 0a1a0h                               ; e8 27 16
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov byte [es:di+00088h], dl               ; 26 88 95 88 00
    mov es, [bp-016h]                         ; 8e 46 ea
    mov bx, word [bp-014h]                    ; 8b 5e ec
    mov ax, word [es:bx+006h]                 ; 26 8b 47 06
    mov bx, word [es:bx+004h]                 ; 26 8b 5f 04
    mov si, word [bp-014h]                    ; 8b 76 ec
    mov cx, word [es:si+002h]                 ; 26 8b 4c 02
    mov dx, word [es:si]                      ; 26 8b 14
    mov si, strict word 00020h                ; be 20 00
    call 0a1a0h                               ; e8 01 16
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov byte [es:di+00089h], dl               ; 26 88 95 89 00
    mov es, [bp-016h]                         ; 8e 46 ea
    mov bx, word [bp-014h]                    ; 8b 5e ec
    mov ax, word [es:bx+006h]                 ; 26 8b 47 06
    mov bx, word [es:bx+004h]                 ; 26 8b 5f 04
    mov si, word [bp-014h]                    ; 8b 76 ec
    mov cx, word [es:si+002h]                 ; 26 8b 4c 02
    mov dx, word [es:si]                      ; 26 8b 14
    mov si, strict word 00028h                ; be 28 00
    call 0a1a0h                               ; e8 db 15
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov byte [es:di+0008ah], dl               ; 26 88 95 8a 00
    mov byte [es:di+0008bh], 000h             ; 26 c6 85 8b 00 00
    mov al, byte [bp-010h]                    ; 8a 46 f0
    mov byte [es:di+0008ch], al               ; 26 88 85 8c 00
    mov al, byte [bp-00fh]                    ; 8a 46 f1
    mov byte [es:di+0008dh], al               ; 26 88 85 8d 00
    mov word [es:di+00276h], strict word 00010h ; 26 c7 85 76 02 10 00
    mov ax, word [bp-010h]                    ; 8b 46 f0
    xor dx, dx                                ; 31 d2
    mov bx, word [bp-012h]                    ; 8b 5e ee
    xor cx, cx                                ; 31 c9
    call 0a150h                               ; e8 59 15
    push dx                                   ; 52
    push ax                                   ; 50
    mov es, [bp-016h]                         ; 8e 46 ea
    mov bx, word [bp-014h]                    ; 8b 5e ec
    mov bx, word [es:bx+008h]                 ; 26 8b 5f 08
    mov si, word [bp-014h]                    ; 8b 76 ec
    mov cx, word [es:si+00ah]                 ; 26 8b 4c 0a
    mov ax, 0026ah                            ; b8 6a 02
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    call 0a0abh                               ; e8 98 14
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov dx, word [es:di+0027eh]               ; 26 8b 95 7e 02
    add dx, strict byte 0ffffh                ; 83 c2 ff
    mov ax, word [es:di+00280h]               ; 26 8b 85 80 02
    adc ax, strict word 0ffffh                ; 15 ff ff
    mov bl, byte [es:di+00263h]               ; 26 8a 9d 63 02
    xor bh, bh                                ; 30 ff
    mov CL, strict byte 004h                  ; b1 04
    sal bx, CL                                ; d3 e3
    mov word [es:bx+0010ch], dx               ; 26 89 97 0c 01
    mov word [es:bx+0010eh], ax               ; 26 89 87 0e 01
    mov bl, byte [es:di+00263h]               ; 26 8a 9d 63 02
    xor bh, bh                                ; 30 ff
    sal bx, CL                                ; d3 e3
    mov ax, word [es:di+0027ah]               ; 26 8b 85 7a 02
    mov dx, word [es:di+0027ch]               ; 26 8b 95 7c 02
    mov word [es:bx+00100h], ax               ; 26 89 87 00 01
    mov word [es:bx+00102h], dx               ; 26 89 97 02 01
    inc byte [es:di+00263h]                   ; 26 fe 85 63 02
    mov es, [bp-016h]                         ; 8e 46 ea
    mov bx, si                                ; 89 f3
    mov ax, word [es:bx+020h]                 ; 26 8b 47 20
    test ax, ax                               ; 85 c0
    je short 08ca3h                           ; 74 39
    dec ax                                    ; 48
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov bl, byte [es:di+00263h]               ; 26 8a 9d 63 02
    xor bh, bh                                ; 30 ff
    sal bx, CL                                ; d3 e3
    mov word [es:bx+0010ch], ax               ; 26 89 87 0c 01
    mov word [es:bx+0010eh], di               ; 26 89 bf 0e 01
    mov bl, byte [es:di+00263h]               ; 26 8a 9d 63 02
    xor bh, bh                                ; 30 ff
    sal bx, CL                                ; d3 e3
    mov ax, word [es:di+00264h]               ; 26 8b 85 64 02
    mov dx, word [es:di+00266h]               ; 26 8b 95 66 02
    mov word [es:bx+00100h], ax               ; 26 89 87 00 01
    mov word [es:bx+00102h], dx               ; 26 89 97 02 01
    inc byte [es:di+00263h]                   ; 26 fe 85 63 02
    mov al, byte [bp-008h]                    ; 8a 46 f8
    cmp AL, strict byte 035h                  ; 3c 35
    jne short 08cb0h                          ; 75 06
    mov byte [bp-008h], 040h                  ; c6 46 f8 40
    jmp short 08cc7h                          ; eb 17
    cmp AL, strict byte 0a0h                  ; 3c a0
    jne short 08cc3h                          ; 75 0f
    or byte [bp-008h], 020h                   ; 80 4e f8 20
    les bx, [bp-00eh]                         ; c4 5e f2
    or byte [es:bx+00083h], 001h              ; 26 80 8f 83 00 01
    jmp short 08cc7h                          ; eb 04
    mov byte [bp-008h], 000h                  ; c6 46 f8 00
    or byte [bp-008h], 005h                   ; 80 4e f8 05
    mov bl, byte [bp-008h]                    ; 8a 5e f8
    xor bh, bh                                ; 30 ff
    mov ax, word [bp-00eh]                    ; 8b 46 f2
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    call 0894ch                               ; e8 73 fc
    mov cx, word [bp-00ch]                    ; 8b 4e f4
    mov bx, word [bp-00eh]                    ; 8b 5e f2
    add bx, 00240h                            ; 81 c3 40 02
    mov ax, word [bp-00eh]                    ; 8b 46 f2
    add ax, 0026ah                            ; 05 6a 02
    mov dx, cx                                ; 89 ca
    call 0a122h                               ; e8 34 14
    mov es, cx                                ; 8e c1
    mov al, byte [es:bx+003h]                 ; 26 8a 47 03
    test al, al                               ; 84 c0
    je short 08cfdh                           ; 74 05
    mov ax, strict word 00004h                ; b8 04 00
    jmp short 08cffh                          ; eb 02
    xor ah, ah                                ; 30 e4
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bp                                    ; 5d
    retn                                      ; c3
ahci_port_deinit_current_:                   ; 0xf8d07 LB 0x183
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00006h                ; 83 ec 06
    mov di, ax                                ; 89 c7
    mov word [bp-00eh], dx                    ; 89 56 f2
    mov es, dx                                ; 8e c2
    mov si, word [es:di+00260h]               ; 26 8b b5 60 02
    mov al, byte [es:di+00262h]               ; 26 8a 85 62 02
    mov byte [bp-00ah], al                    ; 88 46 f6
    cmp AL, strict byte 0ffh                  ; 3c ff
    je short 08d8ah                           ; 74 61
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 007h                  ; b1 07
    sal ax, CL                                ; d3 e0
    add ax, 00118h                            ; 05 18 01
    xor cx, cx                                ; 31 c9
    mov dx, si                                ; 89 f2
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea bx, [si+004h]                         ; 8d 5c 04
    mov dx, bx                                ; 89 da
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    and AL, strict byte 0eeh                  ; 24 ee
    mov cx, dx                                ; 89 d1
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 007h                  ; b1 07
    sal ax, CL                                ; d3 e0
    add ax, 00118h                            ; 05 18 01
    xor cx, cx                                ; 31 c9
    mov dx, si                                ; 89 f2
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea dx, [si+004h]                         ; 8d 54 04
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    test ax, 0c011h                           ; a9 11 c0
    je short 08d8dh                           ; 74 07
    mov AL, strict byte 001h                  ; b0 01
    jmp short 08d8fh                          ; eb 05
    jmp near 08e81h                           ; e9 f4 00
    xor al, al                                ; 30 c0
    cmp AL, strict byte 001h                  ; 3c 01
    je short 08d5ch                           ; 74 c9
    mov cx, strict word 00020h                ; b9 20 00
    xor bx, bx                                ; 31 db
    mov ax, di                                ; 89 f8
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    call 0a1c0h                               ; e8 20 14
    lea ax, [di+00080h]                       ; 8d 85 80 00
    mov cx, strict word 00040h                ; b9 40 00
    xor bx, bx                                ; 31 db
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    call 0a1c0h                               ; e8 11 14
    lea ax, [di+00200h]                       ; 8d 85 00 02
    mov cx, strict word 00060h                ; b9 60 00
    xor bx, bx                                ; 31 db
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    call 0a1c0h                               ; e8 02 14
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 007h                  ; b1 07
    sal ax, CL                                ; d3 e0
    mov word [bp-00ch], ax                    ; 89 46 f4
    add ax, 00108h                            ; 05 08 01
    cwd                                       ; 99
    mov cx, dx                                ; 89 d1
    mov dx, si                                ; 89 f2
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea bx, [si+004h]                         ; 8d 5c 04
    xor ax, ax                                ; 31 c0
    xor cx, cx                                ; 31 c9
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    add ax, 0010ch                            ; 05 0c 01
    cwd                                       ; 99
    mov cx, dx                                ; 89 d1
    mov dx, si                                ; 89 f2
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    xor ax, ax                                ; 31 c0
    xor cx, cx                                ; 31 c9
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    db  0feh, 0c4h
    ; inc ah                                    ; fe c4
    cwd                                       ; 99
    mov cx, dx                                ; 89 d1
    mov dx, si                                ; 89 f2
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    xor ax, ax                                ; 31 c0
    xor cx, cx                                ; 31 c9
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    add ax, 00104h                            ; 05 04 01
    cwd                                       ; 99
    mov cx, dx                                ; 89 d1
    mov dx, si                                ; 89 f2
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    xor ax, ax                                ; 31 c0
    xor cx, cx                                ; 31 c9
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    add ax, 00114h                            ; 05 14 01
    cwd                                       ; 99
    mov cx, dx                                ; 89 d1
    mov dx, si                                ; 89 f2
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    xor ax, ax                                ; 31 c0
    xor cx, cx                                ; 31 c9
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov es, [bp-00eh]                         ; 8e 46 f2
    mov byte [es:di+00262h], 0ffh             ; 26 c6 85 62 02 ff
    lea sp, [bp-008h]                         ; 8d 66 f8
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
ahci_port_init_:                             ; 0xf8e8a LB 0x250
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push cx                                   ; 51
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00006h                ; 83 ec 06
    mov si, ax                                ; 89 c6
    mov word [bp-00ah], dx                    ; 89 56 f6
    mov byte [bp-008h], bl                    ; 88 5e f8
    call 08d07h                               ; e8 69 fe
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 007h                  ; b1 07
    sal ax, CL                                ; d3 e0
    add ax, 00118h                            ; 05 18 01
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov bx, word [es:si+00260h]               ; 26 8b 9c 60 02
    xor cx, cx                                ; 31 c9
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    add bx, strict byte 00004h                ; 83 c3 04
    mov dx, bx                                ; 89 da
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    and AL, strict byte 0eeh                  ; 24 ee
    mov cx, dx                                ; 89 d1
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 007h                  ; b1 07
    sal ax, CL                                ; d3 e0
    add ax, 00118h                            ; 05 18 01
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov bx, word [es:si+00260h]               ; 26 8b 9c 60 02
    xor cx, cx                                ; 31 c9
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea dx, [bx+004h]                         ; 8d 57 04
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    test ax, 0c011h                           ; a9 11 c0
    je short 08f11h                           ; 74 04
    mov AL, strict byte 001h                  ; b0 01
    jmp short 08f13h                          ; eb 02
    xor al, al                                ; 30 c0
    cmp AL, strict byte 001h                  ; 3c 01
    je short 08edbh                           ; 74 c4
    mov cx, strict word 00020h                ; b9 20 00
    xor bx, bx                                ; 31 db
    mov ax, si                                ; 89 f0
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    call 0a1c0h                               ; e8 9c 12
    lea ax, [si+00080h]                       ; 8d 84 80 00
    mov cx, strict word 00040h                ; b9 40 00
    xor bx, bx                                ; 31 db
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    call 0a1c0h                               ; e8 8d 12
    lea di, [si+00200h]                       ; 8d bc 00 02
    mov cx, strict word 00060h                ; b9 60 00
    xor bx, bx                                ; 31 db
    mov ax, di                                ; 89 f8
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    call 0a1c0h                               ; e8 7c 12
    mov bl, byte [bp-008h]                    ; 8a 5e f8
    xor bh, bh                                ; 30 ff
    mov CL, strict byte 007h                  ; b1 07
    sal bx, CL                                ; d3 e3
    lea ax, [bx+00108h]                       ; 8d 87 08 01
    cwd                                       ; 99
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov cx, word [es:si+00260h]               ; 26 8b 8c 60 02
    mov word [bp-00ch], cx                    ; 89 4e f4
    mov cx, dx                                ; 89 d1
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov ax, di                                ; 89 f8
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    call 0892eh                               ; e8 bb f9
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov di, word [es:si+00260h]               ; 26 8b bc 60 02
    add di, strict byte 00004h                ; 83 c7 04
    mov cx, dx                                ; 89 d1
    mov dx, di                                ; 89 fa
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea ax, [bx+0010ch]                       ; 8d 87 0c 01
    cwd                                       ; 99
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov di, word [es:si+00260h]               ; 26 8b bc 60 02
    mov cx, dx                                ; 89 d1
    mov dx, di                                ; 89 fa
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov dx, word [es:si+00260h]               ; 26 8b 94 60 02
    add dx, strict byte 00004h                ; 83 c2 04
    xor ax, ax                                ; 31 c0
    xor cx, cx                                ; 31 c9
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea ax, [bx+00100h]                       ; 8d 87 00 01
    cwd                                       ; 99
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov di, word [es:si+00260h]               ; 26 8b bc 60 02
    mov cx, dx                                ; 89 d1
    mov dx, di                                ; 89 fa
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov ax, si                                ; 89 f0
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    call 0892eh                               ; e8 4f f9
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov di, word [es:si+00260h]               ; 26 8b bc 60 02
    add di, strict byte 00004h                ; 83 c7 04
    mov cx, dx                                ; 89 d1
    mov dx, di                                ; 89 fa
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea ax, [bx+00104h]                       ; 8d 87 04 01
    cwd                                       ; 99
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov di, word [es:si+00260h]               ; 26 8b bc 60 02
    mov cx, dx                                ; 89 d1
    mov dx, di                                ; 89 fa
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov dx, word [es:si+00260h]               ; 26 8b 94 60 02
    add dx, strict byte 00004h                ; 83 c2 04
    xor ax, ax                                ; 31 c0
    xor cx, cx                                ; 31 c9
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea ax, [bx+00114h]                       ; 8d 87 14 01
    cwd                                       ; 99
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov di, word [es:si+00260h]               ; 26 8b bc 60 02
    mov cx, dx                                ; 89 d1
    mov dx, di                                ; 89 fa
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov dx, word [es:si+00260h]               ; 26 8b 94 60 02
    add dx, strict byte 00004h                ; 83 c2 04
    xor ax, ax                                ; 31 c0
    xor cx, cx                                ; 31 c9
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea ax, [bx+00110h]                       ; 8d 87 10 01
    cwd                                       ; 99
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov di, word [es:si+00260h]               ; 26 8b bc 60 02
    mov cx, dx                                ; 89 d1
    mov dx, di                                ; 89 fa
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov dx, word [es:si+00260h]               ; 26 8b 94 60 02
    add dx, strict byte 00004h                ; 83 c2 04
    mov ax, strict word 0ffffh                ; b8 ff ff
    mov cx, ax                                ; 89 c1
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea ax, [bx+00130h]                       ; 8d 87 30 01
    cwd                                       ; 99
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov bx, word [es:si+00260h]               ; 26 8b 9c 60 02
    mov cx, dx                                ; 89 d1
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov dx, word [es:si+00260h]               ; 26 8b 94 60 02
    add dx, strict byte 00004h                ; 83 c2 04
    mov ax, strict word 0ffffh                ; b8 ff ff
    mov cx, ax                                ; 89 c1
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov byte [es:si+00262h], al               ; 26 88 84 62 02
    mov byte [es:si+00263h], 000h             ; 26 c6 84 63 02 00
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bp                                    ; 5d
    retn                                      ; c3
@ahci_read_sectors:                          ; 0xf90da LB 0xaa
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    les bx, [bp+004h]                         ; c4 5e 04
    mov al, byte [es:bx+00ch]                 ; 26 8a 47 0c
    xor ah, ah                                ; 30 e4
    mov di, ax                                ; 89 c7
    sub di, strict byte 0000ch                ; 83 ef 0c
    cmp di, strict byte 00004h                ; 83 ff 04
    jbe short 09105h                          ; 76 13
    push di                                   ; 57
    mov ax, 00c62h                            ; b8 62 0c
    push ax                                   ; 50
    mov ax, 00c74h                            ; b8 74 0c
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 74 88
    add sp, strict byte 00008h                ; 83 c4 08
    xor bx, bx                                ; 31 db
    les si, [bp+004h]                         ; c4 76 04
    mov dx, word [es:si+00232h]               ; 26 8b 94 32 02
    shr eax, 010h                             ; 66 c1 e8 10
    mov es, dx                                ; 8e c2
    mov word [es:bx+00268h], ax               ; 26 89 87 68 02
    mov es, [bp+006h]                         ; 8e 46 06
    add di, si                                ; 01 f7
    mov bl, byte [es:di+0022dh]               ; 26 8a 9d 2d 02
    xor bh, bh                                ; 30 ff
    mov dx, word [es:si+00232h]               ; 26 8b 94 32 02
    xor ax, ax                                ; 31 c0
    call 08e8ah                               ; e8 5a fd
    mov bx, strict word 00025h                ; bb 25 00
    mov ax, si                                ; 89 f0
    mov dx, word [bp+006h]                    ; 8b 56 06
    call 08aa5h                               ; e8 6a f9
    mov bx, ax                                ; 89 c3
    mov es, [bp+006h]                         ; 8e 46 06
    mov ax, word [es:si+00eh]                 ; 26 8b 44 0e
    mov word [es:si+018h], ax                 ; 26 89 44 18
    mov CL, strict byte 009h                  ; b1 09
    sal ax, CL                                ; d3 e0
    mov cx, ax                                ; 89 c1
    shr cx, 1                                 ; d1 e9
    mov di, si                                ; 89 f7
    mov di, word [es:di+008h]                 ; 26 8b 7d 08
    mov ax, word [es:si+00ah]                 ; 26 8b 44 0a
    mov si, di                                ; 89 fe
    mov dx, ax                                ; 89 c2
    mov es, ax                                ; 8e c0
    push DS                                   ; 1e
    mov ds, dx                                ; 8e da
    rep movsw                                 ; f3 a5
    pop DS                                    ; 1f
    xor di, di                                ; 31 ff
    les si, [bp+004h]                         ; c4 76 04
    mov es, [es:si+00232h]                    ; 26 8e 84 32 02
    mov ax, word [es:di+00268h]               ; 26 8b 85 68 02
    sal eax, 010h                             ; 66 c1 e0 10
    mov ax, bx                                ; 89 d8
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 00004h                               ; c2 04 00
@ahci_write_sectors:                         ; 0xf9184 LB 0x88
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    mov si, word [bp+004h]                    ; 8b 76 04
    mov cx, word [bp+006h]                    ; 8b 4e 06
    mov es, cx                                ; 8e c1
    mov bl, byte [es:si+00ch]                 ; 26 8a 5c 0c
    xor bh, bh                                ; 30 ff
    sub bx, strict byte 0000ch                ; 83 eb 0c
    cmp bx, strict byte 00004h                ; 83 fb 04
    jbe short 091b2h                          ; 76 13
    push bx                                   ; 53
    mov ax, 00c93h                            ; b8 93 0c
    push ax                                   ; 50
    mov ax, 00c74h                            ; b8 74 0c
    push ax                                   ; 50
    mov ax, strict word 00007h                ; b8 07 00
    push ax                                   ; 50
    call 01976h                               ; e8 c7 87
    add sp, strict byte 00008h                ; 83 c4 08
    xor di, di                                ; 31 ff
    mov es, cx                                ; 8e c1
    mov dx, word [es:si+00232h]               ; 26 8b 94 32 02
    shr eax, 010h                             ; 66 c1 e8 10
    mov es, dx                                ; 8e c2
    mov word [es:di+00268h], ax               ; 26 89 85 68 02
    mov es, cx                                ; 8e c1
    add bx, si                                ; 01 f3
    mov bl, byte [es:bx+0022dh]               ; 26 8a 9f 2d 02
    xor bh, bh                                ; 30 ff
    mov dx, word [es:si+00232h]               ; 26 8b 94 32 02
    xor ax, ax                                ; 31 c0
    call 08e8ah                               ; e8 af fc
    mov bx, strict word 00035h                ; bb 35 00
    mov ax, si                                ; 89 f0
    mov dx, cx                                ; 89 ca
    call 08aa5h                               ; e8 c0 f8
    mov dx, ax                                ; 89 c2
    mov es, cx                                ; 8e c1
    mov ax, word [es:si+00eh]                 ; 26 8b 44 0e
    mov word [es:si+018h], ax                 ; 26 89 44 18
    xor bx, bx                                ; 31 db
    mov es, [es:si+00232h]                    ; 26 8e 84 32 02
    mov ax, word [es:bx+00268h]               ; 26 8b 87 68 02
    sal eax, 010h                             ; 66 c1 e0 10
    mov ax, dx                                ; 89 d0
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 00004h                               ; c2 04 00
ahci_cmd_packet_:                            ; 0xf920c LB 0x18c
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 0000eh                ; 83 ec 0e
    push ax                                   ; 50
    mov byte [bp-006h], dl                    ; 88 56 fa
    mov word [bp-012h], bx                    ; 89 5e ee
    mov word [bp-010h], cx                    ; 89 4e f0
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 47 84
    mov si, 00122h                            ; be 22 01
    mov word [bp-008h], ax                    ; 89 46 f8
    cmp byte [bp+00ah], 002h                  ; 80 7e 0a 02
    jne short 09256h                          ; 75 23
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 f5 86
    mov ax, 00ca6h                            ; b8 a6 0c
    push ax                                   ; 50
    mov ax, 00cb6h                            ; b8 b6 0c
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 29 87
    add sp, strict byte 00006h                ; 83 c4 06
    mov ax, strict word 00001h                ; b8 01 00
    jmp near 0938fh                           ; e9 39 01
    test byte [bp+004h], 001h                 ; f6 46 04 01
    jne short 09250h                          ; 75 f4
    mov bx, word [bp+006h]                    ; 8b 5e 06
    mov di, word [bp+008h]                    ; 8b 7e 08
    mov cx, strict word 00008h                ; b9 08 00
    sal bx, 1                                 ; d1 e3
    rcl di, 1                                 ; d1 d7
    loop 09265h                               ; e2 fa
    mov es, [bp-008h]                         ; 8e 46 f8
    mov word [es:si], bx                      ; 26 89 1c
    mov word [es:si+002h], di                 ; 26 89 7c 02
    mov word [es:si+004h], strict word 00000h ; 26 c7 44 04 00 00
    mov word [es:si+006h], strict word 00000h ; 26 c7 44 06 00 00
    mov ax, word [bp+00ch]                    ; 8b 46 0c
    mov word [es:si+008h], ax                 ; 26 89 44 08
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    mov word [es:si+00ah], ax                 ; 26 89 44 0a
    mov bx, word [es:si+010h]                 ; 26 8b 5c 10
    mov ax, word [bp+006h]                    ; 8b 46 06
    mov dx, word [bp+008h]                    ; 8b 56 08
    xor cx, cx                                ; 31 c9
    call 0a170h                               ; e8 d2 0e
    mov word [es:si+00eh], ax                 ; 26 89 44 0e
    xor di, di                                ; 31 ff
    mov ax, word [es:si+00232h]               ; 26 8b 84 32 02
    mov word [bp-00ah], ax                    ; 89 46 f6
    mov word [bp-00eh], di                    ; 89 7e f2
    mov word [bp-00ch], ax                    ; 89 46 f4
    sub word [bp-014h], strict byte 0000ch    ; 83 6e ec 0c
    shr eax, 010h                             ; 66 c1 e8 10
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov word [es:di+00268h], ax               ; 26 89 85 68 02
    mov es, [bp-008h]                         ; 8e 46 f8
    mov bx, word [bp-014h]                    ; 8b 5e ec
    add bx, si                                ; 01 f3
    mov bl, byte [es:bx+0022dh]               ; 26 8a 9f 2d 02
    xor bh, bh                                ; 30 ff
    mov dx, word [es:si+00232h]               ; 26 8b 94 32 02
    xor ax, ax                                ; 31 c0
    call 08e8ah                               ; e8 af fb
    mov al, byte [bp-006h]                    ; 8a 46 fa
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov bx, word [bp-012h]                    ; 8b 5e ee
    mov cx, word [bp-010h]                    ; 8b 4e f0
    mov ax, 000c0h                            ; b8 c0 00
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    call 0a1d0h                               ; e8 e0 0e
    mov es, [bp-008h]                         ; 8e 46 f8
    mov word [es:si+018h], di                 ; 26 89 7c 18
    mov word [es:si+01ah], di                 ; 26 89 7c 1a
    mov word [es:si+01ch], di                 ; 26 89 7c 1c
    mov ax, word [es:si+01eh]                 ; 26 8b 44 1e
    test ax, ax                               ; 85 c0
    je short 0932eh                           ; 74 27
    dec ax                                    ; 48
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov word [es:di+0010ch], ax               ; 26 89 85 0c 01
    mov word [es:di+0010eh], di               ; 26 89 bd 0e 01
    mov ax, word [es:di+00264h]               ; 26 8b 85 64 02
    mov dx, word [es:di+00266h]               ; 26 8b 95 66 02
    mov word [es:di+00100h], ax               ; 26 89 85 00 01
    mov word [es:di+00102h], dx               ; 26 89 95 02 01
    inc byte [es:di+00263h]                   ; 26 fe 85 63 02
    mov bx, 000a0h                            ; bb a0 00
    mov ax, si                                ; 89 f0
    mov dx, word [bp-008h]                    ; 8b 56 f8
    call 08aa5h                               ; e8 6c f7
    les bx, [bp-00eh]                         ; c4 5e f2
    mov dx, word [es:bx+004h]                 ; 26 8b 57 04
    mov ax, word [es:bx+006h]                 ; 26 8b 47 06
    mov es, [bp-008h]                         ; 8e 46 f8
    mov word [es:si+01ah], dx                 ; 26 89 54 1a
    mov word [es:si+01ch], ax                 ; 26 89 44 1c
    mov bx, word [es:si+01ah]                 ; 26 8b 5c 1a
    mov cx, ax                                ; 89 c1
    shr cx, 1                                 ; d1 e9
    rcr bx, 1                                 ; d1 db
    mov di, word [es:si+008h]                 ; 26 8b 7c 08
    mov ax, word [es:si+00ah]                 ; 26 8b 44 0a
    mov cx, bx                                ; 89 d9
    mov si, di                                ; 89 fe
    mov dx, ax                                ; 89 c2
    mov es, ax                                ; 8e c0
    push DS                                   ; 1e
    mov ds, dx                                ; 8e da
    rep movsw                                 ; f3 a5
    pop DS                                    ; 1f
    les bx, [bp-00eh]                         ; c4 5e f2
    mov ax, word [es:bx+00268h]               ; 26 8b 87 68 02
    sal eax, 010h                             ; 66 c1 e0 10
    mov es, [bp-00ch]                         ; 8e 46 f4
    mov ax, word [es:bx+006h]                 ; 26 8b 47 06
    or ax, word [es:bx+004h]                  ; 26 0b 47 04
    jne short 0938dh                          ; 75 05
    mov ax, strict word 00004h                ; b8 04 00
    jmp short 0938fh                          ; eb 02
    xor ax, ax                                ; 31 c0
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 0000ch                               ; c2 0c 00
wait_ticks_device_init_:                     ; 0xf9398 LB 0x5
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    pop bp                                    ; 5d
    retn                                      ; c3
ahci_port_detect_device_:                    ; 0xf939d LB 0x4f6
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push cx                                   ; 51
    push si                                   ; 56
    push di                                   ; 57
    sub sp, 0022ch                            ; 81 ec 2c 02
    mov si, ax                                ; 89 c6
    mov word [bp-01ah], dx                    ; 89 56 e6
    mov byte [bp-008h], bl                    ; 88 5e f8
    mov word [bp-01eh], strict word 00000h    ; c7 46 e2 00 00
    mov al, bl                                ; 88 d8
    mov byte [bp-02ah], bl                    ; 88 5e d6
    xor al, bl                                ; 30 d8
    mov byte [bp-029h], al                    ; 88 46 d7
    mov bx, word [bp-02ah]                    ; 8b 5e d6
    mov ax, si                                ; 89 f0
    call 08e8ah                               ; e8 c4 fa
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 9f 82
    mov word [bp-026h], 00122h                ; c7 46 da 22 01
    mov word [bp-024h], ax                    ; 89 46 dc
    mov CL, strict byte 007h                  ; b1 07
    mov ax, word [bp-02ah]                    ; 8b 46 d6
    sal ax, CL                                ; d3 e0
    add ax, 0012ch                            ; 05 2c 01
    cwd                                       ; 99
    mov di, ax                                ; 89 c7
    mov bx, dx                                ; 89 d3
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov dx, word [es:si+00260h]               ; 26 8b 94 60 02
    mov cx, bx                                ; 89 d9
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov dx, word [es:si+00260h]               ; 26 8b 94 60 02
    add dx, strict byte 00004h                ; 83 c2 04
    mov ax, strict word 00001h                ; b8 01 00
    xor cx, cx                                ; 31 c9
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov dx, word [es:si+00260h]               ; 26 8b 94 60 02
    mov ax, di                                ; 89 f8
    mov cx, bx                                ; 89 d9
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov dx, word [es:si+00260h]               ; 26 8b 94 60 02
    add dx, strict byte 00004h                ; 83 c2 04
    xor ax, ax                                ; 31 c0
    xor cx, cx                                ; 31 c9
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov di, strict word 0006ch                ; bf 6c 00
    mov ax, strict word 00040h                ; b8 40 00
    mov word [bp-014h], ax                    ; 89 46 ec
    mov es, ax                                ; 8e c0
    mov ax, word [es:di]                      ; 26 8b 05
    add ax, strict word 00003h                ; 05 03 00
    mov word [bp-01ch], ax                    ; 89 46 e4
    mov ax, word [es:di+002h]                 ; 26 8b 45 02
    adc ax, strict word 00000h                ; 15 00 00
    mov word [bp-028h], ax                    ; 89 46 d8
    mov es, [bp-014h]                         ; 8e 46 ec
    mov ax, word [es:di+002h]                 ; 26 8b 45 02
    cmp ax, word [bp-028h]                    ; 3b 46 d8
    jc short 0947ah                           ; 72 11
    mov ax, word [es:di+002h]                 ; 26 8b 45 02
    cmp ax, word [bp-028h]                    ; 3b 46 d8
    jne short 094c8h                          ; 75 56
    mov ax, word [es:di]                      ; 26 8b 05
    cmp ax, word [bp-01ch]                    ; 3b 46 e4
    jnc short 094c8h                          ; 73 4e
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 007h                  ; b1 07
    sal ax, CL                                ; d3 e0
    add ax, 00128h                            ; 05 28 01
    cwd                                       ; 99
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov bx, word [es:si+00260h]               ; 26 8b 9c 60 02
    mov cx, dx                                ; 89 d1
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov dx, word [es:si+00260h]               ; 26 8b 94 60 02
    add dx, strict byte 00004h                ; 83 c2 04
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    mov word [bp-022h], ax                    ; 89 46 de
    mov word [bp-020h], dx                    ; 89 56 e0
    xor bx, bx                                ; 31 db
    push bx                                   ; 53
    mov bx, strict word 0000fh                ; bb 0f 00
    xor cx, cx                                ; 31 c9
    call 08912h                               ; e8 51 f4
    cmp ax, strict word 00003h                ; 3d 03 00
    jne short 0945dh                          ; 75 97
    jmp short 094d1h                          ; eb 09
    cmp word [bp-01eh], strict byte 00000h    ; 83 7e e2 00
    jne short 094d1h                          ; 75 03
    jmp near 0988bh                           ; e9 ba 03
    xor ax, ax                                ; 31 c0
    push ax                                   ; 50
    mov bx, strict word 0000fh                ; bb 0f 00
    xor cx, cx                                ; 31 c9
    mov ax, word [bp-022h]                    ; 8b 46 de
    mov dx, word [bp-020h]                    ; 8b 56 e0
    call 08912h                               ; e8 30 f4
    cmp ax, strict word 00003h                ; 3d 03 00
    jne short 094ceh                          ; 75 e7
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    mov CL, strict byte 007h                  ; b1 07
    mov di, ax                                ; 89 c7
    sal di, CL                                ; d3 e7
    lea ax, [di+00130h]                       ; 8d 85 30 01
    cwd                                       ; 99
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov bx, word [es:si+00260h]               ; 26 8b 9c 60 02
    mov cx, dx                                ; 89 d1
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov dx, word [es:si+00260h]               ; 26 8b 94 60 02
    add dx, strict byte 00004h                ; 83 c2 04
    mov ax, strict word 0ffffh                ; b8 ff ff
    mov cx, ax                                ; 89 c1
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    les bx, [bp-026h]                         ; c4 5e da
    mov al, byte [es:bx+00231h]               ; 26 8a 87 31 02
    mov byte [bp-00ch], al                    ; 88 46 f4
    cmp AL, strict byte 004h                  ; 3c 04
    jnc short 094ceh                          ; 73 9a
    lea ax, [di+00118h]                       ; 8d 85 18 01
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov bx, word [es:si+00260h]               ; 26 8b 9c 60 02
    xor cx, cx                                ; 31 c9
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    add bx, strict byte 00004h                ; 83 c3 04
    mov dx, bx                                ; 89 da
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    or AL, strict byte 010h                   ; 0c 10
    mov cx, dx                                ; 89 d1
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea ax, [di+00124h]                       ; 8d 85 24 01
    cwd                                       ; 99
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov bx, word [es:si+00260h]               ; 26 8b 9c 60 02
    mov cx, dx                                ; 89 d1
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov dx, word [es:si+00260h]               ; 26 8b 94 60 02
    add dx, strict byte 00004h                ; 83 c2 04
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    mov bl, byte [bp-00ch]                    ; 8a 5e f4
    add bl, 00ch                              ; 80 c3 0c
    mov byte [bp-00eh], bl                    ; 88 5e f2
    test dx, dx                               ; 85 d2
    jne short 095f9h                          ; 75 54
    cmp ax, 00101h                            ; 3d 01 01
    jne short 095f9h                          ; 75 4f
    les bx, [bp-026h]                         ; c4 5e da
    mov word [es:bx+006h], strict word 00000h ; 26 c7 47 06 00 00
    mov word [es:bx+004h], strict word 00000h ; 26 c7 47 04 00 00
    mov word [es:bx+002h], strict word 00000h ; 26 c7 47 02 00 00
    mov word [es:bx], strict word 00000h      ; 26 c7 07 00 00
    lea dx, [bp-00232h]                       ; 8d 96 ce fd
    mov word [es:bx+008h], dx                 ; 26 89 57 08
    mov [es:bx+00ah], ss                      ; 26 8c 57 0a
    mov word [es:bx+00eh], strict word 00001h ; 26 c7 47 0e 01 00
    mov word [es:bx+010h], 00200h             ; 26 c7 47 10 00 02
    mov bx, 000ech                            ; bb ec 00
    mov ax, word [bp-026h]                    ; 8b 46 da
    mov dx, es                                ; 8c c2
    call 08aa5h                               ; e8 be f4
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    mov byte [bp-00ah], al                    ; 88 46 f6
    test byte [bp-00232h], 080h               ; f6 86 ce fd 80
    je short 095fch                           ; 74 08
    mov ax, strict word 00001h                ; b8 01 00
    jmp short 095feh                          ; eb 05
    jmp near 097cah                           ; e9 ce 01
    xor ax, ax                                ; 31 c0
    mov cl, al                                ; 88 c1
    mov ax, word [bp-00230h]                  ; 8b 86 d0 fd
    mov word [bp-018h], ax                    ; 89 46 e8
    mov ax, word [bp-0022ch]                  ; 8b 86 d4 fd
    mov word [bp-016h], ax                    ; 89 46 ea
    mov ax, word [bp-00226h]                  ; 8b 86 da fd
    mov word [bp-02ch], ax                    ; 89 46 d4
    mov di, word [bp-001bah]                  ; 8b be 46 fe
    mov si, word [bp-001b8h]                  ; 8b b6 48 fe
    xor ax, ax                                ; 31 c0
    mov word [bp-012h], ax                    ; 89 46 ee
    mov word [bp-010h], ax                    ; 89 46 f0
    cmp si, 00fffh                            ; 81 fe ff 0f
    jne short 09646h                          ; 75 1b
    cmp di, strict byte 0ffffh                ; 83 ff ff
    jne short 09646h                          ; 75 16
    mov ax, word [bp-00164h]                  ; 8b 86 9c fe
    mov word [bp-010h], ax                    ; 89 46 f0
    mov ax, word [bp-00166h]                  ; 8b 86 9a fe
    mov word [bp-012h], ax                    ; 89 46 ee
    mov si, word [bp-00168h]                  ; 8b b6 98 fe
    mov di, word [bp-0016ah]                  ; 8b be 96 fe
    mov bl, byte [bp-00ch]                    ; 8a 5e f4
    xor bh, bh                                ; 30 ff
    mov es, [bp-024h]                         ; 8e 46 dc
    add bx, word [bp-026h]                    ; 03 5e da
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov byte [es:bx+0022dh], al               ; 26 88 87 2d 02
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov bx, word [bp-026h]                    ; 8b 5e da
    add bx, ax                                ; 01 c3
    mov word [es:bx+022h], 0ff05h             ; 26 c7 47 22 05 ff
    mov byte [es:bx+024h], cl                 ; 26 88 4f 24
    mov byte [es:bx+025h], 000h               ; 26 c6 47 25 00
    mov word [es:bx+028h], 00200h             ; 26 c7 47 28 00 02
    mov byte [es:bx+027h], 001h               ; 26 c6 47 27 01
    mov ax, word [bp-010h]                    ; 8b 46 f0
    mov word [es:bx+03ch], ax                 ; 26 89 47 3c
    mov ax, word [bp-012h]                    ; 8b 46 ee
    mov word [es:bx+03ah], ax                 ; 26 89 47 3a
    mov word [es:bx+038h], si                 ; 26 89 77 38
    mov word [es:bx+036h], di                 ; 26 89 7f 36
    mov ax, word [bp-016h]                    ; 8b 46 ea
    mov word [es:bx+030h], ax                 ; 26 89 47 30
    mov ax, word [bp-018h]                    ; 8b 46 e8
    mov word [es:bx+032h], ax                 ; 26 89 47 32
    mov ax, word [bp-02ch]                    ; 8b 46 d4
    mov word [es:bx+034h], ax                 ; 26 89 47 34
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    cmp AL, strict byte 001h                  ; 3c 01
    jc short 096c0h                           ; 72 0c
    jbe short 096c8h                          ; 76 12
    cmp AL, strict byte 003h                  ; 3c 03
    je short 096d0h                           ; 74 16
    cmp AL, strict byte 002h                  ; 3c 02
    je short 096cch                           ; 74 0e
    jmp short 0971ah                          ; eb 5a
    test al, al                               ; 84 c0
    jne short 0971ah                          ; 75 56
    mov BL, strict byte 040h                  ; b3 40
    jmp short 096d2h                          ; eb 0a
    mov BL, strict byte 048h                  ; b3 48
    jmp short 096d2h                          ; eb 06
    mov BL, strict byte 050h                  ; b3 50
    jmp short 096d2h                          ; eb 02
    mov BL, strict byte 058h                  ; b3 58
    mov dl, bl                                ; 88 da
    add dl, 007h                              ; 80 c2 07
    xor dh, dh                                ; 30 f6
    mov ax, dx                                ; 89 d0
    call 016aeh                               ; e8 d0 7f
    test al, al                               ; 84 c0
    je short 0971ah                           ; 74 38
    mov al, bl                                ; 88 d8
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    xor ah, ah                                ; 30 e4
    call 016aeh                               ; e8 c3 7f
    mov byte [bp-029h], al                    ; 88 46 d7
    mov byte [bp-02ah], dh                    ; 88 76 d6
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    call 016aeh                               ; e8 b6 7f
    xor ah, ah                                ; 30 e4
    mov cx, word [bp-02ah]                    ; 8b 4e d6
    add cx, ax                                ; 01 c1
    mov word [bp-030h], cx                    ; 89 4e d0
    mov al, bl                                ; 88 d8
    add AL, strict byte 002h                  ; 04 02
    call 016aeh                               ; e8 a5 7f
    xor ah, ah                                ; 30 e4
    mov word [bp-032h], ax                    ; 89 46 ce
    mov ax, dx                                ; 89 d0
    call 016aeh                               ; e8 9b 7f
    xor ah, ah                                ; 30 e4
    mov word [bp-02eh], ax                    ; 89 46 d2
    jmp short 0972ah                          ; eb 10
    push word [bp-010h]                       ; ff 76 f0
    push word [bp-012h]                       ; ff 76 ee
    push si                                   ; 56
    push di                                   ; 57
    mov dx, ss                                ; 8c d2
    lea ax, [bp-032h]                         ; 8d 46 ce
    call 05b9fh                               ; e8 75 c4
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 fe 81
    push word [bp-010h]                       ; ff 76 f0
    push word [bp-012h]                       ; ff 76 ee
    push si                                   ; 56
    push di                                   ; 57
    mov ax, word [bp-02eh]                    ; 8b 46 d2
    push ax                                   ; 50
    mov ax, word [bp-032h]                    ; 8b 46 ce
    push ax                                   ; 50
    mov ax, word [bp-030h]                    ; 8b 46 d0
    push ax                                   ; 50
    push word [bp-02ch]                       ; ff 76 d4
    push word [bp-016h]                       ; ff 76 ea
    push word [bp-018h]                       ; ff 76 e8
    mov al, byte [bp-008h]                    ; 8a 46 f8
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    push ax                                   ; 50
    mov ax, 00cd6h                            ; b8 d6 0c
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 0f 82
    add sp, strict byte 0001ch                ; 83 c4 1c
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov di, word [bp-026h]                    ; 8b 7e da
    add di, ax                                ; 01 c7
    mov es, [bp-024h]                         ; 8e 46 dc
    lea di, [di+02ah]                         ; 8d 7d 2a
    push DS                                   ; 1e
    push SS                                   ; 16
    pop DS                                    ; 1f
    lea si, [bp-032h]                         ; 8d 76 ce
    movsw                                     ; a5
    movsw                                     ; a5
    movsw                                     ; a5
    pop DS                                    ; 1f
    mov bx, word [bp-026h]                    ; 8b 5e da
    mov bl, byte [es:bx+001e2h]               ; 26 8a 9f e2 01
    mov dl, byte [bp-00ch]                    ; 8a 56 f4
    add dl, 00ch                              ; 80 c2 0c
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    mov si, word [bp-026h]                    ; 8b 76 da
    add si, ax                                ; 01 c6
    mov byte [es:si+001e3h], dl               ; 26 88 94 e3 01
    db  0feh, 0c3h
    ; inc bl                                    ; fe c3
    mov si, word [bp-026h]                    ; 8b 76 da
    mov byte [es:si+001e2h], bl               ; 26 88 9c e2 01
    mov dx, strict word 00075h                ; ba 75 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01652h                               ; e8 9a 7e
    mov bl, al                                ; 88 c3
    db  0feh, 0c3h
    ; inc bl                                    ; fe c3
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00075h                ; ba 75 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01660h                               ; e8 99 7e
    jmp near 0987dh                           ; e9 b3 00
    cmp dx, 0eb14h                            ; 81 fa 14 eb
    jne short 09824h                          ; 75 54
    cmp ax, 00101h                            ; 3d 01 01
    jne short 09824h                          ; 75 4f
    les bx, [bp-026h]                         ; c4 5e da
    mov word [es:bx+006h], strict word 00000h ; 26 c7 47 06 00 00
    mov word [es:bx+004h], strict word 00000h ; 26 c7 47 04 00 00
    mov word [es:bx+002h], strict word 00000h ; 26 c7 47 02 00 00
    mov word [es:bx], strict word 00000h      ; 26 c7 07 00 00
    lea dx, [bp-00232h]                       ; 8d 96 ce fd
    mov word [es:bx+008h], dx                 ; 26 89 57 08
    mov [es:bx+00ah], ss                      ; 26 8c 57 0a
    mov word [es:bx+00eh], strict word 00001h ; 26 c7 47 0e 01 00
    mov word [es:bx+010h], 00200h             ; 26 c7 47 10 00 02
    mov bx, 000a1h                            ; bb a1 00
    mov ax, word [bp-026h]                    ; 8b 46 da
    mov dx, es                                ; 8c c2
    call 08aa5h                               ; e8 93 f2
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    mov byte [bp-00ah], al                    ; 88 46 f6
    test byte [bp-00232h], 080h               ; f6 86 ce fd 80
    je short 09826h                           ; 74 07
    mov cx, strict word 00001h                ; b9 01 00
    jmp short 09828h                          ; eb 04
    jmp short 0987dh                          ; eb 57
    xor cx, cx                                ; 31 c9
    mov bl, byte [bp-00ch]                    ; 8a 5e f4
    xor bh, bh                                ; 30 ff
    mov es, [bp-024h]                         ; 8e 46 dc
    add bx, word [bp-026h]                    ; 03 5e da
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov byte [es:bx+0022dh], al               ; 26 88 87 2d 02
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    xor ah, ah                                ; 30 e4
    mov dx, strict word 0001ch                ; ba 1c 00
    imul dx                                   ; f7 ea
    mov bx, word [bp-026h]                    ; 8b 5e da
    add bx, ax                                ; 01 c3
    mov word [es:bx+022h], 00505h             ; 26 c7 47 22 05 05
    mov byte [es:bx+024h], cl                 ; 26 88 4f 24
    mov word [es:bx+028h], 00800h             ; 26 c7 47 28 00 08
    mov bx, word [bp-026h]                    ; 8b 5e da
    mov dl, byte [es:bx+001f3h]               ; 26 8a 97 f3 01
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    add AL, strict byte 00ch                  ; 04 0c
    mov bl, dl                                ; 88 d3
    xor bh, bh                                ; 30 ff
    add bx, word [bp-026h]                    ; 03 5e da
    mov byte [es:bx+001f4h], al               ; 26 88 87 f4 01
    db  0feh, 0c2h
    ; inc dl                                    ; fe c2
    mov bx, word [bp-026h]                    ; 8b 5e da
    mov byte [es:bx+001f3h], dl               ; 26 88 97 f3 01
    inc byte [bp-00ch]                        ; fe 46 f4
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    les bx, [bp-026h]                         ; c4 5e da
    mov byte [es:bx+00231h], al               ; 26 88 87 31 02
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bp                                    ; 5d
    retn                                      ; c3
ahci_mem_alloc_:                             ; 0xf9893 LB 0x43
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push si                                   ; 56
    push di                                   ; 57
    mov dx, 00413h                            ; ba 13 04
    xor ax, ax                                ; 31 c0
    call 0166eh                               ; e8 cb 7d
    test ax, ax                               ; 85 c0
    je short 098cch                           ; 74 25
    dec ax                                    ; 48
    mov bx, ax                                ; 89 c3
    xor dx, dx                                ; 31 d2
    mov cx, strict word 0000ah                ; b9 0a 00
    sal ax, 1                                 ; d1 e0
    rcl dx, 1                                 ; d1 d2
    loop 098afh                               ; e2 fa
    mov si, ax                                ; 89 c6
    mov di, dx                                ; 89 d7
    mov cx, strict word 00004h                ; b9 04 00
    shr di, 1                                 ; d1 ef
    rcr si, 1                                 ; d1 de
    loop 098bch                               ; e2 fa
    mov dx, 00413h                            ; ba 13 04
    xor ax, ax                                ; 31 c0
    call 0167ch                               ; e8 b2 7d
    mov ax, si                                ; 89 f0
    lea sp, [bp-00ah]                         ; 8d 66 f6
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
ahci_hba_init_:                              ; 0xf98d6 LB 0x16e
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00006h                ; 83 ec 06
    mov si, ax                                ; 89 c6
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 0166eh                               ; e8 82 7d
    mov bx, 00122h                            ; bb 22 01
    mov di, ax                                ; 89 c7
    mov ax, strict word 00010h                ; b8 10 00
    xor cx, cx                                ; 31 c9
    mov dx, si                                ; 89 f2
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea dx, [si+004h]                         ; 8d 54 04
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    call 09893h                               ; e8 83 ff
    mov word [bp-010h], ax                    ; 89 46 f0
    test ax, ax                               ; 85 c0
    jne short 0991ah                          ; 75 03
    jmp near 09a23h                           ; e9 09 01
    mov ax, word [bp-010h]                    ; 8b 46 f0
    mov es, di                                ; 8e c7
    mov word [es:bx+00232h], ax               ; 26 89 87 32 02
    mov byte [es:bx+00231h], 000h             ; 26 c6 87 31 02 00
    xor bx, bx                                ; 31 db
    mov es, ax                                ; 8e c0
    mov byte [es:bx+00262h], 0ffh             ; 26 c6 87 62 02 ff
    mov word [es:bx+00260h], si               ; 26 89 b7 60 02
    mov word [es:bx+00264h], 0c000h           ; 26 c7 87 64 02 00 c0
    mov word [es:bx+00266h], strict word 0000ch ; 26 c7 87 66 02 0c 00
    mov ax, strict word 00004h                ; b8 04 00
    xor cx, cx                                ; 31 c9
    mov dx, si                                ; 89 f2
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea bx, [si+004h]                         ; 8d 5c 04
    mov dx, bx                                ; 89 da
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    or AL, strict byte 001h                   ; 0c 01
    mov cx, dx                                ; 89 d1
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov ax, strict word 00004h                ; b8 04 00
    xor cx, cx                                ; 31 c9
    mov dx, si                                ; 89 f2
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea bx, [si+004h]                         ; 8d 5c 04
    mov dx, bx                                ; 89 da
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    test AL, strict byte 001h                 ; a8 01
    jne short 09974h                          ; 75 de
    xor ax, ax                                ; 31 c0
    xor cx, cx                                ; 31 c9
    mov dx, si                                ; 89 f2
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov dx, bx                                ; 89 da
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    xor bx, bx                                ; 31 db
    push bx                                   ; 53
    mov bx, strict word 0001fh                ; bb 1f 00
    xor cx, cx                                ; 31 c9
    call 08912h                               ; e8 57 ef
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    mov byte [bp-00eh], al                    ; 88 46 f2
    mov byte [bp-00ch], 000h                  ; c6 46 f4 00
    jmp short 099e7h                          ; eb 21
    xor al, al                                ; 30 c0
    test al, al                               ; 84 c0
    je short 099deh                           ; 74 12
    mov bl, byte [bp-00ch]                    ; 8a 5e f4
    xor bh, bh                                ; 30 ff
    xor ax, ax                                ; 31 c0
    mov dx, word [bp-010h]                    ; 8b 56 f0
    call 0939dh                               ; e8 c4 f9
    dec byte [bp-00eh]                        ; fe 4e f2
    je short 09a21h                           ; 74 43
    inc byte [bp-00ch]                        ; fe 46 f4
    cmp byte [bp-00ch], 020h                  ; 80 7e f4 20
    jnc short 09a21h                          ; 73 3a
    mov cl, byte [bp-00ch]                    ; 8a 4e f4
    xor ch, ch                                ; 30 ed
    mov bx, strict word 00001h                ; bb 01 00
    xor di, di                                ; 31 ff
    jcxz 099f9h                               ; e3 06
    sal bx, 1                                 ; d1 e3
    rcl di, 1                                 ; d1 d7
    loop 099f3h                               ; e2 fa
    mov ax, strict word 0000ch                ; b8 0c 00
    xor cx, cx                                ; 31 c9
    mov dx, si                                ; 89 f2
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    lea dx, [si+004h]                         ; 8d 54 04
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    test dx, di                               ; 85 fa
    jne short 09a1dh                          ; 75 04
    test ax, bx                               ; 85 d8
    je short 099c6h                           ; 74 a9
    mov AL, strict byte 001h                  ; b0 01
    jmp short 099c8h                          ; eb a7
    xor ax, ax                                ; 31 c0
    lea sp, [bp-00ah]                         ; 8d 66 f6
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
    db  00bh, 005h, 004h, 003h, 002h, 001h, 000h, 02ah, 09bh, 008h, 09bh, 00eh, 09bh, 014h, 09bh, 01ah
    db  09bh, 020h, 09bh, 026h, 09bh, 02ah, 09bh
_ahci_init:                                  ; 0xf9a44 LB 0x139
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00012h                ; 83 ec 12
    mov ax, 00601h                            ; b8 01 06
    mov dx, strict word 00001h                ; ba 01 00
    call 09f9ch                               ; e8 47 05
    mov dx, ax                                ; 89 c2
    cmp ax, strict word 0ffffh                ; 3d ff ff
    je short 09aa5h                           ; 74 49
    mov al, ah                                ; 88 e0
    mov byte [bp-008h], ah                    ; 88 66 f8
    mov byte [bp-00ah], dl                    ; 88 56 f6
    xor dh, ah                                ; 30 e6
    xor ah, ah                                ; 30 e4
    mov bx, strict word 00034h                ; bb 34 00
    call 09fc7h                               ; e8 59 05
    mov cl, al                                ; 88 c1
    test cl, cl                               ; 84 c9
    je short 09aa8h                           ; 74 34
    mov bl, cl                                ; 88 cb
    xor bh, bh                                ; 30 ff
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    mov byte [bp-00ch], al                    ; 88 46 f4
    mov byte [bp-00bh], bh                    ; 88 7e f5
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov byte [bp-012h], al                    ; 88 46 ee
    mov byte [bp-011h], bh                    ; 88 7e ef
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    mov ax, word [bp-012h]                    ; 8b 46 ee
    call 09fc7h                               ; e8 34 05
    cmp AL, strict byte 012h                  ; 3c 12
    je short 09aa8h                           ; 74 11
    mov bl, cl                                ; 88 cb
    db  0feh, 0c3h
    ; inc bl                                    ; fe c3
    xor bh, bh                                ; 30 ff
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    mov ax, word [bp-012h]                    ; 8b 46 ee
    jmp short 09a6bh                          ; eb c6
    jmp near 09b76h                           ; e9 ce 00
    test cl, cl                               ; 84 c9
    je short 09aa5h                           ; 74 f9
    add cl, 002h                              ; 80 c1 02
    mov bl, cl                                ; 88 cb
    xor bh, bh                                ; 30 ff
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    mov byte [bp-00eh], al                    ; 88 46 f2
    mov byte [bp-00dh], bh                    ; 88 7e f3
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov byte [bp-014h], al                    ; 88 46 ec
    mov byte [bp-013h], bh                    ; 88 7e ed
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    mov ax, word [bp-014h]                    ; 8b 46 ec
    call 09fc7h                               ; e8 f9 04
    cmp AL, strict byte 010h                  ; 3c 10
    jne short 09aa5h                          ; 75 d3
    mov byte [bp-006h], 000h                  ; c6 46 fa 00
    mov bl, cl                                ; 88 cb
    add bl, 002h                              ; 80 c3 02
    xor bh, bh                                ; 30 ff
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    mov ax, word [bp-014h]                    ; 8b 46 ec
    call 09feeh                               ; e8 08 05
    mov dx, ax                                ; 89 c2
    and ax, strict word 0000fh                ; 25 0f 00
    sub ax, strict word 00004h                ; 2d 04 00
    cmp ax, strict word 0000bh                ; 3d 0b 00
    jnbe short 09b2ah                         ; 77 37
    push CS                                   ; 0e
    pop ES                                    ; 07
    mov cx, strict word 00008h                ; b9 08 00
    mov di, 09a2dh                            ; bf 2d 9a
    repne scasb                               ; f2 ae
    sal cx, 1                                 ; d1 e1
    mov di, cx                                ; 89 cf
    mov ax, word [cs:di-065cch]               ; 2e 8b 85 34 9a
    jmp ax                                    ; ff e0
    mov byte [bp-006h], 010h                  ; c6 46 fa 10
    jmp short 09b2ah                          ; eb 1c
    mov byte [bp-006h], 014h                  ; c6 46 fa 14
    jmp short 09b2ah                          ; eb 16
    mov byte [bp-006h], 018h                  ; c6 46 fa 18
    jmp short 09b2ah                          ; eb 10
    mov byte [bp-006h], 01ch                  ; c6 46 fa 1c
    jmp short 09b2ah                          ; eb 0a
    mov byte [bp-006h], 020h                  ; c6 46 fa 20
    jmp short 09b2ah                          ; eb 04
    mov byte [bp-006h], 024h                  ; c6 46 fa 24
    mov CL, strict byte 004h                  ; b1 04
    mov si, dx                                ; 89 d6
    shr si, CL                                ; d3 ee
    sal si, 1                                 ; d1 e6
    sal si, 1                                 ; d1 e6
    mov al, byte [bp-006h]                    ; 8a 46 fa
    test al, al                               ; 84 c0
    je short 09b76h                           ; 74 3b
    mov bl, al                                ; 88 c3
    xor bh, bh                                ; 30 ff
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    mov byte [bp-010h], al                    ; 88 46 f0
    mov byte [bp-00fh], bh                    ; 88 7e f1
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov byte [bp-016h], al                    ; 88 46 ea
    mov byte [bp-015h], bh                    ; 88 7e eb
    mov dx, word [bp-010h]                    ; 8b 56 f0
    mov ax, word [bp-016h]                    ; 8b 46 ea
    call 0a013h                               ; e8 b9 04
    test AL, strict byte 001h                 ; a8 01
    je short 09b76h                           ; 74 18
    and AL, strict byte 0f0h                  ; 24 f0
    add si, ax                                ; 01 c6
    mov cx, strict word 00007h                ; b9 07 00
    mov bx, strict word 00004h                ; bb 04 00
    mov dx, word [bp-010h]                    ; 8b 56 f0
    mov ax, word [bp-016h]                    ; 8b 46 ea
    call 0a052h                               ; e8 e1 04
    mov ax, si                                ; 89 f0
    call 098d6h                               ; e8 60 fd
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
apm_out_str_:                                ; 0xf9b7d LB 0x39
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    mov bx, ax                                ; 89 c3
    cmp byte [bx], 000h                       ; 80 3f 00
    je short 09b92h                           ; 74 0a
    mov al, byte [bx]                         ; 8a 07
    out DX, AL                                ; ee
    inc bx                                    ; 43
    mov al, byte [bx]                         ; 8a 07
    db  00ah, 0c0h
    ; or al, al                                 ; 0a c0
    jne short 09b8ah                          ; 75 f8
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
    xlatb                                     ; d7
    wait                                      ; 9b
    bound bx, [si-06417h]                     ; 62 9c e9 9b
    add AL, strict byte 09ch                  ; 04 9c
    bound bx, [si-063d1h]                     ; 62 9c 2f 9c
    bound bx, [si-06398h]                     ; 62 9c 68 9c
    xor AL, strict byte 09ch                  ; 34 9c
    xor AL, strict byte 09ch                  ; 34 9c
    xor AL, strict byte 09ch                  ; 34 9c
    cmpsw                                     ; a7
    pushfw                                    ; 9c
    xor AL, strict byte 09ch                  ; 34 9c
    xor AL, strict byte 09ch                  ; 34 9c
    db  0a0h
    pushfw                                    ; 9c
_apm_function:                               ; 0xf9bb6 LB 0xf6
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    and byte [bp+018h], 0feh                  ; 80 66 18 fe
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    cmp ax, strict word 0000eh                ; 3d 0e 00
    jnbe short 09c34h                         ; 77 6c
    mov bx, ax                                ; 89 c3
    sal bx, 1                                 ; d1 e3
    mov dx, word [bp+018h]                    ; 8b 56 18
    or dl, 001h                               ; 80 ca 01
    jmp word [cs:bx-06468h]                   ; 2e ff a7 98 9b
    mov word [bp+012h], 00102h                ; c7 46 12 02 01
    mov word [bp+00ch], 0504dh                ; c7 46 0c 4d 50
    mov word [bp+010h], strict word 00003h    ; c7 46 10 03 00
    jmp near 09c62h                           ; e9 79 00
    mov word [bp+012h], 0f000h                ; c7 46 12 00 f0
    mov word [bp+00ch], 0a244h                ; c7 46 0c 44 a2
    mov word [bp+010h], 0f000h                ; c7 46 10 00 f0
    mov ax, strict word 0fff0h                ; b8 f0 ff
    mov word [bp+006h], ax                    ; 89 46 06
    mov word [bp+004h], ax                    ; 89 46 04
    jmp near 09c62h                           ; e9 5e 00
    mov word [bp+012h], 0f000h                ; c7 46 12 00 f0
    mov word [bp+00ch], 0da40h                ; c7 46 0c 40 da
    mov ax, 0f000h                            ; b8 00 f0
    mov word [bp+010h], ax                    ; 89 46 10
    mov word [bp+00eh], ax                    ; 89 46 0e
    mov ax, strict word 0fff0h                ; b8 f0 ff
    mov word [bp+006h], ax                    ; 89 46 06
    mov word [bp+004h], ax                    ; 89 46 04
    xor bx, bx                                ; 31 db
    sal ebx, 010h                             ; 66 c1 e3 10
    mov si, ax                                ; 89 c6
    sal esi, 010h                             ; 66 c1 e6 10
    jmp near 09c62h                           ; e9 33 00
    sti                                       ; fb
    hlt                                       ; f4
    jmp near 09c62h                           ; e9 2e 00
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 f4 7c
    push word [bp+00ch]                       ; ff 76 0c
    push word [bp+012h]                       ; ff 76 12
    mov ax, 00d29h                            ; b8 29 0d
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 26 7d
    add sp, strict byte 00008h                ; 83 c4 08
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    or ah, 00ch                               ; 80 cc 0c
    mov word [bp+012h], ax                    ; 89 46 12
    or byte [bp+018h], 001h                   ; 80 4e 18 01
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
    cmp word [bp+010h], strict byte 00003h    ; 83 7e 10 03
    je short 09c8dh                           ; 74 1f
    cmp word [bp+010h], strict byte 00002h    ; 83 7e 10 02
    je short 09c85h                           ; 74 11
    cmp word [bp+010h], strict byte 00001h    ; 83 7e 10 01
    jne short 09c95h                          ; 75 1b
    mov dx, 0040fh                            ; ba 0f 04
    mov ax, 00d10h                            ; b8 10 0d
    call 09b7dh                               ; e8 fa fe
    jmp short 09c62h                          ; eb dd
    mov dx, 0040fh                            ; ba 0f 04
    mov ax, 00d18h                            ; b8 18 0d
    jmp short 09c80h                          ; eb f3
    mov dx, 0040fh                            ; ba 0f 04
    mov ax, 00d20h                            ; b8 20 0d
    jmp short 09c80h                          ; eb eb
    or ah, 00ah                               ; 80 cc 0a
    mov word [bp+012h], ax                    ; 89 46 12
    mov word [bp+018h], dx                    ; 89 56 18
    jmp short 09c62h                          ; eb c2
    mov word [bp+012h], 00102h                ; c7 46 12 02 01
    jmp short 09c62h                          ; eb bb
    or ah, 080h                               ; 80 cc 80
    jmp short 09c98h                          ; eb ec
pci16_select_reg_:                           ; 0xf9cac LB 0x24
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    and dl, 0fch                              ; 80 e2 fc
    mov bx, dx                                ; 89 d3
    mov dx, 00cf8h                            ; ba f8 0c
    movzx eax, ax                             ; 66 0f b7 c0
    sal eax, 008h                             ; 66 c1 e0 08
    or eax, strict dword 080000000h           ; 66 0d 00 00 00 80
    db  08ah, 0c3h
    ; mov al, bl                                ; 8a c3
    out DX, eax                               ; 66 ef
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
pci16_find_device_:                          ; 0xf9cd0 LB 0xf7
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 0000ch                ; 83 ec 0c
    push ax                                   ; 50
    push dx                                   ; 52
    mov si, bx                                ; 89 de
    mov di, cx                                ; 89 cf
    test cx, cx                               ; 85 c9
    xor bx, bx                                ; 31 db
    mov byte [bp-006h], 000h                  ; c6 46 fa 00
    test bl, 007h                             ; f6 c3 07
    jne short 09d18h                          ; 75 2d
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, bx                                ; 89 d8
    call 09cach                               ; e8 b9 ff
    mov dx, 00cfeh                            ; ba fe 0c
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov byte [bp-008h], al                    ; 88 46 f8
    cmp AL, strict byte 0ffh                  ; 3c ff
    jne short 09d06h                          ; 75 06
    add bx, strict byte 00008h                ; 83 c3 08
    jmp near 09d98h                           ; e9 92 00
    test byte [bp-008h], 080h                 ; f6 46 f8 80
    je short 09d13h                           ; 74 07
    mov word [bp-00eh], strict word 00001h    ; c7 46 f2 01 00
    jmp short 09d18h                          ; eb 05
    mov word [bp-00eh], strict word 00008h    ; c7 46 f2 08 00
    mov al, byte [bp-008h]                    ; 8a 46 f8
    and AL, strict byte 007h                  ; 24 07
    cmp AL, strict byte 001h                  ; 3c 01
    jne short 09d3fh                          ; 75 1e
    mov al, bh                                ; 88 f8
    xor ah, ah                                ; 30 e4
    test ax, ax                               ; 85 c0
    jne short 09d3fh                          ; 75 16
    mov dx, strict word 0001ah                ; ba 1a 00
    mov ax, bx                                ; 89 d8
    call 09cach                               ; e8 7b ff
    mov dx, 00cfeh                            ; ba fe 0c
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp al, byte [bp-006h]                    ; 3a 46 fa
    jbe short 09d3fh                          ; 76 03
    mov byte [bp-006h], al                    ; 88 46 fa
    test di, di                               ; 85 ff
    je short 09d48h                           ; 74 05
    mov dx, strict word 00008h                ; ba 08 00
    jmp short 09d4ah                          ; eb 02
    xor dx, dx                                ; 31 d2
    mov ax, bx                                ; 89 d8
    call 09cach                               ; e8 5d ff
    mov dx, 00cfch                            ; ba fc 0c
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    mov word [bp-00ah], ax                    ; 89 46 f6
    mov word [bp-010h], dx                    ; 89 56 f0
    mov word [bp-00ch], strict word 00000h    ; c7 46 f4 00 00
    test di, di                               ; 85 ff
    je short 09d79h                           ; 74 0f
    mov cx, strict word 00008h                ; b9 08 00
    shr dx, 1                                 ; d1 ea
    rcr ax, 1                                 ; d1 d8
    loop 09d6dh                               ; e2 fa
    mov word [bp-00ah], ax                    ; 89 46 f6
    mov word [bp-010h], dx                    ; 89 56 f0
    mov ax, word [bp-010h]                    ; 8b 46 f0
    cmp ax, word [bp-014h]                    ; 3b 46 ec
    jne short 09d89h                          ; 75 08
    mov ax, word [bp-00ah]                    ; 8b 46 f6
    cmp ax, word [bp-012h]                    ; 3b 46 ee
    je short 09d8fh                           ; 74 06
    cmp word [bp-00ch], strict byte 00000h    ; 83 7e f4 00
    je short 09d95h                           ; 74 06
    dec si                                    ; 4e
    cmp si, strict byte 0ffffh                ; 83 fe ff
    je short 09da8h                           ; 74 13
    add bx, word [bp-00eh]                    ; 03 5e f2
    mov al, bh                                ; 88 f8
    xor ah, ah                                ; 30 e4
    mov dl, byte [bp-006h]                    ; 8a 56 fa
    xor dh, dh                                ; 30 f6
    cmp ax, dx                                ; 39 d0
    jnbe short 09da8h                         ; 77 03
    jmp near 09ce6h                           ; e9 3e ff
    cmp si, strict byte 0ffffh                ; 83 fe ff
    jne short 09db1h                          ; 75 04
    mov ax, bx                                ; 89 d8
    jmp short 09db4h                          ; eb 03
    mov ax, strict word 0ffffh                ; b8 ff ff
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
    test AL, strict byte 09eh                 ; a8 9e
    rcr word [bp-0612dh], 0e7h                ; c1 9e d3 9e e7
    sahf                                      ; 9e
    stc                                       ; f9
    sahf                                      ; 9e
    or AL, strict byte 09fh                   ; 0c 9f
_pci16_function:                             ; 0xf9dc7 LB 0x1d5
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    push ax                                   ; 50
    and word [bp+020h], 000ffh                ; 81 66 20 ff 00
    and word [bp+02ch], strict byte 0fffeh    ; 83 66 2c fe
    mov bx, word [bp+020h]                    ; 8b 5e 20
    xor bh, bh                                ; 30 ff
    mov ax, word [bp+020h]                    ; 8b 46 20
    xor ah, ah                                ; 30 e4
    cmp bx, strict byte 00003h                ; 83 fb 03
    jc short 09df9h                           ; 72 13
    jbe short 09e4ch                          ; 76 64
    cmp bx, strict byte 0000eh                ; 83 fb 0e
    je short 09e54h                           ; 74 67
    cmp bx, strict byte 00008h                ; 83 fb 08
    jc short 09e03h                           ; 72 11
    cmp bx, strict byte 0000dh                ; 83 fb 0d
    jbe short 09e57h                          ; 76 60
    jmp short 09e03h                          ; eb 0a
    cmp bx, strict byte 00002h                ; 83 fb 02
    je short 09e22h                           ; 74 24
    cmp bx, strict byte 00001h                ; 83 fb 01
    je short 09e06h                           ; 74 03
    jmp near 09f65h                           ; e9 5f 01
    mov word [bp+020h], strict word 00001h    ; c7 46 20 01 00
    mov word [bp+014h], 00210h                ; c7 46 14 10 02
    mov word [bp+01ch], strict word 00000h    ; c7 46 1c 00 00
    mov word [bp+018h], 04350h                ; c7 46 18 50 43
    mov word [bp+01ah], 02049h                ; c7 46 1a 49 20
    jmp near 09f95h                           ; e9 73 01
    cmp word [bp+018h], strict byte 0ffffh    ; 83 7e 18 ff
    jne short 09e2eh                          ; 75 06
    or ah, 083h                               ; 80 cc 83
    jmp near 09f8eh                           ; e9 60 01
    mov bx, word [bp+008h]                    ; 8b 5e 08
    mov dx, word [bp+01ch]                    ; 8b 56 1c
    mov ax, word [bp+018h]                    ; 8b 46 18
    xor cx, cx                                ; 31 c9
    call 09cd0h                               ; e8 94 fe
    cmp ax, strict word 0ffffh                ; 3d ff ff
    jne short 09e4eh                          ; 75 0d
    mov ax, word [bp+020h]                    ; 8b 46 20
    xor ah, ah                                ; 30 e4
    or ah, 086h                               ; 80 cc 86
    jmp near 09f8eh                           ; e9 42 01
    jmp short 09e59h                          ; eb 0b
    mov word [bp+014h], ax                    ; 89 46 14
    jmp near 09f95h                           ; e9 41 01
    jmp near 09f20h                           ; e9 c9 00
    jmp short 09e7eh                          ; eb 25
    mov bx, word [bp+008h]                    ; 8b 5e 08
    mov ax, word [bp+01ch]                    ; 8b 46 1c
    mov dx, word [bp+01eh]                    ; 8b 56 1e
    mov cx, strict word 00001h                ; b9 01 00
    call 09cd0h                               ; e8 68 fe
    cmp ax, strict word 0ffffh                ; 3d ff ff
    jne short 09e78h                          ; 75 0b
    mov ax, word [bp+020h]                    ; 8b 46 20
    xor ah, ah                                ; 30 e4
    or ah, 086h                               ; 80 cc 86
    jmp near 09f8eh                           ; e9 16 01
    mov word [bp+014h], ax                    ; 89 46 14
    jmp near 09f95h                           ; e9 17 01
    cmp word [bp+004h], 00100h                ; 81 7e 04 00 01
    jc short 09e8bh                           ; 72 06
    or ah, 087h                               ; 80 cc 87
    jmp near 09f8eh                           ; e9 03 01
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+014h]                    ; 8b 46 14
    call 09cach                               ; e8 18 fe
    mov bx, word [bp+020h]                    ; 8b 5e 20
    xor bh, bh                                ; 30 ff
    sub bx, strict byte 00008h                ; 83 eb 08
    cmp bx, strict byte 00005h                ; 83 fb 05
    jnbe short 09f09h                         ; 77 68
    sal bx, 1                                 ; d1 e3
    jmp word [cs:bx-06245h]                   ; 2e ff a7 bb 9d
    mov bx, word [bp+01ch]                    ; 8b 5e 1c
    xor bl, bl                                ; 30 db
    mov dx, word [bp+004h]                    ; 8b 56 04
    and dx, strict byte 00003h                ; 83 e2 03
    add dx, 00cfch                            ; 81 c2 fc 0c
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    or bx, ax                                 ; 09 c3
    mov word [bp+01ch], bx                    ; 89 5e 1c
    jmp short 09f09h                          ; eb 48
    mov dx, word [bp+004h]                    ; 8b 56 04
    xor dh, dh                                ; 30 f6
    and dl, 002h                              ; 80 e2 02
    add dx, 00cfch                            ; 81 c2 fc 0c
    in ax, DX                                 ; ed
    mov word [bp+01ch], ax                    ; 89 46 1c
    jmp short 09f09h                          ; eb 36
    mov dx, 00cfch                            ; ba fc 0c
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    mov word [bp+01ch], ax                    ; 89 46 1c
    mov word [bp+01eh], dx                    ; 89 56 1e
    jmp short 09f09h                          ; eb 22
    mov ax, word [bp+01ch]                    ; 8b 46 1c
    mov dx, word [bp+004h]                    ; 8b 56 04
    xor dh, dh                                ; 30 f6
    and dl, 003h                              ; 80 e2 03
    add dx, 00cfch                            ; 81 c2 fc 0c
    out DX, AL                                ; ee
    jmp short 09f09h                          ; eb 10
    mov ax, word [bp+01ch]                    ; 8b 46 1c
    mov dx, word [bp+004h]                    ; 8b 56 04
    xor dh, dh                                ; 30 f6
    and dl, 002h                              ; 80 e2 02
    add dx, 00cfch                            ; 81 c2 fc 0c
    out DX, ax                                ; ef
    jmp near 09f95h                           ; e9 89 00
    mov ax, word [bp+01ch]                    ; 8b 46 1c
    mov cx, word [bp+01eh]                    ; 8b 4e 1e
    mov dx, 00cfch                            ; ba fc 0c
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    jmp short 09f95h                          ; eb 75
    mov bx, word [bp+004h]                    ; 8b 5e 04
    mov es, [bp+026h]                         ; 8e 46 26
    mov word [bp-008h], bx                    ; 89 5e f8
    mov [bp-006h], es                         ; 8c 46 fa
    mov cx, word [0f380h]                     ; 8b 0e 80 f3
    cmp cx, word [es:bx]                      ; 26 3b 0f
    jbe short 09f46h                          ; 76 11
    mov ax, word [bp+020h]                    ; 8b 46 20
    xor ah, ah                                ; 30 e4
    or ah, 089h                               ; 80 cc 89
    mov word [bp+020h], ax                    ; 89 46 20
    or word [bp+02ch], strict byte 00001h     ; 83 4e 2c 01
    jmp short 09f5ah                          ; eb 14
    les di, [es:bx+002h]                      ; 26 c4 7f 02
    mov si, 0f1a0h                            ; be a0 f1
    mov dx, ds                                ; 8c da
    push DS                                   ; 1e
    mov ds, dx                                ; 8e da
    rep movsb                                 ; f3 a4
    pop DS                                    ; 1f
    mov word [bp+014h], 00a00h                ; c7 46 14 00 0a
    mov ax, word [0f380h]                     ; a1 80 f3
    les bx, [bp-008h]                         ; c4 5e f8
    mov word [es:bx], ax                      ; 26 89 07
    jmp short 09f95h                          ; eb 30
    mov bx, 00da0h                            ; bb a0 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01933h                               ; e8 c3 79
    mov ax, word [bp+014h]                    ; 8b 46 14
    push ax                                   ; 50
    mov ax, word [bp+020h]                    ; 8b 46 20
    push ax                                   ; 50
    mov ax, 00d5ch                            ; b8 5c 0d
    push ax                                   ; 50
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 01976h                               ; e8 f3 79
    add sp, strict byte 00008h                ; 83 c4 08
    mov ax, word [bp+020h]                    ; 8b 46 20
    xor ah, ah                                ; 30 e4
    or ah, 081h                               ; 80 cc 81
    mov word [bp+020h], ax                    ; 89 46 20
    or word [bp+02ch], strict byte 00001h     ; 83 4e 2c 01
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
pci_find_classcode_:                         ; 0xf9f9c LB 0x8
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov ax, strict word 0ffffh                ; b8 ff ff
    pop bp                                    ; 5d
    retn                                      ; c3
pci_find_device_:                            ; 0xf9fa4 LB 0x23
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push si                                   ; 56
    mov cx, dx                                ; 89 d1
    xor si, si                                ; 31 f6
    mov dx, ax                                ; 89 c2
    mov ax, 0b102h                            ; b8 02 b1
    int 01ah                                  ; cd 1a
    cmp ah, 000h                              ; 80 fc 00
    je short 09fbdh                           ; 74 03
    mov bx, strict word 0ffffh                ; bb ff ff
    mov ax, bx                                ; 89 d8
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
pci_read_config_byte_:                       ; 0xf9fc7 LB 0x27
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push cx                                   ; 51
    push di                                   ; 57
    mov dh, al                                ; 88 c6
    mov bh, dl                                ; 88 d7
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    xor dl, dl                                ; 30 d2
    mov bl, bh                                ; 88 fb
    mov bh, dh                                ; 88 f7
    mov di, ax                                ; 89 c7
    mov ax, 0b108h                            ; b8 08 b1
    int 01ah                                  ; cd 1a
    mov al, cl                                ; 88 c8
    xor ah, ah                                ; 30 e4
    xor dh, dh                                ; 30 f6
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop cx                                    ; 59
    pop bp                                    ; 5d
    retn                                      ; c3
pci_read_config_word_:                       ; 0xf9fee LB 0x25
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push cx                                   ; 51
    push di                                   ; 57
    mov dh, al                                ; 88 c6
    mov cl, dl                                ; 88 d1
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    mov bh, dh                                ; 88 f7
    xor dh, dh                                ; 30 f6
    mov bl, dl                                ; 88 d3
    mov di, ax                                ; 89 c7
    mov ax, 0b109h                            ; b8 09 b1
    int 01ah                                  ; cd 1a
    mov ax, cx                                ; 89 c8
    xor dl, dl                                ; 30 d2
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop cx                                    ; 59
    pop bp                                    ; 5d
    retn                                      ; c3
pci_read_config_dword_:                      ; 0xfa013 LB 0x3f
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push cx                                   ; 51
    push di                                   ; 57
    push ax                                   ; 50
    mov dh, al                                ; 88 c6
    mov byte [bp-006h], bl                    ; 88 5e fa
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    mov di, ax                                ; 89 c7
    mov bx, dx                                ; 89 d3
    mov ax, 0b109h                            ; b8 09 b1
    int 01ah                                  ; cd 1a
    test cx, cx                               ; 85 c9
    jne short 0a043h                          ; 75 14
    mov al, byte [bp-006h]                    ; 8a 46 fa
    xor ah, ah                                ; 30 e4
    mov di, ax                                ; 89 c7
    inc di                                    ; 47
    inc di                                    ; 47
    mov bx, dx                                ; 89 d3
    mov ax, 0b109h                            ; b8 09 b1
    int 01ah                                  ; cd 1a
    test cx, cx                               ; 85 c9
    je short 0a048h                           ; 74 05
    mov ax, strict word 00001h                ; b8 01 00
    jmp short 0a04ah                          ; eb 02
    xor ax, ax                                ; 31 c0
    cwd                                       ; 99
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop cx                                    ; 59
    pop bp                                    ; 5d
    retn                                      ; c3
pci_write_config_word_:                      ; 0xfa052 LB 0x1e
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push di                                   ; 57
    push ax                                   ; 50
    mov byte [bp-004h], al                    ; 88 46 fc
    mov al, bl                                ; 88 d8
    xor ah, ah                                ; 30 e4
    mov di, ax                                ; 89 c7
    mov bh, byte [bp-004h]                    ; 8a 7e fc
    mov bl, dl                                ; 88 d3
    mov ax, 0b10ch                            ; b8 0c b1
    int 01ah                                  ; cd 1a
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop di                                    ; 5f
    pop bp                                    ; 5d
    retn                                      ; c3
vds_is_present_:                             ; 0xfa070 LB 0x1d
    push bx                                   ; 53
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov bx, strict word 0007bh                ; bb 7b 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    test byte [es:bx], 020h                   ; 26 f6 07 20
    je short 0a088h                           ; 74 06
    mov ax, strict word 00001h                ; b8 01 00
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    retn                                      ; c3
    xor ax, ax                                ; 31 c0
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    retn                                      ; c3
vds_real_to_lin_:                            ; 0xfa08d LB 0x1e
    push bx                                   ; 53
    push cx                                   ; 51
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov bx, ax                                ; 89 c3
    mov ax, dx                                ; 89 d0
    xor dx, dx                                ; 31 d2
    mov cx, strict word 00004h                ; b9 04 00
    sal ax, 1                                 ; d1 e0
    rcl dx, 1                                 ; d1 d2
    loop 0a09bh                               ; e2 fa
    xor cx, cx                                ; 31 c9
    add ax, bx                                ; 01 d8
    adc dx, cx                                ; 11 ca
    pop bp                                    ; 5d
    pop cx                                    ; 59
    pop bx                                    ; 5b
    retn                                      ; c3
vds_build_sg_list_:                          ; 0xfa0ab LB 0x77
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    mov di, ax                                ; 89 c7
    mov si, dx                                ; 89 d6
    mov ax, bx                                ; 89 d8
    mov dx, cx                                ; 89 ca
    mov bx, word [bp+004h]                    ; 8b 5e 04
    mov es, si                                ; 8e c6
    mov word [es:di], bx                      ; 26 89 1d
    mov bx, word [bp+006h]                    ; 8b 5e 06
    mov word [es:di+002h], bx                 ; 26 89 5d 02
    call 0a08dh                               ; e8 c3 ff
    mov es, si                                ; 8e c6
    mov word [es:di+004h], ax                 ; 26 89 45 04
    mov word [es:di+006h], dx                 ; 26 89 55 06
    mov word [es:di+008h], strict word 00000h ; 26 c7 45 08 00 00
    call 0a070h                               ; e8 93 ff
    test ax, ax                               ; 85 c0
    je short 0a0f2h                           ; 74 11
    mov es, si                                ; 8e c6
    mov ax, 08105h                            ; b8 05 81
    mov dx, strict word 00000h                ; ba 00 00
    int 04bh                                  ; cd 4b
    jc short 0a0efh                           ; 72 02
    db  032h, 0c0h
    ; xor al, al                                ; 32 c0
    cbw                                       ; 98
    jmp short 0a119h                          ; eb 27
    mov es, si                                ; 8e c6
    mov word [es:di+00eh], strict word 00001h ; 26 c7 45 0e 01 00
    mov dx, word [es:di+004h]                 ; 26 8b 55 04
    mov ax, word [es:di+006h]                 ; 26 8b 45 06
    mov word [es:di+010h], dx                 ; 26 89 55 10
    mov word [es:di+012h], ax                 ; 26 89 45 12
    mov ax, word [bp+004h]                    ; 8b 46 04
    mov word [es:di+014h], ax                 ; 26 89 45 14
    mov ax, bx                                ; 89 d8
    mov word [es:di+016h], bx                 ; 26 89 5d 16
    xor ax, bx                                ; 31 d8
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 00004h                               ; c2 04 00
vds_free_sg_list_:                           ; 0xfa122 LB 0x2e
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push di                                   ; 57
    mov bx, ax                                ; 89 c3
    call 0a070h                               ; e8 44 ff
    test ax, ax                               ; 85 c0
    je short 0a141h                           ; 74 11
    mov di, bx                                ; 89 df
    mov es, dx                                ; 8e c2
    mov ax, 08106h                            ; b8 06 81
    mov dx, strict word 00000h                ; ba 00 00
    int 04bh                                  ; cd 4b
    jc short 0a140h                           ; 72 02
    db  032h, 0c0h
    ; xor al, al                                ; 32 c0
    cbw                                       ; 98
    mov es, dx                                ; 8e c2
    mov word [es:bx+00eh], strict word 00000h ; 26 c7 47 0e 00 00
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
__U4M:                                       ; 0xfa150 LB 0x20
    pushfw                                    ; 9c
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    db  08bh, 0fah
    ; mov di, dx                                ; 8b fa
    mul bx                                    ; f7 e3
    db  08bh, 0f2h
    ; mov si, dx                                ; 8b f2
    xchg di, ax                               ; 97
    mul bx                                    ; f7 e3
    db  003h, 0f0h
    ; add si, ax                                ; 03 f0
    pop ax                                    ; 58
    mul cx                                    ; f7 e1
    db  003h, 0f0h
    ; add si, ax                                ; 03 f0
    db  08bh, 0d6h
    ; mov dx, si                                ; 8b d6
    db  08bh, 0c7h
    ; mov ax, di                                ; 8b c7
    pop di                                    ; 5f
    pop si                                    ; 5e
    popfw                                     ; 9d
    retn                                      ; c3
    times 0x4 db 0
__U4D:                                       ; 0xfa170 LB 0x30
    pushfw                                    ; 9c
    push DS                                   ; 1e
    push ES                                   ; 06
    push bp                                   ; 55
    sub sp, strict byte 00004h                ; 83 ec 04
    db  08bh, 0ech
    ; mov bp, sp                                ; 8b ec
    push SS                                   ; 16
    push bp                                   ; 55
    add bp, strict byte 00004h                ; 83 c5 04
    push cx                                   ; 51
    push bx                                   ; 53
    push dx                                   ; 52
    push ax                                   ; 50
    call 0a255h                               ; e8 d0 00
    mov cx, word [bp-002h]                    ; 8b 4e fe
    mov bx, word [bp-004h]                    ; 8b 5e fc
    db  08bh, 0e5h
    ; mov sp, bp                                ; 8b e5
    pop bp                                    ; 5d
    pop ES                                    ; 07
    pop DS                                    ; 1f
    popfw                                     ; 9d
    retn                                      ; c3
    times 0xe db 0
__U8RS:                                      ; 0xfa1a0 LB 0x10
    test si, si                               ; 85 f6
    je short 0a1afh                           ; 74 0b
    shr ax, 1                                 ; d1 e8
    rcr bx, 1                                 ; d1 db
    rcr cx, 1                                 ; d1 d9
    rcr dx, 1                                 ; d1 da
    dec si                                    ; 4e
    jne short 0a1a4h                          ; 75 f5
    retn                                      ; c3
__U8LS:                                      ; 0xfa1b0 LB 0x10
    test si, si                               ; 85 f6
    je short 0a1bfh                           ; 74 0b
    sal dx, 1                                 ; d1 e2
    rcl cx, 1                                 ; d1 d1
    rcl bx, 1                                 ; d1 d3
    rcl ax, 1                                 ; d1 d0
    dec si                                    ; 4e
    jne short 0a1b4h                          ; 75 f5
    retn                                      ; c3
_fmemset_:                                   ; 0xfa1c0 LB 0x10
    push di                                   ; 57
    mov es, dx                                ; 8e c2
    db  08bh, 0f8h
    ; mov di, ax                                ; 8b f8
    xchg al, bl                               ; 86 d8
    rep stosb                                 ; f3 aa
    xchg al, bl                               ; 86 d8
    pop di                                    ; 5f
    retn                                      ; c3
    times 0x3 db 0
_fmemcpy_:                                   ; 0xfa1d0 LB 0x3a
    push bp                                   ; 55
    db  08bh, 0ech
    ; mov bp, sp                                ; 8b ec
    push di                                   ; 57
    push DS                                   ; 1e
    push si                                   ; 56
    mov es, dx                                ; 8e c2
    db  08bh, 0f8h
    ; mov di, ax                                ; 8b f8
    mov ds, cx                                ; 8e d9
    db  08bh, 0f3h
    ; mov si, bx                                ; 8b f3
    mov cx, word [bp+004h]                    ; 8b 4e 04
    rep movsb                                 ; f3 a4
    pop si                                    ; 5e
    pop DS                                    ; 1f
    pop di                                    ; 5f
    db  08bh, 0e5h
    ; mov sp, bp                                ; 8b e5
    pop bp                                    ; 5d
    retn                                      ; c3
    add byte [bx+si], al                      ; 00 00
    add byte [bx+si], al                      ; 00 00
    add byte [bx+si], al                      ; 00 00
    and byte [bp+si-05ddeh], ah               ; 20 a2 22 a2
    mov byte [es:0a226h], AL                  ; 26 a2 26 a2
    mov byte [es:0a228h], AL                  ; 26 a2 28 a2
    sub byte [bp+si-05dd6h], ah               ; 28 a2 2a a2
    mov byte [cs:0a22eh], AL                  ; 2e a2 2e a2
    xor byte [bp+si-05dcbh], ah               ; 30 a2 35 a2
    aaa                                       ; 37
    db  0a2h
apm_worker:                                  ; 0xfa20a LB 0x3a
    sti                                       ; fb
    push ax                                   ; 50
    db  032h, 0e4h
    ; xor ah, ah                                ; 32 e4
    sub AL, strict byte 004h                  ; 2c 04
    db  08bh, 0e8h
    ; mov bp, ax                                ; 8b e8
    sal bp, 1                                 ; d1 e5
    cmp AL, strict byte 00dh                  ; 3c 0d
    pop ax                                    ; 58
    mov AH, strict byte 053h                  ; b4 53
    jnc short 0a240h                          ; 73 25
    jmp word [cs:bp-05e10h]                   ; 2e ff a6 f0 a1
    jmp short 0a23eh                          ; eb 1c
    sti                                       ; fb
    hlt                                       ; f4
    jmp short 0a23eh                          ; eb 18
    jmp short 0a23eh                          ; eb 16
    jmp short 0a240h                          ; eb 16
    mov AH, strict byte 080h                  ; b4 80
    jmp short 0a242h                          ; eb 14
    jmp short 0a240h                          ; eb 10
    mov ax, 00102h                            ; b8 02 01
    jmp short 0a23eh                          ; eb 09
    jmp short 0a23eh                          ; eb 07
    mov BL, strict byte 000h                  ; b3 00
    mov cx, strict word 00000h                ; b9 00 00
    jmp short 0a23eh                          ; eb 00
    clc                                       ; f8
    retn                                      ; c3
    mov AH, strict byte 009h                  ; b4 09
    stc                                       ; f9
    retn                                      ; c3
apm_pm16_entry:                              ; 0xfa244 LB 0x11
    mov AH, strict byte 002h                  ; b4 02
    push DS                                   ; 1e
    push bp                                   ; 55
    push CS                                   ; 0e
    pop bp                                    ; 5d
    add bp, strict byte 00008h                ; 83 c5 08
    mov ds, bp                                ; 8e dd
    call 0a20ah                               ; e8 b8 ff
    pop bp                                    ; 5d
    pop DS                                    ; 1f
    retf                                      ; cb
_DoUInt32Div:                                ; 0xfa255 LB 0x26b
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00018h                ; 83 ec 18
    lds bx, [bp+00ch]                         ; c5 5e 0c
    lea si, [bp+004h]                         ; 8d 76 04
    mov word [bp-010h], si                    ; 89 76 f0
    mov [bp-00eh], ss                         ; 8c 56 f2
    lea di, [bp+008h]                         ; 8d 7e 08
    mov [bp-008h], ss                         ; 8c 56 f8
    lea si, [bp-01ch]                         ; 8d 76 e4
    mov word [bp-00ch], si                    ; 89 76 f4
    mov [bp-00ah], ss                         ; 8c 56 f6
    mov si, bx                                ; 89 de
    mov [bp-006h], ds                         ; 8c 5e fa
    cmp word [bx+002h], strict byte 00000h    ; 83 7f 02 00
    jne short 0a2a5h                          ; 75 22
    mov ax, word [bx]                         ; 8b 07
    test ax, ax                               ; 85 c0
    je short 0a2a2h                           ; 74 19
    cmp ax, strict word 00001h                ; 3d 01 00
    jne short 0a2a5h                          ; 75 17
    xor ax, ax                                ; 31 c0
    mov word [bp+00ah], ax                    ; 89 46 0a
    mov word [bp+008h], ax                    ; 89 46 08
    mov ax, word [bp-01ch]                    ; 8b 46 e4
    mov word [bp+004h], ax                    ; 89 46 04
    mov ax, word [bp-01ah]                    ; 8b 46 e6
    mov word [bp+006h], ax                    ; 89 46 06
    jmp near 0a4adh                           ; e9 08 02
    lds bx, [bp-00ch]                         ; c5 5e f4
    mov ax, word [bx+002h]                    ; 8b 47 02
    mov ds, [bp-006h]                         ; 8e 5e fa
    cmp ax, word [si+002h]                    ; 3b 44 02
    je short 0a2cbh                           ; 74 18
    mov ds, [bp-00ah]                         ; 8e 5e f6
    mov ax, word [bx+002h]                    ; 8b 47 02
    mov ds, [bp-006h]                         ; 8e 5e fa
    cmp ax, word [si+002h]                    ; 3b 44 02
    jbe short 0a2c6h                          ; 76 05
    mov ax, strict word 00001h                ; b8 01 00
    jmp short 0a2ech                          ; eb 26
    mov ax, strict word 0ffffh                ; b8 ff ff
    jmp short 0a2ech                          ; eb 21
    mov ds, [bp-00ah]                         ; 8e 5e f6
    mov ax, word [bx]                         ; 8b 07
    mov ds, [bp-006h]                         ; 8e 5e fa
    cmp ax, word [si]                         ; 3b 04
    je short 0a2eah                           ; 74 13
    mov ds, [bp-00ah]                         ; 8e 5e f6
    mov ax, word [bx]                         ; 8b 07
    mov ds, [bp-006h]                         ; 8e 5e fa
    cmp ax, word [si]                         ; 3b 04
    jbe short 0a2e5h                          ; 76 02
    jmp short 0a2c1h                          ; eb dc
    mov ax, strict word 0ffffh                ; b8 ff ff
    jmp short 0a2ech                          ; eb 02
    xor ax, ax                                ; 31 c0
    test ax, ax                               ; 85 c0
    jnl short 0a30eh                          ; 7d 1e
    lds bx, [bp-00ch]                         ; c5 5e f4
    mov ax, word [bx]                         ; 8b 07
    mov dx, word [bx+002h]                    ; 8b 57 02
    mov ds, [bp-008h]                         ; 8e 5e f8
    mov word [di], ax                         ; 89 05
    mov word [di+002h], dx                    ; 89 55 02
    lds bx, [bp-010h]                         ; c5 5e f0
    mov word [bx+002h], strict word 00000h    ; c7 47 02 00 00
    mov word [bx], strict word 00000h         ; c7 07 00 00
    jmp short 0a2a2h                          ; eb 94
    jne short 0a325h                          ; 75 15
    mov ds, [bp-008h]                         ; 8e 5e f8
    mov word [di+002h], ax                    ; 89 45 02
    mov word [di], ax                         ; 89 05
    lds bx, [bp-010h]                         ; c5 5e f0
    mov word [bx], strict word 00001h         ; c7 07 01 00
    mov word [bx+002h], ax                    ; 89 47 02
    jmp near 0a4adh                           ; e9 88 01
    lds bx, [bp-00ch]                         ; c5 5e f4
    mov ax, word [bx+002h]                    ; 8b 47 02
    test ax, ax                               ; 85 c0
    je short 0a340h                           ; 74 11
    push ax                                   ; 50
    mov ax, 0f000h                            ; b8 00 f0
    mov ds, ax                                ; 8e d8
    call 0a4c0h                               ; e8 88 01
    add sp, strict byte 00002h                ; 83 c4 02
    add ax, strict word 00010h                ; 05 10 00
    jmp short 0a34dh                          ; eb 0d
    push word [bx]                            ; ff 37
    mov ax, 0f000h                            ; b8 00 f0
    mov ds, ax                                ; 8e d8
    call 0a4c0h                               ; e8 76 01
    add sp, strict byte 00002h                ; 83 c4 02
    mov word [bp-014h], ax                    ; 89 46 ec
    mov ds, [bp-006h]                         ; 8e 5e fa
    mov ax, word [si+002h]                    ; 8b 44 02
    test ax, ax                               ; 85 c0
    je short 0a36bh                           ; 74 11
    push ax                                   ; 50
    mov ax, 0f000h                            ; b8 00 f0
    mov ds, ax                                ; 8e d8
    call 0a4c0h                               ; e8 5d 01
    add sp, strict byte 00002h                ; 83 c4 02
    add ax, strict word 00010h                ; 05 10 00
    jmp short 0a378h                          ; eb 0d
    push word [si]                            ; ff 34
    mov ax, 0f000h                            ; b8 00 f0
    mov ds, ax                                ; 8e d8
    call 0a4c0h                               ; e8 4b 01
    add sp, strict byte 00002h                ; 83 c4 02
    mov dx, word [bp-014h]                    ; 8b 56 ec
    sub dx, ax                                ; 29 c2
    mov word [bp-012h], dx                    ; 89 56 ee
    mov ds, [bp-006h]                         ; 8e 5e fa
    mov ax, word [si]                         ; 8b 04
    mov word [bp-018h], ax                    ; 89 46 e8
    mov ax, word [si+002h]                    ; 8b 44 02
    mov word [bp-016h], ax                    ; 89 46 ea
    test dx, dx                               ; 85 d2
    je short 0a3e8h                           ; 74 56
    mov cx, dx                                ; 89 d1
    xor ch, dh                                ; 30 f5
    and cl, 01fh                              ; 80 e1 1f
    mov ax, word [si]                         ; 8b 04
    mov dx, word [si+002h]                    ; 8b 54 02
    jcxz 0a3a6h                               ; e3 06
    sal ax, 1                                 ; d1 e0
    rcl dx, 1                                 ; d1 d2
    loop 0a3a0h                               ; e2 fa
    mov word [bp-018h], ax                    ; 89 46 e8
    mov word [bp-016h], dx                    ; 89 56 ea
    mov ax, word [bp-016h]                    ; 8b 46 ea
    lds bx, [bp-00ch]                         ; c5 5e f4
    cmp ax, word [bx+002h]                    ; 3b 47 02
    jnbe short 0a3c6h                         ; 77 0f
    mov ax, word [bp-016h]                    ; 8b 46 ea
    cmp ax, word [bx+002h]                    ; 3b 47 02
    jne short 0a3cah                          ; 75 0b
    mov ax, word [bp-018h]                    ; 8b 46 e8
    cmp ax, word [bx]                         ; 3b 07
    jbe short 0a3cah                          ; 76 04
    mov AL, strict byte 001h                  ; b0 01
    jmp short 0a3cch                          ; eb 02
    xor al, al                                ; 30 c0
    test al, al                               ; 84 c0
    je short 0a3f3h                           ; 74 23
    shr word [bp-016h], 1                     ; d1 6e ea
    rcr word [bp-018h], 1                     ; d1 5e e8
    dec word [bp-012h]                        ; ff 4e ee
    jmp short 0a3f3h                          ; eb 18
    mov cx, strict word 0001fh                ; b9 1f 00
    sal word [bp-018h], 1                     ; d1 66 e8
    rcl word [bp-016h], 1                     ; d1 56 ea
    loop 0a3deh                               ; e2 f8
    jmp short 0a3d6h                          ; eb ee
    mov ax, word [si]                         ; 8b 04
    mov word [bp-018h], ax                    ; 89 46 e8
    mov ax, word [si+002h]                    ; 8b 44 02
    mov word [bp-016h], ax                    ; 89 46 ea
    lds bx, [bp-010h]                         ; c5 5e f0
    mov word [bx+002h], strict word 00000h    ; c7 47 02 00 00
    mov word [bx], strict word 00000h         ; c7 07 00 00
    lds bx, [bp-00ch]                         ; c5 5e f4
    mov dx, word [bx]                         ; 8b 17
    mov ax, word [bx+002h]                    ; 8b 47 02
    mov ds, [bp-008h]                         ; 8e 5e f8
    mov word [di], dx                         ; 89 15
    mov word [di+002h], ax                    ; 89 45 02
    mov dx, word [di]                         ; 8b 15
    mov ds, [bp-006h]                         ; 8e 5e fa
    cmp ax, word [si+002h]                    ; 3b 44 02
    jnbe short 0a41fh                         ; 77 06
    jne short 0a423h                          ; 75 08
    cmp dx, word [si]                         ; 3b 14
    jc short 0a423h                           ; 72 04
    mov AL, strict byte 001h                  ; b0 01
    jmp short 0a425h                          ; eb 02
    xor al, al                                ; 30 c0
    test al, al                               ; 84 c0
    je short 0a48ah                           ; 74 61
    mov ds, [bp-008h]                         ; 8e 5e f8
    mov ax, word [di+002h]                    ; 8b 45 02
    cmp ax, word [bp-016h]                    ; 3b 46 ea
    jnbe short 0a440h                         ; 77 0c
    cmp ax, word [bp-016h]                    ; 3b 46 ea
    jne short 0a444h                          ; 75 0b
    mov ax, word [di]                         ; 8b 05
    cmp ax, word [bp-018h]                    ; 3b 46 e8
    jc short 0a444h                           ; 72 04
    mov AL, strict byte 001h                  ; b0 01
    jmp short 0a446h                          ; eb 02
    xor al, al                                ; 30 c0
    test al, al                               ; 84 c0
    je short 0a470h                           ; 74 26
    mov ax, word [bp-018h]                    ; 8b 46 e8
    mov ds, [bp-008h]                         ; 8e 5e f8
    sub word [di], ax                         ; 29 05
    mov ax, word [bp-016h]                    ; 8b 46 ea
    sbb word [di+002h], ax                    ; 19 45 02
    mov ax, strict word 00001h                ; b8 01 00
    xor dx, dx                                ; 31 d2
    mov cx, word [bp-012h]                    ; 8b 4e ee
    jcxz 0a468h                               ; e3 06
    sal ax, 1                                 ; d1 e0
    rcl dx, 1                                 ; d1 d2
    loop 0a462h                               ; e2 fa
    lds bx, [bp-010h]                         ; c5 5e f0
    or word [bx], ax                          ; 09 07
    or word [bx+002h], dx                     ; 09 57 02
    mov ds, [bp-008h]                         ; 8e 5e f8
    mov dx, word [di]                         ; 8b 15
    mov ax, word [di+002h]                    ; 8b 45 02
    mov ds, [bp-006h]                         ; 8e 5e fa
    cmp ax, word [si+002h]                    ; 3b 44 02
    jc short 0a486h                           ; 72 06
    jne short 0a48ch                          ; 75 0a
    cmp dx, word [si]                         ; 3b 14
    jnc short 0a48ch                          ; 73 06
    mov AL, strict byte 001h                  ; b0 01
    jmp short 0a48eh                          ; eb 04
    jmp short 0a4adh                          ; eb 21
    xor al, al                                ; 30 c0
    test al, al                               ; 84 c0
    je short 0a4a1h                           ; 74 0f
    jmp short 0a4adh                          ; eb 19
    mov cx, strict word 0001fh                ; b9 1f 00
    sal word [bp-018h], 1                     ; d1 66 e8
    rcl word [bp-016h], 1                     ; d1 56 ea
    loop 0a497h                               ; e2 f8
    jmp short 0a4a7h                          ; eb 06
    shr word [bp-016h], 1                     ; d1 6e ea
    rcr word [bp-018h], 1                     ; d1 5e e8
    dec word [bp-012h]                        ; ff 4e ee
    jmp near 0a429h                           ; e9 7c ff
    mov ax, word [bp-01ch]                    ; 8b 46 e4
    mov dx, word [bp-01ah]                    ; 8b 56 e6
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
    times 0x6 db 0
_ASMBitLastSetU16:                           ; 0xfa4c0 LB 0x18
    push bp                                   ; 55
    db  08bh, 0ech
    ; mov bp, sp                                ; 8b ec
    mov cx, word [bp+004h]                    ; 8b 4e 04
    test cx, cx                               ; 85 c9
    je short 0a4d4h                           ; 74 0a
    mov ax, strict word 00010h                ; b8 10 00
    sal cx, 1                                 ; d1 e1
    jc short 0a4d6h                           ; 72 05
    dec ax                                    ; 48
    jmp short 0a4cdh                          ; eb f9
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    pop bp                                    ; 5d
    retn                                      ; c3

  ; Padding 0x3528 bytes at 0xfa4d8
  times 13608 db 0

section BIOS32 progbits vstart=0xda00 align=1 ; size=0x3cb class=CODE group=AUTO
bios32_service:                              ; 0xfda00 LB 0x26
    pushfw                                    ; 9c
    cmp bl, 000h                              ; 80 fb 00
    jne short 0da22h                          ; 75 1c
    cmp ax, 05024h                            ; 3d 24 50
    inc bx                                    ; 43
    dec cx                                    ; 49
    mov AL, strict byte 080h                  ; b0 80
    jne short 0da20h                          ; 75 11
    mov bx, strict word 00000h                ; bb 00 00
    db  00fh
    add byte [bx+di-01000h], bh               ; 00 b9 00 f0
    add byte [bx+si], al                      ; 00 00
    mov dx, 0da26h                            ; ba 26 da
    add byte [bx+si], al                      ; 00 00
    db  032h, 0c0h
    ; xor al, al                                ; 32 c0
    popfw                                     ; 9d
    retf                                      ; cb
    mov AL, strict byte 081h                  ; b0 81
    jmp short 0da20h                          ; eb fa
pcibios32_entry:                             ; 0xfda26 LB 0x1a
    pushfw                                    ; 9c
    cld                                       ; fc
    push ES                                   ; 06
    pushaw                                    ; 60
    call 0db78h                               ; e8 4b 01
    add byte [bx+si], al                      ; 00 00
    popaw                                     ; 61
    pop ES                                    ; 07
    popfw                                     ; 9d
    retf                                      ; cb
    times 0xd db 0
apm_pm32_entry:                              ; 0xfda40 LB 0x21
    push bp                                   ; 55
    mov ebp, cs                               ; 66 8c cd
    push ebp                                  ; 66 55
    mov bp, 0da5fh                            ; bd 5f da
    add byte [bx+si], al                      ; 00 00
    push ebp                                  ; 66 55
    push CS                                   ; 0e
    pop bp                                    ; 5d
    add bp, strict byte 00008h                ; 83 c5 08
    push ebp                                  ; 66 55
    mov bp, 0a246h                            ; bd 46 a2
    add byte [bx+si], al                      ; 00 00
    push ebp                                  ; 66 55
    mov AH, strict byte 003h                  ; b4 03
    db  066h, 0cbh
    ; retf                                      ; 66 cb
    pop bp                                    ; 5d
    retf                                      ; cb
pci32_select_reg_:                           ; 0xfda61 LB 0x22
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    and dl, 0fch                              ; 80 e2 fc
    mov bx, dx                                ; 89 d3
    mov dx, 00cf8h                            ; ba f8 0c
    add byte [bx+si], al                      ; 00 00
    db  00fh, 0b7h, 0c0h
    ; movzx ax, ax                              ; 0f b7 c0
    sal ax, 008h                              ; c1 e0 08
    or ax, strict word 00000h                 ; 0d 00 00
    add byte [bx+si-03c76h], al               ; 00 80 8a c3
    out DX, ax                                ; ef
    lea sp, [di-004h]                         ; 8d 65 fc
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
pci32_find_device_:                          ; 0xfda83 LB 0xf7
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push cx                                   ; 51
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00014h                ; 83 ec 14
    push ax                                   ; 50
    mov cx, dx                                ; 89 d1
    mov si, bx                                ; 89 de
    test bx, bx                               ; 85 db
    xor bx, bx                                ; 31 db
    mov byte [di-010h], 000h                  ; c6 45 f0 00
    test bl, 007h                             ; f6 c3 07
    jne short 0dad4h                          ; 75 36
    db  00fh, 0b7h, 0c3h
    ; movzx ax, bx                              ; 0f b7 c3
    mov dx, strict word 0000eh                ; ba 0e 00
    add byte [bx+si], al                      ; 00 00
    call 0da5fh                               ; e8 b6 ff
    db  0ffh
    db  0ffh
    mov dx, 00cfeh                            ; ba fe 0c
    add byte [bx+si], al                      ; 00 00
    db  02bh, 0c0h
    ; sub ax, ax                                ; 2b c0
    in AL, DX                                 ; ec
    mov byte [di-014h], al                    ; 88 45 ec
    cmp AL, strict byte 0ffh                  ; 3c ff
    jne short 0dac2h                          ; 75 08
    add bx, strict byte 00008h                ; 83 c3 08
    jmp near 0db4ah                           ; e9 8a 00
    add byte [bx+si], al                      ; 00 00
    test byte [di-014h], 080h                 ; f6 45 ec 80
    je short 0dacfh                           ; 74 07
    mov di, strict word 00001h                ; bf 01 00
    add byte [bx+si], al                      ; 00 00
    jmp short 0dad4h                          ; eb 05
    mov di, strict word 00008h                ; bf 08 00
    add byte [bx+si], al                      ; 00 00
    mov al, byte [di-014h]                    ; 8a 45 ec
    and AL, strict byte 007h                  ; 24 07
    cmp AL, strict byte 001h                  ; 3c 01
    jne short 0db03h                          ; 75 26
    db  00fh, 0b7h, 0c3h
    ; movzx ax, bx                              ; 0f b7 c3
    mov dx, ax                                ; 89 c2
    sar dx, 008h                              ; c1 fa 08
    test dx, dx                               ; 85 d2
    jne short 0db03h                          ; 75 1a
    mov dx, strict word 0001ah                ; ba 1a 00
    add byte [bx+si], al                      ; 00 00
    call 0da5fh                               ; e8 6e ff
    db  0ffh
    db  0ffh
    mov dx, 00cfeh                            ; ba fe 0c
    add byte [bx+si], al                      ; 00 00
    db  02bh, 0c0h
    ; sub ax, ax                                ; 2b c0
    in AL, DX                                 ; ec
    cmp al, byte [di-010h]                    ; 3a 45 f0
    jbe short 0db03h                          ; 76 03
    mov byte [di-010h], al                    ; 88 45 f0
    test si, si                               ; 85 f6
    je short 0db0eh                           ; 74 07
    mov ax, strict word 00008h                ; b8 08 00
    add byte [bx+si], al                      ; 00 00
    jmp short 0db10h                          ; eb 02
    xor ax, ax                                ; 31 c0
    db  00fh, 0b7h, 0d0h
    ; movzx dx, ax                              ; 0f b7 d0
    db  00fh, 0b7h, 0c3h
    ; movzx ax, bx                              ; 0f b7 c3
    call 0da5fh                               ; e8 46 ff
    db  0ffh
    db  0ffh
    mov dx, 00cfch                            ; ba fc 0c
    add byte [bx+si], al                      ; 00 00
    in ax, DX                                 ; ed
    mov word [di-018h], ax                    ; 89 45 e8
    mov word [di-020h], strict word 00000h    ; c7 45 e0 00 00
    add byte [bx+si], al                      ; 00 00
    test si, si                               ; 85 f6
    je short 0db35h                           ; 74 06
    shr ax, 008h                              ; c1 e8 08
    mov word [di-018h], ax                    ; 89 45 e8
    mov ax, word [di-018h]                    ; 8b 45 e8
    cmp ax, word [di-024h]                    ; 3b 45 dc
    je short 0db43h                           ; 74 06
    cmp word [di-020h], strict byte 00000h    ; 83 7d e0 00
    je short 0db4ah                           ; 74 07
    dec cx                                    ; 49
    cmp ecx, strict byte 0ffffffffh           ; 66 83 f9 ff
    je short 0db62h                           ; 74 18
    add bx, di                                ; 01 fb
    db  00fh, 0b7h, 0c3h
    ; movzx ax, bx                              ; 0f b7 c3
    sar ax, 008h                              ; c1 f8 08
    mov word [di-01ch], ax                    ; 89 45 e4
    movzx ax, byte [di-010h]                  ; 0f b6 45 f0
    cmp ax, word [di-01ch]                    ; 3b 45 e4
    jnl near 0da97h                           ; 0f 8d 37 ff
    db  0ffh
    jmp word [bp-07dh]                        ; ff 66 83
    stc                                       ; f9
    push word [di+005h]                       ; ff 75 05
    db  00fh, 0b7h, 0c3h
    ; movzx ax, bx                              ; 0f b7 c3
    jmp short 0db72h                          ; eb 05
    mov ax, strict word 0ffffh                ; b8 ff ff
    add byte [bx+si], al                      ; 00 00
    lea sp, [di-00ch]                         ; 8d 65 f4
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bp                                    ; 5d
    retn                                      ; c3
_pci32_function:                             ; 0xfdb7a LB 0x251
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    push ax                                   ; 50
    and dword [di+024h], strict dword 0658100ffh ; 66 81 65 24 ff 00 81 65
    sub AL, strict byte 0feh                  ; 2c fe
    inc word [bx+si]                          ; ff 00
    add byte [bp+di+02445h], cl               ; 00 8b 45 24
    xor ah, ah                                ; 30 e4
    cmp eax, strict dword 029720003h          ; 66 3d 03 00 72 29
    jbe near 0dc37h                           ; 0f 86 99 00
    add byte [bx+si], al                      ; 00 00
    cmp eax, strict dword 0840f000eh          ; 66 3d 0e 00 0f 84
    test ax, strict word 00001h               ; a9 01 00
    add byte [bp+03dh], ah                    ; 00 66 3d
    or byte [bx+si], al                       ; 08 00
    jc near 0ddb1h                            ; 0f 82 ff 01
    add byte [bx+si], al                      ; 00 00
    cmp eax, strict dword 0860f000dh          ; 66 3d 0d 00 0f 86
    test AL, strict byte 000h                 ; a8 00
    add byte [bx+si], al                      ; 00 00
    jmp near 0ddb1h                           ; e9 f0 01
    add byte [bx+si], al                      ; 00 00
    cmp eax, strict dword 028740002h          ; 66 3d 02 00 74 28
    cmp eax, strict dword 0850f0001h          ; 66 3d 01 00 0f 85
    loopne 0dbd2h                             ; e0 01
    add byte [bx+si], al                      ; 00 00
    mov dword [di+024h], strict dword 0c7660001h ; 66 c7 45 24 01 00 66 c7
    inc bp                                    ; 45
    sbb byte [bx+si], dl                      ; 18 10
    add dh, byte [bx+di]                      ; 02 31
    sal byte [bp-077h], 045h                  ; c0 66 89 45
    and bh, al                                ; 20 c7
    inc bp                                    ; 45
    sbb AL, strict byte 050h                  ; 1c 50
    inc bx                                    ; 43
    dec cx                                    ; 49
    and cl, ch                                ; 20 e9
    rol byte [bx+di], CL                      ; d2 01
    add byte [bx+si], al                      ; 00 00
    cmp dword [di+01ch], strict byte 0ffffffffh ; 66 83 7d 1c ff
    jne short 0dc05h                          ; 75 0d
    mov ax, word [di+024h]                    ; 8b 45 24
    xor ah, ah                                ; 30 e4
    or ah, 083h                               ; 80 cc 83
    jmp near 0ddb9h                           ; e9 b6 01
    add byte [bx+si], al                      ; 00 00
    xor bx, bx                                ; 31 db
    db  00fh, 0b7h, 055h, 00ch
    ; movzx dx, [di+00ch]                       ; 0f b7 55 0c
    db  00fh, 0b7h, 045h, 020h
    ; movzx ax, [di+020h]                       ; 0f b7 45 20
    sal ax, 010h                              ; c1 e0 10
    db  00fh, 0b7h, 04dh, 01ch
    ; movzx cx, [di+01ch]                       ; 0f b7 4d 1c
    or ax, cx                                 ; 09 c8
    call 0da81h                               ; e8 66 fe
    db  0ffh
    jmp word [bp+03dh]                        ; ff 66 3d
    db  0ffh
    push word [di+00dh]                       ; ff 75 0d
    mov ax, word [di+024h]                    ; 8b 45 24
    xor ah, ah                                ; 30 e4
    or ah, 086h                               ; 80 cc 86
    jmp near 0ddb9h                           ; e9 8b 01
    add byte [bx+si], al                      ; 00 00
    mov dword [di+018h], eax                  ; 66 89 45 18
    jmp near 0ddc1h                           ; e9 8a 01
    add byte [bx+si], al                      ; 00 00
    db  00fh, 0b7h, 055h, 00ch
    ; movzx dx, [di+00ch]                       ; 0f b7 55 0c
    mov ax, word [di+020h]                    ; 8b 45 20
    mov bx, strict word 00001h                ; bb 01 00
    add byte [bx+si], al                      ; 00 00
    call 0da81h                               ; e8 39 fe
    db  0ffh
    jmp word [bp+03dh]                        ; ff 66 3d
    db  0ffh
    push word [di+00dh]                       ; ff 75 0d
    mov ax, word [di+024h]                    ; 8b 45 24
    xor ah, ah                                ; 30 e4
    or ah, 086h                               ; 80 cc 86
    jmp near 0ddb9h                           ; e9 5e 01
    add byte [bx+si], al                      ; 00 00
    mov dword [di+018h], eax                  ; 66 89 45 18
    jmp near 0ddc1h                           ; e9 5d 01
    add byte [bx+si], al                      ; 00 00
    cmp dword [di+008h], strict dword 00d720100h ; 66 81 7d 08 00 01 72 0d
    mov ax, word [di+024h]                    ; 8b 45 24
    xor ah, ah                                ; 30 e4
    or ah, 087h                               ; 80 cc 87
    jmp near 0ddb9h                           ; e9 40 01
    add byte [bx+si], al                      ; 00 00
    db  00fh, 0b7h, 055h, 008h
    ; movzx dx, [di+008h]                       ; 0f b7 55 08
    db  00fh, 0b7h, 045h, 018h
    ; movzx ax, [di+018h]                       ; 0f b7 45 18
    call 0da5fh                               ; e8 d9 fd
    db  0ffh
    dec word [bp+di+02445h]                   ; ff 8b 45 24
    xor ah, ah                                ; 30 e4
    cmp eax, strict dword 02172000ah          ; 66 3d 0a 00 72 21
    jbe short 0dd04h                          ; 76 6f
    cmp eax, strict dword 0840f000dh          ; 66 3d 0d 00 0f 84
    test ax, strict word 00000h               ; a9 00 00
    add byte [bp+03dh], ah                    ; 00 66 3d
    or AL, strict byte 000h                   ; 0c 00
    je near 0dd2ah                            ; 0f 84 83 00
    add byte [bx+si], al                      ; 00 00
    cmp eax, strict dword 06374000bh          ; 66 3d 0b 00 74 63
    jmp near 0ddc1h                           ; e9 0f 01
    add byte [bx+si], al                      ; 00 00
    cmp eax, strict dword 02d740009h          ; 66 3d 09 00 74 2d
    cmp eax, strict dword 0850f0008h          ; 66 3d 08 00 0f 85
    inc word [bx+si]                          ; ff 00
    add byte [bx+si], al                      ; 00 00
    mov bx, word [di+020h]                    ; 8b 5d 20
    xor bl, bl                                ; 30 db
    mov ax, word [di+008h]                    ; 8b 45 08
    xor ah, ah                                ; 30 e4
    and AL, strict byte 003h                  ; 24 03
    db  00fh, 0b7h, 0d0h
    ; movzx dx, ax                              ; 0f b7 d0
    add dx, 00cfch                            ; 81 c2 fc 0c
    add byte [bx+si], al                      ; 00 00
    db  02bh, 0c0h
    ; sub ax, ax                                ; 2b c0
    in AL, DX                                 ; ec
    or bx, ax                                 ; 09 c3
    mov dword [di+020h], ebx                  ; 66 89 5d 20
    jmp near 0ddc1h                           ; e9 dc 00
    add byte [bx+si], al                      ; 00 00
    mov ax, word [di+008h]                    ; 8b 45 08
    xor ah, ah                                ; 30 e4
    and AL, strict byte 002h                  ; 24 02
    db  00fh, 0b7h, 0d0h
    ; movzx dx, ax                              ; 0f b7 d0
    add dx, 00cfch                            ; 81 c2 fc 0c
    add byte [bx+si], al                      ; 00 00
    db  02bh, 0c0h
    ; sub ax, ax                                ; 2b c0
    in eax, DX                                ; 66 ed
    mov dword [di+020h], eax                  ; 66 89 45 20
    jmp near 0ddc1h                           ; e9 bf 00
    add byte [bx+si], al                      ; 00 00
    mov dx, 00cfch                            ; ba fc 0c
    add byte [bx+si], al                      ; 00 00
    in ax, DX                                 ; ed
    mov word [di+020h], ax                    ; 89 45 20
    jmp near 0ddc1h                           ; e9 b1 00
    add byte [bx+si], al                      ; 00 00
    mov ax, word [di+020h]                    ; 8b 45 20
    mov dx, word [di+008h]                    ; 8b 55 08
    xor dh, dh                                ; 30 f6
    and dl, 003h                              ; 80 e2 03
    db  00fh, 0b7h, 0d2h
    ; movzx dx, dx                              ; 0f b7 d2
    add dx, 00cfch                            ; 81 c2 fc 0c
    add byte [bx+si], al                      ; 00 00
    out DX, AL                                ; ee
    jmp near 0ddc1h                           ; e9 97 00
    add byte [bx+si], al                      ; 00 00
    db  00fh, 0b7h, 045h, 020h
    ; movzx ax, [di+020h]                       ; 0f b7 45 20
    mov dx, word [di+008h]                    ; 8b 55 08
    xor dh, dh                                ; 30 f6
    and dl, 002h                              ; 80 e2 02
    db  00fh, 0b7h, 0d2h
    ; movzx dx, dx                              ; 0f b7 d2
    add dx, 00cfch                            ; 81 c2 fc 0c
    add byte [bx+si], al                      ; 00 00
    out DX, eax                               ; 66 ef
    jmp near 0ddc1h                           ; e9 7b 00
    add byte [bx+si], al                      ; 00 00
    mov ax, word [di+020h]                    ; 8b 45 20
    mov dx, 00cfch                            ; ba fc 0c
    add byte [bx+si], al                      ; 00 00
    out DX, ax                                ; ef
    jmp short 0ddc3h                          ; eb 70
    db  00fh, 0b7h, 045h, 008h
    ; movzx ax, [di+008h]                       ; 0f b7 45 08
    mov es, [di+028h]                         ; 8e 45 28
    mov [di-010h], es                         ; 8c 45 f0
    mov bx, ax                                ; 89 c3
    mov edx, dword [di]                       ; 66 8b 15
    xor bl, 000h                              ; 80 f3 00
    add byte [bp+026h], ah                    ; 00 66 26
    cmp dx, word [bx+si]                      ; 3b 10
    jbe short 0dd7eh                          ; 76 12
    mov ax, word [di+024h]                    ; 8b 45 24
    xor ah, ah                                ; 30 e4
    or ah, 089h                               ; 80 cc 89
    mov dword [di+024h], eax                  ; 66 89 45 24
    or word [di+02ch], strict byte 00001h     ; 83 4d 2c 01
    jmp short 0dda4h                          ; eb 26
    db  00fh, 0b7h, 0cah
    ; movzx cx, dx                              ; 0f b7 ca
    db  066h, 026h, 08bh, 050h, 006h
    ; mov edx, dword [es:bx+si+006h]            ; 66 26 8b 50 06
    mov word [di-014h], dx                    ; 89 55 ec
    mov di, word [es:bx+si+002h]              ; 26 8b 78 02
    mov dx, ds                                ; 8c da
    mov si, 0f1a0h                            ; be a0 f1
    add byte [bx+si], al                      ; 00 00
    mov es, [di-014h]                         ; 8e 45 ec
    push DS                                   ; 1e
    db  066h, 08eh, 0dah
    ; mov ds, edx                               ; 66 8e da
    rep movsb                                 ; f3 a4
    pop DS                                    ; 1f
    mov dword [di+018h], strict dword 0a1660a00h ; 66 c7 45 18 00 0a 66 a1
    xor bl, 000h                              ; 80 f3 00
    add byte [bp-00fbbh], cl                  ; 00 8e 45 f0
    db  066h, 026h, 089h, 003h
    ; mov dword [es:bp+di], eax                 ; 66 26 89 03
    jmp short 0ddc3h                          ; eb 10
    mov ax, word [di+024h]                    ; 8b 45 24
    xor ah, ah                                ; 30 e4
    or ah, 081h                               ; 80 cc 81
    mov dword [di+024h], eax                  ; 66 89 45 24
    or word [di+02ch], strict byte 00001h     ; 83 4d 2c 01
    lea sp, [di-00ch]                         ; 8d 65 f4
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3

  ; Padding 0x1 bytes at 0xfddcb
  times 1 db 0

section BIOS32CONST progbits vstart=0xddcc align=1 ; size=0x0 class=FAR_DATA group=BIOS32_GROUP

section BIOS32CONST2 progbits vstart=0xddcc align=1 ; size=0x0 class=FAR_DATA group=BIOS32_GROUP

section BIOS32_DATA progbits vstart=0xddcc align=1 ; size=0x0 class=FAR_DATA group=BIOS32_GROUP

  ; Padding 0x234 bytes at 0xfddcc
  times 564 db 0

section BIOSSEG progbits vstart=0xe000 align=1 ; size=0x2000 class=CODE group=AUTO
biosorg_check_before_or_at_0E02Eh:           ; 0xfe000 LB 0x30
    times 0x2e db 0
    db  'XM'
eoi_both_pics:                               ; 0xfe030 LB 0x4
    mov AL, strict byte 020h                  ; b0 20
    out strict byte 0a0h, AL                  ; e6 a0
eoi_master_pic:                              ; 0xfe034 LB 0x5
    mov AL, strict byte 020h                  ; b0 20
    out strict byte 020h, AL                  ; e6 20
    retn                                      ; c3
set_int_vects:                               ; 0xfe039 LB 0xb
    mov word [bx], ax                         ; 89 07
    mov word [bx+002h], dx                    ; 89 57 02
    add bx, strict byte 00004h                ; 83 c3 04
    loop 0e039h                               ; e2 f6
    retn                                      ; c3
eoi_jmp_post:                                ; 0xfe044 LB 0x3
    call 0e030h                               ; e8 e9 ff
no_eoi_jmp_post:                             ; 0xfe047 LB 0x8
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    mov ds, ax                                ; 8e d8
    jmp far [00467h]                          ; ff 2e 67 04
seg_40_value:                                ; 0xfe04f LB 0x2
    inc ax                                    ; 40
    times 0x1 db 0
biosorg_check_before_or_at_0E059h:           ; 0xfe051 LB 0xa
    times 0x8 db 0
    db  'XM'
post:                                        ; 0xfe05b LB 0x65
    cli                                       ; fa
    jmp short 0e064h                          ; eb 06
    mov AL, strict byte 001h                  ; b0 01
    out strict byte 092h, AL                  ; e6 92
    jmp short 0e062h                          ; eb fe
    mov AL, strict byte 00fh                  ; b0 0f
    out strict byte 070h, AL                  ; e6 70
    in AL, strict byte 071h                   ; e4 71
    xchg ah, al                               ; 86 c4
    in AL, strict byte 064h                   ; e4 64
    test AL, strict byte 004h                 ; a8 04
    je short 0e085h                           ; 74 13
    db  08ah, 0c4h
    ; mov al, ah                                ; 8a c4
    db  00ah, 0c0h
    ; or al, al                                 ; 0a c0
    jne short 0e085h                          ; 75 0d
    mov ds, [cs:0e04fh]                       ; 2e 8e 1e 4f e0
    cmp word [word 00072h], 01234h            ; 81 3e 72 00 34 12
    jne short 0e05eh                          ; 75 d9
    mov AL, strict byte 00fh                  ; b0 0f
    out strict byte 070h, AL                  ; e6 70
    mov AL, strict byte 000h                  ; b0 00
    out strict byte 071h, AL                  ; e6 71
    db  08ah, 0c4h
    ; mov al, ah                                ; 8a c4
    cmp AL, strict byte 009h                  ; 3c 09
    je short 0e0a5h                           ; 74 12
    cmp AL, strict byte 00ah                  ; 3c 0a
    je short 0e0a5h                           ; 74 0e
    db  032h, 0c0h
    ; xor al, al                                ; 32 c0
    out strict byte 00dh, AL                  ; e6 0d
    out strict byte 0dah, AL                  ; e6 da
    mov AL, strict byte 0c0h                  ; b0 c0
    out strict byte 0d6h, AL                  ; e6 d6
    mov AL, strict byte 000h                  ; b0 00
    out strict byte 0d4h, AL                  ; e6 d4
    db  08ah, 0c4h
    ; mov al, ah                                ; 8a c4
    cmp AL, strict byte 000h                  ; 3c 00
    je short 0e0c0h                           ; 74 15
    cmp AL, strict byte 00dh                  ; 3c 0d
    jnc short 0e0c0h                          ; 73 11
    cmp AL, strict byte 009h                  ; 3c 09
    jne short 0e0b6h                          ; 75 03
    jmp near 0e350h                           ; e9 9a 02
    cmp AL, strict byte 005h                  ; 3c 05
    je short 0e044h                           ; 74 8a
    cmp AL, strict byte 00ah                  ; 3c 0a
    je short 0e047h                           ; 74 89
    jmp short 0e0c0h                          ; eb 00
normal_post:                                 ; 0xfe0c0 LB 0x1f3
    mov ax, 07800h                            ; b8 00 78
    db  08bh, 0e0h
    ; mov sp, ax                                ; 8b e0
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    mov ds, ax                                ; 8e d8
    mov ss, ax                                ; 8e d0
    mov es, ax                                ; 8e c0
    db  033h, 0ffh
    ; xor di, di                                ; 33 ff
    cld                                       ; fc
    mov cx, 00239h                            ; b9 39 02
    rep stosw                                 ; f3 ab
    inc di                                    ; 47
    inc di                                    ; 47
    mov cx, 005c6h                            ; b9 c6 05
    rep stosw                                 ; f3 ab
    db  033h, 0dbh
    ; xor bx, bx                                ; 33 db
    add bx, 01000h                            ; 81 c3 00 10
    cmp bx, 09000h                            ; 81 fb 00 90
    jnc short 0e0f3h                          ; 73 0b
    mov es, bx                                ; 8e c3
    db  033h, 0ffh
    ; xor di, di                                ; 33 ff
    mov cx, 08000h                            ; b9 00 80
    rep stosw                                 ; f3 ab
    jmp short 0e0deh                          ; eb eb
    mov es, bx                                ; 8e c3
    db  033h, 0ffh
    ; xor di, di                                ; 33 ff
    mov cx, 07ff8h                            ; b9 f8 7f
    rep stosw                                 ; f3 ab
    db  033h, 0dbh
    ; xor bx, bx                                ; 33 db
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 01775h                               ; e8 71 36
    db  033h, 0dbh
    ; xor bx, bx                                ; 33 db
    mov ds, bx                                ; 8e db
    mov cx, strict word 00060h                ; b9 60 00
    mov ax, 0ff53h                            ; b8 53 ff
    mov dx, 0f000h                            ; ba 00 f0
    call 0e039h                               ; e8 25 ff
    mov bx, 001a0h                            ; bb a0 01
    mov cx, strict word 00010h                ; b9 10 00
    call 0e039h                               ; e8 1c ff
    mov ax, 0027fh                            ; b8 7f 02
    mov word [00413h], ax                     ; a3 13 04
    mov ax, 0e9dah                            ; b8 da e9
    mov word [00018h], ax                     ; a3 18 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [0001ah], ax                     ; a3 1a 00
    mov ax, 0f84dh                            ; b8 4d f8
    mov word [00044h], ax                     ; a3 44 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [00046h], ax                     ; a3 46 00
    mov ax, 0f841h                            ; b8 41 f8
    mov word [00048h], ax                     ; a3 48 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [0004ah], ax                     ; a3 4a 00
    mov ax, 0f859h                            ; b8 59 f8
    mov word [00054h], ax                     ; a3 54 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [00056h], ax                     ; a3 56 00
    mov ax, 0efd4h                            ; b8 d4 ef
    mov word [0005ch], ax                     ; a3 5c 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [0005eh], ax                     ; a3 5e 00
    mov ax, 0f0a4h                            ; b8 a4 f0
    mov word [00060h], ax                     ; a3 60 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [00062h], ax                     ; a3 62 00
    mov ax, 0e6f2h                            ; b8 f2 e6
    mov word [00064h], ax                     ; a3 64 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [00066h], ax                     ; a3 66 00
    mov ax, 0effbh                            ; b8 fb ef
    mov word [00070h], ax                     ; a3 70 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [00072h], ax                     ; a3 72 00
    call 0e794h                               ; e8 0e 06
    mov ax, 0fe6eh                            ; b8 6e fe
    mov word [00068h], ax                     ; a3 68 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [0006ah], ax                     ; a3 6a 00
    mov ax, 0fea5h                            ; b8 a5 fe
    mov word [00020h], ax                     ; a3 20 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [00022h], ax                     ; a3 22 00
    mov AL, strict byte 034h                  ; b0 34
    out strict byte 043h, AL                  ; e6 43
    mov AL, strict byte 000h                  ; b0 00
    out strict byte 040h, AL                  ; e6 40
    out strict byte 040h, AL                  ; e6 40
    mov ax, 0f065h                            ; b8 65 f0
    mov word [00040h], ax                     ; a3 40 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [00042h], ax                     ; a3 42 00
    mov ax, 0e987h                            ; b8 87 e9
    mov word [00024h], ax                     ; a3 24 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [00026h], ax                     ; a3 26 00
    mov ax, 0e82eh                            ; b8 2e e8
    mov word [00058h], ax                     ; a3 58 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [0005ah], ax                     ; a3 5a 00
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    mov ds, ax                                ; 8e d8
    mov byte [00417h], AL                     ; a2 17 04
    mov byte [00418h], AL                     ; a2 18 04
    mov byte [00419h], AL                     ; a2 19 04
    mov byte [00471h], AL                     ; a2 71 04
    mov byte [00497h], AL                     ; a2 97 04
    mov AL, strict byte 010h                  ; b0 10
    mov byte [00496h], AL                     ; a2 96 04
    mov bx, strict word 0001eh                ; bb 1e 00
    mov word [0041ah], bx                     ; 89 1e 1a 04
    mov word [0041ch], bx                     ; 89 1e 1c 04
    mov word [00480h], bx                     ; 89 1e 80 04
    mov bx, strict word 0003eh                ; bb 3e 00
    mov word [00482h], bx                     ; 89 1e 82 04
    mov AL, strict byte 014h                  ; b0 14
    out strict byte 070h, AL                  ; e6 70
    in AL, strict byte 071h                   ; e4 71
    mov byte [00410h], AL                     ; a2 10 04
    push DS                                   ; 1e
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    mov ax, 0c000h                            ; b8 00 c0
    mov dx, 0c800h                            ; ba 00 c8
    call 01600h                               ; e8 f0 33
    call 0503ch                               ; e8 29 6e
    pop DS                                    ; 1f
    mov ax, 0ff53h                            ; b8 53 ff
    mov word [0003ch], ax                     ; a3 3c 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [0003eh], ax                     ; a3 3e 00
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    mov ds, ax                                ; 8e d8
    db  033h, 0dbh
    ; xor bx, bx                                ; 33 db
    mov CL, strict byte 014h                  ; b1 14
    mov dx, 00378h                            ; ba 78 03
    call 0ed0ah                               ; e8 dc 0a
    mov dx, 00278h                            ; ba 78 02
    call 0ed0ah                               ; e8 d6 0a
    push cx                                   ; 51
    mov CL, strict byte 00eh                  ; b1 0e
    sal bx, CL                                ; d3 e3
    pop cx                                    ; 59
    mov ax, word [00410h]                     ; a1 10 04
    and ax, 03fffh                            ; 25 ff 3f
    db  00bh, 0c3h
    ; or ax, bx                                 ; 0b c3
    mov word [00410h], ax                     ; a3 10 04
    mov ax, 0e754h                            ; b8 54 e7
    mov word [0002ch], ax                     ; a3 2c 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [0002eh], ax                     ; a3 2e 00
    mov ax, 0e754h                            ; b8 54 e7
    mov word [00030h], ax                     ; a3 30 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [00032h], ax                     ; a3 32 00
    mov ax, 0e739h                            ; b8 39 e7
    mov word [00050h], ax                     ; a3 50 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [00052h], ax                     ; a3 52 00
    db  033h, 0dbh
    ; xor bx, bx                                ; 33 db
    mov CL, strict byte 00ah                  ; b1 0a
    mov dx, 003f8h                            ; ba f8 03
    call 0ed28h                               ; e8 b5 0a
    mov dx, 002f8h                            ; ba f8 02
    call 0ed28h                               ; e8 af 0a
    mov dx, 003e8h                            ; ba e8 03
    call 0ed28h                               ; e8 a9 0a
    mov dx, 002e8h                            ; ba e8 02
    call 0ed28h                               ; e8 a3 0a
    push cx                                   ; 51
    mov CL, strict byte 009h                  ; b1 09
    sal bx, CL                                ; d3 e3
    pop cx                                    ; 59
    mov ax, word [00410h]                     ; a1 10 04
    and ax, 0f1ffh                            ; 25 ff f1
    db  00bh, 0c3h
    ; or ax, bx                                 ; 0b c3
    mov word [00410h], ax                     ; a3 10 04
    mov ax, 0ff53h                            ; b8 53 ff
    mov word [00128h], ax                     ; a3 28 01
    mov ax, 0f000h                            ; b8 00 f0
    mov word [0012ah], ax                     ; a3 2a 01
    mov ax, 0f915h                            ; b8 15 f9
    mov word [001c0h], ax                     ; a3 c0 01
    mov ax, 0f000h                            ; b8 00 f0
    mov word [001c2h], ax                     ; a3 c2 01
    call 0ede3h                               ; e8 32 0b
    jmp short 0e303h                          ; eb 50
biosorg_check_before_or_at_0E2C1h:           ; 0xfe2b3 LB 0x10
    times 0xe db 0
    db  'XM'
nmi:                                         ; 0xfe2c3 LB 0x7
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 0174bh                               ; e8 82 34
    iret                                      ; cf
int75_handler:                               ; 0xfe2ca LB 0x8
    out strict byte 0f0h, AL                  ; e6 f0
    call 0e030h                               ; e8 61 fd
    int 002h                                  ; cd 02
    iret                                      ; cf
hard_drive_post:                             ; 0xfe2d2 LB 0xa7
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    mov ds, ax                                ; 8e d8
    mov byte [00474h], AL                     ; a2 74 04
    mov byte [00477h], AL                     ; a2 77 04
    mov byte [0048ch], AL                     ; a2 8c 04
    mov byte [0048dh], AL                     ; a2 8d 04
    mov byte [0048eh], AL                     ; a2 8e 04
    mov AL, strict byte 0c0h                  ; b0 c0
    mov byte [00476h], AL                     ; a2 76 04
    mov ax, 0e3feh                            ; b8 fe e3
    mov word [0004ch], ax                     ; a3 4c 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [0004eh], ax                     ; a3 4e 00
    mov ax, 0f903h                            ; b8 03 f9
    mov word [001d8h], ax                     ; a3 d8 01
    mov ax, 0f000h                            ; b8 00 f0
    mov word [001dah], ax                     ; a3 da 01
    retn                                      ; c3
    mov ax, 0f8c9h                            ; b8 c9 f8
    mov word [001d0h], ax                     ; a3 d0 01
    mov ax, 0f000h                            ; b8 00 f0
    mov word [001d2h], ax                     ; a3 d2 01
    mov ax, 0e2cah                            ; b8 ca e2
    mov word [001d4h], ax                     ; a3 d4 01
    mov ax, 0f000h                            ; b8 00 f0
    mov word [001d6h], ax                     ; a3 d6 01
    call 0e76fh                               ; e8 51 04
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 01cc5h                               ; e8 a1 39
    call 02154h                               ; e8 2d 3e
    sti                                       ; fb
    call 09a44h                               ; e8 19 b7
    cli                                       ; fa
    call 08891h                               ; e8 62 a5
    call 0ed4ch                               ; e8 1a 0a
    call 0e2d2h                               ; e8 9d ff
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    mov ax, 0c800h                            ; b8 00 c8
    mov dx, 0f000h                            ; ba 00 f0
    call 01600h                               ; e8 bf 32
    call 01799h                               ; e8 55 34
    call 03c55h                               ; e8 0e 59
    sti                                       ; fb
    int 019h                                  ; cd 19
    sti                                       ; fb
    hlt                                       ; f4
    jmp short 0e34bh                          ; eb fd
    cli                                       ; fa
    hlt                                       ; f4
    mov ax, strict word 00040h                ; b8 40 00
    mov ds, ax                                ; 8e d8
    mov ss, [word 00069h]                     ; 8e 16 69 00
    mov sp, word [word 00067h]                ; 8b 26 67 00
    in AL, strict byte 092h                   ; e4 92
    and AL, strict byte 0fdh                  ; 24 fd
    out strict byte 092h, AL                  ; e6 92
    lidt [cs:0eff5h]                          ; 2e 0f 01 1e f5 ef
    pop DS                                    ; 1f
    pop ES                                    ; 07
    db  08bh, 0ech
    ; mov bp, sp                                ; 8b ec
    in AL, strict byte 080h                   ; e4 80
    mov byte [bp+00fh], al                    ; 88 46 0f
    db  03ah, 0e0h
    ; cmp ah, al                                ; 3a e0
    popaw                                     ; 61
    sti                                       ; fb
    retf 00002h                               ; ca 02 00
biosorg_check_before_or_at_0E3FCh:           ; 0xfe379 LB 0x85
    times 0x83 db 0
    db  'XM'
int13_handler:                               ; 0xfe3fe LB 0x3
    jmp near 0ec5bh                           ; e9 5a 08
rom_fdpt:                                    ; 0xfe401 LB 0x2f1
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 058h
    db  04dh
int19_handler:                               ; 0xfe6f2 LB 0x3
    jmp near 0f0ach                           ; e9 b7 09
biosorg_check_at_0E6F5h:                     ; 0xfe6f5 LB 0xa
    or word [bx+si], ax                       ; 09 00
    sti                                       ; fb
    add byte [bx+di], al                      ; 00 01
    je short 0e73ch                           ; 74 40
    times 0x3 db 0
biosorg_check_before_or_at_0E727h:           ; 0xfe6ff LB 0x2a
    times 0x28 db 0
    db  'XM'
biosorg_check_at_0E729h:                     ; 0xfe729 LB 0x10
    times 0xe db 0
    db  'XM'
biosorg_check_at_0E739h:                     ; 0xfe739 LB 0x36
    push DS                                   ; 1e
    push ES                                   ; 06
    push ax                                   ; 50
    push cx                                   ; 51
    push dx                                   ; 52
    push bx                                   ; 53
    push sp                                   ; 54
    push bp                                   ; 55
    push si                                   ; 56
    push di                                   ; 57
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 065ffh                               ; e8 b6 7e
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    pop bx                                    ; 5b
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop ax                                    ; 58
    pop ES                                    ; 07
    pop DS                                    ; 1f
    iret                                      ; cf
    push DS                                   ; 1e
    push ES                                   ; 06
    push ax                                   ; 50
    push cx                                   ; 51
    push dx                                   ; 52
    push bx                                   ; 53
    push sp                                   ; 54
    push bp                                   ; 55
    push si                                   ; 56
    push di                                   ; 57
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 016e6h                               ; e8 82 2f
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    pop bx                                    ; 5b
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop ax                                    ; 58
    pop ES                                    ; 07
    pop DS                                    ; 1f
    iret                                      ; cf
init_pic:                                    ; 0xfe76f LB 0x25
    mov AL, strict byte 011h                  ; b0 11
    out strict byte 020h, AL                  ; e6 20
    out strict byte 0a0h, AL                  ; e6 a0
    mov AL, strict byte 008h                  ; b0 08
    out strict byte 021h, AL                  ; e6 21
    mov AL, strict byte 070h                  ; b0 70
    out strict byte 0a1h, AL                  ; e6 a1
    mov AL, strict byte 004h                  ; b0 04
    out strict byte 021h, AL                  ; e6 21
    mov AL, strict byte 002h                  ; b0 02
    out strict byte 0a1h, AL                  ; e6 a1
    mov AL, strict byte 001h                  ; b0 01
    out strict byte 021h, AL                  ; e6 21
    out strict byte 0a1h, AL                  ; e6 a1
    mov AL, strict byte 0b8h                  ; b0 b8
    out strict byte 021h, AL                  ; e6 21
    mov AL, strict byte 08fh                  ; b0 8f
    out strict byte 0a1h, AL                  ; e6 a1
    retn                                      ; c3
ebda_post:                                   ; 0xfe794 LB 0x51
    mov ax, 0e754h                            ; b8 54 e7
    mov word [00034h], ax                     ; a3 34 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [00036h], ax                     ; a3 36 00
    mov ax, 0e754h                            ; b8 54 e7
    mov word [0003ch], ax                     ; a3 3c 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [0003eh], ax                     ; a3 3e 00
    mov ax, 0e754h                            ; b8 54 e7
    mov word [001c8h], ax                     ; a3 c8 01
    mov ax, 0f000h                            ; b8 00 f0
    mov word [001cah], ax                     ; a3 ca 01
    mov ax, 0e754h                            ; b8 54 e7
    mov word [001cch], ax                     ; a3 cc 01
    mov ax, 0f000h                            ; b8 00 f0
    mov word [001ceh], ax                     ; a3 ce 01
    mov ax, 0e754h                            ; b8 54 e7
    mov word [001dch], ax                     ; a3 dc 01
    mov ax, 0f000h                            ; b8 00 f0
    mov word [001deh], ax                     ; a3 de 01
    mov ax, 09fc0h                            ; b8 c0 9f
    mov ds, ax                                ; 8e d8
    mov byte [word 00000h], 001h              ; c6 06 00 00 01
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    mov ds, ax                                ; 8e d8
    mov word [0040eh], 09fc0h                 ; c7 06 0e 04 c0 9f
    retn                                      ; c3
biosorg_check_before_or_at_0E82Ch:           ; 0xfe7e5 LB 0x49
    times 0x47 db 0
    db  'XM'
biosorg_check_at_0E82Eh:                     ; 0xfe82e LB 0x52
    sti                                       ; fb
    pushfw                                    ; 9c
    push ES                                   ; 06
    push DS                                   ; 1e
    push ax                                   ; 50
    push cx                                   ; 51
    push dx                                   ; 52
    push bx                                   ; 53
    push sp                                   ; 54
    push bp                                   ; 55
    push si                                   ; 56
    push di                                   ; 57
    cmp ah, 000h                              ; 80 fc 00
    je short 0e858h                           ; 74 19
    cmp ah, 010h                              ; 80 fc 10
    je short 0e858h                           ; 74 14
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 058a5h                               ; e8 5b 70
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    pop bx                                    ; 5b
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop ax                                    ; 58
    pop DS                                    ; 1f
    pop ES                                    ; 07
    add sp, strict byte 00002h                ; 83 c4 02
    iret                                      ; cf
    mov bx, strict word 00040h                ; bb 40 00
    mov ds, bx                                ; 8e db
    cli                                       ; fa
    mov bx, word [word 0001ah]                ; 8b 1e 1a 00
    cmp bx, word [word 0001ch]                ; 3b 1e 1c 00
    jne short 0e86ch                          ; 75 04
    sti                                       ; fb
    nop                                       ; 90
    jmp short 0e85dh                          ; eb f1
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 058a5h                               ; e8 33 70
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    pop bx                                    ; 5b
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop ax                                    ; 58
    pop DS                                    ; 1f
    pop ES                                    ; 07
    add sp, strict byte 00002h                ; 83 c4 02
    iret                                      ; cf
biosorg_check_before_or_at_0E985h:           ; 0xfe880 LB 0x107
    times 0x105 db 0
    db  'XM'
biosorg_check_at_0E987h:                     ; 0xfe987 LB 0x6e
    cli                                       ; fa
    push ax                                   ; 50
    mov AL, strict byte 0adh                  ; b0 ad
    out strict byte 064h, AL                  ; e6 64
    in AL, strict byte 060h                   ; e4 60
    push DS                                   ; 1e
    push ax                                   ; 50
    push cx                                   ; 51
    push dx                                   ; 52
    push bx                                   ; 53
    push sp                                   ; 54
    push bp                                   ; 55
    push si                                   ; 56
    push di                                   ; 57
    cld                                       ; fc
    mov AH, strict byte 04fh                  ; b4 4f
    stc                                       ; f9
    int 015h                                  ; cd 15
    jnc short 0e9c7h                          ; 73 27
    sti                                       ; fb
    cmp AL, strict byte 0e0h                  ; 3c e0
    jne short 0e9b0h                          ; 75 0b
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    mov ds, ax                                ; 8e d8
    or byte [00496h], 002h                    ; 80 0e 96 04 02
    jmp short 0e9c7h                          ; eb 17
    cmp AL, strict byte 0e1h                  ; 3c e1
    jne short 0e9bfh                          ; 75 0b
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    mov ds, ax                                ; 8e d8
    or byte [00496h], 001h                    ; 80 0e 96 04 01
    jmp short 0e9c7h                          ; eb 08
    push ES                                   ; 06
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 05344h                               ; e8 7e 69
    pop ES                                    ; 07
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    pop bx                                    ; 5b
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop ax                                    ; 58
    pop DS                                    ; 1f
    cli                                       ; fa
    call 0e034h                               ; e8 60 f6
    mov AL, strict byte 0aeh                  ; b0 ae
    out strict byte 064h, AL                  ; e6 64
    pop ax                                    ; 58
    iret                                      ; cf
    push ax                                   ; 50
    push cx                                   ; 51
    push dx                                   ; 52
    push bx                                   ; 53
    push sp                                   ; 54
    push bp                                   ; 55
    push si                                   ; 56
    push di                                   ; 57
    push ES                                   ; 06
    push DS                                   ; 1e
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 06f37h                               ; e8 4d 85
    pop DS                                    ; 1f
    pop ES                                    ; 07
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    pop bx                                    ; 5b
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop ax                                    ; 58
    iret                                      ; cf
biosorg_check_before_or_at_0EC57h:           ; 0xfe9f5 LB 0x264
    times 0x262 db 0
    db  'XM'
biosorg_check_at_0EC59h:                     ; 0xfec59 LB 0x2
    jmp short 0ecc4h                          ; eb 69
int13_relocated:                             ; 0xfec5b LB 0x1f
    cmp ah, 04ah                              ; 80 fc 4a
    jc short 0ec7ch                           ; 72 1c
    cmp ah, 04dh                              ; 80 fc 4d
    jnbe short 0ec7ch                         ; 77 17
    push ax                                   ; 50
    push cx                                   ; 51
    push dx                                   ; 52
    push bx                                   ; 53
    push sp                                   ; 54
    push bp                                   ; 55
    push si                                   ; 56
    push di                                   ; 57
    push ES                                   ; 06
    push DS                                   ; 1e
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    push word [cs:0ec7ah]                     ; 2e ff 36 7a ec
    jmp near 03c99h                           ; e9 1f 50
jmp_call_ret_int13_out:                      ; 0xfec7a LB 0x4a
    db  0ffh
    in AL, DX                                 ; ec
    push ES                                   ; 06
    push ax                                   ; 50
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    call 03c6dh                               ; e8 e9 4f
    cmp AL, strict byte 000h                  ; 3c 00
    je short 0ecbfh                           ; 74 37
    call 03c83h                               ; e8 f8 4f
    pop dx                                    ; 5a
    push dx                                   ; 52
    db  03ah, 0c2h
    ; cmp al, dl                                ; 3a c2
    jne short 0ecabh                          ; 75 1a
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop ax                                    ; 58
    pop ES                                    ; 07
    push ax                                   ; 50
    push cx                                   ; 51
    push dx                                   ; 52
    push bx                                   ; 53
    push sp                                   ; 54
    push bp                                   ; 55
    push si                                   ; 56
    push di                                   ; 57
    push ES                                   ; 06
    push DS                                   ; 1e
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    push word [cs:0ec7ah]                     ; 2e ff 36 7a ec
    jmp near 042a7h                           ; e9 fc 55
    and dl, 0e0h                              ; 80 e2 e0
    db  03ah, 0c2h
    ; cmp al, dl                                ; 3a c2
    jne short 0ecbfh                          ; 75 0d
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop ax                                    ; 58
    pop ES                                    ; 07
    push ax                                   ; 50
    push cx                                   ; 51
    push dx                                   ; 52
    push bx                                   ; 53
    db  0feh, 0cah
    ; dec dl                                    ; fe ca
    jmp short 0ecc8h                          ; eb 09
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop ax                                    ; 58
    pop ES                                    ; 07
int13_noeltorito:                            ; 0xfecc4 LB 0x4
    push ax                                   ; 50
    push cx                                   ; 51
    push dx                                   ; 52
    push bx                                   ; 53
int13_legacy:                                ; 0xfecc8 LB 0x16
    push dx                                   ; 52
    push bp                                   ; 55
    push si                                   ; 56
    push di                                   ; 57
    push ES                                   ; 06
    push DS                                   ; 1e
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    test dl, 080h                             ; f6 c2 80
    jne short 0ecdeh                          ; 75 08
    push word [cs:0ec7ah]                     ; 2e ff 36 7a ec
    jmp near 0321fh                           ; e9 41 45
int13_notfloppy:                             ; 0xfecde LB 0x14
    cmp dl, 0e0h                              ; 80 fa e0
    jc short 0ecf2h                           ; 72 0f
    shr ebx, 010h                             ; 66 c1 eb 10
    push bx                                   ; 53
    call 046fah                               ; e8 0f 5a
    pop bx                                    ; 5b
    sal ebx, 010h                             ; 66 c1 e3 10
    jmp short 0ecffh                          ; eb 0d
int13_disk:                                  ; 0xfecf2 LB 0xd
    cmp ah, 040h                              ; 80 fc 40
    jnbe short 0ecfch                         ; 77 05
    call 05c8ah                               ; e8 90 6f
    jmp short 0ecffh                          ; eb 03
    call 060d4h                               ; e8 d5 73
int13_out:                                   ; 0xfecff LB 0xb
    pop DS                                    ; 1f
    pop ES                                    ; 07
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    pop bx                                    ; 5b
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop ax                                    ; 58
    iret                                      ; cf
detect_parport:                              ; 0xfed0a LB 0x1e
    push dx                                   ; 52
    inc dx                                    ; 42
    inc dx                                    ; 42
    in AL, DX                                 ; ec
    and AL, strict byte 0dfh                  ; 24 df
    out DX, AL                                ; ee
    pop dx                                    ; 5a
    mov AL, strict byte 0aah                  ; b0 aa
    out DX, AL                                ; ee
    in AL, DX                                 ; ec
    cmp AL, strict byte 0aah                  ; 3c aa
    jne short 0ed27h                          ; 75 0d
    push bx                                   ; 53
    sal bx, 1                                 ; d1 e3
    mov word [bx+00408h], dx                  ; 89 97 08 04
    pop bx                                    ; 5b
    mov byte [bx+00478h], cl                  ; 88 8f 78 04
    inc bx                                    ; 43
    retn                                      ; c3
detect_serial:                               ; 0xfed28 LB 0x24
    push dx                                   ; 52
    inc dx                                    ; 42
    mov AL, strict byte 002h                  ; b0 02
    out DX, AL                                ; ee
    in AL, DX                                 ; ec
    cmp AL, strict byte 002h                  ; 3c 02
    jne short 0ed4ah                          ; 75 18
    inc dx                                    ; 42
    in AL, DX                                 ; ec
    cmp AL, strict byte 002h                  ; 3c 02
    jne short 0ed4ah                          ; 75 12
    dec dx                                    ; 4a
    db  032h, 0c0h
    ; xor al, al                                ; 32 c0
    pop dx                                    ; 5a
    push bx                                   ; 53
    sal bx, 1                                 ; d1 e3
    mov word [bx+00400h], dx                  ; 89 97 00 04
    pop bx                                    ; 5b
    mov byte [bx+0047ch], cl                  ; 88 8f 7c 04
    inc bx                                    ; 43
    retn                                      ; c3
    pop dx                                    ; 5a
    retn                                      ; c3
floppy_post:                                 ; 0xfed4c LB 0x8c
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    mov ds, ax                                ; 8e d8
    mov AL, strict byte 000h                  ; b0 00
    mov byte [0043eh], AL                     ; a2 3e 04
    mov byte [0043fh], AL                     ; a2 3f 04
    mov byte [00440h], AL                     ; a2 40 04
    mov byte [00441h], AL                     ; a2 41 04
    mov byte [00442h], AL                     ; a2 42 04
    mov byte [00443h], AL                     ; a2 43 04
    mov byte [00444h], AL                     ; a2 44 04
    mov byte [00445h], AL                     ; a2 45 04
    mov byte [00446h], AL                     ; a2 46 04
    mov byte [00447h], AL                     ; a2 47 04
    mov byte [00448h], AL                     ; a2 48 04
    mov byte [0048bh], AL                     ; a2 8b 04
    mov AL, strict byte 010h                  ; b0 10
    out strict byte 070h, AL                  ; e6 70
    in AL, strict byte 071h                   ; e4 71
    db  08ah, 0e0h
    ; mov ah, al                                ; 8a e0
    shr al, 1                                 ; d0 e8
    shr al, 1                                 ; d0 e8
    shr al, 1                                 ; d0 e8
    shr al, 1                                 ; d0 e8
    je short 0ed8ch                           ; 74 04
    mov BL, strict byte 007h                  ; b3 07
    jmp short 0ed8eh                          ; eb 02
    mov BL, strict byte 000h                  ; b3 00
    db  08ah, 0c4h
    ; mov al, ah                                ; 8a c4
    and AL, strict byte 00fh                  ; 24 0f
    je short 0ed97h                           ; 74 03
    or bl, 070h                               ; 80 cb 70
    mov byte [0048fh], bl                     ; 88 1e 8f 04
    mov AL, strict byte 000h                  ; b0 00
    mov byte [00490h], AL                     ; a2 90 04
    mov byte [00491h], AL                     ; a2 91 04
    mov byte [00492h], AL                     ; a2 92 04
    mov byte [00493h], AL                     ; a2 93 04
    mov byte [00494h], AL                     ; a2 94 04
    mov byte [00495h], AL                     ; a2 95 04
    mov AL, strict byte 002h                  ; b0 02
    out strict byte 00ah, AL                  ; e6 0a
    mov ax, 0efc7h                            ; b8 c7 ef
    mov word [00078h], ax                     ; a3 78 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [0007ah], ax                     ; a3 7a 00
    mov ax, 0ec59h                            ; b8 59 ec
    mov word [00100h], ax                     ; a3 00 01
    mov ax, 0f000h                            ; b8 00 f0
    mov word [00102h], ax                     ; a3 02 01
    mov ax, 0ef57h                            ; b8 57 ef
    mov word [00038h], ax                     ; a3 38 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [0003ah], ax                     ; a3 3a 00
    retn                                      ; c3
bcd_to_bin:                                  ; 0xfedd8 LB 0xb
    push cx                                   ; 51
    mov CL, strict byte 004h                  ; b1 04
    sal ax, CL                                ; d3 e0
    shr al, CL                                ; d2 e8
    pop cx                                    ; 59
    aad 00ah                                  ; d5 0a
    retn                                      ; c3
rtc_post:                                    ; 0xfede3 LB 0x5a
    mov AL, strict byte 000h                  ; b0 00
    out strict byte 070h, AL                  ; e6 70
    in AL, strict byte 071h                   ; e4 71
    call 0edd8h                               ; e8 ec ff
    test al, al                               ; 84 c0
    db  032h, 0e4h
    ; xor ah, ah                                ; 32 e4
    mov dx, 01234h                            ; ba 34 12
    mul dx                                    ; f7 e2
    db  08bh, 0c8h
    ; mov cx, ax                                ; 8b c8
    mov AL, strict byte 002h                  ; b0 02
    out strict byte 070h, AL                  ; e6 70
    in AL, strict byte 071h                   ; e4 71
    call 0edd8h                               ; e8 d8 ff
    test al, al                               ; 84 c0
    je short 0ee0fh                           ; 74 0b
    add cx, 04463h                            ; 81 c1 63 44
    adc dx, strict byte 00004h                ; 83 d2 04
    db  0feh, 0c8h
    ; dec al                                    ; fe c8
    jne short 0ee04h                          ; 75 f5
    mov AL, strict byte 004h                  ; b0 04
    out strict byte 070h, AL                  ; e6 70
    in AL, strict byte 071h                   ; e4 71
    call 0edd8h                               ; e8 c0 ff
    test al, al                               ; 84 c0
    je short 0ee28h                           ; 74 0c
    add cx, 0076ch                            ; 81 c1 6c 07
    adc dx, 00100h                            ; 81 d2 00 01
    db  0feh, 0c8h
    ; dec al                                    ; fe c8
    jne short 0ee1ch                          ; 75 f4
    db  08ah, 0cdh
    ; mov cl, ch                                ; 8a cd
    db  08ah, 0eah
    ; mov ch, dl                                ; 8a ea
    db  08ah, 0d6h
    ; mov dl, dh                                ; 8a d6
    db  032h, 0f6h
    ; xor dh, dh                                ; 32 f6
    mov word [0046ch], cx                     ; 89 0e 6c 04
    mov word [0046eh], dx                     ; 89 16 6e 04
    mov byte [00470h], dh                     ; 88 36 70 04
    retn                                      ; c3
biosorg_check_before_or_at_0EF55h:           ; 0xfee3d LB 0x11a
    times 0x118 db 0
    db  'XM'
int0e_handler:                               ; 0xfef57 LB 0x3b
    push ax                                   ; 50
    push dx                                   ; 52
    mov dx, 003f4h                            ; ba f4 03
    in AL, DX                                 ; ec
    and AL, strict byte 0c0h                  ; 24 c0
    cmp AL, strict byte 0c0h                  ; 3c c0
    je short 0ef81h                           ; 74 1e
    mov dx, 003f5h                            ; ba f5 03
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov dx, 003f4h                            ; ba f4 03
    in AL, DX                                 ; ec
    and AL, strict byte 0c0h                  ; 24 c0
    cmp AL, strict byte 0c0h                  ; 3c c0
    jne short 0ef69h                          ; 75 f6
    mov dx, 003f5h                            ; ba f5 03
    in AL, DX                                 ; ec
    mov dx, 003f4h                            ; ba f4 03
    in AL, DX                                 ; ec
    and AL, strict byte 0c0h                  ; 24 c0
    cmp AL, strict byte 0c0h                  ; 3c c0
    je short 0ef73h                           ; 74 f2
    push DS                                   ; 1e
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    mov ds, ax                                ; 8e d8
    call 0e034h                               ; e8 ab f0
    or byte [0043eh], 080h                    ; 80 0e 3e 04 80
    pop DS                                    ; 1f
    pop dx                                    ; 5a
    pop ax                                    ; 58
    iret                                      ; cf
biosorg_check_before_or_at_0EFC5h:           ; 0xfef92 LB 0x35
    times 0x33 db 0
    db  'XM'
_diskette_param_table:                       ; 0xfefc7 LB 0xb
    scasw                                     ; af
    add ah, byte [di]                         ; 02 25
    add dl, byte [bp+si]                      ; 02 12
    db  01bh, 0ffh
    ; sbb di, di                                ; 1b ff
    insb                                      ; 6c
    db  0f6h
    invd                                      ; 0f 08
biosorg_check_at_0EFD2h:                     ; 0xfefd2 LB 0x2
    jmp short 0efd4h                          ; eb 00
int17_handler:                               ; 0xfefd4 LB 0x1b
    push DS                                   ; 1e
    push ES                                   ; 06
    push ax                                   ; 50
    push cx                                   ; 51
    push dx                                   ; 52
    push bx                                   ; 53
    push sp                                   ; 54
    push bp                                   ; 55
    push si                                   ; 56
    push di                                   ; 57
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 078e4h                               ; e8 00 89
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    pop bx                                    ; 5b
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop ax                                    ; 58
    pop ES                                    ; 07
    pop DS                                    ; 1f
    iret                                      ; cf
_pmode_IDT:                                  ; 0xfefef LB 0x6
    db  000h, 000h, 000h, 000h, 00fh, 000h
_rmode_IDT:                                  ; 0xfeff5 LB 0x6
    db  0ffh, 003h, 000h, 000h, 000h, 000h
int1c_handler:                               ; 0xfeffb LB 0x1
    iret                                      ; cf
biosorg_check_before_or_at_0F043h:           ; 0xfeffc LB 0x49
    times 0x47 db 0
    db  'XM'
biosorg_check_at_0F045h:                     ; 0xff045 LB 0x1
    iret                                      ; cf
biosorg_check_before_or_at_0F063h:           ; 0xff046 LB 0x1f
    times 0x1d db 0
    db  'XM'
int10_handler:                               ; 0xff065 LB 0x1
    iret                                      ; cf
biosorg_check_before_or_at_0F0A2h:           ; 0xff066 LB 0x3e
    times 0x3c db 0
    db  'XM'
biosorg_check_at_0F0A4h:                     ; 0xff0a4 LB 0x8
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 01760h                               ; e8 b6 26
    hlt                                       ; f4
    iret                                      ; cf
int19_relocated:                             ; 0xff0ac LB 0x95
    db  08bh, 0ech
    ; mov bp, sp                                ; 8b ec
    mov ax, word [bp+002h]                    ; 8b 46 02
    cmp ax, 0f000h                            ; 3d 00 f0
    je short 0f0c3h                           ; 74 0d
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    mov ds, ax                                ; 8e d8
    mov ax, 01234h                            ; b8 34 12
    mov word [001d8h], ax                     ; a3 d8 01
    jmp near 0e05bh                           ; e9 98 ef
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    push bp                                   ; 55
    db  08bh, 0ech
    ; mov bp, sp                                ; 8b ec
    mov ax, strict word 00001h                ; b8 01 00
    push ax                                   ; 50
    call 04d99h                               ; e8 c9 5c
    inc sp                                    ; 44
    inc sp                                    ; 44
    test ax, ax                               ; 85 c0
    jne short 0f0fdh                          ; 75 27
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 04d99h                               ; e8 bc 5c
    inc sp                                    ; 44
    inc sp                                    ; 44
    test ax, ax                               ; 85 c0
    jne short 0f0fdh                          ; 75 1a
    mov ax, strict word 00003h                ; b8 03 00
    push ax                                   ; 50
    call 04d99h                               ; e8 af 5c
    inc sp                                    ; 44
    inc sp                                    ; 44
    test ax, ax                               ; 85 c0
    jne short 0f0fdh                          ; 75 0d
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 04d99h                               ; e8 a2 5c
    inc sp                                    ; 44
    inc sp                                    ; 44
    test ax, ax                               ; 85 c0
    je short 0f0a4h                           ; 74 a7
    mov word [byte bp+000h], ax               ; 89 46 00
    sal ax, 1                                 ; d1 e0
    sal ax, 1                                 ; d1 e0
    sal ax, 1                                 ; d1 e0
    sal ax, 1                                 ; d1 e0
    mov word [bp+002h], ax                    ; 89 46 02
    mov ax, word [byte bp+000h]               ; 8b 46 00
    and ax, 0f000h                            ; 25 00 f0
    mov word [bp+004h], ax                    ; 89 46 04
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    mov ds, ax                                ; 8e d8
    mov es, ax                                ; 8e c0
    mov word [byte bp+000h], ax               ; 89 46 00
    mov ax, 0aa55h                            ; b8 55 aa
    pop bp                                    ; 5d
    iret                                      ; cf
    or cx, word [bp+si]                       ; 0b 0a
    or word [bp+di], cx                       ; 09 0b
    push eax                                  ; 66 50
    mov eax, strict dword 000800000h          ; 66 b8 00 00 80 00
    db  08bh, 0c3h
    ; mov ax, bx                                ; 8b c3
    sal eax, 008h                             ; 66 c1 e0 08
    and dl, 0fch                              ; 80 e2 fc
    db  00ah, 0c2h
    ; or al, dl                                 ; 0a c2
    mov dx, 00cf8h                            ; ba f8 0c
    out DX, eax                               ; 66 ef
    pop eax                                   ; 66 58
    retn                                      ; c3
pcibios_init_iomem_bases:                    ; 0xff141 LB 0x12
    push bp                                   ; 55
    db  08bh, 0ech
    ; mov bp, sp                                ; 8b ec
    mov eax, strict dword 00124f9fdh          ; 66 b8 fd f9 24 01
    mov dx, 00410h                            ; ba 10 04
    out DX, eax                               ; 66 ef
    db  08bh, 0e5h
    ; mov sp, bp                                ; 8b e5
    pop bp                                    ; 5d
    retn                                      ; c3
pcibios_init_set_elcr:                       ; 0xff153 LB 0xc
    push ax                                   ; 50
    push cx                                   ; 51
    mov dx, 004d0h                            ; ba d0 04
    test AL, strict byte 008h                 ; a8 08
    je short 0f15fh                           ; 74 03
    inc dx                                    ; 42
    and AL, strict byte 007h                  ; 24 07
is_master_pic:                               ; 0xff15f LB 0xd
    db  08ah, 0c8h
    ; mov cl, al                                ; 8a c8
    mov BL, strict byte 001h                  ; b3 01
    sal bl, CL                                ; d2 e3
    in AL, DX                                 ; ec
    db  00ah, 0c3h
    ; or al, bl                                 ; 0a c3
    out DX, AL                                ; ee
    pop cx                                    ; 59
    pop ax                                    ; 58
    retn                                      ; c3
pcibios_init_irqs:                           ; 0xff16c LB 0x34
    push DS                                   ; 1e
    push bp                                   ; 55
    mov ax, 0f000h                            ; b8 00 f0
    mov ds, ax                                ; 8e d8
    pop bp                                    ; 5d
    pop DS                                    ; 1f
    retn                                      ; c3
    mov ax, ax                                ; 89 c0
    mov ax, ax                                ; 89 c0
    mov ax, ax                                ; 89 c0
    mov ax, ax                                ; 89 c0
    mov ax, ax                                ; 89 c0
    and AL, strict byte 050h                  ; 24 50
    dec cx                                    ; 49
    push dx                                   ; 52
    add byte [bx+di], al                      ; 00 01
    add byte [bp+si], al                      ; 00 02
    add byte [bx+si], cl                      ; 00 08
    add byte [bx+si], al                      ; 00 00
    xchg byte [bx+si+07000h], al              ; 86 80 00 70
    times 0xf db 0
    db  031h
_pci_routing_table:                          ; 0xff1a0 LB 0x1e0
    db  000h, 008h, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 000h, 000h
    db  000h, 010h, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 001h, 000h
    db  000h, 018h, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 002h, 000h
    db  000h, 020h, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 003h, 000h
    db  000h, 028h, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 004h, 000h
    db  000h, 030h, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 005h, 000h
    db  000h, 038h, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 006h, 000h
    db  000h, 040h, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 007h, 000h
    db  000h, 048h, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 008h, 000h
    db  000h, 050h, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 009h, 000h
    db  000h, 058h, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 00ah, 000h
    db  000h, 060h, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 00bh, 000h
    db  000h, 068h, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 00ch, 000h
    db  000h, 070h, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 00dh, 000h
    db  000h, 078h, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 00eh, 000h
    db  000h, 080h, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 00fh, 000h
    db  000h, 088h, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 010h, 000h
    db  000h, 090h, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 011h, 000h
    db  000h, 098h, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 012h, 000h
    db  000h, 0a0h, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 013h, 000h
    db  000h, 0a8h, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 014h, 000h
    db  000h, 0b0h, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 015h, 000h
    db  000h, 0b8h, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 016h, 000h
    db  000h, 0c0h, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 017h, 000h
    db  000h, 0c8h, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 018h, 000h
    db  000h, 0d0h, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 019h, 000h
    db  000h, 0d8h, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 01ah, 000h
    db  000h, 0e0h, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 01bh, 000h
    db  000h, 0e8h, 060h, 0f8h, 0deh, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 01ch, 000h
    db  000h, 0f0h, 061h, 0f8h, 0deh, 062h, 0f8h, 0deh, 063h, 0f8h, 0deh, 060h, 0f8h, 0deh, 01dh, 000h
_pci_routing_table_size:                     ; 0xff380 LB 0x2
    loopne 0f383h                             ; e0 01
biosorg_check_before_or_at_0F83Fh:           ; 0xff382 LB 0x4bf
    times 0x4bd db 0
    db  'XM'
int12_handler:                               ; 0xff841 LB 0xc
    sti                                       ; fb
    push DS                                   ; 1e
    mov ax, strict word 00040h                ; b8 40 00
    mov ds, ax                                ; 8e d8
    mov ax, word [00013h]                     ; a1 13 00
    pop DS                                    ; 1f
    iret                                      ; cf
int11_handler:                               ; 0xff84d LB 0xc
    sti                                       ; fb
    push DS                                   ; 1e
    mov ax, strict word 00040h                ; b8 40 00
    mov ds, ax                                ; 8e d8
    mov ax, word [00010h]                     ; a1 10 00
    pop DS                                    ; 1f
    iret                                      ; cf
int15_handler:                               ; 0xff859 LB 0x3c
    pushfw                                    ; 9c
    push DS                                   ; 1e
    push ES                                   ; 06
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    cmp ah, 086h                              ; 80 fc 86
    je short 0f89ah                           ; 74 36
    cmp ah, 0e8h                              ; 80 fc e8
    je short 0f89ah                           ; 74 31
    cmp ah, 0d0h                              ; 80 fc d0
    je short 0f89ah                           ; 74 2c
    push ax                                   ; 50
    push cx                                   ; 51
    push dx                                   ; 52
    push bx                                   ; 53
    push sp                                   ; 54
    push bp                                   ; 55
    push si                                   ; 56
    push di                                   ; 57
    cmp ah, 053h                              ; 80 fc 53
    je short 0f890h                           ; 74 15
    cmp ah, 0c2h                              ; 80 fc c2
    je short 0f895h                           ; 74 15
    call 06856h                               ; e8 d3 6f
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    pop bx                                    ; 5b
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop ax                                    ; 58
    pop ES                                    ; 07
    pop DS                                    ; 1f
    popfw                                     ; 9d
    jmp short 0f8afh                          ; eb 1f
    call 09bb6h                               ; e8 23 a3
    jmp short 0f883h                          ; eb ee
int15_handler_mouse:                         ; 0xff895 LB 0x5
    call 07544h                               ; e8 ac 7c
    jmp short 0f883h                          ; eb e9
int15_handler32:                             ; 0xff89a LB 0x15
    push ax                                   ; 50
    push cx                                   ; 51
    push dx                                   ; 52
    push bx                                   ; 53
    push sp                                   ; 54
    push bp                                   ; 55
    push si                                   ; 56
    push di                                   ; 57
    call 06b99h                               ; e8 f4 72
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    pop bx                                    ; 5b
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop ax                                    ; 58
    jmp short 0f88bh                          ; eb dc
iret_modify_cf:                              ; 0xff8af LB 0x1a
    jc short 0f8bfh                           ; 72 0e
    push bp                                   ; 55
    db  08bh, 0ech
    ; mov bp, sp                                ; 8b ec
    and byte [bp+006h], 0feh                  ; 80 66 06 fe
    or word [bp+006h], 00200h                 ; 81 4e 06 00 02
    pop bp                                    ; 5d
    iret                                      ; cf
    push bp                                   ; 55
    db  08bh, 0ech
    ; mov bp, sp                                ; 8b ec
    or word [bp+006h], 00201h                 ; 81 4e 06 01 02
    pop bp                                    ; 5d
    iret                                      ; cf
int74_handler:                               ; 0xff8c9 LB 0x3a
    sti                                       ; fb
    push ax                                   ; 50
    push cx                                   ; 51
    push dx                                   ; 52
    push bx                                   ; 53
    push sp                                   ; 54
    push bp                                   ; 55
    push si                                   ; 56
    push di                                   ; 57
    push ES                                   ; 06
    push DS                                   ; 1e
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    push ax                                   ; 50
    push ax                                   ; 50
    push ax                                   ; 50
    push ax                                   ; 50
    push ax                                   ; 50
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 07472h                               ; e8 91 7b
    pop cx                                    ; 59
    jcxz 0f8f1h                               ; e3 0d
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    push ax                                   ; 50
    pop DS                                    ; 1f
    push word [0040eh]                        ; ff 36 0e 04
    pop DS                                    ; 1f
    call far [word 00022h]                    ; ff 1e 22 00
    cli                                       ; fa
    call 0e030h                               ; e8 3b e7
    add sp, strict byte 00008h                ; 83 c4 08
    pop DS                                    ; 1f
    pop ES                                    ; 07
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    pop bx                                    ; 5b
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop ax                                    ; 58
    iret                                      ; cf
int76_handler:                               ; 0xff903 LB 0x12
    push ax                                   ; 50
    push DS                                   ; 1e
    mov ax, strict word 00040h                ; b8 40 00
    mov ds, ax                                ; 8e d8
    mov byte [0008eh], 0ffh                   ; c6 06 8e 00 ff
    call 0e030h                               ; e8 1e e7
    pop DS                                    ; 1f
    pop ax                                    ; 58
    iret                                      ; cf
int70_handler:                               ; 0xff915 LB 0x2d
    push ES                                   ; 06
    push DS                                   ; 1e
    push ax                                   ; 50
    push cx                                   ; 51
    push dx                                   ; 52
    push bx                                   ; 53
    push sp                                   ; 54
    push bp                                   ; 55
    push si                                   ; 56
    push di                                   ; 57
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 0710fh                               ; e8 ea 77
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    pop bx                                    ; 5b
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop ax                                    ; 58
    pop DS                                    ; 1f
    pop ES                                    ; 07
    iret                                      ; cf
    jnbe short 0f937h                         ; 77 05
    cmp ax, 000b0h                            ; 3d b0 00
    jc short 0f93fh                           ; 72 08
    db  033h, 0d2h
    ; xor dx, dx                                ; 33 d2
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    inc byte [word 00070h]                    ; fe 06 70 00
    jmp near 0fec1h                           ; e9 7f 05
biosorg_check_before_or_at_0FA6Ch:           ; 0xff942 LB 0x12c
    times 0x12a db 0
    db  'XM'
font8x8:                                     ; 0xffa6e LB 0x400
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 07eh, 081h, 0a5h, 081h, 0bdh, 099h, 081h, 07eh
    db  07eh, 0ffh, 0dbh, 0ffh, 0c3h, 0e7h, 0ffh, 07eh, 06ch, 0feh, 0feh, 0feh, 07ch, 038h, 010h, 000h
    db  010h, 038h, 07ch, 0feh, 07ch, 038h, 010h, 000h, 038h, 07ch, 038h, 0feh, 0feh, 07ch, 038h, 07ch
    db  010h, 010h, 038h, 07ch, 0feh, 07ch, 038h, 07ch, 000h, 000h, 018h, 03ch, 03ch, 018h, 000h, 000h
    db  0ffh, 0ffh, 0e7h, 0c3h, 0c3h, 0e7h, 0ffh, 0ffh, 000h, 03ch, 066h, 042h, 042h, 066h, 03ch, 000h
    db  0ffh, 0c3h, 099h, 0bdh, 0bdh, 099h, 0c3h, 0ffh, 00fh, 007h, 00fh, 07dh, 0cch, 0cch, 0cch, 078h
    db  03ch, 066h, 066h, 066h, 03ch, 018h, 07eh, 018h, 03fh, 033h, 03fh, 030h, 030h, 070h, 0f0h, 0e0h
    db  07fh, 063h, 07fh, 063h, 063h, 067h, 0e6h, 0c0h, 099h, 05ah, 03ch, 0e7h, 0e7h, 03ch, 05ah, 099h
    db  080h, 0e0h, 0f8h, 0feh, 0f8h, 0e0h, 080h, 000h, 002h, 00eh, 03eh, 0feh, 03eh, 00eh, 002h, 000h
    db  018h, 03ch, 07eh, 018h, 018h, 07eh, 03ch, 018h, 066h, 066h, 066h, 066h, 066h, 000h, 066h, 000h
    db  07fh, 0dbh, 0dbh, 07bh, 01bh, 01bh, 01bh, 000h, 03eh, 063h, 038h, 06ch, 06ch, 038h, 0cch, 078h
    db  000h, 000h, 000h, 000h, 07eh, 07eh, 07eh, 000h, 018h, 03ch, 07eh, 018h, 07eh, 03ch, 018h, 0ffh
    db  018h, 03ch, 07eh, 018h, 018h, 018h, 018h, 000h, 018h, 018h, 018h, 018h, 07eh, 03ch, 018h, 000h
    db  000h, 018h, 00ch, 0feh, 00ch, 018h, 000h, 000h, 000h, 030h, 060h, 0feh, 060h, 030h, 000h, 000h
    db  000h, 000h, 0c0h, 0c0h, 0c0h, 0feh, 000h, 000h, 000h, 024h, 066h, 0ffh, 066h, 024h, 000h, 000h
    db  000h, 018h, 03ch, 07eh, 0ffh, 0ffh, 000h, 000h, 000h, 0ffh, 0ffh, 07eh, 03ch, 018h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 030h, 078h, 078h, 030h, 030h, 000h, 030h, 000h
    db  06ch, 06ch, 06ch, 000h, 000h, 000h, 000h, 000h, 06ch, 06ch, 0feh, 06ch, 0feh, 06ch, 06ch, 000h
    db  030h, 07ch, 0c0h, 078h, 00ch, 0f8h, 030h, 000h, 000h, 0c6h, 0cch, 018h, 030h, 066h, 0c6h, 000h
    db  038h, 06ch, 038h, 076h, 0dch, 0cch, 076h, 000h, 060h, 060h, 0c0h, 000h, 000h, 000h, 000h, 000h
    db  018h, 030h, 060h, 060h, 060h, 030h, 018h, 000h, 060h, 030h, 018h, 018h, 018h, 030h, 060h, 000h
    db  000h, 066h, 03ch, 0ffh, 03ch, 066h, 000h, 000h, 000h, 030h, 030h, 0fch, 030h, 030h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 030h, 030h, 060h, 000h, 000h, 000h, 0fch, 000h, 000h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 030h, 030h, 000h, 006h, 00ch, 018h, 030h, 060h, 0c0h, 080h, 000h
    db  07ch, 0c6h, 0ceh, 0deh, 0f6h, 0e6h, 07ch, 000h, 030h, 070h, 030h, 030h, 030h, 030h, 0fch, 000h
    db  078h, 0cch, 00ch, 038h, 060h, 0cch, 0fch, 000h, 078h, 0cch, 00ch, 038h, 00ch, 0cch, 078h, 000h
    db  01ch, 03ch, 06ch, 0cch, 0feh, 00ch, 01eh, 000h, 0fch, 0c0h, 0f8h, 00ch, 00ch, 0cch, 078h, 000h
    db  038h, 060h, 0c0h, 0f8h, 0cch, 0cch, 078h, 000h, 0fch, 0cch, 00ch, 018h, 030h, 030h, 030h, 000h
    db  078h, 0cch, 0cch, 078h, 0cch, 0cch, 078h, 000h, 078h, 0cch, 0cch, 07ch, 00ch, 018h, 070h, 000h
    db  000h, 030h, 030h, 000h, 000h, 030h, 030h, 000h, 000h, 030h, 030h, 000h, 000h, 030h, 030h, 060h
    db  018h, 030h, 060h, 0c0h, 060h, 030h, 018h, 000h, 000h, 000h, 0fch, 000h, 000h, 0fch, 000h, 000h
    db  060h, 030h, 018h, 00ch, 018h, 030h, 060h, 000h, 078h, 0cch, 00ch, 018h, 030h, 000h, 030h, 000h
    db  07ch, 0c6h, 0deh, 0deh, 0deh, 0c0h, 078h, 000h, 030h, 078h, 0cch, 0cch, 0fch, 0cch, 0cch, 000h
    db  0fch, 066h, 066h, 07ch, 066h, 066h, 0fch, 000h, 03ch, 066h, 0c0h, 0c0h, 0c0h, 066h, 03ch, 000h
    db  0f8h, 06ch, 066h, 066h, 066h, 06ch, 0f8h, 000h, 0feh, 062h, 068h, 078h, 068h, 062h, 0feh, 000h
    db  0feh, 062h, 068h, 078h, 068h, 060h, 0f0h, 000h, 03ch, 066h, 0c0h, 0c0h, 0ceh, 066h, 03eh, 000h
    db  0cch, 0cch, 0cch, 0fch, 0cch, 0cch, 0cch, 000h, 078h, 030h, 030h, 030h, 030h, 030h, 078h, 000h
    db  01eh, 00ch, 00ch, 00ch, 0cch, 0cch, 078h, 000h, 0e6h, 066h, 06ch, 078h, 06ch, 066h, 0e6h, 000h
    db  0f0h, 060h, 060h, 060h, 062h, 066h, 0feh, 000h, 0c6h, 0eeh, 0feh, 0feh, 0d6h, 0c6h, 0c6h, 000h
    db  0c6h, 0e6h, 0f6h, 0deh, 0ceh, 0c6h, 0c6h, 000h, 038h, 06ch, 0c6h, 0c6h, 0c6h, 06ch, 038h, 000h
    db  0fch, 066h, 066h, 07ch, 060h, 060h, 0f0h, 000h, 078h, 0cch, 0cch, 0cch, 0dch, 078h, 01ch, 000h
    db  0fch, 066h, 066h, 07ch, 06ch, 066h, 0e6h, 000h, 078h, 0cch, 0e0h, 070h, 01ch, 0cch, 078h, 000h
    db  0fch, 0b4h, 030h, 030h, 030h, 030h, 078h, 000h, 0cch, 0cch, 0cch, 0cch, 0cch, 0cch, 0fch, 000h
    db  0cch, 0cch, 0cch, 0cch, 0cch, 078h, 030h, 000h, 0c6h, 0c6h, 0c6h, 0d6h, 0feh, 0eeh, 0c6h, 000h
    db  0c6h, 0c6h, 06ch, 038h, 038h, 06ch, 0c6h, 000h, 0cch, 0cch, 0cch, 078h, 030h, 030h, 078h, 000h
    db  0feh, 0c6h, 08ch, 018h, 032h, 066h, 0feh, 000h, 078h, 060h, 060h, 060h, 060h, 060h, 078h, 000h
    db  0c0h, 060h, 030h, 018h, 00ch, 006h, 002h, 000h, 078h, 018h, 018h, 018h, 018h, 018h, 078h, 000h
    db  010h, 038h, 06ch, 0c6h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 0ffh
    db  030h, 030h, 018h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 078h, 00ch, 07ch, 0cch, 076h, 000h
    db  0e0h, 060h, 060h, 07ch, 066h, 066h, 0dch, 000h, 000h, 000h, 078h, 0cch, 0c0h, 0cch, 078h, 000h
    db  01ch, 00ch, 00ch, 07ch, 0cch, 0cch, 076h, 000h, 000h, 000h, 078h, 0cch, 0fch, 0c0h, 078h, 000h
    db  038h, 06ch, 060h, 0f0h, 060h, 060h, 0f0h, 000h, 000h, 000h, 076h, 0cch, 0cch, 07ch, 00ch, 0f8h
    db  0e0h, 060h, 06ch, 076h, 066h, 066h, 0e6h, 000h, 030h, 000h, 070h, 030h, 030h, 030h, 078h, 000h
    db  00ch, 000h, 00ch, 00ch, 00ch, 0cch, 0cch, 078h, 0e0h, 060h, 066h, 06ch, 078h, 06ch, 0e6h, 000h
    db  070h, 030h, 030h, 030h, 030h, 030h, 078h, 000h, 000h, 000h, 0cch, 0feh, 0feh, 0d6h, 0c6h, 000h
    db  000h, 000h, 0f8h, 0cch, 0cch, 0cch, 0cch, 000h, 000h, 000h, 078h, 0cch, 0cch, 0cch, 078h, 000h
    db  000h, 000h, 0dch, 066h, 066h, 07ch, 060h, 0f0h, 000h, 000h, 076h, 0cch, 0cch, 07ch, 00ch, 01eh
    db  000h, 000h, 0dch, 076h, 066h, 060h, 0f0h, 000h, 000h, 000h, 07ch, 0c0h, 078h, 00ch, 0f8h, 000h
    db  010h, 030h, 07ch, 030h, 030h, 034h, 018h, 000h, 000h, 000h, 0cch, 0cch, 0cch, 0cch, 076h, 000h
    db  000h, 000h, 0cch, 0cch, 0cch, 078h, 030h, 000h, 000h, 000h, 0c6h, 0d6h, 0feh, 0feh, 06ch, 000h
    db  000h, 000h, 0c6h, 06ch, 038h, 06ch, 0c6h, 000h, 000h, 000h, 0cch, 0cch, 0cch, 07ch, 00ch, 0f8h
    db  000h, 000h, 0fch, 098h, 030h, 064h, 0fch, 000h, 01ch, 030h, 030h, 0e0h, 030h, 030h, 01ch, 000h
    db  018h, 018h, 018h, 000h, 018h, 018h, 018h, 000h, 0e0h, 030h, 030h, 01ch, 030h, 030h, 0e0h, 000h
    db  076h, 0dch, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 010h, 038h, 06ch, 0c6h, 0c6h, 0feh, 000h
biosorg_check_at_0FE6Eh:                     ; 0xffe6e LB 0x1b
    push ES                                   ; 06
    push DS                                   ; 1e
    push ax                                   ; 50
    push cx                                   ; 51
    push dx                                   ; 52
    push bx                                   ; 53
    push sp                                   ; 54
    push bp                                   ; 55
    push si                                   ; 56
    push di                                   ; 57
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 071ceh                               ; e8 50 73
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    pop bx                                    ; 5b
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop ax                                    ; 58
    pop DS                                    ; 1f
    pop ES                                    ; 07
    iret                                      ; cf
biosorg_check_before_or_at_0FEA3h:           ; 0xffe89 LB 0x1c
    times 0x1a db 0
    db  'XM'
int08_handler:                               ; 0xffea5 LB 0x42
    sti                                       ; fb
    push ax                                   ; 50
    push DS                                   ; 1e
    push dx                                   ; 52
    mov ax, strict word 00040h                ; b8 40 00
    mov ds, ax                                ; 8e d8
    mov ax, word [0006ch]                     ; a1 6c 00
    mov dx, word [word 0006eh]                ; 8b 16 6e 00
    inc ax                                    ; 40
    jne short 0feb9h                          ; 75 01
    inc dx                                    ; 42
    cmp dx, strict byte 00018h                ; 83 fa 18
    jc short 0fec1h                           ; 72 03
    jmp near 0f930h                           ; e9 6f fa
    mov word [0006ch], ax                     ; a3 6c 00
    mov word [word 0006eh], dx                ; 89 16 6e 00
    mov AL, byte [00040h]                     ; a0 40 00
    db  00ah, 0c0h
    ; or al, al                                 ; 0a c0
    je short 0feddh                           ; 74 0e
    db  0feh, 0c8h
    ; dec al                                    ; fe c8
    mov byte [00040h], AL                     ; a2 40 00
    jne short 0feddh                          ; 75 07
    mov dx, 003f2h                            ; ba f2 03
    in AL, DX                                 ; ec
    and AL, strict byte 0cfh                  ; 24 cf
    out DX, AL                                ; ee
    int 01ch                                  ; cd 1c
    cli                                       ; fa
    call 0e034h                               ; e8 51 e1
    pop dx                                    ; 5a
    pop DS                                    ; 1f
    pop ax                                    ; 58
    iret                                      ; cf
biosorg_check_before_or_at_0FEF1h:           ; 0xffee7 LB 0xc
    times 0xa db 0
    db  'XM'
biosorg_check_at_0FEF3h:                     ; 0xffef3 LB 0xd
    times 0xb db 0
    db  'XM'
biosorg_check_at_0FF00h:                     ; 0xfff00 LB 0x19
    dec di                                    ; 4f
    jc short 0ff64h                           ; 72 61
    arpl [si+065h], bp                        ; 63 6c 65
    and byte [bp+04dh], dl                    ; 20 56 4d
    and byte [bp+069h], dl                    ; 20 56 69
    jc short 0ff82h                           ; 72 74
    jne short 0ff71h                          ; 75 61
    insb                                      ; 6c
    inc dx                                    ; 42
    outsw                                     ; 6f
    js short 0ff35h                           ; 78 20
    inc dx                                    ; 42
    dec cx                                    ; 49
    dec di                                    ; 4f
    push bx                                   ; 53
biosorg_check_before_or_at_0FF51h:           ; 0xfff19 LB 0x3a
    times 0x38 db 0
    db  'XM'
dummy_iret:                                  ; 0xfff53 LB 0x1
    iret                                      ; cf
biosorg_check_at_0FF54h:                     ; 0xfff54 LB 0x2c
    iret                                      ; cf
    mov ax, ax                                ; 89 c0
    mov ax, ax                                ; 89 c0
    mov ax, ax                                ; 89 c0
    mov ax, ax                                ; 89 c0
    mov ax, ax                                ; 89 c0
    cld                                       ; fc
    pop di                                    ; 5f
    push bx                                   ; 53
    dec bp                                    ; 4d
    pop di                                    ; 5f
    jnl short 0ff85h                          ; 7d 1f
    add al, byte [di]                         ; 02 05
    inc word [bx+si]                          ; ff 00
    add byte [bx+si], al                      ; 00 00
    add byte [bx+si], al                      ; 00 00
    add byte [bx+si], al                      ; 00 00
    pop di                                    ; 5f
    inc sp                                    ; 44
    dec bp                                    ; 4d
    dec cx                                    ; 49
    pop di                                    ; 5f
    and ax, strict word 00000h                ; 25 00 00
    add byte [bx+si], dl                      ; 00 10
    push CS                                   ; 0e
    add byte [bx+si], al                      ; 00 00
    add byte [di], ah                         ; 00 25
    times 0x1 db 0
biosorg_check_before_or_at_0FFEEh:           ; 0xfff80 LB 0x70
    times 0x6e db 0
    db  'XM'
cpu_reset:                                   ; 0xffff0 LB 0x10
    jmp far 0f000h:0e05bh                     ; ea 5b e0 00 f0
    db  030h, 036h, 02fh, 032h, 033h, 02fh, 039h, 039h, 000h, 0fbh, 008h
