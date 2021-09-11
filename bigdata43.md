# 4-3 시계열 데이터의 최적화

스트리밍형 메시지 배송시 메시지 도달 시간이 지연되면 어떤 문제가 발생할까? 어떻게 해결해야할까?

## 프로세스 시간과 이벤트 시간
- 이벤트 시간: 클라이언트에서 메시지가 생성된 시간
- 프로세스 시간: 서버가 메시지를 처리한 시간
- 데이터 분석 대상은 주로 이벤트 시간

## 프로세스 시간에 의한 분할과 문제점
-  보통은 이벤트가 발생하고, 조금의 시간이 흐른 후에 서버에 도착함 -> 이벤트 시간과 프로세스 시간의 차이가 적은 것이 보통
-  하지만 모바일 기기의 시계는 미쳤음
   - 한참 전의 메시지가 10일 후에 오기도 하고, 미래의 메시지가 오기도 함
   - 모바일 기기의 타이머가 잘못되기도 하고, 네트워크 이슈로 한참 후에 도착하기도 함
- 서버에 도착하는 시간을 바탕으로 메시지를 저장하면, 이벤트 시간을 기준으로 처리하기 위해서는 여러 날짜의 로그를 fullscan 해야 하기도 함
  -  다수의 파일을 모두 검색해야 하니 부하가 크게 높아짐

##  시계열 인덱스를 통해 해결하기
- 이벤트 시간에 인덱스를 만듬
- 대량의 데이터를 집계하는 분산 데이터베이스에서는 그다지 효율적이지 않음

## Columnar storage
- 주기적 배치처리를 통해, 이벤트 시간으로 정렬된 Columnar Storage로 변환
    - 변환과정에서 파일에 각 칼럼별 최소/최대 시간을 저장할 수 있음
    - 예) 20210101.table - 프로세스 시간(1/1 00:00~1/1 23:59) 이벤트 시간 (12/30 05:50 ~ 1/1 23:59)
- 조건절 푸쉬다운(Predicate pushdown)을 통해, 필요한 데이터만 읽음
     - 이벤트 시간으로 정렬되어있으니, 해당 영역의 파일만 읽음
- 단 배치처리를 너무 자주하면 오히려 비효율적. 가령 1분단위로 배치처리를 하면 읽어야 하는 파일이 매우 많아질 수 있음

## Time-series Table
- 처음부터 이벤트 시간을 바탕으로 Partition을 만듬
- 메시지 수신시 이벤트 시간을 바탕으로 각 Partition에 데이터를 분산해서 추가함
- 제한 없이 계속 추가하면, 몇년 후 도착한 메시지에 의해 fragmentation된 파일이 계속 추가될 지도 모름

## 데이터마트
- 데이터 마트에 프로세스 시간만을 이용해 데이터 저장함
# 4-4 비구조화 데이터의 분산 스토리지
NoSQL 소개
## Object Storage
- 1mb ~ 1gb 정도의 파일을 저장하는데 적합
- 하지만 파일의 일부를 수시로 변경하는 연산에는 적합하지 않음 -> Object Storage는 파일을 통채로 교체하는 연산에 최적화되있음.
- Object Storage에 저장된 데이터를 집계하기 좋도록 가공하는데 시간이 걸림
- 용도에 맞게 NoSQL을 사용해야함

## CAP
- RDBMS는 ACID(Atomicity, Consistency, Isolation, Durabilty)
- 분산 시스템은 CAP(Consistency, Availability, Partition-Tolerance)
   - 위 세 조건 중 둘만 만족시킬 수 있음
   - 그런데 분산 시스템의 Network는 불안정하기에, Partition tolerance를 반드시 고려해야함. 결국 Consistency, Availability 둘 중 하나만을 선택할 수 밖에 없음
    - 좀 더 정확히 말해, Consistency, Availability는 역의 관계이기에, 적당한 타협이 필요함
    - Eventual Consistency == In-consistency?

## 분산 KVS
- 작은 데이터를 Key와 Value로 구별해 저장함
- 몇 kb의 데이터를 수만TPS로 read/write하는 데이터 저장소
- Sharding : Key를 바탕으로 부하분산. 쉽게 ScaleOut가능함
    - 안정적인 Read/Write성능을 제공하기에, 처음 데이터를 기록하는 장소로 이용됨 
    - 집계 기능은 없지만, 읽기 성능이 높아 쿼리 엔진에서 직접 붙어 사용해도 큰 문제 없음
- Master/Slave vs P2P
    - Replication 관점에서 Master/Slave, P2P를 구별할 수 있음 ( Consistency vs Availabilty)
    - Master/Slave는 Master가 데이터를 받아 Slave로 전파하기 때문에 Data Conflict이 발생하지 않음. 대신 Master가 종료될 경우 I/O가 멈출 수 있음 (예: Aero spike, redis cluster )
    - P2P는 노드들이 대등한 관계. 어떤 노드에 연결해도 I/O에 문제 없음. 대신 Data Conflict이 일어날 가능성이 있음 (예: Amazon DynamoDB, Cassandra)

## Wide column store
- 분산 KVS는 수억개의 Row를 저장할 수 있음
- Wide column store는 수억개의 Column을 저장할 수 있음 
-  Column단위로 데이터를 저장함으로써, Column이 매우 많아도 처리할 수 있음
- Cassandra의 Compound Key
    - 여러 Column에 Primary Key를 걸 수 있음 -> Compund Key
    - Primary Key로 지정된 Column중 첫번째 Column을 Partition Key, 그 외 Column을 Clustering Key라 함

## Document Store
- 하나의 Key Value가 복잡한 Document(보통 JSon) 형태로 저장되며, 이에 대한 쿼리를 지원하는 DB
- MongoDB
    - 빠른 성능, 다양한 기능을 제공하지만 신뢰성이 낮음

## Search Engine
-  텍스트, 스키마리스 데이터를 집계하는데 사용
- 텍스트 검색을 위해 역 색인(Inverted Index) 롤 만듬 -> 키워드 검색이 고속화됨
    - 기술의 발전으로 Index 없이 FullScan을 통해 전문검색 하는 일도 가능해짐
     - 매우 비효율적이지만, 가끔 한다면 문제 없음.
- NoSQL은 삽입 성능을 위해 Index 생성을 제한함. 하지만 검색 엔진은 색인을 적극적으로 만듬
- 실시간 Summary 시스템의 일부로 사용되어, NoSQL에 데이터를 집계하는 한편, 검색엔진을 통해 인덱싱도 동시에 수행하기도 함
- Elasticssearch
    - ELK(Elasticsearch, Logstash, Kibana)로 자주 이용됨
    - Json으로 저장하기에 Document store와 유사하지만, 기본 옵션으로 모든 필드에 색인을 만들기에 삽입 성능이 느림. 명시적으로 스키마를 결정하는 식의 튜닝이 필요함 
    - 쿼리언어가 비교적 복잡해 Kibana등을 사용하는게 편함
- Splunk
   - 상용 검색 엔진
   - 
# Reference
- https://www.thoughtworks.com/insights/blog/nosql-databases-overview
- https://www.elastic.co/kr/blog/elasticsearch-as-a-column-store
