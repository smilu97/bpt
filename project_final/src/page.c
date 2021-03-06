
/*
 *  Author: smilu97, Gim Yeongjin
 */

#include "page.h"

/*
 * Save all number of opened tables.
 */
int opened_tables[10000];
int opened_tables_num;

/*
 * page_buf[i] is head of hash chain, it will be used to point memory page
 * that has hash(page_num) == i
 * 
 * page_nums in all the memory page linked in same hash chain
 * will have same hashed value
 */
MemoryPage * page_buf[MEMPAGE_MOD];

/*
 * It point the first of whole serialized memory page objects
 */
MemoryPage * mempages;

/*
 * It point the latest free memory page
 * 
 * The `next` pointer of every free memory page points the next
 * free memory page.
 * So, we can find all the free memory pages
 */
MemoryPage * free_mempage;

/*
 * mempage_num: It represents the number of in-used memory pages
 * Since this mempage_num has same value of MAX_MEMPAGE, the transition
 * of memory page will occured
 */
int mempage_num;

/*
 * It has the biggest mempage index ever we used
 * 
 * When we find free memory page to use, if free_mempage is zero,
 * we use mempages[last_mempage_idx++]
 */
int last_mempage_idx;

/*
 * The maximum number of memory pages
 * Initialized by init_db(buf_num), buf_num will be MAX_MEMPAGE
 */
int MAX_MEMPAGE;

/*
 * The number of buffer page, only for buffering
 */
int bufpage_num;

/*
 * Describe header page
 */
void describe_header(HeaderPage * head)
{
    printf("freePage:   %llu\n", head->free_page_offset / PAGE_SIZE);
    printf("num_pages:  %llu\n", head->num_pages);
    printf("rootPage:   %llu\n", head->root_page_offset / PAGE_SIZE);
    printf("page_lsn:   %llu\n", head->page_lsn);
}

/*
 * Describe leaf page
 */
void describe_leaf(MemoryPage * m_leaf)
{
    LeafPage * leaf = (LeafPage*)(m_leaf->p_page);

    puts("LeafPage");
    printf("num_keys: %u\n", leaf->header.num_keys);
    printf("page_lsn: %llu\n", leaf->header.page_lsn);
    printf("parent: %llu\n", leaf->header.parent_offset / PAGE_SIZE);
    printf("right: %llu\n", leaf->header.right_offset / PAGE_SIZE);
    printf("keys: ");
    for(int i=0; i<leaf->header.num_keys; ++i) {
        printf("%llu ", leaf->keyValue[i].key);
    } puts("");
}

/*
 * Describe internal page
 */
void describe_internal(MemoryPage * m_internal)
{
    InternalPage * internal = (InternalPage*)(m_internal->p_page);

    puts("InternalPage");
    printf("num_keys: %u\n", internal->header.num_keys);
    printf("page_lsn: %llu\n", internal->header.page_lsn);
    printf("parent: %llu\n", internal->header.parent_offset / PAGE_SIZE);
    printf("contents: (%llu) ", internal->header.one_more_offset / PAGE_SIZE);
    for(int i=0; i<internal->header.num_keys; ++i) {
        printf("%llu (%llu) ", internal->keyValue[i].key, internal->keyValue[i].offset / PAGE_SIZE);
    } puts("");
}

/*
 * Check if the two file descriptor is reffering same file
 */
int is_same_file(int fd1, int fd2)
{
    struct stat stat1, stat2;
    if(fstat(fd1, &stat1) < 0) return -1;
    if(fstat(fd2, &stat2) < 0) return -1;
    return (stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino);
}

/*
 * Initialize db to prepare memory cache
 * make global variables usable state
 */
int init_db(int buf_num)
{
    MAX_MEMPAGE = buf_num;

    // Initialize hash table
    for(llu idx = 0; idx < MEMPAGE_MOD; ++idx) {
        page_buf[idx] = NULL;
    }

    // Initialize memory pages
    mempage_num = 0;
    mempages = (MemoryPage*)malloc(sizeof(MemoryPage) * MAX_MEMPAGE);  // Allocate big space
    Page * pages = (Page*)malloc(sizeof(Page) * 2 * MAX_MEMPAGE);
    for(int i=0; i<MAX_MEMPAGE; ++i) {
        mempages[i].p_page = pages + (2 * i);
        mempages[i].p_orig = pages + (2 * i + 1);
        mempages[i].cache_idx = i;
        mempages[i].dirty = 0;
        mempages[i].pin_count = 0;
    }
    free_mempage = 0;
    last_mempage_idx = 0;
    opened_tables_num = 1;
    bufpage_num = 0;
    log_buffer_end = 0;
    LRUInit();

    init_trx();

    return 0;
}

/*
 * Open new table
 * 
 * It opens new file and return the file descriptor of that file
 * If the file wasn't exist, initialize headerpage, and root leafpage
 */
int open_table(const char * filepath)
{
    int fd, result;

    // Check if there is file
    if((result = access(filepath, F_OK)) == 0) {
        if((fd = open(filepath, FILE_OPEN_SETTING, 0644)) == 0) {
            return -1;
        } else {
            for(int idx = 0; idx < opened_tables_num; ++idx) {
                if(is_same_file(opened_tables[idx], fd)) {
                    return idx;
                }
            }
            opened_tables[opened_tables_num++] = fd;
            return opened_tables_num - 1;
        }
    } else {
        /*
         * If there isn't file, make new file and build new headerPage
         */
        if((fd = open(filepath, FILE_OPEN_SETTING | O_CREAT, 0644)) < 0) {
            return -1;
        }

        opened_tables[opened_tables_num++] = fd;
        int table_id = opened_tables_num - 1;

        begin_transaction();

        // Make new header page
        MemoryPage * m_head = get_page(table_id, 0);
        HeaderPage * head = (HeaderPage*)(m_head->p_page);

        head->free_page_offset = 0;
        head->root_page_offset = PAGE_SIZE;
        head->num_pages = 2;
        
        // Make new empty leaf page
        MemoryPage * m_root = get_page(table_id, 1);
        LeafPage * root = (LeafPage*)(m_root->p_page);

        root->header.is_leaf = true;
        root->header.num_keys = 0;
        root->header.parent_offset = 0;
        root->header.right_offset = 0;

        register_dirty_page(m_head, make_dirty(0, HEADER_PAGE_COMMIT_SIZE));
        register_dirty_page(m_root, make_dirty(0, PAGE_HEADER_SIZE));

        unpin(m_head);
        unpin(m_root);

        commit_transaction();

        return table_id;
    }
}

/*
 * close the file that has content of table
 * 
 * and de-allocate and commit all in-memory pages
 * related with the table
 */
int close_table(int table_id)
{
    LRUNode * cur = lru_head;
    while(cur) {
        MemoryPage * mem = (MemoryPage*)(cur->mem);

        if(mem->table_id == table_id) {
            make_free_mempage(mem->cache_idx);
            
            LRUNode * next = cur->next;
            
            // Prepare if cur is head or tail
            if(cur == lru_head) lru_head = cur->next;
            if(cur == lru_tail) lru_tail = cur->prev;
            // Unlink cur from the chain
            if(cur->prev) cur->prev->next = cur->next;
            if(cur->next) cur->next->prev = cur->prev;
    
            free(cur);

            --lru_cnt;
            cur = next;
        } else {
            cur = cur->next;
        }
    }
    close(opened_tables[table_id]);

    for(int idx = table_id; idx < opened_tables_num - 1; ++idx) {
        opened_tables[idx] = opened_tables[idx + 1];
    }
    --opened_tables_num;
    
    return 0;
}

int shutdown_db(void)
{
    /*
     * Close all tables
     */
    while(opened_tables_num > 0) {
        close_table(opened_tables[opened_tables_num - 1]);
    }

    MemoryPage * mem;
    while((mem = (MemoryPage*)LRUPop())) {
        make_free_mempage(mem->cache_idx);
    }

    /*
     * free(mempages[0].p_page) can de-allocate all pages
     * because mempages[0].p_page is pointing the first offset of pages
     */
    free(mempages[0].p_page);
    free(mempages);

    // Clean LRU Linked list
    LRUClean();
    LRUInit();

    return 0;
}

MemoryPage * get_header_page(int table_id)
{
    return get_page(table_id, 0);
}

int commit_page(int table_id, Page * p_page, llu page_num, llu size, llu offset)
{
    if(page_num > 0) {
        MemoryPage * m_head = get_header_page(table_id);
        HeaderPage * head = (HeaderPage*)m_head->p_page;
    
        if(page_num >= head->num_pages) {
            head->num_pages = page_num + 1;

            commit_page(table_id, (Page*)head, 0, HEADER_PAGE_COMMIT_SIZE, 0);
        }
    }

    return pwrite(opened_tables[table_id], p_page, size, (page_num * PAGE_SIZE) + offset) == size;
}

char * copy_page(MemoryPage * m_page)
{
    char * ret = (char*)malloc(sizeof(char) * PAGE_SIZE);
    memcpy(ret, m_page->p_page, PAGE_SIZE);
    return ret;
}

int load_page(MemoryPage * m_page, int table_id, int page_num)
{
    int sz = pread(opened_tables[table_id], m_page->p_page, PAGE_SIZE, page_num * PAGE_SIZE);
    memcpy(m_page->p_orig, m_page->p_page, PAGE_SIZE);
    return sz == PAGE_SIZE;
}

/*
 * Find unpinned memory page to de-allocate
 */
MemoryPage * pop_unpinned_lru()
{
    LRUNode * cur = lru_head;
    MemoryPage * mem;
    while(cur) {
        mem = cur->mem;
        if(mem->pin_count == 0) {
            break;
        }
        cur = cur->next;
    }

    if(cur) {
        // Delete LRUNode
        if(cur->prev) cur->prev->next = cur->next;
        if(cur->next) cur->next->prev = cur->prev;
        if(cur == lru_head) lru_head = cur->next;
        if(cur == lru_tail) lru_tail = cur->prev;
        free(cur);
        --(lru_cnt);

        return mem;
    } else {
        myerror("Failed to find memory page: pop_unpinned_lru");
        return NULL;
    }
}

/*
 * Bring page from memory if there are in memory,
 * or load page from disc and cache in memory
 */
MemoryPage * get_page(int table_id, llu page_num)
{
    llu hash_idx = hash_llu(page_num, MEMPAGE_MOD);
    MemoryPage * mp_cur = find_hash_friend(page_buf[hash_idx], table_id, page_num);
    if(mp_cur) {
        LRUAdvance(mp_cur->p_lru);
        enpin(mp_cur);
        return mp_cur;
    }

    if(lru_cnt != mempage_num) {
        myerror("lru_cnt != mempage_num");
    }

    // If EIP reaches here, it couldn't find the page in memory
    // therefore, load the page from file
    
    // If already the number of page in memory is MAX_MEMPAGE,
    // delete one page from the memory
    while(mempage_num >= MAX_MEMPAGE) {
        // Select the front of queue as the pray to deallocate memory for new one
        MemoryPage * pop_mem = pop_unpinned_lru();
        if(pop_mem == 0) {
            myerror("Failed in pop_unpinned_lru");
            return NULL;
        }
        make_free_mempage(pop_mem->cache_idx);
    }

    // Assert mempage_num < MAX_MEMPAGE
    // There are space for new memory page

    // Allocate new page, or use old memory page

    if(last_mempage_idx >= MAX_MEMPAGE && free_mempage == 0) {
        myerror("last_mempage_idx >= MAX_MEMPAGE");
    }

    MemoryPage * new_page = new_mempage(page_num, table_id);
    if(new_page == NULL) {
        myerror("Failed to get new_mempage");
        return NULL;
    }

    // Load from disk
    if(table_id != -1) {  // table_id == -1 => means this is buffer page, so not demanding loading from disk
        if(page_num == 0) {
            load_page(new_page, table_id, page_num);
        } else {
            MemoryPage * m_head = get_header_page(table_id);
            HeaderPage * head = (HeaderPage*)m_head->p_page;
            if(page_num < head->num_pages) {
                load_page(new_page, table_id, page_num);
            }
            unpin(m_head);
        }
    }
    

    // Put into linked list
    new_page->next = page_buf[hash_idx];
    page_buf[hash_idx] = new_page;
    enpin(new_page);

    return new_page;
}

/*
 * Get memory page for only buffering
 */
MemoryPage * get_bufpage()
{
    return get_page(-1, bufpage_num++);
}

MemoryPage * new_page(int table_id)
{
    MemoryPage * m_head = get_header_page(table_id);
    HeaderPage * head = (HeaderPage*)m_head->p_page;

    MemoryPage * ret;

    if(head->free_page_offset == 0) {
        if((ret = get_page(table_id, head->num_pages)) == NULL) return NULL;
        ++(head->num_pages);
    } else {
        if((ret = get_page(table_id, head->free_page_offset / PAGE_SIZE)) == NULL) return NULL;
        head->free_page_offset = ((FreePage*)(ret->p_page))->next_offset;
    }

    register_dirty_page(m_head, make_dirty(0, HEADER_PAGE_COMMIT_SIZE));

    return ret;
}

int set_parent(int table_id, llu page_num, llu parent_num)
{
    MemoryPage * m_page = get_page(table_id, page_num);
    InternalPage * page = (InternalPage*)(m_page->p_page);
    page->header.parent_offset = parent_num * PAGE_SIZE;
    register_dirty_page(m_page, make_dirty(0, 8));
    unpin(m_page);

    return 1;
}

MemoryPage * get_page_if_have(int table_id, llu page_num)
{
    llu hash_idx = hash_llu(page_num, MEMPAGE_MOD);
    return find_hash_friend(page_buf[hash_idx], table_id, page_num);
}

int free_page(int table_id, llu page_num)
{
    MemoryPage * m_head = get_header_page(table_id);
    HeaderPage * head = (HeaderPage*)m_head->p_page;

    // Register as free page
    MemoryPage * m_page = get_page(table_id, page_num);
    if(m_page == NULL) return false;
    FreePage * page = (FreePage*)(m_page->p_page);

    page->next_offset = head->free_page_offset;
    head->free_page_offset = PAGE_SIZE * page_num;

    register_dirty_page(m_page, make_dirty(0, 8));
    register_dirty_page(m_head, make_dirty(0, HEADER_PAGE_COMMIT_SIZE));

    // TODO: Check it's ok to free mempage
    // make_free_mempage(m_page->cache_idx);
    
    return true;
}

MemoryPage * find_hash_friend(MemoryPage * mem, int table_id, llu page_num)
{
    for(; mem != NULL; mem = mem->next) {
        if(mem->table_id == table_id && mem->page_num == page_num) {
            return mem;
        }
    }

    return mem;
}

void make_free_mempage(llu idx)
{
    MemoryPage * mem = mempages + idx;
    int table_id     = mem->table_id;
    Page * page      = mem->p_page;
    llu page_num     = mem->page_num;
    llu hash_del_idx = hash_llu(page_num, MEMPAGE_MOD);
    
    // Find memory page from linked list
    // and delete from page_buf hash chain
    MemoryPage *  del_mp_cur   = page_buf[hash_del_idx];
    MemoryPage ** p_del_mp_cur = page_buf + hash_del_idx;
    while(del_mp_cur != NULL) {
        if(del_mp_cur == mem) {
            (*p_del_mp_cur) = del_mp_cur->next;  // Exclude the page from linked list
            break;
        }
        // Iterate next
        p_del_mp_cur = &(del_mp_cur->next);
        del_mp_cur =     del_mp_cur->next ;
    }

    if(table_id != -1) {  // table_id == -1, means this is buffer page
        // Make clean page
        for(Dirty * cur = mem->dirty; cur; cur = cur->next) {
            int size = cur->right - cur->left;
            commit_page(table_id, page, page_num, size, cur->left);
        }
        // if(mem->dirty) commit_page(table_id, page, page_num, PAGE_SIZE, 0);
        for(Dirty * cur = mem->dirty; cur;) {
            Dirty * next = cur->next;
            free(cur);
            cur = next;
        }
    }

    // Register in free page chain
    mem->next = free_mempage;
    mem->dirty = 0;
    mem->p_lru = 0;
    mem->page_num = 0;
    mem->table_id = 0;
    free_mempage = mempages + idx;

    --mempage_num;
}

MemoryPage * new_mempage(llu page_num, int table_id)
{
    MemoryPage * ret;
    ++mempage_num;

    if(free_mempage) {
        ret = free_mempage;
        free_mempage = free_mempage->next;
    } else {
        if(last_mempage_idx >= MAX_MEMPAGE) {
            myerror("mempage overflow in new_mempage");
            return NULL;
        }
        ret = mempages + last_mempage_idx;
        ++last_mempage_idx;
    }

    ret->page_num = page_num;
    ret->table_id = table_id;
    ret->p_lru = LRUPush(ret);

    return ret;
}

Dirty * make_dirty(int left, int right)
{
    Dirty * ret = (Dirty*)malloc(sizeof(Dirty));

    if(ret == 0) {
        myerror("Failed to make new dirty");
        return NULL;
    }

    // Initialize new dirty
    ret->left = left;
    ret->right = right;
    ret->next = 0;
    
    return ret;
}

int register_dirty_page(MemoryPage * m_page, Dirty * dirty)
{
    /*
     * Find dirty node to merge
     * If not found, create new node
     */
    Dirty * cur = m_page->dirty;
    while(cur) {
        // Check if there are intersecting area
        if(dirty->left <= cur->right || cur->left <= dirty->right) {
            // Calc merged area
            cur->left  = min_int(cur->left,  dirty->left);
            cur->right = max_int(cur->right, dirty->right);
            free(dirty);
            return true;
        }
        cur = cur->next;
    }

    // couldn't find node to merge
    dirty->next = m_page->dirty;
    m_page->dirty = dirty;

    return true;
}

void enpin(MemoryPage * mem)
{
    ++(mem->pin_count);
}

void unpin(MemoryPage * mem)
{
    if(mem->pin_count == 0) {
        myerror("pin_count goes under 0");
    }
    --(mem->pin_count);
    if(mem->pin_count == 0) {
        for(Dirty * dirty = mem->dirty; dirty != NULL; dirty = dirty->next) {
            update_transaction(mem, dirty->left, dirty->right);
        }
    }
}
