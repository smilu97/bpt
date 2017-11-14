## 제공되는 함수

```C
int init_db(int buf_num);
```

메모리 버퍼 페이지를 제공하기 위해, DB를 사용하기전 반드시 호출해야 합니다. 첫 번째 인자로, 예약할 페이지 개수를 지정합니다. 많으면 많을수록 많은 메모리 공간을 소비하며, 동시에 디스크와의 IO가 줄어듭니다.

```C
int open_table(const char * pathname);
```

파일을 열어서 사용할 수 있도록 만듭니다.

성공적으로 파일이 열렸을 경우 파일의 디스크립터를 리턴합니다. 만약, 파일을 열어서 확인하는 데 실패했을 경우 0을 리턴합니다.

```C
char * find(int table_id, unsigned long long key);
```

key값을 이용해서 테이블에 존재하는 value를 찾아 리턴합니다. 없을경우 NULL을 리턴합니다.

```C
int insert(int table_id, unsigned long long key, const char * value);
```

key값에 해당하는 곳에 value를 대입합니다.

성공시 0, 실패시 0이 아닌 값을 리턴합니다.

```C
int delete(int table_id, unsigned long long key);
```

key값에 해당하는 곳에 있던 record를 삭제합니다.

성공시 0, 실패시 0이 아닌 값을 리턴합니다.

```C
int close_table(int table_id);
```

열었던 파일을 닫습니다.

`LRUNode` 리스트로부터 해당 테이블과 관련된 메모레 페이지들을 모두 사용해제 하고 디스크와 동기화합니다.

성공시 0, 실패시 0이 아닌 값을 리턴합니다.

```C
int shutdown_db();
```

여태까지 열었던 모든 테이블들을 닫고 동기화합니다.

할당되었던 메모리 페이지 공간을 해제합니다.

성공시 0, 실패시 0이 아닌 값을 리턴합니다.

## 메모리 캐싱

이 프로젝트에서는 [LRU알고리즘](https://en.wikipedia.org/wiki/Cache_replacement_policies#Least_Recently_Used_.28LRU.29)을 사용하고 있습니다

`LRUNode` 객체는 로직 내부에서, 더블 링크드 리스트 형태로 관리되고 있습니다. `"lru.h"` 파일에서 `lru_head`, `lru_tail` 포인터를 이용해 접근됩니다. 특정 페이지는 B+ Tree로직에서 참조될 때 마다 링크드 리스트의 Tail로 업데이트 됩니다.

이 `LRUNode` 더블 링크드 리스트는, 나중에 지정된 메모리 상의 캐시 페이지가 부족할 경우에 할당 취소될 페이지를 결정하는 역할을 하며, (대부분의 경우) 현재 HEAD에 있는 `LRUNode`가 가지고 있는 `page_num`에 해당하는 페이지가 사용됩니다. 하지만 HEAD에 있는 `LRUNode`가 가리키는 메모리 페이지가 pinned상태일 경우에, 그 뒤에 있는 LRUNode를 해제 시키고 그것이 가리키고 있던 메모리 페이지를 씁니다. 만약에 이 뒤에 있던 `LRUNode`의 것도 pinned일 경우 마찬가지로 계속 뒤에 있는 것을 검사해서 씁니다.

만약 모든 메모리 페이지가 pinned상태일 경우 에러가 납니다. 이러한 상황을 방지하기 위해 알고리즘이 요구하는 최소한의 메모리 페이지 개수가 보장되어야 합니다.(최소 100개 이상을 추천합니다) 이 최소 메모리페이지 개수는 일반적으로 트리의 레벨이 높아질수록 높아집니다.

## MemoryPage

`"page.h"` 파일에서 찾아볼 수 있는 이 구조체는 메모리 캐시페이지의 유닛입니다. 이 메모리 페이지는 `MAX_MEMPAGE` 개 만큼, `int open_db(const char*)` 함수가 호출될 때, 힙에 Serialized 상태로 한꺼번에 할당됩니다.

### Member

* Dirty * dirty
  * dirty는 {int, int} 형태로 이루어진 구조체이며, 현재 디스크와 다를 수 있는 영역을 가리키고 있습니다.
  * 여러 영역을 가리키기 위해 다음 dirty를 가리키고 있는 단방향 리스트 형태를 띄고 있습니다.
  * 연결되어 있는 Dirty들 끼리는 서로 중복되지 않게 관리됩니다.
* cache_idx
  * Serialized된 MemoryPage배열에서 자신이 몇 번째인지 가리킵니다.
* page_num
  * 자신이 가리키고 있는 페이지가 몇 번인지 저장합니다.
* pin_count
  * 현재 메모리 페이지가 알고리즘에 의해 사용되고 있는지 표시합니다.
  * 이 값이 1 이상일 경우, LRUNode리스트의 앞에 있더라도 메모리에서 해제되지 않도록 보호받습니다.
* p_lru
  * 자신이 속한 `LRUNode`를 가리키고 있습니다. `MemoryPage`와 `LRUNode`는, 사용중일 때에 한하여, 서로를 OnebyOne으로 가리킵니다.
* p_page
  * page의 content를 담고 있는 버퍼를 가리킵니다.
* next
  * `MemoryPage`가 사용중일 경우, `next`는 자신의 page_num을 해슁한 결과가 같은 다른 `MemoryPage`를 가리킵니다. 해슁 결과가 같은 `MemoryPage`간의 관계를 필자는 `Hash Friend`라고 부르고 있으며, 이 `Hash Friend`들은 모두 `next`포인터를 통하여 하나의 같은 단방향 링크드 리스트로 연결됩니다.
  * 사용중이지 않을 경우, 즉 이 `MemoryPage`가 Free상태일 경우, 다른 Free `MemoryPage`를 가리킵니다. 마찬가지로 Free상태인 `MemoryPage`들은 모두 같은 단방향 링크드 리스트로 연결되어 있으며, `"page.h"`에 정의되어있는 `MemoryPage * free_mempage` 글로벌 변수가 그 HEAD를 가리키고 있습니다.


### Finding in hash table

* `"hash.c"`에 제공되는 `hash_llu(llu val, llu mod)`함수를 사용해서 찾으려는 페이지의 번호$N$을 해시함수를 적용시켜 $H = \text{hash}(N)$ 를 만듭니다. 2번째 인자인 mod에는 `"page.h"`에 정의되어있는 `MEMPAGE_MOD` 를 넣어줍니다.
* 전역변수로 선언되어 있는 `MemoryPage * page_buf[MEMPAGE_MOD]` 에서 `page_buf[H]`를 가져옵니다.
* `page_buf[H]`의 Hash Friends중에 찾으려는 페이지를 담고있는 메모리 페이지가 존재합니다.

당연히, 이 Hash Friends중에서 찾으려는 페이지를 찾는 알고리즘은 $O(n)$ 시간이 걸립니다. 하지만, 정의되어 있는 MEMPAGE_MOD 상수를 높이면 높일수록, Hash table의 가용성이 줄어드는 대신, 더 빠른 시간에 `MemoryPage` 를 찾을 수 있습니다.