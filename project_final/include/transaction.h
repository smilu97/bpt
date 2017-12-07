
/*
 * Author: smilu97
 * Created date: 7 Dec 2017
 */

#ifndef __TRANSACTION_H__
#define __TRANSACTION_H__

#include "page.h"
#include "algo.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define LOG_UNIT_SIZE 100
#define LOG_IMAGE_SIZE ((LOG_UNIT_SIZE - 40) / 2)

/*
 * Enumeration of type in LogUnit
 */
#define LT_BEGIN  0
#define LT_UPDATE 1
#define LT_COMMIT 2
#define LT_ABORT  3

/* 
 * Pathname of default logfile
 */
#define DEF_LOGFILE "log"

/**********************************************************************
 * Structs
 **********************************************************************/

typedef struct LogUnit
{
    llu lsn;
    llu prev_lsn;
    int trx_id;
    int type;
    int table_id;
    int page_num;
    int offset;
    int data_length;
    char old_image[LOG_IMAGE_SIZE];
    char new_image[LOG_IMAGE_SIZE];
} LogUnit;

/**********************************************************************
 * Functions
 **********************************************************************/


/*
 * Initialize all transaction system
 */
int init_trx();

/*
 * Open log file
 */
int open_logfile(char * pathname);

/*
 * Return 0 if success, otherwise return non-zero value.
 * Allocate transaction structure and initialize it.
 */
int begin_transaction();

/*
 * Return 0 if success, otherwise return non-zero value.
 * User can get response once all modification of transaction are flushed to a log file.
 * If user get successful return, that means your database can recover committed transaction after system crash.
 */
int commit_transaction();

/*
 * Return 0 if success, otherwise return non-zero value.
 * All affected modification should be canceled and return to old state.
 */
int abort_transaction();

/*
 * Log updating page
 * There is no limit of size(right - left)
 * If size is over 30, It automatically split dirty areas,
 * and call below function(update_transaction_small) many times
 */
int update_transaction(MemoryPage * m_page, char * old_data, int left, int right);

/*
 * Log updating page
 * There is limit of size(right - left) to 30byte
 */
int update_transaction_small(MemoryPage * m_page, char * old_data, int left, int right);

/*
 * Make string to represent one LogUnit
 * The area that returned pointer is pointing must be freed by user
 */
char * logunit_tostring(LogUnit * unit);


#endif
