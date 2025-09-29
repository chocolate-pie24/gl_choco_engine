# TODO

- [] linear_allocatorをmemory_systemに統合する
- [] makefileのワーニングルールを一旦緩める
- [] テスト時には別バイナリの作成
- [] サニタイザ
- [] clang-tidy
- [] memory_system_allocateで返すポインタの前にヘッダを付加してsize,tagを格納、freeはヘッダを参照(引数のtagも不要になる)(これでfreeで間違ったtagを指定して破綻するのを防ぐ)
