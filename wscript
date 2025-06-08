

from __future__ import print_function
import rtems_waf.rtems as rtems

rtems_version = "6"

def init(ctx):
    rtems.init(ctx, version=rtems_version, long_commands=True)

def bsp_configure(conf, arch_bsp):
    pass

def options(opt):
    rtems.options(opt)

def configure(conf):
    rtems.configure(conf, bsp_configure=bsp_configure)

def build(bld):
    rtems.build(bld)

    bld.program(
        features='c cprogram',
        target='gpio_blink.exe',
        cflags='-g -O2',
        source=['main.c', 'init.c'],
        use=['rtems']
    )
