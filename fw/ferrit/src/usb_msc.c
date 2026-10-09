/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 * Copyright (c) 2025 Lone Dynamics Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb.h"

#include "ferrit.h"
#include "mapper.h"
#include "drv_fram.h"

enum {
  DISK_BLOCK_SIZE = 512
};

// Invoked to determine max LUN
uint8_t tud_msc_get_maxlun_cb(void) {
  return (uint8_t)mapper_luns();
}

// Invoked when received SCSI_CMD_INQUIRY
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
  (void) lun;

  const char vid[] = "Machdyne";
  const char pid[] = "FERRIT";
  const char rev[] = "1.0";

  memcpy(vendor_id,   vid, strlen(vid));
  memcpy(product_id,  pid, strlen(pid));
  memcpy(product_rev, rev, strlen(rev));
}

// Invoked when received Test Unit Ready command.
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
  if (mapper_lun_size(lun) == 0) {
    tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3A, 0x00);
    return false;
  }
  return true;
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
  *block_count = (uint32_t)(mapper_lun_size(lun) / DISK_BLOCK_SIZE);
  *block_size  = DISK_BLOCK_SIZE;
}

// Invoked when received Start Stop Unit command
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
  (void) lun;
  (void) power_condition;
  (void) start;
  (void) load_eject;
  return true;
}

// Callback invoked when received READ10 command.
// Handles transfers that may span multiple chips by splitting into per-chip chunks.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {

  uint32_t lun_size = (uint32_t)mapper_lun_size(lun);

  if (lba >= (lun_size / DISK_BLOCK_SIZE)) {
    tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x21, 0x00);
    return -1;
  }

  uint32_t lun_addr = (lba * DISK_BLOCK_SIZE) + offset;
  uint32_t remaining = bufsize;
  uint8_t *buf_ptr = (uint8_t *)buffer;

  while (remaining > 0) {

    int rec = mapper_rec(lun, lun_addr);
    if (rec < 0) {
      tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x21, 0x00);
      return -1;
    }

    int ss = mapper_get_ss(lun, lun_addr);
    int cs = mapper_get_cs(lun, lun_addr);
    uint32_t local_addr = mapper_get_local_addr(lun, lun_addr);
    int avail = mapper_remaining(lun, lun_addr);

    uint32_t chunk = (remaining < (uint32_t)avail) ? remaining : (uint32_t)avail;

    fram_read(ss, cs, (char *)buf_ptr, (int)local_addr, (int)chunk);

    buf_ptr  += chunk;
    lun_addr += chunk;
    remaining -= chunk;

  }

  return (int32_t)bufsize;

}

bool tud_msc_is_writable_cb(uint8_t lun) {
  (void) lun;
  return true;
}

// Callback invoked when received WRITE10 command.
// Handles transfers that may span multiple chips by splitting into per-chip chunks.
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {

  uint32_t lun_size = (uint32_t)mapper_lun_size(lun);

  if (lba >= (lun_size / DISK_BLOCK_SIZE)) {
    tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x21, 0x00);
    return -1;
  }

  uint32_t lun_addr = (lba * DISK_BLOCK_SIZE) + offset;
  uint32_t remaining = bufsize;
  uint8_t *buf_ptr = buffer;

  while (remaining > 0) {

    int rec = mapper_rec(lun, lun_addr);
    if (rec < 0) {
      tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x21, 0x00);
      return -1;
    }

    int ss = mapper_get_ss(lun, lun_addr);
    int cs = mapper_get_cs(lun, lun_addr);
    uint32_t local_addr = mapper_get_local_addr(lun, lun_addr);
    int avail = mapper_remaining(lun, lun_addr);

    uint32_t chunk = (remaining < (uint32_t)avail) ? remaining : (uint32_t)avail;

    fram_write(ss, cs, (int)local_addr, (char *)buf_ptr, (int)chunk);

    buf_ptr  += chunk;
    lun_addr += chunk;
    remaining -= chunk;

  }

  return (int32_t)bufsize;

}

// Callback invoked when received an SCSI command not in the built-in list
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize) {
  (void) buffer;
  (void) bufsize;

  tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
  return -1;
}
