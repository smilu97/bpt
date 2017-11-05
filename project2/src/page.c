
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

/*
 * Make global variables same in all environments
 * Make all global variables be zero, 0
 */
void init_buf()
{
    for(llu idx = 0; idx < MEMPAGE_MOD; ++idx) {
        page_buf[idx] = NULL;
    }
    fd = 0;
    mempage_num = 0;
    mempages = (MemoryPage*)malloc(sizeof(MemoryPage) * MAX_MEMPAGE);
    for(int i=0; i<MAX_MEMPAGE; ++i) {
        mempages[i].p_page = (Page*)malloc(sizeof(Page));
        mempages[i].cache_idx = i;
    }
    free_mempage = 0;
    last_mempage_idx = 0;
    dirty_queue_size = 0;
    LRUInit();
}

void delete_cache()
{
    for(lld idx = 0; idx < MEMPAGE_MOD; ++idx) {
        page_buf[idx] = 0;
    }
    mempage_num = 0;
    last_mempage_idx = 0;
    dirty_queue_size = 0;
    for(int i=0; i<MAX_MEMPAGE; ++i) {
        free(mempages[i].p_page);
    }
    free(mempages);
    free_mempage = 0;
    LRUClean();
    LRUInit();
}

int commit_page(Page * p_page, llu page_num, llu size, llu offset)
{
    // If file wasn't opened, FAIL
    if(fd == 0) return false;

    if(page_num >= headerPage.numOfPages) {
        headerPage.numOfPages = page_num + 1;

        commit_page((Page*)&headerPage, 0, PAGE_SIZE, 0);
    }

    return pwrite(fd, p_page, size, (page_num * PAGE_SIZE) + offset) == size;
}

int load_page(Page * p_page, llu page_num, llu size)
{
     // If file wasn't opened, FAIL
    if(fd == 0) return false;
    
    return pread(fd, p_page, size, page_num * PAGE_SIZE) == size;
}

int close_file()
{
    // If file wasn't opened, FAIL
    if(fd == 0) return false;

    commit_page((Page*)&headerPage, 0, PAGE_SIZE, 0);
    close(fd);

    return true;
}

int open_db(const char * filepath)
{
    // TODO: set size in loading or committing header page
    //       for efficiency;

    // If already file was opened, close the previous file
    if(fd != 0) {
        close_file();
    }

    int result;
    init_buf();

    // Check if there is file
    if((result = access(filepath, F_OK)) == 0) {
        if((fd = open(filepath, FILE_OPEN_SETTING, 0644)) == 0) {
            return -1;
        } else {
            load_page((Page*)(&headerPage), 0, PAGE_SIZE);  // load header page
            return 0;
        }
    } else {
        /*
         * If there isn't file, make new file and build new headerPage
         */
        if((fd = open(filepath, FILE_OPEN_SETTING | O_CREAT, 0644)) < 0) {
            return -1;
        }

        headerPage.freePageOffset = 0;
        headerPage.rootPageOffset = PAGE_SIZE;
        headerPage.numOfPages = 1;
        
        MemoryPage * m_root = get_page(1);
        if(m_root == NULL) {
            perror("Failed to make new, first root page");
            return -1;
        }
        LeafPage * root = (LeafPage*)(m_root->p_page);
        root->header.isLeaf = true;
        root->header.numOfKeys = 0;
        root->header.parentOffset = 0;
        root->header.rightOffset = 0;

        headerPage.numOfPages = 2;

        commit_page((Page*)(&headerPage), 0, PAGE_SIZE, 0);
        commit_page((Page*)root, 1, PAGE_SIZE, 0);
        return 0;
    }
}

/* Bring page from memory if there are in memory,
 * or load page from disc and cache in memory
 */
MemoryPage * get_page(llu page_num)
{
    // If file wasn't opened, FAIL
    if(fd == 0) {
        perror("File wasn't opened");
        return NULL;
    }

    llu hash_idx = hash_llu(page_num, MEMPAGE_MOD);

    MemoryPage * mp_cur = find_hash_friend(page_buf[hash_idx], page_num);
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

        llu hash_del_idx = hash_llu(del_num, MEMPAGE_MOD);

        // Find memory page from linked list
        MemoryPage *  del_mp_cur   =   page_buf[hash_del_idx];
        MemoryPage ** p_del_mp_cur = page_buf + hash_del_idx;
        while(del_mp_cur != NULL) {
            if(del_mp_cur->page_num == del_num) {
                (*p_del_mp_cur) = del_mp_cur->next;  // Exclude the page from linked list
                make_free_mempage(del_mp_cur->cache_idx);
                // Decrease the size of mempage
                --mempage_num;
                break;
            }
            // Iterate next
            p_del_mp_cur = &(del_mp_cur->next);
            del_mp_cur = del_mp_cur->next;
        }

        if(del_mp_cur == NULL) {
            puts("Critical Error: couldn't delete mempage in get_page\n");
            exit(1);
        }
        // Critical Error
        // Above finding must success.
        // Linearly find something to delete
        // if(del_mp_cur == NULL) {
        //     // print_error("Critical Error: get_page, failed to find deletee page");

        //     for(llu idx = 0; idx < MEMPAGE_MOD; ++idx) {
        //         if(page_buf[idx] != 0) {
        //             MemoryPage * mp = page_buf[idx];
        //             page_buf[idx] = page_buf[idx]->next;
        //             if(new_free_page == NULL) new_free_page = mp;
        //             else {
        //                 free(mp->p_page);
        //                 free(mp);
        //             }
        //             --mempage_num;
        //             break;
        //         }
        //     }

        //     // Very very critical error
        //     // EIP never reaches here

        //     // print_error("Very critical Error: get_page, failed to find deletee page");
        //     return NULL;
        // }
    }

    // Assert mempage_num < MAX_MEMPAGE
    // There are space for new memory page

    // Allocate new page, or use old memory page
    MemoryPage * new_page = new_mempage();
    if(new_page == NULL) {
        perror("Failed to get new_mempage");
        return NULL;
    }

    // Load from disk
    if(page_num < headerPage.numOfPages) {
        load_page(new_page->p_page, page_num, PAGE_SIZE);
        // if(false == load_page(new_page->p_page, page_num, PAGE_SIZE)) {
        //     make_free_mempage(new_page->cache_idx);
        //     perror("Failed to load page in get_page");
        //     return NULL;
        // }
    }
    
    new_page->next = NULL;  // new_page will be tail of linked list
    new_page->page_num = page_num;
    new_page->p_lru = LRUPush(new_page);

    // Put into linked list
    new_page->next = page_buf[hash_idx];
    page_buf[hash_idx] = new_page;

    ++mempage_num;
    return new_page;
}

MemoryPage * new_page()
{
    // If file wasn't opened, FAIL
    if(fd == 0) return NULL;

    MemoryPage * ret;

    if(headerPage.freePageOffset == 0) {
        if((ret = get_page(headerPage.numOfPages)) == NULL) return NULL;
        ++(headerPage.numOfPages);
    } else {
        if((ret = get_page(headerPage.freePageOffset / PAGE_SIZE)) == NULL) return NULL;
        headerPage.freePageOffset = ((FreePage*)(ret->p_page))->nextOffset;
    }

    if(false == commit_page((Page*)&headerPage, 0, 24, 0)) return NULL;

    return ret;
}

int set_parent(llu page_num, llu parent_num)
{
    MemoryPage * m_page = get_page(page_num);
    InternalPage * page = (InternalPage*)(m_page->p_page);
    page->header.parentOffset = parent_num * PAGE_SIZE;
    commit_page((Page*)page, page_num, PAGE_HEADER_SIZE, 0);

    return 1;
}

int free_page(llu page_num)
{
    // If file wasn't opened, FAIL
    if(fd == 0) return false;

    // Register as free page
    MemoryPage * m_page = get_page(page_num);
    if(m_page == NULL) return false;
    FreePage * page = (FreePage*)(m_page->p_page);
    page->nextOffset = headerPage.freePageOffset;
    headerPage.freePageOffset = PAGE_SIZE * page_num;

    // Commit
    if(false == commit_page((Page*)page, page_num, 8, 0)) return false;
    if(false == commit_page((Page*)&headerPage, 0, 24, 0)) return false;
    
    return true;
}

MemoryPage * find_hash_friend(MemoryPage * mem, llu page_num)
{
    while(mem != NULL && mem->page_num != page_num) {
        mem = mem->next;
    }
    return mem;
}

void make_free_mempage(llu idx)
{
    if(free_mempage == 0) {
        free_mempage = mempages + idx;
        free_mempage->next = 0;
    }  else {
        mempages[idx].next = free_mempage;
        free_mempage = mempages + idx;
    }
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
            perror("mempage overflow in new_mempage");
            return NULL;
        }
        ret = mempages + last_mempage_idx;
        ++last_mempage_idx;

        return ret;
    }
}

Dirty make_dirty(int left, int right)
{
    Dirty ret;
    ret.left = left;
    ret.right = right;
    return ret;
}

void update_dirty(Dirty * dest, Dirty src)
{
    dest->left  = min_int(dest->left,  src.left);
    dest->right = max_int(dest->right, src.right);
}

int register_dirty_page(MemoryPage * m_page, Dirty dirty)
{
    for(int i=0; i<dirty_queue_size; ++i) {
        if(dirty_queue[i] == m_page->page_num) {
            return false;
        }
    }
    dirty_queue[dirty_queue_size++] = m_page->page_num;
    update_dirty(&(m_page->dirty), dirty);

    return true;
}

int commit_dirty_pages()
{
    for(int idx = 0; idx < dirty_queue_size; ++idx) {
        int page_num = dirty_queue[idx];

        MemoryPage * m_page = get_page(page_num);
        Page * page = m_page->p_page;

        int size = m_page->dirty.right - m_page->dirty.left;
        int offset = m_page->dirty.left;

        if(false == commit_page(page, page_num, size, offset)) {
            return false;
        }
    }

    dirty_queue_size = 0;  // Make dirty queue empty

    return true;
}
