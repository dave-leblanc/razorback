#include "config.h"
#include <razorback/debug.h>
#include <razorback/nugget.h>

SO_PUBLIC void
Nugget_Destroy(struct Nugget *p_pNugget)
{
    if (p_pNugget->sName != NULL)
        free(p_pNugget->sName);

    if (p_pNugget->sLocation !=NULL)
        free(p_pNugget->sLocation);

    if (p_pNugget->sContact != NULL)
        free(p_pNugget->sContact);

    if (p_pNugget->sNotes !=NULL)
        free(p_pNugget->sNotes);

    free(p_pNugget);
}

