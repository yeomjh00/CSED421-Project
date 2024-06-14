# EduBtM Report

Name: Jaehu Yeom

Student id: 20180740

# Problem Analysis

전체적인 문제는 데이터베이스 시스템에 필요한 인덱스와 관련된 함수를 작성하는 것이다.

인덱스는 지원하는 기능과 방식에 따라 여러 방식으로 구현될 수 있는데, 이번 프로젝트는 Single Key를 지원하는 B+ Tree 방식을 사용한다.

크게, 인덱스를 생성/삭제하고, 생성한 인덱스에 대해 오브젝트를 CRUD하는 기능을 구현해야 한다.

인덱스에 넣는 오브젝트는 single key를 가지고 있으며, key type도 String / Int 로 제한되어 있다. 또한, 시스템 상 duplicated key를 지원할 수 있는데, 이번 프로젝트에서는 duplicated key를 가진 오브젝트를 허용하지 않는다. (nObject value must be 1 at most.)

B+Tree를 구성하는 노드에는 Internal / Leaf 노드 두 가지가 있다. 일반적인 B+Tree와 Abstract funcionality는 동일하다. 다만, Internal 노드가 저장하는 오브젝트는 해당 키 값의 범위를 가진 하위 노드로의 pageID를 가진다. 반면 Leaf 노드는 index가 아닌, whole information을 가진 object를 포함하는 pageID를 가진 object를 가지고 있다.

코드를 통해 보자면, `Internal Entry = [pageID, keyLength, keyValue]`, `Leaf Entry = [nObject = 1, keyLength, keyValue, ObjectID]` 이다. 

Key에 대한 메타정보는 `keyDesc`에 저장되어 있으며, key에 대한 값과 길이는 `KeyValue`에 저장한다.

더 자세한 내용은 아래 Design에서 다루도록 한다.

# Design For Problem Solving

## High Level
* `EduBtM_CreateIndex()`

    새로운 인덱스를 생성한다. Root를 생성하고 Root에 대한 `pageID`를 반환한다.

* `EduBtM_DropIndex()`

    인덱스를 drop한다. B+Tree 구조를 활용하여, recursive하게 B+Tree를 구성하는 page의 데이터를 deallocate하면 된다.

* `EduBtM_InsertObject()`

    주어진 object를 key값에 맞게 insert한다. root에서 internal, leaf로 recursive하게 key value를 비교하며 알맞은 페이지에 삽입한다.

    크게, Root / Internal / Leaf 에서의 작업으로 나눌 수 있다. Root에서는 Internal과 비슷하게 작동하나, 만약 Root 노드가 Split이 필요하면, 기존 Root의 pageID를 유지한 채로, 새로 페이지를 할당받아 데이터를 복사해 데이터를 나눠준다. Internal 노드에서 split이 일어나는 경우에는 upper level의 internal node가 처리하게 한다.

    Internal에 insert가 일어나는 경우는 leaf에서 split이 일어나는 경우, 새로 할당받은 페이지를 가리키는 pageID와 key value를 넣어준다.

    Leaf 노드는 실질적인 모든 정보를 저장하는데, `slot`에는 key 정보의 ordering에 맞게 저장한다. 

* `EduBtM_DeleteObject()`

    주어진 key/objectID에 맞는 object를 삭제한다. 이 과정 중에서, split이 일어나거나, root가 삭제될 수 있다. 이 경우 적절한 함수 호출을 통해 처리한다.

* `EduBtM_Fetch()`

    특정 값과 조건을 충족하는 object를 찾아가서, 해당 정보를 가진 `cursor`을 return한다. 이 때, 해당 `cursor`는 object를 포함하는 page의 정보, objectID를 포함한다.

    `insertObject`와 비슷하게 recursive하게 조건을 충족하는 object를 찾아가야 한다. 다만, insert와 다르게 해당 조건을 충족하는 object가 실제로 찾은 leaf page의 previous/next page일 수 있다. 이를 고려해야 한다.

* `EduBtM_FetchNext()`

    이전에 찾은 `cursor`와 주어진 `Comparision Operator: CompOp`을 고려하여, 그 다음 `cursor`을 찾는다.

    이후, 조건을 충족하는지 여부에 따라 상태를 업데이트한다.

## Low Level

* `edubtm_InitLeaf()` / `edubtm_InitInternal()`

    주어진 `pageID`에 대해 meta-data를 초기화해주는 작업이다.
    공통적으로 `pageID`, `offset`, `number of slot` 등을 초기화해준다.
    각각 `Leaf`, `Internal` 중 어떤 타입의 노드인지 bit를 통해 명시해주고, Root 여부도 표시한다.

    이후, 초기화된 값이 disk에도 적용될 수 있도록, dirty bit를 set해준다.
  
* `edubtm_FreePages()`

    특정 Index를 Drop할 때, 내부적으로 사용할 수 있는 함수이다. Tree 구조를 잘 활용하기 위해서, Recursive function으로 디자인하였다.

    Page가 가지고 있는 모든 children node에 대해서 recursive하게 `edubtm_FreePages()` 를 호출해줬고, 호출된 페이지는 모두 `FREEPAGE` type으로 바뀌었고, 이전 project에서 거친 것과 동일하게, deallocate해주었다.

* `edubtm_Insert()`

    `Insert` 기능을 담당하는 여러 함수 중 가장 overall한 작업을 맡고 있다. 
    
    이전 high-level design에서 언제 `insertLeaf`를 할 지, 언제 `insertInternal`을 할 지 담당하는 함수이다.

    먼저, `edubtm_BinarySearch (Leaf/Internal)`를 통해서 insert하기 위한 적절한 위치를 찾아준다.

    B+Tree에서 실제로 데이터를 저장하는 것은 Leaf 노드이기 때문에, 적절한 Leaf 노드에 도달할 경우, `edubtm_InsertLeaf`를 호출하고, Dirty bit를 set해준다.

    하지만, 만약 Leaf 노드에서 Split이 일어나거나, Internal 노드에 entry를 삽입해야 하거나 (`edubtm_InsertInternal`), 연쇄적으로 Internal 노드에 Split이 일어나 Upper-level에 삽입을 하는 경우(`edubtm_InsertInternal`)를 담당한다.
    
     Root 노드와 관련된 처리가 필요한 경우 High level API인 `EduBtM_InsertObject`에서 적절한 처리를 해준다.

* `edubtm_InsertLeaf()`

    `edubtm_BinarySearchLeaf`를 통해서, 어떤 위치에 entry가 들어가야 할 지 결정한다.

    이후, entry가 들어갈 수 있는지 판단하고, 가능하다면 `compact`를 통해서 여유 공간을 확보한다. 만약, 공간이 없다면, `split`을 진행한다.

    이제 모든 경우를 나누었기 때문에, entry를 넣을 수 있는 충분한 공간이 있다. `slot` 상에서 데이터가 정렬된 상태여야 하기 때문에, `slot`을 옮겨주고, 데이터를 복사해준다.

* `edubtm_InsertInternal()`

    `edubtm_InsertLeaf`와 유사하게, `BinarySearch`를 통해서 적절한 위치를 먼저 찾아준다.

     이후, entry가 들어갈 수 있는지 판단하고, 가능하다면 `compact`를 통해서 여유 공간을 확보한다. 만약, 공간이 없다면, `split`을 진행한다.

     다만, `Leaf`와 `Internal`은 노드의 구성이 다르기 때문에, 이를 유의하여 `BinarySearch`, `compact`, `split`등을 각각에 맞춘 함수를 사용한다.

    이제 모든 경우를 나누었기 때문에, entry를 넣을 수 있는 충분한 공간이 있다. `slot` 상에서 데이터가 정렬된 상태여야 하기 때문에, `slot`을 옮겨주고, 데이터를 복사해준다.

* `edubtm_SplitLeaf()`

    하나의 `Leaf` 노드를 두 개의 `Leaf`로 나누는 과정이다. 기존 데이터의 명시적인 삭제 없이, 메타 데이터의 업데이트를 통해서, 읽기 혹은 덮어 쓰기를 허용할 수 있다.

    페이지 사이즈의 절반을 넘기게 되는 시점부터의 데이터를 새로 할당받은 페이지에 복사해준다. 이 때, 주의해야 할 점은 추가로 `insert`하려다 페이지 용량 상 하지 못한 데이터이다.

    이를 유의하여 적절한 페이지에 삽입을 해주면 된다.

* `edubtm_SplitInternal()`

    데이터가 가득 차, 이를 두 개의 `Internal` 노드로 나누는 과정이다. 먼저, 기존의 노드는 굳이 데이터를 삭제할 필요 없이, 메타데이터(e.g. `nSlot, free, unused`)를 업데이트 해줌으로써 데이터에 대한 읽기를 제한하거나, 쓰기를 허용할 수 있다.

    다만, 메타 데이터를 업데이트 하기 이전에, 크기를 계산하여, 새로운 `Internal`로 데이터를 옮겨준다. 이 때, 기준은 `Page Size`의 절반이다.

    이를 위해서, 새로운 페이지를 할당 받은 이후, 페이지 사이즈의 절반을 넘기게 되는 부분부터 할당받은 새 페이지로 복사한다.

* `edubtm_root_insert()`

    다른 Split과 유사하지만, 조금 다르게 처리해줘야 한다. 먼저 Root가 `Interal` 노드이고, 이 노드가 Split된 점에서 `edubtm_SplitInternal`과 유사하게 처리를 해줘야 한다. 

    즉, `edubtm_SplitInternal`을 통해 Split이 된 Internal Page가 두 개가 존재하고, 이를 새로운 Root를 만들어 children으로 이 둘을 넣어줘야 한다.

    하지만, root page의 page ID를 유지하기 위해서, 새로 할당받은 page를 root로 정하는 것이 아니라, 새로 할당받은 페이지에 root의 정보를 복제한 후, root를 다시 초기화하여 root로 사용한다.

* `edubtm_Delete()`

    삭제할 엔트리가 저장된 리프 페이지를 찾기 위해 적절한 자식 페이지를 결정합니다.
    결정된 자식 페이지를 루트로 하는 서브트리에서 재귀적으로 edubtm_Delete()를 호출하여 삭제를 수행합니다.
    자식 페이지에서 언더플로우가 발생한 경우, btm_Underflow()를 호출하여 처리합니다.

    btm_Underflow() 호출 후 루트 페이지의 내용이 변경되므로, 호출 후 루트 페이지의 DIRTY 비트를 1로 설정합니다.
    파라미터로 주어진 루트 페이지가 리프 페이지인 경우:

    만약 페이지가 Leaf인 경우, edubtm_DeleteLeaf()를 호출하여 해당 페이지에서 <object의 key, object ID> 쌍을 삭제합니다.
    이와 같이, edubtm_Delete() 함수는 재귀적으로 호출하여 B+ 트리에서 삭제 작업을 수행하며, 언더플로우를 적절히 처리합니다.

* `edubtm_DeleteLeaf()`

    `Leaf` 상에서 데이터를 삭제해준다. 이 때, 실제로 데이터를 삭제할 필요 없이 `slot`을 관리함으로써 데이터 접근을 제한할 수 있다.

     `unused` 혹은 `free`를 사이즈만큼 더해주고, `slot`은 빈 슬롯 없이 dense하게 유지해준다.

     만약 underflow가 발생하는 경우에는 out parameter인 `f`를 True로 set하여서 `edubtm_Delete`에서 처리할 수 있도록 한다.

* `edubtm_CompactLeafPage()` / `edubtm_CompactInternalPage()`

    unused area가 있음에도 contiguous free area가 부족하여 데이터를 삽입하지 못하는 경우 호출하는 함수이다.

    한 페이지를 완전히 동일하게 복사하여, slot 순서대로 데이터를 다시 정렬하여 `data` area에 하나씩 복사하여 저장한다.

    이 과정 중에서, `Internal`은 `p0` 변수를 고려하여 진행하면 되고, 이후에 페이지 헤더에 적절하게 정보를 업데이트할 수 있도록 한다.

* `edubtm_Fetch()`

    만약 `BOF`부터 시작한다면, `First Object`를, `EOF` 부터 시작한다면 `Last Object`를 반환하게 한다.

    특정 값과 Comparison operator가 주어진다면, 해당 조건에 부합하는 object를 찾도록 한다.

    기본적으로 recursive하게 `BinarySearch`를 사용하여 찾고, `Leaf`에 도달한다면, 주어진 Comparison Operator에 대해 조건이 부합하는지 확인하여 `cursor`의 state를 업데이트 해준다.

    `LT, LE, GT, GE`의 경우는 해당 페이지 이전 페이지의 object가, 혹은 해당 페이지 이후의 페이지의 object가 이 조건에 부합할 수 있기 때문에, 이 조건을 따로 염두하여 처리해줘야 한다.

* `edubtm_FetchNext()`

    이미 `edubtm_Fetch`가 실행된 상태에서, 즉 `cursor`가 주어진 상태에서, 조건을 부합하는 그 다음 데이터를 찾는 함수이다. 따라서, 굳이 `BinarySearch`를 사용할 필요 없이, 바로 페이지를 불러와서 다음 데이터에 대해 조건을 부합하는지 검증하면 된다.

    다만, 이전 `edubtm_Fetch`와 동일하게, 다음 데이터가 이전/다음 페이지에 존재할 수 있기 때문에, 이를 잘 처리해줘야 한다.

    그렇게 찾은 데이터에 대해 Comparison Operator의 경우에 맞게 조건이 맞는지 확인해준다.

    이를 바탕으로 return할 `cursor`의 데이터를 업데이트해준다.

* `edubtm_FirstObject()` / `edubtm_LastObject()`

    Index 내의 가장 첫번째 혹은 가장 마지막 object를 찾아주는 함수이다. 가장 처음 혹은 마지막이라는 확실한 조건이 있기 때문에, `p0` 혹은 `nSlot-1`번재 slot 만을 따라가서

    Leaf 노드에서도 slot array 내의 가장 첫, 혹은 마지막에 해당하는 데이터를 읽어서 리턴하면 된다.

* `edubtm_BinarySearchLeaf()` / `edubtm_BinarySearchInternal()`
  
  단순한 Binary Search 문제이다. 특히, 만약 그런 데이터가 없는 경우, 기준보다 작으면서 가장 큰 데이터에 대한 위치를 반환하는 것까지 기존의 Binary Search 문제와 동일하기 때문에,

  해당 함수를 그대로 옮겨주면 된다.

* `edubtm_KeyCompare()`

    key를 비교해주는 함수이다. 이 함수가 필요한 이유는 key가 여러 개로, 여러 타입 혹은 여러 길이로 존재할 수 있기 때문이다.

    하지만, 이번 프로젝트에서는 key가 1개만 존재할 수 있으며, 동시에 `String`, `int` 두 타입 중 하나이기 때문에, 순서대로 비교해주면 된다.

    `string`의 경우, `\0`이 나올 수 있기 때문에 이를 유의하여 설계하면 된다.

# Mapping Between Implementation And the Design

프로젝트 설명에 나와있는 `Function Call Graph`를 참고하여 구현하였다.

`KeyCompare, Compact, init, FreePages`의 경우는 dependent가 없기 때문에, 위에서 서술한 설명대로 구현하였다.

그리고 `keyCompare`를 기반으로 `First/Last Object, NextFetch, BinarySearch`을 구현하였고, `init`을 바탕으로 `Split, root_insert`을 구현하였다.

위에 구현된 함수를 바탕으로 `insert / delete / fetch`등을 구현할 수 있었다.
