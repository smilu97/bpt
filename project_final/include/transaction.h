
/*
 * Author: smilu97
 * Created date: 7 Dec 2017
 */

typedef unsigned long long llu;
typedef unsigned long lu;

#define LOG_UNIT_SIZE 100
#define LOG_IMAGE_SIZE ((LOG_UNIT_SIZE - 40) / 2)

/*
 * Enumeration of type in LogUnit
 */
#define LT_BEGIN  0
#define LT_UPDATE 1
#define LT_COMMIT 2
#define LT_ABORT  3

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
