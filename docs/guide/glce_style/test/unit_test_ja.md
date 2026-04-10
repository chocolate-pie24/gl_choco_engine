# GLCE 単体テストコード実装ガイド

本ドキュメントでは、モジュールが保有する外部公開APIおよびプライベート関数の単体テスト実装方法について記す。

## テスト用失敗注入機能

テストの際には、テスト対象関数内で強制的に指定した返り値を出力させる機能が必要となる。
GLCEでは、この機能を全モジュールで統一感を持って使用できるよう、テストビルド時のみ、外部からの失敗注入機能を用意する。

この機能は、test/include/test_controller.hで定義され、test/src/test_controller.cで実装されている。

test_controller.hでは外部モジュールに対して以下を公開する。失敗注入構造体は、テスト対象関数の返り値ごとに用意する。
ただし、返り値がモジュール固有の型で、共通失敗注入構造体で表現できない返り値型に対して構造体を定義してしまうと
モジュールへの依存が発生するため、この失敗注入の仕組みを使ってはならない。モジュール内で個別に対応すること。

```c
/**
 * @brief 各種APIのテスト用失敗注入構造体(返り値が実行結果コード or intの関数用)
 *
 */
typedef struct test_call_control {
    uint32_t call_count;    /**< API呼び出し回数 */
    uint32_t fail_on_call;  /**< APIを何回目の呼び出しでエラーにさせるかの設定(0なら無効で通常処理、1以上の場合はforced_resultを出力させる) */
    int forced_result;      /**< APIを強制的に失敗させる際の戻り値(各種実行結果コードをintにキャストして使用する) */
} test_call_control_t;

/**
 * @brief 各種APIのテスト用失敗注入構造体(返り値がsize_tの関数用)
 *
 */
typedef struct test_call_control_size_t {
    uint32_t call_count;    /**< API呼び出し回数 */
    uint32_t fail_on_call;  /**< APIを何回目の呼び出しでエラーにさせるかの設定(0なら無効で通常処理、1以上の場合はforced_resultを出力させる) */
    size_t forced_result;   /**< APIを強制的に失敗させる際の戻り値 */
} test_call_control_size_t_t;

/**
 * @brief 各種APIのテスト用失敗注入構造体(返り値がboolの関数用)
 *
 */
typedef struct test_call_control_bool {
    uint32_t call_count;    /**< API呼び出し回数 */
    uint32_t fail_on_call;  /**< APIを何回目の呼び出しでエラーにさせるかの設定(0なら無効で通常処理、1以上の場合はforced_resultを出力させる) */
    bool forced_result;   /**< APIを強制的に失敗させる際の戻り値 */
} test_call_control_bool_t;

/**
 * @brief 失敗注入構造体のフィールドを失敗注入なしにリセットする
 *
 * @details リセット後は構造体のフィールドは以下の値になる
 * - call_count = 0
 * - fail_on_call = 0
 * - forced_result = 0(多くの場合SUCCESS)
 *
 * @param[out] control_ リセット対象構造体へのポインタ
 */
void test_call_control_reset(test_call_control_t* control_);

/**
 * @brief 失敗注入構造体のフィールドを失敗注入なしにリセットする
 *
 * @details リセット後は構造体のフィールドは以下の値になる
 * - call_count = 0
 * - fail_on_call = 0
 * - forced_result = 0
 *
 * @param[out] control_ リセット対象構造体へのポインタ
 */
void test_call_control_size_t_reset(test_call_control_size_t_t* control_);

/**
 * @brief 失敗注入構造体のフィールドを失敗注入なしにリセットする
 *
 * @details リセット後は構造体のフィールドは以下の値になる
 * - call_count = 0
 * - fail_on_call = 0
 * - forced_result = false
 *
 * @param[out] control_ リセット対象構造体へのポインタ
 */
void test_call_control_bool_reset(test_call_control_bool_t* control_);
```

## 単体テストコード作成手順

### テスト用ヘッダファイルの作成

単体テスト作成対象モジュールのテスト用ヘッダファイルを以下のルールに基づいて作成する。

- ヘッダファイルを格納するディレクトリは、テスト対象モジュールの階層と対称になるように設定する。
- テスト用ヘッダファイル名は、test_module_name.hとする。

例えばテスト対象がinclude/engine/core/memory/choco_memory.hであれば、test/include/engine/core/memory/test_choco_memory.hとする。

ヘッダファイル内に記載する内容は以下の内容とする。

#### 失敗注入API

***test_controller.h*** で公開している型による失敗注入が可能なモジュール保有外部公開APIに、失敗注入APIを個別に設ける。
失敗注入APIの名称は、***test_foo_config_set*** とする。fooは外部公開API名称を表す。

このAPIは以下のように使用する。
なお、構造体のcall_countフィールドについては、モジュール内で管理するため、失敗を注入する側では値のセットは行わない。

```c
test_call_control_t config = { 0 };
test_call_control_reset(&config);

config.fail_on_call = 1;                // 1回目の呼び出しで失敗させる
config.forced_result = (int)xxx_result; // xxxモジュールの実行結果コードでAPI fooに強制的に出力させる

test_foo_config_set(&config);   // API fooに対して失敗注入
```

***test_foo_config_set*** は以下のように実装する。

```c
void test_foo_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_foo.fail_on_call = config_->fail_on_call;
    s_test_config_foo.forced_result = config_->forced_result;
}
```

#### モジュールテストAPI

モジュールが保有する全外部公開APIと、プライベート関数のテストを行うAPIを定義する。
API名称は、test_xxxとする。xxxはモジュール名称を表す。

#### 失敗注入設定のリセット

モジュールが保有する全失敗注入設定をリセットするAPIを定義する。
リセットAPIの名称は、test_module_name_config_resetとする。
なお、test_module_name_config_resetは、失敗注入設定以外にもモック関数テスト設定やテストシナリオ設定等の設定値も全てリセットされる。

以上を記載したテスト用ヘッダは以下のようになる。

```c
#ifndef GLCE_TEST_PATH_MODULE_NAME_H
#define GLCE_TEST_PATH_MODULE_NAME_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TEST_BUILD
#include "test_controller.h"    // テスト基盤ヘッダのinclude

// 外部公開APIへの失敗注入API(test_controller.h で提供している失敗注入型で対応可能な全外部公開API分用意する)
void test_foo_config_set(const test_call_control_t* config_);   // API fooの失敗注入設定API
void test_bar_config_set(const test_call_control_t* config_);   // API barの失敗注入設定API

// モジュール内全テスト設定値リセット処理(テスト用処理を無効化する, 失敗注入以外のモック関数テスト設定やテストシナリオ設定等の設定値も全てリセットされる)
void test_module_name_config_reset(void);

// モジュールが保有する全関数(外部公開API, プライベート関数)のテスト処理
void test_module_name(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
```

### テスト用実装の追加

テストに使用する各処理は、テスト対象モジュールの実装ファイルに記載する。
こうすることで、モジュールが使用する各種構造体のフィールドに直接アクセスできるようにし、テストコードをシンプルにする。

#### テスト用データ構造定義、プロトタイプ宣言部

モジュール実装ファイルの各種ヘッダinclude以下にTEST_BUILD用のコードブロックを用意し、以下を記載する。
全テスト関数で統一感を持たせるため、記載する順番は全モジュールで統一する。

- テスト時のみ使用するヘッダのinclude
- 失敗注入機能を有する外部公開APIについて、失敗注入構造体インスタンス宣言(インスタンス名称はs_test_config_xxxとする(xxxは外部公開API名称))。
- (必要であれば)モジュール内プライベート関数のうち、失敗注入機能を入れた関数の失敗注入構造体インスタンス宣言(インスタンス名称はs_test_config_xxxとする(xxxはプライベート関数名称))。
- 全テスト関数プロトタイプ宣言(テスト関数名称はtest_xxxとする。xxxはテスト対象関数名称を表す)。
- プロトタイプ宣言の順番は各テスト対象関数の実装順と同じにすること。

各テスト関数は、モジュールが保有する外部公開API, プライベート関数の実装の後に追加する。各テスト関数の実装順は、以下の通り。

- 全テスト関数を実行するテスト関数(テスト用ヘッダに記載した関数)
- 各関数個別の単体テスト関数(プロトタイプ宣言と同じ順番で実装する)

```c
#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h> // for memset strcmp
#include "xxx/test_xxx.h"   // モジュールxxx用テストヘッダ

// 外部公開APIテスト設定
static test_call_control_t s_test_config_foo;    /**< API fooテスト設定 */
static test_call_control_t s_test_config_bar;    /**< API barテスト設定 */

// プライベート関数テスト設定(必要であれば構造体インスタンス宣言を追加する)
static test_call_control_t s_test_config_baz;    /**< プライベート関数bazテスト設定値 */

// 全テスト関数プロトタイプ宣言(モジュールテスト関数内でこれらが呼ばれる。モジュールテスト関数の名称はtest_module_name)
static void test_xxx(void); /**< 各テスト対象関数のテスト関数でxxxには関数名称が入る */
#endif
```

### 外部公開API実装時の失敗注入処理

上位レイヤーからの呼び出しに対し、返り値を制御可能にするため、共通失敗注入機構またはモジュール個別の失敗注入機構で対応する全外部公開APIには、強制的に値を指定した返り値出力機能を設ける。
強制的な出力は、あらゆるエラー処理に関わらず機能させたいため、関数の先頭に配置する。例えば以下のような形にする(memory_system_allocateの例)。

```c
memory_system_result_t memory_system_allocate(size_t size_, memory_tag_t mem_tag_, void** out_ptr_) {
#ifdef TEST_BUILD
    s_test_config_memory_system_allocate.call_count++;
    if(s_test_config_memory_system_allocate.fail_on_call != 0) {
        if(s_test_config_memory_system_allocate.call_count == s_test_config_memory_system_allocate.fail_on_call) {
            return (memory_system_result_t)s_test_config_memory_system_allocate.forced_result;
        }
    }
#endif
    memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    void* tmp = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(s_mem_sys_ptr, ret, MEMORY_SYSTEM_BAD_OPERATION, rslt_to_str(MEMORY_SYSTEM_BAD_OPERATION), "memory_system_allocate", "s_mem_sys_ptr")
    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, MEMORY_SYSTEM_INVALID_ARGUMENT, rslt_to_str(MEMORY_SYSTEM_INVALID_ARGUMENT), "memory_system_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, MEMORY_SYSTEM_INVALID_ARGUMENT, rslt_to_str(MEMORY_SYSTEM_INVALID_ARGUMENT), "memory_system_allocate", "*out_ptr_")
    IF_ARG_FALSE_GOTO_CLEANUP(mem_tag_ < MEMORY_TAG_MAX, ret, MEMORY_SYSTEM_INVALID_ARGUMENT, rslt_to_str(MEMORY_SYSTEM_INVALID_ARGUMENT), "memory_system_allocate", "mem_tag_")
    if(0 == size_) {
        WARN_MESSAGE("memory_system_allocate - No-op: size_ is 0.");
        ret = MEMORY_SYSTEM_SUCCESS;
        goto cleanup;
    }

    // 途中省略

    ret = MEMORY_SYSTEM_SUCCESS;

cleanup:
    return ret;
}
```
