# Disk-based B+ Tree implementation

Author: smilu97

이 프로젝트는 ITE 2038-11735 Database Systems과목 두 번째 과제로부터 시작되었습니다.



대부분의 구현은 `docs`폴더 안의 pdf 과제명세서가 지시하는 대로 되어있습니다. 이 이외의 특별한 구현방식등은 최대한 README.md 혹은 github, gitlab의 wiki 페이지에 서술하겠습니다.

## 제공되는 함수

```C
int open_db(const char * pathname);
```

db 파일을 열어서 초기화합니다. 나머지 세 함수를 쓰기 전에 반드시 호출되어야 합니다

```C
char * find(unsigned long long key);
```

key값을 이용해서 DB에 존재하는 value를 찾아 리턴합니다. 반환되는 `char*` 포인터는 내부 로직에서 malloc되어 힙 영역을 가리키고 있으므로, 내부 로직에서 메모리가 해제될 수 있으므로, 지속적으로 사용하는 것은 위험합니다.

```C
int insert(unsigned long long key, const char * value);
```

key값에 해당하는 곳에 value를 대입합니다.

```C
int delete(unsigned long long key);
```

key값에 해당하는 곳에 있던 record를 삭제합니다.

## 메모리 캐싱

이 프로젝트에서는 [LRU알고리즘](https://en.wikipedia.org/wiki/Cache_replacement_policies#Least_Recently_Used_.28LRU.29)을 사용하고 있습니다

`LRUNode` 객체는 로직 내부에서, 더블 링크드 리스트 형태로 관리되고 있습니다. `"lru.h"` 파일에서 `lru_head`, `lru_tail` 포인터를 이용해 접근됩니다. 특정 페이지는 B+ Tree로직에서 참조될 때 마다 링크드 리스트의 Tail로 업데이트 됩니다.

이 `LRUNode` 더블 링크드 리스트는, 나중에 지정된 메모리 상의 캐시 페이지가 부족할 경우에 할당 취소될 페이지를 결정하는 역할을 하며, 현재 HEAD에 있는 `LRUNode`가 가지고 있는 `page_num`에 해당하는 페이지가 메모리에서 해제됩니다.

아직 이 알고리즘에서는 pinned개념을 사용하고 있지 않기 때문에, 메모리 캐시페이지의 개수를 결정하는 `MAX_MEMPAGE` 상수가 너무 작을 경우에 전체 로직이 동작하지 않을 수 있습니다.

## MemoryPage

`"page.h"` 파일에서 찾아볼 수 있는 이 구조체는 메모리 캐시페이지의 유닛입니다. 이 메모리 페이지는 `MAX_MEMPAGE` 개 만큼, `int open_db(const char*)` 함수가 호출될 때, 힙에 Serialized 상태로 한꺼번에 할당됩니다.

### Member

* Dirty dirty
  * dirty는 {int, int} 형태로 이루어진 구조체이며, 현재 디스크와 다를 수 있는 영역을 가리키고 있습니다.
* cache_idx
  * Serialized된 MemoryPage배열에서 자신이 몇 번째인지 가리킵니다.
* page_num
  * 자신이 가리키고 있는 페이지가 몇 번인지 저장합니다.
* p_lru
  * 자신이 속한 `LRUNode`를 가리키고 있습니다. `MemoryPage`와 `LRUNode`는, 사용중일 때에 한하여, 서로를 OnebyOne으로 가리킵니다.
* p_page
  * page의 content를 담고 있는 버퍼를 가리킵니다.
* next
  * `MemoryPage`가 사용중일 경우, `next`는 자신의 page_num을 해슁한 결과가 같은 다른 `MemoryPage`를 가리킵니다. 해슁 결과가 같은 `MemoryPage`간의 관계를 필자는 `Hash Friend`라고 부르고 있으며, 이 `Hash Friend`들은 모두 `next`포인터를 통하여 하나의 같은 단방향 링크드 리스트로 연결됩니다.
  * 사용중이지 않을 경우, 즉 이 `MemoryPage`가 Free상태일 경우, 다른 Free `MemoryPage`를 가리킵니다. 마찬가지로 Free상태인 `MemoryPage`들은 모두 같은 단방향 링크드 리스트로 연결되어 있으며, `"page.h"`에 정의되어있는 `MemoryPage * free_mempage` 글로벌 변수가 그 HEAD를 가리키고 있습니다.



## Dirty queue

Dirty한(디스크와 내용이 맞지 않는) 페이지들을 담고 있습니다.

Linear time에 중복을 검사해서 element들을 unique하게 관리합니다.

```C
Dirty make_dirty(int left, int right);  // Dirty한 영역의 범위를 가리키는 구조체를 생성합니다
void update_dirty(Dirty * dest, Dirty src);  // Dirty구조체를 더합니다
int register_dirty_page(MemoryPage * m_page, Dirty dirty);  // Dirty page를 queue에 넣습니다.
int commit_dirty_pages();  // Dirty페이지들을 모두 청소합니다(커밋합니다)
```

위의 함수들을 통해서 관리되고 있습니다.