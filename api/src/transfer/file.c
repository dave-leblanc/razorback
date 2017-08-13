#include "config.h"
#include <razorback/debug.h>
#include <razorback/file.h>
#include <razorback/types.h>
#include <razorback/log.h>
#include <razorback/block_pool.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _MSC_VER
#include <io.h>
#include <direct.h>
#include "bobins.h"
#else
#include <sys/mman.h>
#endif
#include <fcntl.h>

#include "transfer/core.h"
#include "runtime_config.h"

static bool File_Store(struct BlockPoolItem *item, struct ConnectedEntity *dispatcher);
static bool File_Fetch(struct Block *block, struct ConnectedEntity *dispatcher);
static struct TransportDescriptor descriptor =
{
    0,
    "File",
    "Transfer file via shared file system",
    File_Store,
    File_Fetch
};

bool 
File_Init(void)
{
    return Transport_Register(&descriptor);
}
static char * File_mkdir(const char *fmt, ...)
{
    char *dir = NULL;
	va_list argp;
    va_start (argp, fmt);
    if (vasprintf (&dir, fmt, argp) == -1)
    {
        rzb_log (LOG_ERR, "%s: Could not allocate directory string",
                 __func__);
        return NULL;
    }

    if (access (dir, F_OK) == -1)
    {
#ifdef _MSC_VER
		if (_mkdir (dir) != 0)
#else
        if (mkdir (dir, S_IRUSR | S_IWUSR | S_IXUSR |
                   S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1)	
#endif
        {
            rzb_log (LOG_ERR, "%s: Error creating directory %s", __func__,
                     dir);
            free (dir);
            return NULL;
        }
    }
    return dir;
}
static char *
createDirectory(struct Block *block, const char *basepath)
{
    char *hash;
    char *dir;

    hash = Transfer_generateFilename(block);
    if ((dir = File_mkdir("%s/%c", basepath, hash[0])) == NULL)
        goto createDone;
    else 
        free(dir);

    if ((dir = File_mkdir("%s/%c/%c", basepath, hash[0], hash[1])) == NULL)
        goto createDone;
    else 
        free(dir);

    if ((dir = File_mkdir("%s/%c/%c/%c", basepath, hash[0], hash[1], hash[2])) == NULL)
        goto createDone;
    else 
        free(dir);

    dir = File_mkdir("%s/%c/%c/%c/%c", basepath, hash[0], hash[1], hash[2],
         hash[3]);

createDone:
    free(hash);

    return dir;
}

static uint32_t
writeWrap (int fd, uint8_t * data, uint64_t length)
{

    int SizeDword;
    int totalbytes = 0;
    int bytessofar;

    SizeDword = (int) length;

    while (totalbytes < SizeDword)
    {
        bytessofar = write (fd, data + totalbytes, SizeDword - totalbytes);
        if (bytessofar == -1)
        {
            rzb_perror ("writeWrap: Could not write data to file: %s");
            return 0;
        }
        totalbytes += bytessofar;
    }

    return 1;
}

static bool 
File_Store(struct BlockPoolItem *item, struct ConnectedEntity *dispatcher)
{
    int fd;
    char *filename =NULL;
    char *path = NULL;
    char *dir = NULL;
    struct BlockPoolData *dataItem = NULL;
    uint8_t data[4096];
    size_t len;

	ASSERT (item != NULL);

    if ((filename = Transfer_generateFilename (item->pEvent->pBlock)) == NULL)
    {
        rzb_log (LOG_ERR, "%s: failed to generate file name", __func__);
        return false;
    }
    if ((dir = createDirectory(item->pEvent->pBlock, Config_getLocalityBlockStore())) == NULL)
    {
        rzb_log (LOG_ERR, "%s: failed to create storage dir", __func__);
        free(filename);
        return false;
    }
    if (asprintf(&path, "%s/%s", dir, filename) == -1)
    {
        rzb_log (LOG_ERR, "%s: failed to generate file path", __func__);
        free(filename);
        free(dir);
        return false;
    }
    free(filename);
    free(dir);


	if ((fd = open(path, O_RDONLY, 0)) != -1)
    {
        close(fd);
        free(path);
        return true;
    }
    fd = open (path, O_RDWR | O_CREAT | O_TRUNC,
               S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1)
    {
        rzb_perror ("StoreDataAsFile: Could not open file for writing: %s");
        free (filename);
        return 0;
    }
    dataItem = item->pDataHead;
    while (dataItem != NULL)
    {
        if (dataItem->iFlags == BLOCK_POOL_DATA_FLAG_FILE)
        {
            while((len = fread(data,1,4096, dataItem->data.file)) > 0)
            {
                if (writeWrap(fd,data,len) == 0)
                {
                    rzb_log (LOG_ERR, "%s: Write failed.", __func__);
                    free (filename);
                    close (fd);
                    return false;
                }
            }
            rewind(dataItem->data.file);

        }
        else
        {
            if ((writeWrap (fd, dataItem->data.pointer, dataItem->iLength)) == 0)
            {
                rzb_log (LOG_ERR, "%s: Write failed.", __func__);
                free (filename);
                close (fd);
                return false;
            }
        }
        dataItem = dataItem->pNext;
    }

    close (fd);

    free (path);

    return true;

}

static bool 
File_Fetch(struct Block *block, struct ConnectedEntity *dispatcher)
{
    int fd;
    char *filename = NULL;
    char *path = NULL;
    struct stat fs;
#ifdef _MSC_VER
	char *tmp = NULL;
#endif

	ASSERT (block != NULL);

    if ((filename = Transfer_generateFilename (block)) == NULL)
    {
        rzb_log (LOG_ERR, "%s: failed to generate file name", __func__);
        return false;
    }
    if (asprintf(&path, "%s/%c/%c/%c/%c/%s", Config_getLocalityBlockStore(),
                filename[0], filename[1], filename[2], filename[3], filename) == -1)
    {
        rzb_log (LOG_ERR, "%s: failed to generate file path", __func__);
        return false;
    }
    free (filename); filename = NULL;

#ifdef _MSC_VER
	while ((tmp = strchr( ((tmp == NULL) ? path : tmp), '/')) != NULL)
		*tmp = '\\';
#endif    

    fd = open (path, O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1)
    {
        rzb_perror
            ("RetrieveDataAsFile: Could not open file for reading: %s");
        return false;
    }

    if (fstat (fd, &fs) == -1)
    {
        rzb_perror ("RetrieveDataAsFile: Could not stat file: %s");
        close (fd);
        return false;
    }
    close(fd);

    return Transfer_Prepare_File(block, path, false);

}

bool
File_Delete(struct Block *block)
{
    char *filename = NULL;
    char *path = NULL;
#ifdef _MSC_VER
    char *tmp = NULL;
#endif

    ASSERT (block != NULL);

    if ((filename = Transfer_generateFilename (block)) == NULL)
    {
        rzb_log (LOG_ERR, "%s: failed to generate file name", __func__);
        return false;
    }
    if (asprintf(&path, "%s/%c/%c/%c/%c/%s", Config_getLocalityBlockStore(),
                filename[0], filename[1], filename[2], filename[3], filename) == -1)
    {
        rzb_log (LOG_ERR, "%s: failed to generate file path", __func__);
        return false;
    }
    free (filename); filename = NULL;

#ifdef _MSC_VER
    while ((tmp = strchr( ((tmp == NULL) ? path : tmp), '/')) != NULL)
        *tmp = '\\';
#endif

    if (remove(path) != 0)
		rzb_perror("File_Remove: failed to delete file: %s");


	return true;
}
