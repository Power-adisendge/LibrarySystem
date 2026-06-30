# 图书馆管理系统

一个基于 C++17 的命令行图书馆管理系统，用面向对象的方式建模图书、读者与借阅业务，支持借阅/归还、多态的库存策略，以及 CSV 持久化。

## 功能概览

- 三类图书：纸质书（真实库存）、电子书（无限借阅）、杂志（仅馆内阅览，不外借）
- 两类读者：学生（借阅上限 5）、教师（借阅上限 20）
- 核心业务：借书、还书，自动维护库存、读者借阅列表与借阅记录
- 持久化：退出自动存盘，启动自动加载，数据以人类可读的 CSV 落盘
- 交互式命令行菜单

## 构建与运行

需要支持 C++17 的编译器（g++）。

```bash
make          # 构建主程序，产出可执行文件 ./library
make run      # 构建并运行
make test     # 构建并依次运行 test/ 下的所有测试
make clean    # 清理 build/ 与可执行文件
```

数据默认存放在项目根目录的 `data/` 下（`books.csv` / `readers.csv` / `records.csv`）。

## 目录结构

```
LibrarySystem/
├── include/              # 头文件（接口）
│   ├── Entity.h          # 实体抽象基类
│   ├── book/             # Book 基类及三个子类
│   ├── reader/           # Reader 基类及两个子类
│   ├── BorrowingRecord.h # 借阅记录
│   ├── LibrarySystem.h   # 系统总调度
│   ├── CsvStorage.h      # 持久化
│   └── LibraryException.h# 异常体系
├── src/                  # 对应实现
├── test/                 # 轻量测试（手写断言框架，无第三方依赖）
├── data/                 # CSV 数据目录
├── main.cpp              # 命令行入口
└── Makefile
```

## 架构设计

### 类层次

```
Entity (抽象)                         统一身份标识 id/name，禁拷贝、可移动
├── Book (抽象)                       作者、出版社 + 借阅策略纯虚接口
│   ├── PhysicalBook                  真实库存，借一本少一本
│   ├── EBook                         无限借阅，记录下载次数
│   └── Magazine                      不外借，isBorrowable() 恒为 false
└── Reader (抽象)                     借阅列表 + 借阅上限纯虚接口
    ├── StudentReader                 上限 5
    └── TeacherReader                 上限 20

LibrarySystem                         总调度：独占所有实体，协调借还，维护记录
CsvStorage                            领域对象 ↔ CSV 的双向转换（静态方法）
BorrowingRecord                       一条借阅记录（谁、借了什么、何时借还）
LibraryException 体系                 BookNotFound / ReaderNotFound
```

### 设计思路

**1. 用多态封装“策略差异”**

不同图书的借阅规则完全不同，但调用方不该关心这些细节。`Book` 把差异点抽象成纯虚接口 `isBorrowable() / borrowOut() / returnBack() / availabilityInfo()`，由子类各自实现：

- `PhysicalBook`：`availableCopies_` 计数，借出递减、归还递增，库存为 0 时不可借
- `EBook`：`isBorrowable()` 恒为 `true`，`borrowOut()` 只累加下载计数
- `Magazine`：`isBorrowable()` 恒为 `false`，从源头上拒绝外借

`LibrarySystem::borrowBook` 只调用 `book->isBorrowable()`，无需 `if (type == ...)` 分支判断。新增图书类型时，系统调度逻辑零改动。读者借阅上限同理，由 `Reader::maxBorrowLimit()` 多态决定。

**2. 单一所有权，集中内存管理**

`Entity` 禁用拷贝构造与拷贝赋值（一本书的实体是唯一的，拷贝没有语义），仅允许移动。`LibrarySystem` 用 `unordered_map<string, unique_ptr<Book>>` 独占所有图书与读者，是系统中唯一负责对象生命周期的地方。`findBook/findReader` 返回裸指针（`Book*`），仅表示“借用、不转移所有权”，找不到时返回 `nullptr` 而不抛异常。

**3. 错误处理双轨制**

把“程序性错误”和“正常业务分支”分开：

- **异常**用于不该发生的情况：借书时实体 ID 不存在，抛 `BookNotFoundException / ReaderNotFoundException`。两者都继承自 `LibraryException`，调用方可以 `catch (const LibraryException&)` 一网打尽。
- **枚举返回值**用于预期内的业务结果：`BorrowStatus`（成功 / 无库存 / 达上限 / 已借过）、`ReturnStatus`（成功 / 未借过）。这些不是错误，是调用方需要据此给用户不同反馈的正常分支。

**4. 借还的两阶段协调与回滚**

`borrowBook` 先让读者侧记账（检查上限、是否重复借），再扣减图书库存。若库存扣减失败，会回滚读者侧的记账，保证两边状态一致。

### 持久化设计

`CsvStorage` 是系统中唯一接触文件的类，提供静态的 `saveXxx / loadXxx`，把领域对象与 CSV 行互相转换。CSV 每行以类型标签开头（如 `PhysicalBook,P1,...`），加载时据此 `make_unique` 出正确的子类。文件不存在时返回空容器（视为“还没有数据”），不抛异常，因此首次运行可以无障碍启动。

**动态状态靠重放记录重建。** CSV 只存图书的静态信息（如纸质书的总册数），不存当前库存。`LibrarySystem::load` 在读入图书、读者、记录后，会遍历每条借阅记录“重放”一遍：对每条记录执行一次 `borrowOut() + reader.borrow()`，若该记录已标记归还，再执行 `returnBack() + reader.returnBook()`。这样库存余量、读者当前借阅列表、电子书下载次数等动态状态都能被完整还原，而不必把它们冗余地写进文件。借阅记录因此成为唯一的事实来源（single source of truth）。

## 命令行使用

`main.cpp` 提供菜单驱动的交互：启动时自动 `load("data")`，选项 0 退出时自动 `save("data")`，管道输入到达 EOF 时也会存盘。借还操作中对 `LibraryException` 做了捕获，ID 不存在会提示而非崩溃。

```
========== 图书馆管理系统 ==========
  1) 添加图书      2) 添加读者
  3) 借书          4) 还书
  5) 列出所有图书  6) 列出所有读者
  7) 查看借阅记录  8) 保存数据
  0) 保存并退出
====================================
```

## 测试

`test/` 下是手写的轻量断言框架（`CHECK` / `CHECK_THROWS_AS`），无第三方依赖，每个 `test_*.cpp` 编译成独立可执行文件：

- `test_library.cpp`：借还全流程、库存增减、上限、重复借、异常等业务逻辑
- `test_storage.cpp`：save/load 往返，验证动态状态被完整还原
- `test_book.cpp` / `test_reader.cpp`：各实体单元行为

`make test` 会依次构建并运行，任一测试返回非零即中断。
