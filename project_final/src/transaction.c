
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
    
    llu file_pos = lseek(log_fd, 0, SEEK_END);

    // Initialize log 
    LogRecord log;
    log.lsn = file_pos + LOGSIZE_BEGIN;
    log.prev_lsn = file_pos;
    log.trx_id = now_trx_id;
    log.type = LT_BEGIN;
    // log.table_id = 0;
    // log.page_num = 0;
    // log.offset = 0;
    // log.data_length = 0;

    write(log_fd, &log, LOGSIZE_BEGIN);

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

    llu file_pos = lseek(log_fd, 0, SEEK_END);

    // Initialize log
    LogRecord log;
    log.lsn = file_pos + LOGSIZE_COMMIT;
    log.prev_lsn = file_pos;
    log.trx_id = now_trx_id;
    log.type = LT_COMMIT;
    // log.table_id = 0;
    // log.page_num = 0;
    // log.offset = 0;
    // log.data_length = 0;

    now_trx_id = 0;

    write(log_fd, &log, LOGSIZE_COMMIT);

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
int update_transaction(MemoryPage * m_page, int left, int right)
{
    // Assert log file is opened
    if(log_fd == 0) {
        myerror("Log file isn't opened: update_transaction_small");
        return -1;
    }

    int image_size = right - left;
    int record_size = LOGSIZE_UPDATE + 2 * image_size;
    llu file_pos = lseek(log_fd, 0, SEEK_END);

    // Initialize log
    LogRecord log;
    log.lsn = file_pos + record_size;
    log.prev_lsn = file_pos;
    log.trx_id = now_trx_id;
    log.type = LT_UPDATE;
    log.table_id = m_page->table_id;
    log.page_num = m_page->page_num;
    log.offset = left;
    log.data_length = image_size;

    if(m_page->page_num == 0) {
        HeaderPage * page = (HeaderPage*)(m_page->p_page);
        page->page_lsn = log.lsn;
        register_dirty_page(m_page, make_dirty(0, HEADER_PAGE_COMMIT_SIZE));
    } else {
        InternalPage * page = (InternalPage*)(m_page->p_page);
        page->header.page_lsn = log.lsn;
        register_dirty_page(m_page, make_dirty(0, PAGE_HEADER_COMMIT_SIZE));
    }

    write(log_fd, &log, LOGSIZE_UPDATE);
    write(log_fd, (char*)(m_page->p_orig) + left, image_size);
    write(log_fd, (char*)(m_page->p_page) + left, image_size);

    return 0;
}

/*
 * Make string to represent one LogRecord
 * The area that returned pointer is pointing must be freed by user
 */
char * logrecord_tostring(LogRecord * unit)
{
    const char * LT_NAMES[] = {
        "BEGIN ",
        "UPDATE",
        "COMMIT",
        "ABORT "
    };
    
    char buf[1024];
    char * ret = (char*)malloc(sizeof(char)*1024);
    sprintf(
        ret,
        "(%5llu -> %5llu), trx: %d, %s",
        unit->prev_lsn,
        unit->lsn,
        unit->trx_id,
        LT_NAMES[unit->type]
    );

    if(unit->type == LT_UPDATE) {
        sprintf(
            buf,
            ", tbl: %d, pg: %d, off: %d, len: %d",
            unit->table_id,
            unit->page_num,
            unit->offset,
            unit->data_length
        );
        strcat(ret, buf);
    }

    return ret;
}
