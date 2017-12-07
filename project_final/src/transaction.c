
/*
 * Author: smilu97
 * Created date: 7 Dec 2017
 */

#include "transaction.h"

/**********************************************************************
 * Global Variables
 **********************************************************************/

/*
 * File descriptor pointing log file
 */
int log_fd;

/*
 * ID of transaction that processing now
 */
int now_trx_id;

/*
 * Latest transaction ID
 */
int last_trx_id;

/**********************************************************************
 * Declarations
 **********************************************************************/

/*
 * Initialize all transaction system
 */
int init_trx()
{
    open_logfile(NULL);

    log_fd = 0;
    now_trx_id = 0;
    last_trx_id = 0;

    return 0;
}

/*
 * Open log file
 */
int open_logfile(char * pathname)
{
    if(log_fd != 0) {
        close(log_fd);
    }

    if(pathname == NULL) {
        pathname = DEF_LOGFILE;
    }

    log_fd = open(pathname, O_RDWR | O_CREAT, 0644);
    if(log_fd == -1) {
        myerror("Failed to open log file");
        return -1;
    }

    return 0;
}

/*
 * Return 0 if success, otherwise return non-zero value.
 * Allocate transaction structure and initialize it.
 */
int begin_transaction()
{
    // Assert log file is opened
    if(log_fd == 0) {
        open_logfile(NULL);
    }

    now_trx_id = (last_trx_id++) + 1;

    // Initialize log 
    LogUnit log;
    log.lsn = 0;
    log.prev_lsn = 0;
    log.trx_id = now_trx_id;
    log.type = LT_BEGIN;
    log.table_id = 0;
    log.page_num = 0;
    log.offset = 0;
    log.data_length = 0;

    lseek(log_fd, 0, SEEK_END);
    write(log_fd, &log, sizeof(LogUnit));

    return 0;
}

/*
 * Return 0 if success, otherwise return non-zero value.
 * User can get response once all modification of transaction are flushed to a log file.
 * If user get successful return, that means your database can recover committed transaction after system crash.
 */
int commit_transaction()
{
    // Assert log file is opened
    if(log_fd == 0) {
        myerror("Log file isn't opened: commit_transaction");
        return -1;
    }

    // Initialize log
    LogUnit log;
    log.lsn = 0;
    log.prev_lsn = 0;
    log.trx_id = now_trx_id;
    log.type = LT_COMMIT;
    log.table_id = 0;
    log.page_num = 0;
    log.offset = 0;
    log.data_length = 0;

    now_trx_id = 0;

    lseek(log_fd, 0, SEEK_END);
    write(log_fd, &log, sizeof(LogUnit));

    return 0;
}

/*
 * Return 0 if success, otherwise return non-zero value.
 * All affected modification should be canceled and return to old state.
 */
int abort_transaction()
{

    return 0;

}


/*
 * Log updating page
 * There is no limit of size(right - left)
 * If size is over 30, It automatically split dirty areas,
 * and call below function(update_transaction_small) many times
 */
int update_transaction(MemoryPage * m_page, char * old_data, int left, int right)
{
    int size = right - left;
    int ret = 0;
    for(int s_left = 0; s_left < size; s_left += LOG_IMAGE_SIZE) {
        int s_right = min_int(s_left + LOG_IMAGE_SIZE, right);
        ret = update_transaction_small(m_page, old_data + s_left, s_left, s_right);
        if(ret) return ret;
    }

    return 0;
}

/*
 * Log updating page
 * There is limit of size(right - left) to 30byte
 */
int update_transaction_small(MemoryPage * m_page, char * old_data, int left, int right)
{
    // Assert log file is opened
    if(log_fd == 0) {
        myerror("Log file isn't opened: update_transaction_small");
        return -1;
    }

    LeafPage * l_page = (LeafPage*)(m_page->p_page);

    // Initialize log
    LogUnit log;
    log.lsn = l_page->header.page_lsn + 1;
    log.prev_lsn = l_page->header.page_lsn;
    log.trx_id = now_trx_id;
    log.type = LT_COMMIT;
    log.table_id = m_page->table_id;
    log.page_num = m_page->page_num;
    log.offset = left;
    log.data_length = right - left;
    memcpy(log.old_image, old_data, log.data_length);
    memcpy(log.new_image, (char*)(m_page->p_page) + left, log.data_length);

    ++(l_page->header.page_lsn);

    lseek(log_fd, 0, SEEK_END);
    write(log_fd, &log, sizeof(LogUnit));

    return 0;
}

/*
 * Make string to represent one LogUnit
 * The area that returned pointer is pointing must be freed by user
 */
char * logunit_tostring(LogUnit * unit)
{
    const char * LT_NAMES[] = {
        "BEGIN",
        "UPDATE",
        "COMMIT",
        "ABORT"
    };
    
    char * ret = (char*)malloc(sizeof(char)*1024);
    sprintf(
        ret,
        "(%llu -> %llu), trx: %d, %s, tbl: %d, pg: %d, off: %d, len: %d\n",
        unit->prev_lsn,
        unit->lsn,
        unit->trx_id,
        LT_NAMES[unit->type],
        unit->table_id,
        unit->page_num,
        unit->offset,
        unit->data_length
    );
    return ret;
}
