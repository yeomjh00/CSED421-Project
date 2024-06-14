# EduBfM Report

Name: Jaehu Yeom

Student id: 20180740

# Problem Analysis

이번 프로젝트는 주어진 버퍼에 대해, 버퍼를 관리하는 버퍼 매니저 역할의 함수를 구현하는 것이다.

두 가지 종류의 버퍼가 존재하며, 각 버퍼마다, `bufinfo` 변수가 버퍼와 해시테이블의 메타 정보를 담고 있다. 실질적인 버퍼는 `bufferPool` 변수가 담당하고 았으며,
 `bufTable` 타입의 배열이 같은 인덱스를 공유하는 `bufferPool`의 원소의 메타 정보를 담고 있다.

 각 페이지는 API를 통해 관리하며, 각 API는 아래와 같다. (i.e. High level interface)
 1. GetTrain: Page/Train을 fix하고, 불러온 Page가 저장된 버퍼의 인덱스를 반환한다.
 2. FreeTrain: Page/Train을 unfix한다.
 3. SetDirty: DirtyBit을 Set
 4. FlushAll: 모든 버퍼 내의 페이지에 대해 만약 DirtyBit가 Set되어 있다면, flush한다.
 5. DiscardAll: 버퍼풀에 존재하는 모든 Page/Train에 대해 disk에 기록하지 않고 삭제한다. (explicit한 삭제가 아닌, 해당 자료에 대한 Trace를 삭제해서 잃어버리는 방식)

그리고 이를 구현하기 위한 internal function은 아래와 같다. (i.e. Low level interface)
1. readTrain: disk로부터 정보를 읽어와 파라미터로 넘긴 주소에 정보를 저장한다.
2. allocTrain: 페이지를 읽어오기 위해, buffer pool에서 element를 찾는 과정이다. replacement policy에 따라 victim을 결정한다.
3. insert: 해시 테이블에 읽어온 페이지에 대한 정보를 저장한다. 해시 테이블의 coliision handling method는 아래에 서술한다.
4. delete: 특정 페이지에 대한 관리 정보를 삭제한다. 아래에서 언급하듯, chaining method를 사용하기 때문에 linked list의 삭제 방식과 유사하다.
5. deleteAll: 해시 테이블의 모든 정보를 삭제한다.
6. lookUp: 특정 key를 가진 페이지가 담긴 인덱스를 검색해서 반환한다.
7. flushTrain: 만약 page의 Dirty Bit이 set되어 있다면, flush한다. 

버퍼에 접근하는 것은 해시 테이블을 통해서 이뤄지며, 해시 테이블의 원소는 마지막으로 삽입된 페이지가 담긴 버퍼의 인덱스를 가지고 있다. 
(`hashTable[hash_function(key)] <- latest inserted buffer index whose hash value equals to hash_function(key)`) 그리고, `bufTable[index].nextHashEntry`는 같은 해시 값을 가진 다른 버퍼의 인덱스를 가리키고 있다.
즉, `hashTable`과 `bufTable` 두 변수가 하나의 chaining method를 사용하고 있으며, 디스크에서 페이지를 읽어 버퍼에 저장할 땐, linear probing method를 사용한다.

페이지를 읽어오는 도중, replacement policy로는 Clock algorithm을 사용한다. Clock hand에 해당하는 변수는 `bufinfo.nextVictim`에 저장되어 있다.


# Design For Problem Solving

## High Level

DBMS의 가장 크고, 기본적인 기능은 CRUD이다. 하지만, 더 크게 나눈다면 Read/Write로 나눌 수 있을 것이다.
즉, Create, Update는 Write에 속하고, Read는 단순히 데이터를 읽는 것이며, Delete는 해당 정보를 관리하는 매니저에서 관리를 중지하면 된다.

Write에 해당하는 기능으로는,
1. Optional Stage
   1. 비어있는 버퍼 선정 or, Victim 선정
   2. Disk에서 읽어오기
2. 데이터 작성 및 메타 정보 업데이트
가 될 것이고,

Read에 해당하는 기능으로는
1. 비어있는 버퍼 선정 or, Victim 선정
2. Disk에서 읽어오기
가 될 것이다.

마지막으로 Delete에는
1. 메타 정보를 저장하는 변수에서 정보 업데이트
2. 혹은, 해시 테이블 등에서 해당 정보를 가리키는 정보 삭제
가 될 것이다.

위의 대략적인 구성요소를 기능 별로 나누어 함수로 만든 것이, Problem Analysis에 있던, API와 internal function이라고 할 수 있다.

## Low Level

 APIs
 1. GetTrain: Page/Train을 fix하고, 불러온 Page가 저장된 버퍼의 인덱스를 반환한다.
    ```   
    case: page does not exist in buffer
        index = allocTrain();
        readFromDisk(index);
        update_meta_info();
        update_hash_table();

    case: exists in buffer
        update_meta_info();
    
    fix_page();
    ```
 2. FreeTrain: Page/Train을 unfix한다.
    ```
    find_page();
    unfix_page();
    ```
 3. SetDirty: DirtyBit을 Set
    ```
    find_page();
    dirty_bit_update();
    ```
 4. FlushAll: 모든 버퍼 내의 페이지에 대해 만약 DirtyBit가 Set되어 있다면, flush한다.
    ```
    for page in pages:
        flush if dirty
    ```
 5. DiscardAll: 버퍼풀에 존재하는 모든 Page/Train에 대해 disk에 기록하지 않고 삭제한다. (explicit한 삭제가 아닌, 해당 자료에 대한 Trace를 삭제해서 잃어버리는 방식)
    ```
    for element in hashTable:
        element <- invalid data
    ```

Internal functions
1. readTrain: disk로부터 정보를 읽어와 파라미터로 넘긴 주소에 정보를 저장한다.
    ```
    for given address ptr,
    ptr <- read_from_disk();
    ```
2. allocTrain: 페이지를 읽어오기 위해, buffer pool에서 element를 찾는 과정이다. replacement policy에 따라 victim을 결정한다.
    ```
    victim <- using second change algorithm
    flush victim if dirty bit set
    update meta data
    update hash table
    ```
3. insert: 해시 테이블에 읽어온 페이지에 대한 정보를 저장한다. 해시 테이블의 collision handling method는 아래에 서술한다.
    ```
    hashValue <- hash(key)
    if hashTable[hashValue] is Nil:
        hashTable[hashValue] <- index
    else:
        insert page like linked-list
    ```
4. delete: 특정 페이지에 대한 관리 정보를 삭제한다. 아래에서 언급하듯, chaining method를 사용하기 때문에 linked list의 삭제 방식과 유사하다.
    ```
    hashValue <- hash(key)
    if nextHashEntry of hashTable[hashValue] is Nil:
        hashTable[hashValue] <- Nil
    else:
        delete page like linked-list
    ```
5. deleteAll: 해시 테이블의 모든 정보를 삭제한다.
    ```
    for element in hashTable:
        element <- Nil
    ```
6. lookUp: 특정 key를 가진 페이지가 담긴 인덱스를 검색해서 반환한다.
    ```
    index <- hash(key)
    while index is not Nil:
        return if key matched
        index <- nextHashEntry of bufTable[index]
    ```
7. flushTrain: 만약 page의 Dirty Bit이 set되어 있다면, flush한다. 
    ```
    for index in bufTable:
        flush indexed buffer element if dirty 
    ```


# Mapping Between Implementation And the Design

아래는 위의 Design에서 적은 것들을 `Low level`에 적은 것과 매칭한 결과이다.

Write:
1. Optional Stage
   1. 비어있는 버퍼 선정 or, Victim 선정 = getTrain (Victim 선정 중, FreeTrain)
   2. Disk에서 읽어오기
2. 데이터 작성 및 메타 정보 업데이트 = SetDirty
가 될 것이고,

Read:
1. 비어있는 버퍼 선정 or, Victim 선정 = getTrain (Victim 선정 중, FreeTrain)
2. Disk에서 읽어오기
가 될 것이다.

Delete
1. 메타 정보를 저장하는 변수에서 정보 업데이트
2. 혹은, 해시 테이블 등에서 해당 정보를 가리키는 정보 삭제 = FlushAll / DiscardAll
가 될 것이다.


그리고 API를 다시, internal function과 매칭한 것은 아래와 같다.

 1. GetTrain: Page/Train을 fix하고, 불러온 Page가 저장된 버퍼의 인덱스를 반환한다.
    ```
    index => lookUp
    case: page does not exist in buffer
        index = allocTrain(); => allocTrain
        readFromDisk(index); => readTrain
        update_meta_info(); 
        update_hash_table(); => insert

    case: exists in buffer
        update_meta_info();
    
    fix_page();
    ```
 2. FreeTrain: Page/Train을 unfix한다.
    ```
    find_page(); => lookUp
    unfix_page();
    ```
 3. SetDirty: DirtyBit을 Set
    ```
    find_page(); => lookUp
    dirty_bit_update();
    ```
 4. FlushAll: 모든 버퍼 내의 페이지에 대해 만약 DirtyBit가 Set되어 있다면, flush한다.
    ```
    index => lookUp
    for page in pages:
        flush if dirty => flushTrain
    ```
 5. DiscardAll: 버퍼풀에 존재하는 모든 Page/Train에 대해 disk에 기록하지 않고 삭제한다. (explicit한 삭제가 아닌, 해당 자료에 대한 Trace를 삭제해서 잃어버리는 방식)
    ```
    for element in hashTable:
        element <- invalid data => DeleteAll
    ```