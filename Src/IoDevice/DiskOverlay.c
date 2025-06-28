#include "DiskOverlay.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

DiskOverlay* diskOverlayCreate(size_t sectorSize)
{
    DiskOverlay* ov = (DiskOverlay*)calloc(1, sizeof(DiskOverlay));
    if (ov)
        ov->sectorSize = sectorSize;
    return ov;
}

void diskOverlayFree(DiskOverlay* overlay)
{
    DiskOverlayEntry* e;
    DiskOverlayEntry* next;

    if (!overlay)
        return;

    e = overlay->head;

    while (e) {
        next = e->next;
        free(e->data);
        free(e);
        e = next;
    }
    free(overlay);
}

void diskOverlayWrite(DiskOverlay* overlay, int sector, const uint8_t* data)
{
    if (!overlay)
        return;

    DiskOverlayEntry* prev = NULL;
    DiskOverlayEntry* e = overlay->head;
    while (e)
    {
        if (e->sector == sector)
        {
            memcpy(e->data, data, overlay->sectorSize);
            // Optionally move to head for LRU, but not required
            return;
        }
        prev = e;
        e = e->next;
    }

    e = (DiskOverlayEntry*)malloc(sizeof(DiskOverlayEntry));
    if (!e)
        return;
    e->sector = sector;
    e->data = (uint8_t*)malloc(overlay->sectorSize);
    if (!e->data)
    {
        free(e);
        return;
    }
    memcpy(e->data, data, overlay->sectorSize);
    e->next = overlay->head;
    overlay->head = e;
}

int diskOverlayRead(DiskOverlay* overlay, int sector, uint8_t* out)
{
    DiskOverlayEntry* e;
    if (!overlay)
        return 0;

    e = overlay->head;
    while (e)
    {
        if (e->sector == sector)
        {
            memcpy(out, e->data, overlay->sectorSize);
            return 1;
        }
        e = e->next;
    }

    return 0;
}

int diskOverlayIsEmpty(DiskOverlay* overlay)
{
    return (overlay == NULL || overlay->head == NULL);
}

int diskOverlaySerialize(DiskOverlay* overlay, const char* path)
{
    FILE* f;
    DiskOverlayEntry* e;

    if (!overlay || !path)
        return 0;
    
    f = fopen(path, "wb");
    if (!f)
        return 0;

    e = overlay->head;
    while (e)
    {
        fwrite(&e->sector, sizeof(int), 1, f);
        fwrite(e->data, 1, overlay->sectorSize, f);
        e = e->next;
    }

    fclose(f);
    return 1;
}

DiskOverlay* diskOverlayDeserialize(const char* path, size_t sectorSize)
{
    FILE* f;
    DiskOverlay* ov;
    int sector;
    uint8_t* buf;
    size_t n;

    if (!path)
        return NULL;
    
    f = fopen(path, "rb");
    if (!f)
        return NULL;

    ov = diskOverlayCreate(sectorSize);
    if (!ov)
    {
        fclose(f);
        return NULL;
    }

    buf = (uint8_t*)malloc(sectorSize);
    if (!buf)
    {
        fclose(f);
        diskOverlayFree(ov);
        return NULL;
    }

    while (fread(&sector, sizeof(int), 1, f) == 1)
    {
        n = fread(buf, 1, sectorSize, f);
        if (n != sectorSize)
            break;
        // Remove any previous entry for this sector before writing
        DiskOverlayEntry* prev = NULL;
        DiskOverlayEntry* e = ov->head;
        while (e) {
            if (e->sector == sector) {
                if (prev) prev->next = e->next;
                else ov->head = e->next;
                free(e->data);
                free(e);
                break;
            }
            prev = e;
            e = e->next;
        }
        diskOverlayWrite(ov, sector, buf);
    }

    free(buf);
    fclose(f);
    return ov;
}