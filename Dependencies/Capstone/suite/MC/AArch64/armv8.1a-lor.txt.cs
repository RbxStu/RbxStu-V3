# CS_ARCH_AARCH64, None, None
# This regression test file is new. The option flags could not be determined.
# LLVM uses the following mattr = ['mattr=+v8.1a']
0x20,0x7c,0xdf,0x08 == ldlarb w0, [x1]
0x20,0x7c,0xdf,0x48 == ldlarh w0, [x1]
0x20,0x7c,0xdf,0x88 == ldlar  w0, [x1]
0x20,0x7c,0xdf,0xc8 == ldlar  x0, [x1]
0x20,0x7c,0x9f,0x08 == stllrb w0, [x1]
0x20,0x7c,0x9f,0x48 == stllrh w0, [x1]
0x20,0x7c,0x9f,0x88 == stllr  w0, [x1]
0x20,0x7c,0x9f,0xc8 == stllr  x0, [x1]
0x00,0xa4,0x18,0xd5 == msr LORSA_EL1, x0
0x20,0xa4,0x18,0xd5 == msr LOREA_EL1, x0
0x40,0xa4,0x18,0xd5 == msr LORN_EL1, x0
0x60,0xa4,0x18,0xd5 == msr LORC_EL1, x0
0xe0,0xa4,0x18,0xd5 == msr S3_0_C10_C4_7, x0
0x00,0xa4,0x38,0xd5 == mrs x0, LORSA_EL1
0x20,0xa4,0x38,0xd5 == mrs x0, LOREA_EL1
0x40,0xa4,0x38,0xd5 == mrs x0, LORN_EL1
0x60,0xa4,0x38,0xd5 == mrs x0, LORC_EL1
0xe0,0xa4,0x38,0xd5 == mrs x0, LORID_EL1
