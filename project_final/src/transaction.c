
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
    if(log_fd == 0) return -1;

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
 */
int update_transaction(MemoryPage * m_page, char * old_data, int left, int right)
{
    // Assert log file is opened
    if(log_fd == 0) return -1;

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
