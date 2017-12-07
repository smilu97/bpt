
/*
 *  Author: smilu97, Gim Yeongjin
 */

#include "bpt.h"

/* Print all elements in table
 * This function is using BFS tree search algorithm
 * to print elements in a line by level
 */
void print_tree(int table_id)
{
    MemoryPage * m_head = get_header_page(table_id);
    HeaderPage * head = (HeaderPage*)m_head->p_page;

    int * queue = (int*)malloc(sizeof(int)*(head->num_pages));
    int queue_front = 0, queue_back = 1;
    queue[0] = head->root_page_offset / PAGE_SIZE;
    int parent_offset = 0;
    
    while(queue_front != queue_back) {
        int u = queue[queue_front++];

        MemoryPage * m_cur = get_page(table_id, u);
        InternalPage * cur = (InternalPage*)(m_cur->p_page);
        
        if(cur->header.parent_offset != parent_offset) {
            parent_offset = cur->header.parent_offset;
            puts("");
        }

        if(cur->header.is_leaf) {
            LeafPage * leaf = (LeafPage*)cur;
            for(int idx = 0; idx < leaf->header.num_keys; ++idx) {
                printf("%llu ", leaf->keyValue[idx].key);
            }
        } else {
            for(int idx = 0; idx < cur->header.num_keys; ++idx) {
                printf("%llu ", cur->keyValue[idx].key);
            }
        }
        putchar('|');

        if(cur->header.is_leaf) continue;

        queue[queue_back++] = cur->header.one_more_offset / PAGE_SIZE;
        for(int idx = 0; idx < cur->header.num_keys; ++idx) {
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
    MemoryPage * m_head = get_header_page(table_id);
    HeaderPage * head = (HeaderPage*)(m_head->p_page);
    unpin(m_head);
    return get_page(table_id, head->root_page_offset / PAGE_SIZE);
}

/* Measure height of page
 */
int height(MemoryPage * mem)
{
    int table_id = mem->table_id;

    int h = 0;
    InternalPage * c = (InternalPage*)(mem->p_page);
    while(false == c->header.is_leaf) {
        MemoryPage * m = get_page(table_id, (c->header.one_more_offset) / PAGE_SIZE);
        c = (InternalPage*)(m->p_page);
        unpin(m);
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
        int len = cur->header.num_keys;
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
        if(cur->header.right_offset == 0) break;
        unpin(m_cur);
        m_cur = get_page(table_id, cur->header.right_offset / PAGE_SIZE);
        cur = (LeafPage*)(m_cur->p_page);
    }
    unpin(m_cur);
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
    while(false == root->header.is_leaf) {
        len = root->header.num_keys;
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
        unpin(m_root);
        if(root->keyValue[left].key > key) {
            m_root = get_page(table_id, root->header.one_more_offset / PAGE_SIZE);
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
    MemoryPage * res;

    LeafPage * leaf = (LeafPage*)(m_leaf->p_page);

    // If there isn't parent, it must not have left
    if(leaf->header.parent_offset == 0) return NULL;

    // Get parent page
    MemoryPage * m_parent = get_page(table_id, leaf->header.parent_offset / PAGE_SIZE);
    InternalPage * parent = (InternalPage*)(m_parent->p_page);

    // Find my index in parent page
    int leaf_idx = get_left_idx(parent, PAGE_SIZE * m_leaf->page_num);

    if(leaf_idx == 0) res = NULL;  // I'm most left!
    else if(leaf_idx == 1) res = get_page(table_id, parent->header.one_more_offset / PAGE_SIZE);
    else res = get_page(table_id, parent->keyValue[leaf_idx - 2].offset / PAGE_SIZE);

    unpin(m_parent);

    return res;
}

/* Find page on the right of a page
 * The opposite of find_left
 */
MemoryPage * find_right(MemoryPage * m_leaf)
{
    int table_id = m_leaf->table_id;
    MemoryPage * res;

    LeafPage * leaf = (LeafPage*)(m_leaf->p_page);

    // If there isn't parent, it must not have right
    if(leaf->header.parent_offset == 0) return NULL;

    // Get parent page
    MemoryPage * m_parent = get_page(table_id, leaf->header.parent_offset / PAGE_SIZE);
    InternalPage * parent = (InternalPage*)(m_parent->p_page);

    // Find my index in parent page
    int leaf_idx = get_left_idx(parent, PAGE_SIZE * m_leaf->page_num);

    if(leaf_idx == parent->header.num_keys) res = NULL;  // I'm most right!
    else res = get_page(table_id, parent->keyValue[leaf_idx].offset / PAGE_SIZE);

    unpin(m_parent);

    return res;
}

/* Find first(most left) leaf page in tree
 */
MemoryPage * find_first_leaf(int table_id)
{
    // Get root page
    MemoryPage * m_cur = get_root(table_id);
    InternalPage * cur = (InternalPage*)(m_cur->p_page);

    // Until cursor is pointing leaf page, go into first page
    while(false == cur->header.is_leaf) {
        unpin(m_cur);
        m_cur = get_page(table_id, cur->header.one_more_offset / PAGE_SIZE);
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
    llu len = c->header.num_keys;
    
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
    char * ret;

    // Find leaf page that can be having the key
    MemoryPage * m_leaf = find_leaf(table_id, key);
    LeafPage * leaf = (LeafPage*)(m_leaf->p_page);

    // Find record in the leaf page
    int idx = find_idx_from_leaf(m_leaf, key);
    if(idx == -1) ret = NULL;
    else ret = leaf->keyValue[idx].value;

    unpin(m_leaf);  // Un-pin all memory pages

    return ret;
}

/* Delete all records
 */
int destroy_tree(int table_id)
{
    MemoryPage * m_head = get_header_page(table_id);
    HeaderPage * head = (HeaderPage*)(m_head->p_page);

    head->num_pages = 2;
    head->root_page_offset = PAGE_SIZE;
    head->free_page_offset = 0;

    MemoryPage * m_root = get_page(table_id, 1);
    LeafPage * root = (LeafPage*)(m_root->p_page);

    root->header.is_leaf = 1;
    root->header.num_keys = 0;
    root->header.parent_offset = 0;
    root->header.right_offset = 0;
    
    register_dirty_page(m_head, make_dirty(0, HEADER_PAGE_COMMIT_SIZE));
    register_dirty_page(m_root, make_dirty(0, PAGE_HEADER_SIZE));

    unpin(m_head);
    unpin(m_root);

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
    if(leaf->header.num_keys < LEAF_SPLIT_TOLERANCE) {
        ret = insert_into_leaf                (m_leaf, key, value);
    } else {
        ret = insert_into_leaf_after_splitting(m_leaf, key, value);
    }

    unpin(m_leaf);

    return ret - 1;
}

/* Insert record into the leaf
 */
int insert_into_leaf(MemoryPage * m_leaf, llu key, const char * value)
{
    if(find_idx_from_leaf(m_leaf, key) != -1) return false;

    LeafPage * leaf = (LeafPage*)(m_leaf->p_page);  // Get leaf page

    llu len = (leaf->header.num_keys)++;
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
    new_root->header.is_leaf        = false;
    new_root->header.num_keys     = 1;
    new_root->header.one_more_offset = PAGE_SIZE * m_left->page_num;
    new_root->header.parent_offset  = 0;

    // Set first key,offset pair
    new_root->keyValue[0].key    = new_key;
    new_root->keyValue[0].offset = PAGE_SIZE * m_right->page_num;

    head->root_page_offset = PAGE_SIZE * m_new_root->page_num;

    InternalPage * left  = (InternalPage*)(m_left-> p_page);
    InternalPage * right = (InternalPage*)(m_right->p_page);

    left->header.parent_offset  = head->root_page_offset;
    right->header.parent_offset = head->root_page_offset;

    // Commit modifications
    register_dirty_page(m_new_root, make_dirty(0, PAGE_HEADER_SIZE + 20));
    register_dirty_page(m_left,     make_dirty(0, PAGE_HEADER_SIZE));
    register_dirty_page(m_right,    make_dirty(0, PAGE_HEADER_SIZE));
    register_dirty_page(m_head,     make_dirty(0, HEADER_PAGE_COMMIT_SIZE));

    // unpin
    unpin(m_head);
    unpin(m_new_root);
    
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
    llu len = leaf->header.num_keys;
    llu split = (len >> 1);
    int value_len;

    MemoryPage * m_new_leaf = new_page(table_id);
    LeafPage * new_leaf = (LeafPage*)(m_new_leaf->p_page);

    new_leaf->header.is_leaf = true;
    new_leaf->header.num_keys = len - split;
    new_leaf->header.parent_offset = leaf->header.parent_offset;
    new_leaf->header.right_offset = leaf->header.right_offset;

    leaf->header.right_offset = PAGE_SIZE * m_new_leaf->page_num;
    leaf->header.num_keys = split;

    for(llu idx = split; idx < len; ++idx) {
        new_leaf->keyValue[idx - split].key = leaf->keyValue[idx].key;
        value_len = strlen(leaf->keyValue[idx].value);
        memcpy(new_leaf->keyValue[idx - split].value, leaf->keyValue[idx].value, value_len + 1);
    }

    register_dirty_page(m_leaf,     make_dirty(0, PAGE_HEADER_SIZE));
    register_dirty_page(m_new_leaf, make_dirty(0, PAGE_HEADER_SIZE + (len - split) * sizeof(Record)));

    unpin(m_new_leaf);

    return insert_into_parent(m_leaf, new_leaf->keyValue[0].key, m_new_leaf);
}

/* Find the parent of m_left, and insert (new_key, m_new_leaf) to the parent internal node
 */
int insert_into_parent(MemoryPage * m_left, llu new_key, MemoryPage * m_new_leaf)
{
    int table_id = m_left->table_id;
    int res;

    InternalPage * left = (InternalPage*)(m_left->p_page);  // Get left page
    
    if(left->header.parent_offset == 0) {  // If left page was already root, make new root
        return insert_into_new_root(m_left, new_key, m_new_leaf);
    }

    // Get parent
    MemoryPage * m_parent = get_page(table_id, left->header.parent_offset / PAGE_SIZE);
    InternalPage * parent = (InternalPage*)(m_parent->p_page);
    
    // Get offset of left and right page
    llu leftOffset = PAGE_SIZE * m_left->page_num;
    llu newOffset = PAGE_SIZE * m_new_leaf->page_num;

    // Get index of left page in parent page
    llu left_idx = get_left_idx(parent, leftOffset);

    // Insert new offset into parent
    if(parent->header.num_keys < INTERNAL_SPLIT_TOLERANCE) {
        res = insert_into_internal                (m_parent, left_idx, new_key, m_new_leaf);
    } else {
        res = insert_into_internal_after_splitting(m_parent, left_idx, new_key, m_new_leaf);
    }

    unpin(m_parent);

    return res;
}

/* In the internal page, find the index of the offset
 * The first offset(header.one_more_offset) has the 0-index
 */
llu get_left_idx(InternalPage * parent, llu leftOffset)
{
    if(parent->header.one_more_offset == leftOffset) return 0;
    llu left_idx = 0;
    while (left_idx < parent->header.num_keys && 
        parent->keyValue[left_idx].offset != leftOffset)
        left_idx++;
    return left_idx + 1;
}

/* Insert new key and offset into the internal page
 */
int insert_into_internal(MemoryPage * m_internal, llu left_idx, llu new_key, MemoryPage * m_right)
{
    InternalPage * internal = (InternalPage*)(m_internal->p_page);
    llu len = (internal->header.num_keys)++;
    llu idx;

    for(idx = len; idx > left_idx; --idx) {
        internal->keyValue[idx] = internal->keyValue[idx - 1];
    }

    internal->keyValue[idx].key = new_key;
    internal->keyValue[idx].offset = PAGE_SIZE * m_right->page_num;

    InternalPage * right = (InternalPage*)(m_right->p_page);
    right->header.parent_offset = PAGE_SIZE * m_internal->page_num;

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
    llu len = internal->header.num_keys;
    llu split = (len >> 1);

    MemoryPage * m_new_internal = new_page(table_id);
    InternalPage * new_internal = (InternalPage*)(m_new_internal->p_page);

    new_internal->header.is_leaf = false;
    new_internal->header.num_keys = len - split - 1;
    new_internal->header.parent_offset = internal->header.parent_offset;
    new_internal->header.one_more_offset = internal->keyValue[split].offset;

    internal->header.num_keys = split;
    set_parent(table_id, internal->keyValue[split].offset / PAGE_SIZE, m_new_internal->page_num);

    for(llu idx = split + 1; idx < len; ++idx) {
        new_internal->keyValue[idx - split - 1].key    = internal->keyValue[idx].key;
        new_internal->keyValue[idx - split - 1].offset = internal->keyValue[idx].offset;

        set_parent(table_id, internal->keyValue[idx].offset / PAGE_SIZE, m_new_internal->page_num);
    }

    register_dirty_page(m_internal,     make_dirty(0, PAGE_HEADER_SIZE));
    register_dirty_page(m_new_internal, make_dirty(0, PAGE_HEADER_SIZE + (len - split - 1) * sizeof(InternalKeyValue)));

    unpin(m_new_internal);

    return insert_into_parent(m_internal, internal->keyValue[split].key, m_new_internal);
}

/* print all key, values
 */
void print_all(int table_id)
{
    // Iterate leaf pages from left to right
    LeafPage * it = (LeafPage*)(find_first_leaf(table_id)->p_page);
    while(1) {
        int len = it->header.num_keys;
        // print all records in it
        for(int idx = 0; idx < len; ++idx) {
            printf("%llu: %s\n", it->keyValue[idx].key, it->keyValue[idx].value);
        }
        if(it->header.right_offset == 0) break;  // until there isn't any right leaf page
        it = (LeafPage*)(get_page(table_id, it->header.right_offset / PAGE_SIZE)->p_page);
    }
}

MemoryPage * find_internal_parent(MemoryPage * m_page)
{
    int table_id = m_page->table_id;

    InternalPage * page = (InternalPage*)(m_page->p_page);

    MemoryPage * m_parent = get_page(table_id, page->header.parent_offset / PAGE_SIZE);
    InternalPage * parent = (InternalPage*)(m_parent->p_page);
    int pointer_idx = get_left_idx(parent, m_page->page_num * PAGE_SIZE);
    while(pointer_idx == 0 && parent->header.parent_offset != 0) {
        unpin(m_parent);
        int below_offset = m_parent->page_num * PAGE_SIZE;
        m_parent = get_page(table_id, parent->header.parent_offset / PAGE_SIZE);
        parent = (InternalPage*)(m_parent->p_page);
        pointer_idx = get_left_idx(parent, below_offset);
    }

    if(pointer_idx == 0 && parent->header.parent_offset == 0) return NULL;

    return m_parent;
}

/* Change key in parent on the left of pointer that pointing this page
 */
llu change_key_in_parent(MemoryPage * m_page, llu key)
{
    int table_id = m_page->table_id;

    InternalPage * page = (InternalPage*)(m_page->p_page);

    MemoryPage * m_parent = get_page(table_id, page->header.parent_offset / PAGE_SIZE);
    InternalPage * parent = (InternalPage*)(m_parent->p_page);
    int pointer_idx = get_left_idx(parent, m_page->page_num * PAGE_SIZE);
    while(pointer_idx == 0 && parent->header.parent_offset != 0) {
        unpin(m_parent);
        int below_offset = m_parent->page_num * PAGE_SIZE;
        m_parent = get_page(table_id, parent->header.parent_offset / PAGE_SIZE);
        parent = (InternalPage*)(m_parent->p_page);
        pointer_idx = get_left_idx(parent, below_offset);
    }

    if(pointer_idx == 0 && parent->header.parent_offset == 0) return -1;

    llu ret = parent->keyValue[pointer_idx - 1].key;
    parent->keyValue[pointer_idx - 1].key = key;

    register_dirty_page(m_parent, make_dirty(
        PAGE_HEADER_SIZE + sizeof(InternalKeyValue) * (pointer_idx - 1),
        PAGE_HEADER_SIZE + sizeof(InternalKeyValue) * pointer_idx
    ));

    unpin(m_parent);

    return ret;
}

/* Delete record that having the key
 */
int delete(int table_id, llu key)
{
    MemoryPage * m_leaf = find_leaf(table_id, key);
    int ret = delete_leaf_entry(m_leaf, key);

    // unpin_all();

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
    int res;

    LeafPage * leaf = (LeafPage*)(m_leaf->p_page);

    if(leaf->header.num_keys == 0) return false;
    
    // Binary search
    int del_idx = -1;
    int left = 0, right = leaf->header.num_keys - 1;
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
    int len = leaf->header.num_keys, value_len;
    for(int i=del_idx; i<len-1; ++i) {
        leaf->keyValue[i].key = leaf->keyValue[i+1].key;
        value_len = strlen(leaf->keyValue[i+1].value);
        memcpy(leaf->keyValue[i].value, leaf->keyValue[i+1].value, value_len + 1);
    }
    --(leaf->header.num_keys);

    /* If the deleted records was first record in leaf page,
     * change the key in parent on the left of offset that pointing this leaf page
     */
    if(del_idx == 0) change_key_in_parent(m_leaf, leaf->keyValue[0].key);

    register_dirty_page(
        m_leaf,
        make_dirty(
            0,
            PAGE_HEADER_SIZE + leaf->header.num_keys * sizeof(Record)
        )
    );

    /* Check the number of records in this leaf page
     *
     * If the number is too low,
     * find friend to share records and borrow or merge
     */
    if(leaf->header.parent_offset != 0 && leaf->header.num_keys < LEAF_MERGE_TOLERANCE) {
        MemoryPage * m_friend;
        int friend_is_right = 1;
        if(leaf->header.right_offset != 0) {
            m_friend = get_page(table_id, leaf->header.right_offset / PAGE_SIZE);
        } else {
            m_friend = find_left(m_leaf);
            friend_is_right = 0;
        }
        if(m_friend == NULL) {
            return true;
        }
        LeafPage * friend = (LeafPage*)(m_friend->p_page);
        if(friend->header.num_keys + leaf->header.num_keys <= LEAF_SPLIT_TOLERANCE) {
            if(friend_is_right)
                res = coalesce_leaf(m_leaf, m_friend);
            else 
                res = coalesce_leaf(m_friend, m_leaf);
        } else {
            if(friend_is_right)
                res = redistribute_leaf(m_leaf, m_friend);
            else
                res = redistribute_leaf(m_friend, m_leaf);
        }
        unpin(m_friend);
        return res;
    }
    
    return true;
}

/* Delete key and offset from internal page
 *
 * If the number of keys in internal page is too low,
 * borrow some keys or be merged and set parent_offset
 * of new children
 */
int delete_internal_entry(MemoryPage * m_internal, llu key)
{
    int table_id = m_internal->table_id;
    int res;

    InternalPage * internal = (InternalPage*)(m_internal->p_page);

    // Binary search
    int len = internal->header.num_keys;
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
    if(pos == -1 && key < internal->keyValue[0].key) {
        change_key_in_parent(m_internal, internal->keyValue[0].key);
        internal->header.one_more_offset = internal->keyValue[0].offset;
        pos = 0;
    }
    if(pos == -1) {
        return false;
    }
    
    // Shift key-offset pairs
    int copy_size = (len - pos - 1) * sizeof(InternalKeyValue);
    memcpy(internal->keyValue + pos, internal->keyValue + (pos + 1), copy_size);
    --(internal->header.num_keys);

    // Calculate the area to commit
    register_dirty_page(m_internal, make_dirty(0, PAGE_SIZE));

    if(internal->header.parent_offset != 0 && internal->header.num_keys < INTERNAL_MERGE_TOLERANCE) {
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
        if(friend->header.num_keys + internal->header.num_keys + 1 <= INTERNAL_SPLIT_TOLERANCE) {
            if(friend_is_right)
                res = coalesce_internal(m_internal, m_friend);
            else 
                res = coalesce_internal(m_friend, m_internal);
        } else {
            if(friend_is_right)
                res = redistribute_internal(m_internal, m_friend);
            else
                res = redistribute_internal(m_friend, m_internal);
        }
        unpin(m_friend);
        return res;
    }

    /* If this page is root page, and deleted all pairs to be empty,
     * Make this page free page, and set new root page 
     */
    if(internal->header.parent_offset == 0 && internal->header.num_keys == 0) {
        MemoryPage * m_head = get_header_page(table_id);
        HeaderPage * head = (HeaderPage*)(m_head->p_page);

        head->root_page_offset = internal->header.one_more_offset;
        free_page(table_id, m_internal->page_num);

        MemoryPage * m_new_root = get_page(table_id, internal->header.one_more_offset / PAGE_SIZE);
        InternalPage * new_root = (InternalPage*)(m_new_root->p_page);

        new_root->header.parent_offset = 0;
        
        register_dirty_page(m_new_root, make_dirty(0, PAGE_HEADER_COMMIT_SIZE));
        register_dirty_page(m_head, make_dirty(0, HEADER_PAGE_COMMIT_SIZE));

        unpin(m_head);
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

    MemoryPage * m_parent = get_page(table_id, right->header.parent_offset / PAGE_SIZE);

    int left_len = left->header.num_keys;
    int right_len = right->header.num_keys;

    // Copy records from right to left
    memcpy(left->keyValue + left_len, right->keyValue, sizeof(Record) * right_len);
    left->header.num_keys    += right_len;
    left->header.right_offset = right->header.right_offset;

    register_dirty_page(m_left, make_dirty(0, PAGE_HEADER_SIZE + sizeof(Record) * left->header.num_keys));
    free_page(table_id, m_right->page_num);

    // Delete the pair pointing right
    delete_internal_entry(m_parent, right->keyValue[0].key);

    unpin(m_parent);

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
    int len_left  = left ->header.num_keys;
    int len_right = right->header.num_keys; 

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
                left ->keyValue + (len_left - len_borrow + idx),
                sizeof(Record)
            );
        }

        // Adjust number of keys in page
        left ->header.num_keys -= len_borrow;
        right->header.num_keys += len_borrow;

        // Calculate dirty area and register it
        register_dirty_page(m_left, make_dirty(0, PAGE_HEADER_COMMIT_SIZE));
        register_dirty_page(m_right, make_dirty(0, PAGE_HEADER_SIZE + sizeof(Record) * right->header.num_keys));

        change_key_in_parent(m_right, right->keyValue[0].key);
    } else if(len_left < len_right) {
        diff = len_right - len_left;
        len_borrow = (diff >> 1);

        if(len_left + len_borrow > LEAF_SPLIT_TOLERANCE || len_borrow == 0) return true;

        // Copy records from right to left
        for(int idx = 0; idx < len_borrow; ++idx) {
            memcpy(
                left ->keyValue + (len_left + idx),
                right->keyValue + idx,
                sizeof(Record)
            );
        }
        // Shift records in right
        for(int idx = len_borrow; idx < len_right; ++idx) {
            memcpy(
                right->keyValue + (idx - len_borrow),
                right->keyValue + idx,
                sizeof(Record)
            );
        }

        // Adjust number of keys in page
        left ->header.num_keys += len_borrow;
        right->header.num_keys -= len_borrow;

        // Calculate dirty area and register it
        register_dirty_page(m_left,  make_dirty(0, PAGE_HEADER_SIZE + sizeof(Record) * left ->header.num_keys));
        register_dirty_page(m_right, make_dirty(0, PAGE_HEADER_SIZE + sizeof(Record) * right->header.num_keys));

        change_key_in_parent(m_right, right->keyValue[0].key);
    }

    return true;
}

int coalesce_internal(MemoryPage * m_left, MemoryPage * m_right)
{
    int table_id = m_left->table_id;

    InternalPage * left  = (InternalPage*)(m_left->p_page);
    InternalPage * right = (InternalPage*)(m_right->p_page);

    MemoryPage * m_parent = get_page(table_id, right->header.parent_offset / PAGE_SIZE);
    InternalPage * parent = (InternalPage*)(m_parent->p_page);

    int left_len  = left ->header.num_keys;
    int right_len = right->header.num_keys;

    // Find the mid key between two offsets that pointing left and right pages.
    int left_idx = get_left_idx(parent, m_left->page_num * PAGE_SIZE);
    llu mkey = parent->keyValue[left_idx].key;

    // Copy pairs from right to left
    memcpy(left->keyValue + (left_len + 1), right->keyValue, sizeof(InternalKeyValue) * right_len);
    left->keyValue[left_len].offset = right->header.one_more_offset;
    left->keyValue[left_len].key = mkey;
    left->header.num_keys += right_len + 1;

    // Set parent_offset of moved children.
    for(int idx = left_len; idx < left->header.num_keys; ++idx) {
        set_parent(table_id, left->keyValue[idx].offset / PAGE_SIZE, m_left->page_num);
    }

    register_dirty_page(m_left, make_dirty(0, PAGE_HEADER_SIZE + sizeof(InternalKeyValue) * (left_len + right_len + 1)));
    free_page(table_id, m_right->page_num);

    // Delete the pair pointing right
    delete_internal_entry(m_parent, mkey);

    unpin(m_parent);

    return true;
}

int redistribute_internal(MemoryPage * m_left, MemoryPage * m_right)
{
    // Check in which table it is
    int table_id = m_left->table_id;

    // Get two pages
    InternalPage * left  = (InternalPage*)(m_left ->p_page);
    InternalPage * right = (InternalPage*)(m_right->p_page);

    // Save length of two pages
    int len_left  = left ->header.num_keys;
    int len_right = right->header.num_keys; 

    int diff;  // The difference of the number of records between two pages
    int len_borrow;  // The number of records to borrow

    if(len_left > len_right) {

        diff = len_left - len_right;
        len_borrow = (diff >> 1);

        if(len_right + len_borrow > INTERNAL_SPLIT_TOLERANCE || len_borrow <= 1) return true;

        // Shift records in right
        for(int idx = len_right - 1; idx >= 0; --idx) {
            memcpy(
                right->keyValue + (idx + len_borrow),
                right->keyValue + idx,
                sizeof(InternalKeyValue)
            );
        }
        right->keyValue[len_borrow-1].offset = right->header.one_more_offset;
        right->keyValue[len_borrow-1].key = change_key_in_parent(m_right, left->keyValue[len_left-len_borrow].key);
        right->header.one_more_offset = left->keyValue[len_left - len_borrow].offset;
        set_parent(table_id, right->header.one_more_offset / PAGE_SIZE, m_right->page_num);
        // Copy records from left to right
        for(int idx = 0; idx < len_borrow - 1; ++idx) {
            memcpy(
                right->keyValue + idx,
                left ->keyValue + (len_left - len_borrow + idx + 1),
                sizeof(InternalKeyValue)
            );
            set_parent(table_id, right->keyValue[idx].offset / PAGE_SIZE, m_right->page_num);
        }

        // Adjust number of keys in page
        left ->header.num_keys -= len_borrow;
        right->header.num_keys += len_borrow;

        // Calculate dirty area and register it
        register_dirty_page(m_left,  make_dirty(0, PAGE_HEADER_COMMIT_SIZE));
        register_dirty_page(m_right, make_dirty(0, PAGE_HEADER_SIZE + sizeof(InternalKeyValue) * right->header.num_keys));
    } else if(len_left < len_right) {

        diff = len_right - len_left;
        len_borrow = (diff >> 1);

        if(len_left + len_borrow > INTERNAL_SPLIT_TOLERANCE || len_borrow <= 1) return true;

        // Copy records from right to left
        left->keyValue[len_left].key = change_key_in_parent(m_right, right->keyValue[len_borrow-1].key);
        left->keyValue[len_left].offset = right->header.one_more_offset;
        set_parent(table_id, right->header.one_more_offset / PAGE_SIZE, m_left->page_num);
        for(int idx = 0; idx < len_borrow-1; ++idx) {
            memcpy(
                left ->keyValue + (len_left + idx + 1),
                right->keyValue + idx,
                sizeof(InternalKeyValue)
            );
            set_parent(table_id, right->keyValue[idx].offset / PAGE_SIZE, m_left->page_num);
        }
        // Shift records in right
        right->header.one_more_offset = right->keyValue[len_borrow-1].offset;
        for(int idx = len_borrow; idx < len_right; ++idx) {
            memcpy(
                right->keyValue + (idx - len_borrow),
                right->keyValue + idx,
                sizeof(InternalKeyValue)
            );
        }

        // Adjust number of keys in page
        left ->header.num_keys += len_borrow;
        right->header.num_keys -= len_borrow;

        // Calculate dirty area and register it
        register_dirty_page(m_left , make_dirty(0, PAGE_HEADER_SIZE + sizeof(InternalKeyValue) * left ->header.num_keys));
        register_dirty_page(m_right, make_dirty(0, PAGE_HEADER_SIZE + sizeof(InternalKeyValue) * right->header.num_keys));
    }

    return true;
}

/* 
 * Find record having the key, and update the value
 */
int update(int table_id, llu key, char * value)
{
    MemoryPage * m_leaf = find_leaf(table_id, key);
    LeafPage * leaf = (LeafPage*)(m_leaf->p_page);

    int idx = find_idx_from_leaf(m_leaf, key);

    /* 
     * If couldn't find record having the key
     */
    if(idx == -1) {
        unpin(m_leaf);
        return -1;
    }

    strcpy(leaf->keyValue[idx].value, value);
    int commit_left = PAGE_HEADER_SIZE + sizeof(Record) * idx;
    int commit_right = commit_left + sizeof(Record);
    register_dirty_page(m_leaf, make_dirty(commit_left, commit_right));

    unpin(m_leaf);

    return 0;
}

/*
 * This algorithm is O(A+B)
 * Linearly, Sequencely Visit all records of two tables
 * from first to end, to find same key between two tables
 */
int join_table(int table_id_1, int table_id_2, char * pathname)
{
    char cbuf[300];
    int buf_len;
    int page_buf_len = 0;
    int file_len = 0;

    int fd = open(pathname, O_RDWR | O_CREAT, 0644);
    if(fd == 0) {
        myerror("Failed to open file in join_table");
        return -1;
    }

    MemoryPage * m_buf = get_bufpage();
    Page * buf = (Page*)(m_buf->p_page);

    MemoryPage * m_left_leaf  = find_first_leaf(table_id_1);
    MemoryPage * m_right_leaf = find_first_leaf(table_id_2);

    LeafPage * left_leaf  = (LeafPage*)(m_left_leaf ->p_page);
    LeafPage * right_leaf = (LeafPage*)(m_right_leaf->p_page);

    int left_idx  = 0;
    int right_idx = 0;

    int end = 0;

    while(end == 0) {
        lld left_key = left_leaf->keyValue[left_idx].key;

        // Move next until finding same key in the right leaf
        while(right_leaf->keyValue[right_idx].key < left_key) {
            // If end of leaf, move next to the sibling
            if(right_idx + 1 >= right_leaf->header.num_keys) {
                if(right_leaf->header.right_offset == 0) {
                    end = 1;
                    break;
                }
                unpin(m_right_leaf);
                m_right_leaf = get_page(table_id_2, right_leaf->header.right_offset / PAGE_SIZE);
                right_leaf = (LeafPage*)(m_right_leaf->p_page);
                right_idx = -1;
            }
            ++right_idx;
        }
        if(end) break;

        // Found same key in the right leaf
        if(left_key == right_leaf->keyValue[right_idx].key) {
            sprintf(
                cbuf,
                "%lld,%s,%lld,%s\n",
                left_key,
                left_leaf->keyValue[left_idx].value,
                left_key,
                right_leaf->keyValue[right_idx].value
            );
            buf_len = strlen(cbuf);

            if(page_buf_len + buf_len >= 0x1000) {
                pwrite(fd, buf->bytes, page_buf_len, file_len);
                file_len += page_buf_len;
                page_buf_len = 0;
            }

            strcpy(buf->bytes + page_buf_len, cbuf);
            page_buf_len += buf_len;
        }

        // If end of leaf, move next to the sibling
        if(left_idx + 1 >= left_leaf->header.num_keys) {
            if(left_leaf->header.right_offset == 0) {
                end = 1; 
                break;
            }
            unpin(m_left_leaf);
            m_left_leaf = get_page(table_id_1, left_leaf->header.right_offset / PAGE_SIZE);
            left_leaf = (LeafPage*)(m_left_leaf->p_page);
            left_idx = -1;
        }
        ++left_idx;
    }
    pwrite(fd, buf->bytes, page_buf_len, file_len);
    file_len += page_buf_len;
    page_buf_len = 0;

    unpin(m_buf);
    unpin(m_left_leaf);
    unpin(m_right_leaf);

    close(fd);

    return 0;
}
