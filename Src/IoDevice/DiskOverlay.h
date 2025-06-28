#ifndef DISKOVERLAY_H
#define DISKOVERLAY_H

#include <stddef.h>
#include <stdint.h>

typedef struct DiskOverlayEntry
{
   int sector;
   uint8_t* data;
   struct DiskOverlayEntry* next;
} DiskOverlayEntry;

typedef struct DiskOverlay 
{
   DiskOverlayEntry* head;
   size_t sectorSize;
} DiskOverlay;

DiskOverlay* diskOverlayCreate(size_t sectorSize);
void diskOverlayFree(DiskOverlay* overlay);
void diskOverlayWrite(DiskOverlay* overlay, int sector, const uint8_t* data);
int diskOverlayRead(DiskOverlay* overlay, int sector, uint8_t* out);
int diskOverlaySerialize(DiskOverlay* overlay, const char* path);
DiskOverlay* diskOverlayDeserialize(const char* path, size_t sectorSize);
int diskOverlayIsEmpty(DiskOverlay* overlay);

#endif