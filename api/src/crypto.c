#include <razorback/log.h>
#include <razorback/lock.h>
#include <razorback/thread.h>
#include "init.h"


#include <openssl/ssl.h>
#include <openssl/err.h>


static struct Mutex **sslLocks = NULL;

void 
handle_error(const char *file, int lineno, const char *msg)
{
    rzb_log(LOG_ERR, "** %s:%d %s", file, lineno, msg);
//    ERR_print_errors_fp(stderr);
}

static void locking_function(int mode, int n, const char * file, int line)
{
    if (mode & CRYPTO_LOCK)
        Mutex_Lock(sslLocks[n]);
    else
        Mutex_Unlock(sslLocks[n]);
}

static unsigned long id_function(void)
{
	return (unsigned long)Thread_GetCurrentId();
}

static bool Crypto_Initialize_OpenSSL(void)
{
    int i;
 
    SSL_load_error_strings ();

    SSL_library_init();
    if ((sslLocks = calloc(CRYPTO_num_locks(), sizeof(struct Mutex *))) == NULL)
        return false;

    for (i = 0;  i < CRYPTO_num_locks();  i++)
    {
        if ((sslLocks[i] = Mutex_Create(MUTEX_MODE_NORMAL)) == NULL)
            return false;
    }
    CRYPTO_set_id_callback(id_function);
    CRYPTO_set_locking_callback(locking_function);
    OpenSSL_add_all_digests();
    return true;
}

bool Crypto_Initialize(void)
{
	return Crypto_Initialize_OpenSSL();
}
