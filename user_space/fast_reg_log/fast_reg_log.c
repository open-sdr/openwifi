// Author: Xianjun Jiao (xianjun.jiao@imec.be; putaoshu@msn.com)
// SPDX-FileCopyrightText: 2023 UGent
// SPDX-License-Identifier: AGPL-3.0-or-later

// Use this example together with fast_reg_log_analyzer.m (notter release)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>

int main()
{
  unsigned int bram_size = 0x10000; // 64KB, aligned with openwifi hw .bd and devicetree
  off_t bram_pbase = 0x83c40000;    // physical base address, aligned with openwifi hw .bd and devicetree (this example: xpu @ 32bit boards)
  uint32_t *bram32_vptr;
  int fd, i, j;
  uint32_t tsf_reg[524288*2];
  FILE *fp;
  // Map the BRAM physical address into user space getting a virtual address for it
  if ((fd = open("/dev/mem", O_RDONLY | O_SYNC)) != -1) {
    bram32_vptr = (uint32_t *)mmap(NULL, bram_size, PROT_READ, MAP_SHARED, fd, bram_pbase);
    
    fp = fopen ("fast_reg_log.bin", "wb");
    if (fp == NULL) {
      printf("fopen fast_reg_log.bin failed! %d\n", (int)fp);
      close(fd);
      return(0);
    }

    for (j=0; j<10; j++) {
      for (i=0; i<(524288*2); i=i+2) {
        tsf_reg[i+0] = (*(bram32_vptr+57)); // read xpu register 57: rssi trx agc cca status
        tsf_reg[i+1] = (*(bram32_vptr+58)); // read xpu register 58: low 32bit of tsf
      }

      // for (i=0; i<1024; i++) {
      //   printf("%d %x\n", tsf[i], reg[i]);
      // }
      // memcpy(buf, bram64_vptr, bram_size);

      fwrite(tsf_reg, sizeof(uint32_t), 524288*2, fp);
    }

    fclose(fp);
    // printf("%016llx\n", buf[65532]);
    // printf("%016llx\n", buf[65533]);
    // printf("%016llx\n", buf[65534]);
    // printf("%016llx\n", buf[65535]);
    // //for(i=0; i<32; i++) {
    // //    printf("0x%02x\n", buf[i]);
    // //}

    close(fd);
  }
  return(0);
}
