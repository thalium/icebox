; $Id: VBoxBiosAlternative386.asm $ 
;; @file
; Auto Generated source file. Do not edit.
;

;
; Source file: post.c
;
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
;  $Id: VBoxBiosAlternative386.asm $
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
; Source file: pci32.c
;
;  $Id: VBoxBiosAlternative386.asm $
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
    db  000h, 000h, 000h, 000h, 000h, 000h, 07bh, 02ch, 036h, 082h, 0a5h, 091h
_softrst:                                    ; 0xf0076 LB 0xc
    db  000h, 000h, 000h, 000h, 000h, 000h, 063h, 02fh, 0d4h, 03ch, 0d4h, 03ch
_dskacc:                                     ; 0xf0082 LB 0x2e
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 06bh, 02bh, 020h, 02ch, 000h, 000h, 000h, 000h
    db  080h, 080h, 05bh, 081h, 07bh, 090h, 021h, 091h, 000h, 000h, 000h, 000h, 000h, 000h, 05fh, 033h
    db  032h, 05fh, 000h, 0dah, 00fh, 000h, 000h, 001h, 0f3h, 000h, 000h, 000h, 000h, 000h

section CONST progbits vstart=0xb0 align=1 ; size=0xcf0 class=DATA group=DGROUP
    db   'CPUID EDX: 0x%lx', 00ah, 000h
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

section CONST2 progbits vstart=0xda0 align=1 ; size=0x3fa class=DATA group=DGROUP
_bios_cvs_version_string:                    ; 0xf0da0 LB 0x12
    db  'VirtualBox 5.2.14', 000h
_bios_prefix_string:                         ; 0xf0db2 LB 0x8
    db  'BIOS: ', 000h, 000h
_isotag:                                     ; 0xf0dba LB 0x6
    db  'CD001', 000h
_eltorito:                                   ; 0xf0dc0 LB 0x18
    db  'EL TORITO SPECIFICATION', 000h
_drivetypes:                                 ; 0xf0dd8 LB 0x28
    db  046h, 06ch, 06fh, 070h, 070h, 079h, 000h, 000h, 000h, 000h, 048h, 061h, 072h, 064h, 020h, 044h
    db  069h, 073h, 06bh, 000h, 043h, 044h, 02dh, 052h, 04fh, 04dh, 000h, 000h, 000h, 000h, 04ch, 041h
    db  04eh, 000h, 000h, 000h, 000h, 000h, 000h, 000h
_scan_to_scanascii:                          ; 0xf0e00 LB 0x37a
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
_panic_msg_keyb_buffer_full:                 ; 0xf117a LB 0x20
    db  '%s: keyboard input buffer full', 00ah, 000h

  ; Padding 0x466 bytes at 0xf119a
  times 1126 db 0

section _TEXT progbits vstart=0x1600 align=1 ; size=0x8bc5 class=CODE group=AUTO
rom_scan_:                                   ; 0xf1600 LB 0x50
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push si                                   ; 56
    push ax                                   ; 50
    push ax                                   ; 50
    mov bx, ax                                ; 89 c3
    mov cx, dx                                ; 89 d1
    cmp bx, cx                                ; 39 cb
    jnc short 01648h                          ; 73 38
    xor si, si                                ; 31 f6
    mov dx, bx                                ; 89 da
    mov es, bx                                ; 8e c3
    cmp word [es:si], 0aa55h                  ; 26 81 3c 55 aa
    jne short 01642h                          ; 75 25
    mov word [bp-008h], bx                    ; 89 5e f8
    mov word [bp-00ah], strict word 00003h    ; c7 46 f6 03 00
    call far [bp-00ah]                        ; ff 5e f6
    cli                                       ; fa
    mov es, bx                                ; 8e c3
    mov al, byte [es:si+002h]                 ; 26 8a 44 02
    add AL, strict byte 003h                  ; 04 03
    and AL, strict byte 0fch                  ; 24 fc
    xor ah, ah                                ; 30 e4
    cwd                                       ; 99
    sal dx, 002h                              ; c1 e2 02
    db  01bh, 0c2h
    ; sbb ax, dx                                ; 1b c2
    sar ax, 002h                              ; c1 f8 02
    add bx, ax                                ; 01 c3
    jmp short 0160ch                          ; eb ca
    add bx, 00080h                            ; 81 c3 80 00
    jmp short 0160ch                          ; eb c4
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
is_cpuid_supported_:                         ; 0xf1650 LB 0x42
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    pushfd                                    ; 66 9c
    pop edx                                   ; 66 5a
    db  08bh, 0c2h
    ; mov ax, dx                                ; 8b c2
    shr edx, 010h                             ; 66 c1 ea 10
    mov bx, ax                                ; 89 c3
    mov cx, dx                                ; 89 d1
    xor dl, 020h                              ; 80 f2 20
    sal edx, 010h                             ; 66 c1 e2 10
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    push edx                                  ; 66 52
    popfd                                     ; 66 9d
    pushfd                                    ; 66 9c
    pop edx                                   ; 66 5a
    db  08bh, 0c2h
    ; mov ax, dx                                ; 8b c2
    shr edx, 010h                             ; 66 c1 ea 10
    cmp cx, dx                                ; 39 d1
    jne short 01683h                          ; 75 04
    cmp bx, ax                                ; 39 c3
    je short 01688h                           ; 74 05
    mov ax, strict word 00001h                ; b8 01 00
    jmp short 0168ah                          ; eb 02
    xor ax, ax                                ; 31 c0
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
_apic_setup:                                 ; 0xf1692 LB 0xb7
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00010h                ; 83 ec 10
    call 01650h                               ; e8 b3 ff
    test ax, ax                               ; 85 c0
    je near 01742h                            ; 0f 84 9f 00
    mov ax, strict word 00001h                ; b8 01 00
    xor dx, dx                                ; 31 d2
    push SS                                   ; 16
    pop ES                                    ; 07
    lea di, [bp-014h]                         ; 8d 7e ec
    sal edx, 010h                             ; 66 c1 e2 10
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    db  066h, 08bh, 0c2h
    ; mov eax, edx                              ; 66 8b c2
    cpuid                                     ; 0f a2
    db  066h, 026h, 089h, 005h
    ; mov dword [es:di], eax                    ; 66 26 89 05
    db  066h, 026h, 089h, 05dh, 004h
    ; mov dword [es:di+004h], ebx               ; 66 26 89 5d 04
    db  066h, 026h, 089h, 04dh, 008h
    ; mov dword [es:di+008h], ecx               ; 66 26 89 4d 08
    db  066h, 026h, 089h, 055h, 00ch
    ; mov dword [es:di+00ch], edx               ; 66 26 89 55 0c
    mov ax, word [bp-008h]                    ; 8b 46 f8
    mov dx, word [bp-006h]                    ; 8b 56 fa
    push dx                                   ; 52
    push ax                                   ; 50
    push 000b0h                               ; 68 b0 00
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 90 03
    add sp, strict byte 00008h                ; 83 c4 08
    mov ax, word [bp-008h]                    ; 8b 46 f8
    test ah, 002h                             ; f6 c4 02
    je short 01742h                           ; 74 5c
    mov ax, strict word 00078h                ; b8 78 00
    call 017a5h                               ; e8 b9 00
    cmp AL, strict byte 002h                  ; 3c 02
    jne short 016f5h                          ; 75 05
    mov si, 00400h                            ; be 00 04
    jmp short 01704h                          ; eb 0f
    test al, al                               ; 84 c0
    jne short 016feh                          ; 75 05
    mov si, 00800h                            ; be 00 08
    jmp short 01700h                          ; eb 02
    xor si, si                                ; 31 f6
    test si, si                               ; 85 f6
    je short 01742h                           ; 74 3e
    mov ax, strict word 0001bh                ; b8 1b 00
    xor cx, cx                                ; 31 c9
    sal ecx, 010h                             ; 66 c1 e1 10
    db  08bh, 0c8h
    ; mov cx, ax                                ; 8b c8
    rdmsr                                     ; 0f 32
    xchg edx, eax                             ; 66 92
    db  08bh, 0d8h
    ; mov bx, ax                                ; 8b d8
    shr eax, 010h                             ; 66 c1 e8 10
    db  08bh, 0cah
    ; mov cx, dx                                ; 8b ca
    shr edx, 010h                             ; 66 c1 ea 10
    xchg dx, cx                               ; 87 ca
    or dx, si                                 ; 09 f2
    mov si, strict word 0001bh                ; be 1b 00
    xor di, di                                ; 31 ff
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c3h
    ; mov ax, bx                                ; 8b c3
    xchg dx, cx                               ; 87 ca
    sal edx, 010h                             ; 66 c1 e2 10
    db  08bh, 0d1h
    ; mov dx, cx                                ; 8b d1
    xchg edx, eax                             ; 66 92
    db  08bh, 0cfh
    ; mov cx, di                                ; 8b cf
    sal ecx, 010h                             ; 66 c1 e1 10
    db  08bh, 0ceh
    ; mov cx, si                                ; 8b ce
    wrmsr                                     ; 0f 30
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
read_byte_:                                  ; 0xf1749 LB 0xe
    push bx                                   ; 53
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov bx, dx                                ; 89 d3
    mov es, ax                                ; 8e c0
    mov al, byte [es:bx]                      ; 26 8a 07
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    retn                                      ; c3
write_byte_:                                 ; 0xf1757 LB 0xe
    push si                                   ; 56
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov si, dx                                ; 89 d6
    mov es, ax                                ; 8e c0
    mov byte [es:si], bl                      ; 26 88 1c
    pop bp                                    ; 5d
    pop si                                    ; 5e
    retn                                      ; c3
read_word_:                                  ; 0xf1765 LB 0xe
    push bx                                   ; 53
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov bx, dx                                ; 89 d3
    mov es, ax                                ; 8e c0
    mov ax, word [es:bx]                      ; 26 8b 07
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    retn                                      ; c3
write_word_:                                 ; 0xf1773 LB 0xe
    push si                                   ; 56
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov si, dx                                ; 89 d6
    mov es, ax                                ; 8e c0
    mov word [es:si], bx                      ; 26 89 1c
    pop bp                                    ; 5d
    pop si                                    ; 5e
    retn                                      ; c3
read_dword_:                                 ; 0xf1781 LB 0x12
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
write_dword_:                                ; 0xf1793 LB 0x12
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
inb_cmos_:                                   ; 0xf17a5 LB 0x1d
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push dx                                   ; 52
    mov AH, strict byte 070h                  ; b4 70
    cmp AL, strict byte 080h                  ; 3c 80
    jc short 017b1h                           ; 72 02
    mov AH, strict byte 072h                  ; b4 72
    movzx dx, ah                              ; 0f b6 d4
    out DX, AL                                ; ee
    movzx dx, ah                              ; 0f b6 d4
    inc dx                                    ; 42
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop dx                                    ; 5a
    pop bp                                    ; 5d
    retn                                      ; c3
outb_cmos_:                                  ; 0xf17c2 LB 0x1f
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    mov ah, dl                                ; 88 d4
    mov BL, strict byte 070h                  ; b3 70
    cmp AL, strict byte 080h                  ; 3c 80
    jc short 017d0h                           ; 72 02
    mov BL, strict byte 072h                  ; b3 72
    movzx dx, bl                              ; 0f b6 d3
    out DX, AL                                ; ee
    movzx dx, bl                              ; 0f b6 d3
    inc dx                                    ; 42
    mov al, ah                                ; 88 e0
    out DX, AL                                ; ee
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
_dummy_isr_function:                         ; 0xf17e1 LB 0x6b
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
    je short 0183ch                           ; 74 43
    mov AL, strict byte 00bh                  ; b0 0b
    mov dx, 000a0h                            ; ba a0 00
    out DX, AL                                ; ee
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov cx, ax                                ; 89 c1
    test al, al                               ; 84 c0
    je short 0181eh                           ; 74 16
    mov dx, 000a1h                            ; ba a1 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    xor ah, ah                                ; 30 e4
    movzx bx, cl                              ; 0f b6 d9
    or ax, bx                                 ; 09 d8
    out DX, AL                                ; ee
    mov AL, strict byte 020h                  ; b0 20
    mov dx, 000a0h                            ; ba a0 00
    out DX, AL                                ; ee
    jmp short 01833h                          ; eb 15
    mov dx, strict word 00021h                ; ba 21 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and bl, 0fbh                              ; 80 e3 fb
    mov byte [bp-002h], bl                    ; 88 5e fe
    xor ah, ah                                ; 30 e4
    movzx bx, byte [bp-002h]                  ; 0f b6 5e fe
    or ax, bx                                 ; 09 d8
    out DX, AL                                ; ee
    mov AL, strict byte 020h                  ; b0 20
    mov dx, strict word 00020h                ; ba 20 00
    out DX, AL                                ; ee
    mov cl, byte [bp-002h]                    ; 8a 4e fe
    movzx bx, cl                              ; 0f b6 d9
    mov dx, strict word 0006bh                ; ba 6b 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 0f ff
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
_nmi_handler_msg:                            ; 0xf184c LB 0x12
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push 000c2h                               ; 68 c2 00
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 14 02
    add sp, strict byte 00004h                ; 83 c4 04
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
_int18_panic_msg:                            ; 0xf185e LB 0x12
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push 000d6h                               ; 68 d6 00
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 02 02
    add sp, strict byte 00004h                ; 83 c4 04
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
_log_bios_start:                             ; 0xf1870 LB 0x20
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 ac 01
    push 00da0h                               ; 68 a0 0d
    push 000ebh                               ; 68 eb 00
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 e2 01
    add sp, strict byte 00006h                ; 83 c4 06
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
_print_bios_banner:                          ; 0xf1890 LB 0x2e
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov dx, strict word 00072h                ; ba 72 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 c9 fe
    mov cx, ax                                ; 89 c1
    xor bx, bx                                ; 31 db
    mov dx, strict word 00072h                ; ba 72 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01773h                               ; e8 ca fe
    cmp cx, 01234h                            ; 81 f9 34 12
    jne short 018b7h                          ; 75 08
    mov AL, strict byte 003h                  ; b0 03
    mov AH, strict byte 000h                  ; b4 00
    int 010h                                  ; cd 10
    jmp short 018bah                          ; eb 03
    call 07c3eh                               ; e8 84 63
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
send_:                                       ; 0xf18be LB 0x3b
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    mov bx, ax                                ; 89 c3
    mov cl, dl                                ; 88 d1
    test AL, strict byte 008h                 ; a8 08
    je short 018d1h                           ; 74 06
    mov al, dl                                ; 88 d0
    mov dx, 00403h                            ; ba 03 04
    out DX, AL                                ; ee
    test bl, 004h                             ; f6 c3 04
    je short 018dch                           ; 74 06
    mov al, cl                                ; 88 c8
    mov dx, 00504h                            ; ba 04 05
    out DX, AL                                ; ee
    test bl, 002h                             ; f6 c3 02
    je short 018f2h                           ; 74 11
    cmp cl, 00ah                              ; 80 f9 0a
    jne short 018ech                          ; 75 06
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
put_int_:                                    ; 0xf18f9 LB 0x5f
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    push ax                                   ; 50
    mov si, ax                                ; 89 c6
    mov word [bp-008h], dx                    ; 89 56 f8
    mov di, strict word 0000ah                ; bf 0a 00
    mov ax, dx                                ; 89 d0
    cwd                                       ; 99
    idiv di                                   ; f7 ff
    mov word [bp-006h], ax                    ; 89 46 fa
    test ax, ax                               ; 85 c0
    je short 0191eh                           ; 74 0a
    dec bx                                    ; 4b
    mov dx, ax                                ; 89 c2
    mov ax, si                                ; 89 f0
    call 018f9h                               ; e8 dd ff
    jmp short 01939h                          ; eb 1b
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jle short 0192dh                          ; 7e 0a
    mov dx, strict word 00020h                ; ba 20 00
    mov ax, si                                ; 89 f0
    call 018beh                               ; e8 93 ff
    jmp short 0191eh                          ; eb f1
    test cx, cx                               ; 85 c9
    je short 01939h                           ; 74 08
    mov dx, strict word 0002dh                ; ba 2d 00
    mov ax, si                                ; 89 f0
    call 018beh                               ; e8 85 ff
    mov al, byte [bp-006h]                    ; 8a 46 fa
    mov DL, strict byte 00ah                  ; b2 0a
    mul dl                                    ; f6 e2
    mov dl, byte [bp-008h]                    ; 8a 56 f8
    sub dl, al                                ; 28 c2
    mov al, dl                                ; 88 d0
    add AL, strict byte 030h                  ; 04 30
    movzx dx, al                              ; 0f b6 d0
    mov ax, si                                ; 89 f0
    call 018beh                               ; e8 6d ff
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
put_uint_:                                   ; 0xf1958 LB 0x60
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
    je short 0197eh                           ; 74 0a
    dec bx                                    ; 4b
    mov dx, ax                                ; 89 c2
    mov ax, si                                ; 89 f0
    call 01958h                               ; e8 dc ff
    jmp short 01999h                          ; eb 1b
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jle short 0198dh                          ; 7e 0a
    mov dx, strict word 00020h                ; ba 20 00
    mov ax, si                                ; 89 f0
    call 018beh                               ; e8 33 ff
    jmp short 0197eh                          ; eb f1
    test cx, cx                               ; 85 c9
    je short 01999h                           ; 74 08
    mov dx, strict word 0002dh                ; ba 2d 00
    mov ax, si                                ; 89 f0
    call 018beh                               ; e8 25 ff
    mov al, byte [bp-006h]                    ; 8a 46 fa
    mov DL, strict byte 00ah                  ; b2 0a
    mul dl                                    ; f6 e2
    mov dl, byte [bp-008h]                    ; 8a 56 f8
    sub dl, al                                ; 28 c2
    mov al, dl                                ; 88 d0
    add AL, strict byte 030h                  ; 04 30
    movzx dx, al                              ; 0f b6 d0
    mov ax, si                                ; 89 f0
    call 018beh                               ; e8 0d ff
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
put_luint_:                                  ; 0xf19b8 LB 0x72
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
    call 0a0d0h                               ; e8 fe 86
    mov word [bp-008h], ax                    ; 89 46 f8
    mov cx, dx                                ; 89 d1
    mov dx, ax                                ; 89 c2
    or dx, cx                                 ; 09 ca
    je short 019ech                           ; 74 0f
    push word [bp+004h]                       ; ff 76 04
    lea dx, [di-001h]                         ; 8d 55 ff
    mov bx, ax                                ; 89 c3
    mov ax, si                                ; 89 f0
    call 019b8h                               ; e8 ce ff
    jmp short 01a09h                          ; eb 1d
    dec di                                    ; 4f
    test di, di                               ; 85 ff
    jle short 019fbh                          ; 7e 0a
    mov dx, strict word 00020h                ; ba 20 00
    mov ax, si                                ; 89 f0
    call 018beh                               ; e8 c5 fe
    jmp short 019ech                          ; eb f1
    cmp word [bp+004h], strict byte 00000h    ; 83 7e 04 00
    je short 01a09h                           ; 74 08
    mov dx, strict word 0002dh                ; ba 2d 00
    mov ax, si                                ; 89 f0
    call 018beh                               ; e8 b5 fe
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov DL, strict byte 00ah                  ; b2 0a
    mul dl                                    ; f6 e2
    mov dl, byte [bp-006h]                    ; 8a 56 fa
    sub dl, al                                ; 28 c2
    mov al, dl                                ; 88 d0
    add AL, strict byte 030h                  ; 04 30
    movzx dx, al                              ; 0f b6 d0
    mov ax, si                                ; 89 f0
    call 018beh                               ; e8 9d fe
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 00002h                               ; c2 02 00
put_str_:                                    ; 0xf1a2a LB 0x21
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push dx                                   ; 52
    push si                                   ; 56
    mov si, ax                                ; 89 c6
    mov es, cx                                ; 8e c1
    mov dl, byte [es:bx]                      ; 26 8a 17
    test dl, dl                               ; 84 d2
    je short 01a44h                           ; 74 0a
    xor dh, dh                                ; 30 f6
    mov ax, si                                ; 89 f0
    call 018beh                               ; e8 7d fe
    inc bx                                    ; 43
    jmp short 01a31h                          ; eb ed
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop bp                                    ; 5d
    retn                                      ; c3
put_str_near_:                               ; 0xf1a4b LB 0x20
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    mov cx, ax                                ; 89 c1
    mov bx, dx                                ; 89 d3
    mov dl, byte [bx]                         ; 8a 17
    test dl, dl                               ; 84 d2
    je short 01a64h                           ; 74 0a
    xor dh, dh                                ; 30 f6
    mov ax, cx                                ; 89 c8
    call 018beh                               ; e8 5d fe
    inc bx                                    ; 43
    jmp short 01a54h                          ; eb f0
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
bios_printf_:                                ; 0xf1a6b LB 0x33d
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
    xor bx, bx                                ; 31 db
    xor di, di                                ; 31 ff
    mov ax, word [bp+004h]                    ; 8b 46 04
    and ax, strict word 00007h                ; 25 07 00
    cmp ax, strict word 00007h                ; 3d 07 00
    jne short 01a99h                          ; 75 0b
    push 000f0h                               ; 68 f0 00
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 d5 ff
    add sp, strict byte 00004h                ; 83 c4 04
    mov si, word [bp+006h]                    ; 8b 76 06
    mov dl, byte [si]                         ; 8a 14
    test dl, dl                               ; 84 d2
    je near 01d8ch                            ; 0f 84 e8 02
    cmp dl, 025h                              ; 80 fa 25
    jne short 01ab1h                          ; 75 08
    mov bx, strict word 00001h                ; bb 01 00
    xor di, di                                ; 31 ff
    jmp near 01d86h                           ; e9 d5 02
    test bx, bx                               ; 85 db
    je near 01d7eh                            ; 0f 84 c7 02
    cmp dl, 030h                              ; 80 fa 30
    jc short 01acfh                           ; 72 13
    cmp dl, 039h                              ; 80 fa 39
    jnbe short 01acfh                         ; 77 0e
    movzx ax, dl                              ; 0f b6 c2
    imul di, di, strict byte 0000ah           ; 6b ff 0a
    sub ax, strict word 00030h                ; 2d 30 00
    add di, ax                                ; 01 c7
    jmp near 01d86h                           ; e9 b7 02
    mov ax, word [bp-014h]                    ; 8b 46 ec
    mov word [bp-014h], ax                    ; 89 46 ec
    add word [bp-016h], strict byte 00002h    ; 83 46 ea 02
    les bx, [bp-016h]                         ; c4 5e ea
    mov ax, word [es:bx-002h]                 ; 26 8b 47 fe
    mov word [bp-012h], ax                    ; 89 46 ee
    cmp dl, 078h                              ; 80 fa 78
    je short 01aedh                           ; 74 05
    cmp dl, 058h                              ; 80 fa 58
    jne short 01b43h                          ; 75 56
    test di, di                               ; 85 ff
    jne short 01af4h                          ; 75 03
    mov di, strict word 00004h                ; bf 04 00
    cmp dl, 078h                              ; 80 fa 78
    jne short 01b00h                          ; 75 07
    mov word [bp-00eh], strict word 00061h    ; c7 46 f2 61 00
    jmp short 01b05h                          ; eb 05
    mov word [bp-00eh], strict word 00041h    ; c7 46 f2 41 00
    lea ax, [di-001h]                         ; 8d 45 ff
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    test ax, ax                               ; 85 c0
    jl near 01d7ah                            ; 0f 8c 66 02
    mov cx, ax                                ; 89 c1
    sal cx, 002h                              ; c1 e1 02
    mov ax, word [bp-012h]                    ; 8b 46 ee
    shr ax, CL                                ; d3 e8
    xor ah, ah                                ; 30 e4
    and AL, strict byte 00fh                  ; 24 0f
    cmp ax, strict word 00009h                ; 3d 09 00
    jnbe short 01b2eh                         ; 77 07
    mov dx, ax                                ; 89 c2
    add dx, strict byte 00030h                ; 83 c2 30
    jmp short 01b36h                          ; eb 08
    sub ax, strict word 0000ah                ; 2d 0a 00
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    add dx, ax                                ; 01 c2
    xor dh, dh                                ; 30 f6
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 018beh                               ; e8 80 fd
    dec word [bp-00ch]                        ; ff 4e f4
    jmp short 01b0bh                          ; eb c8
    cmp dl, 075h                              ; 80 fa 75
    jne short 01b57h                          ; 75 0f
    xor cx, cx                                ; 31 c9
    mov bx, di                                ; 89 fb
    mov dx, ax                                ; 89 c2
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01958h                               ; e8 04 fe
    jmp near 01d7ah                           ; e9 23 02
    cmp dl, 06ch                              ; 80 fa 6c
    jne near 01c39h                           ; 0f 85 db 00
    mov bx, word [bp+006h]                    ; 8b 5e 06
    cmp dl, byte [bx+001h]                    ; 3a 57 01
    jne near 01c39h                           ; 0f 85 d1 00
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
    je short 01bbah                           ; 74 05
    cmp dl, 058h                              ; 80 fa 58
    jne short 01c2bh                          ; 75 71
    test di, di                               ; 85 ff
    jne short 01bc1h                          ; 75 03
    mov di, strict word 00010h                ; bf 10 00
    cmp dl, 078h                              ; 80 fa 78
    jne short 01bcdh                          ; 75 07
    mov word [bp-00eh], strict word 00061h    ; c7 46 f2 61 00
    jmp short 01bd2h                          ; eb 05
    mov word [bp-00eh], strict word 00041h    ; c7 46 f2 41 00
    lea ax, [di-001h]                         ; 8d 45 ff
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    test ax, ax                               ; 85 c0
    jl near 01d7ah                            ; 0f 8c 99 01
    sal ax, 002h                              ; c1 e0 02
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
    call 0a110h                               ; e8 0c 85
    mov ax, dx                                ; 89 d0
    xor ah, dh                                ; 30 f4
    and AL, strict byte 00fh                  ; 24 0f
    cmp ax, strict word 00009h                ; 3d 09 00
    jnbe short 01c16h                         ; 77 07
    mov dx, ax                                ; 89 c2
    add dx, strict byte 00030h                ; 83 c2 30
    jmp short 01c1eh                          ; eb 08
    sub ax, strict word 0000ah                ; 2d 0a 00
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    add dx, ax                                ; 01 c2
    xor dh, dh                                ; 30 f6
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 018beh                               ; e8 98 fc
    dec word [bp-00ch]                        ; ff 4e f4
    jmp short 01bd8h                          ; eb ad
    push 000f8h                               ; 68 f8 00
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 38 fe
    add sp, strict byte 00004h                ; 83 c4 04
    jmp near 01d7ah                           ; e9 41 01
    lea bx, [di-001h]                         ; 8d 5d ff
    cmp dl, 06ch                              ; 80 fa 6c
    jne near 01d02h                           ; 0f 85 bf 00
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
    jne short 01c91h                          ; 75 2d
    test byte [bp-00fh], 080h                 ; f6 46 f1 80
    je short 01c7fh                           ; 74 15
    push strict byte 00001h                   ; 6a 01
    mov ax, word [bp-012h]                    ; 8b 46 ee
    mov cx, word [bp-010h]                    ; 8b 4e f0
    neg cx                                    ; f7 d9
    neg ax                                    ; f7 d8
    sbb cx, strict byte 00000h                ; 83 d9 00
    mov dx, bx                                ; 89 da
    mov bx, ax                                ; 89 c3
    jmp short 01c88h                          ; eb 09
    push strict byte 00000h                   ; 6a 00
    mov bx, word [bp-012h]                    ; 8b 5e ee
    mov dx, di                                ; 89 fa
    mov cx, ax                                ; 89 c1
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 019b8h                               ; e8 2a fd
    jmp near 01d7ah                           ; e9 e9 00
    cmp dl, 075h                              ; 80 fa 75
    jne short 01c98h                          ; 75 02
    jmp short 01c7fh                          ; eb e7
    cmp dl, 078h                              ; 80 fa 78
    je short 01ca4h                           ; 74 07
    cmp dl, 058h                              ; 80 fa 58
    jne near 01d7ah                           ; 0f 85 d6 00
    test di, di                               ; 85 ff
    jne short 01cabh                          ; 75 03
    mov di, strict word 00008h                ; bf 08 00
    cmp dl, 078h                              ; 80 fa 78
    jne short 01cb7h                          ; 75 07
    mov word [bp-00eh], strict word 00061h    ; c7 46 f2 61 00
    jmp short 01cbch                          ; eb 05
    mov word [bp-00eh], strict word 00041h    ; c7 46 f2 41 00
    lea ax, [di-001h]                         ; 8d 45 ff
    mov word [bp-00ch], ax                    ; 89 46 f4
    cmp word [bp-00ch], strict byte 00000h    ; 83 7e f4 00
    jl near 01d7ah                            ; 0f 8c b0 00
    mov ax, word [bp-012h]                    ; 8b 46 ee
    mov cx, word [bp-00ch]                    ; 8b 4e f4
    sal cx, 002h                              ; c1 e1 02
    mov dx, word [bp-010h]                    ; 8b 56 f0
    jcxz 01cdeh                               ; e3 06
    shr dx, 1                                 ; d1 ea
    rcr ax, 1                                 ; d1 d8
    loop 01cd8h                               ; e2 fa
    and ax, strict word 0000fh                ; 25 0f 00
    cmp ax, strict word 00009h                ; 3d 09 00
    jnbe short 01cedh                         ; 77 07
    mov dx, ax                                ; 89 c2
    add dx, strict byte 00030h                ; 83 c2 30
    jmp short 01cf5h                          ; eb 08
    sub ax, strict word 0000ah                ; 2d 0a 00
    mov dx, word [bp-00eh]                    ; 8b 56 f2
    add dx, ax                                ; 01 c2
    xor dh, dh                                ; 30 f6
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 018beh                               ; e8 c1 fb
    dec word [bp-00ch]                        ; ff 4e f4
    jmp short 01cc2h                          ; eb c0
    cmp dl, 064h                              ; 80 fa 64
    jne short 01d26h                          ; 75 1f
    test byte [bp-011h], 080h                 ; f6 46 ef 80
    je short 01d17h                           ; 74 0a
    mov dx, word [bp-012h]                    ; 8b 56 ee
    neg dx                                    ; f7 da
    mov cx, strict word 00001h                ; b9 01 00
    jmp short 01d1eh                          ; eb 07
    xor cx, cx                                ; 31 c9
    mov bx, di                                ; 89 fb
    mov dx, word [bp-012h]                    ; 8b 56 ee
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 018f9h                               ; e8 d5 fb
    jmp short 01d7ah                          ; eb 54
    cmp dl, 073h                              ; 80 fa 73
    jne short 01d38h                          ; 75 0d
    mov cx, ds                                ; 8c d9
    mov bx, word [bp-012h]                    ; 8b 5e ee
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01a2ah                               ; e8 f4 fc
    jmp short 01d7ah                          ; eb 42
    cmp dl, 053h                              ; 80 fa 53
    jne short 01d5eh                          ; 75 21
    mov ax, word [bp-012h]                    ; 8b 46 ee
    mov word [bp-010h], ax                    ; 89 46 f0
    mov ax, word [bp-014h]                    ; 8b 46 ec
    mov word [bp-014h], ax                    ; 89 46 ec
    add word [bp-016h], strict byte 00002h    ; 83 46 ea 02
    les bx, [bp-016h]                         ; c4 5e ea
    mov ax, word [es:bx-002h]                 ; 26 8b 47 fe
    mov word [bp-012h], ax                    ; 89 46 ee
    mov bx, ax                                ; 89 c3
    mov cx, word [bp-010h]                    ; 8b 4e f0
    jmp short 01d30h                          ; eb d2
    cmp dl, 063h                              ; 80 fa 63
    jne short 01d6fh                          ; 75 0c
    movzx dx, byte [bp-012h]                  ; 0f b6 56 ee
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 018beh                               ; e8 51 fb
    jmp short 01d7ah                          ; eb 0b
    push 00119h                               ; 68 19 01
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 f4 fc
    add sp, strict byte 00004h                ; 83 c4 04
    xor bx, bx                                ; 31 db
    jmp short 01d86h                          ; eb 08
    xor dh, dh                                ; 30 f6
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 018beh                               ; e8 38 fb
    inc word [bp+006h]                        ; ff 46 06
    jmp near 01a99h                           ; e9 0d fd
    xor ax, ax                                ; 31 c0
    mov word [bp-016h], ax                    ; 89 46 ea
    mov word [bp-014h], ax                    ; 89 46 ec
    test byte [bp+004h], 001h                 ; f6 46 04 01
    je short 01d9eh                           ; 74 04
    cli                                       ; fa
    hlt                                       ; f4
    jmp short 01d9bh                          ; eb fd
    lea sp, [bp-00ah]                         ; 8d 66 f6
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
_ata_init:                                   ; 0xf1da8 LB 0xcd
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 b0 f9
    mov si, 00122h                            ; be 22 01
    mov dx, ax                                ; 89 c2
    xor al, al                                ; 30 c0
    jmp short 01dc2h                          ; eb 04
    cmp AL, strict byte 004h                  ; 3c 04
    jnc short 01de6h                          ; 73 24
    movzx bx, al                              ; 0f b6 d8
    imul bx, bx, strict byte 00006h           ; 6b db 06
    mov es, dx                                ; 8e c2
    add bx, si                                ; 01 f3
    mov byte [es:bx+00204h], 000h             ; 26 c6 87 04 02 00
    db  066h, 026h, 0c7h, 087h, 006h, 002h, 000h, 000h, 000h, 000h
    ; mov dword [es:bx+00206h], strict dword 000000000h ; 66 26 c7 87 06 02 00 00 00 00
    mov byte [es:bx+00205h], 000h             ; 26 c6 87 05 02 00
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    jmp short 01dbeh                          ; eb d8
    xor al, al                                ; 30 c0
    jmp short 01deeh                          ; eb 04
    cmp AL, strict byte 008h                  ; 3c 08
    jnc short 01e42h                          ; 73 54
    movzx bx, al                              ; 0f b6 d8
    imul bx, bx, strict byte 0001ch           ; 6b db 1c
    mov es, dx                                ; 8e c2
    add bx, si                                ; 01 f3
    db  066h, 026h, 0c7h, 047h, 022h, 000h, 000h, 000h, 000h
    ; mov dword [es:bx+022h], strict dword 000000000h ; 66 26 c7 47 22 00 00 00 00
    mov byte [es:bx+026h], 000h               ; 26 c6 47 26 00
    mov word [es:bx+028h], 00200h             ; 26 c7 47 28 00 02
    mov byte [es:bx+027h], 000h               ; 26 c6 47 27 00
    db  066h, 026h, 0c7h, 047h, 02ah, 000h, 000h, 000h, 000h
    ; mov dword [es:bx+02ah], strict dword 000000000h ; 66 26 c7 47 2a 00 00 00 00
    db  066h, 026h, 0c7h, 047h, 02eh, 000h, 000h, 000h, 000h
    ; mov dword [es:bx+02eh], strict dword 000000000h ; 66 26 c7 47 2e 00 00 00 00
    db  066h, 026h, 0c7h, 047h, 032h, 000h, 000h, 000h, 000h
    ; mov dword [es:bx+032h], strict dword 000000000h ; 66 26 c7 47 32 00 00 00 00
    db  066h, 026h, 0c7h, 047h, 03ah, 000h, 000h, 000h, 000h
    ; mov dword [es:bx+03ah], strict dword 000000000h ; 66 26 c7 47 3a 00 00 00 00
    db  066h, 026h, 0c7h, 047h, 036h, 000h, 000h, 000h, 000h
    ; mov dword [es:bx+036h], strict dword 000000000h ; 66 26 c7 47 36 00 00 00 00
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    jmp short 01deah                          ; eb a8
    xor al, al                                ; 30 c0
    jmp short 01e4ah                          ; eb 04
    cmp AL, strict byte 010h                  ; 3c 10
    jnc short 01e61h                          ; 73 17
    movzx bx, al                              ; 0f b6 d8
    mov es, dx                                ; 8e c2
    add bx, si                                ; 01 f3
    mov byte [es:bx+001e3h], 010h             ; 26 c6 87 e3 01 10
    mov byte [es:bx+001f4h], 010h             ; 26 c6 87 f4 01 10
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    jmp short 01e46h                          ; eb e5
    mov es, dx                                ; 8e c2
    mov byte [es:si+001e2h], 000h             ; 26 c6 84 e2 01 00
    mov byte [es:si+001f3h], 000h             ; 26 c6 84 f3 01 00
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
ata_reset_:                                  ; 0xf1e75 LB 0xde
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
    call 01765h                               ; e8 dc f8
    mov word [bp-00eh], 00122h                ; c7 46 f2 22 01
    mov di, ax                                ; 89 c7
    mov bx, word [bp-010h]                    ; 8b 5e f0
    shr bx, 1                                 ; d1 eb
    mov dl, byte [bp-010h]                    ; 8a 56 f0
    and dl, 001h                              ; 80 e2 01
    mov byte [bp-00ch], dl                    ; 88 56 f4
    xor bh, bh                                ; 30 ff
    imul bx, bx, strict byte 00006h           ; 6b db 06
    mov es, ax                                ; 8e c0
    add bx, 00122h                            ; 81 c3 22 01
    mov cx, word [es:bx+00206h]               ; 26 8b 8f 06 02
    mov si, word [es:bx+00208h]               ; 26 8b b7 08 02
    lea dx, [si+006h]                         ; 8d 54 06
    mov AL, strict byte 00eh                  ; b0 0e
    out DX, AL                                ; ee
    mov bx, 000ffh                            ; bb ff 00
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 01ecdh                          ; 76 0c
    mov dx, cx                                ; 89 ca
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 080h                 ; a8 80
    je short 01ebch                           ; 74 ef
    lea dx, [si+006h]                         ; 8d 54 06
    mov AL, strict byte 00ah                  ; b0 0a
    out DX, AL                                ; ee
    imul bx, word [bp-010h], strict byte 0001ch ; 6b 5e f0 1c
    mov es, di                                ; 8e c7
    add bx, word [bp-00eh]                    ; 03 5e f2
    cmp byte [es:bx+022h], 000h               ; 26 80 7f 22 00
    je short 01f2fh                           ; 74 4c
    cmp byte [bp-00ch], 000h                  ; 80 7e f4 00
    je short 01eeeh                           ; 74 05
    mov ax, 000b0h                            ; b8 b0 00
    jmp short 01ef1h                          ; eb 03
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
    jne short 01f2fh                          ; 75 22
    cmp al, bl                                ; 38 d8
    jne short 01f2fh                          ; 75 1e
    mov bx, strict word 0ffffh                ; bb ff ff
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 01f2fh                          ; 76 16
    mov dx, cx                                ; 89 ca
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 080h                 ; a8 80
    je short 01f2fh                           ; 74 0a
    mov ax, strict word 0ffffh                ; b8 ff ff
    dec ax                                    ; 48
    test ax, ax                               ; 85 c0
    jnbe short 01f28h                         ; 77 fb
    jmp short 01f14h                          ; eb e5
    mov bx, strict word 00010h                ; bb 10 00
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 01f43h                          ; 76 0c
    mov dx, cx                                ; 89 ca
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 040h                 ; a8 40
    je short 01f32h                           ; 74 ef
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
ata_cmd_data_in_:                            ; 0xf1f53 LB 0x2e2
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00012h                ; 83 ec 12
    push ax                                   ; 50
    push dx                                   ; 52
    push bx                                   ; 53
    push cx                                   ; 51
    mov es, dx                                ; 8e c2
    mov bx, ax                                ; 89 c3
    mov al, byte [es:bx+00ch]                 ; 26 8a 47 0c
    mov byte [bp-00ah], al                    ; 88 46 f6
    movzx bx, al                              ; 0f b6 d8
    mov ax, bx                                ; 89 d8
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    imul ax, ax, strict byte 00006h           ; 6b c0 06
    mov di, word [bp-018h]                    ; 8b 7e e8
    add di, ax                                ; 01 c7
    mov ax, word [es:di+00206h]               ; 26 8b 85 06 02
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov ax, word [es:di+00208h]               ; 26 8b 85 08 02
    mov word [bp-010h], ax                    ; 89 46 f0
    imul bx, bx, strict byte 0001ch           ; 6b db 1c
    mov di, word [bp-018h]                    ; 8b 7e e8
    add di, bx                                ; 01 df
    mov al, byte [es:di+026h]                 ; 26 8a 45 26
    mov byte [bp-008h], al                    ; 88 46 f8
    mov ax, word [es:di+028h]                 ; 26 8b 45 28
    mov word [bp-00eh], ax                    ; 89 46 f2
    test ax, ax                               ; 85 c0
    jne short 01fbah                          ; 75 14
    cmp byte [bp-008h], 001h                  ; 80 7e f8 01
    jne short 01fb3h                          ; 75 07
    mov word [bp-00eh], 04000h                ; c7 46 f2 00 40
    jmp short 01fc9h                          ; eb 16
    mov word [bp-00eh], 08000h                ; c7 46 f2 00 80
    jmp short 01fc9h                          ; eb 0f
    cmp byte [bp-008h], 001h                  ; 80 7e f8 01
    jne short 01fc6h                          ; 75 06
    shr word [bp-00eh], 002h                  ; c1 6e f2 02
    jmp short 01fc9h                          ; eb 03
    shr word [bp-00eh], 1                     ; d1 6e f2
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 080h                 ; a8 80
    je short 01fe5h                           ; 74 0f
    mov dx, word [bp-010h]                    ; 8b 56 f0
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov dx, strict word 00001h                ; ba 01 00
    jmp near 0222ch                           ; e9 47 02
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov di, word [bp-018h]                    ; 8b 7e e8
    mov di, word [es:di+008h]                 ; 26 8b 7d 08
    mov bx, word [bp-018h]                    ; 8b 5e e8
    mov ax, word [es:bx+00ah]                 ; 26 8b 47 0a
    mov word [bp-012h], ax                    ; 89 46 ee
    mov al, byte [es:bx+016h]                 ; 26 8a 47 16
    mov byte [bp-006h], al                    ; 88 46 fa
    mov ax, word [es:bx+012h]                 ; 26 8b 47 12
    mov word [bp-014h], ax                    ; 89 46 ec
    mov bl, byte [es:bx+014h]                 ; 26 8a 5f 14
    mov al, byte [bp-006h]                    ; 8a 46 fa
    test al, al                               ; 84 c0
    jne near 020f7h                           ; 0f 85 e3 00
    xor bx, bx                                ; 31 db
    xor dx, dx                                ; 31 d2
    xor ah, ah                                ; 30 e4
    mov word [bp-016h], ax                    ; 89 46 ea
    mov si, word [bp-018h]                    ; 8b 76 e8
    mov cx, word [es:si]                      ; 26 8b 0c
    add cx, word [bp-01eh]                    ; 03 4e e2
    adc bx, word [es:si+002h]                 ; 26 13 5c 02
    adc dx, word [es:si+004h]                 ; 26 13 54 04
    mov ax, word [es:si+006h]                 ; 26 8b 44 06
    adc ax, word [bp-016h]                    ; 13 46 ea
    test ax, ax                               ; 85 c0
    jnbe short 02049h                         ; 77 10
    jne short 020adh                          ; 75 72
    test dx, dx                               ; 85 d2
    jnbe short 02049h                         ; 77 0a
    jne short 020adh                          ; 75 6c
    cmp bx, 01000h                            ; 81 fb 00 10
    jnbe short 02049h                         ; 77 02
    jne short 020adh                          ; 75 64
    mov bx, si                                ; 89 f3
    mov ax, word [es:bx+006h]                 ; 26 8b 47 06
    mov bx, word [es:bx+004h]                 ; 26 8b 5f 04
    mov cx, word [es:si+002h]                 ; 26 8b 4c 02
    mov dx, word [es:si]                      ; 26 8b 14
    mov si, strict word 00018h                ; be 18 00
    call 0a110h                               ; e8 b0 80
    xor dh, dh                                ; 30 f6
    mov word [bp-016h], dx                    ; 89 56 ea
    mov bx, word [bp-018h]                    ; 8b 5e e8
    mov ax, word [es:bx+006h]                 ; 26 8b 47 06
    mov bx, word [es:bx+004h]                 ; 26 8b 5f 04
    mov si, word [bp-018h]                    ; 8b 76 e8
    mov cx, word [es:si+002h]                 ; 26 8b 4c 02
    mov dx, word [es:si]                      ; 26 8b 14
    mov si, strict word 00020h                ; be 20 00
    call 0a110h                               ; e8 90 80
    mov bx, dx                                ; 89 d3
    mov ax, word [bp-01eh]                    ; 8b 46 e2
    xor al, al                                ; 30 c0
    shr ax, 008h                              ; c1 e8 08
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    inc dx                                    ; 42
    inc dx                                    ; 42
    out DX, AL                                ; ee
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    add dx, strict byte 00003h                ; 83 c2 03
    mov al, byte [bp-016h]                    ; 8a 46 ea
    out DX, AL                                ; ee
    mov ax, bx                                ; 89 d8
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    add dx, strict byte 00004h                ; 83 c2 04
    out DX, AL                                ; ee
    shr ax, 008h                              ; c1 e8 08
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    add dx, strict byte 00005h                ; 83 c2 05
    out DX, AL                                ; ee
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov bx, word [bp-018h]                    ; 8b 5e e8
    mov ax, word [es:bx]                      ; 26 8b 07
    mov byte [bp-006h], al                    ; 88 46 fa
    mov ax, word [es:bx+006h]                 ; 26 8b 47 06
    mov bx, word [es:bx+004h]                 ; 26 8b 5f 04
    mov si, word [bp-018h]                    ; 8b 76 e8
    mov cx, word [es:si+002h]                 ; 26 8b 4c 02
    mov dx, word [es:si]                      ; 26 8b 14
    mov si, strict word 00008h                ; be 08 00
    call 0a110h                               ; e8 3f 80
    mov word [bp-014h], dx                    ; 89 56 ec
    mov bx, word [bp-018h]                    ; 8b 5e e8
    mov ax, word [es:bx+006h]                 ; 26 8b 47 06
    mov bx, word [es:bx+004h]                 ; 26 8b 5f 04
    mov si, word [bp-018h]                    ; 8b 76 e8
    mov cx, word [es:si+002h]                 ; 26 8b 4c 02
    mov dx, word [es:si]                      ; 26 8b 14
    mov si, strict word 00018h                ; be 18 00
    call 0a110h                               ; e8 21 80
    and dx, strict byte 0000fh                ; 83 e2 0f
    or dl, 040h                               ; 80 ca 40
    mov bx, dx                                ; 89 d3
    mov dx, word [bp-010h]                    ; 8b 56 f0
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 00ah                  ; b0 0a
    out DX, AL                                ; ee
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    inc dx                                    ; 42
    xor al, al                                ; 30 c0
    out DX, AL                                ; ee
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    inc dx                                    ; 42
    inc dx                                    ; 42
    mov al, byte [bp-01eh]                    ; 8a 46 e2
    out DX, AL                                ; ee
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    add dx, strict byte 00003h                ; 83 c2 03
    mov al, byte [bp-006h]                    ; 8a 46 fa
    out DX, AL                                ; ee
    mov ax, word [bp-014h]                    ; 8b 46 ec
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    add dx, strict byte 00004h                ; 83 c2 04
    out DX, AL                                ; ee
    shr ax, 008h                              ; c1 e8 08
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    add dx, strict byte 00005h                ; 83 c2 05
    out DX, AL                                ; ee
    test byte [bp-00ah], 001h                 ; f6 46 f6 01
    je short 02139h                           ; 74 05
    mov ax, 000b0h                            ; b8 b0 00
    jmp short 0213ch                          ; eb 03
    mov ax, 000a0h                            ; b8 a0 00
    movzx dx, bl                              ; 0f b6 d3
    or ax, dx                                 ; 09 d0
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    add dx, strict byte 00006h                ; 83 c2 06
    out DX, AL                                ; ee
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    add dx, strict byte 00007h                ; 83 c2 07
    mov al, byte [bp-01ch]                    ; 8a 46 e4
    out DX, AL                                ; ee
    mov ax, word [bp-01ch]                    ; 8b 46 e4
    cmp ax, 000c4h                            ; 3d c4 00
    je short 0215fh                           ; 74 05
    cmp ax, strict word 00029h                ; 3d 29 00
    jne short 02169h                          ; 75 0a
    mov bx, word [bp-01eh]                    ; 8b 5e e2
    mov word [bp-01eh], strict word 00001h    ; c7 46 e2 01 00
    jmp short 0216ch                          ; eb 03
    mov bx, strict word 00001h                ; bb 01 00
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov dl, al                                ; 88 c2
    test AL, strict byte 080h                 ; a8 80
    jne short 0216ch                          ; 75 f1
    test AL, strict byte 001h                 ; a8 01
    je short 0218eh                           ; 74 0f
    mov dx, word [bp-010h]                    ; 8b 56 f0
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov dx, strict word 00002h                ; ba 02 00
    jmp near 0222ch                           ; e9 9e 00
    test dl, 008h                             ; f6 c2 08
    jne short 021a2h                          ; 75 0f
    mov dx, word [bp-010h]                    ; 8b 56 f0
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov dx, strict word 00003h                ; ba 03 00
    jmp near 0222ch                           ; e9 8a 00
    sti                                       ; fb
    cmp di, 0f800h                            ; 81 ff 00 f8
    jc short 021b6h                           ; 72 0d
    sub di, 00800h                            ; 81 ef 00 08
    mov ax, word [bp-012h]                    ; 8b 46 ee
    add ax, 00080h                            ; 05 80 00
    mov word [bp-012h], ax                    ; 89 46 ee
    cmp byte [bp-008h], 001h                  ; 80 7e f8 01
    jne short 021cah                          ; 75 0e
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    mov cx, word [bp-00eh]                    ; 8b 4e f2
    mov es, [bp-012h]                         ; 8e 46 ee
    db  0f3h, 066h, 06dh
    ; rep insd                                  ; f3 66 6d
    jmp short 021d5h                          ; eb 0b
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    mov cx, word [bp-00eh]                    ; 8b 4e f2
    mov es, [bp-012h]                         ; 8e 46 ee
    rep insw                                  ; f3 6d
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov si, word [bp-018h]                    ; 8b 76 e8
    add word [es:si+018h], bx                 ; 26 01 5c 18
    dec word [bp-01eh]                        ; ff 4e e2
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov dl, al                                ; 88 c2
    test AL, strict byte 080h                 ; a8 80
    jne short 021e2h                          ; 75 f1
    cmp word [bp-01eh], strict byte 00000h    ; 83 7e e2 00
    jne short 0220bh                          ; 75 14
    and AL, strict byte 0c9h                  ; 24 c9
    cmp AL, strict byte 040h                  ; 3c 40
    je short 02221h                           ; 74 24
    mov dx, word [bp-010h]                    ; 8b 56 f0
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov dx, strict word 00004h                ; ba 04 00
    jmp short 0222ch                          ; eb 21
    mov al, dl                                ; 88 d0
    and AL, strict byte 0c9h                  ; 24 c9
    cmp AL, strict byte 048h                  ; 3c 48
    je short 021a3h                           ; 74 90
    mov dx, word [bp-010h]                    ; 8b 56 f0
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov dx, strict word 00005h                ; ba 05 00
    jmp short 0222ch                          ; eb 0b
    mov dx, word [bp-010h]                    ; 8b 56 f0
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    xor dx, dx                                ; 31 d2
    mov ax, dx                                ; 89 d0
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
_ata_detect:                                 ; 0xf2235 LB 0x67a
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, 00260h                            ; 81 ec 60 02
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 1e f5
    mov word [bp-024h], ax                    ; 89 46 dc
    mov di, 00122h                            ; bf 22 01
    mov es, ax                                ; 8e c0
    mov word [bp-028h], di                    ; 89 7e d8
    mov word [bp-01ah], ax                    ; 89 46 e6
    mov byte [es:di+00204h], 000h             ; 26 c6 85 04 02 00
    db  066h, 026h, 0c7h, 085h, 006h, 002h, 0f0h, 001h, 0f0h, 003h
    ; mov dword [es:di+00206h], strict dword 003f001f0h ; 66 26 c7 85 06 02 f0 01 f0 03
    mov byte [es:di+00205h], 00eh             ; 26 c6 85 05 02 0e
    mov byte [es:di+0020ah], 000h             ; 26 c6 85 0a 02 00
    db  066h, 026h, 0c7h, 085h, 00ch, 002h, 070h, 001h, 070h, 003h
    ; mov dword [es:di+0020ch], strict dword 003700170h ; 66 26 c7 85 0c 02 70 01 70 03
    mov byte [es:di+0020bh], 00fh             ; 26 c6 85 0b 02 0f
    xor al, al                                ; 30 c0
    mov byte [bp-008h], al                    ; 88 46 f8
    mov byte [bp-012h], al                    ; 88 46 ee
    mov byte [bp-010h], al                    ; 88 46 f0
    jmp near 02835h                           ; e9 a6 05
    mov ax, 000a0h                            ; b8 a0 00
    lea dx, [si+006h]                         ; 8d 54 06
    out DX, AL                                ; ee
    lea bx, [si+002h]                         ; 8d 5c 02
    mov AL, strict byte 055h                  ; b0 55
    mov dx, bx                                ; 89 da
    out DX, AL                                ; ee
    lea di, [si+003h]                         ; 8d 7c 03
    mov AL, strict byte 0aah                  ; b0 aa
    mov dx, di                                ; 89 fa
    out DX, AL                                ; ee
    mov dx, bx                                ; 89 da
    out DX, AL                                ; ee
    mov AL, strict byte 055h                  ; b0 55
    mov dx, di                                ; 89 fa
    out DX, AL                                ; ee
    mov dx, bx                                ; 89 da
    out DX, AL                                ; ee
    mov AL, strict byte 0aah                  ; b0 aa
    mov dx, di                                ; 89 fa
    out DX, AL                                ; ee
    mov dx, bx                                ; 89 da
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov bx, ax                                ; 89 c3
    mov dx, di                                ; 89 fa
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp bl, 055h                              ; 80 fb 55
    jne near 02391h                           ; 0f 85 c8 00
    cmp AL, strict byte 0aah                  ; 3c aa
    jne near 02391h                           ; 0f 85 c2 00
    movzx bx, byte [bp-010h]                  ; 0f b6 5e f0
    imul bx, bx, strict byte 0001ch           ; 6b db 1c
    mov es, [bp-01ah]                         ; 8e 46 e6
    add bx, word [bp-028h]                    ; 03 5e d8
    mov byte [es:bx+022h], 001h               ; 26 c6 47 22 01
    movzx ax, byte [bp-010h]                  ; 0f b6 46 f0
    call 01e75h                               ; e8 8d fb
    cmp byte [bp-016h], 000h                  ; 80 7e ea 00
    je short 022f3h                           ; 74 05
    mov ax, 000b0h                            ; b8 b0 00
    jmp short 022f6h                          ; eb 03
    mov ax, 000a0h                            ; b8 a0 00
    lea dx, [si+006h]                         ; 8d 54 06
    out DX, AL                                ; ee
    lea dx, [si+002h]                         ; 8d 54 02
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov bx, ax                                ; 89 c3
    lea dx, [si+003h]                         ; 8d 54 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp bl, 001h                              ; 80 fb 01
    jne near 02391h                           ; 0f 85 82 00
    cmp al, bl                                ; 38 d8
    jne near 02391h                           ; 0f 85 7c 00
    lea dx, [si+004h]                         ; 8d 54 04
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov bx, ax                                ; 89 c3
    mov byte [bp-00ah], al                    ; 88 46 f6
    lea dx, [si+005h]                         ; 8d 54 05
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov cx, ax                                ; 89 c1
    mov bh, al                                ; 88 c7
    lea dx, [si+007h]                         ; 8d 54 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp bl, 014h                              ; 80 fb 14
    jne short 02350h                          ; 75 1b
    cmp cl, 0ebh                              ; 80 f9 eb
    jne short 02350h                          ; 75 16
    movzx ax, byte [bp-010h]                  ; 0f b6 46 f0
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov bx, word [bp-028h]                    ; 8b 5e d8
    add bx, ax                                ; 01 c3
    mov byte [es:bx+022h], 003h               ; 26 c6 47 22 03
    jmp short 02391h                          ; eb 41
    cmp byte [bp-00ah], 000h                  ; 80 7e f6 00
    jne short 02372h                          ; 75 1c
    test bh, bh                               ; 84 ff
    jne short 02372h                          ; 75 18
    test al, al                               ; 84 c0
    je short 02372h                           ; 74 14
    movzx bx, byte [bp-010h]                  ; 0f b6 5e f0
    imul bx, bx, strict byte 0001ch           ; 6b db 1c
    mov es, [bp-01ah]                         ; 8e 46 e6
    add bx, word [bp-028h]                    ; 03 5e d8
    mov byte [es:bx+022h], 002h               ; 26 c6 47 22 02
    jmp short 02391h                          ; eb 1f
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    cmp AL, strict byte 0ffh                  ; 3c ff
    jne short 02391h                          ; 75 18
    cmp bh, al                                ; 38 c7
    jne short 02391h                          ; 75 14
    movzx ax, byte [bp-010h]                  ; 0f b6 46 f0
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov bx, word [bp-028h]                    ; 8b 5e d8
    add bx, ax                                ; 01 c3
    mov byte [es:bx+022h], 000h               ; 26 c6 47 22 00
    mov dx, word [bp-020h]                    ; 8b 56 e0
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    movzx ax, byte [bp-010h]                  ; 0f b6 46 f0
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov si, word [bp-028h]                    ; 8b 76 d8
    add si, ax                                ; 01 c6
    mov al, byte [es:si+022h]                 ; 26 8a 44 22
    mov byte [bp-00ch], al                    ; 88 46 f4
    cmp AL, strict byte 002h                  ; 3c 02
    jne near 02601h                           ; 0f 85 4b 02
    mov byte [es:si+023h], 0ffh               ; 26 c6 44 23 ff
    mov byte [es:si+026h], 000h               ; 26 c6 44 26 00
    lea dx, [bp-00264h]                       ; 8d 96 9c fd
    mov bx, word [bp-028h]                    ; 8b 5e d8
    mov word [es:bx+008h], dx                 ; 26 89 57 08
    mov [es:bx+00ah], ss                      ; 26 8c 57 0a
    mov al, byte [bp-010h]                    ; 8a 46 f0
    mov byte [es:bx+00ch], al                 ; 26 88 47 0c
    mov cx, strict word 00001h                ; b9 01 00
    mov bx, 000ech                            ; bb ec 00
    mov ax, word [bp-028h]                    ; 8b 46 d8
    mov dx, es                                ; 8c c2
    call 01f53h                               ; e8 6f fb
    test ax, ax                               ; 85 c0
    je short 023f3h                           ; 74 0b
    push 00136h                               ; 68 36 01
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 7b f6
    add sp, strict byte 00004h                ; 83 c4 04
    test byte [bp-00264h], 080h               ; f6 86 9c fd 80
    db  00fh, 095h, 0c0h
    ; setne al                                  ; 0f 95 c0
    xor ah, ah                                ; 30 e4
    mov byte [bp-018h], al                    ; 88 46 e8
    cmp byte [bp-00204h], 000h                ; 80 be fc fd 00
    db  00fh, 095h, 0c0h
    ; setne al                                  ; 0f 95 c0
    xor ah, ah                                ; 30 e4
    mov byte [bp-00eh], al                    ; 88 46 f2
    mov word [bp-02ah], 00200h                ; c7 46 d6 00 02
    mov ax, word [bp-00262h]                  ; 8b 86 9e fd
    mov word [bp-01eh], ax                    ; 89 46 e2
    mov ax, word [bp-0025eh]                  ; 8b 86 a2 fd
    mov word [bp-034h], ax                    ; 89 46 cc
    mov ax, word [bp-00258h]                  ; 8b 86 a8 fd
    mov word [bp-01ch], ax                    ; 89 46 e4
    mov ax, word [bp-001ech]                  ; 8b 86 14 fe
    mov word [bp-030h], ax                    ; 89 46 d0
    mov si, word [bp-001eah]                  ; 8b b6 16 fe
    xor ax, ax                                ; 31 c0
    mov word [bp-032h], ax                    ; 89 46 ce
    mov word [bp-022h], ax                    ; 89 46 de
    cmp si, 00fffh                            ; 81 fe ff 0f
    jne short 0245fh                          ; 75 1f
    cmp word [bp-030h], strict byte 0ffffh    ; 83 7e d0 ff
    jne short 0245fh                          ; 75 19
    mov ax, word [bp-00196h]                  ; 8b 86 6a fe
    mov word [bp-022h], ax                    ; 89 46 de
    mov ax, word [bp-00198h]                  ; 8b 86 68 fe
    mov word [bp-032h], ax                    ; 89 46 ce
    mov si, word [bp-0019ah]                  ; 8b b6 66 fe
    mov ax, word [bp-0019ch]                  ; 8b 86 64 fe
    mov word [bp-030h], ax                    ; 89 46 d0
    mov al, byte [bp-010h]                    ; 8a 46 f0
    cmp AL, strict byte 001h                  ; 3c 01
    jc short 02472h                           ; 72 0c
    jbe short 0247ah                          ; 76 12
    cmp AL, strict byte 003h                  ; 3c 03
    je short 02482h                           ; 74 16
    cmp AL, strict byte 002h                  ; 3c 02
    je short 0247eh                           ; 74 0e
    jmp short 024bfh                          ; eb 4d
    test al, al                               ; 84 c0
    jne short 024bfh                          ; 75 49
    mov BL, strict byte 01eh                  ; b3 1e
    jmp short 02484h                          ; eb 0a
    mov BL, strict byte 026h                  ; b3 26
    jmp short 02484h                          ; eb 06
    mov BL, strict byte 067h                  ; b3 67
    jmp short 02484h                          ; eb 02
    mov BL, strict byte 070h                  ; b3 70
    mov al, bl                                ; 88 d8
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    xor ah, ah                                ; 30 e4
    call 017a5h                               ; e8 18 f3
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    sal dx, 008h                              ; c1 e2 08
    movzx ax, bl                              ; 0f b6 c3
    call 017a5h                               ; e8 0b f3
    xor ah, ah                                ; 30 e4
    add ax, dx                                ; 01 d0
    mov word [bp-038h], ax                    ; 89 46 c8
    mov al, bl                                ; 88 d8
    add AL, strict byte 002h                  ; 04 02
    xor ah, ah                                ; 30 e4
    call 017a5h                               ; e8 fb f2
    xor ah, ah                                ; 30 e4
    mov word [bp-03ah], ax                    ; 89 46 c6
    mov al, bl                                ; 88 d8
    add AL, strict byte 007h                  ; 04 07
    xor ah, ah                                ; 30 e4
    call 017a5h                               ; e8 ed f2
    xor ah, ah                                ; 30 e4
    mov word [bp-036h], ax                    ; 89 46 ca
    jmp short 024d1h                          ; eb 12
    push word [bp-022h]                       ; ff 76 de
    push word [bp-032h]                       ; ff 76 ce
    push si                                   ; 56
    push word [bp-030h]                       ; ff 76 d0
    mov dx, ss                                ; 8c d2
    lea ax, [bp-03ah]                         ; 8d 46 c6
    call 05aeeh                               ; e8 1d 36
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 4e f5
    mov ax, word [bp-036h]                    ; 8b 46 ca
    push ax                                   ; 50
    mov ax, word [bp-03ah]                    ; 8b 46 c6
    push ax                                   ; 50
    mov ax, word [bp-038h]                    ; 8b 46 c8
    push ax                                   ; 50
    push word [bp-01ch]                       ; ff 76 e4
    push word [bp-034h]                       ; ff 76 cc
    push word [bp-01eh]                       ; ff 76 e2
    movzx ax, byte [bp-016h]                  ; 0f b6 46 ea
    push ax                                   ; 50
    movzx ax, byte [bp-006h]                  ; 0f b6 46 fa
    push ax                                   ; 50
    push 0015fh                               ; 68 5f 01
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 68 f5
    add sp, strict byte 00014h                ; 83 c4 14
    movzx ax, byte [bp-010h]                  ; 0f b6 46 f0
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov di, word [bp-028h]                    ; 8b 7e d8
    add di, ax                                ; 01 c7
    mov byte [es:di+023h], 0ffh               ; 26 c6 45 23 ff
    mov al, byte [bp-018h]                    ; 8a 46 e8
    mov byte [es:di+024h], al                 ; 26 88 45 24
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    mov byte [es:di+026h], al                 ; 26 88 45 26
    mov ax, word [bp-02ah]                    ; 8b 46 d6
    mov word [es:di+028h], ax                 ; 26 89 45 28
    mov ax, word [bp-034h]                    ; 8b 46 cc
    mov word [es:di+030h], ax                 ; 26 89 45 30
    mov ax, word [bp-01eh]                    ; 8b 46 e2
    mov word [es:di+032h], ax                 ; 26 89 45 32
    mov ax, word [bp-01ch]                    ; 8b 46 e4
    mov word [es:di+034h], ax                 ; 26 89 45 34
    mov ax, word [bp-022h]                    ; 8b 46 de
    mov word [es:di+03ch], ax                 ; 26 89 45 3c
    mov ax, word [bp-032h]                    ; 8b 46 ce
    mov word [es:di+03ah], ax                 ; 26 89 45 3a
    mov word [es:di+038h], si                 ; 26 89 75 38
    mov ax, word [bp-030h]                    ; 8b 46 d0
    mov word [es:di+036h], ax                 ; 26 89 45 36
    lea di, [di+02ah]                         ; 8d 7d 2a
    push DS                                   ; 1e
    push SS                                   ; 16
    pop DS                                    ; 1f
    lea si, [bp-03ah]                         ; 8d 76 c6
    movsw                                     ; a5
    movsw                                     ; a5
    movsw                                     ; a5
    pop DS                                    ; 1f
    mov al, byte [bp-010h]                    ; 8a 46 f0
    cmp AL, strict byte 002h                  ; 3c 02
    jnc near 025ech                           ; 0f 83 79 00
    test al, al                               ; 84 c0
    jne short 0257ch                          ; 75 05
    mov di, strict word 0003dh                ; bf 3d 00
    jmp short 0257fh                          ; eb 03
    mov di, strict word 0004dh                ; bf 4d 00
    mov dx, word [bp-024h]                    ; 8b 56 dc
    movzx bx, byte [bp-010h]                  ; 0f b6 5e f0
    imul bx, bx, strict byte 00005h           ; 6b db 05
    sal bx, 002h                              ; c1 e3 02
    add bx, 00104h                            ; 81 c3 04 01
    xor ax, ax                                ; 31 c0
    mov es, ax                                ; 8e c0
    mov word [es:bx], di                      ; 26 89 3f
    mov word [es:bx+002h], dx                 ; 26 89 57 02
    mov ax, word [bp-038h]                    ; 8b 46 c8
    mov es, dx                                ; 8e c2
    mov word [es:di], ax                      ; 26 89 05
    mov al, byte [bp-03ah]                    ; 8a 46 c6
    mov byte [es:di+002h], al                 ; 26 88 45 02
    mov byte [es:di+003h], 0a0h               ; 26 c6 45 03 a0
    mov al, byte [bp-01ch]                    ; 8a 46 e4
    mov byte [es:di+004h], al                 ; 26 88 45 04
    mov ax, word [bp-01eh]                    ; 8b 46 e2
    mov word [es:di+009h], ax                 ; 26 89 45 09
    mov al, byte [bp-034h]                    ; 8a 46 cc
    mov byte [es:di+00bh], al                 ; 26 88 45 0b
    mov al, byte [bp-036h]                    ; 8a 46 ca
    mov byte [es:di+00eh], al                 ; 26 88 45 0e
    xor al, al                                ; 30 c0
    xor ah, ah                                ; 30 e4
    jmp short 025d6h                          ; eb 05
    cmp ah, 00fh                              ; 80 fc 0f
    jnc short 025e4h                          ; 73 0e
    movzx bx, ah                              ; 0f b6 dc
    mov es, dx                                ; 8e c2
    add bx, di                                ; 01 fb
    add al, byte [es:bx]                      ; 26 02 07
    db  0feh, 0c4h
    ; inc ah                                    ; fe c4
    jmp short 025d1h                          ; eb ed
    neg al                                    ; f6 d8
    mov es, dx                                ; 8e c2
    mov byte [es:di+00fh], al                 ; 26 88 45 0f
    movzx bx, byte [bp-012h]                  ; 0f b6 5e ee
    mov es, [bp-01ah]                         ; 8e 46 e6
    add bx, word [bp-028h]                    ; 03 5e d8
    mov al, byte [bp-010h]                    ; 8a 46 f0
    mov byte [es:bx+001e3h], al               ; 26 88 87 e3 01
    inc byte [bp-012h]                        ; fe 46 ee
    cmp byte [bp-00ch], 003h                  ; 80 7e f4 03
    jne near 026a7h                           ; 0f 85 9e 00
    movzx ax, byte [bp-010h]                  ; 0f b6 46 f0
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov bx, word [bp-028h]                    ; 8b 5e d8
    add bx, ax                                ; 01 c3
    mov byte [es:bx+023h], 005h               ; 26 c6 47 23 05
    mov byte [es:bx+026h], 000h               ; 26 c6 47 26 00
    lea dx, [bp-00264h]                       ; 8d 96 9c fd
    mov bx, word [bp-028h]                    ; 8b 5e d8
    mov word [es:bx+008h], dx                 ; 26 89 57 08
    mov [es:bx+00ah], ss                      ; 26 8c 57 0a
    mov al, byte [bp-010h]                    ; 8a 46 f0
    mov byte [es:bx+00ch], al                 ; 26 88 47 0c
    mov cx, strict word 00001h                ; b9 01 00
    mov bx, 000a1h                            ; bb a1 00
    mov ax, word [bp-028h]                    ; 8b 46 d8
    mov dx, es                                ; 8c c2
    call 01f53h                               ; e8 0d f9
    test ax, ax                               ; 85 c0
    je short 02655h                           ; 74 0b
    push 00186h                               ; 68 86 01
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 19 f4
    add sp, strict byte 00004h                ; 83 c4 04
    mov dl, byte [bp-00263h]                  ; 8a 96 9d fd
    and dl, 01fh                              ; 80 e2 1f
    test byte [bp-00264h], 080h               ; f6 86 9c fd 80
    db  00fh, 095h, 0c0h
    ; setne al                                  ; 0f 95 c0
    xor ah, ah                                ; 30 e4
    mov cx, ax                                ; 89 c1
    cmp byte [bp-00204h], 000h                ; 80 be fc fd 00
    db  00fh, 095h, 0c0h
    ; setne al                                  ; 0f 95 c0
    xor ah, ah                                ; 30 e4
    mov bx, ax                                ; 89 c3
    movzx ax, byte [bp-010h]                  ; 0f b6 46 f0
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov si, word [bp-028h]                    ; 8b 76 d8
    add si, ax                                ; 01 c6
    mov byte [es:si+023h], dl                 ; 26 88 54 23
    mov byte [es:si+024h], cl                 ; 26 88 4c 24
    mov byte [es:si+026h], bl                 ; 26 88 5c 26
    mov word [es:si+028h], 00800h             ; 26 c7 44 28 00 08
    movzx bx, byte [bp-008h]                  ; 0f b6 5e f8
    add bx, word [bp-028h]                    ; 03 5e d8
    mov al, byte [bp-010h]                    ; 8a 46 f0
    mov byte [es:bx+001f4h], al               ; 26 88 87 f4 01
    inc byte [bp-008h]                        ; fe 46 f8
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    cmp AL, strict byte 003h                  ; 3c 03
    je short 026ddh                           ; 74 2f
    cmp AL, strict byte 002h                  ; 3c 02
    jne near 02740h                           ; 0f 85 8c 00
    movzx di, byte [bp-010h]                  ; 0f b6 7e f0
    imul di, di, strict byte 0001ch           ; 6b ff 1c
    mov es, [bp-01ah]                         ; 8e 46 e6
    add di, word [bp-028h]                    ; 03 7e d8
    mov ax, word [es:di+03ch]                 ; 26 8b 45 3c
    mov bx, word [es:di+03ah]                 ; 26 8b 5d 3a
    mov cx, word [es:di+038h]                 ; 26 8b 4d 38
    mov dx, word [es:di+036h]                 ; 26 8b 55 36
    mov si, strict word 0000bh                ; be 0b 00
    call 0a110h                               ; e8 39 7a
    mov word [bp-02ch], dx                    ; 89 56 d4
    mov word [bp-026h], cx                    ; 89 4e da
    movzx ax, byte [bp-001c3h]                ; 0f b6 86 3d fe
    sal ax, 008h                              ; c1 e0 08
    movzx dx, byte [bp-001c4h]                ; 0f b6 96 3c fe
    or dx, ax                                 ; 09 c2
    mov byte [bp-014h], 00fh                  ; c6 46 ec 0f
    jmp short 026fbh                          ; eb 09
    dec byte [bp-014h]                        ; fe 4e ec
    cmp byte [bp-014h], 000h                  ; 80 7e ec 00
    jbe short 02708h                          ; 76 0d
    movzx cx, byte [bp-014h]                  ; 0f b6 4e ec
    mov ax, strict word 00001h                ; b8 01 00
    sal ax, CL                                ; d3 e0
    test dx, ax                               ; 85 c2
    je short 026f2h                           ; 74 ea
    xor di, di                                ; 31 ff
    jmp short 02711h                          ; eb 05
    cmp di, strict byte 00014h                ; 83 ff 14
    jnl short 02726h                          ; 7d 15
    mov si, di                                ; 89 fe
    add si, di                                ; 01 fe
    mov al, byte [bp+si-0022dh]               ; 8a 82 d3 fd
    mov byte [bp+si-064h], al                 ; 88 42 9c
    mov al, byte [bp+si-0022eh]               ; 8a 82 d2 fd
    mov byte [bp+si-063h], al                 ; 88 42 9d
    inc di                                    ; 47
    jmp short 0270ch                          ; eb e6
    mov byte [bp-03ch], 000h                  ; c6 46 c4 00
    mov di, strict word 00027h                ; bf 27 00
    jmp short 02734h                          ; eb 05
    dec di                                    ; 4f
    test di, di                               ; 85 ff
    jle short 02740h                          ; 7e 0c
    cmp byte [bp+di-064h], 020h               ; 80 7b 9c 20
    jne short 02740h                          ; 75 06
    mov byte [bp+di-064h], 000h               ; c6 43 9c 00
    jmp short 0272fh                          ; eb ef
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    cmp AL, strict byte 003h                  ; 3c 03
    je short 027a5h                           ; 74 5e
    cmp AL, strict byte 002h                  ; 3c 02
    je short 02754h                           ; 74 09
    cmp AL, strict byte 001h                  ; 3c 01
    je near 0280dh                            ; 0f 84 bc 00
    jmp near 0282ch                           ; e9 d8 00
    cmp byte [bp-016h], 000h                  ; 80 7e ea 00
    je short 0275fh                           ; 74 05
    mov ax, 001b1h                            ; b8 b1 01
    jmp short 02762h                          ; eb 03
    mov ax, 001b8h                            ; b8 b8 01
    push ax                                   ; 50
    movzx ax, byte [bp-006h]                  ; 0f b6 46 fa
    push ax                                   ; 50
    push 001bfh                               ; 68 bf 01
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 fb f2
    add sp, strict byte 00008h                ; 83 c4 08
    xor di, di                                ; 31 ff
    movzx ax, byte [bp+di-064h]               ; 0f b6 43 9c
    inc di                                    ; 47
    test ax, ax                               ; 85 c0
    je short 0278ch                           ; 74 0e
    push ax                                   ; 50
    push 001cah                               ; 68 ca 01
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 e4 f2
    add sp, strict byte 00006h                ; 83 c4 06
    jmp short 02775h                          ; eb e9
    push word [bp-026h]                       ; ff 76 da
    push word [bp-02ch]                       ; ff 76 d4
    movzx ax, byte [bp-014h]                  ; 0f b6 46 ec
    push ax                                   ; 50
    push 001cdh                               ; 68 cd 01
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 cc f2
    add sp, strict byte 0000ah                ; 83 c4 0a
    jmp near 0282ch                           ; e9 87 00
    cmp byte [bp-016h], 000h                  ; 80 7e ea 00
    je short 027b0h                           ; 74 05
    mov ax, 001b1h                            ; b8 b1 01
    jmp short 027b3h                          ; eb 03
    mov ax, 001b8h                            ; b8 b8 01
    push ax                                   ; 50
    movzx ax, byte [bp-006h]                  ; 0f b6 46 fa
    push ax                                   ; 50
    push 001bfh                               ; 68 bf 01
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 aa f2
    add sp, strict byte 00008h                ; 83 c4 08
    xor di, di                                ; 31 ff
    movzx ax, byte [bp+di-064h]               ; 0f b6 43 9c
    inc di                                    ; 47
    test ax, ax                               ; 85 c0
    je short 027ddh                           ; 74 0e
    push ax                                   ; 50
    push 001cah                               ; 68 ca 01
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 93 f2
    add sp, strict byte 00006h                ; 83 c4 06
    jmp short 027c6h                          ; eb e9
    movzx bx, byte [bp-010h]                  ; 0f b6 5e f0
    imul bx, bx, strict byte 0001ch           ; 6b db 1c
    mov es, [bp-01ah]                         ; 8e 46 e6
    add bx, word [bp-028h]                    ; 03 5e d8
    cmp byte [es:bx+023h], 005h               ; 26 80 7f 23 05
    jne short 027fbh                          ; 75 0a
    movzx ax, byte [bp-014h]                  ; 0f b6 46 ec
    push ax                                   ; 50
    push 001edh                               ; 68 ed 01
    jmp short 02803h                          ; eb 08
    movzx ax, byte [bp-014h]                  ; 0f b6 46 ec
    push ax                                   ; 50
    push 00207h                               ; 68 07 02
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 63 f2
    add sp, strict byte 00006h                ; 83 c4 06
    jmp short 0282ch                          ; eb 1f
    cmp byte [bp-016h], 000h                  ; 80 7e ea 00
    je short 02818h                           ; 74 05
    mov ax, 001b1h                            ; b8 b1 01
    jmp short 0281bh                          ; eb 03
    mov ax, 001b8h                            ; b8 b8 01
    push ax                                   ; 50
    movzx ax, byte [bp-006h]                  ; 0f b6 46 fa
    push ax                                   ; 50
    push 00219h                               ; 68 19 02
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 42 f2
    add sp, strict byte 00008h                ; 83 c4 08
    inc byte [bp-010h]                        ; fe 46 f0
    cmp byte [bp-010h], 008h                  ; 80 7e f0 08
    jnc short 02885h                          ; 73 50
    movzx bx, byte [bp-010h]                  ; 0f b6 5e f0
    mov ax, bx                                ; 89 d8
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    mov cx, ax                                ; 89 c1
    mov byte [bp-006h], al                    ; 88 46 fa
    mov ax, bx                                ; 89 d8
    cwd                                       ; 99
    mov bx, strict word 00002h                ; bb 02 00
    idiv bx                                   ; f7 fb
    mov word [bp-02eh], dx                    ; 89 56 d2
    mov al, byte [bp-02eh]                    ; 8a 46 d2
    mov byte [bp-016h], al                    ; 88 46 ea
    movzx bx, cl                              ; 0f b6 d9
    imul bx, bx, strict byte 00006h           ; 6b db 06
    mov es, [bp-01ah]                         ; 8e 46 e6
    add bx, word [bp-028h]                    ; 03 5e d8
    mov si, word [es:bx+00206h]               ; 26 8b b7 06 02
    mov ax, word [es:bx+00208h]               ; 26 8b 87 08 02
    mov word [bp-020h], ax                    ; 89 46 e0
    mov dx, ax                                ; 89 c2
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 00ah                  ; b0 0a
    out DX, AL                                ; ee
    cmp byte [bp-02eh], 000h                  ; 80 7e d2 00
    je near 0228fh                            ; 0f 84 10 fa
    mov ax, 000b0h                            ; b8 b0 00
    jmp near 02292h                           ; e9 0d fa
    mov al, byte [bp-012h]                    ; 8a 46 ee
    mov es, [bp-01ah]                         ; 8e 46 e6
    mov bx, word [bp-028h]                    ; 8b 5e d8
    mov byte [es:bx+001e2h], al               ; 26 88 87 e2 01
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov byte [es:bx+001f3h], al               ; 26 88 87 f3 01
    movzx bx, byte [bp-012h]                  ; 0f b6 5e ee
    mov dx, strict word 00075h                ; ba 75 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 af ee
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
ata_cmd_data_out_:                           ; 0xf28af LB 0x2bc
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00022h                ; 83 ec 22
    mov di, ax                                ; 89 c7
    mov word [bp-00ch], dx                    ; 89 56 f4
    mov word [bp-024h], bx                    ; 89 5e dc
    mov word [bp-01ah], cx                    ; 89 4e e6
    mov es, dx                                ; 8e c2
    movzx ax, byte [es:di+00ch]               ; 26 0f b6 45 0c
    mov dx, ax                                ; 89 c2
    shr dx, 1                                 ; d1 ea
    mov dh, al                                ; 88 c6
    and dh, 001h                              ; 80 e6 01
    mov byte [bp-006h], dh                    ; 88 76 fa
    xor dh, dh                                ; 30 f6
    imul dx, dx, strict byte 00006h           ; 6b d2 06
    mov bx, di                                ; 89 fb
    add bx, dx                                ; 01 d3
    mov dx, word [es:bx+00206h]               ; 26 8b 97 06 02
    mov word [bp-00ah], dx                    ; 89 56 f6
    mov dx, word [es:bx+00208h]               ; 26 8b 97 08 02
    mov word [bp-012h], dx                    ; 89 56 ee
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov bx, di                                ; 89 fb
    add bx, ax                                ; 01 c3
    mov al, byte [es:bx+026h]                 ; 26 8a 47 26
    mov byte [bp-008h], al                    ; 88 46 f8
    cmp AL, strict byte 001h                  ; 3c 01
    jne short 02907h                          ; 75 07
    mov word [bp-020h], 00080h                ; c7 46 e0 80 00
    jmp short 0290ch                          ; eb 05
    mov word [bp-020h], 00100h                ; c7 46 e0 00 01
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 080h                 ; a8 80
    je short 02928h                           ; 74 0f
    mov dx, word [bp-012h]                    ; 8b 56 ee
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov dx, strict word 00001h                ; ba 01 00
    jmp near 02b62h                           ; e9 3a 02
    mov es, [bp-00ch]                         ; 8e 46 f4
    mov ax, word [es:di+006h]                 ; 26 8b 45 06
    mov word [bp-010h], ax                    ; 89 46 f0
    mov ax, word [es:di+004h]                 ; 26 8b 45 04
    mov word [bp-014h], ax                    ; 89 46 ec
    mov ax, word [es:di+002h]                 ; 26 8b 45 02
    mov word [bp-016h], ax                    ; 89 46 ea
    mov ax, word [es:di]                      ; 26 8b 05
    mov word [bp-01eh], ax                    ; 89 46 e2
    mov ax, word [es:di+008h]                 ; 26 8b 45 08
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov ax, word [es:di+00ah]                 ; 26 8b 45 0a
    mov word [bp-018h], ax                    ; 89 46 e8
    mov ax, word [es:di+016h]                 ; 26 8b 45 16
    mov word [bp-01ch], ax                    ; 89 46 e4
    mov ax, word [es:di+012h]                 ; 26 8b 45 12
    mov word [bp-026h], ax                    ; 89 46 da
    mov ax, word [es:di+014h]                 ; 26 8b 45 14
    mov word [bp-022h], ax                    ; 89 46 de
    mov ax, word [bp-01ch]                    ; 8b 46 e4
    test ax, ax                               ; 85 c0
    jne near 02a39h                           ; 0f 85 c7 00
    xor dx, dx                                ; 31 d2
    xor bx, bx                                ; 31 db
    mov si, word [bp-01eh]                    ; 8b 76 e2
    add si, word [bp-01ah]                    ; 03 76 e6
    adc dx, word [bp-016h]                    ; 13 56 ea
    adc bx, word [bp-014h]                    ; 13 5e ec
    adc ax, word [bp-010h]                    ; 13 46 f0
    test ax, ax                               ; 85 c0
    jnbe short 02999h                         ; 77 10
    jne short 029fch                          ; 75 71
    test bx, bx                               ; 85 db
    jnbe short 02999h                         ; 77 0a
    jne short 029fch                          ; 75 6b
    cmp dx, 01000h                            ; 81 fa 00 10
    jnbe short 02999h                         ; 77 02
    jne short 029fch                          ; 75 63
    mov ax, word [bp-010h]                    ; 8b 46 f0
    mov bx, word [bp-014h]                    ; 8b 5e ec
    mov cx, word [bp-016h]                    ; 8b 4e ea
    mov dx, word [bp-01eh]                    ; 8b 56 e2
    mov si, strict word 00018h                ; be 18 00
    call 0a110h                               ; e8 65 77
    xor dh, dh                                ; 30 f6
    mov word [bp-01ch], dx                    ; 89 56 e4
    mov ax, word [bp-010h]                    ; 8b 46 f0
    mov bx, word [bp-014h]                    ; 8b 5e ec
    mov cx, word [bp-016h]                    ; 8b 4e ea
    mov dx, word [bp-01eh]                    ; 8b 56 e2
    mov si, strict word 00020h                ; be 20 00
    call 0a110h                               ; e8 4e 77
    mov bx, dx                                ; 89 d3
    mov ax, word [bp-01ah]                    ; 8b 46 e6
    xor al, al                                ; 30 c0
    shr ax, 008h                              ; c1 e8 08
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    inc dx                                    ; 42
    inc dx                                    ; 42
    out DX, AL                                ; ee
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00003h                ; 83 c2 03
    mov al, byte [bp-01ch]                    ; 8a 46 e4
    out DX, AL                                ; ee
    mov ax, bx                                ; 89 d8
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00004h                ; 83 c2 04
    out DX, AL                                ; ee
    shr ax, 008h                              ; c1 e8 08
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00005h                ; 83 c2 05
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    mov byte [bp-015h], al                    ; 88 46 eb
    xor ah, ah                                ; 30 e4
    mov word [bp-014h], ax                    ; 89 46 ec
    mov word [bp-010h], ax                    ; 89 46 f0
    mov ax, word [bp-01eh]                    ; 8b 46 e2
    xor ah, ah                                ; 30 e4
    mov word [bp-01ch], ax                    ; 89 46 e4
    mov ax, word [bp-010h]                    ; 8b 46 f0
    mov bx, word [bp-014h]                    ; 8b 5e ec
    mov cx, word [bp-016h]                    ; 8b 4e ea
    mov dx, word [bp-01eh]                    ; 8b 56 e2
    mov si, strict word 00008h                ; be 08 00
    call 0a110h                               ; e8 fa 76
    mov word [bp-010h], ax                    ; 89 46 f0
    mov word [bp-014h], bx                    ; 89 5e ec
    mov word [bp-016h], cx                    ; 89 4e ea
    mov word [bp-01eh], dx                    ; 89 56 e2
    mov word [bp-026h], dx                    ; 89 56 da
    mov si, strict word 00010h                ; be 10 00
    call 0a110h                               ; e8 e5 76
    mov word [bp-01eh], dx                    ; 89 56 e2
    mov ax, dx                                ; 89 d0
    xor ah, dh                                ; 30 f4
    and AL, strict byte 00fh                  ; 24 0f
    or AL, strict byte 040h                   ; 0c 40
    mov word [bp-022h], ax                    ; 89 46 de
    mov dx, word [bp-012h]                    ; 8b 56 ee
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
    mov al, byte [bp-01ah]                    ; 8a 46 e6
    out DX, AL                                ; ee
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00003h                ; 83 c2 03
    mov al, byte [bp-01ch]                    ; 8a 46 e4
    out DX, AL                                ; ee
    mov ax, word [bp-026h]                    ; 8b 46 da
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00004h                ; 83 c2 04
    out DX, AL                                ; ee
    shr ax, 008h                              ; c1 e8 08
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00005h                ; 83 c2 05
    out DX, AL                                ; ee
    cmp byte [bp-006h], 000h                  ; 80 7e fa 00
    je short 02a7bh                           ; 74 05
    mov ax, 000b0h                            ; b8 b0 00
    jmp short 02a7eh                          ; eb 03
    mov ax, 000a0h                            ; b8 a0 00
    movzx dx, byte [bp-022h]                  ; 0f b6 56 de
    or ax, dx                                 ; 09 d0
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00006h                ; 83 c2 06
    out DX, AL                                ; ee
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00007h                ; 83 c2 07
    mov al, byte [bp-024h]                    ; 8a 46 dc
    out DX, AL                                ; ee
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov dl, al                                ; 88 c2
    test AL, strict byte 080h                 ; a8 80
    jne short 02a95h                          ; 75 f1
    test AL, strict byte 001h                 ; a8 01
    je short 02ab7h                           ; 74 0f
    mov dx, word [bp-012h]                    ; 8b 56 ee
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov dx, strict word 00002h                ; ba 02 00
    jmp near 02b62h                           ; e9 ab 00
    test dl, 008h                             ; f6 c2 08
    jne short 02acbh                          ; 75 0f
    mov dx, word [bp-012h]                    ; 8b 56 ee
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov dx, strict word 00003h                ; ba 03 00
    jmp near 02b62h                           ; e9 97 00
    sti                                       ; fb
    mov ax, word [bp-00eh]                    ; 8b 46 f2
    cmp ax, 0f800h                            ; 3d 00 f8
    jc short 02ae4h                           ; 72 10
    sub ax, 00800h                            ; 2d 00 08
    mov dx, word [bp-018h]                    ; 8b 56 e8
    add dx, 00080h                            ; 81 c2 80 00
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov word [bp-018h], dx                    ; 89 56 e8
    cmp byte [bp-008h], 001h                  ; 80 7e f8 01
    jne short 02afch                          ; 75 12
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    mov cx, word [bp-020h]                    ; 8b 4e e0
    mov si, word [bp-00eh]                    ; 8b 76 f2
    mov es, [bp-018h]                         ; 8e 46 e8
    db  0f3h, 066h, 026h, 06fh
    ; rep es outsd                              ; f3 66 26 6f
    jmp short 02b0bh                          ; eb 0f
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    mov cx, word [bp-020h]                    ; 8b 4e e0
    mov si, word [bp-00eh]                    ; 8b 76 f2
    mov es, [bp-018h]                         ; 8e 46 e8
    db  0f3h, 026h, 06fh
    ; rep es outsw                              ; f3 26 6f
    mov word [bp-00eh], si                    ; 89 76 f2
    mov es, [bp-00ch]                         ; 8e 46 f4
    inc word [es:di+018h]                     ; 26 ff 45 18
    dec word [bp-01ah]                        ; ff 4e e6
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov dl, al                                ; 88 c2
    test AL, strict byte 080h                 ; a8 80
    jne short 02b18h                          ; 75 f1
    cmp word [bp-01ah], strict byte 00000h    ; 83 7e e6 00
    jne short 02b41h                          ; 75 14
    and AL, strict byte 0e9h                  ; 24 e9
    cmp AL, strict byte 040h                  ; 3c 40
    je short 02b57h                           ; 74 24
    mov dx, word [bp-012h]                    ; 8b 56 ee
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov dx, strict word 00006h                ; ba 06 00
    jmp short 02b62h                          ; eb 21
    mov al, dl                                ; 88 d0
    and AL, strict byte 0c9h                  ; 24 c9
    cmp AL, strict byte 048h                  ; 3c 48
    je short 02acch                           ; 74 83
    mov dx, word [bp-012h]                    ; 8b 56 ee
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov dx, strict word 00007h                ; ba 07 00
    jmp short 02b62h                          ; eb 0b
    mov dx, word [bp-012h]                    ; 8b 56 ee
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    xor dx, dx                                ; 31 d2
    mov ax, dx                                ; 89 d0
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
@ata_read_sectors:                           ; 0xf2b6b LB 0xb5
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00008h                ; 83 ec 08
    mov si, word [bp+004h]                    ; 8b 76 04
    mov es, [bp+006h]                         ; 8e 46 06
    mov al, byte [es:si+00ch]                 ; 26 8a 44 0c
    mov cx, word [es:si+00eh]                 ; 26 8b 4c 0e
    mov dx, cx                                ; 89 ca
    sal dx, 009h                              ; c1 e2 09
    cmp word [es:si+016h], strict byte 00000h ; 26 83 7c 16 00
    je short 02bach                           ; 74 1f
    xor ah, ah                                ; 30 e4
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov [bp-00ah], es                         ; 8c 46 f6
    mov di, si                                ; 89 f7
    add di, ax                                ; 01 c7
    mov word [es:di+028h], dx                 ; 26 89 55 28
    mov bx, 000c4h                            ; bb c4 00
    mov ax, si                                ; 89 f0
    mov dx, es                                ; 8c c2
    call 01f53h                               ; e8 ac f3
    mov es, [bp-00ah]                         ; 8e 46 f6
    jmp short 02c11h                          ; eb 65
    xor bx, bx                                ; 31 db
    mov word [bp-00ah], bx                    ; 89 5e f6
    mov word [bp-00ch], bx                    ; 89 5e f4
    mov di, word [es:si]                      ; 26 8b 3c
    add di, cx                                ; 01 cf
    mov word [bp-008h], di                    ; 89 7e f8
    mov di, word [es:si+002h]                 ; 26 8b 7c 02
    adc di, bx                                ; 11 df
    mov word [bp-006h], di                    ; 89 7e fa
    mov bx, word [es:si+004h]                 ; 26 8b 5c 04
    adc bx, word [bp-00ah]                    ; 13 5e f6
    mov di, word [es:si+006h]                 ; 26 8b 7c 06
    adc di, word [bp-00ch]                    ; 13 7e f4
    test di, di                               ; 85 ff
    jnbe short 02be8h                         ; 77 11
    jne short 02bf4h                          ; 75 1b
    test bx, bx                               ; 85 db
    jnbe short 02be8h                         ; 77 0b
    jne short 02bf4h                          ; 75 15
    cmp word [bp-006h], 01000h                ; 81 7e fa 00 10
    jnbe short 02be8h                         ; 77 02
    jne short 02bf4h                          ; 75 0c
    mov bx, strict word 00024h                ; bb 24 00
    mov ax, si                                ; 89 f0
    mov dx, es                                ; 8c c2
    call 01f53h                               ; e8 61 f3
    jmp short 02c17h                          ; eb 23
    xor ah, ah                                ; 30 e4
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov [bp-006h], es                         ; 8c 46 fa
    mov di, si                                ; 89 f7
    add di, ax                                ; 01 c7
    mov word [es:di+028h], dx                 ; 26 89 55 28
    mov bx, 000c4h                            ; bb c4 00
    mov ax, si                                ; 89 f0
    mov dx, es                                ; 8c c2
    call 01f53h                               ; e8 45 f3
    mov es, [bp-006h]                         ; 8e 46 fa
    mov word [es:di+028h], 00200h             ; 26 c7 45 28 00 02
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 00004h                               ; c2 04 00
@ata_write_sectors:                          ; 0xf2c20 LB 0x5b
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    les si, [bp+004h]                         ; c4 76 04
    mov cx, word [es:si+00eh]                 ; 26 8b 4c 0e
    cmp word [es:si+016h], strict byte 00000h ; 26 83 7c 16 00
    je short 02c40h                           ; 74 0c
    mov bx, strict word 00030h                ; bb 30 00
    mov ax, si                                ; 89 f0
    mov dx, es                                ; 8c c2
    call 028afh                               ; e8 71 fc
    jmp short 02c72h                          ; eb 32
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
    jnbe short 02c6dh                         ; 77 0f
    jne short 02c34h                          ; 75 d4
    test bx, bx                               ; 85 db
    jnbe short 02c6dh                         ; 77 09
    jne short 02c34h                          ; 75 ce
    cmp ax, 01000h                            ; 3d 00 10
    jnbe short 02c6dh                         ; 77 02
    jne short 02c34h                          ; 75 c7
    mov bx, strict word 00034h                ; bb 34 00
    jmp short 02c37h                          ; eb c5
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 00004h                               ; c2 04 00
ata_cmd_packet_:                             ; 0xf2c7b LB 0x2e8
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00014h                ; 83 ec 14
    push ax                                   ; 50
    mov byte [bp-008h], dl                    ; 88 56 f8
    mov di, bx                                ; 89 df
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 d3 ea
    mov word [bp-012h], 00122h                ; c7 46 ee 22 01
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov ax, word [bp-01ah]                    ; 8b 46 e6
    shr ax, 1                                 ; d1 e8
    mov ah, byte [bp-01ah]                    ; 8a 66 e6
    and ah, 001h                              ; 80 e4 01
    mov byte [bp-006h], ah                    ; 88 66 fa
    cmp byte [bp+00ah], 002h                  ; 80 7e 0a 02
    jne short 02ccdh                          ; 75 1f
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 71 ed
    push 00233h                               ; 68 33 02
    push 00242h                               ; 68 42 02
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 a7 ed
    add sp, strict byte 00006h                ; 83 c4 06
    mov dx, strict word 00001h                ; ba 01 00
    jmp near 02f58h                           ; e9 8b 02
    test byte [bp+004h], 001h                 ; f6 46 04 01
    jne short 02cc7h                          ; 75 f4
    xor ah, ah                                ; 30 e4
    imul ax, ax, strict byte 00006h           ; 6b c0 06
    mov es, [bp-00eh]                         ; 8e 46 f2
    mov si, word [bp-012h]                    ; 8b 76 ee
    add si, ax                                ; 01 c6
    mov bx, word [es:si+00206h]               ; 26 8b 9c 06 02
    mov ax, word [es:si+00208h]               ; 26 8b 84 08 02
    mov word [bp-010h], ax                    ; 89 46 f0
    imul si, word [bp-01ah], strict byte 0001ch ; 6b 76 e6 1c
    add si, word [bp-012h]                    ; 03 76 ee
    mov al, byte [es:si+026h]                 ; 26 8a 44 26
    mov byte [bp-00ah], al                    ; 88 46 f6
    xor ax, ax                                ; 31 c0
    mov word [bp-018h], ax                    ; 89 46 e8
    mov word [bp-016h], ax                    ; 89 46 ea
    mov al, byte [bp-008h]                    ; 8a 46 f8
    cmp AL, strict byte 00ch                  ; 3c 0c
    jnc short 02d10h                          ; 73 06
    mov byte [bp-008h], 00ch                  ; c6 46 f8 0c
    jmp short 02d16h                          ; eb 06
    jbe short 02d16h                          ; 76 04
    mov byte [bp-008h], 010h                  ; c6 46 f8 10
    shr byte [bp-008h], 1                     ; d0 6e f8
    mov es, [bp-00eh]                         ; 8e 46 f2
    mov si, word [bp-012h]                    ; 8b 76 ee
    db  066h, 026h, 0c7h, 044h, 018h, 000h, 000h, 000h, 000h
    ; mov dword [es:si+018h], strict dword 000000000h ; 66 26 c7 44 18 00 00 00 00
    mov word [es:si+01ch], strict word 00000h ; 26 c7 44 1c 00 00
    lea dx, [bx+007h]                         ; 8d 57 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 080h                 ; a8 80
    je short 02d3eh                           ; 74 06
    mov dx, strict word 00002h                ; ba 02 00
    jmp near 02f58h                           ; e9 1a 02
    mov dx, word [bp-010h]                    ; 8b 56 f0
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 00ah                  ; b0 0a
    out DX, AL                                ; ee
    lea dx, [bx+004h]                         ; 8d 57 04
    mov AL, strict byte 0f0h                  ; b0 f0
    out DX, AL                                ; ee
    lea dx, [bx+005h]                         ; 8d 57 05
    mov AL, strict byte 0ffh                  ; b0 ff
    out DX, AL                                ; ee
    cmp byte [bp-006h], 000h                  ; 80 7e fa 00
    je short 02d5eh                           ; 74 05
    mov ax, 000b0h                            ; b8 b0 00
    jmp short 02d61h                          ; eb 03
    mov ax, 000a0h                            ; b8 a0 00
    lea dx, [bx+006h]                         ; 8d 57 06
    out DX, AL                                ; ee
    lea dx, [bx+007h]                         ; 8d 57 07
    mov AL, strict byte 0a0h                  ; b0 a0
    out DX, AL                                ; ee
    lea dx, [bx+007h]                         ; 8d 57 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov dl, al                                ; 88 c2
    test AL, strict byte 080h                 ; a8 80
    jne short 02d6bh                          ; 75 f4
    test AL, strict byte 001h                 ; a8 01
    je short 02d8ah                           ; 74 0f
    mov dx, word [bp-010h]                    ; 8b 56 f0
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov dx, strict word 00003h                ; ba 03 00
    jmp near 02f58h                           ; e9 ce 01
    test dl, 008h                             ; f6 c2 08
    jne short 02d9eh                          ; 75 0f
    mov dx, word [bp-010h]                    ; 8b 56 f0
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov dx, strict word 00004h                ; ba 04 00
    jmp near 02f58h                           ; e9 ba 01
    sti                                       ; fb
    mov ax, di                                ; 89 f8
    shr ax, 004h                              ; c1 e8 04
    add ax, cx                                ; 01 c8
    mov si, di                                ; 89 fe
    and si, strict byte 0000fh                ; 83 e6 0f
    movzx cx, byte [bp-008h]                  ; 0f b6 4e f8
    mov dx, bx                                ; 89 da
    mov es, ax                                ; 8e c0
    db  0f3h, 026h, 06fh
    ; rep es outsw                              ; f3 26 6f
    cmp byte [bp+00ah], 000h                  ; 80 7e 0a 00
    jne short 02dc7h                          ; 75 0b
    lea dx, [bx+007h]                         ; 8d 57 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov dl, al                                ; 88 c2
    jmp near 02f39h                           ; e9 72 01
    lea dx, [bx+007h]                         ; 8d 57 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov dl, al                                ; 88 c2
    test AL, strict byte 080h                 ; a8 80
    jne short 02dc7h                          ; 75 f4
    test AL, strict byte 088h                 ; a8 88
    je near 02f39h                            ; 0f 84 60 01
    test AL, strict byte 001h                 ; a8 01
    je short 02de8h                           ; 74 0b
    mov dx, word [bp-010h]                    ; 8b 56 f0
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    jmp short 02d84h                          ; eb 9c
    mov al, dl                                ; 88 d0
    and AL, strict byte 0c9h                  ; 24 c9
    cmp AL, strict byte 048h                  ; 3c 48
    je short 02dfbh                           ; 74 0b
    mov dx, word [bp-010h]                    ; 8b 56 f0
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    jmp short 02d98h                          ; eb 9d
    mov ax, word [bp+00ch]                    ; 8b 46 0c
    shr ax, 004h                              ; c1 e8 04
    mov dx, word [bp+00eh]                    ; 8b 56 0e
    add dx, ax                                ; 01 c2
    mov ax, word [bp+00ch]                    ; 8b 46 0c
    and ax, strict word 0000fh                ; 25 0f 00
    mov word [bp+00ch], ax                    ; 89 46 0c
    mov word [bp+00eh], dx                    ; 89 56 0e
    lea dx, [bx+005h]                         ; 8d 57 05
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov cx, ax                                ; 89 c1
    sal cx, 008h                              ; c1 e1 08
    lea dx, [bx+004h]                         ; 8d 57 04
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    add cx, ax                                ; 01 c1
    mov word [bp-014h], cx                    ; 89 4e ec
    mov ax, word [bp+004h]                    ; 8b 46 04
    cmp ax, cx                                ; 39 c8
    jbe short 02e3bh                          ; 76 0c
    mov ax, cx                                ; 89 c8
    sub word [bp+004h], cx                    ; 29 4e 04
    xor ax, cx                                ; 31 c8
    mov word [bp-014h], ax                    ; 89 46 ec
    jmp short 02e45h                          ; eb 0a
    mov cx, ax                                ; 89 c1
    mov word [bp+004h], strict word 00000h    ; c7 46 04 00 00
    sub word [bp-014h], ax                    ; 29 46 ec
    xor ax, ax                                ; 31 c0
    cmp word [bp+008h], strict byte 00000h    ; 83 7e 08 00
    jne short 02e6eh                          ; 75 21
    mov dx, word [bp-014h]                    ; 8b 56 ec
    cmp dx, word [bp+006h]                    ; 3b 56 06
    jbe short 02e6eh                          ; 76 19
    mov ax, word [bp-014h]                    ; 8b 46 ec
    sub ax, word [bp+006h]                    ; 2b 46 06
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov ax, word [bp+006h]                    ; 8b 46 06
    mov word [bp-014h], ax                    ; 89 46 ec
    xor ax, ax                                ; 31 c0
    mov word [bp+006h], ax                    ; 89 46 06
    mov word [bp+008h], ax                    ; 89 46 08
    jmp short 02e7ah                          ; eb 0c
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov dx, word [bp-014h]                    ; 8b 56 ec
    sub word [bp+006h], dx                    ; 29 56 06
    sbb word [bp+008h], ax                    ; 19 46 08
    mov si, word [bp-014h]                    ; 8b 76 ec
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    test cl, 003h                             ; f6 c1 03
    je short 02e87h                           ; 74 02
    xor al, al                                ; 30 c0
    test byte [bp-014h], 003h                 ; f6 46 ec 03
    je short 02e8fh                           ; 74 02
    xor al, al                                ; 30 c0
    test byte [bp-00ch], 003h                 ; f6 46 f4 03
    je short 02e97h                           ; 74 02
    xor al, al                                ; 30 c0
    test byte [bp-014h], 001h                 ; f6 46 ec 01
    je short 02eafh                           ; 74 12
    inc word [bp-014h]                        ; ff 46 ec
    cmp word [bp-00ch], strict byte 00000h    ; 83 7e f4 00
    jbe short 02eafh                          ; 76 09
    test byte [bp-00ch], 001h                 ; f6 46 f4 01
    je short 02eafh                           ; 74 03
    dec word [bp-00ch]                        ; ff 4e f4
    cmp AL, strict byte 001h                  ; 3c 01
    jne short 02ec0h                          ; 75 0d
    shr word [bp-014h], 002h                  ; c1 6e ec 02
    shr cx, 002h                              ; c1 e9 02
    shr word [bp-00ch], 002h                  ; c1 6e f4 02
    jmp short 02ec8h                          ; eb 08
    shr word [bp-014h], 1                     ; d1 6e ec
    shr cx, 1                                 ; d1 e9
    shr word [bp-00ch], 1                     ; d1 6e f4
    cmp AL, strict byte 001h                  ; 3c 01
    jne short 02ef8h                          ; 75 2c
    test cx, cx                               ; 85 c9
    je short 02edah                           ; 74 0a
    mov dx, bx                                ; 89 da
    push eax                                  ; 66 50
    in eax, DX                                ; 66 ed
    loop 02ed4h                               ; e2 fc
    pop eax                                   ; 66 58
    mov dx, bx                                ; 89 da
    mov cx, word [bp-014h]                    ; 8b 4e ec
    les di, [bp+00ch]                         ; c4 7e 0c
    db  0f3h, 066h, 06dh
    ; rep insd                                  ; f3 66 6d
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    test ax, ax                               ; 85 c0
    je short 02f17h                           ; 74 2b
    mov cx, ax                                ; 89 c1
    push eax                                  ; 66 50
    in eax, DX                                ; 66 ed
    loop 02ef0h                               ; e2 fc
    pop eax                                   ; 66 58
    jmp short 02f17h                          ; eb 1f
    test cx, cx                               ; 85 c9
    je short 02f01h                           ; 74 05
    mov dx, bx                                ; 89 da
    in ax, DX                                 ; ed
    loop 02efeh                               ; e2 fd
    mov dx, bx                                ; 89 da
    mov cx, word [bp-014h]                    ; 8b 4e ec
    les di, [bp+00ch]                         ; c4 7e 0c
    rep insw                                  ; f3 6d
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    test ax, ax                               ; 85 c0
    je short 02f17h                           ; 74 05
    mov cx, ax                                ; 89 c1
    in ax, DX                                 ; ed
    loop 02f14h                               ; e2 fd
    add word [bp+00ch], si                    ; 01 76 0c
    xor ax, ax                                ; 31 c0
    add word [bp-018h], si                    ; 01 76 e8
    adc word [bp-016h], ax                    ; 11 46 ea
    mov ax, word [bp-018h]                    ; 8b 46 e8
    mov es, [bp-00eh]                         ; 8e 46 f2
    mov si, word [bp-012h]                    ; 8b 76 ee
    mov word [es:si+01ah], ax                 ; 26 89 44 1a
    mov ax, word [bp-016h]                    ; 8b 46 ea
    mov word [es:si+01ch], ax                 ; 26 89 44 1c
    jmp near 02dc7h                           ; e9 8e fe
    mov al, dl                                ; 88 d0
    and AL, strict byte 0e9h                  ; 24 e9
    cmp AL, strict byte 040h                  ; 3c 40
    je short 02f4dh                           ; 74 0c
    mov dx, word [bp-010h]                    ; 8b 56 f0
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    jmp near 02d98h                           ; e9 4b fe
    mov dx, word [bp-010h]                    ; 8b 56 f0
    add dx, strict byte 00006h                ; 83 c2 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    xor dx, dx                                ; 31 d2
    mov ax, dx                                ; 89 d0
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 0000ch                               ; c2 0c 00
ata_soft_reset_:                             ; 0xf2f63 LB 0x80
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push ax                                   ; 50
    mov bx, ax                                ; 89 c3
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 f0 e7
    mov dx, bx                                ; 89 da
    shr dx, 1                                 ; d1 ea
    and bl, 001h                              ; 80 e3 01
    mov byte [bp-008h], bl                    ; 88 5e f8
    xor dh, dh                                ; 30 f6
    imul bx, dx, strict byte 00006h           ; 6b da 06
    mov es, ax                                ; 8e c0
    add bx, 00122h                            ; 81 c3 22 01
    mov cx, word [es:bx+00206h]               ; 26 8b 8f 06 02
    mov bx, word [es:bx+00208h]               ; 26 8b 9f 08 02
    lea dx, [bx+006h]                         ; 8d 57 06
    mov AL, strict byte 00ah                  ; b0 0a
    out DX, AL                                ; ee
    cmp byte [bp-008h], 000h                  ; 80 7e f8 00
    je short 02fa5h                           ; 74 05
    mov ax, 000b0h                            ; b8 b0 00
    jmp short 02fa8h                          ; eb 03
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
    jne short 02fb6h                          ; 75 f4
    and AL, strict byte 0e9h                  ; 24 e9
    cmp AL, strict byte 040h                  ; 3c 40
    je short 02fd3h                           ; 74 0b
    lea dx, [bx+006h]                         ; 8d 57 06
    mov AL, strict byte 008h                  ; b0 08
    out DX, AL                                ; ee
    mov ax, strict word 00001h                ; b8 01 00
    jmp short 02fdbh                          ; eb 08
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
set_diskette_current_cyl_:                   ; 0xf2fe3 LB 0x2a
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    mov dh, al                                ; 88 c6
    cmp AL, strict byte 001h                  ; 3c 01
    jbe short 02ff8h                          ; 76 0b
    push 00262h                               ; 68 62 02
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 76 ea
    add sp, strict byte 00004h                ; 83 c4 04
    movzx bx, dh                              ; 0f b6 de
    add bx, 00094h                            ; 81 c3 94 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], dl                      ; 26 88 17
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
floppy_wait_for_interrupt_:                  ; 0xf300d LB 0x23
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    cli                                       ; fa
    mov bx, strict word 0003eh                ; bb 3e 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov al, byte [es:bx]                      ; 26 8a 07
    test AL, strict byte 080h                 ; a8 80
    je short 03025h                           ; 74 04
    and AL, strict byte 080h                  ; 24 80
    jmp short 0302ah                          ; eb 05
    sti                                       ; fb
    hlt                                       ; f4
    cli                                       ; fa
    jmp short 03012h                          ; eb e8
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
floppy_wait_for_interrupt_or_timeout_:       ; 0xf3030 LB 0x38
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    cli                                       ; fa
    mov bx, strict word 00040h                ; bb 40 00
    mov es, bx                                ; 8e c3
    mov al, byte [es:bx]                      ; 26 8a 07
    test al, al                               ; 84 c0
    jne short 03044h                          ; 75 03
    sti                                       ; fb
    jmp short 03062h                          ; eb 1e
    mov bx, strict word 0003eh                ; bb 3e 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov al, byte [es:bx]                      ; 26 8a 07
    test AL, strict byte 080h                 ; a8 80
    je short 0305dh                           ; 74 0a
    mov ah, al                                ; 88 c4
    and ah, 07fh                              ; 80 e4 7f
    mov byte [es:bx], ah                      ; 26 88 27
    jmp short 03062h                          ; eb 05
    sti                                       ; fb
    hlt                                       ; f4
    cli                                       ; fa
    jmp short 03035h                          ; eb d3
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
floppy_reset_controller_:                    ; 0xf3068 LB 0x42
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
    movzx ax, bl                              ; 0f b6 c3
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
    jne short 03083h                          ; 75 f4
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
floppy_prepare_controller_:                  ; 0xf30aa LB 0x74
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
    je short 030d6h                           ; 74 04
    mov AL, strict byte 020h                  ; b0 20
    jmp short 030d8h                          ; eb 02
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
    shr al, 006h                              ; c0 e8 06
    mov dx, 003f7h                            ; ba f7 03
    out DX, AL                                ; ee
    mov dx, 003f4h                            ; ba f4 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 0c0h                  ; 24 c0
    cmp AL, strict byte 080h                  ; 3c 80
    jne short 030f6h                          ; 75 f4
    cmp byte [bp-008h], 000h                  ; 80 7e f8 00
    jne short 03116h                          ; 75 0e
    call 0300dh                               ; e8 02 ff
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
floppy_media_known_:                         ; 0xf311e LB 0x49
    push bx                                   ; 53
    push dx                                   ; 52
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov bx, strict word 0003eh                ; bb 3e 00
    mov dx, strict word 00040h                ; ba 40 00
    mov es, dx                                ; 8e c2
    mov bh, byte [es:bx]                      ; 26 8a 3f
    mov bl, bh                                ; 88 fb
    test ax, ax                               ; 85 c0
    je short 03136h                           ; 74 02
    shr bl, 1                                 ; d0 eb
    and bl, 001h                              ; 80 e3 01
    jne short 0313fh                          ; 75 04
    xor bh, bh                                ; 30 ff
    jmp short 03161h                          ; eb 22
    mov bx, 00090h                            ; bb 90 00
    test ax, ax                               ; 85 c0
    je short 03149h                           ; 74 03
    mov bx, 00091h                            ; bb 91 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov al, byte [es:bx]                      ; 26 8a 07
    xor ah, ah                                ; 30 e4
    sar ax, 004h                              ; c1 f8 04
    and AL, strict byte 001h                  ; 24 01
    jne short 0315eh                          ; 75 04
    xor bx, bx                                ; 31 db
    jmp short 03161h                          ; eb 03
    mov bx, strict word 00001h                ; bb 01 00
    mov ax, bx                                ; 89 d8
    pop bp                                    ; 5d
    pop dx                                    ; 5a
    pop bx                                    ; 5b
    retn                                      ; c3
floppy_read_id_:                             ; 0xf3167 LB 0x4e
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push dx                                   ; 52
    push si                                   ; 56
    mov bx, ax                                ; 89 c3
    call 030aah                               ; e8 38 ff
    mov AL, strict byte 04ah                  ; b0 4a
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    mov al, bl                                ; 88 d8
    out DX, AL                                ; ee
    call 0300dh                               ; e8 8f fe
    xor bx, bx                                ; 31 db
    jmp short 03187h                          ; eb 05
    cmp bx, strict byte 00007h                ; 83 fb 07
    jnl short 0319bh                          ; 7d 14
    mov dx, 003f5h                            ; ba f5 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    lea si, [bx+042h]                         ; 8d 77 42
    mov dx, strict word 00040h                ; ba 40 00
    mov es, dx                                ; 8e c2
    mov byte [es:si], al                      ; 26 88 04
    inc bx                                    ; 43
    jmp short 03182h                          ; eb e7
    mov bx, strict word 00042h                ; bb 42 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov al, byte [es:bx]                      ; 26 8a 07
    test AL, strict byte 0c0h                 ; a8 c0
    db  00fh, 094h, 0c0h
    ; sete al                                   ; 0f 94 c0
    xor ah, ah                                ; 30 e4
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
floppy_drive_recal_:                         ; 0xf31b5 LB 0x41
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push dx                                   ; 52
    push si                                   ; 56
    mov bx, ax                                ; 89 c3
    call 030aah                               ; e8 ea fe
    mov AL, strict byte 007h                  ; b0 07
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    mov al, bl                                ; 88 d8
    out DX, AL                                ; ee
    call 0300dh                               ; e8 41 fe
    test bx, bx                               ; 85 db
    je short 031d7h                           ; 74 07
    or AL, strict byte 002h                   ; 0c 02
    mov bx, 00095h                            ; bb 95 00
    jmp short 031dch                          ; eb 05
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
floppy_media_sense_:                         ; 0xf31f6 LB 0xe4
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push si                                   ; 56
    mov bx, ax                                ; 89 c3
    call 031b5h                               ; e8 b3 ff
    test ax, ax                               ; 85 c0
    jne short 0320bh                          ; 75 05
    xor dx, dx                                ; 31 d2
    jmp near 032cfh                           ; e9 c4 00
    mov ax, strict word 00010h                ; b8 10 00
    call 017a5h                               ; e8 94 e5
    test bx, bx                               ; 85 db
    jne short 0321ch                          ; 75 07
    mov dl, al                                ; 88 c2
    shr dl, 004h                              ; c0 ea 04
    jmp short 03221h                          ; eb 05
    mov dl, al                                ; 88 c2
    and dl, 00fh                              ; 80 e2 0f
    cmp dl, 001h                              ; 80 fa 01
    jne short 0322fh                          ; 75 09
    xor dl, dl                                ; 30 d2
    mov DH, strict byte 015h                  ; b6 15
    mov cx, strict word 00001h                ; b9 01 00
    jmp short 0326dh                          ; eb 3e
    cmp dl, 002h                              ; 80 fa 02
    jne short 0323ah                          ; 75 06
    xor dl, dl                                ; 30 d2
    mov DH, strict byte 035h                  ; b6 35
    jmp short 0322ah                          ; eb f0
    cmp dl, 003h                              ; 80 fa 03
    jne short 03245h                          ; 75 06
    xor dl, dl                                ; 30 d2
    mov DH, strict byte 017h                  ; b6 17
    jmp short 0322ah                          ; eb e5
    cmp dl, 004h                              ; 80 fa 04
    jne short 03250h                          ; 75 06
    xor dl, dl                                ; 30 d2
    mov DH, strict byte 017h                  ; b6 17
    jmp short 0322ah                          ; eb da
    cmp dl, 005h                              ; 80 fa 05
    jne short 0325bh                          ; 75 06
    mov DL, strict byte 0cch                  ; b2 cc
    mov DH, strict byte 0d7h                  ; b6 d7
    jmp short 0322ah                          ; eb cf
    cmp dl, 00eh                              ; 80 fa 0e
    je short 03265h                           ; 74 05
    cmp dl, 00fh                              ; 80 fa 0f
    jne short 03267h                          ; 75 02
    jmp short 03255h                          ; eb ee
    xor dl, dl                                ; 30 d2
    xor dh, dh                                ; 30 f6
    xor cx, cx                                ; 31 c9
    mov si, 0008bh                            ; be 8b 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:si], dl                      ; 26 88 14
    mov ax, bx                                ; 89 d8
    call 03167h                               ; e8 ea fe
    test ax, ax                               ; 85 c0
    jne short 032b3h                          ; 75 32
    mov al, dl                                ; 88 d0
    and AL, strict byte 0c0h                  ; 24 c0
    cmp AL, strict byte 080h                  ; 3c 80
    je short 032b3h                           ; 74 2a
    mov al, dl                                ; 88 d0
    and AL, strict byte 0c0h                  ; 24 c0
    cmp AL, strict byte 0c0h                  ; 3c c0
    je short 032a0h                           ; 74 0f
    mov ah, dl                                ; 88 d4
    and ah, 03fh                              ; 80 e4 3f
    cmp AL, strict byte 040h                  ; 3c 40
    je short 032ach                           ; 74 12
    test al, al                               ; 84 c0
    je short 032a5h                           ; 74 07
    jmp short 0326dh                          ; eb cd
    and dl, 03fh                              ; 80 e2 3f
    jmp short 0326dh                          ; eb c8
    mov dl, ah                                ; 88 e2
    or dl, 040h                               ; 80 ca 40
    jmp short 0326dh                          ; eb c1
    mov dl, ah                                ; 88 e2
    or dl, 080h                               ; 80 ca 80
    jmp short 0326dh                          ; eb ba
    test bx, bx                               ; 85 db
    db  00fh, 095h, 0c0h
    ; setne al                                  ; 0f 95 c0
    movzx bx, al                              ; 0f b6 d8
    add bx, 00090h                            ; 81 c3 90 00
    mov si, 0008bh                            ; be 8b 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:si], dl                      ; 26 88 14
    mov byte [es:bx], dh                      ; 26 88 37
    mov dx, cx                                ; 89 ca
    mov ax, dx                                ; 89 d0
    lea sp, [bp-008h]                         ; 8d 66 f8
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
floppy_drive_exists_:                        ; 0xf32da LB 0x47
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push dx                                   ; 52
    mov dx, ax                                ; 89 c2
    mov ax, strict word 00010h                ; b8 10 00
    call 017a5h                               ; e8 bf e4
    test dx, dx                               ; 85 d2
    jne short 032efh                          ; 75 05
    shr al, 004h                              ; c0 e8 04
    jmp short 032f1h                          ; eb 02
    and AL, strict byte 00fh                  ; 24 0f
    test al, al                               ; 84 c0
    db  00fh, 095h, 0c0h
    ; setne al                                  ; 0f 95 c0
    xor ah, ah                                ; 30 e4
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop dx                                    ; 5a
    pop bp                                    ; 5d
    retn                                      ; c3
    sbb byte [bx], dl                         ; 18 17
    push SS                                   ; 16
    adc ax, 00508h                            ; 15 08 05
    add AL, strict byte 003h                  ; 04 03
    add al, byte [bx+di]                      ; 02 01
    add byte [bp+si+0613ch], al               ; 00 82 3c 61
    db  033h, 0deh
    ; xor bx, si                                ; 33 de
    db  033h, 0feh
    ; xor di, si                                ; 33 fe
    db  033h, 0feh
    ; xor di, si                                ; 33 fe
    db  033h, 0feh
    ; xor di, si                                ; 33 fe
    xor di, word [bx+di+037h]                 ; 33 79 37
    aaa                                       ; 37
    cmp word [bx], sp                         ; 39 27
    cmp ch, byte [bx+di+03ah]                 ; 3a 69 3a
    popfw                                     ; 9d
    cmp cl, byte [bx]                         ; 3a 0f
    db  03bh
_int13_diskette_function:                    ; 0xf3321 LB 0x984
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    sub sp, strict byte 0000ch                ; 83 ec 0c
    or byte [bp+01dh], 002h                   ; 80 4e 1d 02
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    cmp ax, strict word 00018h                ; 3d 18 00
    jnbe near 03c82h                          ; 0f 87 4a 09
    push CS                                   ; 0e
    pop ES                                    ; 07
    mov cx, strict word 0000ch                ; b9 0c 00
    mov di, 032feh                            ; bf fe 32
    repne scasb                               ; f2 ae
    sal cx, 1                                 ; d1 e1
    mov di, cx                                ; 89 cf
    mov di, word [cs:di+03309h]               ; 2e 8b bd 09 33
    mov dx, word [bp+016h]                    ; 8b 56 16
    xor dh, dh                                ; 30 f6
    mov ah, byte [bp+00eh]                    ; 8a 66 0e
    mov cx, word [bp+01ch]                    ; 8b 4e 1c
    or cl, 001h                               ; 80 c9 01
    mov si, dx                                ; 89 d6
    or si, 00100h                             ; 81 ce 00 01
    jmp di                                    ; ff e7
    mov bl, byte [bp+00eh]                    ; 8a 5e 0e
    cmp bl, 001h                              ; 80 fb 01
    jbe short 03383h                          ; 76 1a
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 001h                               ; 80 cc 01
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 001h                    ; 26 c6 07 01
    jmp near 03b6fh                           ; e9 ec 07
    mov ax, strict word 00010h                ; b8 10 00
    call 017a5h                               ; e8 1c e4
    test bl, bl                               ; 84 db
    jne short 03394h                          ; 75 07
    mov dl, al                                ; 88 c2
    shr dl, 004h                              ; c0 ea 04
    jmp short 03399h                          ; eb 05
    mov dl, al                                ; 88 c2
    and dl, 00fh                              ; 80 e2 0f
    test dl, dl                               ; 84 d2
    jne short 033b7h                          ; 75 1a
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 080h                               ; 80 cc 80
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 080h                    ; 26 c6 07 80
    jmp near 03b6fh                           ; e9 b8 07
    mov si, strict word 0003eh                ; be 3e 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:si], 000h                    ; 26 c6 04 00
    xor al, al                                ; 30 c0
    mov byte [bp+017h], al                    ; 88 46 17
    mov si, strict word 00041h                ; be 41 00
    mov byte [es:si], al                      ; 26 88 04
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    movzx ax, bl                              ; 0f b6 c3
    xor dx, dx                                ; 31 d2
    call 02fe3h                               ; e8 09 fc
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    mov bx, 00441h                            ; bb 41 04
    xor ax, ax                                ; 31 c0
    mov es, ax                                ; 8e c0
    mov al, byte [es:bx]                      ; 26 8a 07
    movzx bx, al                              ; 0f b6 d8
    sal bx, 008h                              ; c1 e3 08
    or dx, bx                                 ; 09 da
    mov word [bp+016h], dx                    ; 89 56 16
    test al, al                               ; 84 c0
    je short 033dah                           ; 74 df
    jmp near 03b6fh                           ; e9 71 07
    mov bh, byte [bp+016h]                    ; 8a 7e 16
    mov ax, word [bp+014h]                    ; 8b 46 14
    shr ax, 008h                              ; c1 e8 08
    mov byte [bp-002h], al                    ; 88 46 fe
    mov al, byte [bp+014h]                    ; 8a 46 14
    mov byte [bp-006h], al                    ; 88 46 fa
    mov ax, word [bp+012h]                    ; 8b 46 12
    shr ax, 008h                              ; c1 e8 08
    mov byte [bp-004h], al                    ; 88 46 fc
    mov bl, byte [bp+00eh]                    ; 8a 5e 0e
    cmp bl, 001h                              ; 80 fb 01
    jnbe short 0342eh                         ; 77 0d
    cmp AL, strict byte 001h                  ; 3c 01
    jnbe short 0342eh                         ; 77 09
    test bh, bh                               ; 84 ff
    je short 0342eh                           ; 74 05
    cmp bh, 048h                              ; 80 ff 48
    jbe short 03461h                          ; 76 33
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 f1 e5
    push 00287h                               ; 68 87 02
    push 0029fh                               ; 68 9f 02
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 27 e6
    add sp, strict byte 00006h                ; 83 c4 06
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 001h                               ; 80 cc 01
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 001h                    ; 26 c6 07 01
    jmp near 034ech                           ; e9 8b 00
    movzx ax, bl                              ; 0f b6 c3
    call 032dah                               ; e8 73 fe
    test ax, ax                               ; 85 c0
    je near 03583h                            ; 0f 84 16 01
    movzx cx, bl                              ; 0f b6 cb
    mov ax, cx                                ; 89 c8
    call 0311eh                               ; e8 a9 fc
    test ax, ax                               ; 85 c0
    jne short 0349bh                          ; 75 22
    mov ax, cx                                ; 89 c8
    call 031f6h                               ; e8 78 fd
    test ax, ax                               ; 85 c0
    jne short 0349bh                          ; 75 19
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 00ch                               ; 80 cc 0c
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 00ch                    ; 26 c6 07 0c
    jmp short 034ech                          ; eb 51
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    cmp ax, strict word 00002h                ; 3d 02 00
    jne near 03637h                           ; 0f 85 8f 01
    mov dx, word [bp+006h]                    ; 8b 56 06
    shr dx, 00ch                              ; c1 ea 0c
    mov ah, dl                                ; 88 d4
    mov cx, word [bp+006h]                    ; 8b 4e 06
    sal cx, 004h                              ; c1 e1 04
    mov si, word [bp+010h]                    ; 8b 76 10
    add si, cx                                ; 01 ce
    mov word [bp-008h], si                    ; 89 76 f8
    cmp cx, si                                ; 39 f1
    jbe short 034c4h                          ; 76 02
    db  0feh, 0c4h
    ; inc ah                                    ; fe c4
    movzx cx, bh                              ; 0f b6 cf
    sal cx, 009h                              ; c1 e1 09
    dec cx                                    ; 49
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, cx                                ; 01 ca
    cmp dx, word [bp-008h]                    ; 3b 56 f8
    jnc short 034f3h                          ; 73 1e
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 009h                               ; 80 cc 09
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 009h                    ; 26 c6 07 09
    mov byte [bp+016h], 000h                  ; c6 46 16 00
    jmp near 03b6fh                           ; e9 7c 06
    mov AL, strict byte 006h                  ; b0 06
    mov dx, strict word 0000ah                ; ba 0a 00
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    mov dx, strict word 0000ch                ; ba 0c 00
    out DX, AL                                ; ee
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov dx, strict word 00004h                ; ba 04 00
    out DX, AL                                ; ee
    mov dx, word [bp-008h]                    ; 8b 56 f8
    shr dx, 008h                              ; c1 ea 08
    mov al, dl                                ; 88 d0
    mov dx, strict word 00004h                ; ba 04 00
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    mov dx, strict word 0000ch                ; ba 0c 00
    out DX, AL                                ; ee
    mov al, cl                                ; 88 c8
    mov dx, strict word 00005h                ; ba 05 00
    out DX, AL                                ; ee
    shr cx, 008h                              ; c1 e9 08
    mov al, cl                                ; 88 c8
    out DX, AL                                ; ee
    mov AL, strict byte 046h                  ; b0 46
    mov dx, strict word 0000bh                ; ba 0b 00
    out DX, AL                                ; ee
    mov al, ah                                ; 88 e0
    mov dx, 00081h                            ; ba 81 00
    out DX, AL                                ; ee
    mov AL, strict byte 002h                  ; b0 02
    mov dx, strict word 0000ah                ; ba 0a 00
    out DX, AL                                ; ee
    movzx cx, bl                              ; 0f b6 cb
    mov ax, cx                                ; 89 c8
    call 030aah                               ; e8 6c fb
    mov AL, strict byte 0e6h                  ; b0 e6
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    movzx dx, byte [bp-004h]                  ; 0f b6 56 fc
    sal dx, 002h                              ; c1 e2 02
    movzx ax, bl                              ; 0f b6 c3
    or ax, dx                                 ; 09 d0
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    mov al, byte [bp-002h]                    ; 8a 46 fe
    out DX, AL                                ; ee
    mov al, byte [bp-004h]                    ; 8a 46 fc
    out DX, AL                                ; ee
    mov al, byte [bp-006h]                    ; 8a 46 fa
    out DX, AL                                ; ee
    mov AL, strict byte 002h                  ; b0 02
    out DX, AL                                ; ee
    movzx ax, byte [bp-006h]                  ; 0f b6 46 fa
    movzx dx, bh                              ; 0f b6 d7
    add ax, dx                                ; 01 d0
    dec ax                                    ; 48
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    out DX, AL                                ; ee
    mov AL, strict byte 0ffh                  ; b0 ff
    out DX, AL                                ; ee
    call 03030h                               ; e8 b6 fa
    test al, al                               ; 84 c0
    jne short 0359dh                          ; 75 1f
    mov ax, cx                                ; 89 c8
    call 03068h                               ; e8 e5 fa
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 080h                               ; 80 cc 80
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 080h                    ; 26 c6 07 80
    jmp near 034ech                           ; e9 4f ff
    mov dx, 003f4h                            ; ba f4 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 0c0h                  ; 24 c0
    cmp AL, strict byte 0c0h                  ; 3c c0
    je short 035b7h                           ; 74 0e
    push 00287h                               ; 68 87 02
    push 002bah                               ; 68 ba 02
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 b7 e4
    add sp, strict byte 00006h                ; 83 c4 06
    xor cx, cx                                ; 31 c9
    jmp short 035c0h                          ; eb 05
    cmp cx, strict byte 00007h                ; 83 f9 07
    jnl short 035d6h                          ; 7d 16
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
    jmp short 035bbh                          ; eb e5
    mov si, strict word 00042h                ; be 42 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov al, byte [es:si]                      ; 26 8a 04
    test AL, strict byte 0c0h                 ; a8 c0
    je short 03605h                           ; 74 20
    movzx ax, bl                              ; 0f b6 c3
    call 03068h                               ; e8 7d fa
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 020h                               ; 80 cc 20
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 020h                    ; 26 c6 07 20
    jmp near 034ech                           ; e9 e7 fe
    movzx ax, bh                              ; 0f b6 c7
    sal ax, 009h                              ; c1 e0 09
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
    movzx dx, byte [bp-002h]                  ; 0f b6 56 fe
    movzx ax, bl                              ; 0f b6 c3
    call 02fe3h                               ; e8 b7 f9
    mov byte [bp+017h], 000h                  ; c6 46 17 00
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    jmp near 033dah                           ; e9 a3 fd
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    cmp ax, strict word 00003h                ; 3d 03 00
    jne near 03764h                           ; 0f 85 20 01
    mov cx, word [bp+006h]                    ; 8b 4e 06
    shr cx, 00ch                              ; c1 e9 0c
    mov ah, cl                                ; 88 cc
    mov dx, word [bp+006h]                    ; 8b 56 06
    sal dx, 004h                              ; c1 e2 04
    mov si, word [bp+010h]                    ; 8b 76 10
    add si, dx                                ; 01 d6
    mov word [bp-008h], si                    ; 89 76 f8
    cmp dx, si                                ; 39 f2
    jbe short 03660h                          ; 76 02
    db  0feh, 0c4h
    ; inc ah                                    ; fe c4
    movzx cx, bh                              ; 0f b6 cf
    sal cx, 009h                              ; c1 e1 09
    dec cx                                    ; 49
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, cx                                ; 01 ca
    cmp dx, word [bp-008h]                    ; 3b 56 f8
    jc near 034d5h                            ; 0f 82 62 fe
    mov AL, strict byte 006h                  ; b0 06
    mov dx, strict word 0000ah                ; ba 0a 00
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    mov dx, strict word 0000ch                ; ba 0c 00
    out DX, AL                                ; ee
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov dx, strict word 00004h                ; ba 04 00
    out DX, AL                                ; ee
    mov dx, word [bp-008h]                    ; 8b 56 f8
    shr dx, 008h                              ; c1 ea 08
    mov al, dl                                ; 88 d0
    mov dx, strict word 00004h                ; ba 04 00
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    mov dx, strict word 0000ch                ; ba 0c 00
    out DX, AL                                ; ee
    mov al, cl                                ; 88 c8
    mov dx, strict word 00005h                ; ba 05 00
    out DX, AL                                ; ee
    shr cx, 008h                              ; c1 e9 08
    mov al, cl                                ; 88 c8
    out DX, AL                                ; ee
    mov AL, strict byte 04ah                  ; b0 4a
    mov dx, strict word 0000bh                ; ba 0b 00
    out DX, AL                                ; ee
    mov al, ah                                ; 88 e0
    mov dx, 00081h                            ; ba 81 00
    out DX, AL                                ; ee
    mov AL, strict byte 002h                  ; b0 02
    mov dx, strict word 0000ah                ; ba 0a 00
    out DX, AL                                ; ee
    movzx cx, bl                              ; 0f b6 cb
    mov ax, cx                                ; 89 c8
    call 030aah                               ; e8 ec f9
    mov AL, strict byte 0c5h                  ; b0 c5
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    movzx dx, byte [bp-004h]                  ; 0f b6 56 fc
    sal dx, 002h                              ; c1 e2 02
    movzx ax, bl                              ; 0f b6 c3
    or ax, dx                                 ; 09 d0
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    mov al, byte [bp-002h]                    ; 8a 46 fe
    out DX, AL                                ; ee
    mov al, byte [bp-004h]                    ; 8a 46 fc
    out DX, AL                                ; ee
    mov al, byte [bp-006h]                    ; 8a 46 fa
    out DX, AL                                ; ee
    mov AL, strict byte 002h                  ; b0 02
    out DX, AL                                ; ee
    movzx dx, byte [bp-006h]                  ; 0f b6 56 fa
    movzx ax, bh                              ; 0f b6 c7
    add ax, dx                                ; 01 d0
    dec ax                                    ; 48
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    out DX, AL                                ; ee
    mov AL, strict byte 0ffh                  ; b0 ff
    out DX, AL                                ; ee
    call 03030h                               ; e8 36 f9
    test al, al                               ; 84 c0
    je near 0357eh                            ; 0f 84 7e fe
    mov dx, 003f4h                            ; ba f4 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 0c0h                  ; 24 c0
    cmp AL, strict byte 0c0h                  ; 3c c0
    je short 0371ah                           ; 74 0e
    push 00287h                               ; 68 87 02
    push 002bah                               ; 68 ba 02
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 54 e3
    add sp, strict byte 00006h                ; 83 c4 06
    xor cx, cx                                ; 31 c9
    jmp short 03723h                          ; eb 05
    cmp cx, strict byte 00007h                ; 83 f9 07
    jnl short 03739h                          ; 7d 16
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
    jmp short 0371eh                          ; eb e5
    mov si, strict word 00042h                ; be 42 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov al, byte [es:si]                      ; 26 8a 04
    test AL, strict byte 0c0h                 ; a8 c0
    je near 03622h                            ; 0f 84 d8 fe
    mov bx, strict word 00043h                ; bb 43 00
    mov al, byte [es:bx]                      ; 26 8a 07
    test AL, strict byte 002h                 ; a8 02
    je short 0375ch                           ; 74 08
    mov word [bp+016h], 00300h                ; c7 46 16 00 03
    jmp near 03b6fh                           ; e9 13 04
    mov word [bp+016h], 00100h                ; c7 46 16 00 01
    jmp near 03b6fh                           ; e9 0b 04
    movzx dx, byte [bp-002h]                  ; 0f b6 56 fe
    movzx ax, bl                              ; 0f b6 c3
    call 02fe3h                               ; e8 75 f8
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    mov byte [bp+017h], 000h                  ; c6 46 17 00
    jmp near 033dah                           ; e9 61 fc
    mov bh, byte [bp+016h]                    ; 8a 7e 16
    mov ax, word [bp+014h]                    ; 8b 46 14
    shr ax, 008h                              ; c1 e8 08
    mov byte [bp-002h], al                    ; 88 46 fe
    mov dx, word [bp+012h]                    ; 8b 56 12
    shr dx, 008h                              ; c1 ea 08
    mov byte [bp-004h], dl                    ; 88 56 fc
    mov bl, byte [bp+00eh]                    ; 8a 5e 0e
    cmp bl, 001h                              ; 80 fb 01
    jnbe short 037a8h                         ; 77 12
    cmp dl, 001h                              ; 80 fa 01
    jnbe short 037a8h                         ; 77 0d
    cmp AL, strict byte 04fh                  ; 3c 4f
    jnbe short 037a8h                         ; 77 09
    test bh, bh                               ; 84 ff
    je short 037a8h                           ; 74 05
    cmp bh, 012h                              ; 80 ff 12
    jbe short 037c3h                          ; 76 1b
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 001h                               ; 80 cc 01
    mov word [bp+016h], ax                    ; 89 46 16
    mov si, strict word 00041h                ; be 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:si], 001h                    ; 26 c6 04 01
    or byte [bp+01ch], 001h                   ; 80 4e 1c 01
    movzx ax, bl                              ; 0f b6 c3
    call 032dah                               ; e8 11 fb
    test ax, ax                               ; 85 c0
    jne short 037e7h                          ; 75 1a
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 080h                               ; 80 cc 80
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 080h                    ; 26 c6 07 80
    jmp near 03b6fh                           ; e9 88 03
    movzx cx, bl                              ; 0f b6 cb
    mov ax, cx                                ; 89 c8
    call 0311eh                               ; e8 2f f9
    test ax, ax                               ; 85 c0
    jne short 037feh                          ; 75 0b
    mov ax, cx                                ; 89 c8
    call 031f6h                               ; e8 fe f9
    test ax, ax                               ; 85 c0
    je near 03482h                            ; 0f 84 84 fc
    mov dx, word [bp+006h]                    ; 8b 56 06
    shr dx, 00ch                              ; c1 ea 0c
    mov ah, dl                                ; 88 d4
    mov cx, word [bp+006h]                    ; 8b 4e 06
    sal cx, 004h                              ; c1 e1 04
    mov si, word [bp+010h]                    ; 8b 76 10
    add si, cx                                ; 01 ce
    mov word [bp-008h], si                    ; 89 76 f8
    cmp cx, si                                ; 39 f1
    jbe short 0381ah                          ; 76 02
    db  0feh, 0c4h
    ; inc ah                                    ; fe c4
    movzx cx, bh                              ; 0f b6 cf
    sal cx, 002h                              ; c1 e1 02
    dec cx                                    ; 49
    mov dx, word [bp-008h]                    ; 8b 56 f8
    add dx, cx                                ; 01 ca
    cmp dx, word [bp-008h]                    ; 3b 56 f8
    jc near 034d5h                            ; 0f 82 a8 fc
    mov AL, strict byte 006h                  ; b0 06
    mov dx, strict word 0000ah                ; ba 0a 00
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    mov dx, strict word 0000ch                ; ba 0c 00
    out DX, AL                                ; ee
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov dx, strict word 00004h                ; ba 04 00
    out DX, AL                                ; ee
    mov dx, word [bp-008h]                    ; 8b 56 f8
    shr dx, 008h                              ; c1 ea 08
    mov al, dl                                ; 88 d0
    mov dx, strict word 00004h                ; ba 04 00
    out DX, AL                                ; ee
    xor al, al                                ; 30 c0
    mov dx, strict word 0000ch                ; ba 0c 00
    out DX, AL                                ; ee
    mov al, cl                                ; 88 c8
    mov dx, strict word 00005h                ; ba 05 00
    out DX, AL                                ; ee
    shr cx, 008h                              ; c1 e9 08
    mov al, cl                                ; 88 c8
    out DX, AL                                ; ee
    mov AL, strict byte 04ah                  ; b0 4a
    mov dx, strict word 0000bh                ; ba 0b 00
    out DX, AL                                ; ee
    mov al, ah                                ; 88 e0
    mov dx, 00081h                            ; ba 81 00
    out DX, AL                                ; ee
    mov AL, strict byte 002h                  ; b0 02
    mov dx, strict word 0000ah                ; ba 0a 00
    out DX, AL                                ; ee
    movzx cx, bl                              ; 0f b6 cb
    mov ax, cx                                ; 89 c8
    call 030aah                               ; e8 32 f8
    mov AL, strict byte 00fh                  ; b0 0f
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    movzx dx, byte [bp-004h]                  ; 0f b6 56 fc
    sal dx, 002h                              ; c1 e2 02
    movzx ax, bl                              ; 0f b6 c3
    or dx, ax                                 ; 09 c2
    mov word [bp-00ah], dx                    ; 89 56 f6
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    mov dx, 003f5h                            ; ba f5 03
    out DX, AL                                ; ee
    mov al, byte [bp-002h]                    ; 8a 46 fe
    out DX, AL                                ; ee
    mov AL, strict byte 04dh                  ; b0 4d
    out DX, AL                                ; ee
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    out DX, AL                                ; ee
    mov AL, strict byte 002h                  ; b0 02
    out DX, AL                                ; ee
    mov al, bh                                ; 88 f8
    out DX, AL                                ; ee
    xor al, bh                                ; 30 f8
    out DX, AL                                ; ee
    mov AL, strict byte 0f6h                  ; b0 f6
    out DX, AL                                ; ee
    call 03030h                               ; e8 82 f7
    test al, al                               ; 84 c0
    jne short 038bah                          ; 75 08
    mov ax, cx                                ; 89 c8
    call 03068h                               ; e8 b1 f7
    jmp near 037cdh                           ; e9 13 ff
    mov dx, 003f4h                            ; ba f4 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 0c0h                  ; 24 c0
    cmp AL, strict byte 0c0h                  ; 3c c0
    je short 038d4h                           ; 74 0e
    push 00287h                               ; 68 87 02
    push 002bah                               ; 68 ba 02
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 9a e1
    add sp, strict byte 00006h                ; 83 c4 06
    xor cx, cx                                ; 31 c9
    jmp short 038ddh                          ; eb 05
    cmp cx, strict byte 00007h                ; 83 f9 07
    jnl short 038f3h                          ; 7d 16
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
    jmp short 038d8h                          ; eb e5
    mov si, strict word 00042h                ; be 42 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov al, byte [es:si]                      ; 26 8a 04
    test AL, strict byte 0c0h                 ; a8 c0
    je short 0391ch                           ; 74 1a
    mov si, strict word 00043h                ; be 43 00
    mov al, byte [es:si]                      ; 26 8a 04
    test AL, strict byte 002h                 ; a8 02
    jne near 03754h                           ; 0f 85 46 fe
    push 00287h                               ; 68 87 02
    push 002ceh                               ; 68 ce 02
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 52 e1
    add sp, strict byte 00006h                ; 83 c4 06
    mov byte [bp+017h], 000h                  ; c6 46 17 00
    mov si, strict word 00041h                ; be 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:si], 000h                    ; 26 c6 04 00
    movzx ax, bl                              ; 0f b6 c3
    xor dx, dx                                ; 31 d2
    call 02fe3h                               ; e8 af f6
    jmp near 03630h                           ; e9 f9 fc
    mov bl, ah                                ; 88 e3
    cmp ah, 001h                              ; 80 fc 01
    jbe short 0395bh                          ; 76 1d
    xor ax, ax                                ; 31 c0
    mov word [bp+016h], ax                    ; 89 46 16
    mov word [bp+010h], ax                    ; 89 46 10
    mov word [bp+014h], ax                    ; 89 46 14
    mov word [bp+012h], ax                    ; 89 46 12
    mov word [bp+006h], ax                    ; 89 46 06
    mov word [bp+008h], ax                    ; 89 46 08
    movzx ax, bh                              ; 0f b6 c7
    mov word [bp+012h], ax                    ; 89 46 12
    jmp near 03a7dh                           ; e9 22 01
    mov ax, strict word 00010h                ; b8 10 00
    call 017a5h                               ; e8 44 de
    mov dl, al                                ; 88 c2
    xor bh, bh                                ; 30 ff
    test AL, strict byte 0f0h                 ; a8 f0
    je short 0396bh                           ; 74 02
    mov BH, strict byte 001h                  ; b7 01
    test dl, 00fh                             ; f6 c2 0f
    je short 03972h                           ; 74 02
    db  0feh, 0c7h
    ; inc bh                                    ; fe c7
    test bl, bl                               ; 84 db
    jne short 0397bh                          ; 75 05
    shr dl, 004h                              ; c0 ea 04
    jmp short 0397eh                          ; eb 03
    and dl, 00fh                              ; 80 e2 0f
    mov byte [bp+011h], 000h                  ; c6 46 11 00
    movzx ax, dl                              ; 0f b6 c2
    mov word [bp+010h], ax                    ; 89 46 10
    mov word [bp+016h], strict word 00000h    ; c7 46 16 00 00
    mov cx, word [bp+012h]                    ; 8b 4e 12
    xor cl, cl                                ; 30 c9
    movzx ax, bh                              ; 0f b6 c7
    or cx, ax                                 ; 09 c1
    mov word [bp+012h], cx                    ; 89 4e 12
    mov ax, cx                                ; 89 c8
    xor ah, ch                                ; 30 ec
    or ah, 001h                               ; 80 cc 01
    mov word [bp+012h], ax                    ; 89 46 12
    cmp dl, 003h                              ; 80 fa 03
    jc short 039beh                           ; 72 15
    jbe short 039e5h                          ; 76 3a
    cmp dl, 005h                              ; 80 fa 05
    jc short 039ech                           ; 72 3c
    jbe short 039f3h                          ; 76 41
    cmp dl, 00fh                              ; 80 fa 0f
    je short 03a01h                           ; 74 4a
    cmp dl, 00eh                              ; 80 fa 0e
    je short 039fah                           ; 74 3e
    jmp short 03a08h                          ; eb 4a
    cmp dl, 002h                              ; 80 fa 02
    je short 039deh                           ; 74 1b
    cmp dl, 001h                              ; 80 fa 01
    je short 039d7h                           ; 74 0f
    test dl, dl                               ; 84 d2
    jne short 03a08h                          ; 75 3c
    mov word [bp+014h], strict word 00000h    ; c7 46 14 00 00
    mov byte [bp+013h], 000h                  ; c6 46 13 00
    jmp short 03a16h                          ; eb 3f
    mov word [bp+014h], 02709h                ; c7 46 14 09 27
    jmp short 03a16h                          ; eb 38
    mov word [bp+014h], 04f0fh                ; c7 46 14 0f 4f
    jmp short 03a16h                          ; eb 31
    mov word [bp+014h], 04f09h                ; c7 46 14 09 4f
    jmp short 03a16h                          ; eb 2a
    mov word [bp+014h], 04f12h                ; c7 46 14 12 4f
    jmp short 03a16h                          ; eb 23
    mov word [bp+014h], 04f24h                ; c7 46 14 24 4f
    jmp short 03a16h                          ; eb 1c
    mov word [bp+014h], 0fe3fh                ; c7 46 14 3f fe
    jmp short 03a16h                          ; eb 15
    mov word [bp+014h], 0feffh                ; c7 46 14 ff fe
    jmp short 03a16h                          ; eb 0e
    push 00287h                               ; 68 87 02
    push 002dfh                               ; 68 df 02
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 58 e0
    add sp, strict byte 00006h                ; 83 c4 06
    mov word [bp+006h], 0f000h                ; c7 46 06 00 f0
    movzx ax, dl                              ; 0f b6 c2
    call 03ca5h                               ; e8 84 02
    mov word [bp+008h], ax                    ; 89 46 08
    jmp near 03630h                           ; e9 09 fc
    mov bl, ah                                ; 88 e3
    cmp ah, 001h                              ; 80 fc 01
    jbe short 03a33h                          ; 76 05
    mov word [bp+016h], dx                    ; 89 56 16
    jmp short 03a7dh                          ; eb 4a
    mov ax, strict word 00010h                ; b8 10 00
    call 017a5h                               ; e8 6c dd
    test bl, bl                               ; 84 db
    jne short 03a44h                          ; 75 07
    mov dl, al                                ; 88 c2
    shr dl, 004h                              ; c0 ea 04
    jmp short 03a49h                          ; eb 05
    mov dl, al                                ; 88 c2
    and dl, 00fh                              ; 80 e2 0f
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    test dl, dl                               ; 84 d2
    je short 03a63h                           ; 74 0d
    cmp dl, 001h                              ; 80 fa 01
    jbe short 03a60h                          ; 76 05
    or ah, 002h                               ; 80 cc 02
    jmp short 03a63h                          ; eb 03
    or ah, 001h                               ; 80 cc 01
    mov word [bp+016h], ax                    ; 89 46 16
    jmp near 033dah                           ; e9 71 f9
    cmp ah, 001h                              ; 80 fc 01
    jbe short 03a83h                          ; 76 15
    mov word [bp+016h], si                    ; 89 76 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 001h                    ; 26 c6 07 01
    mov word [bp+01ch], cx                    ; 89 4e 1c
    jmp near 033dah                           ; e9 57 f9
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 006h                               ; 80 cc 06
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 006h                    ; 26 c6 07 06
    jmp near 03b6fh                           ; e9 d2 00
    mov bl, ah                                ; 88 e3
    mov dl, byte [bp+016h]                    ; 8a 56 16
    cmp ah, 001h                              ; 80 fc 01
    jnbe short 03a6eh                         ; 77 c7
    movzx ax, bl                              ; 0f b6 c3
    call 032dah                               ; e8 2d f8
    test ax, ax                               ; 85 c0
    je near 037cdh                            ; 0f 84 1a fd
    test bl, bl                               ; 84 db
    db  00fh, 095h, 0c0h
    ; setne al                                  ; 0f 95 c0
    movzx bx, al                              ; 0f b6 d8
    add bx, 00090h                            ; 81 c3 90 00
    mov word [bp-008h], bx                    ; 89 5e f8
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov bl, byte [es:bx]                      ; 26 8a 1f
    and bl, 00fh                              ; 80 e3 0f
    cmp dl, 002h                              ; 80 fa 02
    jc short 03ae1h                           ; 72 0f
    jbe short 03aeeh                          ; 76 1a
    cmp dl, 004h                              ; 80 fa 04
    je short 03ae9h                           ; 74 10
    cmp dl, 003h                              ; 80 fa 03
    je short 03af3h                           ; 74 15
    jmp near 03369h                           ; e9 88 f8
    cmp dl, 001h                              ; 80 fa 01
    je short 03ae9h                           ; 74 03
    jmp near 03369h                           ; e9 80 f8
    or bl, 090h                               ; 80 cb 90
    jmp short 03af6h                          ; eb 08
    or bl, 070h                               ; 80 cb 70
    jmp short 03af6h                          ; eb 03
    or bl, 010h                               ; 80 cb 10
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov si, word [bp-008h]                    ; 8b 76 f8
    mov byte [es:si], bl                      ; 26 88 1c
    xor al, al                                ; 30 c0
    mov byte [bp+017h], al                    ; 88 46 17
    mov bx, strict word 00041h                ; bb 41 00
    mov byte [es:bx], al                      ; 26 88 07
    jmp near 03630h                           ; e9 21 fb
    mov bl, ah                                ; 88 e3
    mov dl, byte [bp+014h]                    ; 8a 56 14
    mov bh, dl                                ; 88 d7
    and bh, 03fh                              ; 80 e7 3f
    xor dh, dh                                ; 30 f6
    sar dx, 006h                              ; c1 fa 06
    sal dx, 008h                              ; c1 e2 08
    mov word [bp-00ch], dx                    ; 89 56 f4
    mov dx, word [bp+014h]                    ; 8b 56 14
    shr dx, 008h                              ; c1 ea 08
    add dx, word [bp-00ch]                    ; 03 56 f4
    mov byte [bp-002h], dl                    ; 88 56 fe
    cmp ah, 001h                              ; 80 fc 01
    jnbe near 03a6eh                          ; 0f 87 37 ff
    movzx ax, bl                              ; 0f b6 c3
    call 032dah                               ; e8 9d f7
    test ax, ax                               ; 85 c0
    je near 037cdh                            ; 0f 84 8a fc
    movzx cx, bl                              ; 0f b6 cb
    mov ax, cx                                ; 89 c8
    call 0311eh                               ; e8 d3 f5
    test ax, ax                               ; 85 c0
    jne short 03b76h                          ; 75 27
    mov ax, cx                                ; 89 c8
    call 031f6h                               ; e8 a2 f6
    test ax, ax                               ; 85 c0
    jne short 03b76h                          ; 75 1e
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 00ch                               ; 80 cc 0c
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 00ch                    ; 26 c6 07 0c
    or byte [bp+01ch], 001h                   ; 80 4e 1c 01
    jmp near 033dah                           ; e9 64 f8
    mov ax, strict word 00010h                ; b8 10 00
    call 017a5h                               ; e8 29 dc
    test bl, bl                               ; 84 db
    jne short 03b87h                          ; 75 07
    mov dl, al                                ; 88 c2
    shr dl, 004h                              ; c0 ea 04
    jmp short 03b8ch                          ; eb 05
    mov dl, al                                ; 88 c2
    and dl, 00fh                              ; 80 e2 0f
    test bl, bl                               ; 84 db
    db  00fh, 095h, 0c0h
    ; setne al                                  ; 0f 95 c0
    movzx si, al                              ; 0f b6 f0
    add si, 00090h                            ; 81 c6 90 00
    mov word [bp-008h], si                    ; 89 76 f8
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov bl, byte [es:si]                      ; 26 8a 1c
    and bl, 00fh                              ; 80 e3 0f
    cmp dl, 003h                              ; 80 fa 03
    jc short 03bc8h                           ; 72 1d
    mov al, bl                                ; 88 d8
    or AL, strict byte 090h                   ; 0c 90
    cmp dl, 003h                              ; 80 fa 03
    jbe short 03c09h                          ; 76 55
    mov ah, bl                                ; 88 dc
    or ah, 010h                               ; 80 cc 10
    cmp dl, 005h                              ; 80 fa 05
    je near 03c2eh                            ; 0f 84 6e 00
    cmp dl, 004h                              ; 80 fa 04
    je short 03c18h                           ; 74 53
    jmp near 03c4ah                           ; e9 82 00
    cmp dl, 002h                              ; 80 fa 02
    je short 03be9h                           ; 74 1c
    cmp dl, 001h                              ; 80 fa 01
    jne near 03c4ah                           ; 0f 85 76 00
    cmp byte [bp-002h], 027h                  ; 80 7e fe 27
    jne near 03c4ah                           ; 0f 85 6e 00
    cmp bh, 009h                              ; 80 ff 09
    jne near 03c4ah                           ; 0f 85 67 00
    or bl, 090h                               ; 80 cb 90
    jmp near 03c4ah                           ; e9 61 00
    cmp byte [bp-002h], 027h                  ; 80 7e fe 27
    jne short 03bf9h                          ; 75 0a
    cmp bh, 009h                              ; 80 ff 09
    jne short 03bf9h                          ; 75 05
    or bl, 070h                               ; 80 cb 70
    jmp short 03c4ah                          ; eb 51
    cmp byte [bp-002h], 04fh                  ; 80 7e fe 4f
    jne short 03c4ah                          ; 75 4b
    cmp bh, 00fh                              ; 80 ff 0f
    jne short 03c4ah                          ; 75 46
    or bl, 010h                               ; 80 cb 10
    jmp short 03c4ah                          ; eb 41
    cmp byte [bp-002h], 04fh                  ; 80 7e fe 4f
    jne short 03c4ah                          ; 75 3b
    cmp bh, 009h                              ; 80 ff 09
    jne short 03c4ah                          ; 75 36
    mov bl, al                                ; 88 c3
    jmp short 03c4ah                          ; eb 32
    cmp byte [bp-002h], 04fh                  ; 80 7e fe 4f
    jne short 03c4ah                          ; 75 2c
    cmp bh, 009h                              ; 80 ff 09
    jne short 03c25h                          ; 75 02
    jmp short 03c14h                          ; eb ef
    cmp bh, 012h                              ; 80 ff 12
    jne short 03c4ah                          ; 75 20
    mov bl, ah                                ; 88 e3
    jmp short 03c4ah                          ; eb 1c
    cmp byte [bp-002h], 04fh                  ; 80 7e fe 4f
    jne short 03c4ah                          ; 75 16
    cmp bh, 009h                              ; 80 ff 09
    jne short 03c3bh                          ; 75 02
    jmp short 03c14h                          ; eb d9
    cmp bh, 012h                              ; 80 ff 12
    jne short 03c42h                          ; 75 02
    jmp short 03c2ah                          ; eb e8
    cmp bh, 024h                              ; 80 ff 24
    jne short 03c4ah                          ; 75 03
    or bl, 0d0h                               ; 80 cb d0
    movzx ax, bl                              ; 0f b6 c3
    sar ax, 004h                              ; c1 f8 04
    test AL, strict byte 001h                 ; a8 01
    je near 03b58h                            ; 0f 84 02 ff
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov si, word [bp-008h]                    ; 8b 76 f8
    mov byte [es:si], bl                      ; 26 88 1c
    mov word [bp+006h], 0f000h                ; c7 46 06 00 f0
    movzx ax, dl                              ; 0f b6 c2
    call 03ca5h                               ; e8 39 00
    mov word [bp+008h], ax                    ; 89 46 08
    mov byte [bp+017h], 000h                  ; c6 46 17 00
    mov bx, strict word 00041h                ; bb 41 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    mov byte [es:bx], 000h                    ; 26 c6 07 00
    jmp near 03630h                           ; e9 ae f9
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 9d dd
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 00287h                               ; 68 87 02
    push 002f4h                               ; 68 f4 02
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 cc dd
    add sp, strict byte 00008h                ; 83 c4 08
    jmp near 03369h                           ; e9 c4 f6
get_floppy_dpt_:                             ; 0xf3ca5 LB 0x2f
    push bx                                   ; 53
    push dx                                   ; 52
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov dl, al                                ; 88 c2
    xor ax, ax                                ; 31 c0
    jmp short 03cb6h                          ; eb 06
    inc ax                                    ; 40
    cmp ax, strict word 00007h                ; 3d 07 00
    jnc short 03ccdh                          ; 73 17
    mov bx, ax                                ; 89 c3
    add bx, ax                                ; 01 c3
    cmp dl, byte [word bx+0005bh]             ; 3a 97 5b 00
    jne short 03cb0h                          ; 75 f0
    movzx ax, byte [word bx+0005ch]           ; 0f b6 87 5c 00
    imul ax, ax, strict byte 0000dh           ; 6b c0 0d
    add ax, strict word 00000h                ; 05 00 00
    jmp short 03cd0h                          ; eb 03
    mov ax, strict word 00041h                ; b8 41 00
    pop bp                                    ; 5d
    pop dx                                    ; 5a
    pop bx                                    ; 5b
    retn                                      ; c3
dummy_soft_reset_:                           ; 0xf3cd4 LB 0x7
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    xor ax, ax                                ; 31 c0
    pop bp                                    ; 5d
    retn                                      ; c3
_cdemu_init:                                 ; 0xf3cdb LB 0x18
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 7e da
    xor bx, bx                                ; 31 db
    mov dx, 00366h                            ; ba 66 03
    call 01757h                               ; e8 68 da
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
_cdemu_isactive:                             ; 0xf3cf3 LB 0x16
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 66 da
    mov dx, 00366h                            ; ba 66 03
    call 01749h                               ; e8 44 da
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
_cdemu_emulated_drive:                       ; 0xf3d09 LB 0x16
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 50 da
    mov dx, 00368h                            ; ba 68 03
    call 01749h                               ; e8 2e da
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
_int13_eltorito:                             ; 0xf3d1f LB 0x189
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 38 da
    mov si, 00366h                            ; be 66 03
    mov di, ax                                ; 89 c7
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    cmp ax, strict word 0004bh                ; 3d 4b 00
    jc short 03d47h                           ; 72 0a
    jbe short 03d6dh                          ; 76 2e
    cmp ax, strict word 0004dh                ; 3d 4d 00
    jbe short 03d4eh                          ; 76 0a
    jmp near 03e6ch                           ; e9 25 01
    cmp ax, strict word 0004ah                ; 3d 4a 00
    jne near 03e6ch                           ; 0f 85 1e 01
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 d1 dc
    push word [bp+016h]                       ; ff 76 16
    push 0030eh                               ; 68 0e 03
    push 0031dh                               ; 68 1d 03
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 04 dd
    add sp, strict byte 00008h                ; 83 c4 08
    jmp near 03e87h                           ; e9 1a 01
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    mov ax, word [bp+004h]                    ; 8b 46 04
    mov bx, strict word 00013h                ; bb 13 00
    call 01757h                               ; e8 de d9
    mov es, di                                ; 8e c7
    movzx bx, byte [es:si+001h]               ; 26 0f b6 5c 01
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    inc dx                                    ; 42
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01757h                               ; e8 cd d9
    mov es, di                                ; 8e c7
    movzx bx, byte [es:si+002h]               ; 26 0f b6 5c 02
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    inc dx                                    ; 42
    inc dx                                    ; 42
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01757h                               ; e8 bb d9
    mov es, di                                ; 8e c7
    movzx bx, byte [es:si+003h]               ; 26 0f b6 5c 03
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00003h                ; 83 c2 03
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01757h                               ; e8 a8 d9
    mov es, di                                ; 8e c7
    mov bx, word [es:si+008h]                 ; 26 8b 5c 08
    mov cx, word [es:si+00ah]                 ; 26 8b 4c 0a
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00004h                ; 83 c2 04
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01793h                               ; e8 ce d9
    mov es, di                                ; 8e c7
    mov bx, word [es:si+004h]                 ; 26 8b 5c 04
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00008h                ; 83 c2 08
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01773h                               ; e8 9c d9
    mov es, di                                ; 8e c7
    mov bx, word [es:si+006h]                 ; 26 8b 5c 06
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 0000ah                ; 83 c2 0a
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01773h                               ; e8 8a d9
    mov es, di                                ; 8e c7
    mov bx, word [es:si+00ch]                 ; 26 8b 5c 0c
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 0000ch                ; 83 c2 0c
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01773h                               ; e8 78 d9
    mov es, di                                ; 8e c7
    mov bx, word [es:si+00eh]                 ; 26 8b 5c 0e
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 0000eh                ; 83 c2 0e
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01773h                               ; e8 66 d9
    mov es, di                                ; 8e c7
    movzx bx, byte [es:si+012h]               ; 26 0f b6 5c 12
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00010h                ; 83 c2 10
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01757h                               ; e8 37 d9
    mov es, di                                ; 8e c7
    movzx bx, byte [es:si+014h]               ; 26 0f b6 5c 14
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00011h                ; 83 c2 11
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01757h                               ; e8 24 d9
    mov es, di                                ; 8e c7
    movzx bx, byte [es:si+010h]               ; 26 0f b6 5c 10
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00012h                ; 83 c2 12
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01757h                               ; e8 11 d9
    test byte [bp+016h], 0ffh                 ; f6 46 16 ff
    jne short 03e52h                          ; 75 06
    mov es, di                                ; 8e c7
    mov byte [es:si], 000h                    ; 26 c6 04 00
    mov byte [bp+017h], 000h                  ; c6 46 17 00
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 f6 d8
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 b3 db
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 0030eh                               ; 68 0e 03
    push 00345h                               ; 68 45 03
    jmp near 03d62h                           ; e9 db fe
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 001h                               ; 80 cc 01
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, ax                                ; 89 c3
    shr bx, 008h                              ; c1 eb 08
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 b5 d8
    or byte [bp+01ch], 001h                   ; 80 4e 1c 01
    jmp short 03e65h                          ; eb bd
device_is_cdrom_:                            ; 0xf3ea8 LB 0x35
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push dx                                   ; 52
    mov bl, al                                ; 88 c3
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 ad d8
    cmp bl, 010h                              ; 80 fb 10
    jc short 03ec1h                           ; 72 04
    xor ax, ax                                ; 31 c0
    jmp short 03ed6h                          ; eb 15
    xor bh, bh                                ; 30 ff
    imul bx, bx, strict byte 0001ch           ; 6b db 1c
    mov es, ax                                ; 8e c0
    add bx, 00122h                            ; 81 c3 22 01
    cmp byte [es:bx+023h], 005h               ; 26 80 7f 23 05
    jne short 03ebdh                          ; 75 ea
    mov ax, strict word 00001h                ; b8 01 00
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop dx                                    ; 5a
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
cdrom_boot_:                                 ; 0xf3edd LB 0x417
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
    call 01765h                               ; e8 73 d8
    mov word [bp-018h], ax                    ; 89 46 e8
    mov si, 00366h                            ; be 66 03
    mov word [bp-010h], ax                    ; 89 46 f0
    mov word [bp-014h], 00122h                ; c7 46 ec 22 01
    mov word [bp-012h], ax                    ; 89 46 ee
    mov byte [bp-00ch], 000h                  ; c6 46 f4 00
    jmp short 03f12h                          ; eb 09
    inc byte [bp-00ch]                        ; fe 46 f4
    cmp byte [bp-00ch], 010h                  ; 80 7e f4 10
    jnc short 03f1dh                          ; 73 0b
    movzx ax, byte [bp-00ch]                  ; 0f b6 46 f4
    call 03ea8h                               ; e8 8f ff
    test ax, ax                               ; 85 c0
    je short 03f09h                           ; 74 ec
    cmp byte [bp-00ch], 010h                  ; 80 7e f4 10
    jc short 03f29h                           ; 72 06
    mov ax, strict word 00002h                ; b8 02 00
    jmp near 04291h                           ; e9 68 03
    mov cx, strict word 0000ch                ; b9 0c 00
    xor bx, bx                                ; 31 db
    mov dx, ss                                ; 8c d2
    lea ax, [bp-026h]                         ; 8d 46 da
    call 0a130h                               ; e8 fa 61
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
    les bx, [bp-014h]                         ; c4 5e ec
    db  066h, 026h, 0c7h, 047h, 00eh, 001h, 000h, 000h, 008h
    ; mov dword [es:bx+00eh], strict dword 008000001h ; 66 26 c7 47 0e 01 00 00 08
    mov byte [bp-00eh], 000h                  ; c6 46 f2 00
    jmp short 03f6eh                          ; eb 09
    inc byte [bp-00eh]                        ; fe 46 f2
    cmp byte [bp-00eh], 004h                  ; 80 7e f2 04
    jnbe short 03fa4h                         ; 77 36
    movzx ax, byte [bp-00ch]                  ; 0f b6 46 f4
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    les di, [bp-014h]                         ; c4 7e ec
    add di, ax                                ; 01 c7
    movzx di, byte [es:di+022h]               ; 26 0f b6 7d 22
    add di, di                                ; 01 ff
    lea dx, [bp-00826h]                       ; 8d 96 da f7
    push SS                                   ; 16
    push dx                                   ; 52
    push strict byte 00001h                   ; 6a 01
    push strict byte 00000h                   ; 6a 00
    push 00800h                               ; 68 00 08
    push strict byte 00000h                   ; 6a 00
    movzx ax, byte [bp-00ch]                  ; 0f b6 46 f4
    mov cx, ss                                ; 8c d1
    lea bx, [bp-026h]                         ; 8d 5e da
    mov dx, strict word 0000ch                ; ba 0c 00
    call word [word di+0006ah]                ; ff 95 6a 00
    test ax, ax                               ; 85 c0
    jne short 03f65h                          ; 75 c1
    test ax, ax                               ; 85 c0
    je short 03faeh                           ; 74 06
    mov ax, strict word 00003h                ; b8 03 00
    jmp near 04291h                           ; e9 e3 02
    cmp byte [bp-00826h], 000h                ; 80 be da f7 00
    je short 03fbbh                           ; 74 06
    mov ax, strict word 00004h                ; b8 04 00
    jmp near 04291h                           ; e9 d6 02
    xor di, di                                ; 31 ff
    jmp short 03fc5h                          ; eb 06
    inc di                                    ; 47
    cmp di, strict byte 00005h                ; 83 ff 05
    jnc short 03fd5h                          ; 73 10
    mov al, byte [bp+di-00825h]               ; 8a 83 db f7
    cmp al, byte [di+00dbah]                  ; 3a 85 ba 0d
    je short 03fbfh                           ; 74 f0
    mov ax, strict word 00005h                ; b8 05 00
    jmp near 04291h                           ; e9 bc 02
    xor di, di                                ; 31 ff
    jmp short 03fdfh                          ; eb 06
    inc di                                    ; 47
    cmp di, strict byte 00017h                ; 83 ff 17
    jnc short 03fefh                          ; 73 10
    mov al, byte [bp+di-0081fh]               ; 8a 83 e1 f7
    cmp al, byte [di+00dc0h]                  ; 3a 85 c0 0d
    je short 03fd9h                           ; 74 f0
    mov ax, strict word 00006h                ; b8 06 00
    jmp near 04291h                           ; e9 a2 02
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
    movzx ax, byte [bp-00ch]                  ; 0f b6 46 f4
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    les di, [bp-014h]                         ; c4 7e ec
    add di, ax                                ; 01 c7
    movzx di, byte [es:di+022h]               ; 26 0f b6 7d 22
    add di, di                                ; 01 ff
    lea dx, [bp-00826h]                       ; 8d 96 da f7
    push SS                                   ; 16
    push dx                                   ; 52
    push strict byte 00001h                   ; 6a 01
    push strict byte 00000h                   ; 6a 00
    push 00800h                               ; 68 00 08
    push strict byte 00000h                   ; 6a 00
    movzx ax, byte [bp-00ch]                  ; 0f b6 46 f4
    mov cx, ss                                ; 8c d1
    lea bx, [bp-026h]                         ; 8d 5e da
    mov dx, strict word 0000ch                ; ba 0c 00
    call word [word di+0006ah]                ; ff 95 6a 00
    test ax, ax                               ; 85 c0
    je short 0404bh                           ; 74 06
    mov ax, strict word 00007h                ; b8 07 00
    jmp near 04291h                           ; e9 46 02
    cmp byte [bp-00826h], 001h                ; 80 be da f7 01
    je short 04058h                           ; 74 06
    mov ax, strict word 00008h                ; b8 08 00
    jmp near 04291h                           ; e9 39 02
    cmp byte [bp-00825h], 000h                ; 80 be db f7 00
    je short 04065h                           ; 74 06
    mov ax, strict word 00009h                ; b8 09 00
    jmp near 04291h                           ; e9 2c 02
    cmp byte [bp-00808h], 055h                ; 80 be f8 f7 55
    je short 04072h                           ; 74 06
    mov ax, strict word 0000ah                ; b8 0a 00
    jmp near 04291h                           ; e9 1f 02
    cmp byte [bp-00807h], 0aah                ; 80 be f9 f7 aa
    jne short 0406ch                          ; 75 f3
    cmp byte [bp-00806h], 088h                ; 80 be fa f7 88
    je short 04086h                           ; 74 06
    mov ax, strict word 0000bh                ; b8 0b 00
    jmp near 04291h                           ; e9 0b 02
    mov al, byte [bp-00805h]                  ; 8a 86 fb f7
    mov es, [bp-010h]                         ; 8e 46 f0
    mov byte [es:si+001h], al                 ; 26 88 44 01
    cmp byte [bp-00805h], 000h                ; 80 be fb f7 00
    jne short 0409fh                          ; 75 07
    mov byte [es:si+002h], 0e0h               ; 26 c6 44 02 e0
    jmp short 040b2h                          ; eb 13
    cmp byte [bp-00805h], 004h                ; 80 be fb f7 04
    jnc short 040adh                          ; 73 07
    mov byte [es:si+002h], 000h               ; 26 c6 44 02 00
    jmp short 040b2h                          ; eb 05
    mov byte [es:si+002h], 080h               ; 26 c6 44 02 80
    movzx di, byte [bp-00ch]                  ; 0f b6 7e f4
    mov ax, di                                ; 89 f8
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    mov es, [bp-010h]                         ; 8e 46 f0
    mov byte [es:si+003h], al                 ; 26 88 44 03
    mov ax, di                                ; 89 f8
    cwd                                       ; 99
    mov bx, strict word 00002h                ; bb 02 00
    idiv bx                                   ; f7 fb
    mov word [es:si+004h], dx                 ; 26 89 54 04
    mov ax, word [bp-00804h]                  ; 8b 86 fc f7
    mov word [bp-016h], ax                    ; 89 46 ea
    test ax, ax                               ; 85 c0
    jne short 040e0h                          ; 75 05
    mov word [bp-016h], 007c0h                ; c7 46 ea c0 07
    mov ax, word [bp-016h]                    ; 8b 46 ea
    mov es, [bp-010h]                         ; 8e 46 f0
    mov word [es:si+00ch], ax                 ; 26 89 44 0c
    mov word [es:si+006h], strict word 00000h ; 26 c7 44 06 00 00
    mov di, word [bp-00800h]                  ; 8b be 00 f8
    mov word [es:si+00eh], di                 ; 26 89 7c 0e
    test di, di                               ; 85 ff
    je short 04102h                           ; 74 06
    cmp di, 00400h                            ; 81 ff 00 04
    jbe short 04108h                          ; 76 06
    mov ax, strict word 0000ch                ; b8 0c 00
    jmp near 04291h                           ; e9 89 01
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
    lea cx, [di-001h]                         ; 8d 4d ff
    shr cx, 002h                              ; c1 e9 02
    inc cx                                    ; 41
    mov ax, cx                                ; 89 c8
    xchg ah, al                               ; 86 c4
    mov word [bp-01fh], ax                    ; 89 46 e1
    les bx, [bp-014h]                         ; c4 5e ec
    mov word [es:bx+00eh], cx                 ; 26 89 4f 0e
    mov word [es:bx+010h], 00200h             ; 26 c7 47 10 00 02
    mov ax, di                                ; 89 f8
    sal ax, 009h                              ; c1 e0 09
    mov dx, 00800h                            ; ba 00 08
    sub dx, ax                                ; 29 c2
    mov ax, dx                                ; 89 d0
    and ah, 007h                              ; 80 e4 07
    mov word [es:bx+020h], ax                 ; 26 89 47 20
    movzx ax, byte [bp-00ch]                  ; 0f b6 46 f4
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    add bx, ax                                ; 01 c3
    movzx ax, byte [es:bx+022h]               ; 26 0f b6 47 22
    add ax, ax                                ; 01 c0
    mov word [bp-01ah], ax                    ; 89 46 e6
    push word [bp-016h]                       ; ff 76 ea
    push dword 000000001h                     ; 66 6a 01
    mov ax, di                                ; 89 f8
    xor di, di                                ; 31 ff
    mov cx, strict word 00009h                ; b9 09 00
    sal ax, 1                                 ; d1 e0
    rcl di, 1                                 ; d1 d7
    loop 04176h                               ; e2 fa
    push di                                   ; 57
    push ax                                   ; 50
    push strict byte 00000h                   ; 6a 00
    movzx ax, byte [bp-00ch]                  ; 0f b6 46 f4
    mov cx, ss                                ; 8c d1
    lea bx, [bp-026h]                         ; 8d 5e da
    mov dx, strict word 0000ch                ; ba 0c 00
    mov di, word [bp-01ah]                    ; 8b 7e e6
    call word [word di+0006ah]                ; ff 95 6a 00
    les bx, [bp-014h]                         ; c4 5e ec
    mov word [es:bx+020h], strict word 00000h ; 26 c7 47 20 00 00
    test ax, ax                               ; 85 c0
    je short 041a6h                           ; 74 06
    mov ax, strict word 0000dh                ; b8 0d 00
    jmp near 04291h                           ; e9 eb 00
    mov es, [bp-010h]                         ; 8e 46 f0
    mov al, byte [es:si+001h]                 ; 26 8a 44 01
    cmp AL, strict byte 002h                  ; 3c 02
    jc short 041beh                           ; 72 0d
    jbe short 041d6h                          ; 76 23
    cmp AL, strict byte 004h                  ; 3c 04
    je short 041ech                           ; 74 35
    cmp AL, strict byte 003h                  ; 3c 03
    je short 041e1h                           ; 74 26
    jmp near 0423ah                           ; e9 7c 00
    cmp AL, strict byte 001h                  ; 3c 01
    jne short 0423ah                          ; 75 78
    mov es, [bp-010h]                         ; 8e 46 f0
    db  066h, 026h, 0c7h, 044h, 012h, 050h, 000h, 00fh, 000h
    ; mov dword [es:si+012h], strict dword 0000f0050h ; 66 26 c7 44 12 50 00 0f 00
    mov word [es:si+010h], strict word 00002h ; 26 c7 44 10 02 00
    jmp short 0423ah                          ; eb 64
    db  066h, 026h, 0c7h, 044h, 012h, 050h, 000h, 012h, 000h
    ; mov dword [es:si+012h], strict dword 000120050h ; 66 26 c7 44 12 50 00 12 00
    jmp short 041ceh                          ; eb ed
    db  066h, 026h, 0c7h, 044h, 012h, 050h, 000h, 024h, 000h
    ; mov dword [es:si+012h], strict dword 000240050h ; 66 26 c7 44 12 50 00 24 00
    jmp short 041ceh                          ; eb e2
    mov dx, 001c4h                            ; ba c4 01
    mov ax, word [bp-016h]                    ; 8b 46 ea
    call 01749h                               ; e8 54 d5
    and AL, strict byte 03fh                  ; 24 3f
    xor ah, ah                                ; 30 e4
    mov es, [bp-010h]                         ; 8e 46 f0
    mov word [es:si+014h], ax                 ; 26 89 44 14
    mov dx, 001c4h                            ; ba c4 01
    mov ax, word [bp-016h]                    ; 8b 46 ea
    call 01749h                               ; e8 40 d5
    movzx bx, al                              ; 0f b6 d8
    and bl, 0c0h                              ; 80 e3 c0
    sal bx, 002h                              ; c1 e3 02
    mov dx, 001c5h                            ; ba c5 01
    mov ax, word [bp-016h]                    ; 8b 46 ea
    call 01749h                               ; e8 2e d5
    xor ah, ah                                ; 30 e4
    add ax, bx                                ; 01 d8
    inc ax                                    ; 40
    mov es, [bp-010h]                         ; 8e 46 f0
    mov word [es:si+012h], ax                 ; 26 89 44 12
    mov dx, 001c3h                            ; ba c3 01
    mov ax, word [bp-016h]                    ; 8b 46 ea
    call 01749h                               ; e8 19 d5
    xor ah, ah                                ; 30 e4
    inc ax                                    ; 40
    mov es, [bp-010h]                         ; 8e 46 f0
    mov word [es:si+010h], ax                 ; 26 89 44 10
    mov es, [bp-010h]                         ; 8e 46 f0
    cmp byte [es:si+001h], 000h               ; 26 80 7c 01 00
    je short 04278h                           ; 74 34
    cmp byte [es:si+002h], 000h               ; 26 80 7c 02 00
    jne short 04261h                          ; 75 16
    mov dx, strict word 00010h                ; ba 10 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 f5 d4
    or AL, strict byte 041h                   ; 0c 41
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00010h                ; ba 10 00
    mov ax, strict word 00040h                ; b8 40 00
    jmp short 04275h                          ; eb 14
    mov dx, 00304h                            ; ba 04 03
    mov ax, word [bp-018h]                    ; 8b 46 e8
    call 01749h                               ; e8 df d4
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    movzx bx, al                              ; 0f b6 d8
    mov dx, 00304h                            ; ba 04 03
    mov ax, word [bp-018h]                    ; 8b 46 e8
    call 01757h                               ; e8 df d4
    mov es, [bp-010h]                         ; 8e 46 f0
    cmp byte [es:si+001h], 000h               ; 26 80 7c 01 00
    je short 04286h                           ; 74 04
    mov byte [es:si], 001h                    ; 26 c6 04 01
    mov es, [bp-010h]                         ; 8e 46 f0
    movzx ax, byte [es:si+002h]               ; 26 0f b6 44 02
    sal ax, 008h                              ; c1 e0 08
    lea sp, [bp-00ah]                         ; 8d 66 f6
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
    db  050h, 04eh, 049h, 048h, 047h, 046h, 045h, 044h, 043h, 042h, 041h, 018h, 016h, 015h, 014h, 011h
    db  010h, 00dh, 00ch, 00bh, 00ah, 009h, 008h, 005h, 004h, 003h, 002h, 001h, 000h, 08ch, 046h, 09ah
    db  043h, 0e4h, 043h, 00ch, 044h, 0d9h, 043h, 00ch, 044h, 0d9h, 043h, 0e2h, 045h, 0bfh, 043h, 08ch
    db  046h, 08ch, 046h, 0bfh, 043h, 0bfh, 043h, 0bfh, 043h, 0bfh, 043h, 0bfh, 043h, 083h, 046h, 0bfh
    db  043h, 08ch, 046h, 08ch, 046h, 08ch, 046h, 08ch, 046h, 08ch, 046h, 08ch, 046h, 08ch, 046h, 08ch
    db  046h, 08ch, 046h, 08ch, 046h, 08ch, 046h, 08ch, 046h
_int13_cdemu:                                ; 0xf42f4 LB 0x434
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 0002ah                ; 83 ec 2a
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 60 d4
    mov di, 00366h                            ; bf 66 03
    mov cx, ax                                ; 89 c1
    mov si, di                                ; 89 fe
    mov word [bp-008h], ax                    ; 89 46 f8
    mov word [bp-00eh], 00122h                ; c7 46 f2 22 01
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov es, ax                                ; 8e c0
    mov al, byte [es:di+003h]                 ; 26 8a 45 03
    add al, al                                ; 00 c0
    mov byte [bp-006h], al                    ; 88 46 fa
    mov al, byte [es:di+004h]                 ; 26 8a 45 04
    add byte [bp-006h], al                    ; 00 46 fa
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 23 d4
    mov es, cx                                ; 8e c1
    cmp byte [es:di], 000h                    ; 26 80 3d 00
    je short 0434ah                           ; 74 0e
    movzx dx, byte [es:di+002h]               ; 26 0f b6 55 02
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    cmp dx, ax                                ; 39 c2
    je short 04373h                           ; 74 29
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 d5 d6
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 0035eh                               ; 68 5e 03
    push 0036ah                               ; 68 6a 03
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 fe d6
    add sp, strict byte 0000ah                ; 83 c4 0a
    jmp near 046ach                           ; e9 39 03
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    cmp ax, strict word 00050h                ; 3d 50 00
    jnbe near 0468ch                          ; 0f 87 0c 03
    push CS                                   ; 0e
    pop ES                                    ; 07
    mov cx, strict word 0001eh                ; b9 1e 00
    mov di, 0429bh                            ; bf 9b 42
    repne scasb                               ; f2 ae
    sal cx, 1                                 ; d1 e1
    mov di, cx                                ; 89 cf
    mov ax, word [cs:di+042b8h]               ; 2e 8b 85 b8 42
    mov bx, word [bp+016h]                    ; 8b 5e 16
    xor bh, bh                                ; 30 ff
    jmp ax                                    ; ff e0
    movzx bx, byte [bp-006h]                  ; 0f b6 5e fa
    imul bx, bx, strict byte 0001ch           ; 6b db 1c
    mov es, [bp-00ch]                         ; 8e 46 f4
    add bx, word [bp-00eh]                    ; 03 5e f2
    movzx bx, byte [es:bx+022h]               ; 26 0f b6 5f 22
    add bx, bx                                ; 01 db
    cmp word [word bx+0006ah], strict byte 00000h ; 83 bf 6a 00 00
    je near 043bfh                            ; 0f 84 08 00
    movzx ax, byte [bp-006h]                  ; 0f b6 46 fa
    call word [word bx+00076h]                ; ff 97 76 00
    mov byte [bp+017h], 000h                  ; c6 46 17 00
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 89 d3
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 003h                               ; 80 cc 03
    jmp near 046b4h                           ; e9 d0 02
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 5c d3
    mov cl, al                                ; 88 c1
    movzx ax, cl                              ; 0f b6 c1
    sal ax, 008h                              ; c1 e0 08
    or bx, ax                                 ; 09 c3
    mov word [bp+016h], bx                    ; 89 5e 16
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 52 d3
    test cl, cl                               ; 84 c9
    je short 043c3h                           ; 74 ba
    jmp near 046c8h                           ; e9 bc 02
    mov es, [bp-008h]                         ; 8e 46 f8
    mov di, word [es:si+014h]                 ; 26 8b 7c 14
    mov dx, word [es:si+012h]                 ; 26 8b 54 12
    mov bx, word [es:si+010h]                 ; 26 8b 5c 10
    mov ax, word [es:si+008h]                 ; 26 8b 44 08
    mov word [bp-014h], ax                    ; 89 46 ec
    mov ax, word [es:si+00ah]                 ; 26 8b 44 0a
    mov word [bp-012h], ax                    ; 89 46 ee
    mov ax, word [bp+014h]                    ; 8b 46 14
    and ax, strict word 0003fh                ; 25 3f 00
    mov word [bp-010h], ax                    ; 89 46 f0
    mov cx, word [bp+014h]                    ; 8b 4e 14
    and cx, 000c0h                            ; 81 e1 c0 00
    sal cx, 002h                              ; c1 e1 02
    mov ax, word [bp+014h]                    ; 8b 46 14
    shr ax, 008h                              ; c1 e8 08
    or ax, cx                                 ; 09 c8
    mov si, word [bp+012h]                    ; 8b 76 12
    shr si, 008h                              ; c1 ee 08
    mov cx, word [bp+016h]                    ; 8b 4e 16
    xor ch, ch                                ; 30 ed
    mov word [bp-00ah], cx                    ; 89 4e f6
    test cx, cx                               ; 85 c9
    jne short 04459h                          ; 75 03
    jmp near 043bfh                           ; e9 66 ff
    cmp di, word [bp-010h]                    ; 3b 7e f0
    jc near 046ach                            ; 0f 82 4c 02
    cmp ax, dx                                ; 39 d0
    jnc near 046ach                           ; 0f 83 46 02
    cmp si, bx                                ; 39 de
    jnc near 046ach                           ; 0f 83 40 02
    mov dx, word [bp+016h]                    ; 8b 56 16
    shr dx, 008h                              ; c1 ea 08
    cmp dx, strict byte 00004h                ; 83 fa 04
    jne short 0447ah                          ; 75 03
    jmp near 043bfh                           ; e9 45 ff
    mov dx, word [bp+010h]                    ; 8b 56 10
    shr dx, 004h                              ; c1 ea 04
    mov cx, word [bp+006h]                    ; 8b 4e 06
    add cx, dx                                ; 01 d1
    mov word [bp-016h], cx                    ; 89 4e ea
    mov dx, word [bp+010h]                    ; 8b 56 10
    and dx, strict byte 0000fh                ; 83 e2 0f
    mov word [bp-01ch], dx                    ; 89 56 e4
    xor dl, dl                                ; 30 d2
    xor cx, cx                                ; 31 c9
    call 0a090h                               ; e8 f8 5b
    xor bx, bx                                ; 31 db
    add ax, si                                ; 01 f0
    adc dx, bx                                ; 11 da
    mov bx, di                                ; 89 fb
    xor cx, cx                                ; 31 c9
    call 0a090h                               ; e8 eb 5b
    mov bx, ax                                ; 89 c3
    mov ax, word [bp-010h]                    ; 8b 46 f0
    dec ax                                    ; 48
    xor cx, cx                                ; 31 c9
    add ax, bx                                ; 01 d8
    adc dx, cx                                ; 11 ca
    mov bx, word [bp+016h]                    ; 8b 5e 16
    xor bl, bl                                ; 30 db
    mov cx, word [bp-00ah]                    ; 8b 4e f6
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
    add ax, word [bp-00ah]                    ; 03 46 f6
    adc dx, bx                                ; 11 da
    add ax, strict word 0ffffh                ; 05 ff ff
    adc dx, strict byte 0ffffh                ; 83 d2 ff
    mov word [bp-022h], ax                    ; 89 46 de
    mov word [bp-020h], dx                    ; 89 56 e0
    shr word [bp-020h], 1                     ; d1 6e e0
    rcr word [bp-022h], 1                     ; d1 5e de
    shr word [bp-020h], 1                     ; d1 6e e0
    rcr word [bp-022h], 1                     ; d1 5e de
    mov cx, strict word 0000ch                ; b9 0c 00
    mov dx, ss                                ; 8c d2
    lea ax, [bp-02eh]                         ; 8d 46 d2
    call 0a130h                               ; e8 34 5c
    mov word [bp-02eh], strict word 00028h    ; c7 46 d2 28 00
    mov ax, word [bp-014h]                    ; 8b 46 ec
    add ax, si                                ; 01 f0
    mov dx, word [bp-012h]                    ; 8b 56 ee
    adc dx, word [bp-01eh]                    ; 13 56 e2
    xchg ah, al                               ; 86 c4
    xchg dh, dl                               ; 86 d6
    xchg dx, ax                               ; 92
    mov word [bp-02ch], ax                    ; 89 46 d4
    mov word [bp-02ah], dx                    ; 89 56 d6
    mov ax, word [bp-022h]                    ; 8b 46 de
    sub ax, si                                ; 29 f0
    inc ax                                    ; 40
    xchg ah, al                               ; 86 c4
    mov word [bp-027h], ax                    ; 89 46 d9
    mov ax, word [bp-00ah]                    ; 8b 46 f6
    les bx, [bp-00eh]                         ; c4 5e f2
    mov word [es:bx+00eh], ax                 ; 26 89 47 0e
    mov word [es:bx+010h], 00200h             ; 26 c7 47 10 00 02
    mov ax, di                                ; 89 f8
    sal ax, 009h                              ; c1 e0 09
    mov word [es:bx+01eh], ax                 ; 26 89 47 1e
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    xor dh, dh                                ; 30 f6
    and dl, 003h                              ; 80 e2 03
    mov bx, strict word 00004h                ; bb 04 00
    sub bx, dx                                ; 29 d3
    mov dx, bx                                ; 89 da
    sub dx, di                                ; 29 fa
    sal dx, 009h                              ; c1 e2 09
    and dh, 007h                              ; 80 e6 07
    mov bx, word [bp-00eh]                    ; 8b 5e f2
    mov word [es:bx+020h], dx                 ; 26 89 57 20
    movzx dx, byte [bp-006h]                  ; 0f b6 56 fa
    imul dx, dx, strict byte 0001ch           ; 6b d2 1c
    add bx, dx                                ; 01 d3
    movzx dx, byte [es:bx+022h]               ; 26 0f b6 57 22
    add dx, dx                                ; 01 d2
    mov word [bp-01ah], dx                    ; 89 56 e6
    push word [bp-016h]                       ; ff 76 ea
    push word [bp-01ch]                       ; ff 76 e4
    push strict byte 00001h                   ; 6a 01
    mov si, word [bp-00ah]                    ; 8b 76 f6
    xor di, di                                ; 31 ff
    mov cx, strict word 00009h                ; b9 09 00
    sal si, 1                                 ; d1 e6
    rcl di, 1                                 ; d1 d7
    loop 0457ch                               ; e2 fa
    push di                                   ; 57
    push si                                   ; 56
    push ax                                   ; 50
    movzx ax, byte [bp-006h]                  ; 0f b6 46 fa
    mov cx, ss                                ; 8c d1
    lea bx, [bp-02eh]                         ; 8d 5e d2
    mov dx, strict word 0000ch                ; ba 0c 00
    mov si, word [bp-01ah]                    ; 8b 76 e6
    call word [word si+0006ah]                ; ff 94 6a 00
    mov dx, ax                                ; 89 c2
    les bx, [bp-00eh]                         ; c4 5e f2
    db  066h, 026h, 0c7h, 047h, 01eh, 000h, 000h, 000h, 000h
    ; mov dword [es:bx+01eh], strict dword 000000000h ; 66 26 c7 47 1e 00 00 00 00
    test al, al                               ; 84 c0
    je near 043bfh                            ; 0f 84 13 fe
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 73 d4
    movzx ax, dl                              ; 0f b6 c2
    push ax                                   ; 50
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 0035eh                               ; 68 5e 03
    push 003a0h                               ; 68 a0 03
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 9e d4
    add sp, strict byte 0000ah                ; 83 c4 0a
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 002h                               ; 80 cc 02
    mov word [bp+016h], ax                    ; 89 46 16
    mov byte [bp+016h], 000h                  ; c6 46 16 00
    jmp near 046b7h                           ; e9 d5 00
    mov es, [bp-008h]                         ; 8e 46 f8
    mov di, word [es:si+014h]                 ; 26 8b 7c 14
    mov dx, word [es:si+012h]                 ; 26 8b 54 12
    dec dx                                    ; 4a
    mov bx, word [es:si+010h]                 ; 26 8b 5c 10
    dec bx                                    ; 4b
    mov byte [bp+016h], 000h                  ; c6 46 16 00
    mov ax, word [bp+010h]                    ; 8b 46 10
    xor al, al                                ; 30 c0
    mov cx, word [bp+014h]                    ; 8b 4e 14
    xor ch, ch                                ; 30 ed
    mov word [bp-01ah], cx                    ; 89 4e e6
    mov cx, dx                                ; 89 d1
    xor ch, dh                                ; 30 f5
    sal cx, 008h                              ; c1 e1 08
    mov word [bp-018h], cx                    ; 89 4e e8
    mov cx, word [bp-01ah]                    ; 8b 4e e6
    or cx, word [bp-018h]                     ; 0b 4e e8
    mov word [bp+014h], cx                    ; 89 4e 14
    shr dx, 002h                              ; c1 ea 02
    xor dh, dh                                ; 30 f6
    and dl, 0c0h                              ; 80 e2 c0
    and di, strict byte 0003fh                ; 83 e7 3f
    or dx, di                                 ; 09 fa
    xor cl, cl                                ; 30 c9
    or cx, dx                                 ; 09 d1
    mov word [bp+014h], cx                    ; 89 4e 14
    mov dx, word [bp+012h]                    ; 8b 56 12
    xor dh, dh                                ; 30 f6
    sal bx, 008h                              ; c1 e3 08
    or dx, bx                                 ; 09 da
    mov word [bp+012h], dx                    ; 89 56 12
    xor dl, dl                                ; 30 d2
    or dl, 002h                               ; 80 ca 02
    mov word [bp+012h], dx                    ; 89 56 12
    mov dl, byte [es:si+001h]                 ; 26 8a 54 01
    mov word [bp+010h], ax                    ; 89 46 10
    cmp dl, 003h                              ; 80 fa 03
    je short 04666h                           ; 74 1a
    cmp dl, 002h                              ; 80 fa 02
    je short 04662h                           ; 74 11
    cmp dl, 001h                              ; 80 fa 01
    jne short 0466ah                          ; 75 14
    mov ax, word [bp+010h]                    ; 8b 46 10
    xor al, al                                ; 30 c0
    or AL, strict byte 002h                   ; 0c 02
    mov word [bp+010h], ax                    ; 89 46 10
    jmp short 0466ah                          ; eb 08
    or AL, strict byte 004h                   ; 0c 04
    jmp short 0465dh                          ; eb f7
    or AL, strict byte 005h                   ; 0c 05
    jmp short 0465dh                          ; eb f3
    mov es, [bp-008h]                         ; 8e 46 f8
    cmp byte [es:si+001h], 004h               ; 26 80 7c 01 04
    jnc near 043bfh                           ; 0f 83 49 fd
    mov word [bp+008h], 0efc7h                ; c7 46 08 c7 ef
    mov word [bp+006h], 0f000h                ; c7 46 06 00 f0
    jmp near 043bfh                           ; e9 3c fd
    or bh, 003h                               ; 80 cf 03
    mov word [bp+016h], bx                    ; 89 5e 16
    jmp near 043c3h                           ; e9 37 fd
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 93 d3
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 0035eh                               ; 68 5e 03
    push 003c1h                               ; 68 c1 03
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 c2 d3
    add sp, strict byte 00008h                ; 83 c4 08
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 001h                               ; 80 cc 01
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, word [bp+016h]                    ; 8b 5e 16
    shr bx, 008h                              ; c1 eb 08
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 8f d0
    or byte [bp+01ch], 001h                   ; 80 4e 1c 01
    jmp near 043d2h                           ; e9 03 fd
    db  050h, 04eh, 049h, 048h, 047h, 046h, 045h, 044h, 043h, 042h, 041h, 018h, 016h, 015h, 014h, 011h
    db  010h, 00dh, 00ch, 00bh, 00ah, 009h, 008h, 005h, 004h, 003h, 002h, 001h, 000h, 06dh, 04ch, 0e9h
    db  049h, 0d5h, 047h, 06dh, 04ch, 0cah, 047h, 06dh, 04ch, 0cah, 047h, 06dh, 04ch, 0e9h, 049h, 06dh
    db  04ch, 06dh, 04ch, 0e9h, 049h, 0e9h, 049h, 0e9h, 049h, 0e9h, 049h, 0e9h, 049h, 0ffh, 047h, 0e9h
    db  049h, 06dh, 04ch, 008h, 048h, 01bh, 048h, 0cah, 047h, 01bh, 048h, 049h, 049h, 003h, 04ah, 01bh
    db  048h, 02ah, 04ah, 026h, 04ch, 02eh, 04ch, 06dh, 04ch
_int13_cdrom:                                ; 0xf4728 LB 0x562
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00028h                ; 83 ec 28
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 2c d0
    mov word [bp-01ah], ax                    ; 89 46 e6
    mov si, 00122h                            ; be 22 01
    mov word [bp-00ch], ax                    ; 89 46 f4
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 0a d0
    mov ax, word [bp+010h]                    ; 8b 46 10
    xor ah, ah                                ; 30 e4
    cmp ax, 000e0h                            ; 3d e0 00
    jc short 0475ch                           ; 72 05
    cmp ax, 000f0h                            ; 3d f0 00
    jc short 0477ah                           ; 72 1e
    mov ax, word [bp+010h]                    ; 8b 46 10
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, word [bp+018h]                    ; 8b 46 18
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 003f1h                               ; 68 f1 03
    push 003fdh                               ; 68 fd 03
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 f7 d2
    add sp, strict byte 0000ah                ; 83 c4 0a
    jmp near 04c4ah                           ; e9 d0 04
    mov ax, word [bp+010h]                    ; 8b 46 10
    xor ah, ah                                ; 30 e4
    mov es, [bp-00ch]                         ; 8e 46 f4
    mov bx, si                                ; 89 f3
    add bx, ax                                ; 01 c3
    mov dl, byte [es:bx+00114h]               ; 26 8a 97 14 01
    mov byte [bp-008h], dl                    ; 88 56 f8
    cmp dl, 010h                              ; 80 fa 10
    jc short 047a3h                           ; 72 10
    push ax                                   ; 50
    mov ax, word [bp+018h]                    ; 8b 46 18
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 003f1h                               ; 68 f1 03
    push 00428h                               ; 68 28 04
    jmp short 0476fh                          ; eb cc
    mov ax, word [bp+018h]                    ; 8b 46 18
    shr ax, 008h                              ; c1 e8 08
    cmp ax, strict word 00050h                ; 3d 50 00
    jnbe near 04c6dh                          ; 0f 87 bd 04
    push CS                                   ; 0e
    pop ES                                    ; 07
    mov cx, strict word 0001eh                ; b9 1e 00
    mov di, 046cfh                            ; bf cf 46
    repne scasb                               ; f2 ae
    sal cx, 1                                 ; d1 e1
    mov di, cx                                ; 89 cf
    mov ax, word [cs:di+046ech]               ; 2e 8b 85 ec 46
    mov bx, word [bp+018h]                    ; 8b 5e 18
    xor bh, bh                                ; 30 ff
    jmp ax                                    ; ff e0
    mov ax, word [bp+018h]                    ; 8b 46 18
    xor ah, ah                                ; 30 e4
    or ah, 003h                               ; 80 cc 03
    jmp near 04c52h                           ; e9 7d 04
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 6b cf
    mov cl, al                                ; 88 c1
    movzx ax, cl                              ; 0f b6 c1
    sal ax, 008h                              ; c1 e0 08
    or bx, ax                                 ; 09 c3
    mov word [bp+018h], bx                    ; 89 5e 18
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 61 cf
    test cl, cl                               ; 84 c9
    je near 049edh                            ; 0f 84 f1 01
    jmp near 04c66h                           ; e9 67 04
    or bh, 002h                               ; 80 cf 02
    mov word [bp+018h], bx                    ; 89 5e 18
    jmp near 04c55h                           ; e9 4d 04
    mov word [bp+012h], 0aa55h                ; c7 46 12 55 aa
    or bh, 030h                               ; 80 cf 30
    mov word [bp+018h], bx                    ; 89 5e 18
    mov word [bp+016h], strict word 00007h    ; c7 46 16 07 00
    jmp near 049edh                           ; e9 d2 01
    mov bx, word [bp+00ch]                    ; 8b 5e 0c
    mov es, [bp+006h]                         ; 8e 46 06
    mov word [bp-014h], bx                    ; 89 5e ec
    mov [bp-012h], es                         ; 8c 46 ee
    mov ax, word [es:bx+002h]                 ; 26 8b 47 02
    mov word [bp-010h], ax                    ; 89 46 f0
    mov ax, word [es:bx+006h]                 ; 26 8b 47 06
    mov word [bp-01ch], ax                    ; 89 46 e4
    mov ax, word [es:bx+004h]                 ; 26 8b 47 04
    mov word [bp-020h], ax                    ; 89 46 e0
    mov ax, word [es:bx+00ch]                 ; 26 8b 47 0c
    mov word [bp-018h], ax                    ; 89 46 e8
    mov di, word [es:bx+00eh]                 ; 26 8b 7f 0e
    or di, ax                                 ; 09 c7
    je short 04863h                           ; 74 18
    mov ax, word [bp+018h]                    ; 8b 46 18
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 003f1h                               ; 68 f1 03
    push 0045ah                               ; 68 5a 04
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 0e d2
    add sp, strict byte 00008h                ; 83 c4 08
    jmp near 04c4ah                           ; e9 e7 03
    les bx, [bp-014h]                         ; c4 5e ec
    mov ax, word [es:bx+008h]                 ; 26 8b 47 08
    mov word [bp-018h], ax                    ; 89 46 e8
    mov di, bx                                ; 89 df
    mov di, word [es:di+00ah]                 ; 26 8b 7d 0a
    mov ax, word [bp+018h]                    ; 8b 46 18
    shr ax, 008h                              ; c1 e8 08
    mov word [bp-016h], ax                    ; 89 46 ea
    cmp ax, strict word 00044h                ; 3d 44 00
    je near 049e9h                            ; 0f 84 66 01
    cmp ax, strict word 00047h                ; 3d 47 00
    je near 049e9h                            ; 0f 84 5f 01
    mov cx, strict word 0000ch                ; b9 0c 00
    xor bx, bx                                ; 31 db
    mov dx, ss                                ; 8c d2
    lea ax, [bp-02ch]                         ; 8d 46 d4
    call 0a130h                               ; e8 99 58
    mov word [bp-02ch], strict word 00028h    ; c7 46 d4 28 00
    mov ax, word [bp-018h]                    ; 8b 46 e8
    mov dx, di                                ; 89 fa
    xchg ah, al                               ; 86 c4
    xchg dh, dl                               ; 86 d6
    xchg dx, ax                               ; 92
    mov word [bp-02ah], ax                    ; 89 46 d6
    mov word [bp-028h], dx                    ; 89 56 d8
    mov ax, word [bp-010h]                    ; 8b 46 f0
    xchg ah, al                               ; 86 c4
    mov word [bp-025h], ax                    ; 89 46 db
    mov ax, word [bp-010h]                    ; 8b 46 f0
    mov es, [bp-00ch]                         ; 8e 46 f4
    mov word [es:si+00eh], ax                 ; 26 89 44 0e
    mov word [es:si+010h], 00800h             ; 26 c7 44 10 00 08
    movzx bx, byte [bp-008h]                  ; 0f b6 5e f8
    imul bx, bx, strict byte 0001ch           ; 6b db 1c
    add bx, si                                ; 01 f3
    movzx di, byte [es:bx+022h]               ; 26 0f b6 7f 22
    add di, di                                ; 01 ff
    push word [bp-01ch]                       ; ff 76 e4
    push word [bp-020h]                       ; ff 76 e0
    push strict byte 00001h                   ; 6a 01
    xor bx, bx                                ; 31 db
    mov cx, strict word 0000bh                ; b9 0b 00
    sal ax, 1                                 ; d1 e0
    rcl bx, 1                                 ; d1 d3
    loop 048e1h                               ; e2 fa
    push bx                                   ; 53
    push ax                                   ; 50
    push strict byte 00000h                   ; 6a 00
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    mov cx, ss                                ; 8c d1
    lea bx, [bp-02ch]                         ; 8d 5e d4
    mov dx, strict word 0000ch                ; ba 0c 00
    call word [word di+0006ah]                ; ff 95 6a 00
    mov dx, ax                                ; 89 c2
    mov es, [bp-00ch]                         ; 8e 46 f4
    mov ax, word [es:si+01ah]                 ; 26 8b 44 1a
    mov di, word [es:si+01ch]                 ; 26 8b 7c 1c
    mov cx, strict word 0000bh                ; b9 0b 00
    shr di, 1                                 ; d1 ef
    rcr ax, 1                                 ; d1 d8
    loop 0490bh                               ; e2 fa
    les bx, [bp-014h]                         ; c4 5e ec
    mov word [es:bx+002h], ax                 ; 26 89 47 02
    test dl, dl                               ; 84 d2
    je near 049e9h                            ; 0f 84 cb 00
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 01 d1
    movzx ax, dl                              ; 0f b6 c2
    push ax                                   ; 50
    push word [bp-016h]                       ; ff 76 ea
    push 003f1h                               ; 68 f1 03
    push 00483h                               ; 68 83 04
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 30 d1
    add sp, strict byte 0000ah                ; 83 c4 0a
    mov ax, word [bp+018h]                    ; 8b 46 18
    xor ah, ah                                ; 30 e4
    or ah, 00ch                               ; 80 cc 0c
    jmp near 04c52h                           ; e9 09 03
    cmp bx, strict byte 00002h                ; 83 fb 02
    jnbe near 04c4ah                          ; 0f 87 fa 02
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov es, [bp-00ch]                         ; 8e 46 f4
    mov di, si                                ; 89 f7
    add di, ax                                ; 01 c7
    mov al, byte [es:di+025h]                 ; 26 8a 45 25
    cmp bx, strict byte 00002h                ; 83 fb 02
    je short 049dah                           ; 74 73
    cmp bx, strict byte 00001h                ; 83 fb 01
    je short 049a7h                           ; 74 3b
    test bx, bx                               ; 85 db
    jne near 049e9h                           ; 0f 85 77 00
    cmp AL, strict byte 0ffh                  ; 3c ff
    jne short 04988h                          ; 75 12
    mov ax, word [bp+018h]                    ; 8b 46 18
    xor ah, ah                                ; 30 e4
    or ah, 0b4h                               ; 80 cc b4
    mov word [bp+018h], ax                    ; 89 46 18
    xor al, al                                ; 30 c0
    or AL, strict byte 001h                   ; 0c 01
    jmp near 04c52h                           ; e9 ca 02
    movzx dx, byte [bp-008h]                  ; 0f b6 56 f8
    imul dx, dx, strict byte 0001ch           ; 6b d2 1c
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    mov es, [bp-00ch]                         ; 8e 46 f4
    add si, dx                                ; 01 d6
    mov byte [es:si+025h], al                 ; 26 88 44 25
    mov ax, word [bp+018h]                    ; 8b 46 18
    xor al, al                                ; 30 c0
    or AL, strict byte 001h                   ; 0c 01
    mov word [bp+018h], ax                    ; 89 46 18
    jmp near 049e9h                           ; e9 42 00
    test al, al                               ; 84 c0
    jne short 049b7h                          ; 75 0c
    or bh, 0b0h                               ; 80 cf b0
    mov word [bp+018h], bx                    ; 89 5e 18
    mov byte [bp+018h], al                    ; 88 46 18
    jmp near 04c55h                           ; e9 9e 02
    movzx dx, byte [bp-008h]                  ; 0f b6 56 f8
    imul dx, dx, strict byte 0001ch           ; 6b d2 1c
    db  0feh, 0c8h
    ; dec al                                    ; fe c8
    mov es, [bp-00ch]                         ; 8e 46 f4
    add si, dx                                ; 01 d6
    mov byte [es:si+025h], al                 ; 26 88 44 25
    test al, al                               ; 84 c0
    db  00fh, 095h, 0c0h
    ; setne al                                  ; 0f 95 c0
    movzx dx, al                              ; 0f b6 d0
    mov ax, word [bp+018h]                    ; 8b 46 18
    xor al, al                                ; 30 c0
    or ax, dx                                 ; 09 d0
    jmp short 049a1h                          ; eb c7
    test al, al                               ; 84 c0
    db  00fh, 095h, 0c0h
    ; setne al                                  ; 0f 95 c0
    xor ah, ah                                ; 30 e4
    mov dx, word [bp+018h]                    ; 8b 56 18
    mov dl, al                                ; 88 c2
    mov word [bp+018h], dx                    ; 89 56 18
    mov byte [bp+019h], 000h                  ; c6 46 19 00
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 5f cd
    and byte [bp+01eh], 0feh                  ; 80 66 1e fe
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov es, [bp-00ch]                         ; 8e 46 f4
    add si, ax                                ; 01 c6
    mov al, byte [es:si+025h]                 ; 26 8a 44 25
    test al, al                               ; 84 c0
    je short 04a1dh                           ; 74 06
    or bh, 0b1h                               ; 80 cf b1
    jmp near 04802h                           ; e9 e5 fd
    je short 049e9h                           ; 74 ca
    mov ax, word [bp+018h]                    ; 8b 46 18
    xor ah, ah                                ; 30 e4
    or ah, 0b1h                               ; 80 cc b1
    jmp near 04c52h                           ; e9 28 02
    mov dx, word [bp+00ch]                    ; 8b 56 0c
    mov cx, word [bp+006h]                    ; 8b 4e 06
    mov bx, dx                                ; 89 d3
    mov word [bp-00ah], cx                    ; 89 4e f6
    mov es, cx                                ; 8e c1
    mov di, dx                                ; 89 d7
    mov ax, word [es:di]                      ; 26 8b 05
    mov word [bp-00eh], ax                    ; 89 46 f2
    cmp ax, strict word 0001ah                ; 3d 1a 00
    jc near 04c4ah                            ; 0f 82 04 02
    jc short 04a97h                           ; 72 4f
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov es, [bp-00ch]                         ; 8e 46 f4
    mov di, si                                ; 89 f7
    add di, ax                                ; 01 c7
    mov ax, word [es:di+028h]                 ; 26 8b 45 28
    mov es, cx                                ; 8e c1
    mov di, dx                                ; 89 d7
    db  066h, 026h, 0c7h, 005h, 01ah, 000h, 074h, 000h
    ; mov dword [es:di], strict dword 00074001ah ; 66 26 c7 05 1a 00 74 00
    db  066h, 026h, 0c7h, 045h, 004h, 0ffh, 0ffh, 0ffh, 0ffh
    ; mov dword [es:di+004h], strict dword 0ffffffffh ; 66 26 c7 45 04 ff ff ff ff
    db  066h, 026h, 0c7h, 045h, 008h, 0ffh, 0ffh, 0ffh, 0ffh
    ; mov dword [es:di+008h], strict dword 0ffffffffh ; 66 26 c7 45 08 ff ff ff ff
    db  066h, 026h, 0c7h, 045h, 00ch, 0ffh, 0ffh, 0ffh, 0ffh
    ; mov dword [es:di+00ch], strict dword 0ffffffffh ; 66 26 c7 45 0c ff ff ff ff
    mov word [es:di+018h], ax                 ; 26 89 45 18
    db  066h, 026h, 0c7h, 045h, 010h, 0ffh, 0ffh, 0ffh, 0ffh
    ; mov dword [es:di+010h], strict dword 0ffffffffh ; 66 26 c7 45 10 ff ff ff ff
    db  066h, 026h, 0c7h, 045h, 014h, 0ffh, 0ffh, 0ffh, 0ffh
    ; mov dword [es:di+014h], strict dword 0ffffffffh ; 66 26 c7 45 14 ff ff ff ff
    cmp word [bp-00eh], strict byte 0001eh    ; 83 7e f2 1e
    jc near 04b6eh                            ; 0f 82 cf 00
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov word [es:bx], strict word 0001eh      ; 26 c7 07 1e 00
    mov ax, word [bp-01ah]                    ; 8b 46 e6
    mov word [es:bx+01ch], ax                 ; 26 89 47 1c
    mov word [es:bx+01ah], 00356h             ; 26 c7 47 1a 56 03
    movzx cx, byte [bp-008h]                  ; 0f b6 4e f8
    mov ax, cx                                ; 89 c8
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    movzx di, al                              ; 0f b6 f8
    imul di, di, strict byte 00006h           ; 6b ff 06
    mov es, [bp-00ch]                         ; 8e 46 f4
    add di, si                                ; 01 f7
    mov ax, word [es:di+00206h]               ; 26 8b 85 06 02
    mov word [bp-01eh], ax                    ; 89 46 e2
    mov dx, word [es:di+00208h]               ; 26 8b 95 08 02
    mov al, byte [es:di+00205h]               ; 26 8a 85 05 02
    mov byte [bp-006h], al                    ; 88 46 fa
    imul cx, cx, strict byte 0001ch           ; 6b c9 1c
    mov di, si                                ; 89 f7
    add di, cx                                ; 01 cf
    mov al, byte [es:di+026h]                 ; 26 8a 45 26
    cmp AL, strict byte 001h                  ; 3c 01
    db  00fh, 094h, 0c0h
    ; sete al                                   ; 0f 94 c0
    xor ah, ah                                ; 30 e4
    or AL, strict byte 070h                   ; 0c 70
    mov di, ax                                ; 89 c7
    mov ax, word [bp-01eh]                    ; 8b 46 e2
    mov word [es:si+00234h], ax               ; 26 89 84 34 02
    mov word [es:si+00236h], dx               ; 26 89 94 36 02
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    cwd                                       ; 99
    mov cx, strict word 00002h                ; b9 02 00
    idiv cx                                   ; f7 f9
    or dl, 00eh                               ; 80 ca 0e
    mov ax, dx                                ; 89 d0
    sal ax, 004h                              ; c1 e0 04
    mov byte [es:si+00238h], al               ; 26 88 84 38 02
    mov byte [es:si+00239h], 0cbh             ; 26 c6 84 39 02 cb
    mov al, byte [bp-006h]                    ; 8a 46 fa
    mov byte [es:si+0023ah], al               ; 26 88 84 3a 02
    mov word [es:si+0023bh], strict word 00001h ; 26 c7 84 3b 02 01 00
    mov byte [es:si+0023dh], 000h             ; 26 c6 84 3d 02 00
    mov word [es:si+0023eh], di               ; 26 89 bc 3e 02
    mov word [es:si+00240h], strict word 00000h ; 26 c7 84 40 02 00 00
    mov byte [es:si+00242h], 011h             ; 26 c6 84 42 02 11
    xor cl, cl                                ; 30 c9
    xor ch, ch                                ; 30 ed
    jmp short 04b51h                          ; eb 05
    cmp ch, 00fh                              ; 80 fd 0f
    jnc short 04b64h                          ; 73 13
    movzx dx, ch                              ; 0f b6 d5
    add dx, 00356h                            ; 81 c2 56 03
    mov ax, word [bp-01ah]                    ; 8b 46 e6
    call 01749h                               ; e8 eb cb
    add cl, al                                ; 00 c1
    db  0feh, 0c5h
    ; inc ch                                    ; fe c5
    jmp short 04b4ch                          ; eb e8
    neg cl                                    ; f6 d9
    mov es, [bp-00ch]                         ; 8e 46 f4
    mov byte [es:si+00243h], cl               ; 26 88 8c 43 02
    cmp word [bp-00eh], strict byte 00042h    ; 83 7e f2 42
    jc near 049e9h                            ; 0f 82 73 fe
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    xor ah, ah                                ; 30 e4
    imul ax, ax, strict byte 00006h           ; 6b c0 06
    mov es, [bp-00ch]                         ; 8e 46 f4
    add si, ax                                ; 01 c6
    mov al, byte [es:si+00204h]               ; 26 8a 84 04 02
    mov dx, word [es:si+00206h]               ; 26 8b 94 06 02
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov word [es:bx], strict word 00042h      ; 26 c7 07 42 00
    db  066h, 026h, 0c7h, 047h, 01eh, 0ddh, 0beh, 024h, 000h
    ; mov dword [es:bx+01eh], strict dword 00024beddh ; 66 26 c7 47 1e dd be 24 00
    mov word [es:bx+022h], strict word 00000h ; 26 c7 47 22 00 00
    test al, al                               ; 84 c0
    jne short 04bb7h                          ; 75 09
    db  066h, 026h, 0c7h, 047h, 024h, 049h, 053h, 041h, 020h
    ; mov dword [es:bx+024h], strict dword 020415349h ; 66 26 c7 47 24 49 53 41 20
    mov es, [bp-00ah]                         ; 8e 46 f6
    db  066h, 026h, 0c7h, 047h, 028h, 041h, 054h, 041h, 020h
    ; mov dword [es:bx+028h], strict dword 020415441h ; 66 26 c7 47 28 41 54 41 20
    db  066h, 026h, 0c7h, 047h, 02ch, 020h, 020h, 020h, 020h
    ; mov dword [es:bx+02ch], strict dword 020202020h ; 66 26 c7 47 2c 20 20 20 20
    test al, al                               ; 84 c0
    jne short 04be3h                          ; 75 13
    mov word [es:bx+030h], dx                 ; 26 89 57 30
    db  066h, 026h, 0c7h, 047h, 032h, 000h, 000h, 000h, 000h
    ; mov dword [es:bx+032h], strict dword 000000000h ; 66 26 c7 47 32 00 00 00 00
    mov word [es:bx+036h], strict word 00000h ; 26 c7 47 36 00 00
    mov al, byte [bp-008h]                    ; 8a 46 f8
    and AL, strict byte 001h                  ; 24 01
    xor ah, ah                                ; 30 e4
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov word [es:bx+038h], ax                 ; 26 89 47 38
    db  066h, 026h, 0c7h, 047h, 03ah, 000h, 000h, 000h, 000h
    ; mov dword [es:bx+03ah], strict dword 000000000h ; 66 26 c7 47 3a 00 00 00 00
    mov word [es:bx+03eh], strict word 00000h ; 26 c7 47 3e 00 00
    xor al, al                                ; 30 c0
    mov AH, strict byte 01eh                  ; b4 1e
    jmp short 04c0bh                          ; eb 05
    cmp ah, 040h                              ; 80 fc 40
    jnc short 04c1ah                          ; 73 0f
    movzx si, ah                              ; 0f b6 f4
    mov es, [bp-00ah]                         ; 8e 46 f6
    add si, bx                                ; 01 de
    add al, byte [es:si]                      ; 26 02 04
    db  0feh, 0c4h
    ; inc ah                                    ; fe c4
    jmp short 04c06h                          ; eb ec
    neg al                                    ; f6 d8
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov byte [es:bx+041h], al                 ; 26 88 47 41
    jmp near 049e9h                           ; e9 c3 fd
    or bh, 006h                               ; 80 cf 06
    mov word [bp+018h], bx                    ; 89 5e 18
    jmp short 04c66h                          ; eb 38
    cmp bx, strict byte 00006h                ; 83 fb 06
    je near 049e9h                            ; 0f 84 b4 fd
    cmp bx, strict byte 00001h                ; 83 fb 01
    jc short 04c4ah                           ; 72 10
    jbe near 049e9h                           ; 0f 86 ab fd
    cmp bx, strict byte 00003h                ; 83 fb 03
    jc short 04c4ah                           ; 72 07
    cmp bx, strict byte 00004h                ; 83 fb 04
    jbe near 049e9h                           ; 0f 86 9f fd
    mov ax, word [bp+018h]                    ; 8b 46 18
    xor ah, ah                                ; 30 e4
    or ah, 001h                               ; 80 cc 01
    mov word [bp+018h], ax                    ; 89 46 18
    mov bx, word [bp+018h]                    ; 8b 5e 18
    shr bx, 008h                              ; c1 eb 08
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 f1 ca
    or byte [bp+01eh], 001h                   ; 80 4e 1e 01
    jmp near 049fch                           ; e9 8f fd
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 b2 cd
    mov ax, word [bp+018h]                    ; 8b 46 18
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 003f1h                               ; 68 f1 03
    push 00345h                               ; 68 45 03
    push strict byte 00004h                   ; 6a 04
    jmp near 0485ah                           ; e9 d0 fb
print_boot_device_:                          ; 0xf4c8a LB 0x4b
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push cx                                   ; 51
    test al, al                               ; 84 c0
    je short 04c97h                           ; 74 05
    mov dx, strict word 00002h                ; ba 02 00
    jmp short 04cb1h                          ; eb 1a
    test dl, dl                               ; 84 d2
    je short 04ca0h                           ; 74 05
    mov dx, strict word 00003h                ; ba 03 00
    jmp short 04cb1h                          ; eb 11
    test bl, 080h                             ; f6 c3 80
    jne short 04ca9h                          ; 75 04
    xor dh, dh                                ; 30 f6
    jmp short 04cb1h                          ; eb 08
    test bl, 080h                             ; f6 c3 80
    je short 04ccfh                           ; 74 21
    mov dx, strict word 00001h                ; ba 01 00
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 6e cd
    imul dx, dx, strict byte 0000ah           ; 6b d2 0a
    add dx, 00dd8h                            ; 81 c2 d8 0d
    push dx                                   ; 52
    push 004a6h                               ; 68 a6 04
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 9f cd
    add sp, strict byte 00006h                ; 83 c4 06
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop cx                                    ; 59
    pop bp                                    ; 5d
    retn                                      ; c3
print_boot_failure_:                         ; 0xf4cd5 LB 0x93
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    mov dh, cl                                ; 88 ce
    mov ah, bl                                ; 88 dc
    and ah, 07fh                              ; 80 e4 7f
    movzx si, ah                              ; 0f b6 f4
    test al, al                               ; 84 c0
    je short 04d02h                           ; 74 1b
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 38 cd
    push 00dech                               ; 68 ec 0d
    push 004bah                               ; 68 ba 04
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 6e cd
    add sp, strict byte 00006h                ; 83 c4 06
    jmp short 04d46h                          ; eb 44
    test dl, dl                               ; 84 d2
    je short 04d16h                           ; 74 10
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 19 cd
    push 00df6h                               ; 68 f6 0d
    jmp short 04cf5h                          ; eb df
    test bl, 080h                             ; f6 c3 80
    je short 04d2ch                           ; 74 11
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 04 cd
    push si                                   ; 56
    push 00de2h                               ; 68 e2 0d
    jmp short 04d3bh                          ; eb 0f
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 f3 cc
    push si                                   ; 56
    push 00dd8h                               ; 68 d8 0d
    push 004cfh                               ; 68 cf 04
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 28 cd
    add sp, strict byte 00008h                ; 83 c4 08
    cmp byte [bp+004h], 001h                  ; 80 7e 04 01
    jne short 04d60h                          ; 75 14
    test dh, dh                               ; 84 f6
    jne short 04d55h                          ; 75 05
    push 004e7h                               ; 68 e7 04
    jmp short 04d58h                          ; eb 03
    push 00511h                               ; 68 11 05
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 0e cd
    add sp, strict byte 00004h                ; 83 c4 04
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 00002h                               ; c2 02 00
print_cdromboot_failure_:                    ; 0xf4d68 LB 0x27
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    mov dx, ax                                ; 89 c2
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 af cc
    push dx                                   ; 52
    push 00546h                               ; 68 46 05
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 e7 cc
    add sp, strict byte 00006h                ; 83 c4 06
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
_int19_function:                             ; 0xf4d8f LB 0x256
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 0000eh                ; 83 ec 0e
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 c5 c9
    mov bx, ax                                ; 89 c3
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov byte [bp-008h], 000h                  ; c6 46 f8 00
    mov ax, strict word 0003dh                ; b8 3d 00
    call 017a5h                               ; e8 f6 c9
    movzx si, al                              ; 0f b6 f0
    mov ax, strict word 00038h                ; b8 38 00
    call 017a5h                               ; e8 ed c9
    and AL, strict byte 0f0h                  ; 24 f0
    xor ah, ah                                ; 30 e4
    sal ax, 004h                              ; c1 e0 04
    or si, ax                                 ; 09 c6
    mov ax, strict word 0003ch                ; b8 3c 00
    call 017a5h                               ; e8 de c9
    and AL, strict byte 00fh                  ; 24 0f
    xor ah, ah                                ; 30 e4
    sal ax, 00ch                              ; c1 e0 0c
    or si, ax                                 ; 09 c6
    mov dx, 0037dh                            ; ba 7d 03
    mov ax, bx                                ; 89 d8
    call 01749h                               ; e8 71 c9
    test al, al                               ; 84 c0
    je short 04de7h                           ; 74 0b
    mov dx, 0037dh                            ; ba 7d 03
    mov ax, bx                                ; 89 d8
    call 01749h                               ; e8 65 c9
    movzx si, al                              ; 0f b6 f0
    cmp byte [bp+004h], 001h                  ; 80 7e 04 01
    jne short 04dfdh                          ; 75 10
    mov ax, strict word 0003ch                ; b8 3c 00
    call 017a5h                               ; e8 b2 c9
    and AL, strict byte 0f0h                  ; 24 f0
    xor ah, ah                                ; 30 e4
    sar ax, 004h                              ; c1 f8 04
    call 07e6fh                               ; e8 72 30
    cmp byte [bp+004h], 002h                  ; 80 7e 04 02
    jne short 04e06h                          ; 75 03
    shr si, 004h                              ; c1 ee 04
    cmp byte [bp+004h], 003h                  ; 80 7e 04 03
    jne short 04e0fh                          ; 75 03
    shr si, 008h                              ; c1 ee 08
    cmp byte [bp+004h], 004h                  ; 80 7e 04 04
    jne short 04e18h                          ; 75 03
    shr si, 00ch                              ; c1 ee 0c
    cmp si, strict byte 00010h                ; 83 fe 10
    jnc short 04e21h                          ; 73 04
    mov byte [bp-008h], 001h                  ; c6 46 f8 01
    xor al, al                                ; 30 c0
    mov byte [bp-006h], al                    ; 88 46 fa
    mov byte [bp-00ah], al                    ; 88 46 f6
    mov byte [bp-00ch], al                    ; 88 46 f4
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 f3 cb
    push si                                   ; 56
    movzx ax, byte [bp+004h]                  ; 0f b6 46 04
    push ax                                   ; 50
    push 00566h                               ; 68 66 05
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 26 cc
    add sp, strict byte 00008h                ; 83 c4 08
    and si, strict byte 0000fh                ; 83 e6 0f
    cmp si, strict byte 00002h                ; 83 fe 02
    jc short 04e5eh                           ; 72 0e
    jbe short 04e6dh                          ; 76 1b
    cmp si, strict byte 00004h                ; 83 fe 04
    je short 04e8bh                           ; 74 34
    cmp si, strict byte 00003h                ; 83 fe 03
    je short 04e81h                           ; 74 25
    jmp short 04eb8h                          ; eb 5a
    cmp si, strict byte 00001h                ; 83 fe 01
    jne short 04eb8h                          ; 75 55
    xor al, al                                ; 30 c0
    mov byte [bp-006h], al                    ; 88 46 fa
    mov byte [bp-00ah], al                    ; 88 46 f6
    jmp short 04ed0h                          ; eb 63
    mov dx, 0037ch                            ; ba 7c 03
    mov ax, word [bp-00eh]                    ; 8b 46 f2
    call 01749h                               ; e8 d3 c8
    add AL, strict byte 080h                  ; 04 80
    mov byte [bp-006h], al                    ; 88 46 fa
    mov byte [bp-00ah], 000h                  ; c6 46 f6 00
    jmp short 04ed0h                          ; eb 4f
    mov byte [bp-006h], 000h                  ; c6 46 fa 00
    mov byte [bp-00ah], 001h                  ; c6 46 f6 01
    jmp short 04e95h                          ; eb 0a
    mov byte [bp-00ch], 001h                  ; c6 46 f4 01
    cmp byte [bp-00ah], 000h                  ; 80 7e f6 00
    je short 04ed0h                           ; 74 3b
    call 03eddh                               ; e8 45 f0
    mov bx, ax                                ; 89 c3
    test AL, strict byte 0ffh                 ; a8 ff
    je short 04ebfh                           ; 74 21
    call 04d68h                               ; e8 c7 fe
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    push ax                                   ; 50
    movzx bx, byte [bp-006h]                  ; 0f b6 5e fa
    movzx dx, byte [bp-00ch]                  ; 0f b6 56 f4
    movzx ax, byte [bp-00ah]                  ; 0f b6 46 f6
    mov cx, strict word 00001h                ; b9 01 00
    call 04cd5h                               ; e8 1d fe
    xor ax, ax                                ; 31 c0
    xor dx, dx                                ; 31 d2
    jmp near 04fdeh                           ; e9 1f 01
    mov dx, 00372h                            ; ba 72 03
    mov ax, word [bp-00eh]                    ; 8b 46 f2
    call 01765h                               ; e8 9d c8
    mov di, ax                                ; 89 c7
    shr bx, 008h                              ; c1 eb 08
    mov byte [bp-006h], bl                    ; 88 5e fa
    cmp byte [bp-00ch], 001h                  ; 80 7e f4 01
    jne near 04f4ch                           ; 0f 85 74 00
    xor si, si                                ; 31 f6
    mov ax, 0e200h                            ; b8 00 e2
    mov es, ax                                ; 8e c0
    cmp word [es:si], 0aa55h                  ; 26 81 3c 55 aa
    jne short 04ea1h                          ; 75 bb
    mov cx, ax                                ; 89 c1
    mov si, word [es:si+01ah]                 ; 26 8b 74 1a
    cmp word [es:si+002h], 0506eh             ; 26 81 7c 02 6e 50
    jne short 04ea1h                          ; 75 ad
    cmp word [es:si], 05024h                  ; 26 81 3c 24 50
    jne short 04ea1h                          ; 75 a6
    mov di, word [es:si+00eh]                 ; 26 8b 7c 0e
    mov dx, word [es:di]                      ; 26 8b 15
    mov ax, word [es:di+002h]                 ; 26 8b 45 02
    cmp ax, 06568h                            ; 3d 68 65
    jne short 04f2ah                          ; 75 1f
    cmp dx, 07445h                            ; 81 fa 45 74
    jne short 04f2ah                          ; 75 19
    movzx bx, byte [bp-006h]                  ; 0f b6 5e fa
    movzx dx, byte [bp-00ch]                  ; 0f b6 56 f4
    movzx ax, byte [bp-00ah]                  ; 0f b6 46 f6
    call 04c8ah                               ; e8 6a fd
    mov word [bp-012h], strict word 00006h    ; c7 46 ee 06 00
    mov word [bp-010h], cx                    ; 89 4e f0
    jmp short 04f46h                          ; eb 1c
    movzx bx, byte [bp-006h]                  ; 0f b6 5e fa
    movzx dx, byte [bp-00ch]                  ; 0f b6 56 f4
    movzx ax, byte [bp-00ah]                  ; 0f b6 46 f6
    call 04c8ah                               ; e8 51 fd
    sti                                       ; fb
    mov word [bp-010h], cx                    ; 89 4e f0
    mov es, cx                                ; 8e c1
    mov ax, word [es:si+01ah]                 ; 26 8b 44 1a
    mov word [bp-012h], ax                    ; 89 46 ee
    call far [bp-012h]                        ; ff 5e ee
    jmp near 04ea1h                           ; e9 55 ff
    cmp byte [bp-00ah], 000h                  ; 80 7e f6 00
    jne short 04f78h                          ; 75 26
    cmp byte [bp-00ch], 000h                  ; 80 7e f4 00
    jne short 04f78h                          ; 75 20
    mov di, 007c0h                            ; bf c0 07
    mov es, di                                ; 8e c7
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
    jne near 04ea1h                           ; 0f 85 29 ff
    cmp byte [bp-006h], 000h                  ; 80 7e fa 00
    db  00fh, 094h, 0c1h
    ; sete cl                                   ; 0f 94 c1
    cmp byte [bp-00ah], 000h                  ; 80 7e f6 00
    je short 04f87h                           ; 74 02
    mov CL, strict byte 001h                  ; b1 01
    xor dx, dx                                ; 31 d2
    mov ax, di                                ; 89 f8
    call 01765h                               ; e8 d7 c7
    mov bx, ax                                ; 89 c3
    mov dx, strict word 00004h                ; ba 04 00
    mov ax, di                                ; 89 f8
    call 01765h                               ; e8 cd c7
    cmp bx, ax                                ; 39 c3
    je short 04fadh                           ; 74 11
    test cl, cl                               ; 84 c9
    jne short 04fc3h                          ; 75 23
    mov dx, 001feh                            ; ba fe 01
    mov ax, di                                ; 89 f8
    call 01765h                               ; e8 bd c7
    cmp ax, 0aa55h                            ; 3d 55 aa
    je short 04fc3h                           ; 74 16
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    push ax                                   ; 50
    movzx bx, byte [bp-006h]                  ; 0f b6 5e fa
    movzx dx, byte [bp-00ch]                  ; 0f b6 56 f4
    movzx ax, byte [bp-00ah]                  ; 0f b6 46 f6
    xor cx, cx                                ; 31 c9
    jmp near 04eb5h                           ; e9 f2 fe
    movzx bx, byte [bp-006h]                  ; 0f b6 5e fa
    movzx dx, byte [bp-00ch]                  ; 0f b6 56 f4
    movzx ax, byte [bp-00ah]                  ; 0f b6 46 f6
    call 04c8ah                               ; e8 b8 fc
    movzx bx, byte [bp-006h]                  ; 0f b6 5e fa
    xor dx, dx                                ; 31 d2
    xor ax, ax                                ; 31 c0
    add ax, di                                ; 01 f8
    adc dx, bx                                ; 11 da
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
keyboard_panic_:                             ; 0xf4fe5 LB 0x13
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push ax                                   ; 50
    push 00586h                               ; 68 86 05
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 7a ca
    add sp, strict byte 00006h                ; 83 c4 06
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
_keyboard_init:                              ; 0xf4ff8 LB 0x26a
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
    je short 0501bh                           ; 74 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 0501bh                          ; 76 08
    xor al, al                                ; 30 c0
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 05004h                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 05024h                          ; 75 05
    xor ax, ax                                ; 31 c0
    call 04fe5h                               ; e8 c1 ff
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 0503eh                          ; 75 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 0503eh                          ; 76 08
    mov AL, strict byte 001h                  ; b0 01
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 05027h                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 05048h                          ; 75 06
    mov ax, strict word 00001h                ; b8 01 00
    call 04fe5h                               ; e8 9d ff
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp ax, strict word 00055h                ; 3d 55 00
    je short 05059h                           ; 74 06
    mov ax, 003dfh                            ; b8 df 03
    call 04fe5h                               ; e8 8c ff
    mov AL, strict byte 0abh                  ; b0 ab
    mov dx, strict word 00064h                ; ba 64 00
    out DX, AL                                ; ee
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 002h                 ; a8 02
    je short 05079h                           ; 74 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 05079h                          ; 76 08
    mov AL, strict byte 010h                  ; b0 10
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 05062h                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 05083h                          ; 75 06
    mov ax, strict word 0000ah                ; b8 0a 00
    call 04fe5h                               ; e8 62 ff
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 0509dh                          ; 75 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 0509dh                          ; 76 08
    mov AL, strict byte 011h                  ; b0 11
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 05086h                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 050a7h                          ; 75 06
    mov ax, strict word 0000bh                ; b8 0b 00
    call 04fe5h                               ; e8 3e ff
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test ax, ax                               ; 85 c0
    je short 050b7h                           ; 74 06
    mov ax, 003e0h                            ; b8 e0 03
    call 04fe5h                               ; e8 2e ff
    mov AL, strict byte 0ffh                  ; b0 ff
    mov dx, strict word 00060h                ; ba 60 00
    out DX, AL                                ; ee
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 002h                 ; a8 02
    je short 050d7h                           ; 74 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 050d7h                          ; 76 08
    mov AL, strict byte 020h                  ; b0 20
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 050c0h                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 050e1h                          ; 75 06
    mov ax, strict word 00014h                ; b8 14 00
    call 04fe5h                               ; e8 04 ff
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 050fbh                          ; 75 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 050fbh                          ; 76 08
    mov AL, strict byte 021h                  ; b0 21
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 050e4h                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 05105h                          ; 75 06
    mov ax, strict word 00015h                ; b8 15 00
    call 04fe5h                               ; e8 e0 fe
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp ax, 000fah                            ; 3d fa 00
    je short 05116h                           ; 74 06
    mov ax, 003e1h                            ; b8 e1 03
    call 04fe5h                               ; e8 cf fe
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 05128h                          ; 75 08
    mov AL, strict byte 031h                  ; b0 31
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 05116h                          ; eb ee
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp ax, 000aah                            ; 3d aa 00
    je short 05141h                           ; 74 0e
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp ax, 000aah                            ; 3d aa 00
    je short 05141h                           ; 74 06
    mov ax, 003e2h                            ; b8 e2 03
    call 04fe5h                               ; e8 a4 fe
    mov AL, strict byte 0f5h                  ; b0 f5
    mov dx, strict word 00060h                ; ba 60 00
    out DX, AL                                ; ee
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 002h                 ; a8 02
    je short 05161h                           ; 74 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 05161h                          ; 76 08
    mov AL, strict byte 040h                  ; b0 40
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 0514ah                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 0516bh                          ; 75 06
    mov ax, strict word 00028h                ; b8 28 00
    call 04fe5h                               ; e8 7a fe
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 05185h                          ; 75 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 05185h                          ; 76 08
    mov AL, strict byte 041h                  ; b0 41
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 0516eh                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 0518fh                          ; 75 06
    mov ax, strict word 00029h                ; b8 29 00
    call 04fe5h                               ; e8 56 fe
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp ax, 000fah                            ; 3d fa 00
    je short 051a0h                           ; 74 06
    mov ax, 003e3h                            ; b8 e3 03
    call 04fe5h                               ; e8 45 fe
    mov AL, strict byte 060h                  ; b0 60
    mov dx, strict word 00064h                ; ba 64 00
    out DX, AL                                ; ee
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 002h                 ; a8 02
    je short 051c0h                           ; 74 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 051c0h                          ; 76 08
    mov AL, strict byte 050h                  ; b0 50
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 051a9h                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 051cah                          ; 75 06
    mov ax, strict word 00032h                ; b8 32 00
    call 04fe5h                               ; e8 1b fe
    mov AL, strict byte 065h                  ; b0 65
    mov dx, strict word 00060h                ; ba 60 00
    out DX, AL                                ; ee
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 002h                 ; a8 02
    je short 051eah                           ; 74 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 051eah                          ; 76 08
    mov AL, strict byte 060h                  ; b0 60
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 051d3h                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 051f4h                          ; 75 06
    mov ax, strict word 0003ch                ; b8 3c 00
    call 04fe5h                               ; e8 f1 fd
    mov AL, strict byte 0f4h                  ; b0 f4
    mov dx, strict word 00060h                ; ba 60 00
    out DX, AL                                ; ee
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 002h                 ; a8 02
    je short 05214h                           ; 74 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 05214h                          ; 76 08
    mov AL, strict byte 070h                  ; b0 70
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 051fdh                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 0521eh                          ; 75 06
    mov ax, strict word 00046h                ; b8 46 00
    call 04fe5h                               ; e8 c7 fd
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 05238h                          ; 75 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 05238h                          ; 76 08
    mov AL, strict byte 071h                  ; b0 71
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 05221h                          ; eb e9
    test bx, bx                               ; 85 db
    jne short 05242h                          ; 75 06
    mov ax, strict word 00046h                ; b8 46 00
    call 04fe5h                               ; e8 a3 fd
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp ax, 000fah                            ; 3d fa 00
    je short 05253h                           ; 74 06
    mov ax, 003e4h                            ; b8 e4 03
    call 04fe5h                               ; e8 92 fd
    mov AL, strict byte 0a8h                  ; b0 a8
    mov dx, strict word 00064h                ; ba 64 00
    out DX, AL                                ; ee
    xor ax, ax                                ; 31 c0
    call 066b2h                               ; e8 54 14
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
enqueue_key_:                                ; 0xf5262 LB 0x99
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
    call 01765h                               ; e8 ed c4
    mov di, ax                                ; 89 c7
    mov dx, strict word 0001ch                ; ba 1c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 e2 c4
    mov si, ax                                ; 89 c6
    lea cx, [si+002h]                         ; 8d 4c 02
    cmp cx, strict byte 0003eh                ; 83 f9 3e
    jc short 05290h                           ; 72 03
    mov cx, strict word 0001eh                ; b9 1e 00
    cmp cx, di                                ; 39 f9
    jne short 05298h                          ; 75 04
    xor ax, ax                                ; 31 c0
    jmp short 052bdh                          ; eb 25
    xor bh, bh                                ; 30 ff
    mov dx, si                                ; 89 f2
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 b5 c4
    movzx bx, byte [bp-00ah]                  ; 0f b6 5e f6
    lea dx, [si+001h]                         ; 8d 54 01
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 a8 c4
    mov bx, cx                                ; 89 cb
    mov dx, strict word 0001ch                ; ba 1c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01773h                               ; e8 b9 c4
    mov ax, strict word 00001h                ; b8 01 00
    lea sp, [bp-008h]                         ; 8d 66 f8
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
    db  0d4h, 0c6h, 0c5h, 0bah, 0b8h, 0b6h, 0aah, 09dh, 054h, 053h, 046h, 045h, 03ah, 038h, 036h, 02ah
    db  01dh, 023h, 056h, 0ech, 053h, 094h, 053h, 094h, 053h, 075h, 054h, 06ah, 053h, 0f3h, 054h, 061h
    db  055h, 009h, 056h, 0e5h, 055h, 030h, 054h, 094h, 053h, 094h, 053h, 0b4h, 054h, 086h, 053h, 042h
    db  055h, 0c6h, 055h, 002h, 056h
_int09_function:                             ; 0xf52fb LB 0x474
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push di                                   ; 57
    sub sp, strict byte 0000eh                ; 83 ec 0e
    mov al, byte [bp+014h]                    ; 8a 46 14
    mov byte [bp-00ch], al                    ; 88 46 f4
    test al, al                               ; 84 c0
    jne short 05325h                          ; 75 19
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 13 c7
    push 00599h                               ; 68 99 05
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 4c c7
    add sp, strict byte 00004h                ; 83 c4 04
    jmp near 053e6h                           ; e9 c1 00
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 1b c4
    mov byte [bp-006h], al                    ; 88 46 fa
    mov byte [bp-00ah], al                    ; 88 46 f6
    mov dx, 00096h                            ; ba 96 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 0c c4
    mov byte [bp-00eh], al                    ; 88 46 f2
    mov byte [bp-004h], al                    ; 88 46 fc
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 fd c3
    mov byte [bp-010h], al                    ; 88 46 f0
    mov byte [bp-008h], al                    ; 88 46 f8
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    push CS                                   ; 0e
    pop ES                                    ; 07
    mov cx, strict word 00012h                ; b9 12 00
    mov di, 052c6h                            ; bf c6 52
    repne scasb                               ; f2 ae
    sal cx, 1                                 ; d1 e1
    mov di, cx                                ; 89 cf
    mov ax, word [cs:di+052d7h]               ; 2e 8b 85 d7 52
    jmp ax                                    ; ff e0
    xor byte [bp-008h], 040h                  ; 80 76 f8 40
    movzx bx, byte [bp-008h]                  ; 0f b6 5e f8
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 dc c3
    or byte [bp-00ah], 040h                   ; 80 4e f6 40
    movzx bx, byte [bp-00ah]                  ; 0f b6 5e f6
    jmp near 055f6h                           ; e9 70 02
    mov al, byte [bp-006h]                    ; 8a 46 fa
    and AL, strict byte 0bfh                  ; 24 bf
    mov byte [bp-00ah], al                    ; 88 46 f6
    movzx bx, al                              ; 0f b6 d8
    jmp near 055f6h                           ; e9 62 02
    test byte [bp-004h], 002h                 ; f6 46 fc 02
    jne near 053c8h                           ; 0f 85 2c 00
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    and AL, strict byte 07fh                  ; 24 7f
    cmp AL, strict byte 02ah                  ; 3c 2a
    db  00fh, 094h, 0c0h
    ; sete al                                   ; 0f 94 c0
    xor ah, ah                                ; 30 e4
    inc ax                                    ; 40
    mov dl, al                                ; 88 c2
    test byte [bp-00ch], 080h                 ; f6 46 f4 80
    je short 053b8h                           ; 74 07
    not al                                    ; f6 d0
    and byte [bp-008h], al                    ; 20 46 f8
    jmp short 053bbh                          ; eb 03
    or byte [bp-008h], al                     ; 08 46 f8
    movzx bx, byte [bp-008h]                  ; 0f b6 5e f8
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 8f c3
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    and AL, strict byte 07fh                  ; 24 7f
    cmp AL, strict byte 01dh                  ; 3c 1d
    je short 053d5h                           ; 74 04
    and byte [bp-004h], 0feh                  ; 80 66 fc fe
    and byte [bp-004h], 0fdh                  ; 80 66 fc fd
    movzx bx, byte [bp-004h]                  ; 0f b6 5e fc
    mov dx, 00096h                            ; ba 96 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 71 c3
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop di                                    ; 5f
    pop bp                                    ; 5d
    retn                                      ; c3
    test byte [bp-00eh], 001h                 ; f6 46 f2 01
    jne short 053c8h                          ; 75 d6
    mov al, byte [bp-010h]                    ; 8a 46 f0
    or AL, strict byte 004h                   ; 0c 04
    mov byte [bp-008h], al                    ; 88 46 f8
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 51 c3
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    test AL, strict byte 002h                 ; a8 02
    je short 0541ah                           ; 74 0d
    or AL, strict byte 004h                   ; 0c 04
    mov byte [bp-004h], al                    ; 88 46 fc
    movzx bx, al                              ; 0f b6 d8
    mov dx, 00096h                            ; ba 96 00
    jmp short 05428h                          ; eb 0e
    mov al, byte [bp-006h]                    ; 8a 46 fa
    or AL, strict byte 001h                   ; 0c 01
    mov byte [bp-00ah], al                    ; 88 46 f6
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 29 c3
    jmp short 053c8h                          ; eb 98
    test byte [bp-00eh], 001h                 ; f6 46 f2 01
    jne short 053c8h                          ; 75 92
    mov al, byte [bp-010h]                    ; 8a 46 f0
    and AL, strict byte 0fbh                  ; 24 fb
    mov byte [bp-008h], al                    ; 88 46 f8
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 0d c3
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    test AL, strict byte 002h                 ; a8 02
    je short 0545eh                           ; 74 0d
    and AL, strict byte 0fbh                  ; 24 fb
    mov byte [bp-004h], al                    ; 88 46 fc
    movzx bx, al                              ; 0f b6 d8
    mov dx, 00096h                            ; ba 96 00
    jmp short 0546ch                          ; eb 0e
    mov al, byte [bp-006h]                    ; 8a 46 fa
    and AL, strict byte 0feh                  ; 24 fe
    mov byte [bp-00ah], al                    ; 88 46 f6
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 e5 c2
    jmp near 053c8h                           ; e9 53 ff
    mov al, byte [bp-010h]                    ; 8a 46 f0
    or AL, strict byte 008h                   ; 0c 08
    mov byte [bp-008h], al                    ; 88 46 f8
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 ce c2
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    test AL, strict byte 002h                 ; a8 02
    je short 0549dh                           ; 74 0d
    or AL, strict byte 008h                   ; 0c 08
    mov byte [bp-004h], al                    ; 88 46 fc
    movzx bx, al                              ; 0f b6 d8
    mov dx, 00096h                            ; ba 96 00
    jmp short 054abh                          ; eb 0e
    mov al, byte [bp-006h]                    ; 8a 46 fa
    or AL, strict byte 002h                   ; 0c 02
    mov byte [bp-00ah], al                    ; 88 46 f6
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 a6 c2
    jmp near 053c8h                           ; e9 14 ff
    mov al, byte [bp-010h]                    ; 8a 46 f0
    and AL, strict byte 0f7h                  ; 24 f7
    mov byte [bp-008h], al                    ; 88 46 f8
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 8f c2
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    test AL, strict byte 002h                 ; a8 02
    je short 054dch                           ; 74 0d
    and AL, strict byte 0f7h                  ; 24 f7
    mov byte [bp-004h], al                    ; 88 46 fc
    movzx bx, al                              ; 0f b6 d8
    mov dx, 00096h                            ; ba 96 00
    jmp short 054eah                          ; eb 0e
    mov al, byte [bp-006h]                    ; 8a 46 fa
    and AL, strict byte 0fdh                  ; 24 fd
    mov byte [bp-00ah], al                    ; 88 46 f6
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 67 c2
    jmp near 053c8h                           ; e9 d5 fe
    test byte [bp-00eh], 003h                 ; f6 46 f2 03
    jne short 05515h                          ; 75 1c
    mov al, byte [bp-006h]                    ; 8a 46 fa
    or AL, strict byte 020h                   ; 0c 20
    mov byte [bp-00ah], al                    ; 88 46 f6
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 4a c2
    mov al, byte [bp-010h]                    ; 8a 46 f0
    xor AL, strict byte 020h                  ; 34 20
    jmp near 055b4h                           ; e9 9f 00
    mov al, byte [bp-006h]                    ; 8a 46 fa
    or AL, strict byte 008h                   ; 0c 08
    mov byte [bp-00ah], al                    ; 88 46 f6
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 2e c2
    mov AL, strict byte 0aeh                  ; b0 ae
    mov dx, strict word 00064h                ; ba 64 00
    out DX, AL                                ; ee
    call 0e034h                               ; e8 02 8b
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 0e c2
    test AL, strict byte 008h                 ; a8 08
    jne short 05532h                          ; 75 f3
    jmp near 053c8h                           ; e9 86 fe
    test byte [bp-00eh], 003h                 ; f6 46 f2 03
    jne near 053c8h                           ; 0f 85 7e fe
    mov al, byte [bp-006h]                    ; 8a 46 fa
    and AL, strict byte 0dfh                  ; 24 df
    mov byte [bp-00ah], al                    ; 88 46 f6
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 f9 c1
    jmp near 053c8h                           ; e9 67 fe
    test byte [bp-00eh], 002h                 ; f6 46 f2 02
    je short 0559bh                           ; 74 34
    mov dx, strict word 0001ah                ; ba 1a 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 f5 c1
    mov bx, ax                                ; 89 c3
    mov dx, strict word 0001ch                ; ba 1c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01773h                               ; e8 f8 c1
    mov bx, 00080h                            ; bb 80 00
    mov dx, strict word 00071h                ; ba 71 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 d0 c1
    mov AL, strict byte 0aeh                  ; b0 ae
    mov dx, strict word 00064h                ; ba 64 00
    out DX, AL                                ; ee
    push bp                                   ; 55
    int 01bh                                  ; cd 1b
    pop bp                                    ; 5d
    xor dx, dx                                ; 31 d2
    xor ax, ax                                ; 31 c0
    call 05262h                               ; e8 ca fc
    jmp near 053c8h                           ; e9 2d fe
    mov al, byte [bp-006h]                    ; 8a 46 fa
    or AL, strict byte 010h                   ; 0c 10
    mov byte [bp-00ah], al                    ; 88 46 f6
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 a8 c1
    mov al, byte [bp-010h]                    ; 8a 46 f0
    xor AL, strict byte 010h                  ; 34 10
    mov byte [bp-008h], al                    ; 88 46 f8
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 94 c1
    jmp near 053c8h                           ; e9 02 fe
    test byte [bp-00eh], 002h                 ; f6 46 f2 02
    jne near 053c8h                           ; 0f 85 fa fd
    mov al, byte [bp-006h]                    ; 8a 46 fa
    and AL, strict byte 0efh                  ; 24 ef
    mov byte [bp-00ah], al                    ; 88 46 f6
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 75 c1
    jmp near 053c8h                           ; e9 e3 fd
    mov al, byte [bp-006h]                    ; 8a 46 fa
    test AL, strict byte 004h                 ; a8 04
    jne near 053c8h                           ; 0f 85 da fd
    or AL, strict byte 004h                   ; 0c 04
    mov byte [bp-00ah], al                    ; 88 46 f6
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 58 c1
    jmp near 053c8h                           ; e9 c6 fd
    mov al, byte [bp-006h]                    ; 8a 46 fa
    and AL, strict byte 0fbh                  ; 24 fb
    jmp short 055f0h                          ; eb e7
    mov al, byte [bp-010h]                    ; 8a 46 f0
    and AL, strict byte 00ch                  ; 24 0c
    cmp AL, strict byte 00ch                  ; 3c 0c
    jne short 05623h                          ; 75 11
    mov bx, 01234h                            ; bb 34 12
    mov dx, strict word 00072h                ; ba 72 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01773h                               ; e8 55 c1
    jmp far 0f000h:0e05bh                     ; ea 5b e0 00 f0
    test byte [bp-00ah], 008h                 ; f6 46 f6 08
    je short 05637h                           ; 74 0e
    and byte [bp-00ah], 0f7h                  ; 80 66 f6 f7
    movzx bx, byte [bp-00ah]                  ; 0f b6 5e f6
    mov dx, strict word 00018h                ; ba 18 00
    jmp near 053e0h                           ; e9 a9 fd
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    test AL, strict byte 080h                 ; a8 80
    je short 05675h                           ; 74 37
    cmp AL, strict byte 0fah                  ; 3c fa
    jne short 05652h                          ; 75 10
    mov dx, 00097h                            ; ba 97 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 fe c0
    mov dl, al                                ; 88 c2
    or dl, 010h                               ; 80 ca 10
    jmp short 05666h                          ; eb 14
    cmp AL, strict byte 0feh                  ; 3c fe
    jne near 053c8h                           ; 0f 85 70 fd
    mov dx, 00097h                            ; ba 97 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 e8 c0
    mov dl, al                                ; 88 c2
    or dl, 020h                               ; 80 ca 20
    movzx bx, dl                              ; 0f b6 da
    mov dx, 00097h                            ; ba 97 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 e5 c0
    jmp near 053c8h                           ; e9 53 fd
    cmp byte [bp-00ch], 058h                  ; 80 7e f4 58
    jbe short 05699h                          ; 76 1e
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 a4 c3
    movzx ax, byte [bp-00ch]                  ; 0f b6 46 f4
    push ax                                   ; 50
    push 005b3h                               ; 68 b3 05
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 d8 c3
    add sp, strict byte 00006h                ; 83 c4 06
    jmp near 053e6h                           ; e9 4d fd
    mov al, byte [bp-008h]                    ; 8a 46 f8
    test AL, strict byte 008h                 ; a8 08
    je short 056b2h                           ; 74 12
    movzx bx, byte [bp-00ch]                  ; 0f b6 5e f4
    imul bx, bx, strict byte 0000ah           ; 6b db 0a
    mov dl, byte [bx+00e06h]                  ; 8a 97 06 0e
    mov ax, word [bx+00e06h]                  ; 8b 87 06 0e
    jmp near 05740h                           ; e9 8e 00
    test AL, strict byte 004h                 ; a8 04
    je short 056c8h                           ; 74 12
    movzx bx, byte [bp-00ch]                  ; 0f b6 5e f4
    imul bx, bx, strict byte 0000ah           ; 6b db 0a
    mov dl, byte [bx+00e04h]                  ; 8a 97 04 0e
    mov ax, word [bx+00e04h]                  ; 8b 87 04 0e
    jmp near 05740h                           ; e9 78 00
    mov al, byte [bp-004h]                    ; 8a 46 fc
    and AL, strict byte 002h                  ; 24 02
    test al, al                               ; 84 c0
    jbe short 056e6h                          ; 76 15
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    cmp AL, strict byte 047h                  ; 3c 47
    jc short 056e6h                           ; 72 0e
    cmp AL, strict byte 053h                  ; 3c 53
    jnbe short 056e6h                         ; 77 0a
    mov DL, strict byte 0e0h                  ; b2 e0
    movzx bx, al                              ; 0f b6 d8
    imul bx, bx, strict byte 0000ah           ; 6b db 0a
    jmp short 0573ch                          ; eb 56
    test byte [bp-008h], 003h                 ; f6 46 f8 03
    je short 0571ah                           ; 74 2e
    movzx bx, byte [bp-00ch]                  ; 0f b6 5e f4
    imul bx, bx, strict byte 0000ah           ; 6b db 0a
    movzx ax, byte [bx+00e08h]                ; 0f b6 87 08 0e
    movzx dx, byte [bp-008h]                  ; 0f b6 56 f8
    test dx, ax                               ; 85 c2
    je short 0570ah                           ; 74 0a
    mov dl, byte [bx+00e00h]                  ; 8a 97 00 0e
    mov ax, word [bx+00e00h]                  ; 8b 87 00 0e
    jmp short 05712h                          ; eb 08
    mov dl, byte [bx+00e02h]                  ; 8a 97 02 0e
    mov ax, word [bx+00e02h]                  ; 8b 87 02 0e
    shr ax, 008h                              ; c1 e8 08
    mov byte [bp-00ch], al                    ; 88 46 f4
    jmp short 05746h                          ; eb 2c
    movzx bx, byte [bp-00ch]                  ; 0f b6 5e f4
    imul bx, bx, strict byte 0000ah           ; 6b db 0a
    movzx ax, byte [bx+00e08h]                ; 0f b6 87 08 0e
    movzx dx, byte [bp-008h]                  ; 0f b6 56 f8
    test dx, ax                               ; 85 c2
    je short 05738h                           ; 74 0a
    mov dl, byte [bx+00e02h]                  ; 8a 97 02 0e
    mov ax, word [bx+00e02h]                  ; 8b 87 02 0e
    jmp short 05740h                          ; eb 08
    mov dl, byte [bx+00e00h]                  ; 8a 97 00 0e
    mov ax, word [bx+00e00h]                  ; 8b 87 00 0e
    shr ax, 008h                              ; c1 e8 08
    mov byte [bp-00ch], al                    ; 88 46 f4
    cmp byte [bp-00ch], 000h                  ; 80 7e f4 00
    jne short 05766h                          ; 75 1a
    test dl, dl                               ; 84 d2
    jne short 05766h                          ; 75 16
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 cf c2
    push 005eah                               ; 68 ea 05
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 08 c3
    add sp, strict byte 00004h                ; 83 c4 04
    xor dh, dh                                ; 30 f6
    movzx ax, byte [bp-00ch]                  ; 0f b6 46 f4
    jmp near 05595h                           ; e9 26 fe
dequeue_key_:                                ; 0xf576f LB 0x94
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
    call 01765h                               ; e8 dc bf
    mov bx, ax                                ; 89 c3
    mov dx, strict word 0001ch                ; ba 1c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 d1 bf
    cmp bx, ax                                ; 39 c3
    je short 057d5h                           ; 74 3d
    mov dx, bx                                ; 89 da
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 a9 bf
    mov cl, al                                ; 88 c1
    lea dx, [bx+001h]                         ; 8d 57 01
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 9e bf
    mov es, [bp-008h]                         ; 8e 46 f8
    mov byte [es:si], cl                      ; 26 88 0c
    mov es, [bp-006h]                         ; 8e 46 fa
    mov byte [es:di], al                      ; 26 88 05
    cmp word [bp+004h], strict byte 00000h    ; 83 7e 04 00
    je short 057d0h                           ; 74 13
    inc bx                                    ; 43
    inc bx                                    ; 43
    cmp bx, strict byte 0003eh                ; 83 fb 3e
    jc short 057c7h                           ; 72 03
    mov bx, strict word 0001eh                ; bb 1e 00
    mov dx, strict word 0001ah                ; ba 1a 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01773h                               ; e8 a3 bf
    mov ax, strict word 00001h                ; b8 01 00
    jmp short 057d7h                          ; eb 02
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
    add byte [di-046a6h], ah                  ; 00 a5 5a b9
    pop ax                                    ; 58
    add byte [bx+di+04dh], bl                 ; 00 59 4d
    pop cx                                    ; 59
    pop bp                                    ; 5d
    pop cx                                    ; 59
    xchg word [bx+di-070h], bx                ; 87 59 90
    pop cx                                    ; 59
    add word [bp+si+032h], bx                 ; 01 5a 32
    pop dx                                    ; 5a
    pushaw                                    ; 60
    pop dx                                    ; 5a
    db  09ah
    pop dx                                    ; 5a
    db  0e8h
    pop dx                                    ; 5a
_int16_function:                             ; 0xf5803 LB 0x2eb
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push di                                   ; 57
    sub sp, strict byte 00006h                ; 83 ec 06
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 36 bf
    mov cl, al                                ; 88 c1
    mov bl, al                                ; 88 c3
    mov dx, 00097h                            ; ba 97 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 29 bf
    mov bh, al                                ; 88 c7
    movzx dx, cl                              ; 0f b6 d1
    sar dx, 004h                              ; c1 fa 04
    and dl, 007h                              ; 80 e2 07
    and AL, strict byte 007h                  ; 24 07
    xor ah, ah                                ; 30 e4
    xor al, dl                                ; 30 d0
    test ax, ax                               ; 85 c0
    je short 05897h                           ; 74 62
    cli                                       ; fa
    mov AL, strict byte 0edh                  ; b0 ed
    mov dx, strict word 00060h                ; ba 60 00
    out DX, AL                                ; ee
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 0584eh                          ; 75 08
    mov AL, strict byte 021h                  ; b0 21
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 0583ch                          ; eb ee
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp ax, 000fah                            ; 3d fa 00
    jne short 05896h                          ; 75 3d
    and bh, 0c8h                              ; 80 e7 c8
    movzx dx, bl                              ; 0f b6 d3
    sar dx, 004h                              ; c1 fa 04
    and dx, strict byte 00007h                ; 83 e2 07
    movzx ax, bh                              ; 0f b6 c7
    or ax, dx                                 ; 09 d0
    mov bh, al                                ; 88 c7
    and AL, strict byte 007h                  ; 24 07
    mov dx, strict word 00060h                ; ba 60 00
    out DX, AL                                ; ee
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 05884h                          ; 75 08
    mov AL, strict byte 021h                  ; b0 21
    mov dx, 00080h                            ; ba 80 00
    out DX, AL                                ; ee
    jmp short 05872h                          ; eb ee
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    movzx bx, bh                              ; 0f b6 df
    mov dx, 00097h                            ; ba 97 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 c1 be
    sti                                       ; fb
    mov ax, word [bp+012h]                    ; 8b 46 12
    shr ax, 008h                              ; c1 e8 08
    cmp ax, 000a2h                            ; 3d a2 00
    jnbe near 05aa5h                          ; 0f 87 01 02
    push CS                                   ; 0e
    pop ES                                    ; 07
    mov cx, strict word 0000ch                ; b9 0c 00
    mov di, 057e0h                            ; bf e0 57
    repne scasb                               ; f2 ae
    sal cx, 1                                 ; d1 e1
    mov di, cx                                ; 89 cf
    mov ax, word [cs:di+057ebh]               ; 2e 8b 85 eb 57
    jmp ax                                    ; ff e0
    push strict byte 00001h                   ; 6a 01
    mov cx, ss                                ; 8c d1
    lea bx, [bp-008h]                         ; 8d 5e f8
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 0576fh                               ; e8 a7 fe
    test ax, ax                               ; 85 c0
    jne short 058d7h                          ; 75 0b
    push 00621h                               ; 68 21 06
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 97 c1
    add sp, strict byte 00004h                ; 83 c4 04
    cmp byte [bp-006h], 000h                  ; 80 7e fa 00
    je short 058e3h                           ; 74 06
    cmp byte [bp-008h], 0f0h                  ; 80 7e f8 f0
    je short 058e9h                           ; 74 06
    cmp byte [bp-008h], 0e0h                  ; 80 7e f8 e0
    jne short 058edh                          ; 75 04
    mov byte [bp-008h], 000h                  ; c6 46 f8 00
    movzx dx, byte [bp-006h]                  ; 0f b6 56 fa
    sal dx, 008h                              ; c1 e2 08
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    or dx, ax                                 ; 09 c2
    mov word [bp+012h], dx                    ; 89 56 12
    jmp near 05ae8h                           ; e9 e8 01
    mov ax, word [bp+018h]                    ; 8b 46 18
    mov word [bp+01eh], ax                    ; 89 46 1e
    push strict byte 00000h                   ; 6a 00
    mov cx, ss                                ; 8c d1
    lea bx, [bp-008h]                         ; 8d 5e f8
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 0576fh                               ; e8 5a fe
    test ax, ax                               ; 85 c0
    jne short 05920h                          ; 75 07
    or word [bp+01eh], strict byte 00040h     ; 83 4e 1e 40
    jmp near 05ae8h                           ; e9 c8 01
    cmp byte [bp-006h], 000h                  ; 80 7e fa 00
    je short 0592ch                           ; 74 06
    cmp byte [bp-008h], 0f0h                  ; 80 7e f8 f0
    je short 05932h                           ; 74 06
    cmp byte [bp-008h], 0e0h                  ; 80 7e f8 e0
    jne short 05936h                          ; 75 04
    mov byte [bp-008h], 000h                  ; c6 46 f8 00
    movzx dx, byte [bp-006h]                  ; 0f b6 56 fa
    sal dx, 008h                              ; c1 e2 08
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    or dx, ax                                 ; 09 c2
    mov word [bp+012h], dx                    ; 89 56 12
    and word [bp+01eh], strict byte 0ffbfh    ; 83 66 1e bf
    jmp near 05ae8h                           ; e9 9b 01
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 f3 bd
    mov dx, word [bp+012h]                    ; 8b 56 12
    mov dl, al                                ; 88 c2
    jmp short 058fah                          ; eb 9d
    mov al, byte [bp+010h]                    ; 8a 46 10
    movzx dx, al                              ; 0f b6 d0
    mov ax, word [bp+010h]                    ; 8b 46 10
    shr ax, 008h                              ; c1 e8 08
    xor ah, ah                                ; 30 e4
    call 05262h                               ; e8 f4 f8
    test ax, ax                               ; 85 c0
    jne short 0597fh                          ; 75 0d
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor al, al                                ; 30 c0
    or AL, strict byte 001h                   ; 0c 01
    mov word [bp+012h], ax                    ; 89 46 12
    jmp near 05ae8h                           ; e9 69 01
    and word [bp+012h], 0ff00h                ; 81 66 12 00 ff
    jmp near 05ae8h                           ; e9 61 01
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor al, al                                ; 30 c0
    or AL, strict byte 030h                   ; 0c 30
    jmp short 05979h                          ; eb e9
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
    jne short 059b7h                          ; 75 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 059b7h                          ; 76 08
    mov dx, 00080h                            ; ba 80 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    jmp short 059a0h                          ; eb e9
    test bx, bx                               ; 85 db
    jbe short 059fbh                          ; 76 40
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp ax, 000fah                            ; 3d fa 00
    jne short 059fbh                          ; 75 35
    mov bx, strict word 0ffffh                ; bb ff ff
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 059e0h                          ; 75 0d
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 059e0h                          ; 76 08
    mov dx, 00080h                            ; ba 80 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    jmp short 059c9h                          ; eb e9
    test bx, bx                               ; 85 db
    jbe short 059f2h                          ; 76 0e
    shr cx, 008h                              ; c1 e9 08
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    sal ax, 008h                              ; c1 e0 08
    or cx, ax                                 ; 09 c1
    dec byte [bp-004h]                        ; fe 4e fc
    cmp byte [bp-004h], 000h                  ; 80 7e fc 00
    jnbe short 059c6h                         ; 77 cb
    mov word [bp+00ch], cx                    ; 89 4e 0c
    jmp near 05ae8h                           ; e9 e7 00
    push strict byte 00001h                   ; 6a 01
    mov cx, ss                                ; 8c d1
    lea bx, [bp-008h]                         ; 8d 5e f8
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 0576fh                               ; e8 5f fd
    test ax, ax                               ; 85 c0
    jne short 05a1fh                          ; 75 0b
    push 00621h                               ; 68 21 06
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 4f c0
    add sp, strict byte 00004h                ; 83 c4 04
    cmp byte [bp-006h], 000h                  ; 80 7e fa 00
    je near 058edh                            ; 0f 84 c6 fe
    cmp byte [bp-008h], 0f0h                  ; 80 7e f8 f0
    je near 058e9h                            ; 0f 84 ba fe
    jmp near 058edh                           ; e9 bb fe
    mov ax, word [bp+018h]                    ; 8b 46 18
    mov word [bp+01eh], ax                    ; 89 46 1e
    push strict byte 00000h                   ; 6a 00
    mov cx, ss                                ; 8c d1
    lea bx, [bp-008h]                         ; 8d 5e f8
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 0576fh                               ; e8 28 fd
    test ax, ax                               ; 85 c0
    je near 05919h                            ; 0f 84 cc fe
    cmp byte [bp-006h], 000h                  ; 80 7e fa 00
    je near 05936h                            ; 0f 84 e1 fe
    cmp byte [bp-008h], 0f0h                  ; 80 7e f8 f0
    je near 05932h                            ; 0f 84 d5 fe
    jmp near 05936h                           ; e9 d6 fe
    mov dx, strict word 00017h                ; ba 17 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 e0 bc
    mov dx, word [bp+012h]                    ; 8b 56 12
    mov dl, al                                ; 88 c2
    mov word [bp+012h], dx                    ; 89 56 12
    mov dx, strict word 00018h                ; ba 18 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 cf bc
    mov bl, al                                ; 88 c3
    and bl, 073h                              ; 80 e3 73
    mov dx, 00096h                            ; ba 96 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 c1 bc
    and AL, strict byte 00ch                  ; 24 0c
    or bl, al                                 ; 08 c3
    mov dx, word [bp+012h]                    ; 8b 56 12
    xor dh, dh                                ; 30 f6
    movzx ax, bl                              ; 0f b6 c3
    sal ax, 008h                              ; c1 e0 08
    jmp near 058f8h                           ; e9 5e fe
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    or ah, 080h                               ; 80 cc 80
    jmp near 05979h                           ; e9 d4 fe
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 7a bf
    mov ax, word [bp+012h]                    ; 8b 46 12
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 00645h                               ; 68 45 06
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 ac bf
    add sp, strict byte 00006h                ; 83 c4 06
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 5d bf
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    push ax                                   ; 50
    mov ax, word [bp+010h]                    ; 8b 46 10
    push ax                                   ; 50
    mov ax, word [bp+00ch]                    ; 8b 46 0c
    push ax                                   ; 50
    mov ax, word [bp+012h]                    ; 8b 46 12
    push ax                                   ; 50
    push 0066dh                               ; 68 6d 06
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 86 bf
    add sp, strict byte 0000ch                ; 83 c4 0c
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop di                                    ; 5f
    pop bp                                    ; 5d
    retn                                      ; c3
set_geom_lba_:                               ; 0xf5aee LB 0xe5
    push bx                                   ; 53
    push cx                                   ; 51
    push si                                   ; 56
    push di                                   ; 57
    enter 00008h, 000h                        ; c8 08 00 00
    mov di, ax                                ; 89 c7
    mov es, dx                                ; 8e c2
    mov dword [bp-008h], strict dword 0007e0000h ; 66 c7 46 f8 00 00 7e 00
    mov word [bp-002h], 000ffh                ; c7 46 fe ff 00
    mov ax, word [bp+012h]                    ; 8b 46 12
    mov bx, word [bp+010h]                    ; 8b 5e 10
    mov cx, word [bp+00eh]                    ; 8b 4e 0e
    mov dx, word [bp+00ch]                    ; 8b 56 0c
    mov si, strict word 00020h                ; be 20 00
    call 0a110h                               ; e8 f7 45
    test ax, ax                               ; 85 c0
    jne short 05b29h                          ; 75 0c
    test bx, bx                               ; 85 db
    jne short 05b29h                          ; 75 08
    test cx, cx                               ; 85 c9
    jne short 05b29h                          ; 75 04
    test dx, dx                               ; 85 d2
    je short 05b30h                           ; 74 07
    mov bx, strict word 0ffffh                ; bb ff ff
    mov si, bx                                ; 89 de
    jmp short 05b36h                          ; eb 06
    mov bx, word [bp+00ch]                    ; 8b 5e 0c
    mov si, word [bp+00eh]                    ; 8b 76 0e
    mov word [bp-004h], bx                    ; 89 5e fc
    xor bx, bx                                ; 31 db
    jmp short 05b42h                          ; eb 05
    cmp bx, strict byte 00004h                ; 83 fb 04
    jnl short 05b65h                          ; 7d 23
    mov ax, word [bp-006h]                    ; 8b 46 fa
    cmp si, ax                                ; 39 c6
    jc short 05b53h                           ; 72 0a
    jne short 05b5ch                          ; 75 11
    mov ax, word [bp-004h]                    ; 8b 46 fc
    cmp ax, word [bp-008h]                    ; 3b 46 f8
    jnbe short 05b5ch                         ; 77 09
    mov ax, word [bp-002h]                    ; 8b 46 fe
    inc ax                                    ; 40
    shr ax, 1                                 ; d1 e8
    mov word [bp-002h], ax                    ; 89 46 fe
    shr word [bp-006h], 1                     ; d1 6e fa
    rcr word [bp-008h], 1                     ; d1 5e f8
    inc bx                                    ; 43
    jmp short 05b3dh                          ; eb d8
    mov ax, word [bp-002h]                    ; 8b 46 fe
    xor dx, dx                                ; 31 d2
    mov bx, strict word 0003fh                ; bb 3f 00
    xor cx, cx                                ; 31 c9
    call 0a090h                               ; e8 1e 45
    mov bx, ax                                ; 89 c3
    mov cx, dx                                ; 89 d1
    mov ax, word [bp-004h]                    ; 8b 46 fc
    mov dx, si                                ; 89 f2
    call 0a0d0h                               ; e8 52 45
    mov word [es:di+002h], ax                 ; 26 89 45 02
    cmp ax, 00400h                            ; 3d 00 04
    jbe short 05b8dh                          ; 76 06
    mov word [es:di+002h], 00400h             ; 26 c7 45 02 00 04
    mov ax, word [bp-002h]                    ; 8b 46 fe
    mov word [es:di], ax                      ; 26 89 05
    mov word [es:di+004h], strict word 0003fh ; 26 c7 45 04 3f 00
    leave                                     ; c9
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bx                                    ; 5b
    retn 00008h                               ; c2 08 00
    popaw                                     ; 61
    pop sp                                    ; 5c
    xchg word [si-04ch], bx                   ; 87 5c b4
    pop sp                                    ; 5c
    mov AH, strict byte 05ch                  ; b4 5c
    mov AH, strict byte 05ch                  ; b4 5c
    xchg bp, ax                               ; 95
    pop si                                    ; 5e
    retn                                      ; c3
    pop di                                    ; 5f
    retn                                      ; c3
    pop di                                    ; 5f
    scasb                                     ; ae
    pop si                                    ; 5e
    mov AL, byte [0c35fh]                     ; a0 5f c3
    pop di                                    ; 5f
    retn                                      ; c3
    pop di                                    ; 5f
    mov AL, byte [0a05fh]                     ; a0 5f a0
    pop di                                    ; 5f
    retn                                      ; c3
    pop di                                    ; 5f
    retn                                      ; c3
    pop di                                    ; 5f
    and AL, strict byte 05fh                  ; 24 5f
    mov AL, byte [0c35fh]                     ; a0 5f c3
    pop di                                    ; 5f
    retn                                      ; c3
    pop di                                    ; 5f
    mov AL, byte [0545fh]                     ; a0 5f 54
    pop di                                    ; 5f
    retn                                      ; c3
    pop di                                    ; 5f
    retn                                      ; c3
    pop di                                    ; 5f
    retn                                      ; c3
    pop di                                    ; 5f
_int13_harddisk:                             ; 0xf5bd3 LB 0x44e
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    sub sp, strict byte 00010h                ; 83 ec 10
    or byte [bp+01dh], 002h                   ; 80 4e 1d 02
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 7f bb
    mov si, 00122h                            ; be 22 01
    mov word [bp-004h], ax                    ; 89 46 fc
    xor bx, bx                                ; 31 db
    mov dx, 0008eh                            ; ba 8e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 60 bb
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    xor ah, ah                                ; 30 e4
    cmp ax, 00080h                            ; 3d 80 00
    jc short 05c06h                           ; 72 05
    cmp ax, 00090h                            ; 3d 90 00
    jc short 05c24h                           ; 72 1e
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 00690h                               ; 68 90 06
    push 0069fh                               ; 68 9f 06
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 4d be
    add sp, strict byte 0000ah                ; 83 c4 0a
    jmp near 05fdeh                           ; e9 ba 03
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    xor ah, ah                                ; 30 e4
    mov es, [bp-004h]                         ; 8e 46 fc
    mov bx, si                                ; 89 f3
    add bx, ax                                ; 01 c3
    mov dl, byte [es:bx+00163h]               ; 26 8a 97 63 01
    mov byte [bp-002h], dl                    ; 88 56 fe
    cmp dl, 010h                              ; 80 fa 10
    jc short 05c4dh                           ; 72 10
    push ax                                   ; 50
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 00690h                               ; 68 90 06
    push 006cah                               ; 68 ca 06
    jmp short 05c19h                          ; eb cc
    mov bx, word [bp+016h]                    ; 8b 5e 16
    shr bx, 008h                              ; c1 eb 08
    cmp bx, strict byte 00018h                ; 83 fb 18
    jnbe near 05fc3h                          ; 0f 87 69 03
    add bx, bx                                ; 01 db
    jmp word [cs:bx+05ba1h]                   ; 2e ff a7 a1 5b
    cmp byte [bp-002h], 008h                  ; 80 7e fe 08
    jnc near 05c70h                           ; 0f 83 07 00
    movzx ax, byte [bp-002h]                  ; 0f b6 46 fe
    call 01e75h                               ; e8 05 c2
    mov byte [bp+017h], 000h                  ; c6 46 17 00
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 d8 ba
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 b9 ba
    mov cl, al                                ; 88 c1
    mov dx, word [bp+016h]                    ; 8b 56 16
    xor dh, dh                                ; 30 f6
    movzx ax, cl                              ; 0f b6 c1
    sal ax, 008h                              ; c1 e0 08
    or dx, ax                                 ; 09 c2
    mov word [bp+016h], dx                    ; 89 56 16
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 aa ba
    test cl, cl                               ; 84 c9
    je short 05c74h                           ; 74 c3
    jmp near 05ffah                           ; e9 46 03
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov di, word [bp+014h]                    ; 8b 7e 14
    shr di, 008h                              ; c1 ef 08
    mov ax, word [bp+014h]                    ; 8b 46 14
    xor ah, ah                                ; 30 e4
    sal ax, 002h                              ; c1 e0 02
    xor al, al                                ; 30 c0
    and ah, 003h                              ; 80 e4 03
    or di, ax                                 ; 09 c7
    mov ax, word [bp+014h]                    ; 8b 46 14
    and ax, strict word 0003fh                ; 25 3f 00
    mov word [bp-006h], ax                    ; 89 46 fa
    mov ax, word [bp+012h]                    ; 8b 46 12
    shr ax, 008h                              ; c1 e8 08
    mov word [bp-008h], ax                    ; 89 46 f8
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    cmp ax, 00080h                            ; 3d 80 00
    jnbe short 05cefh                         ; 77 04
    test ax, ax                               ; 85 c0
    jne short 05d12h                          ; 75 23
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 30 bd
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 00690h                               ; 68 90 06
    push 006fch                               ; 68 fc 06
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 5f bd
    add sp, strict byte 00008h                ; 83 c4 08
    jmp near 05fdeh                           ; e9 cc 02
    movzx ax, byte [bp-002h]                  ; 0f b6 46 fe
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov es, [bp-004h]                         ; 8e 46 fc
    mov bx, si                                ; 89 f3
    add bx, ax                                ; 01 c3
    mov ax, word [es:bx+02ch]                 ; 26 8b 47 2c
    mov cx, word [es:bx+02ah]                 ; 26 8b 4f 2a
    mov dx, word [es:bx+02eh]                 ; 26 8b 57 2e
    mov word [bp-00ah], dx                    ; 89 56 f6
    cmp di, ax                                ; 39 c7
    jnc short 05d3fh                          ; 73 0c
    cmp cx, word [bp-008h]                    ; 3b 4e f8
    jbe short 05d3fh                          ; 76 07
    mov ax, word [bp-006h]                    ; 8b 46 fa
    cmp ax, dx                                ; 39 d0
    jbe short 05d6dh                          ; 76 2e
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 e0 bc
    push dword [bp-008h]                      ; 66 ff 76 f8
    push di                                   ; 57
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 00690h                               ; 68 90 06
    push 00724h                               ; 68 24 07
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 04 bd
    add sp, strict byte 00010h                ; 83 c4 10
    jmp near 05fdeh                           ; e9 71 02
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    cmp ax, strict word 00004h                ; 3d 04 00
    jne short 05d7bh                          ; 75 03
    jmp near 05c70h                           ; e9 f5 fe
    movzx bx, byte [bp-002h]                  ; 0f b6 5e fe
    imul bx, bx, strict byte 0001ch           ; 6b db 1c
    mov es, [bp-004h]                         ; 8e 46 fc
    add bx, si                                ; 01 f3
    cmp cx, word [es:bx+030h]                 ; 26 3b 4f 30
    jne short 05d9ch                          ; 75 0f
    mov ax, word [es:bx+034h]                 ; 26 8b 47 34
    cmp ax, word [bp-00ah]                    ; 3b 46 f6
    jne short 05d9ch                          ; 75 06
    cmp byte [bp-002h], 008h                  ; 80 7e fe 08
    jc short 05dcch                           ; 72 30
    mov ax, di                                ; 89 f8
    xor dx, dx                                ; 31 d2
    mov bx, cx                                ; 89 cb
    xor cx, cx                                ; 31 c9
    call 0a090h                               ; e8 e9 42
    xor bx, bx                                ; 31 db
    add ax, word [bp-008h]                    ; 03 46 f8
    adc dx, bx                                ; 11 da
    mov bx, word [bp-00ah]                    ; 8b 5e f6
    xor cx, cx                                ; 31 c9
    call 0a090h                               ; e8 da 42
    xor bx, bx                                ; 31 db
    add ax, word [bp-006h]                    ; 03 46 fa
    adc dx, bx                                ; 11 da
    add ax, strict word 0ffffh                ; 05 ff ff
    mov word [bp-010h], ax                    ; 89 46 f0
    adc dx, strict byte 0ffffh                ; 83 d2 ff
    mov word [bp-00eh], dx                    ; 89 56 f2
    mov word [bp-006h], bx                    ; 89 5e fa
    mov es, [bp-004h]                         ; 8e 46 fc
    db  066h, 026h, 0c7h, 044h, 018h, 000h, 000h, 000h, 000h
    ; mov dword [es:si+018h], strict dword 000000000h ; 66 26 c7 44 18 00 00 00 00
    mov word [es:si+01ch], strict word 00000h ; 26 c7 44 1c 00 00
    mov ax, word [bp-010h]                    ; 8b 46 f0
    mov word [es:si], ax                      ; 26 89 04
    mov ax, word [bp-00eh]                    ; 8b 46 f2
    mov word [es:si+002h], ax                 ; 26 89 44 02
    db  066h, 026h, 0c7h, 044h, 004h, 000h, 000h, 000h, 000h
    ; mov dword [es:si+004h], strict dword 000000000h ; 66 26 c7 44 04 00 00 00 00
    mov ax, word [bp+010h]                    ; 8b 46 10
    mov dx, word [bp+006h]                    ; 8b 56 06
    mov word [es:si+008h], ax                 ; 26 89 44 08
    mov word [es:si+00ah], dx                 ; 26 89 54 0a
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    mov word [es:si+00eh], ax                 ; 26 89 44 0e
    mov word [es:si+010h], 00200h             ; 26 c7 44 10 00 02
    mov word [es:si+012h], di                 ; 26 89 7c 12
    mov ax, word [bp-008h]                    ; 8b 46 f8
    mov word [es:si+014h], ax                 ; 26 89 44 14
    mov ax, word [bp-006h]                    ; 8b 46 fa
    mov word [es:si+016h], ax                 ; 26 89 44 16
    mov al, byte [bp-002h]                    ; 8a 46 fe
    mov byte [es:si+00ch], al                 ; 26 88 44 0c
    movzx ax, byte [bp-002h]                  ; 0f b6 46 fe
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov bx, si                                ; 89 f3
    add bx, ax                                ; 01 c3
    movzx ax, byte [es:bx+022h]               ; 26 0f b6 47 22
    mov bx, ax                                ; 89 c3
    sal bx, 002h                              ; c1 e3 02
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    add ax, ax                                ; 01 c0
    add bx, ax                                ; 01 c3
    push ES                                   ; 06
    push si                                   ; 56
    call word [word bx+0007eh]                ; ff 97 7e 00
    mov dx, ax                                ; 89 c2
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor al, al                                ; 30 c0
    mov es, [bp-004h]                         ; 8e 46 fc
    mov bx, word [es:si+018h]                 ; 26 8b 5c 18
    or bx, ax                                 ; 09 c3
    mov word [bp+016h], bx                    ; 89 5e 16
    test dl, dl                               ; 84 d2
    je near 05c70h                            ; 0f 84 0a fe
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 b9 bb
    movzx ax, dl                              ; 0f b6 c2
    push ax                                   ; 50
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 00690h                               ; 68 90 06
    push 0076bh                               ; 68 6b 07
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 e4 bb
    add sp, strict byte 0000ah                ; 83 c4 0a
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 00ch                               ; 80 cc 0c
    jmp near 05fe6h                           ; e9 51 01
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 8a bb
    push 0078ch                               ; 68 8c 07
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 c3 bb
    add sp, strict byte 00004h                ; 83 c4 04
    jmp near 05c70h                           ; e9 c2 fd
    movzx ax, byte [bp-002h]                  ; 0f b6 46 fe
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov es, [bp-004h]                         ; 8e 46 fc
    mov bx, si                                ; 89 f3
    add bx, ax                                ; 01 c3
    mov di, word [es:bx+02ch]                 ; 26 8b 7f 2c
    mov cx, word [es:bx+02ah]                 ; 26 8b 4f 2a
    mov ax, word [es:bx+02eh]                 ; 26 8b 47 2e
    mov word [bp-00ah], ax                    ; 89 46 f6
    movzx ax, byte [es:si+001e2h]             ; 26 0f b6 84 e2 01
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov byte [bp+016h], 000h                  ; c6 46 16 00
    mov dx, word [bp+014h]                    ; 8b 56 14
    xor dh, dh                                ; 30 f6
    dec di                                    ; 4f
    mov ax, di                                ; 89 f8
    xor ah, ah                                ; 30 e4
    sal ax, 008h                              ; c1 e0 08
    or dx, ax                                 ; 09 c2
    mov word [bp+014h], dx                    ; 89 56 14
    shr di, 002h                              ; c1 ef 02
    and di, 000c0h                            ; 81 e7 c0 00
    mov ax, word [bp-00ah]                    ; 8b 46 f6
    xor ah, ah                                ; 30 e4
    and AL, strict byte 03fh                  ; 24 3f
    or di, ax                                 ; 09 c7
    mov ax, dx                                ; 89 d0
    xor al, dl                                ; 30 d0
    or ax, di                                 ; 09 f8
    mov word [bp+014h], ax                    ; 89 46 14
    mov dx, word [bp+012h]                    ; 8b 56 12
    xor dh, dh                                ; 30 f6
    mov ax, cx                                ; 89 c8
    sal ax, 008h                              ; c1 e0 08
    sub ax, 00100h                            ; 2d 00 01
    or dx, ax                                 ; 09 c2
    mov word [bp+012h], dx                    ; 89 56 12
    mov ax, dx                                ; 89 d0
    xor al, dl                                ; 30 d0
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    or dx, ax                                 ; 09 c2
    mov word [bp+012h], dx                    ; 89 56 12
    jmp near 05c70h                           ; e9 4c fd
    movzx ax, byte [bp-002h]                  ; 0f b6 46 fe
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    imul ax, ax, strict byte 00006h           ; 6b c0 06
    mov es, [bp-004h]                         ; 8e 46 fc
    add si, ax                                ; 01 c6
    mov dx, word [es:si+00206h]               ; 26 8b 94 06 02
    add dx, strict byte 00007h                ; 83 c2 07
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 0c0h                  ; 24 c0
    cmp AL, strict byte 040h                  ; 3c 40
    jne short 05f49h                          ; 75 03
    jmp near 05c70h                           ; e9 27 fd
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 0aah                               ; 80 cc aa
    jmp near 05fe6h                           ; e9 92 00
    movzx ax, byte [bp-002h]                  ; 0f b6 46 fe
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov es, [bp-004h]                         ; 8e 46 fc
    add si, ax                                ; 01 c6
    mov di, word [es:si+032h]                 ; 26 8b 7c 32
    mov ax, word [es:si+030h]                 ; 26 8b 44 30
    mov word [bp-008h], ax                    ; 89 46 f8
    mov ax, word [es:si+034h]                 ; 26 8b 44 34
    mov word [bp-006h], ax                    ; 89 46 fa
    mov ax, di                                ; 89 f8
    xor dx, dx                                ; 31 d2
    mov bx, word [bp-008h]                    ; 8b 5e f8
    xor cx, cx                                ; 31 c9
    call 0a090h                               ; e8 12 41
    mov bx, word [bp-006h]                    ; 8b 5e fa
    xor cx, cx                                ; 31 c9
    call 0a090h                               ; e8 0a 41
    mov word [bp-010h], ax                    ; 89 46 f0
    mov word [bp-00eh], dx                    ; 89 56 f2
    mov word [bp+014h], dx                    ; 89 56 14
    mov word [bp+012h], ax                    ; 89 46 12
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 003h                               ; 80 cc 03
    mov word [bp+016h], ax                    ; 89 46 16
    jmp near 05c74h                           ; e9 d4 fc
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 7f ba
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 00690h                               ; 68 90 06
    push 007a6h                               ; 68 a6 07
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 ae ba
    add sp, strict byte 00008h                ; 83 c4 08
    jmp near 05c70h                           ; e9 ad fc
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 5c ba
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 00690h                               ; 68 90 06
    push 007d9h                               ; 68 d9 07
    jmp near 05d07h                           ; e9 29 fd
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 001h                               ; 80 cc 01
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, word [bp+016h]                    ; 8b 5e 16
    shr bx, 008h                              ; c1 eb 08
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 5d b7
    or byte [bp+01ch], 001h                   ; 80 4e 1c 01
    jmp near 05c83h                           ; e9 82 fc
    mov di, 0d760h                            ; bf 60 d7
    pushaw                                    ; 60
    xlatb                                     ; d7
    pushaw                                    ; 60
    xlatb                                     ; d7
    pushaw                                    ; 60
    stosw                                     ; ab
    db  064h, 02ch, 062h
    ; fs sub AL, strict byte 062h               ; 64 2c 62
    xlatb                                     ; d7
    pushaw                                    ; 60
    xor ah, byte [bp+si-055h]                 ; 32 62 ab
    db  064h, 0fah
    ; fs cli                                    ; 64 fa
    db  064h, 0fah
    ; fs cli                                    ; 64 fa
    db  064h, 0fah
    ; fs cli                                    ; 64 fa
    db  064h, 0fah
    ; fs cli                                    ; 64 fa
    db  064h, 0c2h, 064h, 0fah
    ; fs retn 0fa64h                            ; 64 c2 64 fa
    db  064h, 0fah
    ; fs cli                                    ; 64 fa
    db  064h
_int13_harddisk_ext:                         ; 0xf6021 LB 0x4f4
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    sub sp, strict byte 00026h                ; 83 ec 26
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 35 b7
    mov word [bp-014h], ax                    ; 89 46 ec
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 29 b7
    mov word [bp-008h], 00122h                ; c7 46 f8 22 01
    mov word [bp-006h], ax                    ; 89 46 fa
    xor bx, bx                                ; 31 db
    mov dx, 0008eh                            ; ba 8e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 08 b7
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    xor ah, ah                                ; 30 e4
    cmp ax, 00080h                            ; 3d 80 00
    jc short 0605eh                           ; 72 05
    cmp ax, 00090h                            ; 3d 90 00
    jc short 0607ch                           ; 72 1e
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 00807h                               ; 68 07 08
    push 0069fh                               ; 68 9f 06
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 f5 b9
    add sp, strict byte 0000ah                ; 83 c4 0a
    jmp near 064d8h                           ; e9 5c 04
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    xor ah, ah                                ; 30 e4
    les bx, [bp-008h]                         ; c4 5e f8
    add bx, ax                                ; 01 c3
    mov dl, byte [es:bx+00163h]               ; 26 8a 97 63 01
    mov byte [bp-004h], dl                    ; 88 56 fc
    cmp dl, 010h                              ; 80 fa 10
    jc short 060a3h                           ; 72 10
    push ax                                   ; 50
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 00807h                               ; 68 07 08
    push 006cah                               ; 68 ca 06
    jmp short 06071h                          ; eb ce
    mov bx, word [bp+016h]                    ; 8b 5e 16
    shr bx, 008h                              ; c1 eb 08
    sub bx, strict byte 00041h                ; 83 eb 41
    cmp bx, strict byte 0000fh                ; 83 fb 0f
    jnbe near 064fah                          ; 0f 87 47 04
    add bx, bx                                ; 01 db
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    jmp word [cs:bx+06001h]                   ; 2e ff a7 01 60
    mov word [bp+010h], 0aa55h                ; c7 46 10 55 aa
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 030h                               ; 80 cc 30
    mov word [bp+016h], ax                    ; 89 46 16
    mov word [bp+014h], strict word 00007h    ; c7 46 14 07 00
    jmp near 064afh                           ; e9 d8 03
    mov di, word [bp+00ah]                    ; 8b 7e 0a
    mov es, [bp+004h]                         ; 8e 46 04
    mov word [bp-01ch], di                    ; 89 7e e4
    mov [bp-01ah], es                         ; 8c 46 e6
    mov ax, word [es:di+002h]                 ; 26 8b 45 02
    mov word [bp-012h], ax                    ; 89 46 ee
    mov ax, word [es:di+006h]                 ; 26 8b 45 06
    mov word [bp-016h], ax                    ; 89 46 ea
    mov ax, word [es:di+004h]                 ; 26 8b 45 04
    mov word [bp-018h], ax                    ; 89 46 e8
    mov dx, word [es:di+00ch]                 ; 26 8b 55 0c
    mov cx, word [es:di+00eh]                 ; 26 8b 4d 0e
    xor ax, ax                                ; 31 c0
    xor bx, bx                                ; 31 db
    mov si, strict word 00020h                ; be 20 00
    call 0a120h                               ; e8 16 40
    mov word [bp-00eh], bx                    ; 89 5e f2
    mov bx, word [es:di+008h]                 ; 26 8b 5d 08
    mov di, word [es:di+00ah]                 ; 26 8b 7d 0a
    or dx, bx                                 ; 09 da
    or cx, di                                 ; 09 f9
    movzx bx, byte [bp-004h]                  ; 0f b6 5e fc
    imul bx, bx, strict byte 0001ch           ; 6b db 1c
    les di, [bp-008h]                         ; c4 7e f8
    add di, bx                                ; 01 df
    mov bl, byte [es:di+022h]                 ; 26 8a 5d 22
    cmp ax, word [es:di+03ch]                 ; 26 3b 45 3c
    jnbe short 06151h                         ; 77 22
    jne short 06174h                          ; 75 43
    mov si, word [bp-00eh]                    ; 8b 76 f2
    cmp si, word [es:di+03ah]                 ; 26 3b 75 3a
    jnbe short 06151h                         ; 77 17
    mov si, word [bp-00eh]                    ; 8b 76 f2
    cmp si, word [es:di+03ah]                 ; 26 3b 75 3a
    jne short 06174h                          ; 75 31
    cmp cx, word [es:di+038h]                 ; 26 3b 4d 38
    jnbe short 06151h                         ; 77 08
    jne short 06174h                          ; 75 29
    cmp dx, word [es:di+036h]                 ; 26 3b 55 36
    jc short 06174h                           ; 72 23
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 ce b8
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 00807h                               ; 68 07 08
    push 0081ah                               ; 68 1a 08
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 fd b8
    add sp, strict byte 00008h                ; 83 c4 08
    jmp near 064d8h                           ; e9 64 03
    mov di, word [bp+016h]                    ; 8b 7e 16
    shr di, 008h                              ; c1 ef 08
    cmp di, strict byte 00044h                ; 83 ff 44
    je near 064abh                            ; 0f 84 2a 03
    cmp di, strict byte 00047h                ; 83 ff 47
    je near 064abh                            ; 0f 84 23 03
    les si, [bp-008h]                         ; c4 76 f8
    db  066h, 026h, 0c7h, 044h, 018h, 000h, 000h, 000h, 000h
    ; mov dword [es:si+018h], strict dword 000000000h ; 66 26 c7 44 18 00 00 00 00
    mov word [es:si+01ch], strict word 00000h ; 26 c7 44 1c 00 00
    mov word [es:si+006h], ax                 ; 26 89 44 06
    mov ax, word [bp-00eh]                    ; 8b 46 f2
    mov word [es:si+004h], ax                 ; 26 89 44 04
    mov word [es:si+002h], cx                 ; 26 89 4c 02
    mov word [es:si], dx                      ; 26 89 14
    mov ax, word [bp-018h]                    ; 8b 46 e8
    mov word [es:si+008h], ax                 ; 26 89 44 08
    mov ax, word [bp-016h]                    ; 8b 46 ea
    mov word [es:si+00ah], ax                 ; 26 89 44 0a
    mov ax, word [bp-012h]                    ; 8b 46 ee
    mov word [es:si+00eh], ax                 ; 26 89 44 0e
    mov word [es:si+010h], 00200h             ; 26 c7 44 10 00 02
    mov word [es:si+016h], strict word 00000h ; 26 c7 44 16 00 00
    mov al, byte [bp-004h]                    ; 8a 46 fc
    mov byte [es:si+00ch], al                 ; 26 88 44 0c
    mov dx, di                                ; 89 fa
    add dx, di                                ; 01 fa
    movzx ax, bl                              ; 0f b6 c3
    mov bx, ax                                ; 89 c3
    sal bx, 002h                              ; c1 e3 02
    add bx, dx                                ; 01 d3
    push ES                                   ; 06
    push si                                   ; 56
    call word [word bx-00002h]                ; ff 97 fe ff
    mov dx, ax                                ; 89 c2
    mov es, [bp-006h]                         ; 8e 46 fa
    mov bx, si                                ; 89 f3
    mov ax, word [es:bx+018h]                 ; 26 8b 47 18
    mov word [bp-012h], ax                    ; 89 46 ee
    les bx, [bp-01ch]                         ; c4 5e e4
    mov word [es:bx+002h], ax                 ; 26 89 47 02
    test dl, dl                               ; 84 d2
    je near 064abh                            ; 0f 84 a8 02
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 1c b8
    movzx ax, dl                              ; 0f b6 c2
    push ax                                   ; 50
    push di                                   ; 57
    push 00807h                               ; 68 07 08
    push 0076bh                               ; 68 6b 07
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 4d b8
    add sp, strict byte 0000ah                ; 83 c4 0a
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 00ch                               ; 80 cc 0c
    jmp near 064e0h                           ; e9 b4 02
    or ah, 0b2h                               ; 80 cc b2
    jmp near 064e0h                           ; e9 ae 02
    mov bx, word [bp+00ah]                    ; 8b 5e 0a
    mov ax, word [bp+004h]                    ; 8b 46 04
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov di, bx                                ; 89 df
    mov word [bp-00ah], ax                    ; 89 46 f6
    mov es, ax                                ; 8e c0
    mov ax, word [es:bx]                      ; 26 8b 07
    mov word [bp-010h], ax                    ; 89 46 f0
    cmp ax, strict word 0001ah                ; 3d 1a 00
    jc near 064d8h                            ; 0f 82 89 02
    jc near 062eeh                            ; 0f 82 9b 00
    movzx ax, byte [bp-004h]                  ; 0f b6 46 fc
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    les bx, [bp-008h]                         ; c4 5e f8
    add bx, ax                                ; 01 c3
    mov ax, word [es:bx+032h]                 ; 26 8b 47 32
    mov word [bp-026h], ax                    ; 89 46 da
    mov ax, word [es:bx+030h]                 ; 26 8b 47 30
    mov word [bp-020h], ax                    ; 89 46 e0
    mov ax, word [es:bx+034h]                 ; 26 8b 47 34
    mov word [bp-024h], ax                    ; 89 46 dc
    mov ax, word [es:bx+03ch]                 ; 26 8b 47 3c
    mov dx, word [es:bx+03ah]                 ; 26 8b 57 3a
    mov word [bp-00eh], dx                    ; 89 56 f2
    mov cx, word [es:bx+038h]                 ; 26 8b 4f 38
    mov dx, word [es:bx+036h]                 ; 26 8b 57 36
    mov bx, word [es:bx+028h]                 ; 26 8b 5f 28
    mov word [bp-022h], bx                    ; 89 5e de
    mov es, [bp-00ch]                         ; 8e 46 f4
    mov bx, di                                ; 89 fb
    db  066h, 026h, 0c7h, 007h, 01ah, 000h, 002h, 000h
    ; mov dword [es:bx], strict dword 00002001ah ; 66 26 c7 07 1a 00 02 00
    mov bx, word [bp-026h]                    ; 8b 5e da
    mov si, di                                ; 89 fe
    mov word [es:si+004h], bx                 ; 26 89 5c 04
    mov bx, si                                ; 89 f3
    mov word [es:bx+006h], strict word 00000h ; 26 c7 47 06 00 00
    mov bx, word [bp-020h]                    ; 8b 5e e0
    mov word [es:si+008h], bx                 ; 26 89 5c 08
    mov bx, si                                ; 89 f3
    mov word [es:bx+00ah], strict word 00000h ; 26 c7 47 0a 00 00
    mov bx, word [bp-024h]                    ; 8b 5e dc
    mov word [es:si+00ch], bx                 ; 26 89 5c 0c
    mov bx, si                                ; 89 f3
    mov word [es:bx+00eh], strict word 00000h ; 26 c7 47 0e 00 00
    mov bx, word [bp-022h]                    ; 8b 5e de
    mov word [es:si+018h], bx                 ; 26 89 5c 18
    mov bx, si                                ; 89 f3
    mov word [es:bx+010h], dx                 ; 26 89 57 10
    mov word [es:bx+012h], cx                 ; 26 89 4f 12
    mov bx, word [bp-00eh]                    ; 8b 5e f2
    mov si, strict word 00020h                ; be 20 00
    call 0a110h                               ; e8 2c 3e
    mov bx, di                                ; 89 fb
    mov word [es:bx+014h], dx                 ; 26 89 57 14
    mov word [es:bx+016h], cx                 ; 26 89 4f 16
    cmp word [bp-010h], strict byte 0001eh    ; 83 7e f0 1e
    jc near 063f3h                            ; 0f 82 fd 00
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov word [es:di], strict word 0001eh      ; 26 c7 05 1e 00
    mov ax, word [bp-014h]                    ; 8b 46 ec
    mov word [es:di+01ch], ax                 ; 26 89 45 1c
    mov word [es:di+01ah], 00356h             ; 26 c7 45 1a 56 03
    movzx cx, byte [bp-004h]                  ; 0f b6 4e fc
    mov ax, cx                                ; 89 c8
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    movzx bx, al                              ; 0f b6 d8
    imul bx, bx, strict byte 00006h           ; 6b db 06
    mov es, [bp-006h]                         ; 8e 46 fa
    add bx, word [bp-008h]                    ; 03 5e f8
    mov ax, word [es:bx+00206h]               ; 26 8b 87 06 02
    mov word [bp-01eh], ax                    ; 89 46 e2
    mov dx, word [es:bx+00208h]               ; 26 8b 97 08 02
    mov al, byte [es:bx+00205h]               ; 26 8a 87 05 02
    mov byte [bp-002h], al                    ; 88 46 fe
    imul bx, cx, strict byte 0001ch           ; 6b d9 1c
    add bx, word [bp-008h]                    ; 03 5e f8
    mov ah, byte [es:bx+026h]                 ; 26 8a 67 26
    mov al, byte [es:bx+027h]                 ; 26 8a 47 27
    test al, al                               ; 84 c0
    jne short 0634dh                          ; 75 04
    xor bx, bx                                ; 31 db
    jmp short 06350h                          ; eb 03
    mov bx, strict word 00008h                ; bb 08 00
    or bl, 010h                               ; 80 cb 10
    cmp ah, 001h                              ; 80 fc 01
    db  00fh, 094h, 0c4h
    ; sete ah                                   ; 0f 94 c4
    movzx cx, ah                              ; 0f b6 cc
    or bx, cx                                 ; 09 cb
    cmp AL, strict byte 001h                  ; 3c 01
    db  00fh, 094h, 0c4h
    ; sete ah                                   ; 0f 94 c4
    movzx cx, ah                              ; 0f b6 cc
    or bx, cx                                 ; 09 cb
    cmp AL, strict byte 003h                  ; 3c 03
    jne short 06371h                          ; 75 05
    mov ax, strict word 00003h                ; b8 03 00
    jmp short 06373h                          ; eb 02
    xor ax, ax                                ; 31 c0
    or bx, ax                                 ; 09 c3
    mov ax, word [bp-01eh]                    ; 8b 46 e2
    les si, [bp-008h]                         ; c4 76 f8
    mov word [es:si+00234h], ax               ; 26 89 84 34 02
    mov word [es:si+00236h], dx               ; 26 89 94 36 02
    movzx ax, byte [bp-004h]                  ; 0f b6 46 fc
    cwd                                       ; 99
    mov cx, strict word 00002h                ; b9 02 00
    idiv cx                                   ; f7 f9
    or dl, 00eh                               ; 80 ca 0e
    mov ax, dx                                ; 89 d0
    sal ax, 004h                              ; c1 e0 04
    mov byte [es:si+00238h], al               ; 26 88 84 38 02
    mov byte [es:si+00239h], 0cbh             ; 26 c6 84 39 02 cb
    mov al, byte [bp-002h]                    ; 8a 46 fe
    mov byte [es:si+0023ah], al               ; 26 88 84 3a 02
    mov word [es:si+0023bh], strict word 00001h ; 26 c7 84 3b 02 01 00
    mov byte [es:si+0023dh], 000h             ; 26 c6 84 3d 02 00
    mov word [es:si+0023eh], bx               ; 26 89 9c 3e 02
    mov bx, si                                ; 89 f3
    mov word [es:bx+00240h], strict word 00000h ; 26 c7 87 40 02 00 00
    mov byte [es:bx+00242h], 011h             ; 26 c6 87 42 02 11
    xor bl, bl                                ; 30 db
    xor bh, bh                                ; 30 ff
    jmp short 063d6h                          ; eb 05
    cmp bh, 00fh                              ; 80 ff 0f
    jnc short 063e9h                          ; 73 13
    movzx dx, bh                              ; 0f b6 d7
    add dx, 00356h                            ; 81 c2 56 03
    mov ax, word [bp-014h]                    ; 8b 46 ec
    call 01749h                               ; e8 66 b3
    add bl, al                                ; 00 c3
    db  0feh, 0c7h
    ; inc bh                                    ; fe c7
    jmp short 063d1h                          ; eb e8
    neg bl                                    ; f6 db
    les si, [bp-008h]                         ; c4 76 f8
    mov byte [es:si+00243h], bl               ; 26 88 9c 43 02
    cmp word [bp-010h], strict byte 00042h    ; 83 7e f0 42
    jc near 064abh                            ; 0f 82 b0 00
    movzx ax, byte [bp-004h]                  ; 0f b6 46 fc
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    xor ah, ah                                ; 30 e4
    imul ax, ax, strict byte 00006h           ; 6b c0 06
    les bx, [bp-008h]                         ; c4 5e f8
    add bx, ax                                ; 01 c3
    mov al, byte [es:bx+00204h]               ; 26 8a 87 04 02
    mov dx, word [es:bx+00206h]               ; 26 8b 97 06 02
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov word [es:di], strict word 00042h      ; 26 c7 05 42 00
    db  066h, 026h, 0c7h, 045h, 01eh, 0ddh, 0beh, 024h, 000h
    ; mov dword [es:di+01eh], strict dword 00024beddh ; 66 26 c7 45 1e dd be 24 00
    mov word [es:di+022h], strict word 00000h ; 26 c7 45 22 00 00
    test al, al                               ; 84 c0
    jne short 0643ch                          ; 75 09
    db  066h, 026h, 0c7h, 045h, 024h, 049h, 053h, 041h, 020h
    ; mov dword [es:di+024h], strict dword 020415349h ; 66 26 c7 45 24 49 53 41 20
    mov es, [bp-00ah]                         ; 8e 46 f6
    db  066h, 026h, 0c7h, 045h, 028h, 041h, 054h, 041h, 020h
    ; mov dword [es:di+028h], strict dword 020415441h ; 66 26 c7 45 28 41 54 41 20
    db  066h, 026h, 0c7h, 045h, 02ch, 020h, 020h, 020h, 020h
    ; mov dword [es:di+02ch], strict dword 020202020h ; 66 26 c7 45 2c 20 20 20 20
    test al, al                               ; 84 c0
    jne short 06468h                          ; 75 13
    mov word [es:di+030h], dx                 ; 26 89 55 30
    db  066h, 026h, 0c7h, 045h, 032h, 000h, 000h, 000h, 000h
    ; mov dword [es:di+032h], strict dword 000000000h ; 66 26 c7 45 32 00 00 00 00
    mov word [es:di+036h], strict word 00000h ; 26 c7 45 36 00 00
    mov al, byte [bp-004h]                    ; 8a 46 fc
    and AL, strict byte 001h                  ; 24 01
    xor ah, ah                                ; 30 e4
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov word [es:di+038h], ax                 ; 26 89 45 38
    db  066h, 026h, 0c7h, 045h, 03ah, 000h, 000h, 000h, 000h
    ; mov dword [es:di+03ah], strict dword 000000000h ; 66 26 c7 45 3a 00 00 00 00
    mov word [es:di+03eh], strict word 00000h ; 26 c7 45 3e 00 00
    xor bl, bl                                ; 30 db
    mov BH, strict byte 01eh                  ; b7 1e
    jmp short 06490h                          ; eb 05
    cmp bh, 040h                              ; 80 ff 40
    jnc short 064a2h                          ; 73 12
    movzx dx, bh                              ; 0f b6 d7
    add dx, word [bp+00ah]                    ; 03 56 0a
    mov ax, word [bp+004h]                    ; 8b 46 04
    call 01749h                               ; e8 ad b2
    add bl, al                                ; 00 c3
    db  0feh, 0c7h
    ; inc bh                                    ; fe c7
    jmp short 0648bh                          ; eb e9
    neg bl                                    ; f6 db
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov byte [es:di+041h], bl                 ; 26 88 5d 41
    mov byte [bp+017h], 000h                  ; c6 46 17 00
    xor bx, bx                                ; 31 db
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 9d b2
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
    cmp ax, strict word 00006h                ; 3d 06 00
    je short 064abh                           ; 74 e4
    cmp ax, strict word 00001h                ; 3d 01 00
    jc short 064d8h                           ; 72 0c
    jbe short 064abh                          ; 76 dd
    cmp ax, strict word 00003h                ; 3d 03 00
    jc short 064d8h                           ; 72 05
    cmp ax, strict word 00004h                ; 3d 04 00
    jbe short 064abh                          ; 76 d3
    mov ax, word [bp+016h]                    ; 8b 46 16
    xor ah, ah                                ; 30 e4
    or ah, 001h                               ; 80 cc 01
    mov word [bp+016h], ax                    ; 89 46 16
    mov bx, word [bp+016h]                    ; 8b 5e 16
    shr bx, 008h                              ; c1 eb 08
    xor bh, bh                                ; 30 ff
    mov dx, strict word 00074h                ; ba 74 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 63 b2
    or byte [bp+01ch], 001h                   ; 80 4e 1c 01
    jmp short 064beh                          ; eb c4
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 25 b5
    mov ax, word [bp+016h]                    ; 8b 46 16
    shr ax, 008h                              ; c1 e8 08
    push ax                                   ; 50
    push 00807h                               ; 68 07 08
    push 007d9h                               ; 68 d9 07
    jmp near 06169h                           ; e9 54 fc
_int14_function:                             ; 0xf6515 LB 0x15a
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    sti                                       ; fb
    mov dx, word [bp+00eh]                    ; 8b 56 0e
    add dx, dx                                ; 01 d2
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 40 b2
    mov si, ax                                ; 89 c6
    mov bx, ax                                ; 89 c3
    mov dx, word [bp+00eh]                    ; 8b 56 0e
    add dx, strict byte 0007ch                ; 83 c2 7c
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 14 b2
    mov cl, al                                ; 88 c1
    cmp word [bp+00eh], strict byte 00004h    ; 83 7e 0e 04
    jnc near 06665h                           ; 0f 83 26 01
    test si, si                               ; 85 f6
    jbe near 06665h                           ; 0f 86 20 01
    mov al, byte [bp+013h]                    ; 8a 46 13
    cmp AL, strict byte 001h                  ; 3c 01
    jc short 0655dh                           ; 72 11
    jbe short 065b6h                          ; 76 68
    cmp AL, strict byte 003h                  ; 3c 03
    je near 0664eh                            ; 0f 84 fa 00
    cmp AL, strict byte 002h                  ; 3c 02
    je near 06604h                            ; 0f 84 aa 00
    jmp near 0665fh                           ; e9 02 01
    test al, al                               ; 84 c0
    jne near 0665fh                           ; 0f 85 fc 00
    lea dx, [bx+003h]                         ; 8d 57 03
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    or AL, strict byte 080h                   ; 0c 80
    out DX, AL                                ; ee
    lea si, [bx+001h]                         ; 8d 77 01
    mov al, byte [bp+012h]                    ; 8a 46 12
    test AL, strict byte 0e0h                 ; a8 e0
    jne short 06582h                          ; 75 0c
    mov AL, strict byte 017h                  ; b0 17
    mov dx, bx                                ; 89 da
    out DX, AL                                ; ee
    mov AL, strict byte 004h                  ; b0 04
    mov dx, si                                ; 89 f2
    out DX, AL                                ; ee
    jmp short 06598h                          ; eb 16
    and AL, strict byte 0e0h                  ; 24 e0
    movzx cx, al                              ; 0f b6 c8
    sar cx, 005h                              ; c1 f9 05
    mov ax, 00600h                            ; b8 00 06
    sar ax, CL                                ; d3 f8
    mov dx, bx                                ; 89 da
    out DX, AL                                ; ee
    shr ax, 008h                              ; c1 e8 08
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
    jmp near 06640h                           ; e9 8a 00
    mov dx, strict word 0006ch                ; ba 6c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 a6 b1
    mov si, ax                                ; 89 c6
    lea dx, [bx+005h]                         ; 8d 57 05
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and ax, strict word 00060h                ; 25 60 00
    cmp ax, strict word 00060h                ; 3d 60 00
    je short 065e6h                           ; 74 17
    test cl, cl                               ; 84 c9
    je short 065e6h                           ; 74 13
    mov dx, strict word 0006ch                ; ba 6c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 89 b1
    cmp ax, si                                ; 39 f0
    je short 065c1h                           ; 74 e1
    mov si, ax                                ; 89 c6
    db  0feh, 0c9h
    ; dec cl                                    ; fe c9
    jmp short 065c1h                          ; eb db
    test cl, cl                               ; 84 c9
    je short 065f0h                           ; 74 06
    mov al, byte [bp+012h]                    ; 8a 46 12
    mov dx, bx                                ; 89 da
    out DX, AL                                ; ee
    lea dx, [bx+005h]                         ; 8d 57 05
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov byte [bp+013h], al                    ; 88 46 13
    test cl, cl                               ; 84 c9
    jne short 06640h                          ; 75 43
    or AL, strict byte 080h                   ; 0c 80
    mov byte [bp+013h], al                    ; 88 46 13
    jmp short 06640h                          ; eb 3c
    mov dx, strict word 0006ch                ; ba 6c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 58 b1
    mov si, ax                                ; 89 c6
    lea dx, [bx+005h]                         ; 8d 57 05
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 06630h                          ; 75 17
    test cl, cl                               ; 84 c9
    je short 06630h                           ; 74 13
    mov dx, strict word 0006ch                ; ba 6c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 3f b1
    cmp ax, si                                ; 39 f0
    je short 0660fh                           ; 74 e5
    mov si, ax                                ; 89 c6
    db  0feh, 0c9h
    ; dec cl                                    ; fe c9
    jmp short 0660fh                          ; eb df
    test cl, cl                               ; 84 c9
    je short 06646h                           ; 74 12
    mov byte [bp+013h], 000h                  ; c6 46 13 00
    mov dx, bx                                ; 89 da
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov byte [bp+012h], al                    ; 88 46 12
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    jmp short 06669h                          ; eb 23
    lea dx, [bx+005h]                         ; 8d 57 05
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    jmp short 065ffh                          ; eb b1
    lea dx, [si+005h]                         ; 8d 54 05
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov byte [bp+013h], al                    ; 88 46 13
    lea dx, [si+006h]                         ; 8d 54 06
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    jmp short 0663dh                          ; eb de
    or byte [bp+01ch], 001h                   ; 80 4e 1c 01
    jmp short 06669h                          ; eb 04
    or byte [bp+01ch], 001h                   ; 80 4e 1c 01
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
timer_wait_:                                 ; 0xf666f LB 0x43
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push ax                                   ; 50
    mov bx, strict word 0000fh                ; bb 0f 00
    xor cx, cx                                ; 31 c9
    call 0a0d0h                               ; e8 53 3a
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
    jne short 0669ch                          ; 75 05
    cmp cx, strict byte 0ffffh                ; 83 f9 ff
    je short 066abh                           ; 74 0f
    mov dx, strict word 00061h                ; ba 61 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 010h                  ; 24 10
    cmp al, byte [bp-006h]                    ; 3a 46 fa
    jne short 0669ch                          ; 75 f3
    jmp short 0668ch                          ; eb e1
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
set_enable_a20_:                             ; 0xf66b2 LB 0x2c
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
    je short 066cbh                           ; 74 05
    or AL, strict byte 002h                   ; 0c 02
    out DX, AL                                ; ee
    jmp short 066ceh                          ; eb 03
    and AL, strict byte 0fdh                  ; 24 fd
    out DX, AL                                ; ee
    test cl, 002h                             ; f6 c1 02
    db  00fh, 095h, 0c0h
    ; setne al                                  ; 0f 95 c0
    xor ah, ah                                ; 30 e4
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
set_e820_range_:                             ; 0xf66de LB 0x89
    push si                                   ; 56
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov es, ax                                ; 8e c0
    mov si, dx                                ; 89 d6
    mov word [es:si], bx                      ; 26 89 1c
    mov word [es:si+002h], cx                 ; 26 89 4c 02
    movzx ax, byte [bp+00ah]                  ; 0f b6 46 0a
    mov word [es:si+004h], ax                 ; 26 89 44 04
    mov word [es:si+006h], strict word 00000h ; 26 c7 44 06 00 00
    sub word [bp+006h], bx                    ; 29 5e 06
    sbb word [bp+008h], cx                    ; 19 4e 08
    mov al, byte [bp+00ah]                    ; 8a 46 0a
    sub byte [bp+00ch], al                    ; 28 46 0c
    mov ax, word [bp+006h]                    ; 8b 46 06
    mov word [es:si+008h], ax                 ; 26 89 44 08
    mov ax, word [bp+008h]                    ; 8b 46 08
    mov word [es:si+00ah], ax                 ; 26 89 44 0a
    movzx ax, byte [bp+00ch]                  ; 0f b6 46 0c
    mov word [es:si+00ch], ax                 ; 26 89 44 0c
    mov word [es:si+00eh], strict word 00000h ; 26 c7 44 0e 00 00
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    mov word [es:si+010h], ax                 ; 26 89 44 10
    mov word [es:si+012h], strict word 00000h ; 26 c7 44 12 00 00
    pop bp                                    ; 5d
    pop si                                    ; 5e
    retn 0000ah                               ; c2 0a 00
    db  0ech, 0e9h, 0d8h, 0c1h, 0c0h, 0bfh, 091h, 090h, 089h, 088h, 083h, 052h, 04fh, 041h, 024h, 000h
    db  06bh, 06ah, 0a4h, 067h, 0b7h, 067h, 04ch, 068h, 052h, 068h, 057h, 068h, 05ch, 068h, 0feh, 068h
    db  028h, 069h, 045h, 068h, 045h, 068h, 0f5h, 069h, 01dh, 06ah, 030h, 06ah, 03fh, 06ah, 04ch, 068h
    db  046h, 06ah
_int15_function:                             ; 0xf6767 LB 0x336
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    mov ax, word [bp+012h]                    ; 8b 46 12
    shr ax, 008h                              ; c1 e8 08
    cmp ax, 000ech                            ; 3d ec 00
    jnbe near 06a6bh                          ; 0f 87 f2 02
    push CS                                   ; 0e
    pop ES                                    ; 07
    mov cx, strict word 00011h                ; b9 11 00
    mov di, 06735h                            ; bf 35 67
    repne scasb                               ; f2 ae
    sal cx, 1                                 ; d1 e1
    mov di, cx                                ; 89 cf
    mov si, word [cs:di+06745h]               ; 2e 8b b5 45 67
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    mov cx, word [bp+018h]                    ; 8b 4e 18
    and cl, 0feh                              ; 80 e1 fe
    mov bx, word [bp+018h]                    ; 8b 5e 18
    or bl, 001h                               ; 80 cb 01
    mov dx, ax                                ; 89 c2
    or dh, 086h                               ; 80 ce 86
    jmp si                                    ; ff e6
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    cmp ax, 000c0h                            ; 3d c0 00
    jne near 06a6bh                           ; 0f 85 bb 02
    or byte [bp+018h], 001h                   ; 80 4e 18 01
    jmp near 06a14h                           ; e9 5d 02
    mov dx, ax                                ; 89 c2
    cmp ax, strict word 00001h                ; 3d 01 00
    jc short 067cch                           ; 72 0e
    jbe short 067e0h                          ; 76 20
    cmp ax, strict word 00003h                ; 3d 03 00
    je short 0680dh                           ; 74 48
    cmp ax, strict word 00002h                ; 3d 02 00
    je short 067f0h                           ; 74 26
    jmp short 0681ah                          ; eb 4e
    test ax, ax                               ; 85 c0
    jne short 0681ah                          ; 75 4a
    xor ax, ax                                ; 31 c0
    call 066b2h                               ; e8 dd fe
    and byte [bp+018h], 0feh                  ; 80 66 18 fe
    mov byte [bp+013h], 000h                  ; c6 46 13 00
    jmp near 06845h                           ; e9 65 00
    mov ax, strict word 00001h                ; b8 01 00
    call 066b2h                               ; e8 cc fe
    and byte [bp+018h], 0feh                  ; 80 66 18 fe
    mov byte [bp+013h], dh                    ; 88 76 13
    jmp near 06845h                           ; e9 55 00
    mov dx, 00092h                            ; ba 92 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    shr ax, 1                                 ; d1 e8
    and ax, strict word 00001h                ; 25 01 00
    mov dx, word [bp+012h]                    ; 8b 56 12
    mov dl, al                                ; 88 c2
    mov word [bp+012h], dx                    ; 89 56 12
    and byte [bp+018h], 0feh                  ; 80 66 18 fe
    mov byte [bp+013h], ah                    ; 88 66 13
    jmp near 06845h                           ; e9 38 00
    and byte [bp+018h], 0feh                  ; 80 66 18 fe
    mov byte [bp+013h], ah                    ; 88 66 13
    mov word [bp+00ch], ax                    ; 89 46 0c
    jmp near 06845h                           ; e9 2b 00
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 05 b2
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    push 00840h                               ; 68 40 08
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 38 b2
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
    mov word [bp+018h], bx                    ; 89 5e 18
    jmp near 068f8h                           ; e9 a6 00
    mov word [bp+018h], bx                    ; 89 5e 18
    jmp short 06845h                          ; eb ee
    mov word [bp+018h], cx                    ; 89 4e 18
    jmp short 06842h                          ; eb e6
    test byte [bp+012h], 0ffh                 ; f6 46 12 ff
    jne short 068ceh                          ; 75 6c
    mov dx, 000a0h                            ; ba a0 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 de ae
    test AL, strict byte 001h                 ; a8 01
    jne near 06a0bh                           ; 0f 85 9a 01
    mov bx, strict word 00001h                ; bb 01 00
    mov dx, 000a0h                            ; ba a0 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 da ae
    mov bx, word [bp+014h]                    ; 8b 5e 14
    mov dx, 00098h                            ; ba 98 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01773h                               ; e8 ea ae
    mov bx, word [bp+00ch]                    ; 8b 5e 0c
    mov dx, 0009ah                            ; ba 9a 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01773h                               ; e8 de ae
    mov bx, word [bp+00eh]                    ; 8b 5e 0e
    mov dx, 0009ch                            ; ba 9c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01773h                               ; e8 d2 ae
    mov bx, word [bp+010h]                    ; 8b 5e 10
    mov dx, 0009eh                            ; ba 9e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01773h                               ; e8 c6 ae
    and byte [bp+018h], 0feh                  ; 80 66 18 fe
    mov dx, 000a1h                            ; ba a1 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 0feh                  ; 24 fe
    out DX, AL                                ; ee
    mov ax, strict word 0000bh                ; b8 0b 00
    call 017a5h                               ; e8 e5 ae
    or AL, strict byte 040h                   ; 0c 40
    movzx dx, al                              ; 0f b6 d0
    mov ax, strict word 0000bh                ; b8 0b 00
    call 017c2h                               ; e8 f7 ae
    jmp near 06845h                           ; e9 77 ff
    cmp ax, strict word 00001h                ; 3d 01 00
    jne short 068ech                          ; 75 19
    xor bx, bx                                ; 31 db
    mov dx, 000a0h                            ; ba a0 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 79 ae
    and byte [bp+018h], 0feh                  ; 80 66 18 fe
    mov ax, strict word 0000bh                ; b8 0b 00
    call 017a5h                               ; e8 bd ae
    and AL, strict byte 0bfh                  ; 24 bf
    jmp short 068c2h                          ; eb d6
    mov word [bp+018h], bx                    ; 89 5e 18
    mov ax, dx                                ; 89 d0
    xor ah, dh                                ; 30 f4
    xor dl, dl                                ; 30 d2
    dec ax                                    ; 48
    or dx, ax                                 ; 09 c2
    mov word [bp+012h], dx                    ; 89 56 12
    jmp near 06845h                           ; e9 47 ff
    mov ax, strict word 00031h                ; b8 31 00
    call 017a5h                               ; e8 a1 ae
    movzx dx, al                              ; 0f b6 d0
    sal dx, 008h                              ; c1 e2 08
    mov ax, strict word 00030h                ; b8 30 00
    call 017a5h                               ; e8 95 ae
    xor ah, ah                                ; 30 e4
    or dx, ax                                 ; 09 c2
    mov word [bp+012h], dx                    ; 89 56 12
    cmp dx, strict byte 0ffc0h                ; 83 fa c0
    jbe short 06921h                          ; 76 05
    mov word [bp+012h], strict word 0ffc0h    ; c7 46 12 c0 ff
    and byte [bp+018h], 0feh                  ; 80 66 18 fe
    jmp near 06845h                           ; e9 1d ff
    cli                                       ; fa
    mov ax, strict word 00001h                ; b8 01 00
    call 066b2h                               ; e8 83 fd
    mov dx, word [bp+006h]                    ; 8b 56 06
    add dx, strict byte 00038h                ; 83 c2 38
    mov ax, word [bp+014h]                    ; 8b 46 14
    mov bx, strict word 0ffffh                ; bb ff ff
    call 01773h                               ; e8 35 ae
    mov dx, word [bp+006h]                    ; 8b 56 06
    add dx, strict byte 0003ah                ; 83 c2 3a
    mov ax, word [bp+014h]                    ; 8b 46 14
    xor bx, bx                                ; 31 db
    call 01773h                               ; e8 27 ae
    mov dx, word [bp+006h]                    ; 8b 56 06
    add dx, strict byte 0003ch                ; 83 c2 3c
    mov ax, word [bp+014h]                    ; 8b 46 14
    mov bx, strict word 0000fh                ; bb 0f 00
    call 01757h                               ; e8 fc ad
    mov dx, word [bp+006h]                    ; 8b 56 06
    add dx, strict byte 0003dh                ; 83 c2 3d
    mov ax, word [bp+014h]                    ; 8b 46 14
    mov bx, 0009bh                            ; bb 9b 00
    call 01757h                               ; e8 ed ad
    mov dx, word [bp+006h]                    ; 8b 56 06
    add dx, strict byte 0003eh                ; 83 c2 3e
    mov ax, word [bp+014h]                    ; 8b 46 14
    xor bx, bx                                ; 31 db
    call 01773h                               ; e8 fb ad
    mov AL, strict byte 011h                  ; b0 11
    mov dx, strict word 00020h                ; ba 20 00
    out DX, AL                                ; ee
    mov dx, 000a0h                            ; ba a0 00
    out DX, AL                                ; ee
    mov ax, word [bp+00ch]                    ; 8b 46 0c
    shr ax, 008h                              ; c1 e8 08
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
    call 069b9h                               ; e8 00 00
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
    jmp near 06845h                           ; e9 50 fe
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 2a b0
    push 00880h                               ; 68 80 08
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 63 b0
    add sp, strict byte 00004h                ; 83 c4 04
    or byte [bp+018h], 001h                   ; 80 4e 18 01
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    or ah, 086h                               ; 80 cc 86
    mov word [bp+012h], ax                    ; 89 46 12
    jmp near 06845h                           ; e9 28 fe
    mov word [bp+018h], cx                    ; 89 4e 18
    mov word [bp+012h], ax                    ; 89 46 12
    mov word [bp+00ch], 0e6f5h                ; c7 46 0c f5 e6
    mov word [bp+014h], 0f000h                ; c7 46 14 00 f0
    jmp near 06845h                           ; e9 15 fe
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 2c ad
    mov word [bp+014h], ax                    ; 89 46 14
    jmp near 06921h                           ; e9 e2 fe
    push 008afh                               ; 68 af 08
    push strict byte 00008h                   ; 6a 08
    jmp short 06a05h                          ; eb bf
    test byte [bp+012h], 0ffh                 ; f6 46 12 ff
    jne short 06a6bh                          ; 75 1f
    mov word [bp+012h], ax                    ; 89 46 12
    mov ax, word [bp+00ch]                    ; 8b 46 0c
    xor ah, ah                                ; 30 e4
    cmp ax, strict word 00001h                ; 3d 01 00
    jc short 06a64h                           ; 72 0b
    cmp ax, strict word 00003h                ; 3d 03 00
    jnbe short 06a64h                         ; 77 06
    mov word [bp+018h], cx                    ; 89 4e 18
    jmp near 06845h                           ; e9 e1 fd
    or byte [bp+018h], 001h                   ; 80 4e 18 01
    jmp near 06845h                           ; e9 da fd
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 b4 af
    push word [bp+00ch]                       ; ff 76 0c
    push word [bp+012h]                       ; ff 76 12
    push 008c6h                               ; 68 c6 08
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 e7 af
    add sp, strict byte 00008h                ; 83 c4 08
    jmp short 06a0bh                          ; eb 82
    push SS                                   ; 16
    insb                                      ; 6c
    cmp byte [si+05bh], ch                    ; 38 6c 5b
    insb                                      ; 6c
    jnl short 06afdh                          ; 7d 6c
    popfw                                     ; 9d
    insb                                      ; 6c
    mov sp, 0e06ch                            ; bc 6c e0
    insb                                      ; 6c
    add AL, strict byte 06dh                  ; 04 6d
    inc bx                                    ; 43
    insw                                      ; 6d
    outsw                                     ; 6f
    insw                                      ; 6d
_int15_function32:                           ; 0xf6a9d LB 0x396
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    sub sp, strict byte 00008h                ; 83 ec 08
    mov ax, word [bp+020h]                    ; 8b 46 20
    shr ax, 008h                              ; c1 e8 08
    mov bx, word [bp+028h]                    ; 8b 5e 28
    and bl, 0feh                              ; 80 e3 fe
    mov dx, word [bp+020h]                    ; 8b 56 20
    xor dh, dh                                ; 30 f6
    cmp ax, 000e8h                            ; 3d e8 00
    je near 06b3ah                            ; 0f 84 7e 00
    cmp ax, 000d0h                            ; 3d d0 00
    je short 06ad5h                           ; 74 14
    cmp ax, 00086h                            ; 3d 86 00
    jne near 06e03h                           ; 0f 85 3b 03
    sti                                       ; fb
    mov dx, word [bp+01ch]                    ; 8b 56 1c
    mov ax, word [bp+018h]                    ; 8b 46 18
    call 0666fh                               ; e8 9d fb
    jmp near 06e2dh                           ; e9 58 03
    cmp dx, strict byte 0004fh                ; 83 fa 4f
    jne near 06e03h                           ; 0f 85 27 03
    cmp word [bp+016h], 05052h                ; 81 7e 16 52 50
    jne near 06e03h                           ; 0f 85 1e 03
    cmp word [bp+014h], 04f43h                ; 81 7e 14 43 4f
    jne near 06e03h                           ; 0f 85 15 03
    cmp word [bp+01eh], 04d4fh                ; 81 7e 1e 4f 4d
    jne near 06e03h                           ; 0f 85 0c 03
    cmp word [bp+01ch], 04445h                ; 81 7e 1c 45 44
    jne near 06e03h                           ; 0f 85 03 03
    mov ax, word [bp+00ah]                    ; 8b 46 0a
    or ax, word [bp+008h]                     ; 0b 46 08
    jne near 06e03h                           ; 0f 85 f9 02
    mov ax, word [bp+006h]                    ; 8b 46 06
    or ax, word [bp+004h]                     ; 0b 46 04
    jne near 06e03h                           ; 0f 85 ef 02
    mov word [bp+028h], bx                    ; 89 5e 28
    mov ax, word [bp+014h]                    ; 8b 46 14
    mov word [bp+008h], ax                    ; 89 46 08
    mov ax, word [bp+016h]                    ; 8b 46 16
    mov word [bp+00ah], ax                    ; 89 46 0a
    mov ax, word [bp+01ch]                    ; 8b 46 1c
    mov word [bp+004h], ax                    ; 89 46 04
    mov ax, word [bp+01eh]                    ; 8b 46 1e
    mov word [bp+006h], ax                    ; 89 46 06
    mov dword [bp+020h], strict dword 049413332h ; 66 c7 46 20 32 33 41 49
    jmp near 06e2dh                           ; e9 f3 02
    cmp dx, strict byte 00020h                ; 83 fa 20
    je short 06b49h                           ; 74 0a
    cmp dx, strict byte 00001h                ; 83 fa 01
    je near 06db6h                            ; 0f 84 70 02
    jmp near 06e03h                           ; e9 ba 02
    cmp word [bp+01ah], 0534dh                ; 81 7e 1a 4d 53
    jne near 06e03h                           ; 0f 85 b1 02
    cmp word [bp+018h], 04150h                ; 81 7e 18 50 41
    jne near 06e03h                           ; 0f 85 a8 02
    mov ax, strict word 00035h                ; b8 35 00
    call 017a5h                               ; e8 44 ac
    movzx bx, al                              ; 0f b6 d8
    xor dx, dx                                ; 31 d2
    mov cx, strict word 00008h                ; b9 08 00
    sal bx, 1                                 ; d1 e3
    rcl dx, 1                                 ; d1 d2
    loop 06b69h                               ; e2 fa
    mov ax, strict word 00034h                ; b8 34 00
    call 017a5h                               ; e8 30 ac
    xor ah, ah                                ; 30 e4
    mov dx, bx                                ; 89 da
    or dx, ax                                 ; 09 c2
    xor bx, bx                                ; 31 db
    add bx, bx                                ; 01 db
    adc dx, 00100h                            ; 81 d2 00 01
    cmp dx, 00100h                            ; 81 fa 00 01
    jc short 06b8fh                           ; 72 06
    jne short 06bbch                          ; 75 31
    test bx, bx                               ; 85 db
    jnbe short 06bbch                         ; 77 2d
    mov ax, strict word 00031h                ; b8 31 00
    call 017a5h                               ; e8 10 ac
    movzx bx, al                              ; 0f b6 d8
    xor dx, dx                                ; 31 d2
    mov cx, strict word 00008h                ; b9 08 00
    sal bx, 1                                 ; d1 e3
    rcl dx, 1                                 ; d1 d2
    loop 06b9dh                               ; e2 fa
    mov ax, strict word 00030h                ; b8 30 00
    call 017a5h                               ; e8 fc ab
    xor ah, ah                                ; 30 e4
    or bx, ax                                 ; 09 c3
    mov cx, strict word 0000ah                ; b9 0a 00
    sal bx, 1                                 ; d1 e3
    rcl dx, 1                                 ; d1 d2
    loop 06bb0h                               ; e2 fa
    add bx, strict byte 00000h                ; 83 c3 00
    adc dx, strict byte 00010h                ; 83 d2 10
    mov ax, strict word 00062h                ; b8 62 00
    call 017a5h                               ; e8 e3 ab
    xor ah, ah                                ; 30 e4
    mov word [bp-00ah], ax                    ; 89 46 f6
    xor al, al                                ; 30 c0
    mov word [bp-008h], ax                    ; 89 46 f8
    mov cx, strict word 00008h                ; b9 08 00
    sal word [bp-00ah], 1                     ; d1 66 f6
    rcl word [bp-008h], 1                     ; d1 56 f8
    loop 06bcfh                               ; e2 f8
    mov ax, strict word 00061h                ; b8 61 00
    call 017a5h                               ; e8 c8 ab
    xor ah, ah                                ; 30 e4
    or word [bp-00ah], ax                     ; 09 46 f6
    mov ax, word [bp-00ah]                    ; 8b 46 f6
    mov word [bp-008h], ax                    ; 89 46 f8
    mov word [bp-00ah], strict word 00000h    ; c7 46 f6 00 00
    mov ax, strict word 00063h                ; b8 63 00
    call 017a5h                               ; e8 b2 ab
    mov byte [bp-004h], al                    ; 88 46 fc
    mov byte [bp-006h], al                    ; 88 46 fa
    mov ax, word [bp+014h]                    ; 8b 46 14
    cmp ax, strict word 00009h                ; 3d 09 00
    jnbe near 06e03h                          ; 0f 87 00 02
    mov si, ax                                ; 89 c6
    add si, ax                                ; 01 c6
    mov ax, bx                                ; 89 d8
    add ax, strict word 00000h                ; 05 00 00
    mov cx, dx                                ; 89 d1
    adc cx, strict byte 0ffffh                ; 83 d1 ff
    jmp word [cs:si+06a89h]                   ; 2e ff a4 89 6a
    push strict byte 00001h                   ; 6a 01
    push dword 000000000h                     ; 66 6a 00
    push strict byte 00009h                   ; 6a 09
    push 0fc00h                               ; 68 00 fc
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+024h]                    ; 8b 46 24
    xor bx, bx                                ; 31 db
    xor cx, cx                                ; 31 c9
    call 066deh                               ; e8 b1 fa
    mov dword [bp+014h], strict dword 000000001h ; 66 c7 46 14 01 00 00 00
    jmp near 06da0h                           ; e9 68 01
    push strict byte 00002h                   ; 6a 02
    push dword 000000000h                     ; 66 6a 00
    push strict byte 0000ah                   ; 6a 0a
    push strict byte 00000h                   ; 6a 00
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+024h]                    ; 8b 46 24
    mov bx, 0fc00h                            ; bb 00 fc
    mov cx, strict word 00009h                ; b9 09 00
    call 066deh                               ; e8 8e fa
    mov dword [bp+014h], strict dword 000000002h ; 66 c7 46 14 02 00 00 00
    jmp near 06da0h                           ; e9 45 01
    push strict byte 00002h                   ; 6a 02
    push dword 000000000h                     ; 66 6a 00
    push strict byte 00010h                   ; 6a 10
    push strict byte 00000h                   ; 6a 00
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+024h]                    ; 8b 46 24
    xor bx, bx                                ; 31 db
    mov cx, strict word 0000fh                ; b9 0f 00
    call 066deh                               ; e8 6c fa
    mov dword [bp+014h], strict dword 000000003h ; 66 c7 46 14 03 00 00 00
    jmp near 06da0h                           ; e9 23 01
    push strict byte 00001h                   ; 6a 01
    push dword 000000000h                     ; 66 6a 00
    push cx                                   ; 51
    push ax                                   ; 50
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+024h]                    ; 8b 46 24
    xor bx, bx                                ; 31 db
    mov cx, strict word 00010h                ; b9 10 00
    call 066deh                               ; e8 4c fa
    mov dword [bp+014h], strict dword 000000004h ; 66 c7 46 14 04 00 00 00
    jmp near 06da0h                           ; e9 03 01
    push strict byte 00003h                   ; 6a 03
    push dword 000000000h                     ; 66 6a 00
    push dx                                   ; 52
    push bx                                   ; 53
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov si, word [bp+024h]                    ; 8b 76 24
    mov bx, ax                                ; 89 c3
    mov ax, si                                ; 89 f0
    call 066deh                               ; e8 2d fa
    mov dword [bp+014h], strict dword 000000005h ; 66 c7 46 14 05 00 00 00
    jmp near 06da0h                           ; e9 e4 00
    push strict byte 00002h                   ; 6a 02
    push dword 000000000h                     ; 66 6a 00
    push 0fec0h                               ; 68 c0 fe
    push 01000h                               ; 68 00 10
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+024h]                    ; 8b 46 24
    xor bx, bx                                ; 31 db
    mov cx, 0fec0h                            ; b9 c0 fe
    call 066deh                               ; e8 09 fa
    mov dword [bp+014h], strict dword 000000006h ; 66 c7 46 14 06 00 00 00
    jmp near 06da0h                           ; e9 c0 00
    push strict byte 00002h                   ; 6a 02
    push dword 000000000h                     ; 66 6a 00
    push 0fee0h                               ; 68 e0 fe
    push 01000h                               ; 68 00 10
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+024h]                    ; 8b 46 24
    xor bx, bx                                ; 31 db
    mov cx, 0fee0h                            ; b9 e0 fe
    call 066deh                               ; e8 e5 f9
    mov dword [bp+014h], strict dword 000000007h ; 66 c7 46 14 07 00 00 00
    jmp near 06da0h                           ; e9 9c 00
    push strict byte 00002h                   ; 6a 02
    push dword 000000000h                     ; 66 6a 00
    push dword 000000000h                     ; 66 6a 00
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+024h]                    ; 8b 46 24
    xor bx, bx                                ; 31 db
    mov cx, strict word 0fffch                ; b9 fc ff
    call 066deh                               ; e8 c4 f9
    cmp byte [bp-004h], 000h                  ; 80 7e fc 00
    jne short 06d27h                          ; 75 07
    mov ax, word [bp-008h]                    ; 8b 46 f8
    test ax, ax                               ; 85 c0
    je short 06d3bh                           ; 74 14
    mov dword [bp+014h], strict dword 000000009h ; 66 c7 46 14 09 00 00 00
    jmp short 06da0h                          ; eb 6f
    mov dword [bp+014h], strict dword 000000008h ; 66 c7 46 14 08 00 00 00
    jmp short 06da0h                          ; eb 65
    mov word [bp+014h], ax                    ; 89 46 14
    mov word [bp+016h], ax                    ; 89 46 16
    jmp short 06da0h                          ; eb 5d
    push strict byte 00002h                   ; 6a 02
    push dword 000000000h                     ; 66 6a 00
    push dword 000000000h                     ; 66 6a 00
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+024h]                    ; 8b 46 24
    xor bx, bx                                ; 31 db
    xor cx, cx                                ; 31 c9
    call 066deh                               ; e8 86 f9
    cmp byte [bp-004h], 000h                  ; 80 7e fc 00
    jne short 06d65h                          ; 75 07
    mov ax, word [bp-008h]                    ; 8b 46 f8
    test ax, ax                               ; 85 c0
    je short 06d67h                           ; 74 02
    jmp short 06d27h                          ; eb c0
    mov word [bp+014h], ax                    ; 89 46 14
    mov word [bp+016h], ax                    ; 89 46 16
    jmp short 06da0h                          ; eb 31
    cmp byte [bp-004h], 000h                  ; 80 7e fc 00
    jne short 06d7bh                          ; 75 06
    cmp word [bp-008h], strict byte 00000h    ; 83 7e f8 00
    je short 06da0h                           ; 74 25
    push strict byte 00001h                   ; 6a 01
    mov al, byte [bp-006h]                    ; 8a 46 fa
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    xor ah, ah                                ; 30 e4
    push ax                                   ; 50
    push strict byte 00001h                   ; 6a 01
    push dword [bp-00ah]                      ; 66 ff 76 f6
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+024h]                    ; 8b 46 24
    xor bx, bx                                ; 31 db
    xor cx, cx                                ; 31 c9
    call 066deh                               ; e8 46 f9
    xor ax, ax                                ; 31 c0
    mov word [bp+014h], ax                    ; 89 46 14
    mov word [bp+016h], ax                    ; 89 46 16
    mov dword [bp+020h], strict dword 0534d4150h ; 66 c7 46 20 50 41 4d 53
    mov dword [bp+01ch], strict dword 000000014h ; 66 c7 46 1c 14 00 00 00
    and byte [bp+028h], 0feh                  ; 80 66 28 fe
    jmp short 06e2dh                          ; eb 77
    mov word [bp+028h], bx                    ; 89 5e 28
    mov ax, strict word 00031h                ; b8 31 00
    call 017a5h                               ; e8 e6 a9
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    sal dx, 008h                              ; c1 e2 08
    mov ax, strict word 00030h                ; b8 30 00
    call 017a5h                               ; e8 d9 a9
    xor ah, ah                                ; 30 e4
    or dx, ax                                 ; 09 c2
    mov word [bp+01ch], dx                    ; 89 56 1c
    cmp dx, 03c00h                            ; 81 fa 00 3c
    jbe short 06ddeh                          ; 76 05
    mov word [bp+01ch], 03c00h                ; c7 46 1c 00 3c
    mov ax, strict word 00035h                ; b8 35 00
    call 017a5h                               ; e8 c1 a9
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    sal dx, 008h                              ; c1 e2 08
    mov ax, strict word 00034h                ; b8 34 00
    call 017a5h                               ; e8 b4 a9
    xor ah, ah                                ; 30 e4
    or dx, ax                                 ; 09 c2
    mov word [bp+018h], dx                    ; 89 56 18
    mov ax, word [bp+01ch]                    ; 8b 46 1c
    mov word [bp+020h], ax                    ; 89 46 20
    mov word [bp+014h], dx                    ; 89 56 14
    jmp short 06e2dh                          ; eb 2a
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 1c ac
    push word [bp+014h]                       ; ff 76 14
    push word [bp+020h]                       ; ff 76 20
    push 008c6h                               ; 68 c6 08
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 4f ac
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
_int15_blkmove:                              ; 0xf6e33 LB 0x1a9
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    push ax                                   ; 50
    cli                                       ; fa
    mov ax, strict word 00001h                ; b8 01 00
    call 066b2h                               ; e8 71 f8
    mov ax, word [bp+006h]                    ; 8b 46 06
    sal ax, 004h                              ; c1 e0 04
    mov cx, word [bp+00ah]                    ; 8b 4e 0a
    add cx, ax                                ; 01 c1
    mov dx, word [bp+006h]                    ; 8b 56 06
    shr dx, 00ch                              ; c1 ea 0c
    mov byte [bp-006h], dl                    ; 88 56 fa
    cmp cx, ax                                ; 39 c1
    jnc short 06e5eh                          ; 73 05
    db  0feh, 0c2h
    ; inc dl                                    ; fe c2
    mov byte [bp-006h], dl                    ; 88 56 fa
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00008h                ; 83 c2 08
    mov ax, word [bp+006h]                    ; 8b 46 06
    mov bx, strict word 0002fh                ; bb 2f 00
    call 01773h                               ; e8 06 a9
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 0000ah                ; 83 c2 0a
    mov ax, word [bp+006h]                    ; 8b 46 06
    mov bx, cx                                ; 89 cb
    call 01773h                               ; e8 f8 a8
    movzx bx, byte [bp-006h]                  ; 0f b6 5e fa
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 0000ch                ; 83 c2 0c
    mov ax, word [bp+006h]                    ; 8b 46 06
    call 01757h                               ; e8 cc a8
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 0000dh                ; 83 c2 0d
    mov ax, word [bp+006h]                    ; 8b 46 06
    mov bx, 00093h                            ; bb 93 00
    call 01757h                               ; e8 bd a8
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 0000eh                ; 83 c2 0e
    mov ax, word [bp+006h]                    ; 8b 46 06
    xor bx, bx                                ; 31 db
    call 01773h                               ; e8 cb a8
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00020h                ; 83 c2 20
    mov ax, word [bp+006h]                    ; 8b 46 06
    mov bx, strict word 0ffffh                ; bb ff ff
    call 01773h                               ; e8 bc a8
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00022h                ; 83 c2 22
    mov ax, word [bp+006h]                    ; 8b 46 06
    xor bx, bx                                ; 31 db
    call 01773h                               ; e8 ae a8
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00024h                ; 83 c2 24
    mov ax, word [bp+006h]                    ; 8b 46 06
    mov bx, strict word 0000fh                ; bb 0f 00
    call 01757h                               ; e8 83 a8
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00025h                ; 83 c2 25
    mov ax, word [bp+006h]                    ; 8b 46 06
    mov bx, 0009bh                            ; bb 9b 00
    call 01757h                               ; e8 74 a8
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00026h                ; 83 c2 26
    mov ax, word [bp+006h]                    ; 8b 46 06
    xor bx, bx                                ; 31 db
    call 01773h                               ; e8 82 a8
    mov ax, ss                                ; 8c d0
    mov cx, ax                                ; 89 c1
    sal cx, 004h                              ; c1 e1 04
    shr ax, 00ch                              ; c1 e8 0c
    mov word [bp-008h], ax                    ; 89 46 f8
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 00028h                ; 83 c2 28
    mov ax, word [bp+006h]                    ; 8b 46 06
    mov bx, strict word 0ffffh                ; bb ff ff
    call 01773h                               ; e8 66 a8
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 0002ah                ; 83 c2 2a
    mov ax, word [bp+006h]                    ; 8b 46 06
    mov bx, cx                                ; 89 cb
    call 01773h                               ; e8 58 a8
    movzx bx, byte [bp-008h]                  ; 0f b6 5e f8
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 0002ch                ; 83 c2 2c
    mov ax, word [bp+006h]                    ; 8b 46 06
    call 01757h                               ; e8 2c a8
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 0002dh                ; 83 c2 2d
    mov ax, word [bp+006h]                    ; 8b 46 06
    mov bx, 00093h                            ; bb 93 00
    call 01757h                               ; e8 1d a8
    mov dx, word [bp+00ah]                    ; 8b 56 0a
    add dx, strict byte 0002eh                ; 83 c2 2e
    mov ax, word [bp+006h]                    ; 8b 46 06
    xor bx, bx                                ; 31 db
    call 01773h                               ; e8 2b a8
    lea ax, [bp+004h]                         ; 8d 46 04
    mov si, word [bp+00ah]                    ; 8b 76 0a
    mov es, [bp+006h]                         ; 8e 46 06
    mov cx, word [bp+014h]                    ; 8b 4e 14
    push DS                                   ; 1e
    push eax                                  ; 66 50
    db  066h, 033h, 0c0h
    ; xor eax, eax                              ; 66 33 c0
    mov ds, ax                                ; 8e d8
    mov word [00467h], sp                     ; 89 26 67 04
    mov [00469h], ss                          ; 8c 16 69 04
    call 06f67h                               ; e8 00 00
    pop ax                                    ; 58
    add ax, strict byte 0001bh                ; 83 c0 1b
    push strict byte 00020h                   ; 6a 20
    push ax                                   ; 50
    lgdt [es:si+008h]                         ; 26 0f 01 54 08
    lidt [cs:0efe1h]                          ; 2e 0f 01 1e e1 ef
    mov eax, cr0                              ; 0f 20 c0
    or AL, strict byte 001h                   ; 0c 01
    mov cr0, eax                              ; 0f 22 c0
    retf                                      ; cb
    mov ax, strict word 00028h                ; b8 28 00
    mov ss, ax                                ; 8e d0
    mov ax, strict word 00010h                ; b8 10 00
    mov ds, ax                                ; 8e d8
    mov ax, strict word 00018h                ; b8 18 00
    mov es, ax                                ; 8e c0
    db  033h, 0f6h
    ; xor si, si                                ; 33 f6
    db  033h, 0ffh
    ; xor di, di                                ; 33 ff
    cld                                       ; fc
    rep movsw                                 ; f3 a5
    call 06f9bh                               ; e8 00 00
    pop ax                                    ; 58
    push 0f000h                               ; 68 00 f0
    add ax, strict byte 00018h                ; 83 c0 18
    push ax                                   ; 50
    mov ax, strict word 00028h                ; b8 28 00
    mov ds, ax                                ; 8e d8
    mov es, ax                                ; 8e c0
    mov eax, cr0                              ; 0f 20 c0
    and AL, strict byte 0feh                  ; 24 fe
    mov cr0, eax                              ; 0f 22 c0
    retf                                      ; cb
    lidt [cs:0efe7h]                          ; 2e 0f 01 1e e7 ef
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    mov ds, ax                                ; 8e d8
    mov es, ax                                ; 8e c0
    lss sp, [00467h]                          ; 0f b2 26 67 04
    pop eax                                   ; 66 58
    pop DS                                    ; 1f
    xor ax, ax                                ; 31 c0
    call 066b2h                               ; e8 e6 f6
    sti                                       ; fb
    mov byte [bp+017h], 000h                  ; c6 46 17 00
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
_inv_op_handler:                             ; 0xf6fdc LB 0x195
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    push ax                                   ; 50
    les bx, [bp+018h]                         ; c4 5e 18
    cmp byte [es:bx], 0f0h                    ; 26 80 3f f0
    jne short 06ff2h                          ; 75 06
    inc word [bp+018h]                        ; ff 46 18
    jmp near 0716ah                           ; e9 78 01
    cmp word [es:bx], 0050fh                  ; 26 81 3f 0f 05
    jne near 07166h                           ; 0f 85 6b 01
    mov si, 00800h                            ; be 00 08
    xor ax, ax                                ; 31 c0
    mov word [bp-006h], ax                    ; 89 46 fa
    mov word [bp-008h], ax                    ; 89 46 f8
    mov es, ax                                ; 8e c0
    mov bx, word [es:si+02ch]                 ; 26 8b 5c 2c
    sub bx, strict byte 00006h                ; 83 eb 06
    mov dx, word [es:si+020h]                 ; 26 8b 54 20
    mov ax, word [es:si+01ah]                 ; 26 8b 44 1a
    mov es, dx                                ; 8e c2
    mov word [es:bx], ax                      ; 26 89 07
    mov es, [bp-006h]                         ; 8e 46 fa
    mov ax, word [es:si+022h]                 ; 26 8b 44 22
    mov es, dx                                ; 8e c2
    mov word [es:bx+002h], ax                 ; 26 89 47 02
    mov es, [bp-006h]                         ; 8e 46 fa
    mov ax, word [es:si+018h]                 ; 26 8b 44 18
    mov es, dx                                ; 8e c2
    mov word [es:bx+004h], ax                 ; 26 89 47 04
    mov es, [bp-006h]                         ; 8e 46 fa
    movzx bx, byte [es:si+038h]               ; 26 0f b6 5c 38
    mov di, word [es:si+036h]                 ; 26 8b 7c 36
    mov ax, word [es:si+024h]                 ; 26 8b 44 24
    xor dx, dx                                ; 31 d2
    mov cx, strict word 00004h                ; b9 04 00
    sal ax, 1                                 ; d1 e0
    rcl dx, 1                                 ; d1 d2
    loop 0704bh                               ; e2 fa
    cmp bx, dx                                ; 39 d3
    jne short 07059h                          ; 75 04
    cmp di, ax                                ; 39 c7
    je short 0705eh                           ; 74 05
    mov word [bp-008h], strict word 00001h    ; c7 46 f8 01 00
    mov es, [bp-006h]                         ; 8e 46 fa
    movzx di, byte [es:si+04ah]               ; 26 0f b6 7c 4a
    mov bx, word [es:si+048h]                 ; 26 8b 5c 48
    mov ax, word [es:si+01eh]                 ; 26 8b 44 1e
    xor dx, dx                                ; 31 d2
    mov cx, strict word 00004h                ; b9 04 00
    sal ax, 1                                 ; d1 e0
    rcl dx, 1                                 ; d1 d2
    loop 07073h                               ; e2 fa
    cmp di, dx                                ; 39 d7
    jne short 07081h                          ; 75 04
    cmp bx, ax                                ; 39 c3
    je short 07085h                           ; 74 04
    or byte [bp-008h], 002h                   ; 80 4e f8 02
    push strict byte 00000h                   ; 6a 00
    push 00800h                               ; 68 00 08
    push strict byte 0001fh                   ; 6a 1f
    db  08bh, 0dch
    ; mov bx, sp                                ; 8b dc
    lgdt [ss:bx]                              ; 36 0f 01 17
    add sp, strict byte 00006h                ; 83 c4 06
    mov es, [bp-006h]                         ; 8e 46 fa
    mov ax, word [es:si+03ah]                 ; 26 8b 44 3a
    mov word [es:si+008h], ax                 ; 26 89 44 08
    mov ax, word [es:si+036h]                 ; 26 8b 44 36
    mov word [es:si+00ah], ax                 ; 26 89 44 0a
    movzx dx, byte [es:si+039h]               ; 26 0f b6 54 39
    sal dx, 008h                              ; c1 e2 08
    movzx ax, byte [es:si+038h]               ; 26 0f b6 44 38
    or dx, ax                                 ; 09 c2
    mov word [es:si+00ch], dx                 ; 26 89 54 0c
    mov word [es:si+00eh], strict word 00000h ; 26 c7 44 0e 00 00
    mov ax, word [es:si+04ch]                 ; 26 8b 44 4c
    mov word [es:si], ax                      ; 26 89 04
    mov ax, word [es:si+048h]                 ; 26 8b 44 48
    mov word [es:si+002h], ax                 ; 26 89 44 02
    movzx dx, byte [es:si+04bh]               ; 26 0f b6 54 4b
    sal dx, 008h                              ; c1 e2 08
    movzx ax, byte [es:si+04ah]               ; 26 0f b6 44 4a
    or dx, ax                                 ; 09 c2
    mov word [es:si+004h], dx                 ; 26 89 54 04
    movzx ax, byte [es:si+05ch]               ; 26 0f b6 44 5c
    mov dx, word [es:si+05ah]                 ; 26 8b 54 5a
    push ax                                   ; 50
    push dx                                   ; 52
    push word [es:si+05eh]                    ; 26 ff 74 5e
    db  08bh, 0dch
    ; mov bx, sp                                ; 8b dc
    lidt [ss:bx]                              ; 36 0f 01 1f
    add sp, strict byte 00006h                ; 83 c4 06
    mov cx, word [bp-008h]                    ; 8b 4e f8
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
    je near 07123h                            ; 0f 84 02 00
    mov es, ax                                ; 8e c0
    test cx, strict word 00002h               ; f7 c1 02 00
    je near 0714bh                            ; 0f 84 20 00
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
    jmp short 0716ah                          ; eb 04
    sti                                       ; fb
    hlt                                       ; f4
    jmp short 07167h                          ; eb fd
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
init_rtc_:                                   ; 0xf7171 LB 0x28
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push dx                                   ; 52
    mov dx, strict word 00026h                ; ba 26 00
    mov ax, strict word 0000ah                ; b8 0a 00
    call 017c2h                               ; e8 44 a6
    mov dx, strict word 00002h                ; ba 02 00
    mov ax, strict word 0000bh                ; b8 0b 00
    call 017c2h                               ; e8 3b a6
    mov ax, strict word 0000ch                ; b8 0c 00
    call 017a5h                               ; e8 18 a6
    mov ax, strict word 0000dh                ; b8 0d 00
    call 017a5h                               ; e8 12 a6
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop dx                                    ; 5a
    pop bp                                    ; 5d
    retn                                      ; c3
rtc_updating_:                               ; 0xf7199 LB 0x21
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push dx                                   ; 52
    mov dx, 061a8h                            ; ba a8 61
    dec dx                                    ; 4a
    je short 071b1h                           ; 74 0e
    mov ax, strict word 0000ah                ; b8 0a 00
    call 017a5h                               ; e8 fc a5
    test AL, strict byte 080h                 ; a8 80
    jne short 071a0h                          ; 75 f3
    xor ax, ax                                ; 31 c0
    jmp short 071b4h                          ; eb 03
    mov ax, strict word 00001h                ; b8 01 00
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop dx                                    ; 5a
    pop bp                                    ; 5d
    retn                                      ; c3
_int70_function:                             ; 0xf71ba LB 0xbe
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push ax                                   ; 50
    mov ax, strict word 0000bh                ; b8 0b 00
    call 017a5h                               ; e8 e0 a5
    mov dl, al                                ; 88 c2
    mov byte [bp-004h], al                    ; 88 46 fc
    mov ax, strict word 0000ch                ; b8 0c 00
    call 017a5h                               ; e8 d5 a5
    mov dh, al                                ; 88 c6
    test dl, 060h                             ; f6 c2 60
    je near 0725fh                            ; 0f 84 86 00
    test AL, strict byte 020h                 ; a8 20
    je short 071e1h                           ; 74 04
    sti                                       ; fb
    int 04ah                                  ; cd 4a
    cli                                       ; fa
    test dh, 040h                             ; f6 c6 40
    je near 0725fh                            ; 0f 84 77 00
    mov dx, 000a0h                            ; ba a0 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 58 a5
    test al, al                               ; 84 c0
    je short 0725fh                           ; 74 6a
    mov dx, 0009ch                            ; ba 9c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01781h                               ; e8 83 a5
    test dx, dx                               ; 85 d2
    jne short 0724bh                          ; 75 49
    cmp ax, 003d1h                            ; 3d d1 03
    jnc short 0724bh                          ; 73 44
    mov dx, 00098h                            ; ba 98 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 55 a5
    mov si, ax                                ; 89 c6
    mov dx, 0009ah                            ; ba 9a 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 4a a5
    mov cx, ax                                ; 89 c1
    xor bx, bx                                ; 31 db
    mov dx, 000a0h                            ; ba a0 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 2f a5
    mov al, byte [bp-004h]                    ; 8a 46 fc
    and AL, strict byte 037h                  ; 24 37
    movzx dx, al                              ; 0f b6 d0
    mov ax, strict word 0000bh                ; b8 0b 00
    call 017c2h                               ; e8 8c a5
    mov dx, cx                                ; 89 ca
    mov ax, si                                ; 89 f0
    call 01749h                               ; e8 0c a5
    or AL, strict byte 080h                   ; 0c 80
    movzx bx, al                              ; 0f b6 d8
    mov dx, cx                                ; 89 ca
    mov ax, si                                ; 89 f0
    call 01757h                               ; e8 0e a5
    jmp short 0725fh                          ; eb 14
    mov bx, ax                                ; 89 c3
    add bx, 0fc2fh                            ; 81 c3 2f fc
    mov cx, dx                                ; 89 d1
    adc cx, strict byte 0ffffh                ; 83 d1 ff
    mov dx, 0009ch                            ; ba 9c 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01793h                               ; e8 34 a5
    call 0e030h                               ; e8 ce 6d
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
    db  08fh, 072h, 0b8h
    ; pop word [bp+si-048h]                     ; 8f 72 b8
    jc short 0724ah                           ; 72 dd
    jc short 0727eh                           ; 72 0f
    jnc short 072cfh                          ; 73 5e
    jnc short 07209h                          ; 73 96
    jnc short 0724eh                          ; 73 d9
    jnc short 072a7h                          ; 73 30
    db  074h
_int1a_function:                             ; 0xf7278 LB 0x1c8
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    sti                                       ; fb
    mov al, byte [bp+013h]                    ; 8a 46 13
    cmp AL, strict byte 007h                  ; 3c 07
    jnbe near 072b4h                          ; 0f 87 2f 00
    movzx bx, al                              ; 0f b6 d8
    add bx, bx                                ; 01 db
    jmp word [cs:bx+07268h]                   ; 2e ff a7 68 72
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
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
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
    jmp short 072b4h                          ; eb d7
    call 07199h                               ; e8 b9 fe
    test ax, ax                               ; 85 c0
    je short 072e6h                           ; 74 02
    jmp short 072b4h                          ; eb ce
    xor ax, ax                                ; 31 c0
    call 017a5h                               ; e8 ba a4
    mov byte [bp+00fh], al                    ; 88 46 0f
    mov ax, strict word 00002h                ; b8 02 00
    call 017a5h                               ; e8 b1 a4
    mov byte [bp+010h], al                    ; 88 46 10
    mov ax, strict word 00004h                ; b8 04 00
    call 017a5h                               ; e8 a8 a4
    mov bl, al                                ; 88 c3
    mov byte [bp+011h], al                    ; 88 46 11
    mov ax, strict word 0000bh                ; b8 0b 00
    call 017a5h                               ; e8 9d a4
    and AL, strict byte 001h                  ; 24 01
    mov byte [bp+00eh], al                    ; 88 46 0e
    jmp short 07354h                          ; eb 45
    call 07199h                               ; e8 87 fe
    test ax, ax                               ; 85 c0
    je short 07319h                           ; 74 03
    call 07171h                               ; e8 58 fe
    movzx dx, byte [bp+00fh]                  ; 0f b6 56 0f
    xor ax, ax                                ; 31 c0
    call 017c2h                               ; e8 a0 a4
    movzx dx, byte [bp+010h]                  ; 0f b6 56 10
    mov ax, strict word 00002h                ; b8 02 00
    call 017c2h                               ; e8 96 a4
    movzx dx, byte [bp+011h]                  ; 0f b6 56 11
    mov ax, strict word 00004h                ; b8 04 00
    call 017c2h                               ; e8 8c a4
    mov ax, strict word 0000bh                ; b8 0b 00
    call 017a5h                               ; e8 69 a4
    mov bl, al                                ; 88 c3
    and bl, 060h                              ; 80 e3 60
    or bl, 002h                               ; 80 cb 02
    mov al, byte [bp+00eh]                    ; 8a 46 0e
    and AL, strict byte 001h                  ; 24 01
    or bl, al                                 ; 08 c3
    movzx dx, bl                              ; 0f b6 d3
    mov ax, strict word 0000bh                ; b8 0b 00
    call 017c2h                               ; e8 6e a4
    mov byte [bp+013h], 000h                  ; c6 46 13 00
    mov byte [bp+012h], bl                    ; 88 5e 12
    jmp near 072b4h                           ; e9 56 ff
    mov byte [bp+013h], 000h                  ; c6 46 13 00
    call 07199h                               ; e8 34 fe
    test ax, ax                               ; 85 c0
    je short 0736ch                           ; 74 03
    jmp near 072b4h                           ; e9 48 ff
    mov ax, strict word 00009h                ; b8 09 00
    call 017a5h                               ; e8 33 a4
    mov byte [bp+010h], al                    ; 88 46 10
    mov ax, strict word 00008h                ; b8 08 00
    call 017a5h                               ; e8 2a a4
    mov byte [bp+00fh], al                    ; 88 46 0f
    mov ax, strict word 00007h                ; b8 07 00
    call 017a5h                               ; e8 21 a4
    mov byte [bp+00eh], al                    ; 88 46 0e
    mov ax, strict word 00032h                ; b8 32 00
    call 017a5h                               ; e8 18 a4
    mov byte [bp+011h], al                    ; 88 46 11
    mov byte [bp+012h], al                    ; 88 46 12
    jmp near 072b4h                           ; e9 1e ff
    call 07199h                               ; e8 00 fe
    test ax, ax                               ; 85 c0
    je short 073a3h                           ; 74 06
    call 07171h                               ; e8 d1 fd
    jmp near 072b4h                           ; e9 11 ff
    movzx dx, byte [bp+010h]                  ; 0f b6 56 10
    mov ax, strict word 00009h                ; b8 09 00
    call 017c2h                               ; e8 15 a4
    movzx dx, byte [bp+00fh]                  ; 0f b6 56 0f
    mov ax, strict word 00008h                ; b8 08 00
    call 017c2h                               ; e8 0b a4
    movzx dx, byte [bp+00eh]                  ; 0f b6 56 0e
    mov ax, strict word 00007h                ; b8 07 00
    call 017c2h                               ; e8 01 a4
    movzx dx, byte [bp+011h]                  ; 0f b6 56 11
    mov ax, strict word 00032h                ; b8 32 00
    call 017c2h                               ; e8 f7 a3
    mov ax, strict word 0000bh                ; b8 0b 00
    call 017a5h                               ; e8 d4 a3
    mov bl, al                                ; 88 c3
    and bl, 07fh                              ; 80 e3 7f
    jmp near 0734bh                           ; e9 72 ff
    mov ax, strict word 0000bh                ; b8 0b 00
    call 017a5h                               ; e8 c6 a3
    mov bl, al                                ; 88 c3
    mov word [bp+012h], strict word 00000h    ; c7 46 12 00 00
    test AL, strict byte 020h                 ; a8 20
    je short 073edh                           ; 74 03
    jmp near 072b4h                           ; e9 c7 fe
    call 07199h                               ; e8 a9 fd
    test ax, ax                               ; 85 c0
    je short 073f7h                           ; 74 03
    call 07171h                               ; e8 7a fd
    movzx dx, byte [bp+00fh]                  ; 0f b6 56 0f
    mov ax, strict word 00001h                ; b8 01 00
    call 017c2h                               ; e8 c1 a3
    movzx dx, byte [bp+010h]                  ; 0f b6 56 10
    mov ax, strict word 00003h                ; b8 03 00
    call 017c2h                               ; e8 b7 a3
    movzx dx, byte [bp+011h]                  ; 0f b6 56 11
    mov ax, strict word 00005h                ; b8 05 00
    call 017c2h                               ; e8 ad a3
    mov dx, 000a1h                            ; ba a1 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 0feh                  ; 24 fe
    out DX, AL                                ; ee
    mov al, bl                                ; 88 d8
    and AL, strict byte 05fh                  ; 24 5f
    or AL, strict byte 020h                   ; 0c 20
    movzx dx, al                              ; 0f b6 d0
    mov ax, strict word 0000bh                ; b8 0b 00
    call 017c2h                               ; e8 95 a3
    jmp near 072b4h                           ; e9 84 fe
    mov ax, strict word 0000bh                ; b8 0b 00
    call 017a5h                               ; e8 6f a3
    mov bl, al                                ; 88 c3
    and AL, strict byte 057h                  ; 24 57
    movzx dx, al                              ; 0f b6 d0
    jmp near 0734eh                           ; e9 0e ff
send_to_mouse_ctrl_:                         ; 0xf7440 LB 0x34
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
    je short 0745fh                           ; 74 0e
    push 00900h                               ; 68 00 09
    push 0117ah                               ; 68 7a 11
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 0f a6
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
get_mouse_data_:                             ; 0xf7474 LB 0x5c
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
    je short 074b6h                           ; 74 27
    test cx, cx                               ; 85 c9
    je short 074b6h                           ; 74 23
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
    movzx ax, byte [bp-006h]                  ; 0f b6 46 fa
    cmp dx, ax                                ; 39 c2
    je short 0749eh                           ; 74 eb
    dec cx                                    ; 49
    jmp short 07481h                          ; eb cb
    test cx, cx                               ; 85 c9
    jne short 074beh                          ; 75 04
    mov AL, strict byte 001h                  ; b0 01
    jmp short 074c9h                          ; eb 0b
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
set_kbd_command_byte_:                       ; 0xf74d0 LB 0x32
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
    je short 074efh                           ; 74 0e
    push 0090ah                               ; 68 0a 09
    push 0117ah                               ; 68 7a 11
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 7f a5
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
_int74_function:                             ; 0xf7502 LB 0xca
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    sub sp, strict byte 00008h                ; 83 ec 08
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 54 a2
    mov cx, ax                                ; 89 c1
    mov word [bp+004h], strict word 00000h    ; c7 46 04 00 00
    mov dx, strict word 00064h                ; ba 64 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    and AL, strict byte 021h                  ; 24 21
    cmp AL, strict byte 021h                  ; 3c 21
    jne near 075b8h                           ; 0f 85 92 00
    mov dx, strict word 00060h                ; ba 60 00
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov bl, al                                ; 88 c3
    mov dx, strict word 00026h                ; ba 26 00
    mov ax, cx                                ; 89 c8
    call 01749h                               ; e8 13 a2
    mov byte [bp-006h], al                    ; 88 46 fa
    mov dx, strict word 00027h                ; ba 27 00
    mov ax, cx                                ; 89 c8
    call 01749h                               ; e8 08 a2
    mov byte [bp-008h], al                    ; 88 46 f8
    test AL, strict byte 080h                 ; a8 80
    je short 075b8h                           ; 74 70
    mov al, byte [bp-008h]                    ; 8a 46 f8
    and AL, strict byte 007h                  ; 24 07
    mov byte [bp-002h], al                    ; 88 46 fe
    mov al, byte [bp-006h]                    ; 8a 46 fa
    and AL, strict byte 007h                  ; 24 07
    mov byte [bp-004h], al                    ; 88 46 fc
    xor bh, bh                                ; 30 ff
    movzx dx, al                              ; 0f b6 d0
    add dx, strict byte 00028h                ; 83 c2 28
    mov ax, cx                                ; 89 c8
    call 01757h                               ; e8 f2 a1
    mov al, byte [bp-004h]                    ; 8a 46 fc
    cmp al, byte [bp-002h]                    ; 3a 46 fe
    jc short 075a9h                           ; 72 3c
    mov dx, strict word 00028h                ; ba 28 00
    mov ax, cx                                ; 89 c8
    call 01749h                               ; e8 d4 a1
    xor ah, ah                                ; 30 e4
    mov word [bp+00ch], ax                    ; 89 46 0c
    mov dx, strict word 00029h                ; ba 29 00
    mov ax, cx                                ; 89 c8
    call 01749h                               ; e8 c7 a1
    xor ah, ah                                ; 30 e4
    mov word [bp+00ah], ax                    ; 89 46 0a
    mov dx, strict word 0002ah                ; ba 2a 00
    mov ax, cx                                ; 89 c8
    call 01749h                               ; e8 ba a1
    xor ah, ah                                ; 30 e4
    mov word [bp+008h], ax                    ; 89 46 08
    xor al, al                                ; 30 c0
    mov word [bp+006h], ax                    ; 89 46 06
    mov byte [bp-006h], ah                    ; 88 66 fa
    test byte [bp-008h], 080h                 ; f6 46 f8 80
    je short 075ach                           ; 74 0a
    mov word [bp+004h], strict word 00001h    ; c7 46 04 01 00
    jmp short 075ach                          ; eb 03
    inc byte [bp-006h]                        ; fe 46 fa
    movzx bx, byte [bp-006h]                  ; 0f b6 5e fa
    mov dx, strict word 00026h                ; ba 26 00
    mov ax, cx                                ; 89 c8
    call 01757h                               ; e8 9f a1
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
    adc byte [bp-07ah], dh                    ; 10 76 86
    jbe short 075cah                          ; 76 09
    jnbe short 0755dh                         ; 77 9a
    jnbe short 075cdh                         ; 77 08
    js short 07623h                           ; 78 5c
    jbe short 075f9h                          ; 76 30
    js short 075c0h                           ; 78 f5
    db  078h
_int15_function_mouse:                       ; 0xf75cc LB 0x38b
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    sub sp, strict byte 00006h                ; 83 ec 06
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 89 a1
    mov cx, ax                                ; 89 c1
    cmp byte [bp+012h], 007h                  ; 80 7e 12 07
    jbe short 075efh                          ; 76 0b
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 001h                  ; c6 46 13 01
    jmp near 07951h                           ; e9 62 03
    mov ax, strict word 00065h                ; b8 65 00
    call 074d0h                               ; e8 db fe
    and word [bp+018h], strict byte 0fffeh    ; 83 66 18 fe
    mov byte [bp+013h], 000h                  ; c6 46 13 00
    mov al, byte [bp+012h]                    ; 8a 46 12
    cmp AL, strict byte 007h                  ; 3c 07
    jnbe near 07938h                          ; 0f 87 32 03
    movzx si, al                              ; 0f b6 f0
    add si, si                                ; 01 f6
    jmp word [cs:si+075bch]                   ; 2e ff a4 bc 75
    cmp byte [bp+00dh], 001h                  ; 80 7e 0d 01
    jnbe near 07943h                          ; 0f 87 2b 03
    mov dx, strict word 00027h                ; ba 27 00
    mov ax, cx                                ; 89 c8
    call 01749h                               ; e8 29 a1
    test AL, strict byte 080h                 ; a8 80
    jne short 0762fh                          ; 75 0b
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 005h                  ; c6 46 13 05
    jmp near 0794bh                           ; e9 1c 03
    cmp byte [bp+00dh], 000h                  ; 80 7e 0d 00
    db  00fh, 094h, 0c0h
    ; sete al                                   ; 0f 94 c0
    add AL, strict byte 0f4h                  ; 04 f4
    xor ah, ah                                ; 30 e4
    call 07440h                               ; e8 03 fe
    test al, al                               ; 84 c0
    jne near 078d1h                           ; 0f 85 8e 02
    mov dx, ss                                ; 8c d2
    lea ax, [bp-008h]                         ; 8d 46 f8
    call 07474h                               ; e8 29 fe
    test al, al                               ; 84 c0
    je near 0794bh                            ; 0f 84 fa 02
    cmp byte [bp-008h], 0fah                  ; 80 7e f8 fa
    jne near 078d1h                           ; 0f 85 78 02
    jmp near 0794bh                           ; e9 ef 02
    mov al, byte [bp+00dh]                    ; 8a 46 0d
    cmp AL, strict byte 001h                  ; 3c 01
    jc short 07667h                           ; 72 04
    cmp AL, strict byte 008h                  ; 3c 08
    jbe short 0766ah                          ; 76 03
    jmp near 077fdh                           ; e9 93 01
    mov dx, strict word 00027h                ; ba 27 00
    mov ax, cx                                ; 89 c8
    call 01749h                               ; e8 d7 a0
    mov ah, byte [bp+00dh]                    ; 8a 66 0d
    db  0feh, 0cch
    ; dec ah                                    ; fe cc
    and AL, strict byte 0f8h                  ; 24 f8
    or al, ah                                 ; 08 e0
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00027h                ; ba 27 00
    mov ax, cx                                ; 89 c8
    call 01757h                               ; e8 d1 a0
    mov dx, strict word 00026h                ; ba 26 00
    mov ax, cx                                ; 89 c8
    call 01749h                               ; e8 bb a0
    and AL, strict byte 0f8h                  ; 24 f8
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00026h                ; ba 26 00
    mov ax, cx                                ; 89 c8
    call 01757h                               ; e8 bc a0
    mov ax, 000ffh                            ; b8 ff 00
    call 07440h                               ; e8 9f fd
    test al, al                               ; 84 c0
    jne near 078d1h                           ; 0f 85 2a 02
    mov dx, ss                                ; 8c d2
    lea ax, [bp-004h]                         ; 8d 46 fc
    call 07474h                               ; e8 c5 fd
    mov cl, al                                ; 88 c1
    cmp byte [bp-004h], 0feh                  ; 80 7e fc fe
    jne short 076c2h                          ; 75 0b
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 004h                  ; c6 46 13 04
    jmp near 0794bh                           ; e9 89 02
    cmp byte [bp-004h], 0fah                  ; 80 7e fc fa
    je short 076d8h                           ; 74 10
    movzx ax, byte [bp-004h]                  ; 0f b6 46 fc
    push ax                                   ; 50
    push 00915h                               ; 68 15 09
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 96 a3
    add sp, strict byte 00006h                ; 83 c4 06
    test cl, cl                               ; 84 c9
    jne near 078d1h                           ; 0f 85 f3 01
    mov dx, ss                                ; 8c d2
    lea ax, [bp-008h]                         ; 8d 46 f8
    call 07474h                               ; e8 8e fd
    test al, al                               ; 84 c0
    jne near 078d1h                           ; 0f 85 e5 01
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 07474h                               ; e8 80 fd
    test al, al                               ; 84 c0
    jne near 078d1h                           ; 0f 85 d7 01
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov byte [bp+00ch], al                    ; 88 46 0c
    mov al, byte [bp-006h]                    ; 8a 46 fa
    mov byte [bp+00dh], al                    ; 88 46 0d
    jmp near 0794bh                           ; e9 42 02
    mov al, byte [bp+00dh]                    ; 8a 46 0d
    cmp AL, strict byte 003h                  ; 3c 03
    jc short 07720h                           ; 72 10
    jbe short 0773eh                          ; 76 2c
    cmp AL, strict byte 006h                  ; 3c 06
    je short 07750h                           ; 74 3a
    cmp AL, strict byte 005h                  ; 3c 05
    je short 0774ah                           ; 74 30
    cmp AL, strict byte 004h                  ; 3c 04
    je short 07744h                           ; 74 26
    jmp short 07756h                          ; eb 36
    cmp AL, strict byte 002h                  ; 3c 02
    je short 07738h                           ; 74 14
    cmp AL, strict byte 001h                  ; 3c 01
    je short 07732h                           ; 74 0a
    test al, al                               ; 84 c0
    jne short 07756h                          ; 75 2a
    mov byte [bp-008h], 00ah                  ; c6 46 f8 0a
    jmp short 0775ah                          ; eb 28
    mov byte [bp-008h], 014h                  ; c6 46 f8 14
    jmp short 0775ah                          ; eb 22
    mov byte [bp-008h], 028h                  ; c6 46 f8 28
    jmp short 0775ah                          ; eb 1c
    mov byte [bp-008h], 03ch                  ; c6 46 f8 3c
    jmp short 0775ah                          ; eb 16
    mov byte [bp-008h], 050h                  ; c6 46 f8 50
    jmp short 0775ah                          ; eb 10
    mov byte [bp-008h], 064h                  ; c6 46 f8 64
    jmp short 0775ah                          ; eb 0a
    mov byte [bp-008h], 0c8h                  ; c6 46 f8 c8
    jmp short 0775ah                          ; eb 04
    mov byte [bp-008h], 000h                  ; c6 46 f8 00
    cmp byte [bp-008h], 000h                  ; 80 7e f8 00
    jbe short 0778fh                          ; 76 2f
    mov ax, 000f3h                            ; b8 f3 00
    call 07440h                               ; e8 da fc
    test al, al                               ; 84 c0
    jne short 07784h                          ; 75 1a
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 07474h                               ; e8 02 fd
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    call 07440h                               ; e8 c7 fc
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 07474h                               ; e8 f3 fc
    jmp near 0794bh                           ; e9 c7 01
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 003h                  ; c6 46 13 03
    jmp near 0794bh                           ; e9 bc 01
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 002h                  ; c6 46 13 02
    jmp near 0794bh                           ; e9 b1 01
    cmp byte [bp+00dh], 004h                  ; 80 7e 0d 04
    jnc short 077fdh                          ; 73 5d
    mov ax, 000e8h                            ; b8 e8 00
    call 07440h                               ; e8 9a fc
    test al, al                               ; 84 c0
    jne short 077f2h                          ; 75 48
    mov dx, ss                                ; 8c d2
    lea ax, [bp-008h]                         ; 8d 46 f8
    call 07474h                               ; e8 c2 fc
    cmp byte [bp-008h], 0fah                  ; 80 7e f8 fa
    je short 077c8h                           ; 74 10
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    push ax                                   ; 50
    push 00940h                               ; 68 40 09
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 a6 a2
    add sp, strict byte 00006h                ; 83 c4 06
    movzx ax, byte [bp+00dh]                  ; 0f b6 46 0d
    call 07440h                               ; e8 71 fc
    mov dx, ss                                ; 8c d2
    lea ax, [bp-008h]                         ; 8d 46 f8
    call 07474h                               ; e8 9d fc
    cmp byte [bp-008h], 0fah                  ; 80 7e f8 fa
    je near 0794bh                            ; 0f 84 6c 01
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    push ax                                   ; 50
    push 00940h                               ; 68 40 09
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 7f a2
    add sp, strict byte 00006h                ; 83 c4 06
    jmp near 0794bh                           ; e9 59 01
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 003h                  ; c6 46 13 03
    jmp near 0794bh                           ; e9 4e 01
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 002h                  ; c6 46 13 02
    jmp near 0794bh                           ; e9 43 01
    mov ax, 000f2h                            ; b8 f2 00
    call 07440h                               ; e8 32 fc
    test al, al                               ; 84 c0
    jne short 07825h                          ; 75 13
    mov dx, ss                                ; 8c d2
    lea ax, [bp-008h]                         ; 8d 46 f8
    call 07474h                               ; e8 5a fc
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 07474h                               ; e8 52 fc
    jmp near 07700h                           ; e9 db fe
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 003h                  ; c6 46 13 03
    jmp near 0794bh                           ; e9 1b 01
    mov al, byte [bp+00dh]                    ; 8a 46 0d
    test al, al                               ; 84 c0
    jbe short 0783eh                          ; 76 07
    cmp AL, strict byte 002h                  ; 3c 02
    jbe short 078a7h                          ; 76 6c
    jmp near 078dbh                           ; e9 9d 00
    mov ax, 000e9h                            ; b8 e9 00
    call 07440h                               ; e8 fc fb
    test al, al                               ; 84 c0
    jne near 078d1h                           ; 0f 85 87 00
    mov dx, ss                                ; 8c d2
    lea ax, [bp-008h]                         ; 8d 46 f8
    call 07474h                               ; e8 22 fc
    mov cl, al                                ; 88 c1
    cmp byte [bp-008h], 0fah                  ; 80 7e f8 fa
    je short 0786ah                           ; 74 10
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    push ax                                   ; 50
    push 00940h                               ; 68 40 09
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 04 a2
    add sp, strict byte 00006h                ; 83 c4 06
    test cl, cl                               ; 84 c9
    jne short 078d1h                          ; 75 63
    mov dx, ss                                ; 8c d2
    lea ax, [bp-008h]                         ; 8d 46 f8
    call 07474h                               ; e8 fe fb
    test al, al                               ; 84 c0
    jne short 078d1h                          ; 75 57
    mov dx, ss                                ; 8c d2
    lea ax, [bp-006h]                         ; 8d 46 fa
    call 07474h                               ; e8 f2 fb
    test al, al                               ; 84 c0
    jne short 078d1h                          ; 75 4b
    mov dx, ss                                ; 8c d2
    lea ax, [bp-004h]                         ; 8d 46 fc
    call 07474h                               ; e8 e6 fb
    test al, al                               ; 84 c0
    jne short 078d1h                          ; 75 3f
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov byte [bp+00ch], al                    ; 88 46 0c
    mov al, byte [bp-006h]                    ; 8a 46 fa
    mov byte [bp+010h], al                    ; 88 46 10
    mov al, byte [bp-004h]                    ; 8a 46 fc
    mov byte [bp+00eh], al                    ; 88 46 0e
    jmp near 0794bh                           ; e9 a4 00
    cmp AL, strict byte 001h                  ; 3c 01
    jne short 078b0h                          ; 75 05
    mov ax, 000e6h                            ; b8 e6 00
    jmp short 078b3h                          ; eb 03
    mov ax, 000e7h                            ; b8 e7 00
    call 07440h                               ; e8 8a fb
    mov cl, al                                ; 88 c1
    test cl, cl                               ; 84 c9
    jne short 078cbh                          ; 75 0f
    mov dx, ss                                ; 8c d2
    lea ax, [bp-008h]                         ; 8d 46 f8
    call 07474h                               ; e8 b0 fb
    cmp byte [bp-008h], 0fah                  ; 80 7e f8 fa
    db  00fh, 095h, 0c1h
    ; setne cl                                  ; 0f 95 c1
    test cl, cl                               ; 84 c9
    je near 0794bh                            ; 0f 84 7a 00
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 003h                  ; c6 46 13 03
    jmp short 0794bh                          ; eb 70
    movzx ax, byte [bp+00dh]                  ; 0f b6 46 0d
    push ax                                   ; 50
    push 0096ch                               ; 68 6c 09
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 83 a1
    add sp, strict byte 00006h                ; 83 c4 06
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 001h                  ; c6 46 13 01
    jmp short 0794bh                          ; eb 56
    mov si, word [bp+00ch]                    ; 8b 76 0c
    mov bx, si                                ; 89 f3
    mov dx, strict word 00022h                ; ba 22 00
    mov ax, cx                                ; 89 c8
    call 01773h                               ; e8 71 9e
    mov bx, word [bp+014h]                    ; 8b 5e 14
    mov dx, strict word 00024h                ; ba 24 00
    mov ax, cx                                ; 89 c8
    call 01773h                               ; e8 66 9e
    mov dx, strict word 00027h                ; ba 27 00
    mov ax, cx                                ; 89 c8
    call 01749h                               ; e8 34 9e
    mov ah, al                                ; 88 c4
    test si, si                               ; 85 f6
    jne short 07929h                          ; 75 0e
    cmp word [bp+014h], strict byte 00000h    ; 83 7e 14 00
    jne short 07929h                          ; 75 08
    test AL, strict byte 080h                 ; a8 80
    je short 0792bh                           ; 74 06
    and AL, strict byte 07fh                  ; 24 7f
    jmp short 0792bh                          ; eb 02
    or AL, strict byte 080h                   ; 0c 80
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00027h                ; ba 27 00
    mov ax, cx                                ; 89 c8
    call 01757h                               ; e8 21 9e
    jmp short 0794bh                          ; eb 13
    push 00986h                               ; 68 86 09
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 2b a1
    add sp, strict byte 00004h                ; 83 c4 04
    or word [bp+018h], strict byte 00001h     ; 83 4e 18 01
    mov byte [bp+013h], 001h                  ; c6 46 13 01
    mov ax, strict word 00047h                ; b8 47 00
    call 074d0h                               ; e8 7f fb
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
_int17_function:                             ; 0xf7957 LB 0xb3
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push ax                                   ; 50
    sti                                       ; fb
    mov dx, word [bp+00eh]                    ; 8b 56 0e
    add dx, dx                                ; 01 d2
    add dx, strict byte 00008h                ; 83 c2 08
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 fa 9d
    mov bx, ax                                ; 89 c3
    mov si, ax                                ; 89 c6
    cmp byte [bp+013h], 003h                  ; 80 7e 13 03
    jnc near 07a00h                           ; 0f 83 89 00
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    cmp ax, strict word 00003h                ; 3d 03 00
    jnc near 07a00h                           ; 0f 83 7f 00
    test bx, bx                               ; 85 db
    jbe near 07a00h                           ; 0f 86 79 00
    mov dx, ax                                ; 89 c2
    add dx, strict byte 00078h                ; 83 c2 78
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 b7 9d
    movzx cx, al                              ; 0f b6 c8
    sal cx, 008h                              ; c1 e1 08
    cmp byte [bp+013h], 000h                  ; 80 7e 13 00
    jne short 079cbh                          ; 75 2d
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
    movzx ax, byte [bp-004h]                  ; 0f b6 46 fc
    and AL, strict byte 0feh                  ; 24 fe
    out DX, AL                                ; ee
    lea dx, [si+001h]                         ; 8d 54 01
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 040h                 ; a8 40
    je short 079cbh                           ; 74 07
    test cx, cx                               ; 85 c9
    je short 079cbh                           ; 74 03
    dec cx                                    ; 49
    jmp short 079bah                          ; eb ef
    cmp byte [bp+013h], 001h                  ; 80 7e 13 01
    jne short 079e7h                          ; 75 16
    lea dx, [si+002h]                         ; 8d 54 02
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov word [bp-004h], ax                    ; 89 46 fc
    movzx ax, byte [bp-004h]                  ; 0f b6 46 fc
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
    jne short 079fah                          ; 75 04
    or byte [bp+013h], 001h                   ; 80 4e 13 01
    and byte [bp+01ch], 0feh                  ; 80 66 1c fe
    jmp short 07a04h                          ; eb 04
    or byte [bp+01ch], 001h                   ; 80 4e 1c 01
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
wait_:                                       ; 0xf7a0a LB 0xb2
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 0000ah                ; 83 ec 0a
    mov si, ax                                ; 89 c6
    mov byte [bp-00ch], dl                    ; 88 56 f4
    mov byte [bp-00ah], 000h                  ; c6 46 f6 00
    pushfw                                    ; 9c
    pop ax                                    ; 58
    mov word [bp-010h], ax                    ; 89 46 f0
    sti                                       ; fb
    xor cx, cx                                ; 31 c9
    mov dx, 0046ch                            ; ba 6c 04
    xor ax, ax                                ; 31 c0
    call 01781h                               ; e8 54 9d
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov bx, dx                                ; 89 d3
    hlt                                       ; f4
    mov dx, 0046ch                            ; ba 6c 04
    xor ax, ax                                ; 31 c0
    call 01781h                               ; e8 46 9d
    mov word [bp-012h], ax                    ; 89 46 ee
    mov di, dx                                ; 89 d7
    cmp dx, bx                                ; 39 da
    jnbe short 07a4bh                         ; 77 07
    jne short 07a52h                          ; 75 0c
    cmp ax, word [bp-00eh]                    ; 3b 46 f2
    jbe short 07a52h                          ; 76 07
    sub ax, word [bp-00eh]                    ; 2b 46 f2
    sbb dx, bx                                ; 19 da
    jmp short 07a5dh                          ; eb 0b
    cmp dx, bx                                ; 39 da
    jc short 07a5dh                           ; 72 07
    jne short 07a61h                          ; 75 09
    cmp ax, word [bp-00eh]                    ; 3b 46 f2
    jnc short 07a61h                          ; 73 04
    sub si, ax                                ; 29 c6
    sbb cx, dx                                ; 19 d1
    mov ax, word [bp-012h]                    ; 8b 46 ee
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov bx, di                                ; 89 fb
    mov ax, 00100h                            ; b8 00 01
    int 016h                                  ; cd 16
    je near 07a77h                            ; 0f 84 05 00
    mov AL, strict byte 001h                  ; b0 01
    jmp near 07a79h                           ; e9 02 00
    db  032h, 0c0h
    ; xor al, al                                ; 32 c0
    test al, al                               ; 84 c0
    je short 07aa1h                           ; 74 24
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    int 016h                                  ; cd 16
    xchg ah, al                               ; 86 c4
    mov dl, al                                ; 88 c2
    mov byte [bp-00ah], al                    ; 88 46 f6
    movzx ax, dl                              ; 0f b6 c2
    push ax                                   ; 50
    push 009a8h                               ; 68 a8 09
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 d7 9f
    add sp, strict byte 00006h                ; 83 c4 06
    cmp byte [bp-00ch], 000h                  ; 80 7e f4 00
    je short 07aa1h                           ; 74 04
    mov al, dl                                ; 88 d0
    jmp short 07ab3h                          ; eb 12
    test cx, cx                               ; 85 c9
    jnle short 07a32h                         ; 7f 8d
    jne short 07aabh                          ; 75 04
    test si, si                               ; 85 f6
    jnbe short 07a32h                         ; 77 87
    mov ax, word [bp-010h]                    ; 8b 46 f0
    push ax                                   ; 50
    popfw                                     ; 9d
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    lea sp, [bp-008h]                         ; 8d 66 f8
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
read_logo_byte_:                             ; 0xf7abc LB 0x16
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
read_logo_word_:                             ; 0xf7ad2 LB 0x14
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
print_detected_harddisks_:                   ; 0xf7ae6 LB 0x130
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    push ax                                   ; 50
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 6c 9c
    mov si, ax                                ; 89 c6
    mov byte [bp-00ch], 000h                  ; c6 46 f4 00
    xor cl, cl                                ; 30 c9
    xor ch, ch                                ; 30 ed
    mov dx, 00304h                            ; ba 04 03
    call 01749h                               ; e8 40 9c
    mov byte [bp-00eh], al                    ; 88 46 f2
    xor bl, bl                                ; 30 db
    cmp bl, byte [bp-00eh]                    ; 3a 5e f2
    jnc near 07be8h                           ; 0f 83 d3 00
    movzx dx, bl                              ; 0f b6 d3
    add dx, 00305h                            ; 81 c2 05 03
    mov ax, si                                ; 89 f0
    call 01749h                               ; e8 28 9c
    mov bh, al                                ; 88 c7
    cmp AL, strict byte 00ch                  ; 3c 0c
    jc short 07b4bh                           ; 72 24
    test cl, cl                               ; 84 c9
    jne short 07b38h                          ; 75 0d
    push 009b9h                               ; 68 b9 09
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 38 9f
    add sp, strict byte 00004h                ; 83 c4 04
    mov CL, strict byte 001h                  ; b1 01
    movzx ax, bl                              ; 0f b6 c3
    inc ax                                    ; 40
    push ax                                   ; 50
    push 009ceh                               ; 68 ce 09
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 26 9f
    add sp, strict byte 00006h                ; 83 c4 06
    jmp near 07be3h                           ; e9 98 00
    cmp AL, strict byte 008h                  ; 3c 08
    jc short 07b62h                           ; 72 13
    test ch, ch                               ; 84 ed
    jne short 07b60h                          ; 75 0d
    push 009e1h                               ; 68 e1 09
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 10 9f
    add sp, strict byte 00004h                ; 83 c4 04
    mov CH, strict byte 001h                  ; b5 01
    jmp short 07b38h                          ; eb d6
    cmp AL, strict byte 004h                  ; 3c 04
    jnc short 07b7dh                          ; 73 17
    cmp byte [bp-00ch], 000h                  ; 80 7e f4 00
    jne short 07b7dh                          ; 75 11
    push 009f6h                               ; 68 f6 09
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 f7 9e
    add sp, strict byte 00004h                ; 83 c4 04
    mov byte [bp-00ch], 001h                  ; c6 46 f4 01
    jmp short 07b93h                          ; eb 16
    cmp bh, 004h                              ; 80 ff 04
    jc short 07b93h                           ; 72 11
    test cl, cl                               ; 84 c9
    jne short 07b93h                          ; 75 0d
    push 00a08h                               ; 68 08 0a
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 dd 9e
    add sp, strict byte 00004h                ; 83 c4 04
    mov CL, strict byte 001h                  ; b1 01
    movzx ax, bl                              ; 0f b6 c3
    inc ax                                    ; 40
    push ax                                   ; 50
    push 00a1ch                               ; 68 1c 0a
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 cb 9e
    add sp, strict byte 00006h                ; 83 c4 06
    cmp bh, 004h                              ; 80 ff 04
    jc short 07babh                           ; 72 03
    sub bh, 004h                              ; 80 ef 04
    movzx ax, bh                              ; 0f b6 c7
    cwd                                       ; 99
    db  02bh, 0c2h
    ; sub ax, dx                                ; 2b c2
    sar ax, 1                                 ; d1 f8
    test ax, ax                               ; 85 c0
    je short 07bbch                           ; 74 05
    push 00a26h                               ; 68 26 0a
    jmp short 07bbfh                          ; eb 03
    push 00a31h                               ; 68 31 0a
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 a7 9e
    add sp, strict byte 00004h                ; 83 c4 04
    movzx ax, bh                              ; 0f b6 c7
    mov di, strict word 00002h                ; bf 02 00
    cwd                                       ; 99
    idiv di                                   ; f7 ff
    test dx, dx                               ; 85 d2
    je short 07bd9h                           ; 74 05
    push 00a3ah                               ; 68 3a 0a
    jmp short 07bdch                          ; eb 03
    push 00a40h                               ; 68 40 0a
    push di                                   ; 57
    call 01a6bh                               ; e8 8b 9e
    add sp, strict byte 00004h                ; 83 c4 04
    db  0feh, 0c3h
    ; inc bl                                    ; fe c3
    jmp near 07b0eh                           ; e9 26 ff
    cmp byte [bp-00ch], 000h                  ; 80 7e f4 00
    jne short 07c01h                          ; 75 13
    test cl, cl                               ; 84 c9
    jne short 07c01h                          ; 75 0f
    test ch, ch                               ; 84 ed
    jne short 07c01h                          ; 75 0b
    push 00a47h                               ; 68 47 0a
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 6d 9e
    add sp, strict byte 00004h                ; 83 c4 04
    push 00a5bh                               ; 68 5b 0a
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 62 9e
    add sp, strict byte 00004h                ; 83 c4 04
    lea sp, [bp-00ah]                         ; 8d 66 f6
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
get_boot_drive_:                             ; 0xf7c16 LB 0x28
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push dx                                   ; 52
    mov bl, al                                ; 88 c3
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 3f 9b
    mov dx, 00304h                            ; ba 04 03
    call 01749h                               ; e8 1d 9b
    sub bl, 002h                              ; 80 eb 02
    cmp bl, al                                ; 38 c3
    jc short 07c35h                           ; 72 02
    mov BL, strict byte 0ffh                  ; b3 ff
    mov al, bl                                ; 88 d8
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop dx                                    ; 5a
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
show_logo_:                                  ; 0xf7c3e LB 0x231
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 0000ch                ; 83 ec 0c
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 13 9b
    mov si, ax                                ; 89 c6
    xor cl, cl                                ; 30 c9
    xor dx, dx                                ; 31 d2
    mov AL, strict byte 034h                  ; b0 34
    out strict byte 043h, AL                  ; e6 43
    mov AL, strict byte 0d3h                  ; b0 d3
    out strict byte 040h, AL                  ; e6 40
    mov AL, strict byte 048h                  ; b0 48
    out strict byte 040h, AL                  ; e6 40
    movzx ax, dl                              ; 0f b6 c2
    call 07ad2h                               ; e8 68 fe
    cmp ax, 066bbh                            ; 3d bb 66
    jne near 07d4fh                           ; 0f 85 de 00
    push SS                                   ; 16
    pop ES                                    ; 07
    lea di, [bp-016h]                         ; 8d 7e ea
    mov ax, 04f03h                            ; b8 03 4f
    int 010h                                  ; cd 10
    mov word [es:di], bx                      ; 26 89 1d
    cmp ax, strict word 0004fh                ; 3d 4f 00
    jne near 07d4fh                           ; 0f 85 ca 00
    mov al, dl                                ; 88 d0
    add AL, strict byte 004h                  ; 04 04
    xor ah, ah                                ; 30 e4
    call 07abch                               ; e8 2e fe
    mov ch, al                                ; 88 c5
    mov byte [bp-00ch], al                    ; 88 46 f4
    mov al, dl                                ; 88 d0
    add AL, strict byte 005h                  ; 04 05
    xor ah, ah                                ; 30 e4
    call 07abch                               ; e8 20 fe
    mov dh, al                                ; 88 c6
    mov byte [bp-010h], al                    ; 88 46 f0
    mov al, dl                                ; 88 d0
    add AL, strict byte 002h                  ; 04 02
    xor ah, ah                                ; 30 e4
    call 07ad2h                               ; e8 28 fe
    mov bx, ax                                ; 89 c3
    mov word [bp-014h], ax                    ; 89 46 ec
    mov al, dl                                ; 88 d0
    add AL, strict byte 006h                  ; 04 06
    xor ah, ah                                ; 30 e4
    call 07abch                               ; e8 04 fe
    mov byte [bp-012h], al                    ; 88 46 ee
    test ch, ch                               ; 84 ed
    jne short 07cc9h                          ; 75 0a
    test dh, dh                               ; 84 f6
    jne short 07cc9h                          ; 75 06
    test bx, bx                               ; 85 db
    je near 07d4fh                            ; 0f 84 86 00
    mov bx, 00142h                            ; bb 42 01
    mov ax, 04f02h                            ; b8 02 4f
    int 010h                                  ; cd 10
    cmp byte [bp-00ch], 000h                  ; 80 7e f4 00
    je short 07cfah                           ; 74 23
    xor bx, bx                                ; 31 db
    jmp short 07ce1h                          ; eb 06
    inc bx                                    ; 43
    cmp bx, strict byte 00010h                ; 83 fb 10
    jnbe short 07d01h                         ; 77 20
    mov ax, bx                                ; 89 d8
    or ah, 002h                               ; 80 cc 02
    mov dx, 003b8h                            ; ba b8 03
    out DX, ax                                ; ef
    xor dx, dx                                ; 31 d2
    mov ax, strict word 00001h                ; b8 01 00
    call 07a0ah                               ; e8 18 fd
    cmp AL, strict byte 086h                  ; 3c 86
    jne short 07cdbh                          ; 75 e5
    mov CL, strict byte 001h                  ; b1 01
    jmp short 07d01h                          ; eb 07
    mov ax, 00210h                            ; b8 10 02
    mov dx, 003b8h                            ; ba b8 03
    out DX, ax                                ; ef
    test cl, cl                               ; 84 c9
    jne short 07d17h                          ; 75 12
    mov ax, word [bp-014h]                    ; 8b 46 ec
    shr ax, 004h                              ; c1 e8 04
    mov dx, strict word 00001h                ; ba 01 00
    call 07a0ah                               ; e8 f9 fc
    cmp AL, strict byte 086h                  ; 3c 86
    jne short 07d17h                          ; 75 02
    mov CL, strict byte 001h                  ; b1 01
    cmp byte [bp-010h], 000h                  ; 80 7e f0 00
    je short 07d44h                           ; 74 27
    test cl, cl                               ; 84 c9
    jne short 07d44h                          ; 75 23
    mov bx, strict word 00010h                ; bb 10 00
    jmp short 07d2bh                          ; eb 05
    dec bx                                    ; 4b
    test bx, bx                               ; 85 db
    jbe short 07d4fh                          ; 76 24
    mov ax, bx                                ; 89 d8
    or ah, 002h                               ; 80 cc 02
    mov dx, 003b8h                            ; ba b8 03
    out DX, ax                                ; ef
    xor dx, dx                                ; 31 d2
    mov ax, strict word 00001h                ; b8 01 00
    call 07a0ah                               ; e8 ce fc
    cmp AL, strict byte 086h                  ; 3c 86
    jne short 07d26h                          ; 75 e6
    mov CL, strict byte 001h                  ; b1 01
    jmp short 07d4fh                          ; eb 0b
    test cl, cl                               ; 84 c9
    jne short 07d4fh                          ; 75 07
    mov ax, 00200h                            ; b8 00 02
    mov dx, 003b8h                            ; ba b8 03
    out DX, ax                                ; ef
    xor bx, bx                                ; 31 db
    mov dx, 0037dh                            ; ba 7d 03
    mov ax, si                                ; 89 f0
    call 01757h                               ; e8 fe 99
    mov AL, strict byte 003h                  ; b0 03
    mov AH, strict byte 000h                  ; b4 00
    int 010h                                  ; cd 10
    cmp byte [bp-012h], 000h                  ; 80 7e ee 00
    je near 07e50h                            ; 0f 84 e9 00
    cmp byte [bp-00ch], 000h                  ; 80 7e f4 00
    jne short 07d9dh                          ; 75 30
    cmp byte [bp-010h], 000h                  ; 80 7e f0 00
    jne short 07d9dh                          ; 75 2a
    cmp word [bp-014h], strict byte 00000h    ; 83 7e ec 00
    jne short 07d9dh                          ; 75 24
    cmp byte [bp-012h], 002h                  ; 80 7e ee 02
    jne short 07d8ah                          ; 75 0b
    push 00a5dh                               ; 68 5d 0a
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 e4 9c
    add sp, strict byte 00004h                ; 83 c4 04
    test cl, cl                               ; 84 c9
    jne short 07d9dh                          ; 75 0f
    mov dx, strict word 00001h                ; ba 01 00
    mov ax, 000c0h                            ; b8 c0 00
    call 07a0ah                               ; e8 73 fc
    cmp AL, strict byte 086h                  ; 3c 86
    jne short 07d9dh                          ; 75 02
    mov CL, strict byte 001h                  ; b1 01
    test cl, cl                               ; 84 c9
    je near 07e50h                            ; 0f 84 ad 00
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
    push 00a7fh                               ; 68 7f 0a
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 9f 9c
    add sp, strict byte 00004h                ; 83 c4 04
    call 07ae6h                               ; e8 14 fd
    push 00ac3h                               ; 68 c3 0a
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 91 9c
    add sp, strict byte 00004h                ; 83 c4 04
    mov dx, strict word 00001h                ; ba 01 00
    mov ax, strict word 00040h                ; b8 40 00
    call 07a0ah                               ; e8 24 fc
    mov bl, al                                ; 88 c3
    test al, al                               ; 84 c0
    je short 07dddh                           ; 74 f1
    cmp AL, strict byte 030h                  ; 3c 30
    je short 07e3eh                           ; 74 4e
    cmp bl, 002h                              ; 80 fb 02
    jc short 07e17h                           ; 72 22
    cmp bl, 009h                              ; 80 fb 09
    jnbe short 07e17h                         ; 77 1d
    movzx ax, bl                              ; 0f b6 c3
    call 07c16h                               ; e8 16 fe
    cmp AL, strict byte 0ffh                  ; 3c ff
    jne short 07e06h                          ; 75 02
    jmp short 07dddh                          ; eb d7
    movzx bx, al                              ; 0f b6 d8
    mov dx, 0037ch                            ; ba 7c 03
    mov ax, si                                ; 89 f0
    call 01757h                               ; e8 46 99
    mov byte [bp-00eh], 002h                  ; c6 46 f2 02
    jmp short 07e3eh                          ; eb 27
    cmp bl, 02eh                              ; 80 fb 2e
    je short 07e2ch                           ; 74 10
    cmp bl, 026h                              ; 80 fb 26
    je short 07e32h                           ; 74 11
    cmp bl, 021h                              ; 80 fb 21
    jne short 07e38h                          ; 75 12
    mov byte [bp-00eh], 001h                  ; c6 46 f2 01
    jmp short 07e3eh                          ; eb 12
    mov byte [bp-00eh], 003h                  ; c6 46 f2 03
    jmp short 07e3eh                          ; eb 0c
    mov byte [bp-00eh], 004h                  ; c6 46 f2 04
    jmp short 07e3eh                          ; eb 06
    cmp byte [bp-00eh], 000h                  ; 80 7e f2 00
    je short 07dddh                           ; 74 9f
    movzx bx, byte [bp-00eh]                  ; 0f b6 5e f2
    mov dx, 0037dh                            ; ba 7d 03
    mov ax, si                                ; 89 f0
    call 01757h                               ; e8 0d 99
    mov AL, strict byte 003h                  ; b0 03
    mov AH, strict byte 000h                  ; b4 00
    int 010h                                  ; cd 10
    mov AL, strict byte 034h                  ; b0 34
    out strict byte 043h, AL                  ; e6 43
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    out strict byte 040h, AL                  ; e6 40
    out strict byte 040h, AL                  ; e6 40
    pushad                                    ; 66 60
    push DS                                   ; 1e
    mov ds, ax                                ; 8e d8
    call 0edbfh                               ; e8 5d 6f
    pop DS                                    ; 1f
    popad                                     ; 66 61
    lea sp, [bp-00ah]                         ; 8d 66 f6
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
delay_boot_:                                 ; 0xf7e6f LB 0x67
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push dx                                   ; 52
    mov dx, ax                                ; 89 c2
    test ax, ax                               ; 85 c0
    je short 07ecfh                           ; 74 55
    mov AL, strict byte 034h                  ; b0 34
    out strict byte 043h, AL                  ; e6 43
    mov AL, strict byte 0d3h                  ; b0 d3
    out strict byte 040h, AL                  ; e6 40
    mov AL, strict byte 048h                  ; b0 48
    out strict byte 040h, AL                  ; e6 40
    push dx                                   ; 52
    push 00b0dh                               ; 68 0d 0b
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 dc 9b
    add sp, strict byte 00006h                ; 83 c4 06
    mov bx, dx                                ; 89 d3
    test bx, bx                               ; 85 db
    jbe short 07eafh                          ; 76 17
    push bx                                   ; 53
    push 00b2bh                               ; 68 2b 0b
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 ca 9b
    add sp, strict byte 00006h                ; 83 c4 06
    xor dx, dx                                ; 31 d2
    mov ax, strict word 00040h                ; b8 40 00
    call 07a0ah                               ; e8 5e fb
    dec bx                                    ; 4b
    jmp short 07e94h                          ; eb e5
    push 00a5bh                               ; 68 5b 0a
    push strict byte 00002h                   ; 6a 02
    call 01a6bh                               ; e8 b4 9b
    add sp, strict byte 00004h                ; 83 c4 04
    mov AL, strict byte 034h                  ; b0 34
    out strict byte 043h, AL                  ; e6 43
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    out strict byte 040h, AL                  ; e6 40
    out strict byte 040h, AL                  ; e6 40
    pushad                                    ; 66 60
    push DS                                   ; 1e
    mov ds, ax                                ; 8e d8
    call 0edbfh                               ; e8 f3 6e
    pop DS                                    ; 1f
    popad                                     ; 66 61
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop dx                                    ; 5a
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
scsi_cmd_data_in_:                           ; 0xf7ed6 LB 0xd5
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
    jne short 07eech                          ; 75 f7
    cmp byte [bp+004h], 010h                  ; 80 7e 04 10
    jne short 07effh                          ; 75 04
    xor ax, ax                                ; 31 c0
    jmp short 07f03h                          ; eb 04
    movzx ax, byte [bp+004h]                  ; 0f b6 46 04
    mov di, ax                                ; 89 c7
    mov ax, bx                                ; 89 d8
    mov dx, word [bp+00ch]                    ; 8b 56 0c
    mov cx, strict word 0000ch                ; b9 0c 00
    shr dx, 1                                 ; d1 ea
    rcr ax, 1                                 ; d1 d8
    loop 07f0dh                               ; e2 fa
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
    loop 07f32h                               ; e2 fa
    mov dx, si                                ; 89 f2
    out DX, AL                                ; ee
    xor cx, cx                                ; 31 c9
    movzx ax, byte [bp+004h]                  ; 0f b6 46 04
    cmp cx, ax                                ; 39 c1
    jnc short 07f53h                          ; 73 0e
    les di, [bp-00ah]                         ; c4 7e f6
    add di, cx                                ; 01 cf
    mov al, byte [es:di]                      ; 26 8a 05
    mov dx, si                                ; 89 f2
    out DX, AL                                ; ee
    inc cx                                    ; 41
    jmp short 07f3dh                          ; eb ea
    mov dx, si                                ; 89 f2
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 07f53h                          ; 75 f7
    test AL, strict byte 002h                 ; a8 02
    je short 07f6eh                           ; 74 0e
    lea dx, [si+003h]                         ; 8d 54 03
    xor al, al                                ; 30 c0
    out DX, AL                                ; ee
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov di, strict word 00004h                ; bf 04 00
    jmp short 07fa0h                          ; eb 32
    lea dx, [si+001h]                         ; 8d 54 01
    cmp word [bp+00ch], strict byte 00000h    ; 83 7e 0c 00
    jne short 07f7dh                          ; 75 06
    cmp bx, 08000h                            ; 81 fb 00 80
    jbe short 07f97h                          ; 76 1a
    mov cx, 08000h                            ; b9 00 80
    les di, [bp+006h]                         ; c4 7e 06
    rep insb                                  ; f3 6c
    add bx, 08000h                            ; 81 c3 00 80
    adc word [bp+00ch], strict byte 0ffffh    ; 83 56 0c ff
    mov ax, es                                ; 8c c0
    add ax, 00800h                            ; 05 00 08
    mov word [bp+008h], ax                    ; 89 46 08
    jmp short 07f6eh                          ; eb d7
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
scsi_cmd_data_out_:                          ; 0xf7fab LB 0xd5
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
    jne short 07fc1h                          ; 75 f7
    cmp byte [bp+004h], 010h                  ; 80 7e 04 10
    jne short 07fd4h                          ; 75 04
    xor ax, ax                                ; 31 c0
    jmp short 07fd8h                          ; eb 04
    movzx ax, byte [bp+004h]                  ; 0f b6 46 04
    mov si, ax                                ; 89 c6
    mov ax, bx                                ; 89 d8
    mov dx, word [bp+00ch]                    ; 8b 56 0c
    mov cx, strict word 0000ch                ; b9 0c 00
    shr dx, 1                                 ; d1 ea
    rcr ax, 1                                 ; d1 d8
    loop 07fe2h                               ; e2 fa
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
    loop 08007h                               ; e2 fa
    mov dx, di                                ; 89 fa
    out DX, AL                                ; ee
    xor cx, cx                                ; 31 c9
    movzx ax, byte [bp+004h]                  ; 0f b6 46 04
    cmp cx, ax                                ; 39 c1
    jnc short 08028h                          ; 73 0e
    les si, [bp-00ah]                         ; c4 76 f6
    add si, cx                                ; 01 ce
    mov al, byte [es:si]                      ; 26 8a 04
    mov dx, di                                ; 89 fa
    out DX, AL                                ; ee
    inc cx                                    ; 41
    jmp short 08012h                          ; eb ea
    lea dx, [di+001h]                         ; 8d 55 01
    cmp word [bp+00ch], strict byte 00000h    ; 83 7e 0c 00
    jne short 08037h                          ; 75 06
    cmp bx, 08000h                            ; 81 fb 00 80
    jbe short 08052h                          ; 76 1b
    mov cx, 08000h                            ; b9 00 80
    les si, [bp+006h]                         ; c4 76 06
    db  0f3h, 026h, 06eh
    ; rep es outsb                              ; f3 26 6e
    add bx, 08000h                            ; 81 c3 00 80
    adc word [bp+00ch], strict byte 0ffffh    ; 83 56 0c ff
    mov ax, es                                ; 8c c0
    add ax, 00800h                            ; 05 00 08
    mov word [bp+008h], ax                    ; 89 46 08
    jmp short 08028h                          ; eb d6
    mov cx, bx                                ; 89 d9
    les si, [bp+006h]                         ; c4 76 06
    db  0f3h, 026h, 06eh
    ; rep es outsb                              ; f3 26 6e
    mov dx, di                                ; 89 fa
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 0805ah                          ; 75 f7
    test AL, strict byte 002h                 ; a8 02
    je short 08075h                           ; 74 0e
    lea dx, [di+003h]                         ; 8d 55 03
    xor al, al                                ; 30 c0
    out DX, AL                                ; ee
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov ax, strict word 00004h                ; b8 04 00
    jmp short 08077h                          ; eb 02
    xor ax, ax                                ; 31 c0
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 0000ah                               ; c2 0a 00
@scsi_read_sectors:                          ; 0xf8080 LB 0xdb
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
    jbe short 080aeh                          ; 76 13
    movzx ax, byte [bp-006h]                  ; 0f b6 46 fa
    push ax                                   ; 50
    push 00b30h                               ; 68 30 0b
    push 00b42h                               ; 68 42 0b
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 c0 99
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
    movzx ax, byte [bp-006h]                  ; 0f b6 46 fa
    sal ax, 002h                              ; c1 e0 02
    mov bx, si                                ; 89 f3
    add bx, ax                                ; 01 c3
    mov ax, word [es:bx+0021ch]               ; 26 8b 87 1c 02
    mov dl, byte [es:bx+0021eh]               ; 26 8a 97 1e 02
    mov word [bp-00ah], di                    ; 89 7e f6
    mov word [bp-008h], strict word 00000h    ; c7 46 f8 00 00
    mov cx, strict word 00009h                ; b9 09 00
    sal word [bp-00ah], 1                     ; d1 66 f6
    rcl word [bp-008h], 1                     ; d1 56 f8
    loop 08117h                               ; e2 f8
    push dword [bp-00ah]                      ; 66 ff 76 f6
    db  066h, 026h, 0ffh, 074h, 008h
    ; push dword [es:si+008h]                   ; 66 26 ff 74 08
    push strict byte 00010h                   ; 6a 10
    xor dh, dh                                ; 30 f6
    mov cx, ss                                ; 8c d1
    lea bx, [bp-01ah]                         ; 8d 5e e6
    call 07ed6h                               ; e8 a2 fd
    mov ah, al                                ; 88 c4
    test al, al                               ; 84 c0
    jne short 0814fh                          ; 75 15
    mov es, [bp+006h]                         ; 8e 46 06
    mov word [es:si+018h], di                 ; 26 89 7c 18
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    mov word [es:si+01ah], dx                 ; 26 89 54 1a
    mov dx, word [bp-008h]                    ; 8b 56 f8
    mov word [es:si+01ch], dx                 ; 26 89 54 1c
    movzx ax, ah                              ; 0f b6 c4
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 00004h                               ; c2 04 00
@scsi_write_sectors:                         ; 0xf815b LB 0xdb
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
    jbe short 08189h                          ; 76 13
    movzx ax, byte [bp-006h]                  ; 0f b6 46 fa
    push ax                                   ; 50
    push 00b61h                               ; 68 61 0b
    push 00b42h                               ; 68 42 0b
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 e5 98
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
    movzx ax, byte [bp-006h]                  ; 0f b6 46 fa
    sal ax, 002h                              ; c1 e0 02
    mov bx, si                                ; 89 f3
    add bx, ax                                ; 01 c3
    mov ax, word [es:bx+0021ch]               ; 26 8b 87 1c 02
    mov dl, byte [es:bx+0021eh]               ; 26 8a 97 1e 02
    mov word [bp-00ah], di                    ; 89 7e f6
    mov word [bp-008h], strict word 00000h    ; c7 46 f8 00 00
    mov cx, strict word 00009h                ; b9 09 00
    sal word [bp-00ah], 1                     ; d1 66 f6
    rcl word [bp-008h], 1                     ; d1 56 f8
    loop 081f2h                               ; e2 f8
    push dword [bp-00ah]                      ; 66 ff 76 f6
    db  066h, 026h, 0ffh, 074h, 008h
    ; push dword [es:si+008h]                   ; 66 26 ff 74 08
    push strict byte 00010h                   ; 6a 10
    xor dh, dh                                ; 30 f6
    mov cx, ss                                ; 8c d1
    lea bx, [bp-01ah]                         ; 8d 5e e6
    call 07fabh                               ; e8 9c fd
    mov ah, al                                ; 88 c4
    test al, al                               ; 84 c0
    jne short 0822ah                          ; 75 15
    mov es, [bp+006h]                         ; 8e 46 06
    mov word [es:si+018h], di                 ; 26 89 7c 18
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    mov word [es:si+01ah], dx                 ; 26 89 54 1a
    mov dx, word [bp-008h]                    ; 8b 56 f8
    mov word [es:si+01ch], dx                 ; 26 89 54 1c
    movzx ax, ah                              ; 0f b6 c4
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 00004h                               ; c2 04 00
scsi_cmd_packet_:                            ; 0xf8236 LB 0x166
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 0000ch                ; 83 ec 0c
    mov di, ax                                ; 89 c7
    mov byte [bp-006h], dl                    ; 88 56 fa
    mov word [bp-00ch], bx                    ; 89 5e f4
    mov word [bp-00ah], cx                    ; 89 4e f6
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 13 95
    mov si, 00122h                            ; be 22 01
    mov word [bp-00eh], ax                    ; 89 46 f2
    cmp byte [bp+00ah], 002h                  ; 80 7e 0a 02
    jne short 0827dh                          ; 75 1f
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 c1 97
    push 00b74h                               ; 68 74 0b
    push 00b84h                               ; 68 84 0b
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 f7 97
    add sp, strict byte 00006h                ; 83 c4 06
    mov dx, strict word 00001h                ; ba 01 00
    jmp near 08391h                           ; e9 14 01
    sub di, strict byte 00008h                ; 83 ef 08
    sal di, 002h                              ; c1 e7 02
    sub byte [bp-006h], 002h                  ; 80 6e fa 02
    mov es, [bp-00eh]                         ; 8e 46 f2
    add di, si                                ; 01 f7
    mov bx, word [es:di+0021ch]               ; 26 8b 9d 1c 02
    mov al, byte [es:di+0021eh]               ; 26 8a 85 1e 02
    mov byte [bp-008h], al                    ; 88 46 f8
    mov dx, bx                                ; 89 da
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 08299h                          ; 75 f7
    xor ax, ax                                ; 31 c0
    mov dx, word [bp+006h]                    ; 8b 56 06
    add dx, word [bp+004h]                    ; 03 56 04
    adc ax, word [bp+008h]                    ; 13 46 08
    mov es, [bp-00eh]                         ; 8e 46 f2
    mov cx, word [es:si+020h]                 ; 26 8b 4c 20
    xor di, di                                ; 31 ff
    add cx, dx                                ; 01 d1
    mov word [bp-010h], cx                    ; 89 4e f0
    adc di, ax                                ; 11 c7
    mov ax, cx                                ; 89 c8
    mov dx, di                                ; 89 fa
    mov cx, strict word 0000ch                ; b9 0c 00
    shr dx, 1                                 ; d1 ea
    rcr ax, 1                                 ; d1 d8
    loop 082c4h                               ; e2 fa
    and ax, 000f0h                            ; 25 f0 00
    movzx cx, byte [bp-006h]                  ; 0f b6 4e fa
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
    loop 082ebh                               ; e2 fa
    mov dx, bx                                ; 89 da
    out DX, AL                                ; ee
    xor cx, cx                                ; 31 c9
    movzx ax, byte [bp-006h]                  ; 0f b6 46 fa
    cmp cx, ax                                ; 39 c1
    jnc short 0830ch                          ; 73 0e
    les di, [bp-00ch]                         ; c4 7e f4
    add di, cx                                ; 01 cf
    mov al, byte [es:di]                      ; 26 8a 05
    mov dx, bx                                ; 89 da
    out DX, AL                                ; ee
    inc cx                                    ; 41
    jmp short 082f6h                          ; eb ea
    mov dx, bx                                ; 89 da
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    test AL, strict byte 001h                 ; a8 01
    jne short 0830ch                          ; 75 f7
    test AL, strict byte 002h                 ; a8 02
    je short 08327h                           ; 74 0e
    lea dx, [bx+003h]                         ; 8d 57 03
    xor al, al                                ; 30 c0
    out DX, AL                                ; ee
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov dx, strict word 00003h                ; ba 03 00
    jmp short 08391h                          ; eb 6a
    mov ax, word [bp+004h]                    ; 8b 46 04
    test ax, ax                               ; 85 c0
    je short 08336h                           ; 74 08
    lea dx, [bx+001h]                         ; 8d 57 01
    mov cx, ax                                ; 89 c1
    in AL, DX                                 ; ec
    loop 08333h                               ; e2 fd
    mov ax, word [bp+006h]                    ; 8b 46 06
    mov es, [bp-00eh]                         ; 8e 46 f2
    mov word [es:si+01ah], ax                 ; 26 89 44 1a
    mov ax, word [bp+008h]                    ; 8b 46 08
    mov word [es:si+01ch], ax                 ; 26 89 44 1c
    lea ax, [bx+001h]                         ; 8d 47 01
    cmp word [bp+008h], strict byte 00000h    ; 83 7e 08 00
    jne short 08357h                          ; 75 07
    cmp word [bp+006h], 08000h                ; 81 7e 06 00 80
    jbe short 08374h                          ; 76 1d
    mov dx, ax                                ; 89 c2
    mov cx, 08000h                            ; b9 00 80
    les di, [bp+00ch]                         ; c4 7e 0c
    rep insb                                  ; f3 6c
    add word [bp+006h], 08000h                ; 81 46 06 00 80
    adc word [bp+008h], strict byte 0ffffh    ; 83 56 08 ff
    mov ax, es                                ; 8c c0
    add ax, 00800h                            ; 05 00 08
    mov word [bp+00eh], ax                    ; 89 46 0e
    jmp short 08347h                          ; eb d3
    mov dx, ax                                ; 89 c2
    mov cx, word [bp+006h]                    ; 8b 4e 06
    les di, [bp+00ch]                         ; c4 7e 0c
    rep insb                                  ; f3 6c
    mov es, [bp-00eh]                         ; 8e 46 f2
    cmp word [es:si+020h], strict byte 00000h ; 26 83 7c 20 00
    je short 0838fh                           ; 74 07
    mov cx, word [es:si+020h]                 ; 26 8b 4c 20
    in AL, DX                                 ; ec
    loop 0838ch                               ; e2 fd
    xor dx, dx                                ; 31 d2
    mov ax, dx                                ; 89 d0
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 0000ch                               ; c2 0c 00
scsi_enumerate_attached_devices_:            ; 0xf839c LB 0x482
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push si                                   ; 56
    push di                                   ; 57
    sub sp, 0023ch                            ; 81 ec 3c 02
    push ax                                   ; 50
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 b3 93
    mov di, 00122h                            ; bf 22 01
    mov word [bp-02eh], ax                    ; 89 46 d2
    mov word [bp-01eh], strict word 00000h    ; c7 46 e2 00 00
    jmp near 087a0h                           ; e9 e0 03
    cmp AL, strict byte 004h                  ; 3c 04
    jnc near 08814h                           ; 0f 83 4e 04
    mov cx, strict word 00010h                ; b9 10 00
    xor bx, bx                                ; 31 db
    mov dx, ss                                ; 8c d2
    lea ax, [bp-046h]                         ; 8d 46 ba
    call 0a130h                               ; e8 5d 1d
    mov byte [bp-046h], 09eh                  ; c6 46 ba 9e
    mov byte [bp-045h], 010h                  ; c6 46 bb 10
    mov byte [bp-039h], 020h                  ; c6 46 c7 20
    push dword 000000020h                     ; 66 6a 20
    lea dx, [bp-00246h]                       ; 8d 96 ba fd
    push SS                                   ; 16
    push dx                                   ; 52
    push strict byte 00010h                   ; 6a 10
    movzx dx, byte [bp-01eh]                  ; 0f b6 56 e2
    mov cx, ss                                ; 8c d1
    lea bx, [bp-046h]                         ; 8d 5e ba
    mov ax, word [bp-00248h]                  ; 8b 86 b8 fd
    call 07ed6h                               ; e8 dc fa
    test al, al                               ; 84 c0
    je short 0840ch                           ; 74 0e
    push 00ba4h                               ; 68 a4 0b
    push 00bddh                               ; 68 dd 0b
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 62 96
    add sp, strict byte 00006h                ; 83 c4 06
    mov ax, word [bp-00240h]                  ; 8b 86 c0 fd
    mov bx, word [bp-00242h]                  ; 8b 9e be fd
    mov cx, word [bp-00244h]                  ; 8b 8e bc fd
    mov dx, word [bp-00246h]                  ; 8b 96 ba fd
    xchg ah, al                               ; 86 c4
    xchg bh, bl                               ; 86 df
    xchg ch, cl                               ; 86 cd
    xchg dh, dl                               ; 86 d6
    xchg dx, ax                               ; 92
    xchg bx, cx                               ; 87 cb
    add dx, strict byte 00001h                ; 83 c2 01
    mov word [bp-016h], dx                    ; 89 56 ea
    adc cx, strict byte 00000h                ; 83 d1 00
    mov word [bp-012h], cx                    ; 89 4e ee
    adc bx, strict byte 00000h                ; 83 d3 00
    mov word [bp-030h], bx                    ; 89 5e d0
    adc ax, strict word 00000h                ; 15 00 00
    mov word [bp-010h], ax                    ; 89 46 f0
    movzx ax, byte [bp-0023eh]                ; 0f b6 86 c2 fd
    sal ax, 008h                              ; c1 e0 08
    movzx si, byte [bp-0023dh]                ; 0f b6 b6 c3 fd
    xor bx, bx                                ; 31 db
    or si, ax                                 ; 09 c6
    movzx ax, byte [bp-0023ch]                ; 0f b6 86 c4 fd
    xor dx, dx                                ; 31 d2
    mov cx, strict word 00008h                ; b9 08 00
    sal ax, 1                                 ; d1 e0
    rcl dx, 1                                 ; d1 d2
    loop 0845ah                               ; e2 fa
    or bx, ax                                 ; 09 c3
    or dx, si                                 ; 09 f2
    movzx ax, byte [bp-0023bh]                ; 0f b6 86 c5 fd
    or bx, ax                                 ; 09 c3
    mov word [bp-024h], bx                    ; 89 5e dc
    test dx, dx                               ; 85 d2
    jne short 08478h                          ; 75 06
    cmp bx, 00200h                            ; 81 fb 00 02
    je short 08498h                           ; 74 20
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 a7 95
    push dx                                   ; 52
    push word [bp-024h]                       ; ff 76 dc
    push word [bp-01eh]                       ; ff 76 e2
    push 00bfch                               ; 68 fc 0b
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 d9 95
    add sp, strict byte 0000ah                ; 83 c4 0a
    jmp near 08797h                           ; e9 ff 02
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    cmp AL, strict byte 001h                  ; 3c 01
    jc short 084abh                           ; 72 0c
    jbe short 084b3h                          ; 76 12
    cmp AL, strict byte 003h                  ; 3c 03
    je short 084bbh                           ; 74 16
    cmp AL, strict byte 002h                  ; 3c 02
    je short 084b7h                           ; 74 0e
    jmp short 08504h                          ; eb 59
    test al, al                               ; 84 c0
    jne short 08504h                          ; 75 55
    mov BL, strict byte 090h                  ; b3 90
    jmp short 084bdh                          ; eb 0a
    mov BL, strict byte 098h                  ; b3 98
    jmp short 084bdh                          ; eb 06
    mov BL, strict byte 0a0h                  ; b3 a0
    jmp short 084bdh                          ; eb 02
    mov BL, strict byte 0a8h                  ; b3 a8
    mov al, bl                                ; 88 d8
    add AL, strict byte 007h                  ; 04 07
    movzx cx, al                              ; 0f b6 c8
    mov ax, cx                                ; 89 c8
    call 017a5h                               ; e8 dc 92
    test al, al                               ; 84 c0
    je short 08504h                           ; 74 37
    mov al, bl                                ; 88 d8
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    xor ah, ah                                ; 30 e4
    call 017a5h                               ; e8 cf 92
    movzx dx, al                              ; 0f b6 d0
    sal dx, 008h                              ; c1 e2 08
    movzx ax, bl                              ; 0f b6 c3
    call 017a5h                               ; e8 c3 92
    xor ah, ah                                ; 30 e4
    add ax, dx                                ; 01 d0
    cwd                                       ; 99
    mov si, ax                                ; 89 c6
    mov al, bl                                ; 88 d8
    add AL, strict byte 002h                  ; 04 02
    xor ah, ah                                ; 30 e4
    call 017a5h                               ; e8 b3 92
    xor ah, ah                                ; 30 e4
    mov word [bp-034h], ax                    ; 89 46 cc
    mov ax, cx                                ; 89 c8
    call 017a5h                               ; e8 a9 92
    xor ah, ah                                ; 30 e4
    mov word [bp-032h], ax                    ; 89 46 ce
    jmp near 085edh                           ; e9 e9 00
    mov ax, word [bp-010h]                    ; 8b 46 f0
    mov bx, word [bp-030h]                    ; 8b 5e d0
    mov cx, word [bp-012h]                    ; 8b 4e ee
    mov dx, word [bp-016h]                    ; 8b 56 ea
    mov si, strict word 0000ch                ; be 0c 00
    call 0a110h                               ; e8 fa 1b
    mov word [bp-018h], ax                    ; 89 46 e8
    mov word [bp-014h], bx                    ; 89 5e ec
    mov word [bp-036h], cx                    ; 89 4e ca
    mov word [bp-020h], dx                    ; 89 56 e0
    mov ax, word [bp-010h]                    ; 8b 46 f0
    test ax, ax                               ; 85 c0
    jnbe short 0853fh                         ; 77 16
    jne near 085b2h                           ; 0f 85 85 00
    cmp word [bp-030h], strict byte 00000h    ; 83 7e d0 00
    jnbe short 0853fh                         ; 77 0c
    jne near 085b2h                           ; 0f 85 7b 00
    cmp word [bp-012h], strict byte 00040h    ; 83 7e ee 40
    jnbe short 0853fh                         ; 77 02
    jne short 085b2h                          ; 75 73
    mov dword [bp-034h], strict dword 0003f00ffh ; 66 c7 46 cc ff 00 3f 00
    mov bx, word [bp-030h]                    ; 8b 5e d0
    mov cx, word [bp-012h]                    ; 8b 4e ee
    mov dx, word [bp-016h]                    ; 8b 56 ea
    mov si, strict word 00006h                ; be 06 00
    call 0a110h                               ; e8 ba 1b
    mov si, word [bp-020h]                    ; 8b 76 e0
    add si, dx                                ; 01 d6
    mov word [bp-02ah], si                    ; 89 76 d6
    mov dx, word [bp-036h]                    ; 8b 56 ca
    adc dx, cx                                ; 11 ca
    mov word [bp-028h], dx                    ; 89 56 d8
    mov dx, word [bp-014h]                    ; 8b 56 ec
    adc dx, bx                                ; 11 da
    mov word [bp-01ch], dx                    ; 89 56 e4
    mov dx, word [bp-018h]                    ; 8b 56 e8
    adc dx, ax                                ; 11 c2
    mov word [bp-026h], dx                    ; 89 56 da
    mov ax, dx                                ; 89 d0
    mov bx, word [bp-01ch]                    ; 8b 5e e4
    mov cx, word [bp-028h]                    ; 8b 4e d8
    mov dx, si                                ; 89 f2
    mov si, strict word 00008h                ; be 08 00
    call 0a110h                               ; e8 8a 1b
    mov word [bp-022h], bx                    ; 89 5e de
    mov word [bp-02ch], cx                    ; 89 4e d4
    mov word [bp-01ah], dx                    ; 89 56 e6
    mov ax, word [bp-026h]                    ; 8b 46 da
    mov bx, word [bp-01ch]                    ; 8b 5e e4
    mov cx, word [bp-028h]                    ; 8b 4e d8
    mov dx, word [bp-02ah]                    ; 8b 56 d6
    mov si, strict word 00010h                ; be 10 00
    call 0a110h                               ; e8 6f 1b
    mov si, word [bp-01ah]                    ; 8b 76 e6
    add si, dx                                ; 01 d6
    mov dx, word [bp-02ch]                    ; 8b 56 d4
    adc dx, cx                                ; 11 ca
    mov ax, word [bp-022h]                    ; 8b 46 de
    adc ax, bx                                ; 11 d8
    jmp short 085edh                          ; eb 3b
    test ax, ax                               ; 85 c0
    jnbe short 085c8h                         ; 77 12
    jne short 085d2h                          ; 75 1a
    cmp word [bp-030h], strict byte 00000h    ; 83 7e d0 00
    jnbe short 085c8h                         ; 77 0a
    jne short 085d2h                          ; 75 12
    cmp word [bp-012h], strict byte 00020h    ; 83 7e ee 20
    jnbe short 085c8h                         ; 77 02
    jne short 085d2h                          ; 75 0a
    mov dword [bp-034h], strict dword 000200080h ; 66 c7 46 cc 80 00 20 00
    jmp short 085e9h                          ; eb 17
    mov dword [bp-034h], strict dword 000200040h ; 66 c7 46 cc 40 00 20 00
    mov bx, word [bp-030h]                    ; 8b 5e d0
    mov cx, word [bp-012h]                    ; 8b 4e ee
    mov dx, word [bp-016h]                    ; 8b 56 ea
    mov si, strict word 0000bh                ; be 0b 00
    call 0a110h                               ; e8 27 1b
    mov si, dx                                ; 89 d6
    mov dx, cx                                ; 89 ca
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    add AL, strict byte 008h                  ; 04 08
    mov byte [bp-00eh], al                    ; 88 46 f2
    movzx ax, byte [bp-00ch]                  ; 0f b6 46 f4
    sal ax, 002h                              ; c1 e0 02
    mov es, [bp-02eh]                         ; 8e 46 d2
    mov bx, di                                ; 89 fb
    add bx, ax                                ; 01 c3
    mov ax, word [bp-00248h]                  ; 8b 86 b8 fd
    mov word [es:bx+0021ch], ax               ; 26 89 87 1c 02
    mov al, byte [bp-01eh]                    ; 8a 46 e2
    mov byte [es:bx+0021eh], al               ; 26 88 87 1e 02
    movzx ax, byte [bp-00eh]                  ; 0f b6 46 f2
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov bx, di                                ; 89 fb
    add bx, ax                                ; 01 c3
    db  066h, 026h, 0c7h, 047h, 022h, 004h, 0ffh, 000h, 000h
    ; mov dword [es:bx+022h], strict dword 00000ff04h ; 66 26 c7 47 22 04 ff 00 00
    mov ax, word [bp-024h]                    ; 8b 46 dc
    mov word [es:bx+028h], ax                 ; 26 89 47 28
    mov byte [es:bx+027h], 001h               ; 26 c6 47 27 01
    mov ax, word [bp-034h]                    ; 8b 46 cc
    mov word [es:bx+02ah], ax                 ; 26 89 47 2a
    mov ax, word [bp-032h]                    ; 8b 46 ce
    mov word [es:bx+02eh], ax                 ; 26 89 47 2e
    mov ax, word [bp-034h]                    ; 8b 46 cc
    mov word [es:bx+030h], ax                 ; 26 89 47 30
    mov ax, word [bp-032h]                    ; 8b 46 ce
    mov word [es:bx+034h], ax                 ; 26 89 47 34
    test dx, dx                               ; 85 d2
    jne short 0865ah                          ; 75 06
    cmp si, 00400h                            ; 81 fe 00 04
    jbe short 08668h                          ; 76 0e
    mov word [es:bx+02ch], 00400h             ; 26 c7 47 2c 00 04
    mov word [es:bx+032h], 00400h             ; 26 c7 47 32 00 04
    jmp short 08670h                          ; eb 08
    mov word [es:bx+02ch], si                 ; 26 89 77 2c
    mov word [es:bx+032h], si                 ; 26 89 77 32
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 af 93
    push word [bp-010h]                       ; ff 76 f0
    push word [bp-030h]                       ; ff 76 d0
    push word [bp-012h]                       ; ff 76 ee
    push word [bp-016h]                       ; ff 76 ea
    push dword [bp-034h]                      ; 66 ff 76 cc
    push dx                                   ; 52
    push si                                   ; 56
    push word [bp-01eh]                       ; ff 76 e2
    movzx ax, byte [bp-00ch]                  ; 0f b6 46 f4
    push ax                                   ; 50
    push 00c2ah                               ; 68 2a 0c
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 ce 93
    add sp, strict byte 00018h                ; 83 c4 18
    movzx bx, byte [bp-00eh]                  ; 0f b6 5e f2
    imul bx, bx, strict byte 0001ch           ; 6b db 1c
    mov es, [bp-02eh]                         ; 8e 46 d2
    add bx, di                                ; 01 fb
    mov ax, word [bp-010h]                    ; 8b 46 f0
    mov word [es:bx+03ch], ax                 ; 26 89 47 3c
    mov ax, word [bp-030h]                    ; 8b 46 d0
    mov word [es:bx+03ah], ax                 ; 26 89 47 3a
    mov ax, word [bp-012h]                    ; 8b 46 ee
    mov word [es:bx+038h], ax                 ; 26 89 47 38
    mov ax, word [bp-016h]                    ; 8b 46 ea
    mov word [es:bx+036h], ax                 ; 26 89 47 36
    mov al, byte [es:di+001e2h]               ; 26 8a 85 e2 01
    mov ah, byte [bp-00ch]                    ; 8a 66 f4
    add ah, 008h                              ; 80 c4 08
    movzx bx, al                              ; 0f b6 d8
    add bx, di                                ; 01 fb
    mov byte [es:bx+001e3h], ah               ; 26 88 a7 e3 01
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    mov byte [es:di+001e2h], al               ; 26 88 85 e2 01
    mov dx, strict word 00075h                ; ba 75 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 5c 90
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00075h                ; ba 75 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 5c 90
    inc byte [bp-00ch]                        ; fe 46 f4
    jmp near 0878ch                           ; e9 8b 00
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 1e 93
    push word [bp-01eh]                       ; ff 76 e2
    movzx ax, byte [bp-00ch]                  ; 0f b6 46 f4
    push ax                                   ; 50
    push 00c58h                               ; 68 58 0c
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 4f 93
    add sp, strict byte 00008h                ; 83 c4 08
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    add AL, strict byte 008h                  ; 04 08
    mov byte [bp-00eh], al                    ; 88 46 f2
    test byte [bp-00245h], 080h               ; f6 86 bb fd 80
    db  00fh, 095h, 0c0h
    ; setne al                                  ; 0f 95 c0
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    movzx ax, byte [bp-00ch]                  ; 0f b6 46 f4
    sal ax, 002h                              ; c1 e0 02
    mov es, [bp-02eh]                         ; 8e 46 d2
    mov bx, di                                ; 89 fb
    add bx, ax                                ; 01 c3
    mov ax, word [bp-00248h]                  ; 8b 86 b8 fd
    mov word [es:bx+0021ch], ax               ; 26 89 87 1c 02
    mov al, byte [bp-01eh]                    ; 8a 46 e2
    mov byte [es:bx+0021eh], al               ; 26 88 87 1e 02
    movzx ax, byte [bp-00eh]                  ; 0f b6 46 f2
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov bx, di                                ; 89 fb
    add bx, ax                                ; 01 c3
    mov word [es:bx+022h], 00504h             ; 26 c7 47 22 04 05
    mov byte [es:bx+024h], dl                 ; 26 88 57 24
    mov word [es:bx+028h], 00800h             ; 26 c7 47 28 00 08
    mov al, byte [es:di+001f3h]               ; 26 8a 85 f3 01
    mov ah, byte [bp-00ch]                    ; 8a 66 f4
    add ah, 008h                              ; 80 c4 08
    movzx bx, al                              ; 0f b6 d8
    add bx, di                                ; 01 fb
    mov byte [es:bx+001f4h], ah               ; 26 88 a7 f4 01
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    mov byte [es:di+001f3h], al               ; 26 88 85 f3 01
    inc byte [bp-00ch]                        ; fe 46 f4
    mov al, byte [bp-00ch]                    ; 8a 46 f4
    mov es, [bp-02eh]                         ; 8e 46 d2
    mov byte [es:di+0022ch], al               ; 26 88 85 2c 02
    inc word [bp-01eh]                        ; ff 46 e2
    cmp word [bp-01eh], strict byte 00010h    ; 83 7e e2 10
    jnl short 08814h                          ; 7d 74
    mov byte [bp-046h], 012h                  ; c6 46 ba 12
    xor al, al                                ; 30 c0
    mov byte [bp-045h], al                    ; 88 46 bb
    mov byte [bp-044h], al                    ; 88 46 bc
    mov byte [bp-043h], al                    ; 88 46 bd
    mov byte [bp-042h], 005h                  ; c6 46 be 05
    mov byte [bp-041h], al                    ; 88 46 bf
    push dword 000000005h                     ; 66 6a 05
    lea dx, [bp-00246h]                       ; 8d 96 ba fd
    push SS                                   ; 16
    push dx                                   ; 52
    push strict byte 00006h                   ; 6a 06
    movzx dx, byte [bp-01eh]                  ; 0f b6 56 e2
    mov cx, ss                                ; 8c d1
    lea bx, [bp-046h]                         ; 8d 5e ba
    mov ax, word [bp-00248h]                  ; 8b 86 b8 fd
    call 07ed6h                               ; e8 05 f7
    test al, al                               ; 84 c0
    je short 087e3h                           ; 74 0e
    push 00ba4h                               ; 68 a4 0b
    push 00bc4h                               ; 68 c4 0b
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 8b 92
    add sp, strict byte 00006h                ; 83 c4 06
    mov es, [bp-02eh]                         ; 8e 46 d2
    mov al, byte [es:di+0022ch]               ; 26 8a 85 2c 02
    mov byte [bp-00ch], al                    ; 88 46 f4
    test byte [bp-00246h], 0e0h               ; f6 86 ba fd e0
    jne short 087feh                          ; 75 09
    test byte [bp-00246h], 01fh               ; f6 86 ba fd 1f
    je near 083c0h                            ; 0f 84 c2 fb
    test byte [bp-00246h], 0e0h               ; f6 86 ba fd e0
    jne short 0878ch                          ; 75 87
    mov al, byte [bp-00246h]                  ; 8a 86 ba fd
    and AL, strict byte 01fh                  ; 24 1f
    cmp AL, strict byte 005h                  ; 3c 05
    je near 08701h                            ; 0f 84 f0 fe
    jmp near 0878ch                           ; e9 78 ff
    lea sp, [bp-00ah]                         ; 8d 66 f6
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
scsi_pci_init_:                              ; 0xf881e LB 0x2d
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push si                                   ; 56
    call 09eefh                               ; e8 c8 16
    mov dx, ax                                ; 89 c2
    cmp ax, strict word 0ffffh                ; 3d ff ff
    je short 08843h                           ; 74 15
    movzx si, dl                              ; 0f b6 f2
    mov ax, dx                                ; 89 d0
    shr ax, 008h                              ; c1 e8 08
    xor ah, ah                                ; 30 e4
    mov cx, strict word 00007h                ; b9 07 00
    mov bx, strict word 00004h                ; bb 04 00
    mov dx, si                                ; 89 f2
    call 09f81h                               ; e8 3e 17
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
_scsi_init:                                  ; 0xf884b LB 0x81
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 0e 8f
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
    jne short 08884h                          ; 75 15
    xor al, al                                ; 30 c0
    mov dx, 00433h                            ; ba 33 04
    out DX, AL                                ; ee
    mov ax, 00430h                            ; b8 30 04
    call 0839ch                               ; e8 21 fb
    mov dx, 01040h                            ; ba 40 10
    mov ax, 0104bh                            ; b8 4b 10
    call 0881eh                               ; e8 9a ff
    mov AL, strict byte 055h                  ; b0 55
    mov dx, 00436h                            ; ba 36 04
    out DX, AL                                ; ee
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp AL, strict byte 055h                  ; 3c 55
    jne short 088a6h                          ; 75 15
    xor al, al                                ; 30 c0
    mov dx, 00437h                            ; ba 37 04
    out DX, AL                                ; ee
    mov ax, 00434h                            ; b8 34 04
    call 0839ch                               ; e8 ff fa
    mov dx, strict word 00030h                ; ba 30 00
    mov ax, 01000h                            ; b8 00 10
    call 0881eh                               ; e8 78 ff
    mov AL, strict byte 055h                  ; b0 55
    mov dx, 0043ah                            ; ba 3a 04
    out DX, AL                                ; ee
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp AL, strict byte 055h                  ; 3c 55
    jne short 088c8h                          ; 75 15
    xor al, al                                ; 30 c0
    mov dx, 0043bh                            ; ba 3b 04
    out DX, AL                                ; ee
    mov ax, 00438h                            ; b8 38 04
    call 0839ch                               ; e8 dd fa
    mov dx, strict word 00054h                ; ba 54 00
    mov ax, 01000h                            ; b8 00 10
    call 0881eh                               ; e8 56 ff
    mov sp, bp                                ; 89 ec
    pop bp                                    ; 5d
    retn                                      ; c3
ahci_ctrl_extract_bits_:                     ; 0xf88cc LB 0x1b
    push si                                   ; 56
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov si, bx                                ; 89 de
    and ax, bx                                ; 21 d8
    and dx, cx                                ; 21 ca
    movzx cx, byte [bp+006h]                  ; 0f b6 4e 06
    jcxz 088e2h                               ; e3 06
    shr dx, 1                                 ; d1 ea
    rcr ax, 1                                 ; d1 d8
    loop 088dch                               ; e2 fa
    pop bp                                    ; 5d
    pop si                                    ; 5e
    retn 00002h                               ; c2 02 00
ahci_addr_to_phys_:                          ; 0xf88e7 LB 0x1e
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
    loop 088f5h                               ; e2 fa
    xor cx, cx                                ; 31 c9
    add ax, bx                                ; 01 d8
    adc dx, cx                                ; 11 ca
    pop bp                                    ; 5d
    pop cx                                    ; 59
    pop bx                                    ; 5b
    retn                                      ; c3
ahci_port_cmd_sync_:                         ; 0xf8905 LB 0x14b
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push cx                                   ; 51
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    mov si, ax                                ; 89 c6
    mov cx, dx                                ; 89 d1
    mov al, bl                                ; 88 d8
    mov es, dx                                ; 8e c2
    mov ah, byte [es:si+00262h]               ; 26 8a a4 62 02
    mov byte [bp-008h], ah                    ; 88 66 f8
    mov bx, word [es:si+00260h]               ; 26 8b 9c 60 02
    cmp ah, 0ffh                              ; 80 fc ff
    je near 08a48h                            ; 0f 84 20 01
    movzx dx, byte [es:si+00263h]             ; 26 0f b6 94 63 02
    xor di, di                                ; 31 ff
    or di, 00080h                             ; 81 cf 80 00
    xor ah, ah                                ; 30 e4
    or di, ax                                 ; 09 c7
    mov word [es:si], di                      ; 26 89 3c
    mov word [es:si+002h], dx                 ; 26 89 54 02
    db  066h, 026h, 0c7h, 044h, 004h, 000h, 000h, 000h, 000h
    ; mov dword [es:si+004h], strict dword 000000000h ; 66 26 c7 44 04 00 00 00 00
    lea ax, [si+00080h]                       ; 8d 84 80 00
    mov dx, cx                                ; 89 ca
    call 088e7h                               ; e8 96 ff
    mov es, cx                                ; 8e c1
    mov word [es:si+008h], ax                 ; 26 89 44 08
    mov word [es:si+00ah], dx                 ; 26 89 54 0a
    movzx di, byte [bp-008h]                  ; 0f b6 7e f8
    sal di, 007h                              ; c1 e7 07
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
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    sal ax, 007h                              ; c1 e0 07
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
    jne short 089deh                          ; 75 04
    test AL, strict byte 001h                 ; a8 01
    je short 089e2h                           ; 74 04
    mov AL, strict byte 001h                  ; b0 01
    jmp short 089e4h                          ; eb 02
    xor al, al                                ; 30 c0
    test al, al                               ; 84 c0
    je short 089b2h                           ; 74 ca
    movzx di, byte [bp-008h]                  ; 0f b6 7e f8
    sal di, 007h                              ; c1 e7 07
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
ahci_cmd_data_:                              ; 0xf8a50 LB 0x262
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
    mov word [bp-012h], ax                    ; 89 46 ee
    mov ax, word [es:bx+010h]                 ; 26 8b 47 10
    mov word [bp-010h], ax                    ; 89 46 f0
    mov cx, strict word 00040h                ; b9 40 00
    xor bx, bx                                ; 31 db
    mov ax, 00080h                            ; b8 80 00
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    call 0a130h                               ; e8 a2 16
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
    call 0a110h                               ; e8 3e 16
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
    call 0a110h                               ; e8 18 16
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
    call 0a110h                               ; e8 ec 15
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
    call 0a110h                               ; e8 c6 15
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
    call 0a110h                               ; e8 a0 15
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov byte [es:di+0008ah], dl               ; 26 88 95 8a 00
    mov byte [es:di+0008bh], 000h             ; 26 c6 85 8b 00 00
    mov al, byte [bp-012h]                    ; 8a 46 ee
    mov byte [es:di+0008ch], al               ; 26 88 85 8c 00
    mov ax, word [bp-012h]                    ; 8b 46 ee
    shr ax, 008h                              ; c1 e8 08
    mov byte [es:di+0008dh], al               ; 26 88 85 8d 00
    mov word [es:di+00276h], strict word 00010h ; 26 c7 85 76 02 10 00
    mov ax, word [bp-012h]                    ; 8b 46 ee
    xor dx, dx                                ; 31 d2
    mov bx, word [bp-010h]                    ; 8b 5e f0
    xor cx, cx                                ; 31 c9
    call 0a090h                               ; e8 eb 14
    push dx                                   ; 52
    push ax                                   ; 50
    mov es, [bp-016h]                         ; 8e 46 ea
    mov bx, word [bp-014h]                    ; 8b 5e ec
    mov bx, word [es:bx+008h]                 ; 26 8b 5f 08
    mov si, word [bp-014h]                    ; 8b 76 ec
    mov cx, word [es:si+00ah]                 ; 26 8b 4c 0a
    mov ax, 0026ah                            ; b8 6a 02
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    call 09fd8h                               ; e8 17 14
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov ax, word [es:di+0027eh]               ; 26 8b 85 7e 02
    add ax, strict word 0ffffh                ; 05 ff ff
    mov dx, word [es:di+00280h]               ; 26 8b 95 80 02
    adc dx, strict byte 0ffffh                ; 83 d2 ff
    movzx bx, byte [es:di+00263h]             ; 26 0f b6 9d 63 02
    sal bx, 004h                              ; c1 e3 04
    mov word [es:bx+0010ch], ax               ; 26 89 87 0c 01
    mov word [es:bx+0010eh], dx               ; 26 89 97 0e 01
    movzx bx, byte [es:di+00263h]             ; 26 0f b6 9d 63 02
    sal bx, 004h                              ; c1 e3 04
    mov ax, word [es:di+0027ah]               ; 26 8b 85 7a 02
    mov dx, word [es:di+0027ch]               ; 26 8b 95 7c 02
    mov word [es:bx+00100h], ax               ; 26 89 87 00 01
    mov word [es:bx+00102h], dx               ; 26 89 97 02 01
    inc byte [es:di+00263h]                   ; 26 fe 85 63 02
    mov es, [bp-016h]                         ; 8e 46 ea
    mov bx, si                                ; 89 f3
    mov ax, word [es:bx+020h]                 ; 26 8b 47 20
    test ax, ax                               ; 85 c0
    je short 08c4fh                           ; 74 39
    dec ax                                    ; 48
    mov es, [bp-00ah]                         ; 8e 46 f6
    movzx bx, byte [es:di+00263h]             ; 26 0f b6 9d 63 02
    sal bx, 004h                              ; c1 e3 04
    mov word [es:bx+0010ch], ax               ; 26 89 87 0c 01
    mov word [es:bx+0010eh], di               ; 26 89 bf 0e 01
    movzx bx, byte [es:di+00263h]             ; 26 0f b6 9d 63 02
    sal bx, 004h                              ; c1 e3 04
    mov dx, word [es:di+00264h]               ; 26 8b 95 64 02
    mov ax, word [es:di+00266h]               ; 26 8b 85 66 02
    mov word [es:bx+00100h], dx               ; 26 89 97 00 01
    mov word [es:bx+00102h], ax               ; 26 89 87 02 01
    inc byte [es:di+00263h]                   ; 26 fe 85 63 02
    mov al, byte [bp-008h]                    ; 8a 46 f8
    cmp AL, strict byte 035h                  ; 3c 35
    jne short 08c5ch                          ; 75 06
    mov byte [bp-008h], 040h                  ; c6 46 f8 40
    jmp short 08c73h                          ; eb 17
    cmp AL, strict byte 0a0h                  ; 3c a0
    jne short 08c6fh                          ; 75 0f
    or byte [bp-008h], 020h                   ; 80 4e f8 20
    les bx, [bp-00eh]                         ; c4 5e f2
    or byte [es:bx+00083h], 001h              ; 26 80 8f 83 00 01
    jmp short 08c73h                          ; eb 04
    mov byte [bp-008h], 000h                  ; c6 46 f8 00
    or byte [bp-008h], 005h                   ; 80 4e f8 05
    movzx bx, byte [bp-008h]                  ; 0f b6 5e f8
    mov ax, word [bp-00eh]                    ; 8b 46 f2
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    call 08905h                               ; e8 81 fc
    mov cx, word [bp-00ch]                    ; 8b 4e f4
    mov bx, word [bp-00eh]                    ; 8b 5e f2
    add bx, 00240h                            ; 81 c3 40 02
    mov ax, word [bp-00eh]                    ; 8b 46 f2
    add ax, 0026ah                            ; 05 6a 02
    mov dx, cx                                ; 89 ca
    call 0a051h                               ; e8 b8 13
    mov es, cx                                ; 8e c1
    mov al, byte [es:bx+003h]                 ; 26 8a 47 03
    test al, al                               ; 84 c0
    je short 08ca8h                           ; 74 05
    mov ax, strict word 00004h                ; b8 04 00
    jmp short 08caah                          ; eb 02
    xor ah, ah                                ; 30 e4
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bp                                    ; 5d
    retn                                      ; c3
ahci_port_deinit_current_:                   ; 0xf8cb2 LB 0x17f
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00006h                ; 83 ec 06
    mov di, ax                                ; 89 c7
    mov word [bp-00ch], dx                    ; 89 56 f4
    mov es, dx                                ; 8e c2
    mov si, word [es:di+00260h]               ; 26 8b b5 60 02
    mov al, byte [es:di+00262h]               ; 26 8a 85 62 02
    mov byte [bp-00ah], al                    ; 88 46 f6
    cmp AL, strict byte 0ffh                  ; 3c ff
    je near 08e28h                            ; 0f 84 52 01
    movzx ax, byte [bp-00ah]                  ; 0f b6 46 f6
    sal ax, 007h                              ; c1 e0 07
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
    movzx ax, byte [bp-00ah]                  ; 0f b6 46 f6
    sal ax, 007h                              ; c1 e0 07
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
    je short 08d36h                           ; 74 04
    mov AL, strict byte 001h                  ; b0 01
    jmp short 08d38h                          ; eb 02
    xor al, al                                ; 30 c0
    cmp AL, strict byte 001h                  ; 3c 01
    je short 08d0ah                           ; 74 ce
    mov cx, strict word 00020h                ; b9 20 00
    xor bx, bx                                ; 31 db
    mov ax, di                                ; 89 f8
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    call 0a130h                               ; e8 e7 13
    lea ax, [di+00080h]                       ; 8d 85 80 00
    mov cx, strict word 00040h                ; b9 40 00
    xor bx, bx                                ; 31 db
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    call 0a130h                               ; e8 d8 13
    lea ax, [di+00200h]                       ; 8d 85 00 02
    mov cx, strict word 00060h                ; b9 60 00
    xor bx, bx                                ; 31 db
    mov dx, word [bp-00ch]                    ; 8b 56 f4
    call 0a130h                               ; e8 c9 13
    movzx ax, byte [bp-00ah]                  ; 0f b6 46 f6
    sal ax, 007h                              ; c1 e0 07
    mov word [bp-00eh], ax                    ; 89 46 f2
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
    mov ax, word [bp-00eh]                    ; 8b 46 f2
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
    mov ax, word [bp-00eh]                    ; 8b 46 f2
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
    mov ax, word [bp-00eh]                    ; 8b 46 f2
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
    mov ax, word [bp-00eh]                    ; 8b 46 f2
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
    mov es, [bp-00ch]                         ; 8e 46 f4
    mov byte [es:di+00262h], 0ffh             ; 26 c6 85 62 02 ff
    lea sp, [bp-008h]                         ; 8d 66 f8
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
ahci_port_init_:                             ; 0xf8e31 LB 0x24a
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push cx                                   ; 51
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00006h                ; 83 ec 06
    mov si, ax                                ; 89 c6
    mov word [bp-00ah], dx                    ; 89 56 f6
    mov byte [bp-008h], bl                    ; 88 5e f8
    call 08cb2h                               ; e8 6d fe
    movzx ax, bl                              ; 0f b6 c3
    sal ax, 007h                              ; c1 e0 07
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
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    sal ax, 007h                              ; c1 e0 07
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
    je short 08eb4h                           ; 74 04
    mov AL, strict byte 001h                  ; b0 01
    jmp short 08eb6h                          ; eb 02
    xor al, al                                ; 30 c0
    cmp AL, strict byte 001h                  ; 3c 01
    je short 08e80h                           ; 74 c6
    mov cx, strict word 00020h                ; b9 20 00
    xor bx, bx                                ; 31 db
    mov ax, si                                ; 89 f0
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    call 0a130h                               ; e8 69 12
    lea ax, [si+00080h]                       ; 8d 84 80 00
    mov cx, strict word 00040h                ; b9 40 00
    xor bx, bx                                ; 31 db
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    call 0a130h                               ; e8 5a 12
    lea di, [si+00200h]                       ; 8d bc 00 02
    mov cx, strict word 00060h                ; b9 60 00
    xor bx, bx                                ; 31 db
    mov ax, di                                ; 89 f8
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    call 0a130h                               ; e8 49 12
    movzx bx, byte [bp-008h]                  ; 0f b6 5e f8
    sal bx, 007h                              ; c1 e3 07
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
    call 088e7h                               ; e8 d3 f9
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
    call 088e7h                               ; e8 67 f9
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
@ahci_read_sectors:                          ; 0xf907b LB 0xa6
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    push ax                                   ; 50
    les di, [bp+004h]                         ; c4 7e 04
    movzx di, byte [es:di+00ch]               ; 26 0f b6 7d 0c
    sub di, strict byte 0000ch                ; 83 ef 0c
    cmp di, strict byte 00004h                ; 83 ff 04
    jbe short 090a0h                          ; 76 0f
    push di                                   ; 57
    push 00c74h                               ; 68 74 0c
    push 00c86h                               ; 68 86 0c
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 ce 89
    add sp, strict byte 00008h                ; 83 c4 08
    xor bx, bx                                ; 31 db
    les si, [bp+004h]                         ; c4 76 04
    mov dx, word [es:si+00232h]               ; 26 8b 94 32 02
    shr eax, 010h                             ; 66 c1 e8 10
    mov es, dx                                ; 8e c2
    mov word [es:bx+00268h], ax               ; 26 89 87 68 02
    mov es, [bp+006h]                         ; 8e 46 06
    add di, si                                ; 01 f7
    movzx bx, byte [es:di+0022dh]             ; 26 0f b6 9d 2d 02
    mov di, si                                ; 89 f7
    mov dx, word [es:di+00232h]               ; 26 8b 95 32 02
    xor ax, ax                                ; 31 c0
    call 08e31h                               ; e8 65 fd
    mov bx, strict word 00025h                ; bb 25 00
    mov ax, si                                ; 89 f0
    mov dx, word [bp+006h]                    ; 8b 56 06
    call 08a50h                               ; e8 79 f9
    mov word [bp-006h], ax                    ; 89 46 fa
    mov es, [bp+006h]                         ; 8e 46 06
    mov bx, si                                ; 89 f3
    mov ax, word [es:bx+00eh]                 ; 26 8b 47 0e
    mov word [es:bx+018h], ax                 ; 26 89 47 18
    mov cx, ax                                ; 89 c1
    sal cx, 009h                              ; c1 e1 09
    shr cx, 1                                 ; d1 e9
    mov di, word [es:di+008h]                 ; 26 8b 7d 08
    mov ax, word [es:bx+00ah]                 ; 26 8b 47 0a
    mov si, di                                ; 89 fe
    mov dx, ax                                ; 89 c2
    mov es, ax                                ; 8e c0
    push DS                                   ; 1e
    mov ds, dx                                ; 8e da
    rep movsw                                 ; f3 a5
    pop DS                                    ; 1f
    xor bx, bx                                ; 31 db
    les di, [bp+004h]                         ; c4 7e 04
    mov es, [es:di+00232h]                    ; 26 8e 85 32 02
    mov ax, word [es:bx+00268h]               ; 26 8b 87 68 02
    sal eax, 010h                             ; 66 c1 e0 10
    mov ax, word [bp-006h]                    ; 8b 46 fa
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 00004h                               ; c2 04 00
@ahci_write_sectors:                         ; 0xf9121 LB 0x84
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    mov si, word [bp+004h]                    ; 8b 76 04
    mov cx, word [bp+006h]                    ; 8b 4e 06
    mov es, cx                                ; 8e c1
    movzx dx, byte [es:si+00ch]               ; 26 0f b6 54 0c
    sub dx, strict byte 0000ch                ; 83 ea 0c
    cmp dx, strict byte 00004h                ; 83 fa 04
    jbe short 0914ah                          ; 76 0f
    push dx                                   ; 52
    push 00ca5h                               ; 68 a5 0c
    push 00c86h                               ; 68 86 0c
    push strict byte 00007h                   ; 6a 07
    call 01a6bh                               ; e8 24 89
    add sp, strict byte 00008h                ; 83 c4 08
    xor bx, bx                                ; 31 db
    mov es, cx                                ; 8e c1
    mov di, word [es:si+00232h]               ; 26 8b bc 32 02
    shr eax, 010h                             ; 66 c1 e8 10
    mov es, di                                ; 8e c7
    mov word [es:bx+00268h], ax               ; 26 89 87 68 02
    mov es, cx                                ; 8e c1
    mov bx, si                                ; 89 f3
    add bx, dx                                ; 01 d3
    movzx bx, byte [es:bx+0022dh]             ; 26 0f b6 9f 2d 02
    mov dx, word [es:si+00232h]               ; 26 8b 94 32 02
    xor ax, ax                                ; 31 c0
    call 08e31h                               ; e8 bd fc
    mov bx, strict word 00035h                ; bb 35 00
    mov ax, si                                ; 89 f0
    mov dx, cx                                ; 89 ca
    call 08a50h                               ; e8 d2 f8
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
ahci_cmd_packet_:                            ; 0xf91a5 LB 0x183
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
    call 01765h                               ; e8 a5 85
    mov si, 00122h                            ; be 22 01
    mov word [bp-008h], ax                    ; 89 46 f8
    cmp byte [bp+00ah], 002h                  ; 80 7e 0a 02
    jne short 091ebh                          ; 75 1f
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 53 88
    push 00cb8h                               ; 68 b8 0c
    push 00cc8h                               ; 68 c8 0c
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 89 88
    add sp, strict byte 00006h                ; 83 c4 06
    mov ax, strict word 00001h                ; b8 01 00
    jmp near 0931fh                           ; e9 34 01
    test byte [bp+004h], 001h                 ; f6 46 04 01
    jne short 091e5h                          ; 75 f4
    mov ax, word [bp+006h]                    ; 8b 46 06
    mov dx, word [bp+008h]                    ; 8b 56 08
    mov cx, strict word 00008h                ; b9 08 00
    sal ax, 1                                 ; d1 e0
    rcl dx, 1                                 ; d1 d2
    loop 091fah                               ; e2 fa
    mov es, [bp-008h]                         ; 8e 46 f8
    mov word [es:si], ax                      ; 26 89 04
    mov word [es:si+002h], dx                 ; 26 89 54 02
    db  066h, 026h, 0c7h, 044h, 004h, 000h, 000h, 000h, 000h
    ; mov dword [es:si+004h], strict dword 000000000h ; 66 26 c7 44 04 00 00 00 00
    mov ax, word [bp+00ch]                    ; 8b 46 0c
    mov word [es:si+008h], ax                 ; 26 89 44 08
    mov ax, word [bp+00eh]                    ; 8b 46 0e
    mov word [es:si+00ah], ax                 ; 26 89 44 0a
    mov bx, word [es:si+010h]                 ; 26 8b 5c 10
    mov ax, word [bp+006h]                    ; 8b 46 06
    mov dx, word [bp+008h]                    ; 8b 56 08
    xor cx, cx                                ; 31 c9
    call 0a0d0h                               ; e8 a0 0e
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
    movzx bx, byte [es:bx+0022dh]             ; 26 0f b6 9f 2d 02
    mov dx, word [es:si+00232h]               ; 26 8b 94 32 02
    xor ax, ax                                ; 31 c0
    call 08e31h                               ; e8 c5 fb
    movzx ax, byte [bp-006h]                  ; 0f b6 46 fa
    push ax                                   ; 50
    mov bx, word [bp-012h]                    ; 8b 5e ee
    mov cx, word [bp-010h]                    ; 8b 4e f0
    mov ax, 000c0h                            ; b8 c0 00
    mov dx, word [bp-00ah]                    ; 8b 56 f6
    call 0a140h                               ; e8 c0 0e
    mov es, [bp-008h]                         ; 8e 46 f8
    mov word [es:si+018h], di                 ; 26 89 7c 18
    mov word [es:si+01ah], di                 ; 26 89 7c 1a
    mov word [es:si+01ch], di                 ; 26 89 7c 1c
    mov ax, word [es:si+01eh]                 ; 26 8b 44 1e
    test ax, ax                               ; 85 c0
    je short 092beh                           ; 74 27
    dec ax                                    ; 48
    mov es, [bp-00ah]                         ; 8e 46 f6
    mov word [es:di+0010ch], ax               ; 26 89 85 0c 01
    mov word [es:di+0010eh], di               ; 26 89 bd 0e 01
    mov dx, word [es:di+00264h]               ; 26 8b 95 64 02
    mov ax, word [es:di+00266h]               ; 26 8b 85 66 02
    mov word [es:di+00100h], dx               ; 26 89 95 00 01
    mov word [es:di+00102h], ax               ; 26 89 85 02 01
    inc byte [es:di+00263h]                   ; 26 fe 85 63 02
    mov bx, 000a0h                            ; bb a0 00
    mov ax, si                                ; 89 f0
    mov dx, word [bp-008h]                    ; 8b 56 f8
    call 08a50h                               ; e8 87 f7
    les bx, [bp-00eh]                         ; c4 5e f2
    mov ax, word [es:bx+004h]                 ; 26 8b 47 04
    mov dx, word [es:bx+006h]                 ; 26 8b 57 06
    mov es, [bp-008h]                         ; 8e 46 f8
    mov word [es:si+01ah], ax                 ; 26 89 44 1a
    mov word [es:si+01ch], dx                 ; 26 89 54 1c
    mov bx, word [es:si+01ah]                 ; 26 8b 5c 1a
    mov cx, dx                                ; 89 d1
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
    jne short 0931dh                          ; 75 05
    mov ax, strict word 00004h                ; b8 04 00
    jmp short 0931fh                          ; eb 02
    xor ax, ax                                ; 31 c0
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn 0000ch                               ; c2 0c 00
wait_ticks_device_init_:                     ; 0xf9328 LB 0x5
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    pop bp                                    ; 5d
    retn                                      ; c3
ahci_port_detect_device_:                    ; 0xf932d LB 0x4b8
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push cx                                   ; 51
    push si                                   ; 56
    push di                                   ; 57
    sub sp, 0022ch                            ; 81 ec 2c 02
    mov si, ax                                ; 89 c6
    mov word [bp-02ah], dx                    ; 89 56 d6
    mov byte [bp-008h], bl                    ; 88 5e f8
    mov word [bp-026h], strict word 00000h    ; c7 46 da 00 00
    movzx di, bl                              ; 0f b6 fb
    mov bx, di                                ; 89 fb
    call 08e31h                               ; e8 e5 fa
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01765h                               ; e8 10 84
    mov word [bp-012h], 00122h                ; c7 46 ee 22 01
    mov word [bp-010h], ax                    ; 89 46 f0
    mov ax, di                                ; 89 f8
    sal ax, 007h                              ; c1 e0 07
    add ax, 0012ch                            ; 05 2c 01
    cwd                                       ; 99
    mov di, ax                                ; 89 c7
    mov bx, dx                                ; 89 d3
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov dx, word [es:si+00260h]               ; 26 8b 94 60 02
    mov cx, bx                                ; 89 d9
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov dx, word [es:si+00260h]               ; 26 8b 94 60 02
    add dx, strict byte 00004h                ; 83 c2 04
    mov ax, strict word 00001h                ; b8 01 00
    xor cx, cx                                ; 31 c9
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov dx, word [es:si+00260h]               ; 26 8b 94 60 02
    mov ax, di                                ; 89 f8
    mov cx, bx                                ; 89 d9
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov es, [bp-02ah]                         ; 8e 46 d6
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
    mov word [bp-024h], ax                    ; 89 46 dc
    mov ax, word [es:di+002h]                 ; 26 8b 45 02
    adc ax, strict word 00000h                ; 15 00 00
    mov word [bp-01ch], ax                    ; 89 46 e4
    mov es, [bp-014h]                         ; 8e 46 ec
    mov ax, word [es:di+002h]                 ; 26 8b 45 02
    cmp ax, word [bp-01ch]                    ; 3b 46 e4
    jc short 093feh                           ; 72 11
    mov ax, word [es:di+002h]                 ; 26 8b 45 02
    cmp ax, word [bp-01ch]                    ; 3b 46 e4
    jne short 09449h                          ; 75 53
    mov ax, word [es:di]                      ; 26 8b 05
    cmp ax, word [bp-024h]                    ; 3b 46 dc
    jnc short 09449h                          ; 73 4b
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    sal ax, 007h                              ; c1 e0 07
    add ax, 00128h                            ; 05 28 01
    cwd                                       ; 99
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov bx, word [es:si+00260h]               ; 26 8b 9c 60 02
    mov cx, dx                                ; 89 d1
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov dx, word [es:si+00260h]               ; 26 8b 94 60 02
    add dx, strict byte 00004h                ; 83 c2 04
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    mov word [bp-028h], ax                    ; 89 46 d8
    mov word [bp-022h], dx                    ; 89 56 de
    push strict byte 00000h                   ; 6a 00
    mov bx, strict word 0000fh                ; bb 0f 00
    xor cx, cx                                ; 31 c9
    call 088cch                               ; e8 8a f4
    cmp ax, strict word 00003h                ; 3d 03 00
    jne short 093e1h                          ; 75 9a
    jmp short 09451h                          ; eb 08
    cmp word [bp-026h], strict byte 00000h    ; 83 7e da 00
    je near 097ddh                            ; 0f 84 8c 03
    push strict byte 00000h                   ; 6a 00
    mov bx, strict word 0000fh                ; bb 0f 00
    xor cx, cx                                ; 31 c9
    mov ax, word [bp-028h]                    ; 8b 46 d8
    mov dx, word [bp-022h]                    ; 8b 56 de
    call 088cch                               ; e8 6b f4
    cmp ax, strict word 00003h                ; 3d 03 00
    jne near 097ddh                           ; 0f 85 75 03
    movzx di, byte [bp-008h]                  ; 0f b6 7e f8
    sal di, 007h                              ; c1 e7 07
    lea ax, [di+00130h]                       ; 8d 85 30 01
    cwd                                       ; 99
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov bx, word [es:si+00260h]               ; 26 8b 9c 60 02
    mov cx, dx                                ; 89 d1
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov dx, word [es:si+00260h]               ; 26 8b 94 60 02
    add dx, strict byte 00004h                ; 83 c2 04
    mov ax, strict word 0ffffh                ; b8 ff ff
    mov cx, ax                                ; 89 c1
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    les bx, [bp-012h]                         ; c4 5e ee
    mov al, byte [es:bx+00231h]               ; 26 8a 87 31 02
    mov byte [bp-00ah], al                    ; 88 46 f6
    cmp AL, strict byte 004h                  ; 3c 04
    jnc near 097ddh                           ; 0f 83 2a 03
    lea ax, [di+00118h]                       ; 8d 85 18 01
    mov es, [bp-02ah]                         ; 8e 46 d6
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
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov bx, word [es:si+00260h]               ; 26 8b 9c 60 02
    mov cx, dx                                ; 89 d1
    mov dx, bx                                ; 89 da
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    mov es, [bp-02ah]                         ; 8e 46 d6
    mov dx, word [es:si+00260h]               ; 26 8b 94 60 02
    add dx, strict byte 00004h                ; 83 c2 04
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    mov bx, ax                                ; 89 c3
    mov ax, dx                                ; 89 d0
    mov dl, byte [bp-00ah]                    ; 8a 56 f6
    add dl, 00ch                              ; 80 c2 0c
    mov byte [bp-00eh], dl                    ; 88 56 f2
    test ax, ax                               ; 85 c0
    jne near 0972fh                           ; 0f 85 05 02
    cmp bx, 00101h                            ; 81 fb 01 01
    jne near 0972fh                           ; 0f 85 fd 01
    les bx, [bp-012h]                         ; c4 5e ee
    db  066h, 026h, 0c7h, 047h, 004h, 000h, 000h, 000h, 000h
    ; mov dword [es:bx+004h], strict dword 000000000h ; 66 26 c7 47 04 00 00 00 00
    db  066h, 026h, 0c7h, 007h, 000h, 000h, 000h, 000h
    ; mov dword [es:bx], strict dword 000000000h ; 66 26 c7 07 00 00 00 00
    lea dx, [bp-00232h]                       ; 8d 96 ce fd
    mov word [es:bx+008h], dx                 ; 26 89 57 08
    mov [es:bx+00ah], ss                      ; 26 8c 57 0a
    db  066h, 026h, 0c7h, 047h, 00eh, 001h, 000h, 000h, 002h
    ; mov dword [es:bx+00eh], strict dword 002000001h ; 66 26 c7 47 0e 01 00 00 02
    mov bx, 000ech                            ; bb ec 00
    mov ax, word [bp-012h]                    ; 8b 46 ee
    mov dx, es                                ; 8c c2
    call 08a50h                               ; e8 ea f4
    mov al, byte [bp-00eh]                    ; 8a 46 f2
    mov byte [bp-00ch], al                    ; 88 46 f4
    test byte [bp-00232h], 080h               ; f6 86 ce fd 80
    db  00fh, 095h, 0c0h
    ; setne al                                  ; 0f 95 c0
    xor ah, ah                                ; 30 e4
    mov dx, word [bp-00230h]                  ; 8b 96 d0 fd
    mov word [bp-01eh], dx                    ; 89 56 e2
    mov dx, word [bp-0022ch]                  ; 8b 96 d4 fd
    mov word [bp-020h], dx                    ; 89 56 e0
    mov dx, word [bp-00226h]                  ; 8b 96 da fd
    mov word [bp-01ah], dx                    ; 89 56 e6
    mov di, word [bp-001bah]                  ; 8b be 46 fe
    mov si, word [bp-001b8h]                  ; 8b b6 48 fe
    mov dword [bp-018h], strict dword 000000000h ; 66 c7 46 e8 00 00 00 00
    cmp si, 00fffh                            ; 81 fe ff 0f
    jne short 095bch                          ; 75 1b
    cmp di, strict byte 0ffffh                ; 83 ff ff
    jne short 095bch                          ; 75 16
    mov dx, word [bp-00164h]                  ; 8b 96 9c fe
    mov word [bp-016h], dx                    ; 89 56 ea
    mov dx, word [bp-00166h]                  ; 8b 96 9a fe
    mov word [bp-018h], dx                    ; 89 56 e8
    mov si, word [bp-00168h]                  ; 8b b6 98 fe
    mov di, word [bp-0016ah]                  ; 8b be 96 fe
    movzx bx, byte [bp-00ah]                  ; 0f b6 5e f6
    mov es, [bp-010h]                         ; 8e 46 f0
    add bx, word [bp-012h]                    ; 03 5e ee
    mov ah, byte [bp-008h]                    ; 8a 66 f8
    mov byte [es:bx+0022dh], ah               ; 26 88 a7 2d 02
    movzx dx, byte [bp-00ch]                  ; 0f b6 56 f4
    imul dx, dx, strict byte 0001ch           ; 6b d2 1c
    mov bx, word [bp-012h]                    ; 8b 5e ee
    add bx, dx                                ; 01 d3
    mov word [es:bx+022h], 0ff05h             ; 26 c7 47 22 05 ff
    mov byte [es:bx+024h], al                 ; 26 88 47 24
    mov byte [es:bx+025h], 000h               ; 26 c6 47 25 00
    mov word [es:bx+028h], 00200h             ; 26 c7 47 28 00 02
    mov byte [es:bx+027h], 001h               ; 26 c6 47 27 01
    mov ax, word [bp-016h]                    ; 8b 46 ea
    mov word [es:bx+03ch], ax                 ; 26 89 47 3c
    mov ax, word [bp-018h]                    ; 8b 46 e8
    mov word [es:bx+03ah], ax                 ; 26 89 47 3a
    mov word [es:bx+038h], si                 ; 26 89 77 38
    mov word [es:bx+036h], di                 ; 26 89 7f 36
    mov ax, word [bp-020h]                    ; 8b 46 e0
    mov word [es:bx+030h], ax                 ; 26 89 47 30
    mov ax, word [bp-01eh]                    ; 8b 46 e2
    mov word [es:bx+032h], ax                 ; 26 89 47 32
    mov ax, word [bp-01ah]                    ; 8b 46 e6
    mov word [es:bx+034h], ax                 ; 26 89 47 34
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    cmp AL, strict byte 001h                  ; 3c 01
    jc short 09632h                           ; 72 0c
    jbe short 0963ah                          ; 76 12
    cmp AL, strict byte 003h                  ; 3c 03
    je short 09642h                           ; 74 16
    cmp AL, strict byte 002h                  ; 3c 02
    je short 0963eh                           ; 74 0e
    jmp short 0968fh                          ; eb 5d
    test al, al                               ; 84 c0
    jne short 0968fh                          ; 75 59
    mov BL, strict byte 040h                  ; b3 40
    jmp short 09644h                          ; eb 0a
    mov BL, strict byte 048h                  ; b3 48
    jmp short 09644h                          ; eb 06
    mov BL, strict byte 050h                  ; b3 50
    jmp short 09644h                          ; eb 02
    mov BL, strict byte 058h                  ; b3 58
    mov al, bl                                ; 88 d8
    add AL, strict byte 007h                  ; 04 07
    movzx dx, al                              ; 0f b6 d0
    mov ax, dx                                ; 89 d0
    call 017a5h                               ; e8 55 81
    test al, al                               ; 84 c0
    je short 0968fh                           ; 74 3b
    mov al, bl                                ; 88 d8
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    xor ah, ah                                ; 30 e4
    call 017a5h                               ; e8 48 81
    xor ah, ah                                ; 30 e4
    sal ax, 008h                              ; c1 e0 08
    mov word [bp-02ch], ax                    ; 89 46 d4
    movzx ax, bl                              ; 0f b6 c3
    call 017a5h                               ; e8 3a 81
    xor ah, ah                                ; 30 e4
    mov cx, word [bp-02ch]                    ; 8b 4e d4
    add cx, ax                                ; 01 c1
    mov word [bp-030h], cx                    ; 89 4e d0
    mov al, bl                                ; 88 d8
    add AL, strict byte 002h                  ; 04 02
    xor ah, ah                                ; 30 e4
    call 017a5h                               ; e8 27 81
    xor ah, ah                                ; 30 e4
    mov word [bp-032h], ax                    ; 89 46 ce
    mov ax, dx                                ; 89 d0
    call 017a5h                               ; e8 1d 81
    xor ah, ah                                ; 30 e4
    mov word [bp-02eh], ax                    ; 89 46 d2
    jmp short 0969dh                          ; eb 0e
    push dword [bp-018h]                      ; 66 ff 76 e8
    push si                                   ; 56
    push di                                   ; 57
    mov dx, ss                                ; 8c d2
    lea ax, [bp-032h]                         ; 8d 46 ce
    call 05aeeh                               ; e8 51 c4
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 82 83
    push dword [bp-018h]                      ; 66 ff 76 e8
    push si                                   ; 56
    push di                                   ; 57
    mov ax, word [bp-02eh]                    ; 8b 46 d2
    push ax                                   ; 50
    mov ax, word [bp-032h]                    ; 8b 46 ce
    push ax                                   ; 50
    mov ax, word [bp-030h]                    ; 8b 46 d0
    push ax                                   ; 50
    push word [bp-01ah]                       ; ff 76 e6
    push word [bp-020h]                       ; ff 76 e0
    push word [bp-01eh]                       ; ff 76 e2
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    push ax                                   ; 50
    movzx ax, byte [bp-00ah]                  ; 0f b6 46 f6
    push ax                                   ; 50
    push 00ce8h                               ; 68 e8 0c
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 96 83
    add sp, strict byte 0001ch                ; 83 c4 1c
    movzx di, byte [bp-00ch]                  ; 0f b6 7e f4
    imul di, di, strict byte 0001ch           ; 6b ff 1c
    add di, word [bp-012h]                    ; 03 7e ee
    mov es, [bp-010h]                         ; 8e 46 f0
    lea di, [di+02ah]                         ; 8d 7d 2a
    push DS                                   ; 1e
    push SS                                   ; 16
    pop DS                                    ; 1f
    lea si, [bp-032h]                         ; 8d 76 ce
    movsw                                     ; a5
    movsw                                     ; a5
    movsw                                     ; a5
    pop DS                                    ; 1f
    mov bx, word [bp-012h]                    ; 8b 5e ee
    mov al, byte [es:bx+001e2h]               ; 26 8a 87 e2 01
    mov ah, byte [bp-00ah]                    ; 8a 66 f6
    add ah, 00ch                              ; 80 c4 0c
    movzx bx, al                              ; 0f b6 d8
    add bx, word [bp-012h]                    ; 03 5e ee
    mov byte [es:bx+001e3h], ah               ; 26 88 a7 e3 01
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    mov bx, word [bp-012h]                    ; 8b 5e ee
    mov byte [es:bx+001e2h], al               ; 26 88 87 e2 01
    mov dx, strict word 00075h                ; ba 75 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01749h                               ; e8 2b 80
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    movzx bx, al                              ; 0f b6 d8
    mov dx, strict word 00075h                ; ba 75 00
    mov ax, strict word 00040h                ; b8 40 00
    call 01757h                               ; e8 2b 80
    jmp near 097cfh                           ; e9 a0 00
    cmp ax, 0eb14h                            ; 3d 14 eb
    jne near 097cfh                           ; 0f 85 99 00
    cmp bx, 00101h                            ; 81 fb 01 01
    jne near 097cfh                           ; 0f 85 91 00
    les bx, [bp-012h]                         ; c4 5e ee
    db  066h, 026h, 0c7h, 047h, 004h, 000h, 000h, 000h, 000h
    ; mov dword [es:bx+004h], strict dword 000000000h ; 66 26 c7 47 04 00 00 00 00
    db  066h, 026h, 0c7h, 007h, 000h, 000h, 000h, 000h
    ; mov dword [es:bx], strict dword 000000000h ; 66 26 c7 07 00 00 00 00
    lea dx, [bp-00232h]                       ; 8d 96 ce fd
    mov word [es:bx+008h], dx                 ; 26 89 57 08
    mov [es:bx+00ah], ss                      ; 26 8c 57 0a
    db  066h, 026h, 0c7h, 047h, 00eh, 001h, 000h, 000h, 002h
    ; mov dword [es:bx+00eh], strict dword 002000001h ; 66 26 c7 47 0e 01 00 00 02
    mov bx, 000a1h                            ; bb a1 00
    mov ax, word [bp-012h]                    ; 8b 46 ee
    mov dx, es                                ; 8c c2
    call 08a50h                               ; e8 de f2
    test byte [bp-00232h], 080h               ; f6 86 ce fd 80
    db  00fh, 095h, 0c0h
    ; setne al                                  ; 0f 95 c0
    xor ah, ah                                ; 30 e4
    mov dx, ax                                ; 89 c2
    movzx bx, byte [bp-00ah]                  ; 0f b6 5e f6
    mov es, [bp-010h]                         ; 8e 46 f0
    add bx, word [bp-012h]                    ; 03 5e ee
    mov al, byte [bp-008h]                    ; 8a 46 f8
    mov byte [es:bx+0022dh], al               ; 26 88 87 2d 02
    movzx ax, byte [bp-00eh]                  ; 0f b6 46 f2
    imul ax, ax, strict byte 0001ch           ; 6b c0 1c
    mov bx, word [bp-012h]                    ; 8b 5e ee
    add bx, ax                                ; 01 c3
    mov word [es:bx+022h], 00505h             ; 26 c7 47 22 05 05
    mov byte [es:bx+024h], dl                 ; 26 88 57 24
    mov word [es:bx+028h], 00800h             ; 26 c7 47 28 00 08
    mov bx, word [bp-012h]                    ; 8b 5e ee
    mov al, byte [es:bx+001f3h]               ; 26 8a 87 f3 01
    mov ah, byte [bp-00ah]                    ; 8a 66 f6
    add ah, 00ch                              ; 80 c4 0c
    movzx bx, al                              ; 0f b6 d8
    add bx, word [bp-012h]                    ; 03 5e ee
    mov byte [es:bx+001f4h], ah               ; 26 88 a7 f4 01
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    mov bx, word [bp-012h]                    ; 8b 5e ee
    mov byte [es:bx+001f3h], al               ; 26 88 87 f3 01
    inc byte [bp-00ah]                        ; fe 46 f6
    mov al, byte [bp-00ah]                    ; 8a 46 f6
    les bx, [bp-012h]                         ; c4 5e ee
    mov byte [es:bx+00231h], al               ; 26 88 87 31 02
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bp                                    ; 5d
    retn                                      ; c3
ahci_mem_alloc_:                             ; 0xf97e5 LB 0x43
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    push si                                   ; 56
    push di                                   ; 57
    mov dx, 00413h                            ; ba 13 04
    xor ax, ax                                ; 31 c0
    call 01765h                               ; e8 70 7f
    test ax, ax                               ; 85 c0
    je short 0981eh                           ; 74 25
    dec ax                                    ; 48
    mov bx, ax                                ; 89 c3
    xor dx, dx                                ; 31 d2
    mov cx, strict word 0000ah                ; b9 0a 00
    sal ax, 1                                 ; d1 e0
    rcl dx, 1                                 ; d1 d2
    loop 09801h                               ; e2 fa
    mov si, ax                                ; 89 c6
    mov di, dx                                ; 89 d7
    mov cx, strict word 00004h                ; b9 04 00
    shr di, 1                                 ; d1 ef
    rcr si, 1                                 ; d1 de
    loop 0980eh                               ; e2 fa
    mov dx, 00413h                            ; ba 13 04
    xor ax, ax                                ; 31 c0
    call 01773h                               ; e8 57 7f
    mov ax, si                                ; 89 f0
    lea sp, [bp-00ah]                         ; 8d 66 f6
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
ahci_hba_init_:                              ; 0xf9828 LB 0x166
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
    call 01765h                               ; e8 27 7f
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
    call 097e5h                               ; e8 83 ff
    mov word [bp-010h], ax                    ; 89 46 f0
    test ax, ax                               ; 85 c0
    je near 0996dh                            ; 0f 84 02 01
    mov ax, word [bp-010h]                    ; 8b 46 f0
    mov es, di                                ; 8e c7
    mov word [es:bx+00232h], ax               ; 26 89 87 32 02
    mov byte [es:bx+00231h], 000h             ; 26 c6 87 31 02 00
    xor bx, bx                                ; 31 db
    mov es, ax                                ; 8e c0
    mov byte [es:bx+00262h], 0ffh             ; 26 c6 87 62 02 ff
    mov word [es:bx+00260h], si               ; 26 89 b7 60 02
    db  066h, 026h, 0c7h, 087h, 064h, 002h, 000h, 0c0h, 00ch, 000h
    ; mov dword [es:bx+00264h], strict dword 0000cc000h ; 66 26 c7 87 64 02 00 c0 0c 00
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
    jne short 098c1h                          ; 75 de
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
    push strict byte 00000h                   ; 6a 00
    mov bx, strict word 0001fh                ; bb 1f 00
    xor cx, cx                                ; 31 c9
    call 088cch                               ; e8 c5 ef
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    mov byte [bp-00eh], al                    ; 88 46 f2
    mov byte [bp-00ch], 000h                  ; c6 46 f4 00
    jmp short 09932h                          ; eb 20
    xor al, al                                ; 30 c0
    test al, al                               ; 84 c0
    je short 09929h                           ; 74 11
    movzx bx, byte [bp-00ch]                  ; 0f b6 5e f4
    xor ax, ax                                ; 31 c0
    mov dx, word [bp-010h]                    ; 8b 56 f0
    call 0932dh                               ; e8 09 fa
    dec byte [bp-00eh]                        ; fe 4e f2
    je short 0996bh                           ; 74 42
    inc byte [bp-00ch]                        ; fe 46 f4
    cmp byte [bp-00ch], 020h                  ; 80 7e f4 20
    jnc short 0996bh                          ; 73 39
    movzx cx, byte [bp-00ch]                  ; 0f b6 4e f4
    mov bx, strict word 00001h                ; bb 01 00
    xor di, di                                ; 31 ff
    jcxz 09943h                               ; e3 06
    sal bx, 1                                 ; d1 e3
    rcl di, 1                                 ; d1 d7
    loop 0993dh                               ; e2 fa
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
    jne short 09967h                          ; 75 04
    test ax, bx                               ; 85 d8
    je short 09912h                           ; 74 ab
    mov AL, strict byte 001h                  ; b0 01
    jmp short 09914h                          ; eb a9
    xor ax, ax                                ; 31 c0
    lea sp, [bp-00ah]                         ; 8d 66 f6
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
    db  00bh, 005h, 004h, 003h, 002h, 001h, 000h, 05eh, 09ah, 03ch, 09ah, 042h, 09ah, 048h, 09ah, 04eh
    db  09ah, 054h, 09ah, 05ah, 09ah, 05eh, 09ah
_ahci_init:                                  ; 0xf998e LB 0x116
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    push di                                   ; 57
    sub sp, strict byte 00008h                ; 83 ec 08
    mov ax, 00601h                            ; b8 01 06
    mov dx, strict word 00001h                ; ba 01 00
    call 09ec4h                               ; e8 25 05
    mov bx, ax                                ; 89 c3
    cmp ax, strict word 0ffffh                ; 3d ff ff
    je near 09a9dh                            ; 0f 84 f5 00
    xor al, al                                ; 30 c0
    shr ax, 008h                              ; c1 e8 08
    mov byte [bp-00ah], al                    ; 88 46 f6
    mov byte [bp-008h], bl                    ; 88 5e f8
    movzx dx, bl                              ; 0f b6 d3
    xor ah, ah                                ; 30 e4
    mov bx, strict word 00034h                ; bb 34 00
    call 09f14h                               ; e8 56 05
    mov cl, al                                ; 88 c1
    test cl, cl                               ; 84 c9
    je short 099e7h                           ; 74 23
    movzx bx, cl                              ; 0f b6 d9
    movzx di, byte [bp-008h]                  ; 0f b6 7e f8
    movzx si, byte [bp-00ah]                  ; 0f b6 76 f6
    mov dx, di                                ; 89 fa
    mov ax, si                                ; 89 f0
    call 09f14h                               ; e8 3e 05
    cmp AL, strict byte 012h                  ; 3c 12
    je short 099e7h                           ; 74 0d
    mov al, cl                                ; 88 c8
    db  0feh, 0c0h
    ; inc al                                    ; fe c0
    movzx bx, al                              ; 0f b6 d8
    mov dx, di                                ; 89 fa
    mov ax, si                                ; 89 f0
    jmp short 099bbh                          ; eb d4
    test cl, cl                               ; 84 c9
    je near 09a9dh                            ; 0f 84 b0 00
    add cl, 002h                              ; 80 c1 02
    movzx bx, cl                              ; 0f b6 d9
    movzx si, byte [bp-008h]                  ; 0f b6 76 f8
    movzx di, byte [bp-00ah]                  ; 0f b6 7e f6
    mov dx, si                                ; 89 f2
    mov ax, di                                ; 89 f8
    call 09f14h                               ; e8 12 05
    cmp AL, strict byte 010h                  ; 3c 10
    jne near 09a9dh                           ; 0f 85 95 00
    mov byte [bp-006h], 000h                  ; c6 46 fa 00
    mov al, cl                                ; 88 c8
    add AL, strict byte 002h                  ; 04 02
    movzx bx, al                              ; 0f b6 d8
    mov dx, si                                ; 89 f2
    mov ax, di                                ; 89 f8
    call 09f38h                               ; e8 1e 05
    mov dx, ax                                ; 89 c2
    and ax, strict word 0000fh                ; 25 0f 00
    sub ax, strict word 00004h                ; 2d 04 00
    cmp ax, strict word 0000bh                ; 3d 0b 00
    jnbe short 09a5eh                         ; 77 37
    push CS                                   ; 0e
    pop ES                                    ; 07
    mov cx, strict word 00008h                ; b9 08 00
    mov di, 09977h                            ; bf 77 99
    repne scasb                               ; f2 ae
    sal cx, 1                                 ; d1 e1
    mov di, cx                                ; 89 cf
    mov ax, word [cs:di-06682h]               ; 2e 8b 85 7e 99
    jmp ax                                    ; ff e0
    mov byte [bp-006h], 010h                  ; c6 46 fa 10
    jmp short 09a5eh                          ; eb 1c
    mov byte [bp-006h], 014h                  ; c6 46 fa 14
    jmp short 09a5eh                          ; eb 16
    mov byte [bp-006h], 018h                  ; c6 46 fa 18
    jmp short 09a5eh                          ; eb 10
    mov byte [bp-006h], 01ch                  ; c6 46 fa 1c
    jmp short 09a5eh                          ; eb 0a
    mov byte [bp-006h], 020h                  ; c6 46 fa 20
    jmp short 09a5eh                          ; eb 04
    mov byte [bp-006h], 024h                  ; c6 46 fa 24
    mov cx, dx                                ; 89 d1
    shr cx, 004h                              ; c1 e9 04
    sal cx, 002h                              ; c1 e1 02
    mov al, byte [bp-006h]                    ; 8a 46 fa
    test al, al                               ; 84 c0
    je short 09a9dh                           ; 74 30
    movzx bx, al                              ; 0f b6 d8
    movzx di, byte [bp-008h]                  ; 0f b6 7e f8
    movzx si, byte [bp-00ah]                  ; 0f b6 76 f6
    mov dx, di                                ; 89 fa
    mov ax, si                                ; 89 f0
    call 09f5ah                               ; e8 db 04
    test AL, strict byte 001h                 ; a8 01
    je short 09a9dh                           ; 74 1a
    and AL, strict byte 0f0h                  ; 24 f0
    add ax, cx                                ; 01 c8
    mov word [bp-00ch], ax                    ; 89 46 f4
    mov cx, strict word 00007h                ; b9 07 00
    mov bx, strict word 00004h                ; bb 04 00
    mov dx, di                                ; 89 fa
    mov ax, si                                ; 89 f0
    call 09f81h                               ; e8 ea 04
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    call 09828h                               ; e8 8b fd
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
apm_out_str_:                                ; 0xf9aa4 LB 0x39
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    mov bx, ax                                ; 89 c3
    cmp byte [bx], 000h                       ; 80 3f 00
    je short 09ab9h                           ; 74 0a
    mov al, byte [bx]                         ; 8a 07
    out DX, AL                                ; ee
    inc bx                                    ; 43
    mov al, byte [bx]                         ; 8a 07
    db  00ah, 0c0h
    ; or al, al                                 ; 0a c0
    jne short 09ab1h                          ; 75 f8
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
    add byte [bp+di-06434h], bl               ; 00 9b cc 9b
    adc bl, byte [bp+di-064d3h]               ; 12 9b 2d 9b
    int3                                      ; cc
    wait                                      ; 9b
    pop ax                                    ; 58
    wait                                      ; 9b
    int3                                      ; cc
    wait                                      ; 9b
    pop bp                                    ; 5d
    wait                                      ; 9b
    mov ax, word [0a19bh]                     ; a1 9b a1
    wait                                      ; 9b
    mov ax, word [09c9bh]                     ; a1 9b 9c
    wait                                      ; 9b
    mov ax, word [0a19bh]                     ; a1 9b a1
    wait                                      ; 9b
    xchg bp, ax                               ; 95
    wait                                      ; 9b
_apm_function:                               ; 0xf9add LB 0xf5
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push si                                   ; 56
    and byte [bp+018h], 0feh                  ; 80 66 18 fe
    mov ax, word [bp+012h]                    ; 8b 46 12
    xor ah, ah                                ; 30 e4
    cmp ax, strict word 0000eh                ; 3d 0e 00
    jnbe near 09ba1h                          ; 0f 87 b0 00
    mov bx, ax                                ; 89 c3
    add bx, ax                                ; 01 c3
    mov dx, word [bp+018h]                    ; 8b 56 18
    or dl, 001h                               ; 80 ca 01
    jmp word [cs:bx-06541h]                   ; 2e ff a7 bf 9a
    mov word [bp+012h], 00102h                ; c7 46 12 02 01
    mov word [bp+00ch], 0504dh                ; c7 46 0c 4d 50
    mov word [bp+010h], strict word 00003h    ; c7 46 10 03 00
    jmp near 09bcch                           ; e9 ba 00
    mov word [bp+012h], 0f000h                ; c7 46 12 00 f0
    mov word [bp+00ch], 0a1b4h                ; c7 46 0c b4 a1
    mov word [bp+010h], 0f000h                ; c7 46 10 00 f0
    mov ax, strict word 0fff0h                ; b8 f0 ff
    mov word [bp+006h], ax                    ; 89 46 06
    mov word [bp+004h], ax                    ; 89 46 04
    jmp near 09bcch                           ; e9 9f 00
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
    jmp near 09bcch                           ; e9 74 00
    sti                                       ; fb
    hlt                                       ; f4
    jmp near 09bcch                           ; e9 6f 00
    cmp word [bp+010h], strict byte 00003h    ; 83 7e 10 03
    je short 09b82h                           ; 74 1f
    cmp word [bp+010h], strict byte 00002h    ; 83 7e 10 02
    je short 09b7ah                           ; 74 11
    cmp word [bp+010h], strict byte 00001h    ; 83 7e 10 01
    jne short 09b8ah                          ; 75 1b
    mov dx, 0040fh                            ; ba 0f 04
    mov ax, 00d22h                            ; b8 22 0d
    call 09aa4h                               ; e8 2c ff
    jmp short 09bcch                          ; eb 52
    mov dx, 0040fh                            ; ba 0f 04
    mov ax, 00d2ah                            ; b8 2a 0d
    jmp short 09b75h                          ; eb f3
    mov dx, 0040fh                            ; ba 0f 04
    mov ax, 00d32h                            ; b8 32 0d
    jmp short 09b75h                          ; eb eb
    or ah, 00ah                               ; 80 cc 0a
    mov word [bp+012h], ax                    ; 89 46 12
    mov word [bp+018h], dx                    ; 89 56 18
    jmp short 09bcch                          ; eb 37
    mov word [bp+012h], 00102h                ; c7 46 12 02 01
    jmp short 09bcch                          ; eb 30
    or ah, 080h                               ; 80 cc 80
    jmp short 09b8dh                          ; eb ec
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 7e 7e
    push word [bp+00ch]                       ; ff 76 0c
    push word [bp+012h]                       ; ff 76 12
    push 00d3bh                               ; 68 3b 0d
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 b1 7e
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
pci16_select_reg_:                           ; 0xf9bd2 LB 0x24
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
pci16_find_device_:                          ; 0xf9bf6 LB 0xf7
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
    mov byte [bp-008h], 000h                  ; c6 46 f8 00
    test bl, 007h                             ; f6 c3 07
    jne short 09c3eh                          ; 75 2d
    mov dx, strict word 0000eh                ; ba 0e 00
    mov ax, bx                                ; 89 d8
    call 09bd2h                               ; e8 b9 ff
    mov dx, 00cfeh                            ; ba fe 0c
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    mov byte [bp-006h], al                    ; 88 46 fa
    cmp AL, strict byte 0ffh                  ; 3c ff
    jne short 09c2ch                          ; 75 06
    add bx, strict byte 00008h                ; 83 c3 08
    jmp near 09cbfh                           ; e9 93 00
    test byte [bp-006h], 080h                 ; f6 46 fa 80
    je short 09c39h                           ; 74 07
    mov word [bp-00ah], strict word 00001h    ; c7 46 f6 01 00
    jmp short 09c3eh                          ; eb 05
    mov word [bp-00ah], strict word 00008h    ; c7 46 f6 08 00
    mov al, byte [bp-006h]                    ; 8a 46 fa
    and AL, strict byte 007h                  ; 24 07
    cmp AL, strict byte 001h                  ; 3c 01
    jne short 09c66h                          ; 75 1f
    mov ax, bx                                ; 89 d8
    shr ax, 008h                              ; c1 e8 08
    test ax, ax                               ; 85 c0
    jne short 09c66h                          ; 75 16
    mov dx, strict word 0001ah                ; ba 1a 00
    mov ax, bx                                ; 89 d8
    call 09bd2h                               ; e8 7a ff
    mov dx, 00cfeh                            ; ba fe 0c
    in AL, DX                                 ; ec
    db  02ah, 0e4h
    ; sub ah, ah                                ; 2a e4
    cmp al, byte [bp-008h]                    ; 3a 46 f8
    jbe short 09c66h                          ; 76 03
    mov byte [bp-008h], al                    ; 88 46 f8
    test di, di                               ; 85 ff
    je short 09c6fh                           ; 74 05
    mov dx, strict word 00008h                ; ba 08 00
    jmp short 09c71h                          ; eb 02
    xor dx, dx                                ; 31 d2
    mov ax, bx                                ; 89 d8
    call 09bd2h                               ; e8 5c ff
    mov dx, 00cfch                            ; ba fc 0c
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov word [bp-00ch], dx                    ; 89 56 f4
    mov word [bp-010h], strict word 00000h    ; c7 46 f0 00 00
    test di, di                               ; 85 ff
    je short 09ca0h                           ; 74 0f
    mov cx, strict word 00008h                ; b9 08 00
    shr dx, 1                                 ; d1 ea
    rcr ax, 1                                 ; d1 d8
    loop 09c94h                               ; e2 fa
    mov word [bp-00eh], ax                    ; 89 46 f2
    mov word [bp-00ch], dx                    ; 89 56 f4
    mov ax, word [bp-00ch]                    ; 8b 46 f4
    cmp ax, word [bp-014h]                    ; 3b 46 ec
    jne short 09cb0h                          ; 75 08
    mov ax, word [bp-00eh]                    ; 8b 46 f2
    cmp ax, word [bp-012h]                    ; 3b 46 ee
    je short 09cb6h                           ; 74 06
    cmp word [bp-010h], strict byte 00000h    ; 83 7e f0 00
    je short 09cbch                           ; 74 06
    dec si                                    ; 4e
    cmp si, strict byte 0ffffh                ; 83 fe ff
    je short 09cceh                           ; 74 12
    add bx, word [bp-00ah]                    ; 03 5e f6
    mov dx, bx                                ; 89 da
    shr dx, 008h                              ; c1 ea 08
    movzx ax, byte [bp-008h]                  ; 0f b6 46 f8
    cmp dx, ax                                ; 39 c2
    jbe near 09c0ch                           ; 0f 86 3e ff
    cmp si, strict byte 0ffffh                ; 83 fe ff
    jne short 09cd7h                          ; 75 04
    mov ax, bx                                ; 89 d8
    jmp short 09cdah                          ; eb 03
    mov ax, strict word 0ffffh                ; b8 ff ff
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop si                                    ; 5e
    pop bp                                    ; 5d
    retn                                      ; c3
    iret                                      ; cf
    popfw                                     ; 9d
    jmp near 09983h                           ; e9 9d fc
    popfw                                     ; 9d
    adc word [bp-061dch], bx                  ; 11 9e 24 9e
    aaa                                       ; 37
    sahf                                      ; 9e
_pci16_function:                             ; 0xf9ced LB 0x1d7
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
    jc short 09d26h                           ; 72 1a
    jbe short 09d7eh                          ; 76 70
    cmp bx, strict byte 0000eh                ; 83 fb 0e
    je near 09e4bh                            ; 0f 84 36 01
    cmp bx, strict byte 00008h                ; 83 fb 08
    jc near 09e90h                            ; 0f 82 74 01
    cmp bx, strict byte 0000dh                ; 83 fb 0d
    jbe near 09da3h                           ; 0f 86 80 00
    jmp near 09e90h                           ; e9 6a 01
    cmp bx, strict byte 00002h                ; 83 fb 02
    je short 09d4eh                           ; 74 23
    cmp bx, strict byte 00001h                ; 83 fb 01
    jne near 09e90h                           ; 0f 85 5e 01
    mov word [bp+020h], strict word 00001h    ; c7 46 20 01 00
    mov word [bp+014h], 00210h                ; c7 46 14 10 02
    mov word [bp+01ch], strict word 00000h    ; c7 46 1c 00 00
    mov word [bp+018h], 04350h                ; c7 46 18 50 43
    mov word [bp+01ah], 02049h                ; c7 46 1a 49 20
    jmp near 09ebdh                           ; e9 6f 01
    cmp word [bp+018h], strict byte 0ffffh    ; 83 7e 18 ff
    jne short 09d5ah                          ; 75 06
    or ah, 083h                               ; 80 cc 83
    jmp near 09eb6h                           ; e9 5c 01
    mov bx, word [bp+008h]                    ; 8b 5e 08
    mov dx, word [bp+01ch]                    ; 8b 56 1c
    mov ax, word [bp+018h]                    ; 8b 46 18
    xor cx, cx                                ; 31 c9
    call 09bf6h                               ; e8 8e fe
    cmp ax, strict word 0ffffh                ; 3d ff ff
    jne short 09d78h                          ; 75 0b
    mov ax, word [bp+020h]                    ; 8b 46 20
    xor ah, ah                                ; 30 e4
    or ah, 086h                               ; 80 cc 86
    jmp near 09eb6h                           ; e9 3e 01
    mov word [bp+014h], ax                    ; 89 46 14
    jmp near 09ebdh                           ; e9 3f 01
    mov bx, word [bp+008h]                    ; 8b 5e 08
    mov ax, word [bp+01ch]                    ; 8b 46 1c
    mov dx, word [bp+01eh]                    ; 8b 56 1e
    mov cx, strict word 00001h                ; b9 01 00
    call 09bf6h                               ; e8 69 fe
    cmp ax, strict word 0ffffh                ; 3d ff ff
    jne short 09d9dh                          ; 75 0b
    mov ax, word [bp+020h]                    ; 8b 46 20
    xor ah, ah                                ; 30 e4
    or ah, 086h                               ; 80 cc 86
    jmp near 09eb6h                           ; e9 19 01
    mov word [bp+014h], ax                    ; 89 46 14
    jmp near 09ebdh                           ; e9 1a 01
    cmp word [bp+004h], 00100h                ; 81 7e 04 00 01
    jc short 09db0h                           ; 72 06
    or ah, 087h                               ; 80 cc 87
    jmp near 09eb6h                           ; e9 06 01
    mov dx, word [bp+004h]                    ; 8b 56 04
    mov ax, word [bp+014h]                    ; 8b 46 14
    call 09bd2h                               ; e8 19 fe
    mov bx, word [bp+020h]                    ; 8b 5e 20
    xor bh, bh                                ; 30 ff
    sub bx, strict byte 00008h                ; 83 eb 08
    cmp bx, strict byte 00005h                ; 83 fb 05
    jnbe near 09ebdh                          ; 0f 87 f5 00
    add bx, bx                                ; 01 db
    jmp word [cs:bx-0631fh]                   ; 2e ff a7 e1 9c
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
    jmp near 09ebdh                           ; e9 d4 00
    mov dx, word [bp+004h]                    ; 8b 56 04
    xor dh, dh                                ; 30 f6
    and dl, 002h                              ; 80 e2 02
    add dx, 00cfch                            ; 81 c2 fc 0c
    in ax, DX                                 ; ed
    mov word [bp+01ch], ax                    ; 89 46 1c
    jmp near 09ebdh                           ; e9 c1 00
    mov dx, 00cfch                            ; ba fc 0c
    in eax, DX                                ; 66 ed
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    shr eax, 010h                             ; 66 c1 e8 10
    xchg dx, ax                               ; 92
    mov word [bp+01ch], ax                    ; 89 46 1c
    mov word [bp+01eh], dx                    ; 89 56 1e
    jmp near 09ebdh                           ; e9 ac 00
    mov ax, word [bp+01ch]                    ; 8b 46 1c
    mov dx, word [bp+004h]                    ; 8b 56 04
    xor dh, dh                                ; 30 f6
    and dl, 003h                              ; 80 e2 03
    add dx, 00cfch                            ; 81 c2 fc 0c
    out DX, AL                                ; ee
    jmp near 09ebdh                           ; e9 99 00
    mov ax, word [bp+01ch]                    ; 8b 46 1c
    mov dx, word [bp+004h]                    ; 8b 56 04
    xor dh, dh                                ; 30 f6
    and dl, 002h                              ; 80 e2 02
    add dx, 00cfch                            ; 81 c2 fc 0c
    out DX, ax                                ; ef
    jmp near 09ebdh                           ; e9 86 00
    mov ax, word [bp+01ch]                    ; 8b 46 1c
    mov cx, word [bp+01eh]                    ; 8b 4e 1e
    mov dx, 00cfch                            ; ba fc 0c
    xchg cx, ax                               ; 91
    sal eax, 010h                             ; 66 c1 e0 10
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    out DX, eax                               ; 66 ef
    jmp short 09ebdh                          ; eb 72
    mov bx, word [bp+004h]                    ; 8b 5e 04
    mov es, [bp+026h]                         ; 8e 46 26
    mov word [bp-008h], bx                    ; 89 5e f8
    mov [bp-006h], es                         ; 8c 46 fa
    mov cx, word [0f370h]                     ; 8b 0e 70 f3
    cmp cx, word [es:bx]                      ; 26 3b 0f
    jbe short 09e71h                          ; 76 11
    mov ax, word [bp+020h]                    ; 8b 46 20
    xor ah, ah                                ; 30 e4
    or ah, 089h                               ; 80 cc 89
    mov word [bp+020h], ax                    ; 89 46 20
    or word [bp+02ch], strict byte 00001h     ; 83 4e 2c 01
    jmp short 09e85h                          ; eb 14
    les di, [es:bx+002h]                      ; 26 c4 7f 02
    mov si, 0f190h                            ; be 90 f1
    mov dx, ds                                ; 8c da
    push DS                                   ; 1e
    mov ds, dx                                ; 8e da
    rep movsb                                 ; f3 a4
    pop DS                                    ; 1f
    mov word [bp+014h], 00a00h                ; c7 46 14 00 0a
    mov ax, word [0f370h]                     ; a1 70 f3
    les bx, [bp-008h]                         ; c4 5e f8
    mov word [es:bx], ax                      ; 26 89 07
    jmp short 09ebdh                          ; eb 2d
    mov bx, 00db2h                            ; bb b2 0d
    mov cx, ds                                ; 8c d9
    mov ax, strict word 00004h                ; b8 04 00
    call 01a2ah                               ; e8 8f 7b
    mov ax, word [bp+014h]                    ; 8b 46 14
    push ax                                   ; 50
    mov ax, word [bp+020h]                    ; 8b 46 20
    push ax                                   ; 50
    push 00d6eh                               ; 68 6e 0d
    push strict byte 00004h                   ; 6a 04
    call 01a6bh                               ; e8 c0 7b
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
pci_find_classcode_:                         ; 0xf9ec4 LB 0x2b
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push cx                                   ; 51
    push si                                   ; 56
    mov cx, dx                                ; 89 d1
    xor si, si                                ; 31 f6
    mov dx, ax                                ; 89 c2
    mov ax, 0b103h                            ; b8 03 b1
    sal ecx, 010h                             ; 66 c1 e1 10
    db  08bh, 0cah
    ; mov cx, dx                                ; 8b ca
    int 01ah                                  ; cd 1a
    cmp ah, 000h                              ; 80 fc 00
    je near 09ee5h                            ; 0f 84 03 00
    mov bx, strict word 0ffffh                ; bb ff ff
    mov ax, bx                                ; 89 d8
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
pci_find_device_:                            ; 0xf9eef LB 0x25
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
    je near 09f0ah                            ; 0f 84 03 00
    mov bx, strict word 0ffffh                ; bb ff ff
    mov ax, bx                                ; 89 d8
    lea sp, [bp-006h]                         ; 8d 66 fa
    pop si                                    ; 5e
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop bp                                    ; 5d
    retn                                      ; c3
pci_read_config_byte_:                       ; 0xf9f14 LB 0x24
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push cx                                   ; 51
    push di                                   ; 57
    movzx di, bl                              ; 0f b6 fb
    movzx bx, al                              ; 0f b6 d8
    sal bx, 008h                              ; c1 e3 08
    movzx ax, dl                              ; 0f b6 c2
    or bx, ax                                 ; 09 c3
    mov ax, 0b108h                            ; b8 08 b1
    int 01ah                                  ; cd 1a
    movzx ax, cl                              ; 0f b6 c1
    xor dx, dx                                ; 31 d2
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop cx                                    ; 59
    pop bp                                    ; 5d
    retn                                      ; c3
pci_read_config_word_:                       ; 0xf9f38 LB 0x22
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push cx                                   ; 51
    push di                                   ; 57
    movzx di, bl                              ; 0f b6 fb
    xor ah, ah                                ; 30 e4
    sal ax, 008h                              ; c1 e0 08
    movzx bx, dl                              ; 0f b6 da
    or bx, ax                                 ; 09 c3
    mov ax, 0b109h                            ; b8 09 b1
    int 01ah                                  ; cd 1a
    mov ax, cx                                ; 89 c8
    xor dx, dx                                ; 31 d2
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop cx                                    ; 59
    pop bp                                    ; 5d
    retn                                      ; c3
pci_read_config_dword_:                      ; 0xf9f5a LB 0x27
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push cx                                   ; 51
    push di                                   ; 57
    movzx di, bl                              ; 0f b6 fb
    movzx bx, al                              ; 0f b6 d8
    sal bx, 008h                              ; c1 e3 08
    movzx ax, dl                              ; 0f b6 c2
    or bx, ax                                 ; 09 c3
    mov ax, 0b10ah                            ; b8 0a b1
    int 01ah                                  ; cd 1a
    db  08bh, 0c1h
    ; mov ax, cx                                ; 8b c1
    shr ecx, 010h                             ; 66 c1 e9 10
    mov dx, cx                                ; 89 ca
    lea sp, [bp-004h]                         ; 8d 66 fc
    pop di                                    ; 5f
    pop cx                                    ; 59
    pop bp                                    ; 5d
    retn                                      ; c3
pci_write_config_word_:                      ; 0xf9f81 LB 0x1c
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push di                                   ; 57
    movzx di, bl                              ; 0f b6 fb
    xor ah, ah                                ; 30 e4
    sal ax, 008h                              ; c1 e0 08
    movzx bx, dl                              ; 0f b6 da
    or bx, ax                                 ; 09 c3
    mov ax, 0b10ch                            ; b8 0c b1
    int 01ah                                  ; cd 1a
    lea sp, [bp-002h]                         ; 8d 66 fe
    pop di                                    ; 5f
    pop bp                                    ; 5d
    retn                                      ; c3
vds_is_present_:                             ; 0xf9f9d LB 0x1d
    push bx                                   ; 53
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    mov bx, strict word 0007bh                ; bb 7b 00
    mov ax, strict word 00040h                ; b8 40 00
    mov es, ax                                ; 8e c0
    test byte [es:bx], 020h                   ; 26 f6 07 20
    je short 09fb5h                           ; 74 06
    mov ax, strict word 00001h                ; b8 01 00
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    retn                                      ; c3
    xor ax, ax                                ; 31 c0
    pop bp                                    ; 5d
    pop bx                                    ; 5b
    retn                                      ; c3
vds_real_to_lin_:                            ; 0xf9fba LB 0x1e
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
    loop 09fc8h                               ; e2 fa
    xor cx, cx                                ; 31 c9
    add ax, bx                                ; 01 d8
    adc dx, cx                                ; 11 ca
    pop bp                                    ; 5d
    pop cx                                    ; 59
    pop bx                                    ; 5b
    retn                                      ; c3
vds_build_sg_list_:                          ; 0xf9fd8 LB 0x79
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
    call 09fbah                               ; e8 c3 ff
    mov es, si                                ; 8e c6
    mov word [es:di+004h], ax                 ; 26 89 45 04
    mov word [es:di+006h], dx                 ; 26 89 55 06
    mov word [es:di+008h], strict word 00000h ; 26 c7 45 08 00 00
    call 09f9dh                               ; e8 93 ff
    test ax, ax                               ; 85 c0
    je short 0a021h                           ; 74 13
    mov es, si                                ; 8e c6
    mov ax, 08105h                            ; b8 05 81
    mov dx, strict word 00000h                ; ba 00 00
    int 04bh                                  ; cd 4b
    jc near 0a01eh                            ; 0f 82 02 00
    db  032h, 0c0h
    ; xor al, al                                ; 32 c0
    cbw                                       ; 98
    jmp short 0a048h                          ; eb 27
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
vds_free_sg_list_:                           ; 0xfa051 LB 0x3f
    push bp                                   ; 55
    mov bp, sp                                ; 89 e5
    push bx                                   ; 53
    push di                                   ; 57
    mov bx, ax                                ; 89 c3
    call 09f9dh                               ; e8 42 ff
    test ax, ax                               ; 85 c0
    je short 0a072h                           ; 74 13
    mov di, bx                                ; 89 df
    mov es, dx                                ; 8e c2
    mov ax, 08106h                            ; b8 06 81
    mov dx, strict word 00000h                ; ba 00 00
    int 04bh                                  ; cd 4b
    jc near 0a071h                            ; 0f 82 02 00
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
    times 0xf db 0
__U4M:                                       ; 0xfa090 LB 0x40
    pushfw                                    ; 9c
    push eax                                  ; 66 50
    push edx                                  ; 66 52
    push ecx                                  ; 66 51
    rol eax, 010h                             ; 66 c1 c0 10
    db  08bh, 0c2h
    ; mov ax, dx                                ; 8b c2
    ror eax, 010h                             ; 66 c1 c8 10
    db  066h, 033h, 0d2h
    ; xor edx, edx                              ; 66 33 d2
    shr ecx, 010h                             ; 66 c1 e9 10
    db  08bh, 0cbh
    ; mov cx, bx                                ; 8b cb
    mul ecx                                   ; 66 f7 e1
    pop ecx                                   ; 66 59
    pop edx                                   ; 66 5a
    ror eax, 010h                             ; 66 c1 c8 10
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    add sp, strict byte 00002h                ; 83 c4 02
    pop ax                                    ; 58
    rol eax, 010h                             ; 66 c1 c0 10
    popfw                                     ; 9d
    retn                                      ; c3
    times 0xf db 0
__U4D:                                       ; 0xfa0d0 LB 0x40
    pushfw                                    ; 9c
    push eax                                  ; 66 50
    push edx                                  ; 66 52
    push ecx                                  ; 66 51
    rol eax, 010h                             ; 66 c1 c0 10
    db  08bh, 0c2h
    ; mov ax, dx                                ; 8b c2
    ror eax, 010h                             ; 66 c1 c8 10
    db  066h, 033h, 0d2h
    ; xor edx, edx                              ; 66 33 d2
    shr ecx, 010h                             ; 66 c1 e9 10
    db  08bh, 0cbh
    ; mov cx, bx                                ; 8b cb
    div ecx                                   ; 66 f7 f1
    db  08bh, 0dah
    ; mov bx, dx                                ; 8b da
    pop ecx                                   ; 66 59
    shr edx, 010h                             ; 66 c1 ea 10
    db  08bh, 0cah
    ; mov cx, dx                                ; 8b ca
    pop edx                                   ; 66 5a
    ror eax, 010h                             ; 66 c1 c8 10
    db  08bh, 0d0h
    ; mov dx, ax                                ; 8b d0
    add sp, strict byte 00002h                ; 83 c4 02
    pop ax                                    ; 58
    rol eax, 010h                             ; 66 c1 c0 10
    popfw                                     ; 9d
    retn                                      ; c3
    times 0x7 db 0
__U8RS:                                      ; 0xfa110 LB 0x10
    test si, si                               ; 85 f6
    je short 0a11fh                           ; 74 0b
    shr ax, 1                                 ; d1 e8
    rcr bx, 1                                 ; d1 db
    rcr cx, 1                                 ; d1 d9
    rcr dx, 1                                 ; d1 da
    dec si                                    ; 4e
    jne short 0a114h                          ; 75 f5
    retn                                      ; c3
__U8LS:                                      ; 0xfa120 LB 0x10
    test si, si                               ; 85 f6
    je short 0a12fh                           ; 74 0b
    sal dx, 1                                 ; d1 e2
    rcl cx, 1                                 ; d1 d1
    rcl bx, 1                                 ; d1 d3
    rcl ax, 1                                 ; d1 d0
    dec si                                    ; 4e
    jne short 0a124h                          ; 75 f5
    retn                                      ; c3
_fmemset_:                                   ; 0xfa130 LB 0x10
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
_fmemcpy_:                                   ; 0xfa140 LB 0x3a
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
    nop                                       ; 90
    mov ax, word [0a192h]                     ; a1 92 a1
    xchg si, ax                               ; 96
    mov ax, word [0a196h]                     ; a1 96 a1
    xchg si, ax                               ; 96
    mov ax, word [0a198h]                     ; a1 98 a1
    cbw                                       ; 98
    mov ax, word [0a19ah]                     ; a1 9a a1
    sahf                                      ; 9e
    mov ax, word [0a19eh]                     ; a1 9e a1
    mov AL, byte [0a5a1h]                     ; a0 a1 a5
    mov ax, word [0a1a7h]                     ; a1 a7 a1
apm_worker:                                  ; 0xfa17a LB 0x3a
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
    jnc short 0a1b0h                          ; 73 25
    jmp word [cs:bp-05ea0h]                   ; 2e ff a6 60 a1
    jmp short 0a1aeh                          ; eb 1c
    sti                                       ; fb
    hlt                                       ; f4
    jmp short 0a1aeh                          ; eb 18
    jmp short 0a1aeh                          ; eb 16
    jmp short 0a1b0h                          ; eb 16
    mov AH, strict byte 080h                  ; b4 80
    jmp short 0a1b2h                          ; eb 14
    jmp short 0a1b0h                          ; eb 10
    mov ax, 00102h                            ; b8 02 01
    jmp short 0a1aeh                          ; eb 09
    jmp short 0a1aeh                          ; eb 07
    mov BL, strict byte 000h                  ; b3 00
    mov cx, strict word 00000h                ; b9 00 00
    jmp short 0a1aeh                          ; eb 00
    clc                                       ; f8
    retn                                      ; c3
    mov AH, strict byte 009h                  ; b4 09
    stc                                       ; f9
    retn                                      ; c3
apm_pm16_entry:                              ; 0xfa1b4 LB 0x11
    mov AH, strict byte 002h                  ; b4 02
    push DS                                   ; 1e
    push bp                                   ; 55
    push CS                                   ; 0e
    pop bp                                    ; 5d
    add bp, strict byte 00008h                ; 83 c5 08
    mov ds, bp                                ; 8e dd
    call 0a17ah                               ; e8 b8 ff
    pop bp                                    ; 5d
    pop DS                                    ; 1f
    retf                                      ; cb

  ; Padding 0x383b bytes at 0xfa1c5
  times 14395 db 0

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
    mov bp, 0a1b6h                            ; bd b6 a1
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
    jo short 0dd57h                           ; 70 f3
    add byte [bx+si], al                      ; 00 00
    db  066h, 026h, 03bh, 010h
    ; cmp edx, dword [es:bx+si]                 ; 66 26 3b 10
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
    mov si, 0f190h                            ; be 90 f1
    add byte [bx+si], al                      ; 00 00
    mov es, [di-014h]                         ; 8e 45 ec
    push DS                                   ; 1e
    db  066h, 08eh, 0dah
    ; mov ds, edx                               ; 66 8e da
    rep movsb                                 ; f3 a4
    pop DS                                    ; 1f
    mov dword [di+018h], strict dword 0a1660a00h ; 66 c7 45 18 00 0a 66 a1
    jo short 0dd9bh                           ; 70 f3
    add byte [bx+si], al                      ; 00 00
    mov es, [di-010h]                         ; 8e 45 f0
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
post:                                        ; 0xfe05b LB 0x6b
    cli                                       ; fa
    smsw ax                                   ; 0f 01 e0
    test ax, strict word 00001h               ; a9 01 00
    je short 0e06ah                           ; 74 06
    mov AL, strict byte 001h                  ; b0 01
    out strict byte 092h, AL                  ; e6 92
    jmp short 0e068h                          ; eb fe
    mov AL, strict byte 00fh                  ; b0 0f
    out strict byte 070h, AL                  ; e6 70
    in AL, strict byte 071h                   ; e4 71
    xchg ah, al                               ; 86 c4
    in AL, strict byte 064h                   ; e4 64
    test AL, strict byte 004h                 ; a8 04
    je short 0e08bh                           ; 74 13
    db  08ah, 0c4h
    ; mov al, ah                                ; 8a c4
    db  00ah, 0c0h
    ; or al, al                                 ; 0a c0
    jne short 0e08bh                          ; 75 0d
    mov ds, [cs:0e04fh]                       ; 2e 8e 1e 4f e0
    cmp word [word 00072h], 01234h            ; 81 3e 72 00 34 12
    jne short 0e064h                          ; 75 d9
    mov AL, strict byte 00fh                  ; b0 0f
    out strict byte 070h, AL                  ; e6 70
    mov AL, strict byte 000h                  ; b0 00
    out strict byte 071h, AL                  ; e6 71
    db  08ah, 0c4h
    ; mov al, ah                                ; 8a c4
    cmp AL, strict byte 009h                  ; 3c 09
    je short 0e0abh                           ; 74 12
    cmp AL, strict byte 00ah                  ; 3c 0a
    je short 0e0abh                           ; 74 0e
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
    je short 0e0c6h                           ; 74 15
    cmp AL, strict byte 00dh                  ; 3c 0d
    jnc short 0e0c6h                          ; 73 11
    cmp AL, strict byte 009h                  ; 3c 09
    jne short 0e0bch                          ; 75 03
    jmp near 0e363h                           ; e9 a7 02
    cmp AL, strict byte 005h                  ; 3c 05
    je short 0e044h                           ; 74 84
    cmp AL, strict byte 00ah                  ; 3c 0a
    je short 0e047h                           ; 74 83
    jmp short 0e0c6h                          ; eb 00
normal_post:                                 ; 0xfe0c6 LB 0x1f6
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
    jnc short 0e0f9h                          ; 73 0b
    mov es, bx                                ; 8e c3
    db  033h, 0ffh
    ; xor di, di                                ; 33 ff
    mov cx, 08000h                            ; b9 00 80
    rep stosw                                 ; f3 ab
    jmp short 0e0e4h                          ; eb eb
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
    call 01870h                               ; e8 66 37
    call 0e8e7h                               ; e8 da 07
    db  033h, 0dbh
    ; xor bx, bx                                ; 33 db
    mov ds, bx                                ; 8e db
    mov cx, strict word 00060h                ; b9 60 00
    mov ax, 0ff53h                            ; b8 53 ff
    mov dx, 0f000h                            ; ba 00 f0
    call 0e039h                               ; e8 1c ff
    mov bx, 001a0h                            ; bb a0 01
    mov cx, strict word 00010h                ; b9 10 00
    call 0e039h                               ; e8 13 ff
    mov ax, 0027fh                            ; b8 7f 02
    mov word [00413h], ax                     ; a3 13 04
    mov ax, 0e9cch                            ; b8 cc e9
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
    mov ax, 0efedh                            ; b8 ed ef
    mov word [00070h], ax                     ; a3 70 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [00072h], ax                     ; a3 72 00
    call 0e778h                               ; e8 e9 05
    call 0f13bh                               ; e8 a9 0f
    call 0f166h                               ; e8 d1 0f
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
    call 01600h                               ; e8 e1 33
    call 04ff8h                               ; e8 d6 6d
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
    call 0ecedh                               ; e8 b0 0a
    mov dx, 00278h                            ; ba 78 02
    call 0ecedh                               ; e8 aa 0a
    sal bx, 00eh                              ; c1 e3 0e
    mov ax, word [00410h]                     ; a1 10 04
    and ax, 03fffh                            ; 25 ff 3f
    db  00bh, 0c3h
    ; or ax, bx                                 ; 0b c3
    mov word [00410h], ax                     ; a3 10 04
    mov ax, 0e746h                            ; b8 46 e7
    mov word [0002ch], ax                     ; a3 2c 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [0002eh], ax                     ; a3 2e 00
    mov ax, 0e746h                            ; b8 46 e7
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
    call 0ed0bh                               ; e8 8c 0a
    mov dx, 002f8h                            ; ba f8 02
    call 0ed0bh                               ; e8 86 0a
    mov dx, 003e8h                            ; ba e8 03
    call 0ed0bh                               ; e8 80 0a
    mov dx, 002e8h                            ; ba e8 02
    call 0ed0bh                               ; e8 7a 0a
    sal bx, 009h                              ; c1 e3 09
    mov ax, word [00410h]                     ; a1 10 04
    and ax, 0f1ffh                            ; 25 ff f1
    db  00bh, 0c3h
    ; or ax, bx                                 ; 0b c3
    mov word [00410h], ax                     ; a3 10 04
    mov ax, 0ff53h                            ; b8 53 ff
    mov word [00128h], ax                     ; a3 28 01
    mov ax, 0f000h                            ; b8 00 f0
    mov word [0012ah], ax                     ; a3 2a 01
    mov ax, 0f8feh                            ; b8 fe f8
    mov word [001c0h], ax                     ; a3 c0 01
    mov ax, 0f000h                            ; b8 00 f0
    mov word [001c2h], ax                     ; a3 c2 01
    call 0edbfh                               ; e8 05 0b
    jmp short 0e303h                          ; eb 47
biosorg_check_before_or_at_0E2C1h:           ; 0xfe2bc LB 0x7
    add byte [bx+si], al                      ; 00 00
    add byte [bx+si], al                      ; 00 00
    add byte [bx+si+04dh], bl                 ; 00 58 4d
nmi:                                         ; 0xfe2c3 LB 0x7
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 0184ch                               ; e8 83 35
    iret                                      ; cf
int75_handler:                               ; 0xfe2ca LB 0x8
    out strict byte 0f0h, AL                  ; e6 f0
    call 0e030h                               ; e8 61 fd
    int 002h                                  ; cd 02
    iret                                      ; cf
hard_drive_post:                             ; 0xfe2d2 LB 0xba
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
    mov ax, 0f8ech                            ; b8 ec f8
    mov word [001d8h], ax                     ; a3 d8 01
    mov ax, 0f000h                            ; b8 00 f0
    mov word [001dah], ax                     ; a3 da 01
    retn                                      ; c3
    mov ax, 0f8c1h                            ; b8 c1 f8
    mov word [001d0h], ax                     ; a3 d0 01
    mov ax, 0f000h                            ; b8 00 f0
    mov word [001d2h], ax                     ; a3 d2 01
    mov ax, 0e2cah                            ; b8 ca e2
    mov word [001d4h], ax                     ; a3 d4 01
    mov ax, 0f000h                            ; b8 00 f0
    mov word [001d6h], ax                     ; a3 d6 01
    call 0e753h                               ; e8 35 04
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    pushad                                    ; 66 60
    call 01692h                               ; e8 6c 33
    popad                                     ; 66 61
    call 01da8h                               ; e8 7d 3a
    call 02235h                               ; e8 07 3f
    sti                                       ; fb
    call 0998eh                               ; e8 5c b6
    cli                                       ; fa
    call 0884bh                               ; e8 15 a5
    call 0ed2fh                               ; e8 f6 09
    call 0e2d2h                               ; e8 96 ff
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    mov ax, 0c800h                            ; b8 00 c8
    mov dx, 0f000h                            ; ba 00 f0
    call 01600h                               ; e8 b8 32
    db  066h, 033h, 0c0h
    ; xor eax, eax                              ; 66 33 c0
    db  066h, 033h, 0dbh
    ; xor ebx, ebx                              ; 66 33 db
    db  066h, 033h, 0c9h
    ; xor ecx, ecx                              ; 66 33 c9
    db  066h, 033h, 0d2h
    ; xor edx, edx                              ; 66 33 d2
    call 01890h                               ; e8 39 35
    call 03cdbh                               ; e8 81 59
    sti                                       ; fb
    int 019h                                  ; cd 19
    sti                                       ; fb
    hlt                                       ; f4
    jmp short 0e35eh                          ; eb fd
    cli                                       ; fa
    hlt                                       ; f4
    mov ax, strict word 00040h                ; b8 40 00
    mov ds, ax                                ; 8e d8
    mov ss, [word 00069h]                     ; 8e 16 69 00
    mov sp, word [word 00067h]                ; 8b 26 67 00
    in AL, strict byte 092h                   ; e4 92
    and AL, strict byte 0fdh                  ; 24 fd
    out strict byte 092h, AL                  ; e6 92
    lidt [cs:0efe7h]                          ; 2e 0f 01 1e e7 ef
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
biosorg_check_before_or_at_0E3FCh:           ; 0xfe38c LB 0x72
    times 0x70 db 0
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
    cld                                       ; fc
    add byte [bx+di], al                      ; 00 01
    je short 0e73ch                           ; 74 40
    times 0x3 db 0
biosorg_check_before_or_at_0E727h:           ; 0xfe6ff LB 0x2a
    times 0x28 db 0
    db  'XM'
biosorg_check_at_0E729h:                     ; 0xfe729 LB 0x10
    times 0xe db 0
    db  'XM'
biosorg_check_at_0E739h:                     ; 0xfe739 LB 0x1a
    push DS                                   ; 1e
    push ES                                   ; 06
    pushaw                                    ; 60
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 06515h                               ; e8 d3 7d
    popaw                                     ; 61
    pop ES                                    ; 07
    pop DS                                    ; 1f
    iret                                      ; cf
    push DS                                   ; 1e
    push ES                                   ; 06
    pushaw                                    ; 60
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 017e1h                               ; e8 92 30
    popaw                                     ; 61
    pop ES                                    ; 07
    pop DS                                    ; 1f
    iret                                      ; cf
init_pic:                                    ; 0xfe753 LB 0x25
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
ebda_post:                                   ; 0xfe778 LB 0x51
    mov ax, 0e746h                            ; b8 46 e7
    mov word [00034h], ax                     ; a3 34 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [00036h], ax                     ; a3 36 00
    mov ax, 0e746h                            ; b8 46 e7
    mov word [0003ch], ax                     ; a3 3c 00
    mov ax, 0f000h                            ; b8 00 f0
    mov word [0003eh], ax                     ; a3 3e 00
    mov ax, 0e746h                            ; b8 46 e7
    mov word [001c8h], ax                     ; a3 c8 01
    mov ax, 0f000h                            ; b8 00 f0
    mov word [001cah], ax                     ; a3 ca 01
    mov ax, 0e746h                            ; b8 46 e7
    mov word [001cch], ax                     ; a3 cc 01
    mov ax, 0f000h                            ; b8 00 f0
    mov word [001ceh], ax                     ; a3 ce 01
    mov ax, 0e746h                            ; b8 46 e7
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
biosorg_check_before_or_at_0E82Ch:           ; 0xfe7c9 LB 0x65
    times 0x63 db 0
    db  'XM'
biosorg_check_at_0E82Eh:                     ; 0xfe82e LB 0x3d
    sti                                       ; fb
    pushfw                                    ; 9c
    push ES                                   ; 06
    push DS                                   ; 1e
    pushaw                                    ; 60
    cmp ah, 000h                              ; 80 fc 00
    je short 0e84ah                           ; 74 12
    cmp ah, 010h                              ; 80 fc 10
    je short 0e84ah                           ; 74 0d
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 05803h                               ; e8 c0 6f
    popaw                                     ; 61
    pop DS                                    ; 1f
    pop ES                                    ; 07
    add sp, strict byte 00002h                ; 83 c4 02
    iret                                      ; cf
    mov bx, strict word 00040h                ; bb 40 00
    mov ds, bx                                ; 8e db
    cli                                       ; fa
    mov bx, word [word 0001ah]                ; 8b 1e 1a 00
    cmp bx, word [word 0001ch]                ; 3b 1e 1c 00
    jne short 0e85eh                          ; 75 04
    sti                                       ; fb
    nop                                       ; 90
    jmp short 0e84fh                          ; eb f1
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 05803h                               ; e8 9f 6f
    popaw                                     ; 61
    pop DS                                    ; 1f
    pop ES                                    ; 07
    add sp, strict byte 00002h                ; 83 c4 02
    iret                                      ; cf
pmode_enter:                                 ; 0xfe86b LB 0x1b
    push CS                                   ; 0e
    pop DS                                    ; 1f
    lgdt [cs:0e899h]                          ; 2e 0f 01 16 99 e8
    mov eax, cr0                              ; 0f 20 c0
    or AL, strict byte 001h                   ; 0c 01
    mov cr0, eax                              ; 0f 22 c0
    jmp far 00020h:0e880h                     ; ea 80 e8 20 00
    mov ax, strict word 00018h                ; b8 18 00
    mov ds, ax                                ; 8e d8
    retn                                      ; c3
pmode_exit:                                  ; 0xfe886 LB 0x13
    mov ax, strict word 00040h                ; b8 40 00
    mov ds, ax                                ; 8e d8
    mov eax, cr0                              ; 0f 20 c0
    and AL, strict byte 0feh                  ; 24 fe
    mov cr0, eax                              ; 0f 22 c0
    jmp far 0f000h:0e898h                     ; ea 98 e8 00 f0
    retn                                      ; c3
pmbios_gdt_desc:                             ; 0xfe899 LB 0x6
    db  047h, 000h, 09fh, 0e8h, 00fh, 000h
pmbios_gdt:                                  ; 0xfe89f LB 0x48
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  0ffh, 0ffh, 000h, 000h, 000h, 09bh, 0cfh, 000h, 0ffh, 0ffh, 000h, 000h, 000h, 093h, 0cfh, 000h
    db  0ffh, 0ffh, 000h, 000h, 00fh, 09bh, 000h, 000h, 0ffh, 0ffh, 000h, 000h, 000h, 093h, 000h, 000h
    db  000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
    db  0ffh, 0ffh, 000h, 004h, 000h, 093h, 000h, 000h
pmode_setup:                                 ; 0xfe8e7 LB 0x5c
    push eax                                  ; 66 50
    push esi                                  ; 66 56
    pushfw                                    ; 9c
    cli                                       ; fa
    call 0e86bh                               ; e8 7b ff
    mov eax, cr0                              ; 0f 20 c0
    and eax, strict dword 09fffffffh          ; 66 25 ff ff ff 9f
    mov cr0, eax                              ; 0f 22 c0
    mov esi, strict dword 0fee000f0h          ; 66 be f0 00 e0 fe
    mov eax, strict dword 00000010fh          ; 66 b8 0f 01 00 00
    mov dword [esi], eax                      ; 67 66 89 06
    mov esi, strict dword 0fee00350h          ; 66 be 50 03 e0 fe
    mov eax, dword [esi]                      ; 67 66 8b 06
    and eax, strict dword 0fffe00ffh          ; 66 25 ff 00 fe ff
    or ah, 007h                               ; 80 cc 07
    mov dword [esi], eax                      ; 67 66 89 06
    mov esi, strict dword 0fee00360h          ; 66 be 60 03 e0 fe
    mov eax, dword [esi]                      ; 67 66 8b 06
    and eax, strict dword 0fffe00ffh          ; 66 25 ff 00 fe ff
    or ah, 004h                               ; 80 cc 04
    mov dword [esi], eax                      ; 67 66 89 06
    call 0e886h                               ; e8 49 ff
    popfw                                     ; 9d
    pop esi                                   ; 66 5e
    pop eax                                   ; 66 58
    retn                                      ; c3
biosorg_check_before_or_at_0E985h:           ; 0xfe943 LB 0x44
    times 0x42 db 0
    db  'XM'
biosorg_check_at_0E987h:                     ; 0xfe987 LB 0x52
    cli                                       ; fa
    push ax                                   ; 50
    mov AL, strict byte 0adh                  ; b0 ad
    out strict byte 064h, AL                  ; e6 64
    in AL, strict byte 060h                   ; e4 60
    push DS                                   ; 1e
    pushaw                                    ; 60
    cld                                       ; fc
    mov AH, strict byte 04fh                  ; b4 4f
    stc                                       ; f9
    int 015h                                  ; cd 15
    jnc short 0e9c0h                          ; 73 27
    sti                                       ; fb
    cmp AL, strict byte 0e0h                  ; 3c e0
    jne short 0e9a9h                          ; 75 0b
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    mov ds, ax                                ; 8e d8
    or byte [00496h], 002h                    ; 80 0e 96 04 02
    jmp short 0e9c0h                          ; eb 17
    cmp AL, strict byte 0e1h                  ; 3c e1
    jne short 0e9b8h                          ; 75 0b
    db  033h, 0c0h
    ; xor ax, ax                                ; 33 c0
    mov ds, ax                                ; 8e d8
    or byte [00496h], 001h                    ; 80 0e 96 04 01
    jmp short 0e9c0h                          ; eb 08
    push ES                                   ; 06
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 052fbh                               ; e8 3c 69
    pop ES                                    ; 07
    popaw                                     ; 61
    pop DS                                    ; 1f
    cli                                       ; fa
    call 0e034h                               ; e8 6e f6
    mov AL, strict byte 0aeh                  ; b0 ae
    out strict byte 064h, AL                  ; e6 64
    pop ax                                    ; 58
    iret                                      ; cf
    pushaw                                    ; 60
    push ES                                   ; 06
    push DS                                   ; 1e
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 06fdch                               ; e8 07 86
    pop DS                                    ; 1f
    pop ES                                    ; 07
    popaw                                     ; 61
    iret                                      ; cf
biosorg_check_before_or_at_0EC57h:           ; 0xfe9d9 LB 0x280
    times 0x27e db 0
    db  'XM'
biosorg_check_at_0EC59h:                     ; 0xfec59 LB 0x2
    jmp short 0ecb0h                          ; eb 55
int13_relocated:                             ; 0xfec5b LB 0x55
    cmp ah, 04ah                              ; 80 fc 4a
    jc short 0ec71h                           ; 72 11
    cmp ah, 04dh                              ; 80 fc 4d
    jnbe short 0ec71h                         ; 77 0c
    pushaw                                    ; 60
    push ES                                   ; 06
    push DS                                   ; 1e
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    push 0ece9h                               ; 68 e9 ec
    jmp near 03d1fh                           ; e9 ae 50
    push ES                                   ; 06
    push ax                                   ; 50
    push bx                                   ; 53
    push cx                                   ; 51
    push dx                                   ; 52
    call 03cf3h                               ; e8 7a 50
    cmp AL, strict byte 000h                  ; 3c 00
    je short 0ecabh                           ; 74 2e
    call 03d09h                               ; e8 89 50
    pop dx                                    ; 5a
    push dx                                   ; 52
    db  03ah, 0c2h
    ; cmp al, dl                                ; 3a c2
    jne short 0ec97h                          ; 75 11
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop ax                                    ; 58
    pop ES                                    ; 07
    pushaw                                    ; 60
    push ES                                   ; 06
    push DS                                   ; 1e
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    push 0ece9h                               ; 68 e9 ec
    jmp near 042f4h                           ; e9 5d 56
    and dl, 0e0h                              ; 80 e2 e0
    db  03ah, 0c2h
    ; cmp al, dl                                ; 3a c2
    jne short 0ecabh                          ; 75 0d
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
    jmp short 0ecb4h                          ; eb 09
    pop dx                                    ; 5a
    pop cx                                    ; 59
    pop bx                                    ; 5b
    pop ax                                    ; 58
    pop ES                                    ; 07
int13_noeltorito:                            ; 0xfecb0 LB 0x4
    push ax                                   ; 50
    push cx                                   ; 51
    push dx                                   ; 52
    push bx                                   ; 53
int13_legacy:                                ; 0xfecb4 LB 0x14
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
    jne short 0ecc8h                          ; 75 06
    push 0ece9h                               ; 68 e9 ec
    jmp near 03321h                           ; e9 59 46
int13_notfloppy:                             ; 0xfecc8 LB 0x14
    cmp dl, 0e0h                              ; 80 fa e0
    jc short 0ecdch                           ; 72 0f
    shr ebx, 010h                             ; 66 c1 eb 10
    push bx                                   ; 53
    call 04728h                               ; e8 53 5a
    pop bx                                    ; 5b
    sal ebx, 010h                             ; 66 c1 e3 10
    jmp short 0ece9h                          ; eb 0d
int13_disk:                                  ; 0xfecdc LB 0xd
    cmp ah, 040h                              ; 80 fc 40
    jnbe short 0ece6h                         ; 77 05
    call 05bd3h                               ; e8 ef 6e
    jmp short 0ece9h                          ; eb 03
    call 06021h                               ; e8 38 73
int13_out:                                   ; 0xfece9 LB 0x4
    pop DS                                    ; 1f
    pop ES                                    ; 07
    popaw                                     ; 61
    iret                                      ; cf
detect_parport:                              ; 0xfeced LB 0x1e
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
    jne short 0ed0ah                          ; 75 0d
    push bx                                   ; 53
    sal bx, 1                                 ; d1 e3
    mov word [bx+00408h], dx                  ; 89 97 08 04
    pop bx                                    ; 5b
    mov byte [bx+00478h], cl                  ; 88 8f 78 04
    inc bx                                    ; 43
    retn                                      ; c3
detect_serial:                               ; 0xfed0b LB 0x24
    push dx                                   ; 52
    inc dx                                    ; 42
    mov AL, strict byte 002h                  ; b0 02
    out DX, AL                                ; ee
    in AL, DX                                 ; ec
    cmp AL, strict byte 002h                  ; 3c 02
    jne short 0ed2dh                          ; 75 18
    inc dx                                    ; 42
    in AL, DX                                 ; ec
    cmp AL, strict byte 002h                  ; 3c 02
    jne short 0ed2dh                          ; 75 12
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
floppy_post:                                 ; 0xfed2f LB 0x87
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
    shr al, 004h                              ; c0 e8 04
    je short 0ed6ah                           ; 74 04
    mov BL, strict byte 007h                  ; b3 07
    jmp short 0ed6ch                          ; eb 02
    mov BL, strict byte 000h                  ; b3 00
    db  08ah, 0c4h
    ; mov al, ah                                ; 8a c4
    and AL, strict byte 00fh                  ; 24 0f
    je short 0ed75h                           ; 74 03
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
bcd_to_bin:                                  ; 0xfedb6 LB 0x9
    sal ax, 004h                              ; c1 e0 04
    shr al, 004h                              ; c0 e8 04
    aad 00ah                                  ; d5 0a
    retn                                      ; c3
rtc_post:                                    ; 0xfedbf LB 0x77
    db  066h, 033h, 0c0h
    ; xor eax, eax                              ; 66 33 c0
    mov AL, strict byte 000h                  ; b0 00
    out strict byte 070h, AL                  ; e6 70
    in AL, strict byte 071h                   ; e4 71
    call 0edb6h                               ; e8 eb ff
    mov edx, strict dword 00115cf2bh          ; 66 ba 2b cf 15 01
    mul edx                                   ; 66 f7 e2
    mov ebx, strict dword 0000f4240h          ; 66 bb 40 42 0f 00
    db  066h, 033h, 0d2h
    ; xor edx, edx                              ; 66 33 d2
    div ebx                                   ; 66 f7 f3
    db  066h, 08bh, 0c8h
    ; mov ecx, eax                              ; 66 8b c8
    db  066h, 033h, 0c0h
    ; xor eax, eax                              ; 66 33 c0
    mov AL, strict byte 002h                  ; b0 02
    out strict byte 070h, AL                  ; e6 70
    in AL, strict byte 071h                   ; e4 71
    call 0edb6h                               ; e8 c7 ff
    mov edx, strict dword 000a6af80h          ; 66 ba 80 af a6 00
    mul edx                                   ; 66 f7 e2
    mov ebx, strict dword 000002710h          ; 66 bb 10 27 00 00
    db  066h, 033h, 0d2h
    ; xor edx, edx                              ; 66 33 d2
    div ebx                                   ; 66 f7 f3
    db  066h, 003h, 0c8h
    ; add ecx, eax                              ; 66 03 c8
    db  066h, 033h, 0c0h
    ; xor eax, eax                              ; 66 33 c0
    mov AL, strict byte 004h                  ; b0 04
    out strict byte 070h, AL                  ; e6 70
    in AL, strict byte 071h                   ; e4 71
    call 0edb6h                               ; e8 a3 ff
    mov edx, strict dword 003e81d03h          ; 66 ba 03 1d e8 03
    mul edx                                   ; 66 f7 e2
    mov ebx, strict dword 0000003e8h          ; 66 bb e8 03 00 00
    db  066h, 033h, 0d2h
    ; xor edx, edx                              ; 66 33 d2
    div ebx                                   ; 66 f7 f3
    db  066h, 003h, 0c8h
    ; add ecx, eax                              ; 66 03 c8
    mov dword [0046ch], ecx                   ; 66 89 0e 6c 04
    db  032h, 0c0h
    ; xor al, al                                ; 32 c0
    mov byte [00470h], AL                     ; a2 70 04
    retn                                      ; c3
biosorg_check_before_or_at_0EF55h:           ; 0xfee36 LB 0x121
    times 0x11f db 0
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
int17_handler:                               ; 0xfefd4 LB 0xd
    push DS                                   ; 1e
    push ES                                   ; 06
    pushaw                                    ; 60
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 07957h                               ; e8 7a 89
    popaw                                     ; 61
    pop ES                                    ; 07
    pop DS                                    ; 1f
    iret                                      ; cf
_pmode_IDT:                                  ; 0xfefe1 LB 0x6
    db  000h, 000h, 000h, 000h, 00fh, 000h
_rmode_IDT:                                  ; 0xfefe7 LB 0x6
    db  0ffh, 003h, 000h, 000h, 000h, 000h
int1c_handler:                               ; 0xfefed LB 0x1
    iret                                      ; cf
biosorg_check_before_or_at_0F043h:           ; 0xfefee LB 0x57
    times 0x55 db 0
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
    call 0185eh                               ; e8 b4 27
    hlt                                       ; f4
    iret                                      ; cf
int19_relocated:                             ; 0xff0ac LB 0x8f
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
    call 04d8fh                               ; e8 bf 5c
    inc sp                                    ; 44
    inc sp                                    ; 44
    test ax, ax                               ; 85 c0
    jne short 0f0fdh                          ; 75 27
    mov ax, strict word 00002h                ; b8 02 00
    push ax                                   ; 50
    call 04d8fh                               ; e8 b2 5c
    inc sp                                    ; 44
    inc sp                                    ; 44
    test ax, ax                               ; 85 c0
    jne short 0f0fdh                          ; 75 1a
    mov ax, strict word 00003h                ; b8 03 00
    push ax                                   ; 50
    call 04d8fh                               ; e8 a5 5c
    inc sp                                    ; 44
    inc sp                                    ; 44
    test ax, ax                               ; 85 c0
    jne short 0f0fdh                          ; 75 0d
    mov ax, strict word 00004h                ; b8 04 00
    push ax                                   ; 50
    call 04d8fh                               ; e8 98 5c
    inc sp                                    ; 44
    inc sp                                    ; 44
    test ax, ax                               ; 85 c0
    je short 0f0a4h                           ; 74 a7
    sal eax, 004h                             ; 66 c1 e0 04
    mov word [bp+002h], ax                    ; 89 46 02
    shr eax, 004h                             ; 66 c1 e8 04
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
pcibios_init_iomem_bases:                    ; 0xff13b LB 0x12
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
pcibios_init_set_elcr:                       ; 0xff14d LB 0xc
    push ax                                   ; 50
    push cx                                   ; 51
    mov dx, 004d0h                            ; ba d0 04
    test AL, strict byte 008h                 ; a8 08
    je short 0f159h                           ; 74 03
    inc dx                                    ; 42
    and AL, strict byte 007h                  ; 24 07
is_master_pic:                               ; 0xff159 LB 0xd
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
pcibios_init_irqs:                           ; 0xff166 LB 0x2a
    push DS                                   ; 1e
    push bp                                   ; 55
    mov ax, 0f000h                            ; b8 00 f0
    mov ds, ax                                ; 8e d8
    pop bp                                    ; 5d
    pop DS                                    ; 1f
    retn                                      ; c3
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
_pci_routing_table:                          ; 0xff190 LB 0x1e0
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
_pci_routing_table_size:                     ; 0xff370 LB 0x2
    loopne 0f373h                             ; e0 01
biosorg_check_before_or_at_0F83Fh:           ; 0xff372 LB 0x4cf
    times 0x4cd db 0
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
int15_handler:                               ; 0xff859 LB 0x40
    cmp ah, 087h                              ; 80 fc 87
    jne short 0f86bh                          ; 75 0d
    pushaw                                    ; 60
    push ES                                   ; 06
    push DS                                   ; 1e
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 06e33h                               ; e8 cc 75
    pop DS                                    ; 1f
    pop ES                                    ; 07
    popaw                                     ; 61
    iret                                      ; cf
    pushfw                                    ; 9c
    push DS                                   ; 1e
    push ES                                   ; 06
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    cmp ah, 086h                              ; 80 fc 86
    je short 0f89eh                           ; 74 28
    cmp ah, 0e8h                              ; 80 fc e8
    je short 0f89eh                           ; 74 23
    cmp ah, 0d0h                              ; 80 fc d0
    je short 0f89eh                           ; 74 1e
    pushaw                                    ; 60
    cmp ah, 053h                              ; 80 fc 53
    je short 0f894h                           ; 74 0e
    cmp ah, 0c2h                              ; 80 fc c2
    je short 0f899h                           ; 74 0e
    call 06767h                               ; e8 d9 6e
    popaw                                     ; 61
    pop ES                                    ; 07
    pop DS                                    ; 1f
    popfw                                     ; 9d
    jmp short 0f8a7h                          ; eb 13
    call 09addh                               ; e8 46 a2
    jmp short 0f88eh                          ; eb f5
int15_handler_mouse:                         ; 0xff899 LB 0x5
    call 075cch                               ; e8 30 7d
    jmp short 0f88eh                          ; eb f0
int15_handler32:                             ; 0xff89e LB 0x9
    pushad                                    ; 66 60
    call 06a9dh                               ; e8 fa 71
    popad                                     ; 66 61
    jmp short 0f88fh                          ; eb e8
iret_modify_cf:                              ; 0xff8a7 LB 0x1a
    jc short 0f8b7h                           ; 72 0e
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
int74_handler:                               ; 0xff8c1 LB 0x2b
    sti                                       ; fb
    pushaw                                    ; 60
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
    call 07502h                               ; e8 30 7c
    pop cx                                    ; 59
    jcxz 0f8e1h                               ; e3 0c
    push strict byte 00000h                   ; 6a 00
    pop DS                                    ; 1f
    push word [0040eh]                        ; ff 36 0e 04
    pop DS                                    ; 1f
    call far [word 00022h]                    ; ff 1e 22 00
    cli                                       ; fa
    call 0e030h                               ; e8 4b e7
    add sp, strict byte 00008h                ; 83 c4 08
    pop DS                                    ; 1f
    pop ES                                    ; 07
    popaw                                     ; 61
    iret                                      ; cf
int76_handler:                               ; 0xff8ec LB 0x12
    push ax                                   ; 50
    push DS                                   ; 1e
    mov ax, strict word 00040h                ; b8 40 00
    mov ds, ax                                ; 8e d8
    mov byte [0008eh], 0ffh                   ; c6 06 8e 00 ff
    call 0e030h                               ; e8 35 e7
    pop DS                                    ; 1f
    pop ax                                    ; 58
    iret                                      ; cf
int70_handler:                               ; 0xff8fe LB 0xd
    push ES                                   ; 06
    push DS                                   ; 1e
    pushaw                                    ; 60
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 071bah                               ; e8 b3 78
    popaw                                     ; 61
    pop DS                                    ; 1f
    pop ES                                    ; 07
    iret                                      ; cf
biosorg_check_before_or_at_0FA6Ch:           ; 0xff90b LB 0x163
    times 0x161 db 0
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
biosorg_check_at_0FE6Eh:                     ; 0xffe6e LB 0x21
    cmp ah, 0b1h                              ; 80 fc b1
    jne short 0fe82h                          ; 75 0f
    push ES                                   ; 06
    push DS                                   ; 1e
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    pushad                                    ; 66 60
    call 09cedh                               ; e8 70 9e
    popad                                     ; 66 61
    pop DS                                    ; 1f
    pop ES                                    ; 07
    iret                                      ; cf
    push ES                                   ; 06
    push DS                                   ; 1e
    pushaw                                    ; 60
    push CS                                   ; 0e
    pop DS                                    ; 1f
    cld                                       ; fc
    call 07278h                               ; e8 ed 73
    popaw                                     ; 61
    pop DS                                    ; 1f
    pop ES                                    ; 07
    iret                                      ; cf
biosorg_check_before_or_at_0FEA3h:           ; 0xffe8f LB 0x16
    times 0x14 db 0
    db  'XM'
int08_handler:                               ; 0xffea5 LB 0x43
    sti                                       ; fb
    push eax                                  ; 66 50
    push DS                                   ; 1e
    push dx                                   ; 52
    mov ax, strict word 00040h                ; b8 40 00
    mov ds, ax                                ; 8e d8
    mov eax, dword [0006ch]                   ; 66 a1 6c 00
    inc eax                                   ; 66 40
    cmp eax, strict dword 0001800b0h          ; 66 3d b0 00 18 00
    jc short 0fec4h                           ; 72 07
    db  066h, 033h, 0c0h
    ; xor eax, eax                              ; 66 33 c0
    inc byte [word 00070h]                    ; fe 06 70 00
    mov dword [0006ch], eax                   ; 66 a3 6c 00
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
    pop eax                                   ; 66 58
    iret                                      ; cf
biosorg_check_before_or_at_0FEF1h:           ; 0xffee8 LB 0xb
    times 0x9 db 0
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
    db  030h, 036h, 02fh, 032h, 033h, 02fh, 039h, 039h, 000h, 0fch, 0edh
