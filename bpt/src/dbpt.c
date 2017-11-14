
/*
 *  Author: smilu97, Gim Yeongjin
 */

#include "dbpt.h"

/* Print all elements in table
 * This function is using BFS tree search algorithm
 * to print elements in a line by level
 */
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
        free_pinned();

        if(cur->header.isLeaf) continue;

        queue[queue_back++] = cur->header.oneMoreOffset / PAGE_SIZE;
        for(int idx = 0; idx < cur->header.numOfKeys; ++idx) {
            queue[queue_back++] = cur->keyValue[idx].offset / PAGE_SIZE;
        }
    }
    puts("");

    free(queue);
}

/* Get root page of table
 */
MemoryPage * get_root(int table_id)
{
    HeaderPage * head = (HeaderPage*)(get_header_page(table_id)->p_page);
    return get_page(table_id, head->rootPageOffset / PAGE_SIZE);
}

/* Measure height of page
 */
int height(MemoryPage * mem)
{
    int table_id = mem->table_id;

    int h = 0;
    InternalPage * c = (InternalPage*)(mem->p_page);
    while(false == c->header.isLeaf) {
        c = (InternalPage*)((get_page(table_id, (c->header.oneMoreOffset) / PAGE_SIZE))->p_page);
        ++h;
    }
    return h;
}

/* Print all elements in range (left, right)
 */
void find_and_print_range(int table_id, llu left, llu right)
{
    MemoryPage * m_cur = find_leaf(table_id, left);
    LeafPage * cur = (LeafPage*)(m_cur->p_page);

    int end = 0;
    while(1) {
        int len = cur->header.numOfKeys;
        for(int idx = 0; idx < len; ++idx) {
            llu key = cur->keyValue[idx].key;
            if(key < left) continue;
            if(key > right) {
                end = 1;
                break;
            }
            printf("(%llu, %s) ", key, cur->keyValue[idx].value);
        }
        if(end) break;
        if(cur->header.rightOffset == 0) break;
        m_cur = get_page(table_id, cur->header.rightOffset / PAGE_SIZE);
        cur = (LeafPage*)(m_cur->p_page);
    }
    puts("");
}

/* Find leaf page in which record having key should be
 */
MemoryPage * find_leaf(int table_id, llu key)
{
    MemoryPage * m_root = get_root(table_id);
    InternalPage * root = (InternalPage*)(m_root->p_page);

    /* Binary search to find next page
     */
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

/* Find page on the left of a page
 * The opposite of find_right
 */
MemoryPage * find_left(MemoryPage * m_leaf)
{
    int table_id = m_leaf->table_id;

    LeafPage * leaf = (LeafPage*)(m_leaf->p_page);

    // If there isn't parent, it must not have left
    if(leaf->header.parentOffset == 0) return NULL;

    // Get parent page
    MemoryPage * m_parent = get_page(table_id, leaf->header.parentOffset / PAGE_SIZE);
    InternalPage * parent = (InternalPage*)(m_parent->p_page);

    // Find my index in parent page
    int leaf_idx = get_left_idx(parent, PAGE_SIZE * m_leaf->page_num);

    if(leaf_idx == 0) return NULL;  // I'm most left!

    if(leaf_idx == 1) return get_page(table_id, parent->header.oneMoreOffset / PAGE_SIZE);
    return get_page(table_id, parent->keyValue[leaf_idx - 2].offset / PAGE_SIZE);
}

/* Find page on the right of a page
 * The opposite of find_left
 */
MemoryPage * find_right(MemoryPage * m_leaf)
{
    int table_id = m_leaf->table_id;

    LeafPage * leaf = (LeafPage*)(m_leaf->p_page);

    // If there isn't parent, it must not have right
    if(leaf->header.parentOffset == 0) return NULL;

    // Get parent page
    MemoryPage * m_parent = get_page(table_id, leaf->header.parentOffset / PAGE_SIZE);
    InternalPage * parent = (InternalPage*)(m_parent->p_page);

    // Find my index in parent page
    int leaf_idx = get_left_idx(parent, PAGE_SIZE * m_leaf->page_num);

    if(leaf_idx == parent->header.numOfKeys) return NULL;  // I'm most right!

    return get_page(table_id, parent->keyValue[leaf_idx].offset / PAGE_SIZE);
}

/* Find first(most left) leaf page in tree
 */
MemoryPage * find_first_leaf(int table_id)
{
    // Get root page
    MemoryPage * m_cur = get_root(table_id);
    InternalPage * cur = (InternalPage*)(m_cur->p_page);

    // Until cursor is pointing leaf page, go into first page
    while(false == cur->header.isLeaf) {
        m_cur = get_page(table_id, cur->header.oneMoreOffset / PAGE_SIZE);
        cur = (InternalPage*)(m_cur->p_page);
    }

    return m_cur;
}

/* Find the position of record that having the key
 * in a leaf page
 */
int find_idx_from_leaf(MemoryPage * m_leaf, llu key)
{
    LeafPage * c = (LeafPage*)(m_leaf->p_page);
    llu len = c->header.numOfKeys;
    
    if(len == 0) return -1;
    
    // Binary search
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

/* Find record having the key from table
 */
char * find(int table_id, llu key)
{
    // Find leaf page that can be having the key
    MemoryPage * m_leaf = find_leaf(table_id, key);
    LeafPage * leaf = (LeafPage*)(m_leaf->p_page);

    // Find record in the leaf page
    int idx = find_idx_from_leaf(m_leaf, key);
    if(idx == -1) return NULL;  // There isn't....

    free_pinned();  // Un-pin all memory pages

    return leaf->keyValue[idx].value;
}

/* Delete all records
 */
int destroy_tree(int table_id)
{
    MemoryPage * m_head = get_header_page(table_id);
    HeaderPage * head = (HeaderPage*)(m_head->p_page);

    head->numOfPages = 2;
    head->rootPageOffset = PAGE_SIZE;
    head->freePageOffset = 0;

    MemoryPage * m_root = get_page(table_id, 1);
    LeafPage * root = (LeafPage*)(m_root->p_page);

    root->header.isLeaf = 1;
    root->header.numOfKeys = 0;
    root->header.parentOffset = 0;
    root->header.rightOffset = 0;
    
    register_dirty_page(m_head, make_dirty(0, HEADER_PAGE_COMMIT_SIZE));
    register_dirty_page(m_root, make_dirty(0, PAGE_HEADER_SIZE));

    return true;
}

/* Insert record into database
 *
 * If the number of records in leaf page is over tolerance,
 * it split the leaf page into two leaf page after inserting.
 * 
 * If leaf page was splitted, insert new offset into the parent
 * internal page. If parent wasn't existing, create new internal page
 * and make it new root page
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

/* Insert record into the leaf
 */
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

/* Create internal page to be a new root.
 */
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

    // Set first key,offset pair
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

/* In the internal page, find the index of the offset
 * The first offset(header.oneMoreOffset) has the 0-index
 */
llu get_left_idx(InternalPage * parent, llu leftOffset)
{
    if(parent->header.oneMoreOffset == leftOffset) return 0;
    llu left_idx = 0;
    while (left_idx < parent->header.numOfKeys && 
        parent->keyValue[left_idx].offset != leftOffset)
        left_idx++;
    return left_idx + 1;
}

/* Insert new key and offset into the internal page
 */
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

/* Insert new key and offset into the internal page, and then
 * split the internal page
 */
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

/* Change key in parent on the left of pointer that pointing this page
 */
int change_key_in_parent(MemoryPage * m_page, llu key)
{
    int table_id = m_page->table_id;

    InternalPage * page = (InternalPage*)(m_page);

    MemoryPage * m_parent = get_page(table_id, page->header.parentOffset / PAGE_SIZE);
    InternalPage * parent = (InternalPage*)(m_parent->p_page);
    int pointer_idx = get_left_idx(parent, m_page->page_num * PAGE_SIZE);
    while(pointer_idx == 0 && parent->header.parentOffset != 0) {
        int below_offset = m_parent->page_num * PAGE_SIZE;
        m_parent = get_page(table_id, parent->header.parentOffset / PAGE_SIZE);
        parent = (InternalPage*)(m_parent->p_page);
        pointer_idx = get_left_idx(parent, below_offset);
    }

    if(pointer_idx == 0 && parent->header.parentOffset == 0) return false;

    parent->keyValue[pointer_idx - 1].key = key;

    register_dirty_page(m_parent, make_dirty(
        PAGE_HEADER_SIZE + sizeof(InternalKeyValue) * (pointer_idx - 1),
        PAGE_HEADER_SIZE + sizeof(InternalKeyValue) * pointer_idx
    ));

    return true;
}

/* Delete record that having the key
 */
int delete(int table_id, llu key)
{
    MemoryPage * m_leaf = find_leaf(table_id, key);
    int ret = delete_leaf_entry(m_leaf, key);

    free_pinned();

    return ret - 1;
}

/* Delete record that having the key from the leaf page
 *
 * If the number of records in leaf page is too low,
 * borrow some records from neighbor friends or be merged
 */
int delete_leaf_entry(MemoryPage * m_leaf, llu key)
{
    int table_id = m_leaf->table_id;

    LeafPage * leaf = (LeafPage*)(m_leaf->p_page);

    if(leaf->header.numOfKeys == 0) return false;
    
    // Binary search
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
        return false;
    }

    // Shift records
    int len = leaf->header.numOfKeys, value_len;
    for(int i=del_idx; i<len-1; ++i) {
        leaf->keyValue[i].key = leaf->keyValue[i+1].key;
        value_len = strlen(leaf->keyValue[i+1].value);
        memcpy(leaf->keyValue[i].value, leaf->keyValue[i+1].value, value_len + 1);
    }
    --(leaf->header.numOfKeys);

    /* If the deleted records was first record in leaf page,
     * change the key in parent on the left of offset that pointing this leaf page
     */
    change_key_in_parent(m_leaf, leaf->keyValue[0].key);

    register_dirty_page(m_leaf, make_dirty(0, PAGE_HEADER_SIZE + leaf->header.numOfKeys * sizeof(Record)));

    /* Check the number of records in this leaf page
     *
     * If the number is too low,
     * find friend to share records and borrow or merge
     */
    if(leaf->header.parentOffset != 0 && leaf->header.numOfKeys < LEAF_MERGE_TOLERANCE) {
        MemoryPage * m_friend;
        int friend_is_right = 1;
        if(leaf->header.rightOffset != 0) {
            m_friend = get_page(table_id, leaf->header.rightOffset / PAGE_SIZE);
        } else {
            m_friend = find_left(m_leaf);
            friend_is_right = 0;
        }
        if(m_friend == NULL) {
            return true;
        }
        LeafPage * friend = (LeafPage*)(m_friend->p_page);
        if(friend->header.numOfKeys + leaf->header.numOfKeys <= LEAF_SPLIT_TOLERANCE) {
            if(friend_is_right)
                return coalesce_leaf(m_leaf, m_friend);
            else 
                return coalesce_leaf(m_friend, m_leaf);
        } else {
            if(friend_is_right)
                return redistribute_leaf(m_leaf, m_friend);
            else
                return redistribute_leaf(m_friend, m_leaf);
        }
    }
    
    return true;
}

/* Delete key and offset from internal page
 *
 * If the number of keys in internal page is too low,
 * borrow some keys or be merged and set parentOffset
 * of new children
 */
int delete_internal_entry(MemoryPage * m_internal, llu key)
{
    int table_id = m_internal->table_id;

    InternalPage * internal = (InternalPage*)(m_internal->p_page);

    // Binary search
    int len = internal->header.numOfKeys;
    int left = 0, right = len - 1;
    int pos = -1;
    while(left + 1 < right) {
        int mid = ((left + right) >> 1);
        int mid_key = internal->keyValue[mid].key;
        if(mid_key < key) {
            left = mid;
        } else if(mid_key > key) {
            right = mid;
        } else {
            pos = mid;
            break;
        }
    }
    if(pos == -1) {
        if(internal->keyValue[left].key == key) {
            pos = left;
        } else if(internal->keyValue[right].key == key) {
            pos = right;
        }
    }
    if(pos == -1) {
        if(internal->keyValue[0].key <= key)
            return false;
        pos = 0;
        internal->header.oneMoreOffset = internal->keyValue[0].offset;
    }
    
    // Shift key-offset pairs
    int copy_size = (len - pos - 1) * sizeof(InternalKeyValue);
    memcpy(internal->keyValue + pos, internal->keyValue + (pos + 1), copy_size);
    --(internal->header.numOfKeys);

    // Calculate the area to commit
    int commit_left = sizeof(InternalPageHeader) + sizeof(InternalKeyValue) * pos;
    int commit_right = commit_left + sizeof(InternalKeyValue) * copy_size;

    register_dirty_page(m_internal, make_dirty(0, PAGE_HEADER_SIZE));
    register_dirty_page(m_internal, make_dirty(commit_left, commit_right));

    if(internal->header.parentOffset != 0 && internal->header.numOfKeys < INTERNAL_MERGE_TOLERANCE) {
        MemoryPage * m_friend;
        int friend_is_right = 1;
        m_friend = find_right(m_internal);
        if(m_friend == NULL) {
            m_friend = find_left(m_internal);
            friend_is_right = 0;
        }
        if(m_friend == NULL) {
            return true;
        }
        InternalPage * friend = (InternalPage*)(m_friend->p_page);
        if(friend->header.numOfKeys + internal->header.numOfKeys + 1 <= INTERNAL_SPLIT_TOLERANCE) {
            if(friend_is_right)
                return coalesce_internal(m_internal, m_friend);
            else 
                return coalesce_internal(m_friend, m_internal);
        } else {
            if(friend_is_right)
                return redistribute_internal(m_internal, m_friend);
            else
                return redistribute_internal(m_friend, m_internal);
        }
    }

    /* If this page is root page, and deleted all pairs to be empty,
     * Make this page free page, and set new root page 
     */
    if(internal->header.parentOffset == 0 && internal->header.numOfKeys == 0) {
        MemoryPage * m_head = get_header_page(table_id);
        HeaderPage * head = (HeaderPage*)(m_head->p_page);

        head->rootPageOffset = internal->header.oneMoreOffset;
        free_page(table_id, m_internal->page_num);

        MemoryPage * m_new_root = get_page(table_id, internal->header.oneMoreOffset / PAGE_SIZE);
        InternalPage * new_root = (InternalPage*)(m_new_root->p_page);

        new_root->header.parentOffset = 0;
        
        register_dirty_page(m_new_root, make_dirty(0, PAGE_HEADER_COMMIT_SIZE));
        register_dirty_page(m_head, make_dirty(0, HEADER_PAGE_COMMIT_SIZE));
    }

    return true;
}

/* This function merge two leaf node
 */
int coalesce_leaf(MemoryPage * m_left, MemoryPage * m_right)
{
    int table_id = m_left->table_id;

    LeafPage * left = (LeafPage*)(m_left->p_page);
    LeafPage * right = (LeafPage*)(m_right->p_page);

    MemoryPage * m_parent = get_page(table_id, right->header.parentOffset / PAGE_SIZE);

    int left_len = left->header.numOfKeys;
    int right_len = right->header.numOfKeys;

    // Copy records from right to left
    memcpy(left->keyValue + left_len, right->keyValue, sizeof(Record) * right_len);
    left->header.numOfKeys += right_len;
    left->header.rightOffset = right->header.rightOffset;

    register_dirty_page(m_left, make_dirty(0, PAGE_HEADER_SIZE));
    register_dirty_page(m_left, make_dirty(PAGE_HEADER_SIZE + sizeof(Record) * left_len, PAGE_HEADER_SIZE + sizeof(Record) * (left_len + right_len)));
    free_page(table_id, m_right->page_num);

    // Delete the pair pointing right
    delete_internal_entry(m_parent, right->keyValue[0].key);

    return true;
}

/* This function redistribute records
 */
int redistribute_leaf(MemoryPage * m_left, MemoryPage * m_right)
{
    // Check in which table it is
    int table_id = m_left->table_id;

    // Get two pages
    LeafPage * left  = (LeafPage*)(m_left ->p_page);
    LeafPage * right = (LeafPage*)(m_right->p_page);

    // Save length of two pages
    int len_left  = left ->header.numOfKeys;
    int len_right = right->header.numOfKeys; 

    int diff;  // The difference of the number of records between two pages
    int len_borrow;  // The number of records to borrow

    if(len_left > len_right) {
        diff = len_left - len_right;
        len_borrow = (diff >> 1);

        if(len_right + len_borrow > LEAF_SPLIT_TOLERANCE || len_borrow == 0) return true;

        // Shift records in right
        for(int idx = len_right - 1; idx >= 0; --idx) {
            memcpy(
                right->keyValue + (idx + len_borrow),
                right->keyValue + idx,
                sizeof(Record)
            );
        }
        // Copy records from left to right
        for(int idx = 0; idx < len_borrow; ++idx) {
            memcpy(
                right->keyValue + idx,
                left->keyValue + (len_left - len_borrow + idx),
                sizeof(Record)
            );
        }

        // Adjust number of keys in page
        left ->header.numOfKeys -= len_borrow;
        right->header.numOfKeys += len_borrow;

        // Calculate dirty area and register it
        register_dirty_page(m_left, make_dirty(0, PAGE_HEADER_COMMIT_SIZE));
        register_dirty_page(m_right, make_dirty(0, PAGE_HEADER_COMMIT_SIZE));
        register_dirty_page(
            m_right,
            make_dirty(
                PAGE_HEADER_SIZE,
                PAGE_HEADER_SIZE + sizeof(Record) * right->header.numOfKeys
            )
        );
    } else if(len_left < len_right) {
        diff = len_right - len_left;
        len_borrow = (diff >> 1);

        if(len_left + len_borrow > LEAF_SPLIT_TOLERANCE || len_borrow == 0) return true;

        // Copy records from right to left
        for(int idx = 0; idx < len_borrow; ++idx) {
            memcpy(
                left->keyValue + (len_left + idx),
                right->keyValue + idx,
                sizeof(Record)
            );
        }
        // Shift records in left
        for(int idx = 0; idx < len_borrow; ++idx) {
            memcpy(
                right->keyValue + idx,
                right->keyValue + (len_borrow + idx),
                sizeof(Record)
            );
        }

        // Adjust number of keys in page
        left ->header.numOfKeys += len_borrow;
        right->header.numOfKeys -= len_borrow;

        // Calculate dirty area and register it
        register_dirty_page(m_left, make_dirty(0, PAGE_HEADER_COMMIT_SIZE));
        register_dirty_page(
            m_left,
            make_dirty(
                PAGE_HEADER_SIZE + sizeof(Record) * len_left,
                PAGE_HEADER_SIZE + sizeof(Record) * left->header.numOfKeys
            )
        );
        register_dirty_page(m_right, make_dirty(0, PAGE_HEADER_COMMIT_SIZE));
        register_dirty_page(
            m_right,
            make_dirty(
                PAGE_HEADER_SIZE,
                PAGE_HEADER_SIZE + sizeof(Record) * right->header.numOfKeys
            )
        );
    }

    change_key_in_parent(m_right, right->keyValue[0].key);

    return true;
}

int coalesce_internal(MemoryPage * m_left, MemoryPage * m_right)
{
    int table_id = m_left->table_id;

    InternalPage * left  = (InternalPage*)(m_left->p_page);
    InternalPage * right = (InternalPage*)(m_right->p_page);

    MemoryPage * m_parent = get_page(table_id, right->header.parentOffset / PAGE_SIZE);
    InternalPage * parent = (InternalPage*)(m_parent->p_page);

    int left_len  = left ->header.numOfKeys;
    int right_len = right->header.numOfKeys;

    // Find the mid key between two offsets that pointing left and right pages.
    int left_idx = get_left_idx(parent, m_left->page_num * PAGE_SIZE);
    llu mkey = parent->keyValue[left_idx].key;

    // Copy pairs from right to left
    memcpy(left->keyValue + (left_len + 1), right->keyValue, sizeof(InternalKeyValue) * right_len);
    left->keyValue[left_len].offset = right->header.oneMoreOffset;
    left->keyValue[left_len].key = mkey;
    left->header.numOfKeys += right_len + 1;

    // Set parentOffset of moved children.
    for(int idx = left_len; idx < left->header.numOfKeys; ++idx) {
        set_parent(table_id, left->keyValue[idx].offset / PAGE_SIZE, m_left->page_num);
    }

    register_dirty_page(m_left, make_dirty(0, PAGE_HEADER_SIZE));
    register_dirty_page(m_left, make_dirty(PAGE_HEADER_SIZE + sizeof(Record) * left_len, PAGE_HEADER_SIZE + sizeof(Record) * (left_len + right_len + 1)));
    free_page(table_id, m_right->page_num);

    // Delete the pair pointing right
    delete_internal_entry(m_parent, mkey);

    return true;
}

int redistribute_internal(MemoryPage * m_left, MemoryPage * m_right)
{
    return true;
}

