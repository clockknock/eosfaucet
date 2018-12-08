## eosio 水龙头(eosfaucet)合约

> ps:本教程算是进阶教程,需要有一定的eosio合约开发基础.请先参阅官方文档 https://developers.eos.io/ 或 我翻译的过时的[教程](https://github.com/clockknock/EOS-Tutorial)进行基础学习 



我们将要编写一个自己的水龙头合约,通过用户请求,我们将会给用户发送`EOS`,并且请求会有限制,每分钟只能请求一次.

通过本文你可以学到合约中的数据持久化(Data Persistence),内联函数(inline_action)的使用, `asset` 和 `symbol`对象的构建与使用,通过其他合约调用`eosio.token` 的`transfer`.



### 第一步 创建文件夹

进入自己存放合约的文件夹:

```bash
cd /Users/zhong/coding/CLion/contracts
```



创建一个文件夹以存放我们将要编写的合约:

```bash
mkdir eosfaucet
cd eosfaucet

#pwd
/Users/zhong/coding/CLion/contracts/eosfaucet
```





### 第二步 创建并打开一个新文件夹

```bach
touch eosfaucet.cpp
```

然后用你喜欢的编辑器打开它(或打开该文件夹)



### 第三步 编写合约类以及include EOSIO

```c++
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;

CONTRACT eosfaucet : public contract {
public:
    
    eosfaucet(name receiver, name code, datastream<const char *> ds)
        : contract(receiver, code, ds) {}
        
private:

};
```

当使用C++时,第一个需要创建public method应该是constructor,constructor代表初始化该合约.

EOSIO合约继承了 *contract* 类,使用合约的 `code` 以及 `receiver` 来初始化父合约.这里最重要的一个参数是`code`,它是一个区块链上的account,该合约将部署到该account上.



### 第四步 创建表的数据结构

在`private:`的后一行开始加入以下代码:

```c++
TABLE limit {
    name name;
    uint32_t time = 0;

    uint64_t primary_key() const { return name.value; }
};
```

我们的表很简单,就两个字段,一个`name`记录是哪个用户,一个`time`记录时间,使用`name.value`作为主键,因为它的类型是`uint64_t`,所以索引起来会很快.



### 第五步 配置 Multi-Index Table

在刚刚定义好的table后加入以下代码:

```c++
typedef eosio::multi_index<"limit"_n, limit> limit_table;
```

通过这行代码我们能定义好一个名字为`limit_table` 的 multi-index table.

`eosio::multi_index<>` 中的第一个参数`"limit"_n`是表的名字,`_n`是一个宏,表的名称不能有下划线而且好像有字符长度限制,可以自己把表名写长一些看看报什么错.

`eosio::multi_index<>` 中的第二个参数`limit`是我们这张表存储的`row`的类型

`limit_table`是我们这张表的定义



目前来说我们`eosfaucet.cpp`中的代码是这样的:

```c++
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;

CONTRACT eosfaucet : public contract {
public:

    eosfaucet(name receiver, name code, datastream<const char *> ds)
        : contract(receiver, code, ds) {}

private:
    TABLE limit {
        name name;
        uint32_t time = 0;

        uint64_t primary_key() const { return name.value; }
    };
    typedef eosio::multi_index<"limit"_n, limit> limit_table;

};
```



### 第六步 初始化 multi-index table

在刚刚定义的`limit_table`后进行一个对象声明:

```c++
...
limit_table _limit_table;
```



然后回到constructor进行初始化:

```c++
  eosfaucet(name receiver, name code, datastream<const char *> ds)
        : contract(receiver, code, ds), _limit_table(_code, _code.value) {}
```

到这里为止,eos合约中的multi-index table的存储范围还需要解释说明一下,它的构造函数(`multi_index( name code, uint64_t scope )`)需要传入两个参数,`code` 和` scope`:

> `code`是指该表属于哪个account
>
> `scope`是指代码层次结构中的范围标识符

那回到我们的构造函数后面`_limit_table(_code, _code.value)`,我们构造`limit_table`时决定了该表属于`_code`,当前的`_code`是`eosfaucet`, `_code.value` 是`eosfaucet`的uint64_t值.



为什么需要知道这两个参数的含义?因为在查找表内容的时候需要指定他们,比如通过`cleos`命令:

```bash
cleos get table [OPTIONS] account scope table
```

命令中的`account`就是`code`,`scope`就是`scope`, `table` 是先前声明好的,例如我们例子中的`"limit"_n`,那么如果我们想通过`cleos`查找该表,就使用:

```bash
cleos get table eosfaucet eosfaucet limit
```



### 第七步 添加数据到表中

在构造函数后声明一个action:

```c++
ACTION get(name user) {

}
```

该action接受一个参数:`user` ,我们将通过该参数来查找`limit`表,以确认是否可以给该用户发钱.



```c++
ACTION get(name user) {
    auto iterator = _limit_table.find(user.value);

    if (iterator != _limit_table.end()) {
        //用户在表里

    } else {
        //用户没在表里


    }
}
```

在表中查找该用户,获取迭代器`_limit_table.find(user.value)`, 如果`iterator`不等于`_limit_table.end()`则说明该用户之前通过该函数取过钱,`_limit_table.end()`代表比该table最后一行记录还大一条的迭代器,也就是说该行是没存记录的,也就是没有通过该值找到迭代器.

我们先处理else的情况:

```c++
//用户没在表里
_limit_table.emplace(get_self(), [&](auto & row) {
    row.name = user;
    row.time = now() + 60;

    //给用户发送EOS
    
});
```

如果是else,说明用户没领过钱,我们使用emplace添加到表中.

`emplace`的第一个参数是`payer`,它是指这条记录的`RAM`花费将由谁来支付,我们使用`get_self()`,代表由合约本身的account来支付.

记录的数据需要将用户记录上,并将时间记录上,因为我们不允许用户太频繁的从该合约取钱,限制为一分钟一次.

`now()`取出来的值是以秒为单位的`uint64_t`,我们将其加60则是指多一分钟.

到这位置我们先编译合约来跑一跑吧!



### 第八步 使用DISPATCH

在文件末尾加上:

```c++
EOSIO_DISPATCH( eosfaucet, (get) )
```

第一个参数是当前合约的名字,第二个参数是action列表

我们的合约现在是这样的:

```c++
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;

CONTRACT eosfaucet : public contract {
public:

    eosfaucet(name receiver, name code, datastream<const char *> ds)
            : contract(receiver, code, ds), _limit_table(_code, _code.value) {}

    ACTION get(name user) {
        auto iterator = _limit_table.find(user.value);

        if (iterator != _limit_table.end()) {
            //用户在表里

        } else {
            //用户没在表里
            _limit_table.emplace(get_self(), [&](auto & row) {
                row.name = user;
                row.time = now() + 60;

                //给用户发送EOS
//                sendEOS(user);
            });
        }
    }

private:
    TABLE limit {
        name name;
        uint32_t time = 0;

        uint64_t primary_key() const { return name.value; }
    };

    typedef eosio::multi_index<"limit"_n, limit> limit_table;
    limit_table _limit_table;

};

EOSIO_DISPATCH( eosfaucet, (get) )
```



### 第九步 创建合约account,编译,部署

* 创建account

```bash
cleos create account eosio eosfaucet EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
```

response:

```bash
executed transaction: 2fa15186334b6a8792bd5df708c68daa5dbbedd1cf42fe750229a1b442a352a4  200 bytes  391 us
#         eosio <= eosio::newaccount            {"creator":"eosio","name":"eosfaucet","owner":{"threshold":1,"keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnV...
warning: transaction executed locally, but may not be confirmed by the network yet    ] 
```



我使用的是自己的测试网络,我通过`eosio`这一account来创建新的account,创建的account name是`eosfaucet`,将会持有该account的publicKey为`EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV`.



* 编译合约

```bash
#进入eosfaucet.cpp的文件夹
cd /Users/zhong/coding/CLion/contracts/eosfaucet
#编译
eosio-cpp -o eosfaucet.wasm eosfaucet.cpp --abigen
```

我们使用`eosio-cpp`来编译`eosfaucet.cpp`文件,`-o eosfaucet.wasm`声明输出的内容写到`eosfaucet.wasm`中,

`--abigen`同时告诉`eosio-cpp`我们还需要生成abi文件.如果一步一步跟着来应该不会报错,没报错就可以部署合约了.



* 部署合约

```bash
cleos set contract eosfaucet /Users/zhong/coding/CLion/contracts/eosfaucet eosfaucet.wasm eosfaucet.abi
```

> 如果提示你的钱包未解锁请先解锁

response:

```bash
Reading WASM from /Users/zhong/coding/CLion/contracts/eosfaucet/eosfaucet.wasm...
Publishing contract...
executed transaction: 8e6a7e923a1c9ffcb0037b974b2a5803a69f5f62896546bcdf0d18873bf19a5b  2768 bytes  652 us
#         eosio <= eosio::setcode               {"account":"eosfaucet","vmtype":0,"vmversion":0,"code":"0061736d0100000001590f60027f7e00600000600001...
#         eosio <= eosio::setabi                {"account":"eosfaucet","abi":"0e656f73696f3a3a6162692f312e3100020367657400010475736572046e616d65056c...
warning: transaction executed locally, but may not be confirmed by the network yet    ] 
```



### 第十步 调用 action

```bash
cleos push action eosfaucet get '["alice"]' -p alice@active
```

我们将要调用`eosfaucet`的action,调用名称为`get`的action,我们传的参数是`["alice"]`,使用的用户权限是`alice@active`

response:

```bash
eosfaucet <= eosfaucet::get               {"user":"alice"}
```

能看到这行说明我们调用已经成功了



### 第十一步 让合约能transfer EOS

现在我们实现以下在`get`函数中还没完成的`sendEOS`函数.

我们先在合约声明上定义`symbol`对象:

```c++
#define EOS_SYMBOL symbol("EOS",4)
```

我们定义了一个`EOS_SYMBOL`为`symbol("EOS",4)`,`"EOS"`是该symbol的名称,`4`是该symbol的小数精度,例如我们定义好的symbol就会是`1.0000 EOS`.

然后在`private:`后添加以下代码:

```c++
private:

    void sendEOS(name user){
        asset money = asset(10, EOS_SYMBOL);
        action(
                permission_level{get_self(), "active"_n},
                "eosio.token"_n,
                "transfer"_n,
                std::make_tuple(get_self(), user, money, std::string("memo"))
        ).send();
    }
```

我们先使用定义好的`EOS_SYMBOL`构建一个`asset`对象,` asset(10, EOS_SYMBOL)`中的`10`代表该asset的大小,将其除以精度的位数就能得到asset的大小,例如我们构造的`money`的结果是`0.0010 EOS`.

然后定义action:

```
 permission_level{get_self(), "active"_n},
                "eosio.token"_n,
                "transfer"_n,
                std::make_tuple(get_self(), user, money, std::string("memo"))
```

> permission_level{get_self(), "active"_n}  使用自己的account,权限是active
>
>
> "eosio.token"_n 调用的合约是eosio.token
>
> "transfer"_n  调用的函数是transfer
> std::make_tuple(get_self(), user, money, std::string("get eos from faucet"))  transfer需要的四个参数



回到`get`函数中将注释打开:

```c++
...
    else {
            //用户没在表里
            _limit_table.emplace(get_self(), [&](auto & row) {
                row.name = user;
                row.time = now() + 60;

                //给用户发送EOS
                sendEOS(user);
            });
        }
```



### 第十二步 再次编译合约,部署及调用get函数

```bash
eosio-cpp -o eosfaucet.wasm eosfaucet.cpp --abigen

cleos set contract eosfaucet /Users/zhong/coding/CLion/contracts/eosfaucet eosfaucet.wasm eosfaucet.abi
```

重新部署好eosfaucet合约后我们还需要一步,就是先给这个合约一些EOS,不然它也没法给其他人转钱.给它转钱的命令我就不写了,你应该是会的.



由于我们使用过alice来调用get,表里存在她的记录,所以我们需要换一个账号来调用.

```bash
cleos push action eosfaucet get '["bob"]' -p bob@active
```

response:

```bash
Error 3090003: Provided keys, permissions, and delays do not satisfy declared authorizations
Ensure that you have the related private keys inside your wallet and your wallet is unlocked.
Error Details:
transaction declares authority '{"actor":"eosfaucet","permission":"active"}', but does not have signatures for it under a provided delay of 0 ms, provided permissions [{"actor":"eosfaucet","permission":"eosio.code"}], provided keys [], and a delay max limit of 3888000000 ms
```

你会看到如上所示的错误,这是因为在eos中,一个合约不能直接调用其他合约的action,它缺少permission.如果想要调用其他合约的action,我们需要通过以下命令给它权限:

```bash
cleos set account permission eosfaucet active '{"threshold": 1,"keys": [{"key": "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","weight": 1}], "accounts": [{"permission":{"actor":"eosfaucet","permission":"eosio.code"},"weight":1}]}' -p eosfaucet@owner
```

这行命令的意思是我们通过` -p eosfaucet@owner`给`eosfaucet active`一定的权限,什么权限呢,`{"permission":{"actor":"eosfaucet","permission":"eosio.code"},"weight":1}`让eosfaucet拥有`eosio.code`的权限.有了这个权限,才能在合约内调用其他合约的内联函数.

再次调用:

```bash
cleos push action eosfaucet get '["bob"]' -p bob@active
```

response:

```bash
zhong:eosfaucet zhong$ cleos push action eosfaucet get '["bob"]' -p bob@active
executed transaction: 199157afa21e65bf5702b235db673c44eb14690ce8338f1e2db70661acd34b95  104 bytes  526 us
#     eosfaucet <= eosfaucet::get               {"user":"bob"}
#   eosio.token <= eosio.token::transfer        {"from":"eosfaucet","to":"bob","quantity":"0.0010 EOS","memo":""}
#     eosfaucet <= eosio.token::transfer        {"from":"eosfaucet","to":"bob","quantity":"0.0010 EOS","memo":""}
#           bob <= eosio.token::transfer        {"from":"eosfaucet","to":"bob","quantity":"0.0010 EOS","memo":""}
```

现在我们就能从eosfaucet中获取到eos了



### 第十三步 完善get action

```c++
 ACTION get(name user) {
     auto iterator = _limit_table.find(user.value);

     if (iterator != _limit_table.end()) {
         //用户在表里
         auto find = _limit_table.get(user.value);

         //判断等待时间
         eosio_assert(find.time < now(), "you can not get EOS yet");

         _limit_table.modify(iterator, get_self(), [&](auto & row) {
             row.time = now() + 60;
             sendEOS(user);
         });
     }
```

回到`if`判断中,先通过` _limit_table.get(user.value)`获取row对象,即我们定义好的`limit` Table.判断该row的time是否小于当前时间,如果小于,才能给他发EOS.

由于又一次的领取,我们需要修改表中的数据,modify需要三个参数,第一个参数是将要修改的iterator,第二个参数是RAM的payer,第三个参数是修改数据的回调函数.

由于我们的primary_key不用变,只需要将它的领取时间再添加一分钟即可.



### 第十四步 再次编译合约,部署及调用get函数

```bash
eosio-cpp -o eosfaucet.wasm eosfaucet.cpp --abigen

cleos set contract eosfaucet /Users/zhong/coding/CLion/contracts/eosfaucet eosfaucet.wasm eosfaucet.abi
```

连续调用get函数:

```bash
cleos push action eosfaucet get '["bob"]' -p bob@active
```

第一次会获取成功,但第二次会得到Error(assert的详细提醒需要在打开nodeos的时候添加`--verbose-http-errors`指令):

```bash
Error 3050003: eosio_assert_message assertion failure
Error Details:
assertion failure with message: you can not get EOS yet
```



到此,我们的水龙头合约就简单的完成了