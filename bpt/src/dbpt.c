
/*
 *  Author: smilu97, Gim Yeongjin
 */

#include "dbpt.h"

void print_tree(int table_id)
{
    MemoryPage * m_head = get_header_page(table_id);
    HeaderPage * head = (HeaderPage*)m_head->p_page;

    int * queue = (int*)malloc(sizeof(int)*(head->numOfPages));
    int queue_front = 0, queue_back = 1;
    queue[0] = head->rootPageOffset / PAGE_SIZE;
    int parent_offset = 0;
    
    while(queue_front != queue_back) {
        int u = queue[queue_front++];

        MemoryPage * m_cur = get_page(table_id, u);
        InternalPage * cur = (InternalPage*)(m_cur->p_page);
        
        if(cur->header.parentOffset != parent_offset) {
            parent_offset = cur->header.parentOffset;
            puts("");
        }

        if(cur->header.isLeaf) {
            LeafPage * leaf = (LeafPage*)cur;
            for(int idx = 0; idx < leaf->header.numOfKeys; ++idx) {
                printf("%llu ", leaf->keyValue[idx].key);
            }
        } else {
            for(int idx = 0; idx < cur->header.numOfKeys; ++idx) {
                printf("%llu ", cur->keyValue[idx].key);
            }
        }
        putchar('|');

        if(cur->header.isLeaf) continue;

        queue[queue_back++] = cur->header.oneMoreOffset / PAGE_SIZE;
        for(int idx = 0; idx < cur->header.numOfKeys; ++idx) {
            queue[queue_back++] = cur->keyValue[idx].offset / PAGE_SIZE;
        }
    }
    puts("");

    free(queue);
}

MemoryPage * get_root(int table_id)
{
    HeaderPage * head = (HeaderPage*)(get_header_page(table_id)->p_page);
    return get_page(table_id, head->rootPageOffset / PAGE_SIZE);
}

int height(MemoryPage * root)
{
    int table_id = root->table_id;

    int h = 0;
    InternalPage * c = (InternalPage*)(root->p_page);
    while(false == c->header.isLeaf) {
        c = (InternalPage*)((get_page(table_id, (c->header.oneMoreOffset) / PAGE_SIZE))->p_page);
        ++h;
    }
    return h;
}

MemoryPage * find_leaf(int table_id, llu key)
{
    MemoryPage * m_root = get_root(table_id);
    InternalPage * root = (InternalPage*)(m_root->p_page);
    llu idx = 0, len;
    while(false == root->header.isLeaf) {
        len = root->header.numOfKeys;
        llu left = 0, right = len - 1;
        while(left + 1 < right) {
            llu mid = ((left + right) >> 1);
            llu mid_key = root->keyValue[mid].key;
            if(mid_key < key) {
                left = mid;
            } else if(key < mid_key) {
                right = mid;
            } else {
                left = mid;
                break;
            }
        }
        if(root->keyValue[left].key > key) {
            m_root = get_page(table_id, root->header.oneMoreOffset / PAGE_SIZE);
            root = (InternalPage*)(m_root->p_page);
        } else if(root->keyValue[right].key <= key) {
            m_root = get_page(table_id, root->keyValue[right].offset / PAGE_SIZE);
            root = (InternalPage*)(m_root->p_page);
        } else {
            m_root = get_page(table_id, root->keyValue[left].offset / PAGE_SIZE);
            root = (InternalPage*)(m_root->p_page);
        }
    }
    return m_root;
}

MemoryPage * find_left_leaf(MemoryPage * m_leaf)
{
    int table_id = m_leaf->table_id;

    LeafPage * leaf = (LeafPage*)(m_leaf->p_page);

    if(leaf->header.parentOffset == 0) return NULL;

    MemoryPage * m_parent = get_page(table_id, leaf->header.parentOffset / PAGE_SIZE);
    InternalPage * parent = (InternalPage*)(m_parent->p_page);

    int leaf_idx = get_left_idx(parent, PAGE_SIZE * m_leaf->page_num);
    if(leaf_idx == 0) return NULL;

    return get_page(table_id, parent->keyValue[leaf_idx - 1].offset / PAGE_SIZE);
}

MemoryPage * find_first_leaf(int table_id)
{
    MemoryPage * m_head = get_header_page(table_id);
    HeaderPage * head = (HeaderPage*)(m_head->p_page);
    MemoryPage * m_cur = get_page(table_id, head->rootPageOffset / PAGE_SIZE);
    InternalPage * cur = (InternalPage*)(m_cur->p_page);
    while(false == cur->header.isLeaf) {
        m_cur = get_page(table_id, cur->header.oneMoreOffset / PAGE_SIZE);
        cur = (InternalPage*)(m_cur->p_page);
    }

    return m_cur;
}

int find_idx_from_leaf(MemoryPage * m_leaf, llu key)
{
    LeafPage * c = (LeafPage*)(m_leaf->p_page);
    llu len = c->header.numOfKeys;
    
    if(len == 0) return -1;
    
    llu left = 0, right = len - 1, mid, mid_key;
    while(left + 1 < right) {
        mid = ((left + right) >> 1);
        mid_key = c->keyValue[mid].key;
        if(mid_key < key) {
            left = mid;
        } else if(key < mid_key) {
            right = mid;
        } else {  // if(key ==  mid_key)
            return mid;
        }
    }

    if(c->keyValue[left] .key == key) return left;
    if(c->keyValue[right].key == key) return right;
    return -1;
}

char * find(int table_id, llu key)
{
    MemoryPage * m_leaf = find_leaf(table_id, key);
    int idx = find_idx_from_leaf(m_leaf, key);
    if(idx == -1) return NULL;
    return ((LeafPage*)(m_leaf->p_page))->keyValue[idx].value;
}

/*
 * Insert record into database
 */
int insert(int table_id, llu key, const char * value)
{
    // Find leaf page to insert key
    MemoryPage * m_leaf = find_leaf(table_id, key);
    LeafPage * leaf = (LeafPage*)(m_leaf->p_page);

    int ret;
    // Insert record into the leaf page
    if(leaf->header.numOfKeys < LEAF_SPLIT_TOLERANCE) {
        ret = insert_into_leaf                (m_leaf, key, value);
    } else {
        ret = insert_into_leaf_after_splitting(m_leaf, key, value);
    }
    free_pinned();

    return ret - 1;
}

int insert_into_leaf(MemoryPage * m_leaf, llu key, const char * value)
{
    if(find_idx_from_leaf(m_leaf, key) != -1) return false;

    LeafPage * leaf = (LeafPage*)(m_leaf->p_page);  // Get leaf page

    llu len = (leaf->header.numOfKeys)++;
    llu idx;
    int value_len;

    // Shift records one space for new record inserting
    for(idx = len; idx > 0 && key < leaf->keyValue[idx - 1].key; --idx) {
        leaf->keyValue[idx].key = leaf->keyValue[idx - 1].key;
        value_len = strlen(leaf->keyValue[idx - 1].value);
        memcpy(leaf->keyValue[idx].value, leaf->keyValue[idx - 1].value, value_len + 1);
    }

    // Copy key and value into the space
    leaf->keyValue[idx].key = key;
    value_len = strlen(value);

    if(value_len >= VALUE_SIZE)  // limit the length of value
        value_len = VALUE_SIZE - 1;

    memcpy(leaf->keyValue[idx].value, value, value_len + 1);

    // commit modification
    register_dirty_page(m_leaf, make_dirty(0, PAGE_SIZE));

    return true;
}

int insert_into_new_root(MemoryPage * m_left, llu new_key, MemoryPage * m_right)
{
    int table_id = m_left->table_id;

    MemoryPage * m_head = get_header_page(table_id);
    HeaderPage * head = (HeaderPage*)(m_head->p_page);

    // Make new page
    MemoryPage * m_new_root = new_page(table_id);
    InternalPage * new_root = (InternalPage*)(m_new_root->p_page);

    // Make the new page header
    new_root->header.isLeaf        = false;
    new_root->header.numOfKeys     = 1;
    new_root->header.oneMoreOffset = PAGE_SIZE * m_left->page_num;
    new_root->header.parentOffset  = 0;

    new_root->keyValue[0].key    = new_key;
    new_root->keyValue[0].offset = PAGE_SIZE * m_right->page_num;

    head->rootPageOffset = PAGE_SIZE * m_new_root->page_num;

    InternalPage * left  = (InternalPage*)(m_left-> p_page);
    InternalPage * right = (InternalPage*)(m_right->p_page);

    left->header.parentOffset  = head->rootPageOffset;
    right->header.parentOffset = head->rootPageOffset;

    // Commit modifications
    register_dirty_page(m_new_root, make_dirty(0, PAGE_HEADER_SIZE + 20));
    register_dirty_page(m_left,     make_dirty(0, PAGE_HEADER_SIZE));
    register_dirty_page(m_right,    make_dirty(0, PAGE_HEADER_SIZE));
    register_dirty_page(m_head,     make_dirty(0, HEADER_PAGE_COMMIT_SIZE));
    
    return true;
}

/* Insert record into m_leaf and split into two leaf pages to make new one.
 * 
 * and insert the new one into the parent if it exists, or make new root internal node
 */
int insert_into_leaf_after_splitting(MemoryPage * m_leaf, llu key, const char * value)
{
    int table_id = m_leaf->table_id;

    // Insert value into the node
    if(false == insert_into_leaf(m_leaf, key, value))
        return false;
    
    LeafPage * leaf = (LeafPage*)(m_leaf->p_page);
    llu len = leaf->header.numOfKeys;
    llu split = (len >> 1);
    int value_len;

    MemoryPage * m_new_leaf = new_page(table_id);
    LeafPage * new_leaf = (LeafPage*)(m_new_leaf->p_page);

    new_leaf->header.isLeaf = true;
    new_leaf->header.numOfKeys = len - split;
    new_leaf->header.parentOffset = leaf->header.parentOffset;
    new_leaf->header.rightOffset = leaf->header.rightOffset;

    leaf->header.rightOffset = PAGE_SIZE * m_new_leaf->page_num;
    leaf->header.numOfKeys = split;

    for(llu idx = split; idx < len; ++idx) {
        new_leaf->keyValue[idx - split].key = leaf->keyValue[idx].key;
        value_len = strlen(leaf->keyValue[idx].value);
        memcpy(new_leaf->keyValue[idx - split].value, leaf->keyValue[idx].value, value_len + 1);
    }

    register_dirty_page(m_leaf,     make_dirty(0, PAGE_HEADER_SIZE));
    register_dirty_page(m_new_leaf, make_dirty(0, PAGE_HEADER_SIZE + (len - split) * sizeof(Record)));

    return insert_into_parent(m_leaf, new_leaf->keyValue[0].key, m_new_leaf);
}

/* Find the parent of m_left, and insert (new_key, m_new_leaf) to the parent internal node
 */
int insert_into_parent(MemoryPage * m_left, llu new_key, MemoryPage * m_new_leaf)
{
    int table_id = m_left->table_id;

    InternalPage * left = (InternalPage*)(m_left->p_page);  // Get left page
    
    if(left->header.parentOffset == 0) {  // If left page was already root, make new root
        return insert_into_new_root(m_left, new_key, m_new_leaf);
    }

    // Get parent
    MemoryPage * m_parent = get_page(table_id, left->header.parentOffset / PAGE_SIZE);
    InternalPage * parent = (InternalPage*)(m_parent->p_page);
    
    // Get offset of left and right page
    llu leftOffset = PAGE_SIZE * m_left->page_num;
    llu newOffset = PAGE_SIZE * m_new_leaf->page_num;

    // Get index of left page in parent page
    llu left_idx = get_left_idx(parent, leftOffset);

    // Insert new offset into parent
    if(parent->header.numOfKeys < INTERNAL_SPLIT_TOLERANCE) {
        return insert_into_internal                (m_parent, left_idx, new_key, m_new_leaf);
    } else {
        return insert_into_internal_after_splitting(m_parent, left_idx, new_key, m_new_leaf);
    }
}

llu get_left_idx(InternalPage * parent, llu leftOffset)
{
    // TODO: This algorhthm is O(n)
    if(parent->header.oneMoreOffset == leftOffset) return 0;
    llu left_idx = 0;
    while (left_idx <= parent->header.numOfKeys && 
        parent->keyValue[left_idx].offset != leftOffset)
        left_idx++;
    return left_idx + 1;
}

int insert_into_internal(MemoryPage * m_internal, llu left_idx, llu new_key, MemoryPage * m_right)
{
    InternalPage * internal = (InternalPage*)(m_internal->p_page);
    llu len = (internal->header.numOfKeys)++;
    llu idx;

    for(idx = len; idx > left_idx; --idx) {
        internal->keyValue[idx] = internal->keyValue[idx - 1];
    }

    internal->keyValue[idx].key = new_key;
    internal->keyValue[idx].offset = PAGE_SIZE * m_right->page_num;

    InternalPage * right = (InternalPage*)(m_right->p_page);
    right->header.parentOffset = PAGE_SIZE * m_internal->page_num;

    register_dirty_page(m_internal, make_dirty(0, PAGE_SIZE));
    register_dirty_page(m_right,    make_dirty(0, PAGE_HEADER_SIZE));

    return true;
}

int insert_into_internal_after_splitting(MemoryPage * m_internal, llu left_idx, llu new_key, MemoryPage * m_right)
{
    int table_id = m_internal->table_id;
    if(false == insert_into_internal(m_internal, left_idx, new_key, m_right))
        return false;

    InternalPage * internal = (InternalPage*)(m_internal->p_page);
    llu len = internal->header.numOfKeys;
    llu split = (len >> 1);

    MemoryPage * m_new_internal = new_page(table_id);
    InternalPage * new_internal = (InternalPage*)(m_new_internal->p_page);

    new_internal->header.isLeaf = false;
    new_internal->header.numOfKeys = len - split - 1;
    new_internal->header.parentOffset = internal->header.parentOffset;
    new_internal->header.oneMoreOffset = internal->keyValue[split].offset;

    internal->header.numOfKeys = split;
    set_parent(table_id, internal->keyValue[split].offset / PAGE_SIZE, m_new_internal->page_num);

    for(llu idx = split + 1; idx < len; ++idx) {
        new_internal->keyValue[idx - split - 1].key    = internal->keyValue[idx].key;
        new_internal->keyValue[idx - split - 1].offset = internal->keyValue[idx].offset;

        set_parent(table_id, internal->keyValue[idx].offset / PAGE_SIZE, m_new_internal->page_num);
    }

    register_dirty_page(m_internal,     make_dirty(0, PAGE_HEADER_SIZE));
    register_dirty_page(m_new_internal, make_dirty(0, PAGE_HEADER_SIZE + (len - split - 1) * sizeof(InternalKeyValue)));

    return insert_into_parent(m_internal, internal->keyValue[split].key, m_new_internal);
}

/* print all key, values
 */
void print_all(int table_id)
{
    // Iterate leaf pages from left to right
    LeafPage * it = (LeafPage*)(find_first_leaf(table_id)->p_page);
    while(1) {
        int len = it->header.numOfKeys;
        // print all records in it
        for(int idx = 0; idx < len; ++idx) {
            printf("%llu: %s\n", it->keyValue[idx].key, it->keyValue[idx].value);
        }
        if(it->header.rightOffset == 0) break;  // until there isn't any right leaf page
        it = (LeafPage*)(get_page(table_id, it->header.rightOffset / PAGE_SIZE)->p_page);
    }
}

int delete(int table_id, llu key)
{
    MemoryPage * m_head = get_header_page(table_id);
    HeaderPage * head = (HeaderPage*)(m_head->p_page);

    MemoryPage * m_leaf = find_leaf(table_id, key);
    LeafPage * leaf = (LeafPage*)(m_leaf->p_page);
    
    int del_idx = -1;
    int left = 0, right = leaf->header.numOfKeys - 1;
    while(left + 1 < right) {
        int mid = ((left + right) >> 1);
        int mid_key = leaf->keyValue[mid].key;
        if(mid_key < key) {
            left = mid;
        } else if(mid_key > key) {
            right = mid;
        } else {
            del_idx = mid;
            break;
        }
    }
    if(del_idx == -1) {
        if(leaf->keyValue[left].key == key) {
            del_idx = left;
        } else if(leaf->keyValue[right].key == key) {
            del_idx = right;
        }
    }
    if(del_idx == -1) {
        return -1;
    }

    int len = leaf->header.numOfKeys, value_len;
    for(int i=del_idx; i<len-1; ++i) {
        leaf->keyValue[i].key = leaf->keyValue[i+1].key;
        value_len = strlen(leaf->keyValue[i+1].value);
        memcpy(leaf->keyValue[i].value, leaf->keyValue[i+1].value, value_len + 1);
    }
    --(leaf->header.numOfKeys);

    register_dirty_page(m_head, make_dirty(0, HEADER_PAGE_COMMIT_SIZE));
    register_dirty_page(m_leaf, make_dirty(0, PAGE_HEADER_SIZE + leaf->header.numOfKeys * sizeof(Record)));

    // if(leaf->header.numOfKeys < LEAF_MERGE_TOLERANCE) {
    //      return merge_leaf(m_leaf) - 1;
    // }
    free_pinned();
    
    return 0;
}

int merge_leaf(MemoryPage * m_leaf)
{
    return true;
}
