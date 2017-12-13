
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

/*
 * Log buffer
 */
char log_buffer[LOGBUF_SIZE];
int log_buffer_end;

/**********************************************************************
 * Declarations
 **********************************************************************/

/*
 * Initialize all transaction system
 */
int init_trx()
{
    rename(DEF_LOGFILE, DEF_LOGFILE_BAK);
    open_logfile(DEF_LOGFILE);
    redo_records(DEF_LOGFILE_BAK);
    unlink(DEF_LOGFILE_BAK);

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
 * Close log file
 */
int close_logfile()
{
    int ret = 0;

    if(log_fd == 0) return -1;

    if(log_buffer_end > 0) {
        ret = log_force();
        if(ret) {
            myerror("Failed to force log: close_logfile");
        }
    }

    close(log_fd);

    return 0;
}

/*
 * Force log in buffer into disk
 */
int log_force()
{
    /*
     * File must be opend in forcing log
     */
    if(log_fd == 0) return -1;

    if(log_buffer_end == 0) return 0;

    lseek(log_fd, 0, SEEK_END);
    write(log_fd, log_buffer, log_buffer_end);
    log_buffer_end = 0;

    return 0;
}

/*
 * Append log data into buffer
 */
int log_append(void * log, int log_size)
{
    /*
     * Predict if there will be overflow in log buffer
     */
    if(log_buffer_end + log_size >= LOGBUF_SIZE) {
        if(log_force()) {
            myerror("Failed to force log to prevent overflow: log_append");
        }
    }

    memcpy(log_buffer + log_buffer_end, log, log_size);
    log_buffer_end += log_size;

    return 0;
}

/*
 * Append many log data into buffer
 */
int log_append_many(void ** logs, int log_num)
{
    int size_sum = 0, ret = 0;

    for(int idx = 0; idx < log_num; ++idx) {
        size_sum += (int)logs[idx * 2 + 1];
    }
    if(log_buffer_end + size_sum >= LOGBUF_SIZE) {
        if((ret = log_force())) {
            myerror("Failed to force in log_append_many");
            return ret;
        }
    }

    for(int idx = 0; idx < log_num; ++idx) {
        log_append(logs[idx<<1], (int)logs[(idx<<1)+1]);
    }

    return 0;
}

/*
 * Calculate last lsn
 */
llu get_last_lsn()
{
    if(log_fd == 0) return -1;

    return (llu)lseek(log_fd, 0, SEEK_END) + log_buffer_end;
}

/*
 * Set page LSN
 */
int set_page_lsn(MemoryPage * mem, llu page_lsn)
{
    if(mem->page_num == 0) {
        HeaderPage * page = (HeaderPage*)(mem->p_page);
        page->page_lsn = page_lsn;
        register_dirty_page(mem, make_dirty(0, HEADER_PAGE_COMMIT_SIZE));
    } else {
        InternalPage * page = (InternalPage*)(mem->p_page);
        page->header.page_lsn = page_lsn;
        register_dirty_page(mem, make_dirty(0, PAGE_HEADER_COMMIT_SIZE));
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
    llu last_lsn = get_last_lsn();

    // Initialize log 
    LogRecord log;
    log.lsn = last_lsn + LOGSIZE_BEGIN;
    log.prev_lsn = last_lsn;
    log.trx_id = now_trx_id;
    log.type = LT_BEGIN;
    // log.table_id = 0;
    // log.page_num = 0;
    // log.offset = 0;
    // log.data_length = 0;

    log_append(&log, LOGSIZE_BEGIN);

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

    llu last_lsn = get_last_lsn();

    // Initialize log
    LogRecord log;
    log.lsn = last_lsn + LOGSIZE_COMMIT;
    log.prev_lsn = last_lsn;
    log.trx_id = now_trx_id;
    log.type = LT_COMMIT;
    // log.table_id = 0;
    // log.page_num = 0;
    // log.offset = 0;
    // log.data_length = 0;

    now_trx_id = 0;

    log_append(&log, LOGSIZE_COMMIT);

    log_force();

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
    llu last_lsn = get_last_lsn();

    // Initialize log
    LogRecord log;
    log.lsn = last_lsn + record_size;
    log.prev_lsn = last_lsn;
    log.trx_id = now_trx_id;
    log.type = LT_UPDATE;
    log.table_id = m_page->table_id;
    log.page_num = m_page->page_num;
    log.offset = left;
    log.data_length = image_size;

    set_page_lsn(m_page, log.lsn);

    void * log_data[] = {
        &log, (void*)(llu)LOGSIZE_UPDATE,
        (char*)(m_page->p_orig) + left, (void*)(llu)image_size,
        (char*)(m_page->p_page) + left, (void*)(llu)image_size
    };

    log_append_many(log_data, 3);

    return 0;
}

/*
 * Redo records
 */
int redo_records(char * pathname)
{
    int fd = open(pathname, O_RDWR);

    if(fd == 0) {
        myerror("Failed to open log file: redo_record");
        return -1;
    }

    lseek(fd, 0, SEEK_SET);  // Assert file descriptor is pointing 0 offset
    LogRecord rec;
    char old_buf[PAGE_SIZE];
    char new_buf[PAGE_SIZE];

    while(1) {
        int sz = read(fd, &rec, LOGSIZE_BEGIN);
        if(sz < LOGSIZE_BEGIN) break;

        if(rec.type == LT_BEGIN) begin_transaction();
        else if(rec.type == LT_COMMIT) commit_transaction();
        else if(rec.type == LT_UPDATE) {

            // Read more metadata
            read(fd, (char*)(&rec) + LOGSIZE_BEGIN, LOGSIZE_UPDATE - LOGSIZE_BEGIN);

            MemoryPage * m_page = get_page(rec.table_id, rec.page_num);

            InternalPage * i_page = (InternalPage*)(m_page->p_page);
            llu page_lsn = i_page->header.page_lsn;

            if(page_lsn < rec.lsn) {
                lseek(fd, rec.data_length, SEEK_CUR);  // Skip old_image
                read(fd, new_buf, rec.data_length);
                BYTE * page = m_page->p_page->bytes;
                memcpy(page + rec.offset, new_buf, rec.data_length);
                i_page->header.page_lsn = rec.lsn;
                register_dirty_page(m_page, make_dirty(0, PAGE_HEADER_COMMIT_SIZE));
                register_dirty_page(m_page, make_dirty(rec.offset, rec.offset + rec.data_length));
            } else {
                lseek(fd, rec.data_length << 1, SEEK_CUR); // Skip images
            }

            unpin(m_page);
        }
    }

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
