
/*
 *  Author: smilu97, Gim Yeongjin
 */

#include "page.h"

void describe_header(HeaderPage * head)
{
    printf("freePage:   %llu\n", head->freePageOffset / PAGE_SIZE);
    printf("numOfPages: %llu\n", head->numOfPages);
    printf("rootPage:   %llu\n", head->rootPageOffset / PAGE_SIZE);
}

void describe_leaf(MemoryPage * m_leaf)
{
    LeafPage * leaf = (LeafPage*)(m_leaf->p_page);

    puts("LeafPage");
    printf("numOfKeys: %u\n", leaf->header.numOfKeys);
    printf("parent: %llu\n", leaf->header.parentOffset / PAGE_SIZE);
    printf("right: %llu\n", leaf->header.rightOffset / PAGE_SIZE);
    printf("keys: ");
    for(int i=0; i<leaf->header.numOfKeys; ++i) {
        printf("%llu ", leaf->keyValue[i].key);
    } puts("");
}

void describe_internal(MemoryPage * m_internal)
{
    InternalPage * internal = (InternalPage*)(m_internal->p_page);

    puts("InternalPage");
    printf("numOfKeys: %u\n", internal->header.numOfKeys);
    printf("parent: %llu\n", internal->header.parentOffset / PAGE_SIZE);
    printf("contents: (%llu) ", internal->header.oneMoreOffset / PAGE_SIZE);
    for(int i=0; i<internal->header.numOfKeys; ++i) {
        printf("%llu (%llu) ", internal->keyValue[i].key, internal->keyValue[i].offset / PAGE_SIZE);
    } puts("");
}

int init_db(int buf_num)
{
    MAX_MEMPAGE = buf_num;
    return init_buf() - 1;
}

int open_table(const char * filepath)
{
    int fd, result;

    // Check if there is file
    if((result = access(filepath, F_OK)) == 0) {
        if((fd = open(filepath, FILE_OPEN_SETTING, 0644)) == 0) {
            return 0;
        } else {
            return fd;
        }
    } else {
        /*
         * If there isn't file, make new file and build new headerPage
         */
        if((fd = open(filepath, FILE_OPEN_SETTING | O_CREAT, 0644)) < 0) {
            return 0;
        }

        // Make new header page
        MemoryPage * m_head = get_page(fd, 0);
        HeaderPage * head = (HeaderPage*)(m_head->p_page);

        head->freePageOffset = 0;
        head->rootPageOffset = PAGE_SIZE;
        head->numOfPages = 2;
        
        // Make new empty leaf page
        MemoryPage * m_root = get_page(fd, 1);
        LeafPage * root = (LeafPage*)(m_root->p_page);

        root->header.isLeaf = true;
        root->header.numOfKeys = 0;
        root->header.parentOffset = 0;
        root->header.rightOffset = 0;

        return fd;
    }
}

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

            cur = next;
        } else {
            cur = cur->next;
        }
    }
    close(table_id);
    
    return 0;
}

int shutdown_db(void)
{
    // Make sure not to commit free memory pages
    MemoryPage * m_cur = free_mempage;
    while(m_cur) {
        free_mempage->dirty = 0;
        m_cur = m_cur->next;
    }
    free_mempage = 0;

    // Commit memory pages
    for(int idx = 0; idx < last_mempage_idx; ++idx) {
        Dirty * d_cur = mempages[idx].dirty;
        while(d_cur) {
            int size = d_cur->right - d_cur->left;
            commit_page(mempages[idx].table_id, mempages[idx].p_page, mempages[idx].page_num, size, d_cur->left);
        }
    }

    for(lld idx = 0; idx < MEMPAGE_MOD; ++idx) {
        page_buf[idx] = 0;
    }
    mempage_num = 0;
    last_mempage_idx = 0;
    dirty_queue_size = 0;

    /* free(mempages[0].p_page) can de-allocate all pages
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

/*
 * Make global variables same in all environments
 * Make all global variables be zero, 0
 */
int init_buf()
{
    // Initialize hash table
    for(llu idx = 0; idx < MEMPAGE_MOD; ++idx) {
        page_buf[idx] = NULL;
    }

    // Initialize memory pages
    mempage_num = 0;
    mempages = (MemoryPage*)malloc(sizeof(MemoryPage) * MAX_MEMPAGE);  // Allocate big space
    Page * pages = (Page*)malloc(sizeof(Page) * MAX_MEMPAGE);
    for(int i=0; i<MAX_MEMPAGE; ++i) {
        mempages[i].p_page = pages + i;
        mempages[i].cache_idx = i;
        mempages[i].dirty = 0;
    }
    free_mempage = 0;
    last_mempage_idx = 0;
    dirty_queue_size = 0;
    LRUInit();

    return true;
}

int commit_page(int table_id, Page * p_page, llu page_num, llu size, llu offset)
{
    MemoryPage * m_head = get_header_page(table_id);
    HeaderPage * head = (HeaderPage*)m_head->p_page;

    if(page_num >= head->numOfPages) {
        head->numOfPages = page_num + 1;

        commit_page(table_id, (Page*)head, 0, PAGE_SIZE, 0);
    }

    return pwrite(table_id, p_page, size, (page_num * PAGE_SIZE) + offset) == size;
}

int load_page(int table_id, Page * p_page, llu page_num, llu size)
{
    
    return pread(table_id, p_page, size, page_num * PAGE_SIZE) == size;
}

/* Bring page from memory if there are in memory,
 * or load page from disc and cache in memory
 */
MemoryPage * get_page(int table_id, llu page_num)
{
    llu hash_idx = hash_llu(page_num, MEMPAGE_MOD);

    MemoryPage * mp_cur = find_hash_friend(page_buf[hash_idx], table_id, page_num);
    if(mp_cur) {
        LRUAdvance(mp_cur->p_lru);
        return mp_cur;
    }

    // If EIP reaches here, it couldn't find the page in memory
    // therefore, load the page from file
    
    // If already the number of page in memory is MAX_MEMPAGE,
    // delete one page from the memory
    while(mempage_num >= MAX_MEMPAGE) {
        // Select the front of queue as the pray to deallocate memory for new one
        llu del_num = ((MemoryPage*)LRUPop())->page_num;
        make_free_mempage(mempages[del_num].cache_idx);
    }

    // Assert mempage_num < MAX_MEMPAGE
    // There are space for new memory page

    // Allocate new page, or use old memory page
    MemoryPage * new_page = new_mempage();
    if(new_page == NULL) {
        myerror("Failed to get new_mempage");
        return NULL;
    }

    // Load from disk
    if(page_num == 0) {
        load_page(table_id, new_page->p_page, page_num, PAGE_SIZE);
    } else {
        MemoryPage * m_head = get_header_page(table_id);
        HeaderPage * head = (HeaderPage*)m_head->p_page;
        if(page_num < head->numOfPages) {
            load_page(table_id, new_page->p_page, page_num, PAGE_SIZE);
        }
    }
    
    new_page->next = NULL;  // new_page will be tail of linked list
    new_page->page_num = page_num;
    new_page->p_lru = LRUPush(new_page);
    new_page->table_id = table_id;

    // Put into linked list
    new_page->next = page_buf[hash_idx];
    page_buf[hash_idx] = new_page;

    ++mempage_num;
    return new_page;
}

MemoryPage * new_page(int table_id)
{
    MemoryPage * m_head = get_header_page(table_id);
    HeaderPage * head = (HeaderPage*)m_head->p_page;

    MemoryPage * ret;

    if(head->freePageOffset == 0) {
        if((ret = get_page(table_id, head->numOfPages)) == NULL) return NULL;
        ++(head->numOfPages);
    } else {
        if((ret = get_page(table_id, head->freePageOffset / PAGE_SIZE)) == NULL) return NULL;
        head->freePageOffset = ((FreePage*)(ret->p_page))->nextOffset;
    }

    register_dirty_page(m_head, make_dirty(0, 24));

    return ret;
}

int set_parent(int table_id, llu page_num, llu parent_num)
{
    MemoryPage * m_page = get_page(table_id, page_num);
    InternalPage * page = (InternalPage*)(m_page->p_page);
    page->header.parentOffset = parent_num * PAGE_SIZE;

    register_dirty_page(m_page, make_dirty(8, 16));

    return 1;
}

int free_page(int table_id, llu page_num)
{
    MemoryPage * m_head = get_header_page(table_id);
    HeaderPage * head = (HeaderPage*)m_head->p_page;

    // Register as free page
    MemoryPage * m_page = get_page(table_id, page_num);
    if(m_page == NULL) return false;
    FreePage * page = (FreePage*)(m_page->p_page);

    page->nextOffset = head->freePageOffset;
    head->freePageOffset = PAGE_SIZE * page_num;

    register_dirty_page(m_page, make_dirty(0, 8));
    register_dirty_page(m_head, make_dirty(0, 24));
    
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
    int table_id = mem->table_id;
    Page * page = mem->p_page;
    llu page_num = mem->page_num;
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
        del_mp_cur = del_mp_cur->next;
    }

    // Make clean page
    for(Dirty * cur = mem->dirty; cur; cur = cur->next) {
        int size = cur->right - cur->left;
        commit_page(table_id, page, page_num, size, cur->left);
    }
    for(Dirty * cur = mem->dirty; cur;) {
        Dirty * next = cur->next;
        free(cur);
        cur = next;
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

MemoryPage * new_mempage()
{
    MemoryPage * ret;

    if(free_mempage) {
        ret = free_mempage;
        free_mempage = free_mempage->next;

        return ret;
    } else {
        if(last_mempage_idx >= MAX_MEMPAGE) {
            myerror("mempage overflow in new_mempage");
            return NULL;
        }
        ret = mempages + last_mempage_idx;
        ++last_mempage_idx;

        return ret;
    }
}

Dirty * make_dirty(int left, int right)
{
    Dirty * ret = (Dirty*)malloc(sizeof(Dirty));
    ret->left = left;
    ret->right = right;
    ret->next = 0;
    
    return ret;
}

int register_dirty_page(MemoryPage * m_page, Dirty * dirty)
{
    /* Find dirty node to merge
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
