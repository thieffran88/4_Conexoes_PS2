/*
 * Módulos do IOP embutidos no ELF.
 * AUDSRV_IRX é definido no Makefile com o caminho do audsrv.irx do PS2SDK.
 */
#ifndef AUDSRV_IRX
#error "defina AUDSRV_IRX com o caminho de audsrv.irx"
#endif

__asm__(
    ".section .data\n"
    ".balign 64\n"
    ".global audsrv_irx\n"
    "audsrv_irx:\n"
    ".incbin \"" AUDSRV_IRX "\"\n"
    ".global audsrv_irx_end\n"
    "audsrv_irx_end:\n"
    ".previous\n"
);
