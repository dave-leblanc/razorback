void
Test_MemcacheDInsertionAndRetrieval (void)
{
    // set global variables required
    g_iMaxMessageSize = 1000000;


    // temporary storage
    uint32_t l_iIndex;

    // initialize global cache
    if (!GlobalCache_Initialize ((const uint8_t *) "127.0.0.1", 13001))
    {
        rzb_log (LOG_ERR, "GlobalCache_Initialize failed\n");
        return;
    }
    rzb_log (LOG_INFO, "GlobalCache_Initialize success\n");

    rzb_log (LOG_INFO, "Beginning Puts\n");

    // intialize block to all zeros
    uint8_t l_sBlockData[1000];
    memset (l_sBlockData, 0, 1000);

    // intialize data type to NULL
    uuid_t l_uuidDataType;
    uuid_clear (l_uuidDataType);

    // put some entries in the cache
    for (l_iIndex = 1; l_iIndex < 10; l_iIndex++)
    {
        struct BlockId l_bidBlockId;
        if (!BlockId_Initialize
            (&l_bidBlockId, l_uuidDataType, l_iIndex, l_sBlockData))
        {
            rzb_log (LOG_ERR, "BlockId_Initialize failed\n");
            return;
        }
        if (!GlobalCache_Put (&l_bidBlockId, l_iIndex))
        {
            rzb_log (LOG_ERR, "GlobalCache_Put failed\n");
            return;
        }
        BlockId_Destroy (&l_bidBlockId);
    }

    // put some existing entries into the cache
    for (l_iIndex = 1; l_iIndex < 10; l_iIndex++)
    {
        struct BlockId l_bidBlockId;
        if (!BlockId_Initialize
            (&l_bidBlockId, l_uuidDataType, l_iIndex, l_sBlockData))
        {
            rzb_log (LOG_ERR, "BlockId_Initialize failed\n");
            return;
        }
        if (!GlobalCache_Put (&l_bidBlockId, l_iIndex))
        {
            rzb_log (LOG_ERR, "GlobalCache_Put failed\n");
            return;
        }
        BlockId_Destroy (&l_bidBlockId);
    }

    rzb_log (LOG_INFO, "Puts pass\n");

    rzb_log (LOG_INFO, "Beginning Gets\n");

    // get these same entries from the cache
    for (l_iIndex = 1; l_iIndex < 10; l_iIndex++)
    {
        struct BlockId l_bidBlockId;
        uint32_t l_iDisposition;

        rzb_log (LOG_INFO, "GETTING %i\n", l_iIndex);

        if (!BlockId_Initialize
            (&l_bidBlockId, l_uuidDataType, l_iIndex, l_sBlockData))
        {
            rzb_log (LOG_ERR, "BlockId_Initialize failed\n");
            return;
        }
        if (!GlobalCache_Get (&l_bidBlockId, &l_iDisposition))
        {
            rzb_log (LOG_ERR, "GlobalCache_Get failed\n");
            return;
        }
        // test if disposition is the same as placed in there....
        if (l_iIndex != l_iDisposition)
        {
            rzb_log (LOG_ERR,
                     "GlobalCache_Get returns incorrect disposition (ret %i should be %i)\n",
                     l_iDisposition, l_iIndex);
            return;
        }
        BlockId_Destroy (&l_bidBlockId);
    }

    // get some entries from the cache that don't exist...
    for (l_iIndex = 10; l_iIndex < 20; l_iIndex++)
    {
        struct BlockId l_bidBlockId;
        uint32_t l_iDisposition;

        rzb_log (LOG_INFO, "GETTING %i\n", l_iIndex);

        if (!BlockId_Initialize
            (&l_bidBlockId, l_uuidDataType, l_iIndex, l_sBlockData))
        {
            rzb_log (LOG_ERR, "BlockId_Initialize failed\n");
            return;
        }
        if (!GlobalCache_Get (&l_bidBlockId, &l_iDisposition))
        {
            rzb_log (LOG_ERR, "GlobalCache_Get failed\n");
            return;
        }
        // test if disposition returned is not found
        if (l_iDisposition != CACHE_NOT_FOUND)
        {
            rzb_log (LOG_ERR,
                     "GlobalCache_Get does not return CACHE_NOT_FOUND\n");
            return;
        }
        BlockId_Destroy (&l_bidBlockId);
    }

    rzb_log (LOG_INFO, "Gets pass\n");

    // clean up
    GlobalCache_Terminate ();

    // done
    rzb_log (LOG_INFO, "Test_MemcacheDInsertAndRetrieval passed\n");
}

