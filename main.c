/*
 * figure out which are the newest files in a subtree
 * copy them to a destination subtree, deleting enough
 * of the oldest ones in the destination to keep it
 * from growing beyond some defined limit
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <popt.h>
#define __USE_XOPEN_EXTENDED
#include <ftw.h>

typedef struct tFileObj {
    struct tFileObj * next;
    const char      * path;
    int               base;
    int               level;
    time_t            timestamp;
    off_t             size;
    blkcnt_t          blocks;
} tFileObj;

static void usage( void )
{
    fprintf(stderr, "Say what?\n");
}

/*
 * nftw doesn't let us pass anything to the callback
 * so we're forced to use static data to keep state
 */
static struct {
    tFileObj * root;
    int        oldestFirst;
} gScanSubtree;

int newerThan( tFileObj *a, tFileObj *b )
{
    return a->timestamp > b->timestamp;
}

void freeList( tFileObj *fileObj )
{
    while ( fileObj != NULL)
    {
        tFileObj *next = fileObj->next;
        free( fileObj );
        fileObj = next;
    }
}

tFileObj *insertFile( tFileObj *newFile, int (*compare)( tFileObj *, tFileObj *) )
{
    if ( newFile != NULL )
    {
        tFileObj *current = gScanSubtree.root;
        tFileObj **prevptr = &gScanSubtree.root;

        while ( current != NULL)
        {
            if ( compare(newFile, current)  )
            {
                // link in the new fileObj before 'current'
                newFile->next = *prevptr;
                *prevptr = newFile;
                break;
            }

            prevptr = &current->next;
            current = current->next;
        }
        if (current == NULL)
        {
            // we didn't break out early; i.e. we reached the end of the list
            newFile->next = NULL;
            *prevptr = newFile;
        }
    }
    return newFile;
}

tFileObj *newFile(const char * filename, const struct stat *status, struct FTW *info)
{
    tFileObj * fileObj = (tFileObj *)malloc( sizeof(tFileObj) );
    if (fileObj != NULL)
    {
        fileObj->next      = NULL;
        fileObj->path      = strdup( filename );
        fileObj->base      = info->base;
        fileObj->level     = info->level;
        fileObj->timestamp = status->st_mtim.tv_sec;
        fileObj->size      = status->st_size;
        fileObj->blocks    = status->st_blocks;
    }
    return fileObj;
}

int nftwCallback( const char * filename, const struct stat *status, int type, struct FTW *info)
{
    switch (type)
    {
    case FTW_F:		/* Regular file */
        insertFile( newFile( filename, status, info ), newerThan );
        break;

    case FTW_D:		/* Directory */
    case FTW_DNR:	/* Unreadable directory */
    case FTW_NS:	/* Unstatable file */
    case FTW_SL:	/* Symbolic link */
    case FTW_DP:	/* Directory, all subdirs have been visited */
    case FTW_SLN:	/* Symbolic link naming non-existing file */
    default:
        /* ignore the rest */
        break;
    }
    return 0;
}

tFileObj * ScanSubtree( const char * path )
{
    freeList( gScanSubtree.root );
    gScanSubtree.root = NULL;

    nftw( path, nftwCallback, 32, FTW_DEPTH );

    return gScanSubtree.root;
}

tFileObj *dumpSubtree( tFileObj * file )
{
    tFileObj *p = file;
    struct tm scratchTime;
    char timeAsString[32];

    while (p != NULL)
    {
        strftime( timeAsString, sizeof(timeAsString), "%F %T", localtime_r( &p->timestamp, &scratchTime ) );
        fprintf( stdout, "%s %12ld %s\n", timeAsString, p->size, p->path );

        p = p->next;
    }
    return file;
}


/* Set up a table of options. */
static struct poptOption commandLineOptions[] = {
    //{ "int",   'i', POPT_ARG_INT,    NULL, 0, "follow with an integer value", "2, 4, 8, or 16" },
    //{ "file",  'f', POPT_ARG_STRING, NULL, 0, "follow with a file name",      NULL },
    //{ "print", 'p', POPT_ARG_NONE,   NULL, 0, "send output to the printer",   NULL },
    POPT_AUTOALIAS
    POPT_AUTOHELP
    POPT_TABLEEND
};

const char * getLastPathElement( const char *str )
{
    const char *result, *p;

    result = str;
    for ( p = str; *p != '\0'; ++p )
    {
        if (*p == '/') result = &p[1];
    }

    return result;
}

int main( int argc, const char *argv[] )
{
    int result = 0;
    poptContext context;
    const char *myName;

    myName = getLastPathElement( argv[0] );

    fprintf(stderr, "invoked as \'%s\'\n", myName );

    context = poptGetContext( myName, argc, argv, commandLineOptions, 0);

    if (argc > 1)
    {
        dumpSubtree( ScanSubtree( argv[1] ) );
    }
    else
    {
        usage();
        result = -1;
    }

    //poptFreeContext( context );

    return result;
}
