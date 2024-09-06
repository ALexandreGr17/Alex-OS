#include "fat.h"
#include <disk/disk.h>
#include <memory/memory.h>
#include <stdint.h>
#include <string/string.h>

#define SECTOR_SIZE         512
#define MAX_PATH_SIZE           256
#define FAT_CACHE_SIZE      5
#define MAX_FILE_HANDLES    10
#define ROOT_DIRECTORY_HANDLE -1

typedef struct {
    uint8_t drive_number;
    uint8_t _reserved;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t system_id[8];
} __attribute__((packed)) ebpb_t;

typedef struct {
    uint32_t sector_per_fat;
    uint16_t flags;
    uint16_t fat_version;
    uint32_t root_dir_cluster;
    uint16_t fs_info_cluster;
    uint16_t backup_boot_sector;
    uint8_t _reserved[12];
} __attribute__((packed)) ebpb_fat32_t;

typedef struct 
{
    uint8_t BootJumpInstruction[3];
    uint8_t OemIdentifier[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FatCount;
    uint16_t DirEntryCount;
    uint16_t TotalSectors;
    uint8_t MediaDescriptorType;
    uint16_t SectorsPerFat;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t LargeSectorCount;

    union {
        ebpb_t ebpb_1216;
        ebpb_fat32_t ebpb_32;
    }ebpb;
   
} __attribute__((packed)) FAT_BootSector;

typedef struct
{
    uint8_t Buffer[SECTOR_SIZE];
    FAT_file Public;
    uint8_t Opened;
    uint32_t FirstCluster;
    uint32_t CurrentCluster;
    uint32_t CurrentSectorInCluster;

} FAT_FileData;

typedef struct
{
    union
    {
        FAT_BootSector BootSector;
        uint8_t BootSectorBytes[SECTOR_SIZE];
    } BS;

    FAT_FileData RootDirectory;

    FAT_FileData OpenedFiles[MAX_FILE_HANDLES];

    uint8_t FatCache[FAT_CACHE_SIZE * SECTOR_SIZE];
    uint32_t FatCachePosition;

} FAT_Data;

static FAT_Data* g_Data;
static uint32_t g_DataSectionLba;
static uint8_t g_FatType;
static uint32_t g_TotalSectors;
static uint32_t g_SectorsPerFat;

uint8_t FAT_ReadBootSector(disk_t* disk){
    return disk->disk_read(disk->disk, 0, 1, g_Data->BS.BootSectorBytes);
}

uint8_t FAT_ReadFat(disk_t* disk,uint32_t lbaIndex){
    return disk->disk_read(disk->disk, g_Data->BS.BootSector.ReservedSectors + lbaIndex, FAT_CACHE_SIZE, g_Data->FatCache);
}

void FAT_Detect(){
    uint32_t data_cluster = (g_TotalSectors - g_DataSectionLba) / g_Data->BS.BootSector.SectorsPerCluster;
    if(data_cluster < 0xFF5){
        g_FatType = 12;
    }
    else if(g_Data->BS.BootSector.SectorsPerFat != 0){
        g_FatType = 16;
    }
    else {
        g_FatType = 32;
    }
}

uint32_t FAT_ClusterToLba(uint32_t cluster)
{
    return g_DataSectionLba + (cluster - 2) * g_Data->BS.BootSector.SectorsPerCluster;
}

uint8_t FAT_Initialize(disk_t* disk, void* fat_addr){
    g_Data = (FAT_Data*)fat_addr;
    if(!FAT_ReadBootSector(disk)){
        return -1;
    }

    g_Data->FatCachePosition = 0xFFFFFFFF;

    g_TotalSectors = g_Data->BS.BootSector.TotalSectors;
    if(g_TotalSectors == 0){ // FAT32
        g_TotalSectors = g_Data->BS.BootSector.LargeSectorCount;
    }

    g_SectorsPerFat = g_Data->BS.BootSector.SectorsPerFat;
    if(g_SectorsPerFat == 0){ //FAT32
        g_SectorsPerFat = g_Data->BS.BootSector.ebpb.ebpb_32.sector_per_fat;
    }

    uint32_t rootDirSize;
    uint32_t rootDirLba;
    if(g_Data->BS.BootSector.SectorsPerFat == 0){
        g_DataSectionLba = g_Data->BS.BootSector.ReservedSectors + g_SectorsPerFat * g_Data->BS.BootSector.FatCount;
        rootDirSize = 0;
        rootDirLba = FAT_ClusterToLba(g_Data->BS.BootSector.ebpb.ebpb_32.root_dir_cluster);
    }
    else {
        rootDirLba = g_Data->BS.BootSector.ReservedSectors + g_SectorsPerFat * g_Data->BS.BootSector.FatCount;
        rootDirSize = sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.DirEntryCount;
        uint32_t rootDirSector = (rootDirSize + g_Data->BS.BootSector.BytesPerSector - 1) / g_Data->BS.BootSector.BytesPerSector;
        g_DataSectionLba = rootDirLba + rootDirSector;
    }


    g_Data->RootDirectory.Public.Handle = ROOT_DIRECTORY_HANDLE;
    g_Data->RootDirectory.Public.isDirectory = 1;
    g_Data->RootDirectory.Public.Position = 0;
    g_Data->RootDirectory.Public.Size = sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.DirEntryCount;
    g_Data->RootDirectory.Opened = 1;
    g_Data->RootDirectory.FirstCluster = rootDirLba;
    g_Data->RootDirectory.CurrentCluster = rootDirLba;
    g_Data->RootDirectory.CurrentSectorInCluster = 0;

    if(! disk->disk_read(disk->disk, rootDirLba, 1, g_Data->RootDirectory.Buffer)){
        return -2;
    }

    FAT_Detect();

    for(int i = 0; i < MAX_FILE_HANDLES; i++){
        g_Data->OpenedFiles[i].Opened = 0;
    }
    return 1;
}

FAT_file* FAT_OpenEntry(disk_t* disk, FAT_DirectoryEntry* entry){
    int handle = -1;
    for(int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++){
        if(!g_Data->OpenedFiles[i].Opened){
            handle = i;
        }
    }

    if(handle < 0){
 //       printf("FAT: out of file handle\r\n");
        return 0;
    }

    FAT_FileData* fd = &g_Data->OpenedFiles[handle];
    fd->Public.Handle = handle;
    fd->Public.isDirectory = (entry->file_attribute & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->Public.Position = 0;
    fd->Public.Size = entry->file_size;
    fd->FirstCluster = entry->first_cluster_low + ((uint32_t)entry->first_cluster_high << 16);
    fd->CurrentCluster = fd->FirstCluster;
    fd->CurrentSectorInCluster = 0;

    if(!disk->disk_read(disk->disk, FAT_ClusterToLba(fd->CurrentCluster), 1, fd->Buffer)){
 //       printf("FAT: open entry failed - read error cluster=%u lba=%u\n", fd->CurrentCluster, FAT_ClusterToLba(fd->CurrentCluster));
   //     for(int i = 0; i < 11; i++){
    //        printf("%c", entry->file_name[i]);
     //   }
      //  printf("\n");
        return 0;
    }

    fd->Opened = 1;
    return &fd->Public;
}

uint32_t min(uint32_t x, uint32_t y){
    return x < y ? x : y;
}

uint32_t FAT_NextCluster(disk_t* disk, uint32_t currentCluster)
{    uint32_t fatIndex;
    if(g_FatType == 12){
       fatIndex = currentCluster * 3 / 2;
    }
    else if (g_FatType == 16) {
        fatIndex = currentCluster * 2;
    }
    else {
        fatIndex = currentCluster * 4;
    }

    uint32_t fatIndexSector = fatIndex / SECTOR_SIZE;
    if(fatIndexSector < g_Data->FatCachePosition
            || fatIndexSector >= g_Data->FatCachePosition + FAT_CACHE_SIZE){
        FAT_ReadFat(disk, fatIndexSector);
        g_Data->FatCachePosition = fatIndexSector;
    }

    fatIndex -= (g_Data->FatCachePosition * SECTOR_SIZE);

    uint32_t nextCluster;
    if (g_FatType == 12){
        if (currentCluster % 2 == 0)
            nextCluster = (*(uint16_t*)(g_Data->FatCache + fatIndex)) & 0x0FFF;
        else
            nextCluster = (*(uint16_t*)(g_Data->FatCache + fatIndex)) >> 4;

        if (nextCluster >= 0xFF8){
            nextCluster |= 0xFFFFF000;
        }
    }

    else if (g_FatType == 16){
        nextCluster = *(uint16_t*)(g_Data->FatCache + fatIndex);
        if(nextCluster >= 0xFFF8){
            nextCluster |= 0xFFFF0000;
        }
    }
    else {
        nextCluster = *(uint32_t*)(g_Data->FatCache + fatIndex);
    }
    return nextCluster;

}

uint32_t FAT_Read(disk_t* disk, FAT_file *file, uint32_t byteCount, void *output_buffer){
    FAT_FileData* fd = (file->Handle == ROOT_DIRECTORY_HANDLE) ? &g_Data->RootDirectory : &g_Data->OpenedFiles[file->Handle];
    
    uint8_t* u8DataOut = (uint8_t*)output_buffer;
    
    if(!fd->Public.isDirectory || (fd->Public.isDirectory && fd->Public.Size != 0)){
        byteCount = min(byteCount, fd->Public.Size - fd->Public.Position);
    }

    while(byteCount > 0){
        uint32_t leftInBuffer = SECTOR_SIZE - (fd->Public.Position % SECTOR_SIZE);
        uint32_t take = min(byteCount, leftInBuffer);
        memcpy(u8DataOut, fd->Buffer + fd->Public.Position % SECTOR_SIZE, take);
        u8DataOut += take;
        fd->Public.Position += take;
        byteCount -= take;

        if(leftInBuffer == take){
            if(fd->Public.Handle == ROOT_DIRECTORY_HANDLE){
                ++fd->CurrentCluster;

                if(!disk->disk_read(disk, fd->CurrentCluster, 1, fd->Buffer)){
 //                   printf("FAT: read error\r\n");
                    break;
                }
            }
            else {
                if(++fd->CurrentSectorInCluster >= g_Data->BS.BootSector.SectorsPerCluster){
                    fd->CurrentSectorInCluster = 0;
                    fd->CurrentCluster = FAT_NextCluster(disk, fd->CurrentCluster);
                }
                if(fd->CurrentCluster >= 0x0FF8){
                    fd->Public.Size = fd->Public.Position;
                    break;
                }

                if(!disk->disk_read(disk->disk, FAT_ClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster, 1, fd->Buffer)){
//                    printf("FAT: read error\r\n");
                    break;
                }
            }
        }
    }
    return u8DataOut - (uint8_t*)output_buffer;
}

uint8_t FAT_ReadEntry(disk_t* disk, FAT_file *file, FAT_DirectoryEntry *dirEntry){
    return FAT_Read(disk, file, sizeof(FAT_DirectoryEntry), dirEntry) == sizeof(FAT_DirectoryEntry);
}

void FAT_Close(FAT_file *file){
    if(file->Handle == ROOT_DIRECTORY_HANDLE){
        file->Position = 0;
        g_Data->RootDirectory.CurrentCluster = g_Data->RootDirectory.FirstCluster;
    }
    else{
        g_Data->OpenedFiles[file->Handle].Opened = 0;
    }
}

uint8_t FAT_FindFile(disk_t* disk, FAT_file* file, const char* name, FAT_DirectoryEntry* entryOut){
    char fatName[12];
    FAT_DirectoryEntry entry;

    memset(fatName, ' ', sizeof(fatName));
    fatName[11] = 0;
    const char* ext = strchr(name, '.');
    if(!ext){
        ext = name + 11;
    }

    for(int i = 0; i < 8 && name[i] && name + i < ext; i++){
        fatName[i] = toupper(name[i]);
    }

    if(ext != name + 11){
        for(int i = 0; i < 3 && ext[i + 1]; i++){
            fatName[i + 8] = toupper(ext[i+1]);
        }
    }

    while(FAT_ReadEntry(disk, file, &entry)){
        if(memcmp(fatName, entry.file_name, 11) == 0){
            *entryOut = entry;
            return 1;
        }
    }
    return 0;
}

FAT_file* FAT_Open(disk_t* disk, const char* path){
    char name[MAX_PATH_SIZE];
    if(path[0] == '/'){
        path++;
    }
    else if(path[0] == '.' && path[1] == '/'){
        path+=2;
    }

    FAT_file* current = &g_Data->RootDirectory.Public;
    while(*path){
        uint8_t isLast = 0;
        const char* delim = strchr(path, '/');
        if(delim != NULL){
            memcpy(name, path, delim - path);
            name[delim - path] = 0;
            path = delim + 1;
        }
        else {
            unsigned len = strlen(path);
            memcpy(name, path, len);
            name[len + 1] = 0;
            path += len;
            isLast = 0;
        }

        FAT_DirectoryEntry entry;
        if(FAT_FindFile(disk, current, name, &entry)){
            FAT_Close(current);

            if(!isLast && (entry.file_attribute & FAT_ATTRIBUTE_DIRECTORY) == 0){
 //               printf("FAT: %s not a directory\r\n", name);
                return NULL;
            }

            current = FAT_OpenEntry(disk, &entry);
        }
        else {
            FAT_Close(current);
//            printf("FAT: %s not found\r\n", name);
            return NULL;
        }
    }
    return current;
}
